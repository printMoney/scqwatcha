#pragma once

#include "../HotkeyMap.h"
#include "LookAndFeel.h"

namespace ts
{

// The "?" screen: every action with its key, plus buttons to edit and reload
// the hotkeys.json file.
class HotkeyOverlay final : public juce::Component
{
public:
    HotkeyOverlay();

    void show (const HotkeyMap& map, const juce::StringArray& problems);
    std::function<void()> onClose;
    std::function<void()> onReload;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray keys, labels, problems;
    juce::TextButton editButton { "Edit hotkeys.json" }, reloadButton { "Reload" }, closeButton { "Close (Esc)" };
};

} // namespace ts
