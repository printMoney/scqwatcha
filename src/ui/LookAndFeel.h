#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{

namespace Colours
{
inline const juce::Colour background { 0xff141519 };
inline const juce::Colour panel { 0xff1f2127 };
inline const juce::Colour panelEdge { 0xff2c2f37 };
inline const juce::Colour text { 0xffe9e9ec };
inline const juce::Colour dim { 0xff8b8f99 };
inline const juce::Colour accent { 0xffff8a2b };   // transport / platter
inline const juce::Colour filter { 0xff38d0c4 };   // filter pad
inline const juce::Colour repeater { 0xffb57bff }; // repeater
inline const juce::Colour pad { 0xff4f9dff };      // loop pads
inline const juce::Colour danger { 0xffff4d5e };
} // namespace Colours

class TSLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TSLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    static juce::Font font (float size, bool bold = false);
};

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint);

} // namespace ts
