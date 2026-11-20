#include "DeckEngine.h"
#include <cmath>

namespace ts
{

namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double edgeFadeFrames = 64.0;

inline double onePole (double seconds, double rate)
{
    return 1.0 - std::exp (-1.0 / (seconds * rate));
}

inline float hermite (float xm1, float x0, float x1, float x2, float t) noexcept
{
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}
} // namespace

void DeckEngine::prepare (double hostSampleRate)
{
    hostRate = hostSampleRate > 0 ? hostSampleRate : 44100.0;

    fadeLen = juce::jmax (16, (int) std::lround (0.005 * hostRate));    // 5 ms jump crossfade
    grainLen = juce::jmax (64, (int) std::lround (0.040 * hostRate));   // 40 ms keep-pitch grains
    releaseLen = juce::jmax (16, (int) std::lround (0.080 * hostRate)); // 80 ms scratch release ramp

    // Hand follower: critically damped, ~16 ms position lag.
    handK1 = 1.0 / (0.016 * hostRate);
    handK2 = onePole (0.004, hostRate);

    tempoCoef = onePole (0.030, hostRate);
    smoothCoef = onePole (0.020, hostRate);
    fastCoef = onePole (0.003, hostRate);
    dcR = (float) (1.0 - 2.0 * pi * 5.0 / hostRate);

    for (int c = 0; c < 2; ++c)
        dcX[c] = dcY[c] = ic1[c] = ic2[c] = 0.0f;

    main.fadeLeft = wet.fadeLeft = 0;
    if (logCutoff == 0.0)
        logCutoff = std::log (20000.0);
}

void DeckEngine::setSample (const SampleData* s) noexcept
{
    if (s == sample)
        return;

    sample = s;
    main = Head {};
    wet = Head {};
    loopOn = false;
    loopIn = loopOut = 0.0;
    cue = 0.0;
    repeatActive = -1;
    repeatHeldMask = 0;
    repGain = 0.0f;
    scratching = false;
    releaseLeft = 0;
    fadeIn = 0.0f; // short fade so a mid-play swap does not click
}

double DeckEngine::clampPos (double p) const noexcept
{
    if (sample == nullptr)
        return 0.0;
    return juce::jlimit (0.0, (double) sample->numFrames(), p);
}

double DeckEngine::tempoTarget (const EngineParams& p) const noexcept
{
    if (! p.sync || p.sampleBpm < 1.0 || p.effectiveBpm < 1.0)
        return 1.0;
    return juce::jlimit (0.25, 4.0, p.effectiveBpm / p.sampleBpm);
}

void DeckEngine::read (double pos, float& l, float& r) const noexcept
{
    l = r = 0.0f;
    if (sample == nullptr)
        return;

    const int n = sample->numFrames();
    if (n < 4 || pos <= 0.0 || pos >= (double) n)
        return;

    const double fl = std::floor (pos);
    const int i = (int) fl;
    const float t = (float) (pos - fl);
    const float* dl = sample->audio.getReadPointer (0);
    const float* dr = sample->audio.getReadPointer (1);

    if (i >= 1 && i + 2 < n)
    {
        l = hermite (dl[i - 1], dl[i], dl[i + 1], dl[i + 2], t);
        r = hermite (dr[i - 1], dr[i], dr[i + 1], dr[i + 2], t);
    }
    else
    {
        auto at = [n] (const float* d, int k) { return (k >= 0 && k < n) ? d[k] : 0.0f; };
        l = hermite (at (dl, i - 1), at (dl, i), at (dl, i + 1), at (dl, i + 2), t);
        r = hermite (at (dr, i - 1), at (dr, i), at (dr, i + 1), at (dr, i + 2), t);
    }

    // Fade the very start and end so running off the record never clicks.
    const double edge = juce::jmin (pos, (double) n - pos);
    if (edge < edgeFadeFrames)
    {
        const float g = (float) (edge / edgeFadeFrames);
        l *= g;
        r *= g;
    }
}

void DeckEngine::jump (Head& h, double newPos) noexcept
{
    h.fadePos = h.pos;
    h.fadeLeft = fadeLen;
    h.pos = clampPos (newPos);
}

