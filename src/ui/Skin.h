#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace ts
{

struct Palette
{
    juce::Colour background, panel, panelEdge, text, dim;
    juce::Colour transport, loop, repeater, filter, danger;
    juce::Colour padIdle, screen, screenText;
};

struct PadLook
{
    enum class Kind { Pad, Transport, Small };
    Kind kind = Kind::Pad;
    juce::String label, sub;
    juce::Colour tint;
    float lit = 0.0f;   // 0..1, eased
    float press = 0.0f; // 0..1, eased
    float hover = 0.0f; // 0..1, eased
    float glow = 0.0f;  // 1 on hit, fades out
    bool filled = false;
    bool enabled = true;
};

struct ToggleLook
{
    juce::String text;
    juce::Colour tint;
    float on = 0.0f, hover = 0.0f, press = 0.0f;
    bool enabled = true;
};

struct PlatterLook
{
    double angle = 0.0;       // radians
    bool hasSample = false;
    juce::String name;
    juce::String message;     // shown when not empty ("Loading...", "Drop a sample")
    float scratch = 0.0f;     // 0..1 hand on the record
    bool dropHover = false;
    float speed = 0.0f;       // 1 = normal
};

struct XYLook
{
    float x = 0.0f, y = 0.0f; // 0..1
    bool on = false;
    float press = 0.0f;
    juce::String info;
};

// A complete visual style. Every custom component draws through the skin of
// its LookAndFeel, so switching skins restyles everything at once.
class Skin : public juce::LookAndFeel_V4
{
public:
    explicit Skin (const Palette& p);

    static juce::StringArray names();
    static std::unique_ptr<Skin> create (const juce::String& name);
    static Skin& of (juce::Component& c);
    static const juce::String defaultName;

    const Palette& pal() const noexcept { return palette; }
    virtual juce::String name() const = 0;

    // Type.
    virtual juce::Font font (float size, bool bold = false) const;
    virtual juce::Font labelFont (float size) const { return font (size, true); }
    virtual juce::Font screenFont (float size, bool bold = false) const { return font (size, bold); }
    virtual bool upperCaseLabels() const { return true; }

    // Window pieces.
    virtual void drawBackground (juce::Graphics&, juce::Rectangle<float> bounds);
    virtual void drawTitle (juce::Graphics&, juce::Rectangle<float> area);
    virtual void drawPanel (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint);
    virtual void drawSmallLabel (juce::Graphics&, juce::Rectangle<float> r, const juce::String& text, juce::Justification j);
    virtual void drawScreen (juce::Graphics&, juce::Rectangle<float> r, const juce::StringArray& lines, juce::Justification j,
                             bool warning = false);
    virtual void drawStatusBar (juce::Graphics&, juce::Rectangle<float> r, const juce::String& left, const juce::String& right,
                                bool warning);
    virtual void drawMeter (juce::Graphics&, juce::Rectangle<float> r, float left, float right);
    virtual void drawOverlay (juce::Graphics&, juce::Rectangle<float> r);

    // Controls.
    virtual void drawPad (juce::Graphics&, juce::Rectangle<float> r, const PadLook&);
    virtual void drawToggle (juce::Graphics&, juce::Rectangle<float> r, const ToggleLook&);
    virtual void drawKnob (juce::Graphics&, juce::Rectangle<float> r, float pos, float hover, juce::Colour tint, bool enabled);
    virtual void drawPlatter (juce::Graphics&, juce::Rectangle<float> r, const PlatterLook&);
    virtual void drawWaveformFrame (juce::Graphics&, juce::Rectangle<float> r);
    virtual juce::Colour waveColour() const { return palette.transport.withAlpha (0.8f); }
    virtual void drawXYPad (juce::Graphics&, juce::Rectangle<float> r, const XYLook&, juce::Colour tint);

    // juce::LookAndFeel hooks (sliders' text boxes, inc/dec buttons, combo box, menus).
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

protected:
    void applyColours();
    juce::String caps (const juce::String& s) const { return upperCaseLabels() ? s.toUpperCase() : s; }

    // Shared helpers for subclasses.
    static void fillGlow (juce::Graphics&, juce::Rectangle<float> r, juce::Colour c, float amount, float spread, float corner);
    static void drawVinyl (juce::Graphics&, juce::Point<float> c, float R, const PlatterLook&, juce::Colour label,
                           juce::Colour labelText, const juce::Font& labelFont);

    Palette palette;
};

} // namespace ts
