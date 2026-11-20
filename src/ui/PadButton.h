#pragma once

#include "Animation.h"
#include "Skin.h"
#include <functional>

namespace ts
{

// A touch-style pad. Reports press and release separately so it can be used
// for momentary actions (repeater, tape stop) as well as taps. It sinks in
// when pressed, flashes on hit and fades its light in and out.
class PadButton final : public juce::Component, private Animated
{
public:
    enum class Tint { Transport, Loop, Repeater, Neutral };

    PadButton (juce::String text, Tint t, PadLook::Kind k = PadLook::Kind::Pad)
        : Animated (static_cast<juce::Component&> (*this)), label (std::move (text)), tint (t), kind (k)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setRepaintsOnMouseActivity (false);
    }

    std::function<void (const juce::ModifierKeys&)> onPress;
    std::function<void()> onRelease;

    void setLabel (const juce::String& t) { if (t != label) { label = t; repaint(); } }
    void setSubLabel (const juce::String& t) { if (t != sub) { sub = t; repaint(); } }
    void setLit (bool b)
    {
        lit.target = b ? 1.0f : 0.0f;
        if (! isShowing())
            lit.snap (lit.target);
    }
    void setFilled (bool b) { if (b != filled) { filled = b; repaint(); } }

    // Visual hit feedback, e.g. when the pad's hotkey is pressed.
    void flash() { glow.value = 1.0f; press.value = 1.0f; repaint(); }

    void paint (juce::Graphics& g) override
    {
        PadLook look;
        look.kind = kind;
        look.label = label;
        look.sub = sub;
        auto& skin = Skin::of (*this);
        look.tint = tint == Tint::Transport ? skin.pal().transport
                  : tint == Tint::Loop      ? skin.pal().loop
                  : tint == Tint::Repeater  ? skin.pal().repeater
                                            : skin.pal().dim;
        look.lit = lit.value;
        look.press = press.value;
        look.hover = hover.value;
        look.glow = glow.value;
        look.filled = filled;
        look.enabled = isEnabled();

        // Sink in slightly when pressed.
        // Leave room around the pad so its glow fades out instead of being cut off.
        const auto r = getLocalBounds().toFloat().reduced (kind == PadLook::Kind::Small ? 1.0f : 5.0f);
        const float scale = 1.0f - 0.035f * press.value;
        g.addTransform (juce::AffineTransform::scale (scale, scale, r.getCentreX(), r.getCentreY()));
        skin.drawPad (g, r, look);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover.target = 1.0f; }
    void mouseExit (const juce::MouseEvent&) override { hover.target = 0.0f; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        held = true;
        press.target = 1.0f;
        press.value = juce::jmax (press.value, 0.6f);
        glow.value = 1.0f;
        repaint();
        if (onPress)
            onPress (e.mods);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        held = false;
        press.target = 0.0f;
        if (onRelease)
            onRelease();
    }

private:
    bool animate (float dt) override
    {
        if (! held)
            press.target = 0.0f;
        glow.target = 0.0f;
        bool moved = press.update (dt, 28.0f);
        moved |= hover.update (dt, 16.0f);
        moved |= lit.update (dt, 22.0f);
        moved |= glow.update (dt, 5.5f);
        return moved;
    }

    juce::String label, sub;
    Tint tint;
    PadLook::Kind kind;
    bool filled = false, held = false;
    Smooth press, hover, lit, glow;
};

// On/off switch drawn by the skin, with an eased transition.
class SkinToggle final : public juce::ToggleButton, private Animated
{
public:
    enum class Tint { Transport, Loop, Filter };

    SkinToggle (const juce::String& text, Tint t)
        : juce::ToggleButton (text), Animated (static_cast<juce::Component&> (*this)), tint (t)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        if (! isShowing() || ! initialised)
        {
            on.snap (getToggleState() ? 1.0f : 0.0f);
            initialised = true;
        }
        auto& skin = Skin::of (*this);
        ToggleLook t;
        t.text = getButtonText();
        t.tint = tint == Tint::Transport ? skin.pal().transport : tint == Tint::Loop ? skin.pal().loop : skin.pal().filter;
        t.on = on.value;
        t.hover = hover.value;
        t.press = press.value;
        t.enabled = isEnabled();
        skin.drawToggle (g, getLocalBounds().toFloat(), t);
    }

private:
    bool animate (float dt) override
    {
        on.target = getToggleState() ? 1.0f : 0.0f;
        hover.target = isOver() ? 1.0f : 0.0f;
        press.target = isDown() ? 1.0f : 0.0f;
        bool moved = on.update (dt, 20.0f);
        moved |= hover.update (dt, 16.0f);
        moved |= press.update (dt, 30.0f);
        return moved;
    }

    Tint tint;
    Smooth on, hover, press;
    bool initialised = false;
};

// Rotary knob whose pointer glides to its value. Double-click resets,
// Shift gives fine control.
class Knob final : public juce::Slider, private Animated
{
public:
    enum class Tint { Transport, Repeater, Neutral };

    explicit Knob (Tint t, bool textBox = true)
        : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, textBox ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox),
          Animated (static_cast<juce::Component&> (*this)), tint (t)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setPopupDisplayEnabled (false, false, nullptr);
        setMouseDragSensitivity (220);
        // Holding Shift switches to slow, fine-grained dragging.
        setVelocityModeParameters (0.35, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        if (textBox)
            setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    }

    void resetTo (double defaultValue) { setDoubleClickReturnValue (true, defaultValue); }

    void paint (juce::Graphics& g) override
    {
        auto& skin = Skin::of (*this);
        const auto area = getLookAndFeel().getSliderLayout (*this).sliderBounds.toFloat();
        const auto col = tint == Tint::Transport ? skin.pal().transport : tint == Tint::Repeater ? skin.pal().repeater : skin.pal().text;
        if (! initialised || ! isShowing())
        {
            shown.snap (target());
            initialised = true;
        }
        skin.drawKnob (g, area, shown.value, hover.value, col, isEnabled());
    }

    void mouseEnter (const juce::MouseEvent& e) override { hover.target = 1.0f; juce::Slider::mouseEnter (e); }
    void mouseExit (const juce::MouseEvent& e) override { hover.target = 0.0f; juce::Slider::mouseExit (e); }

private:
    float target() { return (float) valueToProportionOfLength (getValue()); }

    bool animate (float dt) override
    {
        shown.target = target();
        bool moved = shown.update (dt, 26.0f);
        moved |= hover.update (dt, 16.0f);
        return moved;
    }

    Tint tint;
    Smooth shown, hover;
    bool initialised = false;
};

} // namespace ts