void DeckEngine::renderHead (Head& h, double inc, double readInc, float kp, float& l, float& r) noexcept
{
    float vl, vr;
    read (h.pos, vl, vr);

    if (h.fadeLeft > 0)
    {
        float ol, orr;
        read (h.fadePos, ol, orr);
        const float t = 1.0f - (float) h.fadeLeft / (float) fadeLen;
        const float wNew = 0.5f - 0.5f * std::cos ((float) pi * t);
        vl = vl * wNew + ol * (1.0f - wNew);
        vr = vr * wNew + orr * (1.0f - wNew);
        h.fadePos += inc;
        --h.fadeLeft;
    }

    if (kp > 0.0001f)
    {
        // Two overlapping Hann grains. Each grain starts at the timeline
        // position and reads at the pitch rate, so tempo and pitch separate.
        float gl = 0.0f, gr = 0.0f;
        for (auto& g : h.grains)
        {
            if (g.phase >= grainLen)
            {
                g.phase = 0;
                g.readPos = h.pos;
            }
            const float w = 0.5f - 0.5f * std::cos (2.0f * (float) pi * (float) g.phase / (float) grainLen);
            float a, b;
            read (g.readPos, a, b);
            gl += a * w;
            gr += b * w;
            g.readPos += readInc;
            ++g.phase;
        }
        vl = vl * (1.0f - kp) + gl * kp;
        vr = vr * (1.0f - kp) + gr * kp;
    }
    else
    {
        // Keep grains aligned with the playhead so switching on is seamless.
        h.grains[0] = { h.pos, 0 };
        h.grains[1] = { h.pos, grainLen / 2 };
    }

    l = vl;
    r = vr;
}

void DeckEngine::advanceMain (double inc) noexcept
{
    const double prev = main.pos;
    main.pos += inc;

    if (loopOn && ! scratching && loopOut - loopIn > 16.0)
    {
        const double len = loopOut - loopIn;
        if (inc > 0.0 && prev < loopOut && main.pos >= loopOut && prev >= loopIn - 1.0)
            jump (main, loopIn + std::fmod (main.pos - loopOut, len));
        else if (inc < 0.0 && prev >= loopIn && main.pos < loopIn && prev <= loopOut + 1.0)
            jump (main, loopOut - std::fmod (loopIn - main.pos, len));
    }

    const double n = sample != nullptr ? (double) sample->numFrames() : 0.0;
    if (main.pos >= n)
    {
        main.pos = n;
        if (scratching)
            handVel = juce::jmin (handVel, 0.0);
        else if (inc > 0.0)
        {
            // Ran off the end of the record.
            playing = false;
            motor = 0.0;
            releaseLeft = 0;
        }
    }
    else if (main.pos < 0.0)
    {
        main.pos = 0.0;
        if (scratching)
            handVel = juce::jmax (handVel, 0.0);
    }
}

void DeckEngine::advanceWet (double inc) noexcept
{
    wet.pos += inc;
    const double len = repEnd - repStart;
    if (len < 16.0)
        return;
    if (wet.pos >= repEnd)
        jump (wet, wet.pos - len);
    else if (wet.pos < repStart)
        jump (wet, wet.pos + len);
}

void DeckEngine::startRepeat (int index, const EngineParams& p) noexcept
{
    if (sample == nullptr)
        return;

    // A slice of 1/N bar (4/4) at the current BPM, in source frames.
    const double bpm = (p.sync && p.sampleBpm >= 1.0) ? p.sampleBpm : juce::jmax (1.0, p.effectiveBpm);
    const double len = repeatBeats (index) * 60.0 / bpm * sample->sampleRate;
    const bool backwards = lastSpeed < 0.0;

    repStart = backwards ? main.pos : main.pos - len;
    repStart = juce::jlimit (0.0, juce::jmax (0.0, (double) sample->numFrames() - len), repStart);
    repEnd = repStart + len;

    if (repGain < 0.001f)
    {
        wet = Head {};
        wet.pos = backwards ? repEnd : repStart;
    }
    else
        jump (wet, backwards ? repEnd : repStart);

    repeatActive = index;
}

