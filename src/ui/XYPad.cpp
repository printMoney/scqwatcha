#include "XYPad.h"

namespace ts
{

XYPad::XYPad (juce::RangedAudioParameter& c, juce::RangedAudioParameter& r, juce::RangedAudioParameter& b)
    : cutoff (c), resonance (r), bypass (b)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

juce::Rectangle<float> XYPad::padArea() const
{
    return getLocalBounds().toFloat().reduced (2.0f);
}

void XYPad::refresh()
{
    const float x = cutoff.getValue(), y = resonance.getValue();
    const bool b = bypass.getValue() > 0.5f;
    if (x != lastX || y != lastY || b != lastBypass)
    {
        lastX = x;
        lastY = y;
        lastBypass = b;
        repaint();
    }
}

void XYPad::paint (juce::Graphics& g)
{
    const auto r = padArea();
    const bool on = bypass.getValue() < 0.5f;
    const auto tint = on ? Colours::filter : Colours::dim;

    g.setGradientFill (juce::ColourGradient (tint.withAlpha (0.18f), r.getX(), r.getBottom(),
                                             tint.withAlpha (0.04f), r.getRight(), r.getY(), false));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (tint.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i)
    {
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * i / 4.0f), r.getY(), r.getBottom());
        g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * i / 4.0f), r.getX(), r.getRight());
    }

    g.setFont (TSLookAndFeel::font (11.0f, true));
    g.setColour (Colours::dim);
    g.drawText ("CUTOFF  >", r.reduced (10.0f).removeFromBottom (14.0f), juce::Justification::right);
    g.drawText ("^  RESONANCE", r.reduced (10.0f).removeFromTop (16.0f), juce::Justification::left);

    const auto dotArea = r.reduced (14.0f);
    const float px = dotArea.getX() + cutoff.getValue() * dotArea.getWidth();
    const float py = dotArea.getBottom() - resonance.getValue() * dotArea.getHeight();
    g.setColour (tint.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (44, 44).withCentre ({ px, py }));
    g.setColour (tint);
    g.fillEllipse (juce::Rectangle<float> (18, 18).withCentre ({ px, py }));

    g.setColour (Colours::text);
    g.setFont (TSLookAndFeel::font (12.0f));
    const auto info = on ? cutoff.getCurrentValueAsText() + "   Res " + juce::String (resonance.convertFrom0to1 (resonance.getValue()), 2)
                         : juce::String ("BYPASSED - touch to engage");
    g.drawText (info, r.reduced (10.0f).removeFromTop (16.0f), juce::Justification::right);
}

void XYPad::setFrom (juce::Point<float> p)
{
    const auto r = padArea().reduced (14.0f);
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
