#include "XYPad.h"

namespace ts
{

XYPad::XYPad (juce::RangedAudioParameter& c, juce::RangedAudioParameter& r, juce::RangedAudioParameter& b)
    : Animated (static_cast<juce::Component&> (*this)), cutoff (c), resonance (r), bypass (b)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

juce::Rectangle<float> XYPad::padArea() const
{
    return getLocalBounds().toFloat().reduced (2.0f);
}

bool XYPad::animate (float dt)
{
    shownX.target = cutoff.getValue();
    shownY.target = resonance.getValue();
    if (! initialised)
    {
        shownX.snap (shownX.target);
        shownY.snap (shownY.target);
        initialised = true;
    }
    press.target = dragging ? 1.0f : 0.0f;
    const bool b = bypass.getValue() > 0.5f;
    bool moved = b != lastBypass;
    lastBypass = b;
    moved |= shownX.update (dt, 30.0f);
    moved |= shownY.update (dt, 30.0f);
    moved |= press.update (dt, 18.0f);
    return moved;
}

void XYPad::paint (juce::Graphics& g)
{
    auto& skin = Skin::of (*this);
    if (! initialised || ! isShowing())
    {
        shownX.snap (cutoff.getValue());
        shownY.snap (resonance.getValue());
        initialised = true;
    }
    XYLook look;
    look.x = shownX.value;
    look.y = shownY.value;
    look.on = bypass.getValue() < 0.5f;
    look.press = press.value;
    look.info = look.on ? cutoff.getCurrentValueAsText() + "   Res " + juce::String (resonance.convertFrom0to1 (resonance.getValue()), 2)
                        : juce::String ("Bypassed - touch to engage");
    skin.drawXYPad (g, padArea(), look, skin.pal().filter);
}

void XYPad::setFrom (juce::Point<float> p)
{
    const auto r = padArea().reduced (16.0f);
    cutoff.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (p.x - r.getX()) / r.getWidth()));
    resonance.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (r.getBottom() - p.y) / r.getHeight()));
}

void XYPad::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    if (bypass.getValue() > 0.5f)
    {
        bypass.beginChangeGesture();
        bypass.setValueNotifyingHost (0.0f);
        bypass.endChangeGesture();
    }
    cutoff.beginChangeGesture();
    resonance.beginChangeGesture();
    setFrom (e.position);
}

void XYPad::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        setFrom (e.position);
}

void XYPad::mouseUp (const juce::MouseEvent&)
{
    if (! dragging)
        return;
    dragging = false;
    cutoff.endChangeGesture();
    resonance.endChangeGesture();
}

} // namespace ts