void DeckEngine::handle (const Command& c, const EngineParams& p) noexcept
{
    const double n = sample != nullptr ? (double) sample->numFrames() : 0.0;

    switch (c.type)
    {
        case Cmd::TogglePlay:
            if (! playing && main.pos >= n - 1.0)
                jump (main, cue);
            playing = ! playing;
            break;

        case Cmd::Play:
            if (main.pos >= n - 1.0)
                jump (main, cue);
            playing = true;
            break;

        case Cmd::Pause:
            playing = false;
            break;

        case Cmd::HardRestart:
            playing = true;
            motor = 1.0;
            releaseLeft = 0;
            jump (main, cue);
            break;

        case Cmd::TapeStopDown: tapeHeld = true; break;
        case Cmd::TapeStopUp:   tapeHeld = false; break;

        case Cmd::RepeatDown:
            if (c.index >= 0 && c.index < numRepeatRates)
            {
                repeatHeldMask |= (1u << c.index);
                startRepeat (c.index, p);
            }
            break;

        case Cmd::RepeatUp:
            if (c.index >= 0 && c.index < numRepeatRates)
            {
                repeatHeldMask &= ~(1u << c.index);
                if (repeatActive == c.index)
                {
                    repeatActive = -1;
                    for (int i = 0; i < numRepeatRates; ++i)
                        if ((repeatHeldMask & (1u << i)) != 0)
                        {
                            startRepeat (i, p);
                            break;
                        }
                }
            }
            break;

        case Cmd::ScratchBegin:
            if (! scratching)
            {
                scratching = true;
                scratchAnchor = main.pos;
                handVel = lastSpeed * (sample != nullptr ? sample->sampleRate / hostRate : 1.0);
                releaseLeft = 0;
            }
            break;

        case Cmd::ScratchEnd:
            if (scratching)
            {
                scratching = false;
                releaseVel = handVel;
                releaseLeft = releaseLen;
            }
            break;

        case Cmd::SetLoop:
            loopIn = clampPos (juce::jmin (c.a, c.b));
            loopOut = clampPos (juce::jmax (c.a, c.b));
            loopOn = c.flag && loopOut - loopIn > 16.0;
            break;

        case Cmd::Jump:
            jump (main, c.a);
            break;

        case Cmd::SetCue:
            cue = clampPos (c.a);
            break;
    }
}

