#include "KeyboardLayout.h"

#include <Carbon/Carbon.h>

namespace ts
{

std::map<juce::juce_wchar, juce::juce_wchar> currentLayoutShiftMap()
{
    std::map<juce::juce_wchar, juce::juce_wchar> result;

    TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
    if (source == nullptr)
        return result;

    if (auto data = (CFDataRef) TISGetInputSourceProperty (source, kTISPropertyUnicodeKeyLayoutData))
    {
        const auto* layout = (const UCKeyboardLayout*) CFDataGetBytePtr (data);
        const UInt32 keyboardType = LMGetKbdType();

        auto translate = [&] (UInt16 virtualKey, UInt32 modifiers) -> UniChar {
            UInt32 deadKeyState = 0;
            UniCharCount length = 0;
            UniChar chars[4] {};
            const OSStatus status = UCKeyTranslate (layout, virtualKey, kUCKeyActionDown, (modifiers >> 8) & 0xFF, keyboardType,
                                                    kUCKeyTranslateNoDeadKeysBit, &deadKeyState, 4, &length, chars);
            return (status == noErr && length == 1) ? chars[0] : 0;
        };

        for (UInt16 vk = 0; vk < 128; ++vk)
        {
            const auto base = (juce::juce_wchar) translate (vk, 0);
            const auto shifted = (juce::juce_wchar) translate (vk, shiftKey);
            if (base == 0 || shifted == 0 || base == shifted)
                continue;
            if (juce::CharacterFunctions::isLetter (base) || base < 32 || shifted < 32)
                continue;
            // Keep the first mapping found (main keyboard comes before the keypad).
            result.emplace (shifted, base);
        }
    }

    CFRelease (source);
    return result;
}

} // namespace ts
