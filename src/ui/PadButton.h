#pragma once

#include "LookAndFeel.h"
#include <functional>

namespace ts
{

// A big touch-style pad. Reports press and release separately so it can be
// used for momentary actions (repeater, tape stop) as well as taps.
class PadButton final : public juce::Component
{
public:
    explicit PadButton (juce::String text, juce::Colour c) : label (std::move (text)), tint (c)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    std::function<void (const juce::ModifierKeys&)> onPress;
    std::function<void()> onRelease;

    void setLabel (const juce::String& t) { if (t != label) { label = t; repaint(); } }
    void setSubLabel (const juce::String& t) { if (t != sub) { sub = t; repaint(); } }
    void setLit (bool b) { if (b != lit) { lit = b; repaint(); } }
    void setFilled (bool b) { if (b != filled) { filled = b; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        const bool active = lit || pressed;
        juce::Colour base = filled ? tint.withAlpha (0.35f) : Colours::panelEdge;
        if (active)
            base = tint;
        g.setColour (base);
        g.fillRoundedRectangle (r, 8.0f);
        if (filled && ! active)
        {
            g.setColour (tint);
            g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 1.5f);
        }

        g.setColour (active ? juce::Colours::black : Colours::text);
        auto textArea = r.reduced (4.0f);
        if (sub.isNotEmpty())
        {
            g.setFont (TSLookAndFeel::font (10.0f));
            g.setColour (active ? juce::Colours::black.withAlpha (0.7f) : Colours::dim);
            g.drawText (sub, textArea.removeFromBottom (14.0f), juce::Justification::centred);
            g.setColour (active ? juce::Colours::black : Colours::text);
        }
        g.setFont (TSLookAndFeel::font (juce::jmin (18.0f, textArea.getHeight() * 0.45f), true));
        g.drawFittedText (label, textArea.toNearestInt(), juce::Justification::centred, 2);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        pressed = true;
        repaint();
        if (onPress)
            onPress (e.mods);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        pressed = false;
        repaint();
        if (onRelease)
            onRelease();
    }

private:
    juce::String label, sub;
    juce::Colour tint;
    bool pressed = false, lit = false, filled = false;
};

} // namespace ts
