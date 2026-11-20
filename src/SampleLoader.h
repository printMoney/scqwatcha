#pragma once

#include "engine/SampleData.h"
#include <memory>

namespace ts
{

// Decodes WAV / AIFF / MP3 (and anything else macOS Core Audio reads).
// Background thread only. Returns null and sets error on failure.
std::unique_ptr<SampleData> decodeSample (const juce::File& file, juce::String& error);

bool isSupportedAudioFile (const juce::File& file);

} // namespace ts