void DeckEngine::process (float* left, float* right, int n, const EngineParams& p, double scratchOffset) noexcept
{
    peakL = peakR = 0.0f;

    if (sample == nullptr || sample->numFrames() < 4)
    {
        for (int i = 0; i < n; ++i)
        {
            left[i] = 0.0f;
            if (right != nullptr)
                right[i] = 0.0f;
        }
        lastSpeed = 0.0;
        return;
    }

    const double srRatio = sample->sampleRate / hostRate;
    const double tTarget = tempoTarget (p);
    if (! tempoInitialised)
    {
        tempo = tTarget;
        tempoInitialised = true;
    }

    const double motorStep = 1.0 / (juce::jlimit (0.05, 10.0, (double) p.tapeStopSeconds) * hostRate);
    const double semitoneRatio = p.keepPitch ? std::pow (2.0, juce::jlimit (-24.0, 24.0, (double) p.pitchSemitones) / 12.0) : 1.0;
    const double scratchTarget = scratchAnchor + scratchOffset;
    const double maxHand = 30.0 * srRatio;

    const double nyquistSafe = juce::jmin (20000.0, 0.45 * hostRate);
    const double targetLogCutoff = std::log (juce::jlimit (20.0, nyquistSafe, (double) p.cutoffHz));
    const float targetRes = juce::jlimit (0.0f, 1.0f, p.resonance);
    const float targetFilterMix = p.filterBypass ? 0.0f : 1.0f;
    const float targetGain = juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 24.0f, p.gainDb));
    const float targetMix = juce::jlimit (0.0f, 1.0f, p.repeaterMix);
    const float sc = (float) smoothCoef;
    const float fc = (float) fastCoef;
    const float fadeStep = 1.0f / (float) fadeLen;

    double inc = 0.0;

    for (int i = 0; i < n; ++i)
    {
        tempo += (tTarget - tempo) * tempoCoef;

        // Motor: linear ramps give the tape-stop / spin-up curve.
        const double motorTarget = (playing && ! tapeHeld) ? 1.0 : 0.0;
        if (motor < motorTarget)
            motor = juce::jmin (motorTarget, motor + motorStep);
        else if (motor > motorTarget)
            motor = juce::jmax (motorTarget, motor - motorStep);

        const double motorInc = srRatio * tempo * motor;

        if (scratching)
        {
            const double desired = (scratchTarget - main.pos) * handK1;
            handVel += (desired - handVel) * handK2;
            handVel = juce::jlimit (-maxHand, maxHand, handVel);
            inc = handVel;
        }
        else if (releaseLeft > 0)
        {
            const double t = (double) releaseLeft / (double) releaseLen;
            inc = releaseVel * t + motorInc * (1.0 - t);
            --releaseLeft;
        }
        else
            inc = motorInc;

        const float kpTarget = (p.keepPitch && ! scratching && releaseLeft == 0) ? 1.0f : 0.0f;
        keepPitchAmount += (kpTarget - keepPitchAmount) * sc;
        const double readInc = inc / tempo * semitoneRatio;

        float l, r;
        renderHead (main, inc, readInc, keepPitchAmount, l, r);
        advanceMain (inc);

        // Repeater: the main head keeps running underneath (slip), so
        // releasing a repeat just fades the wet head out.
        repGain += ((repeatActive >= 0 ? 1.0f : 0.0f) - repGain) * fc;
        mixSmoothed += (targetMix - mixSmoothed) * sc;
        if (repGain > 0.0001f)
        {
            float wl, wr;
            renderHead (wet, inc, readInc, keepPitchAmount, wl, wr);
            advanceWet (inc);
            const float m = mixSmoothed * repGain;
            l = l * (1.0f - m) + wl * m;
            r = r * (1.0f - m) + wr * m;
        }

        // DC blocker: a stopped record outputs a constant sample value.
        float in[2] = { l, r };
        for (int c = 0; c < 2; ++c)
        {
            const float y = in[c] - dcX[c] + dcR * dcY[c];
            dcX[c] = in[c];
            dcY[c] = y;
            in[c] = y;
        }

        // Low-pass state-variable filter (TPT form).
        logCutoff += (targetLogCutoff - logCutoff) * smoothCoef;
        resSmoothed += (targetRes - resSmoothed) * sc;
        filterMix += (targetFilterMix - filterMix) * sc;
        if (filterMix > 0.0001f)
        {
            const float g = (float) std::tan (pi * std::exp (logCutoff) / hostRate);
            const float k = 2.0f - 1.94f * resSmoothed;
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            for (int c = 0; c < 2; ++c)
            {
                const float v3 = in[c] - ic2[c];
                const float v1 = a1 * ic1[c] + a2 * v3;
                const float v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
                ic1[c] = 2.0f * v1 - ic1[c];
                ic2[c] = 2.0f * v2 - ic2[c];
                in[c] = in[c] * (1.0f - filterMix) + v2 * filterMix;
            }
        }
        else
        {
            for (int c = 0; c < 2; ++c)
                ic1[c] = ic2[c] = 0.0f;
        }

        gainSmoothed += (targetGain - gainSmoothed) * sc;
        fadeIn = juce::jmin (1.0f, fadeIn + fadeStep);
        const float g = gainSmoothed * fadeIn;
        const float outL = in[0] * g;
        const float outR = in[1] * g;

        peakL = juce::jmax (peakL, std::abs (outL));
        peakR = juce::jmax (peakR, std::abs (outR));

        if (right != nullptr)
        {
            left[i] = outL;
            right[i] = outR;
        }
        else
            left[i] = 0.5f * (outL + outR);
    }

    lastSpeed = srRatio > 0 ? inc / srRatio : 0.0;
}

} // namespace ts
