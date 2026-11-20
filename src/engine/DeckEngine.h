#pragma once

#include "Commands.h"
#include "SampleData.h"

namespace ts
{

// Everything the engine needs from parameters, read once per block.
struct EngineParams
{
    double effectiveBpm = 120.0; // host BPM or the system BPM knob
    double sampleBpm = 120.0;
    bool sync = true;            // tempo-match the sample to effectiveBpm
    bool keepPitch = false;      // pitch-preserving tempo change
    float pitchSemitones = 0.0f; // only used with keepPitch
    float tapeStopSeconds = 1.0f;
    float repeaterMix = 1.0f;
    float cutoffHz = 20000.0f;
    float resonance = 0.1f;      // 0..1
    bool filterBypass = true;
    float gainDb = 0.0f;
};

// The turntable. Audio thread only, apart from prepare().
// No allocation, no locks, no system calls inside process().
//
// Positions are in source frames (the sample's own sample rate), so the
// sample rate of the host can change without moving anything.
class DeckEngine
{
public:
    static constexpr double revolutionSeconds = 1.8; // 33 1/3 rpm
    static constexpr int numRepeatRates = 5;         // 1/1 .. 1/16 bar

    void prepare (double hostSampleRate);
    void setSample (const SampleData* s) noexcept;
    const SampleData* getSample() const noexcept { return sample; }

    void handle (const Command& c, const EngineParams& p) noexcept;

    // Writes n frames. right may be null for mono output.
    // scratchOffset: hand movement in source frames since ScratchBegin.
    void process (float* left, float* right, int n, const EngineParams& p, double scratchOffset) noexcept;

    double getPlayhead() const noexcept { return main.pos; }
    double getSpeed() const noexcept { return lastSpeed; }
    double getTempoRatio() const noexcept { return tempo; }
    bool isPlaying() const noexcept { return playing; }
    bool isScratching() const noexcept { return scratching; }
    int getRepeatIndex() const noexcept { return repeatActive; }
    double getMotor() const noexcept { return motor; }
    float getPeakL() const noexcept { return peakL; }
    float getPeakR() const noexcept { return peakR; }

    static double repeatBeats (int index) noexcept { return 4.0 / (double) (1 << index); }

private:
    struct Grain
    {
        double readPos = 0.0;
        int phase = 0;
    };

    struct Head
    {
        double pos = 0.0;
        double fadePos = 0.0; // old position, faded out after a jump
        int fadeLeft = 0;
        Grain grains[2];
    };

    void read (double pos, float& l, float& r) const noexcept;
    void jump (Head& h, double newPos) noexcept;
    void renderHead (Head& h, double inc, double readInc, float keepPitchAmount, float& l, float& r) noexcept;
    void advanceMain (double inc) noexcept;
    void advanceWet (double inc) noexcept;
    void startRepeat (int index, const EngineParams& p) noexcept;
    double tempoTarget (const EngineParams& p) const noexcept;
    double clampPos (double p) const noexcept;

    const SampleData* sample = nullptr;
    double hostRate = 44100.0;

    // Times converted to samples / coefficients in prepare().
    int fadeLen = 240;
    int grainLen = 1764;
    int releaseLen = 3528;
    double handK1 = 0.0, handK2 = 0.0;
    double tempoCoef = 0.0, smoothCoef = 0.0, fastCoef = 0.0;
    float dcR = 0.999f;

    // Transport.
    Head main, wet;
    bool playing = false;
    bool tapeHeld = false;
    double motor = 0.0; // 0..1
    double tempo = 1.0;
    bool tempoInitialised = false;
    double cue = 0.0;
    double lastSpeed = 0.0;

    // Scratch.
    bool scratching = false;
    double scratchAnchor = 0.0;
    double handVel = 0.0;
    double releaseVel = 0.0;
    int releaseLeft = 0;

    // Loop.
    bool loopOn = false;
    double loopIn = 0.0, loopOut = 0.0;

    // Repeater.
    int repeatActive = -1;
    unsigned repeatHeldMask = 0;
    double repStart = 0.0, repEnd = 0.0;
    float repGain = 0.0f;
    float mixSmoothed = 1.0f;

    // Keep pitch blend (0 = varispeed, 1 = granular).
    float keepPitchAmount = 0.0f;

    // Output stage.
    float fadeIn = 1.0f;
    float dcX[2] {}, dcY[2] {};
    float ic1[2] {}, ic2[2] {};
    double logCutoff = 0.0;
    float resSmoothed = 0.1f;
    float filterMix = 0.0f;
    float gainSmoothed = 1.0f;
    float peakL = 0.0f, peakR = 0.0f;
};

} // namespace ts
