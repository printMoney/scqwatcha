#pragma once

#include <juce_core/juce_core.h>
#include <map>

namespace ts
{

// For the current keyboard layout: the character a key produces with Shift
// mapped to the character it produces without (e.g. on a German or Austrian
// layout '"' -> '2', '§' -> '3'; on US '!' -> '1'). Letters are not included.
// Message thread only (queries the system keyboard layout).
std::map<juce::juce_wchar, juce::juce_wchar> currentLayoutShiftMap();

} // namespace ts
