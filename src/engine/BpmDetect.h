#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace ts
{

// Reads a BPM from a file name such as "break_92bpm.wav" or "Loop 128 BPM.aif".
// Returns 0 if there is none.
double bpmFromFileName (const juce::String& fileName);

// Estimates tempo (70..180 BPM) from an onset envelope and autocorrelation,
// snapping to a whole number of beats when the sample looks like a loop.
// Background thread only: allocates.
double detectBpm (const juce::AudioBuffer<float>& audio, double sampleRate);

} // namespace ts
