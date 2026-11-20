#include "LookAndFeel.h"

namespace ts
{

TSLookAndFeel::TSLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, Colours::background.withAlpha (0.6f));
    setColour (juce::Slider::rotarySliderFillColourId, Colours::accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, Colours::panelEdge);
    setColour (juce::Slider::thumbColourId, Colours::text);
    setColour (juce::TextButton::buttonColourId, Colours::panelEdge);
    setColour (juce::TextButton::buttonOnColourId, Colours::accent);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::ToggleButton::textColourId, Colours::text);
    setColour (juce::ToggleButton::tickColourId, Colours::accent);
    setColour (juce::TextEditor::backgroundColourId, Colours::background);
    setColour (juce::TextEditor::textColourId, Colours::text);
    setColour (juce::TextEditor::highlightColourId, Colours::accent.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Colours::accent);
}

juce::Font TSLookAndFeel::font (float size, bool bold)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font TSLookAndFeel::getLabelFont (juce::Label& l)
{
    return font (juce::jmin (14.0f, (float) l.getHeight() * 0.8f));
}

juce::Font TSLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return font (juce::jmin (14.0f, (float) h * 0.45f), true);
}

void TSLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start,
                                      float end, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float track = juce::jmax (3.0f, radius * 0.14f);
    const auto fill = s.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, start, end, true);
    g.setColour (Colours::panelEdge);
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, start, angle, true);
    g.setColour (s.isEnabled() ? fill : Colours::dim);
    g.strokePath (val, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = radius - track * 2.2f;
    g.setColour (Colours::panel.brighter (0.15f));
    g.fillEllipse (juce::Rectangle<float> (knobR * 2, knobR * 2).withCentre (centre));

    const juce::Point<float> tip (centre.x + std::sin (angle) * knobR * 0.8f, centre.y - std::cos (angle) * knobR * 0.8f);
    g.setColour (Colours::text);
    g.drawLine ({ centre, tip }, 2.5f);
}

void TSLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& colour, bool over,
                                          bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : colour;
    if (down)
        c = c.brighter (0.25f);
    else if (over)
        c = c.brighter (0.1f);
    if (! b.isEnabled())
        c = c.withAlpha (0.4f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 6.0f);
}

void TSLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto tint = b.findColour (juce::ToggleButton::tickColourId);
    g.setColour (on ? tint.withAlpha (0.9f) : Colours::panelEdge.brighter (over ? 0.1f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? juce::Colours::black : (b.isEnabled() ? Colours::text : Colours::dim));
    g.setFont (font (juce::jmin (13.0f, r.getHeight() * 0.5f), true));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().reduced (4, 0), juce::Justification::centred, 1);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint)
{
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (tint);
        g.setFont (TSLookAndFeel::font (12.0f, true));
        g.drawText (title.toUpperCase(), r.reduced (12.0f, 6.0f).removeFromTop (16.0f), juce::Justification::left);
    }
}

} // namespace ts
