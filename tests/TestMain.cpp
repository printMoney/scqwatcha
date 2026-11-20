// Offline tests for the deck engine, the sample hand-over and the hotkey map.
// Run: build/TurntableTests_artefacts/Release/TurntableTests

#include "HotkeyMap.h"
#include "engine/BpmDetect.h"
#include "engine/DeckEngine.h"
#include "engine/SampleSlot.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>
#include <vector>

// ---- allocation counter (proves process() never allocates) ----
static std::atomic<bool> countAllocs { false };
static std::atomic<int> allocCount { 0 };

void* operator new (std::size_t n)
{
    if (countAllocs.load (std::memory_order_relaxed))
        allocCount.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (n == 0 ? 1 : n))
        return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n) { return operator new (n); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

using namespace ts;

namespace
{
int failures = 0, checks = 0;

void check (bool ok, const char* what, double value = 0.0)
{
    ++checks;
    if (! ok)
    {
        ++failures;
        std::printf ("    FAIL: %s (value %.6f)\n", what, value);
    }
}

constexpr double rate = 48000.0;

std::unique_ptr<SampleData> makeSine (double seconds, double freq, double sr = rate, float amp = 0.5f)
{
    auto s = std::make_unique<SampleData>();
    s->sampleRate = sr;
    s->name = "sine";
    const int n = (int) (seconds * sr);
    s->audio.setSize (2, n);
    for (int i = 0; i < n; ++i)
    {
        const float v = amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / sr);
        s->audio.setSample (0, i, v);
        s->audio.setSample (1, i, v);
    }
    s->computePeaks (256);
    return s;
}

struct Rig
{
    DeckEngine engine;
    EngineParams params;
    std::unique_ptr<SampleData> sample;
    double scratchOffset = 0.0;
    std::vector<float> out; // left channel history
    int block = 128;

    explicit Rig (std::unique_ptr<SampleData> s, double hostRate = rate) : sample (std::move (s))
    {
        engine.prepare (hostRate);
        engine.setSample (sample.get());
        params.sync = false;
        params.filterBypass = true;
        out.reserve (48000 * 40);
    }

    void cmd (Cmd t, int index = 0, double a = 0, double b = 0, bool flag = false)
    {
        Command c;
        c.type = t;
        c.index = index;
        c.a = a;
        c.b = b;
        c.flag = flag;
        engine.handle (c, params);
    }

    // Runs for `seconds`, calling perBlock before each block.
    void run (double seconds, const std::function<void (double)>& perBlock = {}, double hostRate = rate)
    {
        float l[4096], r[4096];
        int remaining = (int) (seconds * hostRate);
        double t = 0.0;
        while (remaining > 0)
        {
            const int n = juce::jmin (block, remaining);
            if (perBlock)
                perBlock (t);
            countAllocs = true;
            engine.process (l, r, n, params, scratchOffset);
            countAllocs = false;
            for (int i = 0; i < n; ++i)
                out.push_back (l[i]);
            remaining -= n;
            t += n / hostRate;
        }
    }

    // Largest sample-to-sample jump in the recorded output from `from` on.
    float maxStep (size_t from = 0) const
    {
        float m = 0.0f;
        for (size_t i = juce::jmax<size_t> (1, from); i < out.size(); ++i)
            m = juce::jmax (m, std::abs (out[i] - out[i - 1]));
        return m;
    }

    bool allFinite() const
    {
        for (auto v : out)
            if (! std::isfinite (v))
                return false;
        return true;
    }
};

double rms (const std::vector<float>& v, size_t from, size_t to)
{
    double s = 0.0;
    for (size_t i = from; i < to && i < v.size(); ++i)
        s += (double) v[i] * v[i];
    return std::sqrt (s / (double) juce::jmax<size_t> (1, to - from));
}

double zeroCrossingFreq (const std::vector<float>& v, size_t from, size_t to, double sr)
{
    int crossings = 0;
    for (size_t i = from + 1; i < to && i < v.size(); ++i)
        if ((v[i - 1] < 0.0f) != (v[i] < 0.0f))
            ++crossings;
    return crossings / 2.0 / ((double) (to - from) / sr);
}

// A 110 Hz sine at 0.5 changes by ~0.007 per sample at normal speed.
// Anything above 0.05 is an audible click.
constexpr float clickThreshold = 0.05f;

void testPlayback()
{
    std::puts ("playback at normal speed");
    Rig rig (makeSine (10.0, 110.0));
    rig.cmd (Cmd::Play);
    rig.params.tapeStopSeconds = 0.2f;
    rig.run (2.0);
    const double expected = 2.0 * rate - 0.1 * rate; // 0.2 s spin-up covers 0.1 s of audio
    check (std::abs (rig.engine.getPlayhead() - expected) < rate * 0.01, "playhead advances at 1x after spin-up",
           rig.engine.getPlayhead());
    check (std::abs (rms (rig.out, (size_t) rate, (size_t) (2 * rate)) - 0.3535) < 0.02, "output level matches sample");
    check (std::abs (zeroCrossingFreq (rig.out, (size_t) rate, (size_t) (2 * rate), rate) - 110.0) < 2.0, "pitch unchanged at 1x");
    check (rig.maxStep() < clickThreshold, "no clicks during start", rig.maxStep());
}

void testTapeStop()
{
    std::puts ("tape stop and spin-up");
    Rig rig (makeSine (20.0, 110.0));
    rig.params.tapeStopSeconds = 1.0f;
    rig.cmd (Cmd::HardRestart);
    rig.run (1.0);
    rig.cmd (Cmd::TapeStopDown);
    rig.run (0.5);
    check (std::abs (rig.engine.getMotor() - 0.5) < 0.02, "half speed half-way through a 1 s tape stop", rig.engine.getMotor());
    rig.run (0.6);
    check (rig.engine.getMotor() == 0.0, "stopped after the tape stop time", rig.engine.getMotor());
    const double stoppedAt = rig.engine.getPlayhead();
    rig.run (0.5);
    check (std::abs (rig.engine.getPlayhead() - stoppedAt) < 1.0, "holding keeps it stopped");
    const size_t tail = rig.out.size() - (size_t) (0.1 * rate);
    check (rms (rig.out, tail, rig.out.size()) < 0.01, "silent while stopped", rms (rig.out, tail, rig.out.size()));
    rig.cmd (Cmd::TapeStopUp);
    rig.run (1.05);
    check (rig.engine.getMotor() == 1.0, "back to full speed after spin-up", rig.engine.getMotor());
    check (rig.maxStep() < clickThreshold, "no clicks through tape stop", rig.maxStep());

    // 0 s = instant stop and start, still without a click.
    {
        Rig r0 (makeSine (20.0, 110.0));
        r0.params.tapeStopSeconds = 0.0f;
        r0.cmd (Cmd::HardRestart);
        r0.run (0.5);
        const size_t mark0 = r0.out.size();
        r0.cmd (Cmd::TapeStopDown);
        r0.run (0.012);
        check (r0.engine.getMotor() == 0.0, "tape stop at 0 s stops within 12 ms", r0.engine.getMotor());
        r0.run (0.2);
        r0.cmd (Cmd::TapeStopUp);
        r0.run (0.012);
        check (r0.engine.getMotor() == 1.0, "and restarts within 12 ms", r0.engine.getMotor());
        r0.run (0.2);
        check (r0.maxStep (mark0) < clickThreshold, "no click on instant stop or start", r0.maxStep (mark0));
    }

    // Short and long settings.
    for (float secs : { 0.2f, 4.0f })
    {
        Rig r2 (makeSine (20.0, 110.0));
        r2.params.tapeStopSeconds = secs;
        r2.cmd (Cmd::HardRestart);
        r2.run (0.5);
        r2.cmd (Cmd::TapeStopDown);
        r2.run (secs * 0.98);
        check (r2.engine.getMotor() > 0.0, "still slowing just before the end", r2.engine.getMotor());
        r2.run (secs * 0.04 + 0.01);
        check (r2.engine.getMotor() == 0.0, "stopped right at the set time", r2.engine.getMotor());
    }
}

void testPlayPauseAndHardRestart()
{
    std::puts ("play/pause ramp and hard restart at cue");
    Rig rig (makeSine (20.0, 110.0));
    rig.params.tapeStopSeconds = 0.5f;
    rig.cmd (Cmd::SetCue, 0, rate * 2.0);
    rig.cmd (Cmd::TogglePlay);
    rig.run (1.0);
    check (rig.engine.isPlaying(), "space starts playback");
    rig.cmd (Cmd::TogglePlay);
    rig.run (0.25);
    check (rig.engine.getMotor() > 0.4 && rig.engine.getMotor() < 0.6, "pause ramps down like a tape stop", rig.engine.getMotor());
    rig.run (0.3);
    check (rig.engine.getMotor() == 0.0, "pause reaches zero");

    const size_t mark = rig.out.size();
    rig.cmd (Cmd::HardRestart);
    rig.run (0.5);
    check (std::abs (rig.engine.getPlayhead() - rate * 2.5) < 2.0, "hard restart jumps to cue and plays at full speed instantly",
           rig.engine.getPlayhead());
    check (rig.maxStep (mark) < clickThreshold, "no click on hard restart", rig.maxStep (mark));
}

void testScratch()
{
    std::puts ("scratch follows the hand, no clicks, ramps back");
    Rig rig (makeSine (20.0, 110.0));
    rig.params.tapeStopSeconds = 1.0f;
    rig.cmd (Cmd::HardRestart);
    rig.run (1.0);
    const double before = rig.engine.getPlayhead();

    rig.scratchOffset = 0.0;
    rig.cmd (Cmd::ScratchBegin);
    // Hand moves back and forth, updated at 60 Hz like a mouse: +-0.25 s at 2 Hz.
    double lastUpdate = -1.0;
    rig.run (2.0, [&] (double t) {
        if (t - lastUpdate >= 1.0 / 60.0)
        {
            rig.scratchOffset = 0.25 * rate * std::sin (2.0 * juce::MathConstants<double>::pi * 2.0 * t);
            lastUpdate = t;
        }
    });
    // Hold the record still for a moment.
    rig.run (0.3);
    const double target = before + rig.scratchOffset;
    check (std::abs (rig.engine.getPlayhead() - target) < rate * 0.01, "playhead follows the hand", rig.engine.getPlayhead() - target);
    const size_t still = rig.out.size() - (size_t) (0.05 * rate);
    check (rms (rig.out, still, rig.out.size()) < 0.01, "silent with the hand holding the record");

    // Scratch speed peaks at about 3x, so allow 3x the normal step.
    check (rig.maxStep() < clickThreshold * 1.5f, "no clicks while scratching", rig.maxStep());

    const size_t mark = rig.out.size();
    rig.cmd (Cmd::ScratchEnd);
    rig.run (0.2);
    check (std::abs (rig.engine.getSpeed() - 1.0) < 0.01, "release returns to the previous speed", rig.engine.getSpeed());
    check (rig.maxStep (mark) < clickThreshold, "no click on release", rig.maxStep (mark));

    // Backwards scratch past the start of the record must not get stuck.
    Rig r2 (makeSine (5.0, 110.0));
    r2.cmd (Cmd::HardRestart);
    r2.run (0.2);
    const double anchor = r2.engine.getPlayhead();
    r2.cmd (Cmd::ScratchBegin);
    r2.scratchOffset = -rate * 2.0;
    r2.run (0.5);
    check (r2.engine.getPlayhead() == 0.0, "backwards scratch stops at the start", r2.engine.getPlayhead());
    r2.scratchOffset = -anchor + rate * 0.5; // hand back to 0.5 s
    r2.run (0.5);
    check (std::abs (r2.engine.getPlayhead() - rate * 0.5) < rate * 0.01, "and can be dragged forward again", r2.engine.getPlayhead());
    check (r2.allFinite(), "finite output");
}

void testLoop()
{
    std::puts ("loop wraps without clicks");
    Rig rig (makeSine (20.0, 110.0));
    rig.cmd (Cmd::SetLoop, 0, rate * 1.0, rate * 1.537, true);
    rig.cmd (Cmd::HardRestart); // cue is 0
    rig.run (1.0);
    double minPos = 1e30, maxPos = -1e30;
    rig.run (5.0, [&] (double) {
        minPos = juce::jmin (minPos, rig.engine.getPlayhead());
        maxPos = juce::jmax (maxPos, rig.engine.getPlayhead());
    });
    check (minPos >= rate * 1.0 - 1.0 && maxPos <= rate * 1.537 + 1.0, "playhead stays inside the loop");
    check (rig.maxStep() < clickThreshold, "no clicks at loop boundaries", rig.maxStep());

    rig.cmd (Cmd::SetLoop, 0, rate * 1.0, rate * 1.537, false);
    rig.run (1.0);
    check (rig.engine.getPlayhead() > rate * 1.537, "loop off lets it play on", rig.engine.getPlayhead());
}

void testRepeater()
{
    std::puts ("repeater 1/4 at 120 BPM");
    Rig rig (makeSine (20.0, 97.0)); // not a divisor of the slice, so wraps are real jumps
    rig.params.sync = true;
    rig.params.sampleBpm = 120.0;
    rig.params.effectiveBpm = 120.0;
    rig.params.repeaterMix = 1.0f;
    rig.cmd (Cmd::HardRestart);
    rig.run (2.0);
    const double dryBefore = rig.engine.getPlayhead();
    const size_t mark = rig.out.size();
    rig.cmd (Cmd::RepeatDown, 2);
    check (rig.engine.getRepeatIndex() == 2, "repeater engaged");
    rig.run (2.0);

    // 1/4 bar at 120 BPM = 1 beat = 0.5 s. Output must repeat every 0.5 s.
    const size_t slice = (size_t) (0.5 * rate);
    const size_t a = mark + slice + slice / 4;
    double diff = 0.0;
    for (size_t i = 0; i < slice / 2; ++i)
        diff = juce::jmax (diff, (double) std::abs (rig.out[a + i] - rig.out[a + slice + i]));
    check (diff < 0.01, "output repeats every 1/4 bar", diff);
    check (rig.maxStep (mark) < clickThreshold, "no clicks while repeating", rig.maxStep (mark));
    check (std::abs (rig.engine.getPlayhead() - (dryBefore + 2.0 * rate)) < 2.0, "playhead keeps running underneath (slip)");

    const size_t rel = rig.out.size();
    rig.cmd (Cmd::RepeatUp, 2);
    rig.run (0.5);
    check (rig.engine.getRepeatIndex() == -1, "repeater released");
    check (rig.maxStep (rel) < clickThreshold, "no click on release", rig.maxStep (rel));

    // Mix 0 = dry only.
    Rig r2 (makeSine (20.0, 97.0));
    r2.params.repeaterMix = 0.0f;
    r2.cmd (Cmd::HardRestart);
    r2.run (1.0);
    Rig r3 (makeSine (20.0, 97.0));
    r3.cmd (Cmd::HardRestart);
    r3.run (1.0);
    r2.cmd (Cmd::RepeatDown, 0);
    r2.run (1.0);
    r3.run (1.0);
    double d2 = 0.0;
    for (size_t i = 0; i < r2.out.size(); ++i)
        d2 = juce::jmax (d2, (double) std::abs (r2.out[i] - r3.out[i]));
    check (d2 < 1e-4, "mix at 0 leaves the dry signal untouched", d2);
}

void testBpmChange()
{
    std::puts ("BPM sync and BPM change mid-playback");
    Rig rig (makeSine (30.0, 110.0));
    rig.params.sync = true;
    rig.params.sampleBpm = 120.0;
    rig.params.effectiveBpm = 120.0;
    rig.cmd (Cmd::HardRestart);
    rig.run (1.0);
    const size_t mark = rig.out.size();
    double maxJump = 0.0, last = rig.engine.getPlayhead();
    rig.params.effectiveBpm = 150.0;
    rig.run (2.0, [&] (double) {
        maxJump = juce::jmax (maxJump, std::abs (rig.engine.getPlayhead() - last));
        last = rig.engine.getPlayhead();
    });
    check (std::abs (rig.engine.getTempoRatio() - 1.25) < 1e-4, "rate = project BPM / sample BPM", rig.engine.getTempoRatio());
    check (maxJump < rig.block * 1.26 + 1.0, "playhead never jumps", maxJump);
    check (std::abs (zeroCrossingFreq (rig.out, rig.out.size() - (size_t) rate, rig.out.size(), rate) - 137.5) < 2.0,
           "pitch follows speed (varispeed)");
    check (rig.maxStep (mark) < clickThreshold, "no click on BPM change", rig.maxStep (mark));

    std::puts ("keep pitch");
    Rig kp (makeSine (30.0, 220.0));
    kp.params.sync = true;
    kp.params.keepPitch = true;
    kp.params.sampleBpm = 100.0;
    kp.params.effectiveBpm = 130.0;
    kp.cmd (Cmd::HardRestart);
    kp.run (3.0);
    check (std::abs (kp.engine.getPlayhead() - 3.0 * 1.3 * rate) < rate * 0.05, "tempo changes with keep pitch");
    const double f = zeroCrossingFreq (kp.out, kp.out.size() - (size_t) rate, kp.out.size(), rate);
    check (std::abs (f - 220.0) < 4.0, "pitch stays put with keep pitch", f);

    kp.params.pitchSemitones = 12.0f;
    kp.run (1.0);
    const double f2 = zeroCrossingFreq (kp.out, kp.out.size() - (size_t) (0.5 * rate), kp.out.size(), rate);
    check (std::abs (f2 - 440.0) < 8.0, "+12 semitones doubles the pitch", f2);
}

void testFilter()
{
    std::puts ("filter");
    Rig open (makeSine (5.0, 5000.0));
    open.params.filterBypass = false;
    open.params.cutoffHz = 200.0f;
    open.params.resonance = 0.0f;
    open.cmd (Cmd::HardRestart);
    open.run (1.0);
    const double filtered = rms (open.out, (size_t) (0.5 * rate), (size_t) rate);
    check (filtered < 0.01, "low cutoff removes 5 kHz", filtered);

    open.params.filterBypass = true;
    open.run (1.0);
    const double bypassed = rms (open.out, open.out.size() - (size_t) (0.5 * rate), open.out.size());
    check (bypassed > 0.3, "bypass lets it through", bypassed);

    Rig res (makeSine (5.0, 110.0));
    res.params.filterBypass = false;
    res.params.resonance = 1.0f;
    res.params.cutoffHz = 20000.0f;
    res.cmd (Cmd::HardRestart);
    res.run (0.5);
    res.params.cutoffHz = 60.0f; // fast sweep down at full resonance
    res.run (1.0);
    check (res.allFinite(), "filter stays stable at full resonance");
}

void testSampleRateChanges()
{
    std::puts ("sample rate changes and odd block sizes");
    auto s = makeSine (5.0, 110.0, 44100.0);
    DeckEngine e;
    EngineParams p;
    float l[4096], r[4096];
    bool finite = true;
    int blockIndex = 0;
    for (double sr : { 44100.0, 96000.0, 48000.0, 192000.0, 22050.0 })
    {
        e.prepare (sr);
        e.setSample (s.get());
        Command c;
        c.type = Cmd::HardRestart;
        e.handle (c, p);
        for (int i = 0; i < 200; ++i)
        {
            const int n = 1 + (blockIndex++ * 37) % 2048;
            e.process (l, (i % 3 == 0) ? nullptr : r, n, p, 0.0);
            for (int k = 0; k < n; ++k)
                finite = finite && std::isfinite (l[k]);
        }
    }
    check (finite, "finite output at every rate and block size");

    // Sample at 44.1k played at a 96k host must still play at normal pitch.
    Rig rig (makeSine (5.0, 110.0, 44100.0), 96000.0);
    rig.cmd (Cmd::HardRestart);
    rig.run (1.0, {}, 96000.0);
    const double f = zeroCrossingFreq (rig.out, (size_t) (0.2 * 96000), rig.out.size(), 96000.0);
    check (std::abs (f - 110.0) < 2.0, "correct pitch when host and file rates differ", f);

    // No sample loaded: silence, no crash.
    DeckEngine empty;
    empty.prepare (48000.0);
    empty.process (l, r, 512, p, 0.0);
    check (l[0] == 0.0f && r[511] == 0.0f, "silence without a sample");
}

void testRunOffEnd()
{
    std::puts ("running off the end");
    Rig rig (makeSine (1.0, 110.0));
    rig.cmd (Cmd::HardRestart);
    rig.run (1.5);
    check (! rig.engine.isPlaying(), "stops at the end of the sample");
    check (rig.maxStep() < clickThreshold, "no click at the end", rig.maxStep());
    rig.cmd (Cmd::TogglePlay);
    rig.run (0.3);
    check (rig.engine.isPlaying() && rig.engine.getPlayhead() < rate * 0.5, "play again restarts from the cue");
}

void testSampleSlot()
{
    std::puts ("sample hand-over");
    SampleSlot slot;
    slot.submit (makeSine (0.1, 100.0));
    slot.submit (makeSine (0.1, 100.0)); // replaces the first before audio sees it
    const auto* a = slot.acquire();
    check (a != nullptr && a->seq == 2, "audio thread gets the newest sample");
    slot.collectGarbage();
    check (slot.numOwned() == 1, "unused older sample freed after acknowledgement", (double) slot.numOwned());
    slot.submit (makeSine (0.1, 100.0));
    slot.collectGarbage();
    check (slot.numOwned() == 2, "sample in use is not freed before the audio thread moves on", (double) slot.numOwned());
    const auto* b = slot.acquire();
    slot.collectGarbage();
    check (b != a && slot.numOwned() == 1, "old sample freed once the new one is acknowledged");
}

void testBpmDetect()
{
    std::puts ("BPM detection");
    check (bpmFromFileName ("break_92bpm.wav") == 92.0, "92 from file name");
    check (bpmFromFileName ("Loop 128 BPM.aif") == 128.0, "128 from file name");
    check (bpmFromFileName ("kick.wav") == 0.0, "none in file name");

    for (double bpm : { 96.0, 128.0, 140.0 })
    {
        const double sr = 44100.0;
        const int n = (int) (sr * 16.0);
        juce::AudioBuffer<float> buf (2, n);
        buf.clear();
        const double beat = 60.0 / bpm * sr;
        for (double t = 0; t < n; t += beat)
            for (int i = 0; i < 2000 && (int) t + i < n; ++i)
            {
                const float v = std::exp ((float) -i / 300.0f) * (float) std::sin (i * 0.3);
                buf.setSample (0, (int) t + i, v);
                buf.setSample (1, (int) t + i, v);
            }
        const double got = detectBpm (buf, sr);
        check (std::abs (got - bpm) < 1.5, "click track tempo detected", got);
    }
}

void testHotkeys()
{
    std::puts ("hotkeys");
    HotkeyMap map;
    // Every default parses and no two actions share a key.
    int bound = 0;
    for (auto& a : HotkeyMap::actions())
    {
        const auto b = map.get (a.action);
        check (b.isValid(), a.id);
        if (b.isValid())
            ++bound;
    }
    check (bound == (int) HotkeyMap::actions().size(), "all defaults bound");
    const auto problems = map.loadFromJson (map.toJson());
    check (problems.isEmpty(), "defaults have no clashes and survive a JSON round trip", problems.size());
    if (! problems.isEmpty())
        std::printf ("      %s\n", problems.joinIntoString ("; ").toRawUTF8());

    check (map.find (*HotkeyMap::parse ("Space")) == Action::PlayPause, "Space = play/pause");
    check (map.find (*HotkeyMap::parse ("Shift+Space")) == Action::HardRestart, "Shift+Space = hard restart");
    check (map.find (*HotkeyMap::parse ("G")) == Action::TapeStop, "G = tape stop");
    check (map.find (*HotkeyMap::parse ("T")) == Action::Repeat16, "T = repeater 1/16");
    check (map.find (*HotkeyMap::parse ("?")) == Action::ShowHelp, "? = help");

    // A shifted digit as macOS reports it ('!' with shift) must hit "store pad 1".
    const juce::KeyPress shifted1 ('!', juce::ModifierKeys::shiftModifier, '!');
    check (map.find (HotkeyMap::fromKeyPress (shifted1)) == Action::PadStore1, "Shift+1 stores pad 1");
    const juce::KeyPress lowerS ('s', 0, 's');
    check (map.find (HotkeyMap::fromKeyPress (lowerS)) == Action::ScratchHold, "lower-case s = scratch");
    const juce::KeyPress bracket ('{', juce::ModifierKeys::shiftModifier, '{');
    check (! map.find (HotkeyMap::fromKeyPress (bracket)).has_value(), "Shift+[ is not loop-in");

    // User overrides and clash detection.
    auto p2 = map.loadFromJson (R"({"bindings":{"tape_stop":"H","repeat_1_16":"Q"}})");
    check (map.find (*HotkeyMap::parse ("H")) == Action::TapeStop, "user can rebind");
    check (p2.size() == 1, "clash reported when two actions share a key", p2.size());
    auto p3 = map.loadFromJson ("{ not json");
    check (p3.size() == 1 && map.find (*HotkeyMap::parse ("G")) == Action::TapeStop, "bad JSON falls back to defaults");
}

} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc > 1 && juce::String (argv[1]) == "--print-hotkeys")
    {
        std::puts (HotkeyMap().toJson().toRawUTF8());
        return 0;
    }

    testPlayback();
    testTapeStop();
    testPlayPauseAndHardRestart();
    testScratch();
    testLoop();
    testRepeater();
    testBpmChange();
    testFilter();
    testSampleRateChanges();
    testRunOffEnd();
    testSampleSlot();
    testBpmDetect();
    testHotkeys();

    std::printf ("\nallocations inside process(): %d\n", allocCount.load());
    check (allocCount.load() == 0, "no allocations on the audio thread", allocCount.load());

    std::printf ("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
