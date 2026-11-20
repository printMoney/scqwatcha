#pragma once

#include "Skin.h"

namespace ts
{

// Calm, high-contrast dark skin made for performing. Colour only marks what
// is active: orange transport, blue loops, purple repeater, teal filter.
class StudioInkSkin final : public Skin
{
public:
    StudioInkSkin();
    juce::String name() const override { return "Studio Ink"; }
};

// Current Apple style: frosted glass panels over a soft colour field,
// glossy pebble controls, light type.
class LiquidGlassSkin final : public Skin
{
public:
    LiquidGlassSkin();
    juce::String name() const override { return "Liquid Glass"; }

    juce::Font font (float size, bool bold = false) const override;
    juce::Font labelFont (float size) const override;
    juce::Font screenFont (float size, bool bold = false) const override;
    bool upperCaseLabels() const override { return false; }

    void drawBackground (juce::Graphics&, juce::Rectangle<float>) override;
    void drawTitle (juce::Graphics&, juce::Rectangle<float>) override;
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour) override;
    void drawPad (juce::Graphics&, juce::Rectangle<float>, const PadLook&) override;
    void drawToggle (juce::Graphics&, juce::Rectangle<float>, const ToggleLook&) override;
    void drawKnob (juce::Graphics&, juce::Rectangle<float>, float, float, juce::Colour, bool) override;
    void drawPlatter (juce::Graphics&, juce::Rectangle<float>, const PlatterLook&) override;
    void drawWaveformFrame (juce::Graphics&, juce::Rectangle<float>) override;
    juce::Colour waveColour() const override { return juce::Colours::white.withAlpha (0.75f); }
    void drawXYPad (juce::Graphics&, juce::Rectangle<float>, const XYLook&, juce::Colour) override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float, float) override;
    void drawOverlay (juce::Graphics&, juce::Rectangle<float>) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;

private:
    void glass (juce::Graphics&, juce::Rectangle<float> r, float corner, float strength = 1.0f) const;
    juce::Image backdrop;
};

// Late-90s Japanese sampler / groovebox: warm grey plastic, recessed panels
// with screws, amber dot-matrix screens, backlit rubber pads.
class Retro97Skin final : public Skin
{
public:
    Retro97Skin();
    juce::String name() const override { return "Retro '97"; }

    juce::Font font (float size, bool bold = false) const override;
    juce::Font labelFont (float size) const override;
    juce::Font screenFont (float size, bool bold = false) const override;

    void drawBackground (juce::Graphics&, juce::Rectangle<float>) override;
    void drawTitle (juce::Graphics&, juce::Rectangle<float>) override;
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour) override;
    void drawSmallLabel (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Justification) override;
    void drawScreen (juce::Graphics&, juce::Rectangle<float>, const juce::StringArray&, juce::Justification, bool) override;
    void drawStatusBar (juce::Graphics&, juce::Rectangle<float>, const juce::String&, const juce::String&, bool) override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float, float) override;
    void drawOverlay (juce::Graphics&, juce::Rectangle<float>) override;
    void drawPad (juce::Graphics&, juce::Rectangle<float>, const PadLook&) override;
    void drawToggle (juce::Graphics&, juce::Rectangle<float>, const ToggleLook&) override;
    void drawKnob (juce::Graphics&, juce::Rectangle<float>, float, float, juce::Colour, bool) override;
    void drawPlatter (juce::Graphics&, juce::Rectangle<float>, const PlatterLook&) override;
    void drawWaveformFrame (juce::Graphics&, juce::Rectangle<float>) override;
    juce::Colour waveColour() const override { return palette.screenText.withAlpha (0.85f); }
    void drawXYPad (juce::Graphics&, juce::Rectangle<float>, const XYLook&, juce::Colour) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;

private:
    void lcd (juce::Graphics&, juce::Rectangle<float> r) const;
    void lcdTexture (juce::Graphics&, juce::Rectangle<float> r) const;
    void screw (juce::Graphics&, juce::Point<float> c, float angle) const;
    juce::Image plastic;
};

} // namespace ts
