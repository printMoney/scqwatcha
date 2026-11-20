#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cstdint>
#include <vector>

namespace ts
{

// A decoded sample. Created on a background thread, then read-only.
// Always holds exactly two channels (mono files are duplicated).
struct SampleData
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String path;
    juce::String name;
    double detectedBpm = 120.0;
    std::vector<float> peakMin, peakMax; // overview for the waveform display
    uint64_t seq = 0;                    // set by SampleSlot

    int numFrames() const noexcept { return audio.getNumSamples(); }
    double lengthSeconds() const noexcept { return sampleRate > 0 ? numFrames() / sampleRate : 0.0; }

    void computePeaks (int bins)
    {
        peakMin.assign ((size_t) bins, 0.0f);
        peakMax.assign ((size_t) bins, 0.0f);
        const int n = numFrames();
        if (n == 0 || bins <= 0)
            return;

        const float* l = audio.getReadPointer (0);
        const float* r = audio.getReadPointer (1);
        for (int b = 0; b < bins; ++b)
        {
            const int start = (int) ((int64_t) b * n / bins);
            const int end = juce::jmax (start + 1, (int) ((int64_t) (b + 1) * n / bins));
            float lo = 0.0f, hi = 0.0f;
            for (int i = start; i < end && i < n; ++i)
            {
                const float v = 0.5f * (l[i] + r[i]);
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }
            peakMin[(size_t) b] = lo;
            peakMax[(size_t) b] = hi;
        }
    }
};

} // namespace ts
