#include "Skin.h"
#include "Skins.h"

namespace ts
{

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;
} // namespace

const juce::String Skin::defaultName { "Studio Ink" };

juce::StringArray Skin::names()
{
    return { "Studio Ink", "Liquid Glass", "Retro '97" };
}

std::unique_ptr<Skin> Skin::create (const juce::String& name)
{
    if (name == "Liquid Glass")
        return std::make_unique<LiquidGlassSkin>();
    if (name == "Retro '97")
        return std::make_unique<Retro97Skin>();
    return std::make_unique<StudioInkSkin>();
}

Skin& Skin::of (juce::Component& c)
{
    if (auto* s = dynamic_cast<Skin*> (&c.getLookAndFeel()))
        return *s;
    static StudioInkSkin fallback;
    return fallback;
}

Skin::Skin (const Palette& p) : palette (p)
{
    applyColours();
}

void Skin::applyColours()
{
    const auto& p = palette;
    setColour (juce::ResizableWindow::backgroundColourId, p.background);
    setColour (juce::Label::textColourId, p.text);
    setColour (juce::Label::textWhenEditingColourId, p.text);
    setColour (juce::Slider::textBoxTextColourId, p.text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, p.transport.withAlpha (0.4f));
    setColour (juce::Slider::rotarySliderFillColourId, p.transport);
    setColour (juce::TextButton::buttonColourId, p.padIdle);
    setColour (juce::TextButton::buttonOnColourId, p.transport);
    setColour (juce::TextButton::textColourOffId, p.text);
    setColour (juce::TextButton::textColourOnId, p.background);
    setColour (juce::ToggleButton::textColourId, p.text);
    setColour (juce::TextEditor::backgroundColourId, p.screen);
    setColour (juce::TextEditor::textColourId, p.screenText);
    setColour (juce::TextEditor::highlightColourId, p.transport.withAlpha (0.4f));
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, p.transport);
    setColour (juce::CaretComponent::caretColourId, p.transport);
    setColour (juce::ComboBox::textColourId, p.text);
    setColour (juce::ComboBox::backgroundColourId, p.padIdle);
    setColour (juce::ComboBox::arrowColourId, p.dim);
    setColour (juce::ComboBox::outlineColourId, p.panelEdge);
    setColour (juce::PopupMenu::backgroundColourId, p.panel);
    setColour (juce::PopupMenu::textColourId, p.text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.transport);
    setColour (juce::PopupMenu::highlightedTextColourId, p.background);
}

juce::Font Skin::font (float size, bool bold) const
{
    return juce::Font (juce::FontOptions ("Avenir Next", bold ? "Demi Bold" : "Medium", size));
}

// ---------------------------------------------------------------- helpers

void Skin::fillGlow (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float amount, float spread, float corner)
{
    if (amount <= 0.001f)
        return;
    constexpr int steps = 5;
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / steps;
        g.setColour (c.withAlpha (amount * 0.09f * (1.0f - t * 0.6f)));
        g.fillRoundedRectangle (r.expanded (spread * t), corner + spread * t);
    }
}

void Skin::drawVinyl (juce::Graphics& g, juce::Point<float> c, float R, const PlatterLook& look, juce::Colour label,
                      juce::Colour labelText, const juce::Font& labelFont)
{
    // Record.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1c20), c.x, c.y, juce::Colour (0xff070708), c.x + R, c.y + R, true));
    g.fillEllipse (juce::Rectangle<float> (R * 2, R * 2).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (float r = R * 0.42f; r < R * 0.97f; r += 2.6f)
        g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 0.5f);

    // Two light reflections fixed in space (they do not turn with the record).
    juce::Path sheen;
    sheen.addPieSegment (juce::Rectangle<float> (R * 2, R * 2).withCentre (c), -0.55f, 0.25f, 0.4f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillPath (sheen);
    sheen.applyTransform (juce::AffineTransform::rotation (pi, c.x, c.y));
    g.fillPath (sheen);

    // Turning part: label, text and a marker stripe so the speed is visible.
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation ((float) look.angle, c.x, c.y));

    const float L = R * 0.36f;
    g.setColour (look.hasSample ? label : label.withSaturation (0.0f).withMultipliedBrightness (0.45f));
    g.fillEllipse (juce::Rectangle<float> (L * 2, L * 2).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawEllipse (juce::Rectangle<float> (L * 1.6f, L * 1.6f).withCentre (c), 1.0f);

    g.setColour (labelText);
    g.setFont (labelFont.withHeight (juce::jmax (10.0f, L * 0.17f)));
    g.drawFittedText (look.hasSample ? look.name : juce::String ("NO SAMPLE"),
                      juce::Rectangle<float> (L * 1.5f, L * 0.5f).withCentre (c.translated (0, -L * 0.42f)).toNearestInt(),
                      juce::Justification::centred, 2, 0.6f);

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillRoundedRectangle (juce::Rectangle<float> (3.5f, R * 0.5f).withPosition (c.x - 1.75f, c.y - R * 0.97f), 1.75f);
}

// ---------------------------------------------------------------- window pieces

void Skin::drawBackground (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setGradientFill (juce::ColourGradient (palette.background.brighter (0.04f), r.getX(), r.getY(),
                                             palette.background.darker (0.2f), r.getX(), r.getBottom(), false));
    g.fillRect (r);
}

void Skin::drawTitle (juce::Graphics& g, juce::Rectangle<float> a)
{
    g.setColour (palette.text);
    g.setFont (font (20.0f, true));
    g.drawText ("TURNTABLE", a.removeFromTop (a.getHeight() * 0.55f), juce::Justification::bottomLeft);
    g.setColour (palette.transport);
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("SAMPLER", a, juce::Justification::topLeft);
}

void Skin::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint)
{
    g.setColour (palette.panel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (palette.panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    if (title.isNotEmpty())
    {
        auto t = r.reduced (14.0f, 8.0f).removeFromTop (16.0f);
        g.setColour (tint);
        g.fillEllipse (t.removeFromLeft (6.0f).withSizeKeepingCentre (6.0f, 6.0f));
        t.removeFromLeft (7.0f);
        g.setColour (palette.text.withAlpha (0.9f));
        g.setFont (labelFont (11.5f).withExtraKerningFactor (0.12f));
        g.drawText (caps (title), t, juce::Justification::centredLeft);
    }
}

void Skin::drawSmallLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Justification j)
{
    g.setColour (palette.dim);
    g.setFont (labelFont (10.5f).withExtraKerningFactor (0.08f));
    g.drawText (caps (text), r, j);
}

void Skin::drawScreen (juce::Graphics& g, juce::Rectangle<float> r, const juce::StringArray& lines, juce::Justification j,
                       bool warning)
{
    // Line 0 small, line 1 big (the main value), line 2 small.
    const float heights[] = { 14.0f, 24.0f, 14.0f };
    const float sizes[] = { 11.5f, 21.0f, 11.0f };
    for (int i = 0; i < lines.size() && i < 3; ++i)
    {
        auto row = r.removeFromTop (heights[i]);
        g.setColour (i == 1 ? (warning ? palette.danger : palette.text) : palette.dim);
        g.setFont (screenFont (sizes[i], i == 1));
        g.drawText (lines[i], row, j);
    }
}

void Skin::drawStatusBar (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& left, const juce::String& right,
                          bool warning)
{
    g.setFont (font (12.0f));
    g.setColour (warning ? palette.danger : palette.dim);
    g.drawText (left, r, juce::Justification::centredLeft);
    g.setColour (palette.dim);
    g.drawText (right, r, juce::Justification::centredRight);
}

void Skin::drawMeter (juce::Graphics& g, juce::Rectangle<float> m, float left, float right)
{
    g.setColour (palette.padIdle);
    g.fillRoundedRectangle (m, 3.0f);
    auto bar = [&] (juce::Rectangle<float> b, float level) {
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
        auto filled = b.removeFromBottom (b.getHeight() * frac);
        g.setGradientFill (juce::ColourGradient (palette.filter, 0.0f, b.getBottom(), palette.danger, 0.0f, b.getY(), false));
        g.fillRect (filled);
    };
    auto l = m.reduced (2.0f);
    auto r = l.removeFromRight (l.getWidth() * 0.5f);
    bar (l.withTrimmedRight (1.0f), left);
    bar (r.withTrimmedLeft (1.0f), right);
}

void Skin::drawOverlay (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (palette.background.withAlpha (0.94f));
    g.fillRect (r);
}

// ---------------------------------------------------------------- controls

void Skin::drawPad (juce::Graphics& g, juce::Rectangle<float> r, const PadLook& p)
{
    const bool small = p.kind == PadLook::Kind::Small;
    const float corner = small ? 7.0f : 10.0f;
    r = r.reduced (2.0f);

    fillGlow (g, r, p.tint, juce::jmax (p.lit * 0.8f, p.glow), small ? 2.5f : 6.0f, corner);

    auto base = palette.padIdle.brighter (p.hover * 0.12f);
    g.setColour (base);
    g.fillRoundedRectangle (r, corner);

    // Lit pads glow from underneath.
    const float lit = juce::jmax (p.lit, p.glow * 0.7f);
    if (lit > 0.001f)
    {
        g.setGradientFill (juce::ColourGradient (p.tint.withAlpha (lit), r.getCentreX(), r.getBottom(),
                                                 p.tint.withAlpha (lit * 0.55f), r.getCentreX(), r.getY(), false));
        g.fillRoundedRectangle (r, corner);
    }
    else if (p.filled)
    {
        g.setColour (p.tint.withAlpha (0.16f));
        g.fillRoundedRectangle (r, corner);
    }

    g.setColour (p.filled || lit > 0.5f ? p.tint.withAlpha (0.9f) : palette.panelEdge.brighter (p.hover * 0.3f));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, p.filled ? 1.5f : 1.0f);

    // Pressed: a slight inner shade.
    if (p.press > 0.001f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f * p.press));
        g.fillRoundedRectangle (r, corner);
    }

    const auto textCol = palette.text.interpolatedWith (palette.background, juce::jlimit (0.0f, 1.0f, lit * 1.2f));
    auto text = r.reduced (4.0f);
    if (p.sub.isNotEmpty() && ! small)
    {
        g.setColour (textCol.withAlpha (0.6f));
        g.setFont (font (10.0f));
        g.drawText (p.sub, text.removeFromBottom (16.0f), juce::Justification::centred);
    }
    g.setColour (p.enabled ? textCol : textCol.withAlpha (0.4f));
    g.setFont (font (small ? juce::jmin (13.0f, text.getHeight() * 0.55f) : juce::jmin (19.0f, text.getHeight() * 0.42f, text.getWidth() * 0.5f), true));
    g.drawFittedText (p.label, text.toNearestInt(), juce::Justification::centred, 2, 0.6f);
}

void Skin::drawToggle (juce::Graphics& g, juce::Rectangle<float> r, const ToggleLook& t)
{
    r = r.reduced (1.0f);
    g.setColour (palette.padIdle.brighter (t.hover * 0.12f));
    g.fillRoundedRectangle (r, 7.0f);
    if (t.on > 0.001f)
    {
        g.setColour (t.tint.withAlpha (t.on));
        g.fillRoundedRectangle (r, 7.0f);
    }
    g.setColour (palette.panelEdge.interpolatedWith (t.tint, t.on));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);

    const auto textCol = palette.text.interpolatedWith (palette.background, t.on);
    g.setColour (t.enabled ? textCol : textCol.withAlpha (0.4f));
    g.setFont (labelFont (juce::jmin (12.0f, r.getHeight() * 0.48f)));
    g.drawFittedText (caps (t.text), r.toNearestInt().reduced (4, 0), juce::Justification::centred, 1, 0.6f);
}

void Skin::drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float pos, float hover, juce::Colour tint, bool enabled)
{
    const float start = pi * 1.2f, end = pi * 2.8f;
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float track = juce::jmax (3.0f, radius * 0.13f);
    const auto col = enabled ? tint : palette.dim;

    juce::Path bg;
    bg.addCentredArc (c.x, c.y, radius - track, radius - track, 0.0f, start, end, true);
    g.setColour (palette.padIdle.brighter (0.1f));
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, radius - track, radius - track, 0.0f, start, angle, true);
    g.setColour (col.brighter (hover * 0.25f));
    g.strokePath (val, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float capR = radius - track * 2.3f;
    auto cap = juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c);
    g.setGradientFill (juce::ColourGradient (palette.panel.brighter (0.35f + hover * 0.1f), cap.getX(), cap.getY(),
                                             palette.panel.brighter (0.05f), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (cap, 1.0f);

    const juce::Point<float> tip (c.x + std::sin (angle) * capR * 0.78f, c.y - std::cos (angle) * capR * 0.78f);
    g.setColour (palette.text);
    g.drawLine ({ c.getPointOnCircumference (capR * 0.25f, angle), tip }, 2.5f);
}

void Skin::drawPlatter (juce::Graphics& g, juce::Rectangle<float> bounds, const PlatterLook& look)
{
    const auto c = bounds.getCentre();
    const float R = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 18.0f;

    g.setColour (palette.panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (palette.panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);

    // Soft spotlight on the deck.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), c.x - R * 0.5f, c.y - R * 0.7f,
                                             juce::Colours::transparentWhite, c.x + R, c.y + R, true));
    g.fillEllipse (juce::Rectangle<float> ((R + 14) * 2, (R + 14) * 2).withCentre (c));

    // Platter with strobe dots that turn with it.
    g.setColour (juce::Colour (0xff2a2c33));
    g.fillEllipse (juce::Rectangle<float> ((R + 10) * 2, (R + 10) * 2).withCentre (c));
    for (int i = 0; i < 72; ++i)
    {
        const float a = (float) look.angle + twoPi * (float) i / 72.0f;
        g.setColour (juce::Colour (0xff8a8f9c).withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (2.6f, 2.6f).withCentre (c.getPointOnCircumference (R + 5.5f, a)));
    }

    if (look.scratch > 0.001f)
    {
        g.setColour (palette.transport.withAlpha (look.scratch));
        g.drawEllipse (juce::Rectangle<float> ((R + 10) * 2, (R + 10) * 2).withCentre (c), 3.0f);
    }

    drawVinyl (g, c, R, look, palette.transport, palette.background, font (12.0f, true));

    g.setColour (juce::Colour (0xffd6d8de));
    g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre (c));

    if (look.message.isNotEmpty())
    {
        g.setColour (palette.text.withAlpha (0.9f));
        g.setFont (font (15.0f, true));
        g.drawFittedText (look.message, juce::Rectangle<float> (R * 1.4f, 50.0f).withCentre (c.translated (0, R * 0.6f)).toNearestInt(),
                          juce::Justification::centred, 2, 0.6f);
    }

    if (look.dropHover)
    {
        g.setColour (palette.transport);
        g.drawRoundedRectangle (bounds.reduced (2.0f), 14.0f, 3.0f);
    }
}

void Skin::drawWaveformFrame (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (palette.panel);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (palette.panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
}

void Skin::drawXYPad (juce::Graphics& g, juce::Rectangle<float> r, const XYLook& xy, juce::Colour tint)
{
    const auto t = xy.on ? tint : palette.dim;
    g.setGradientFill (juce::ColourGradient (t.withAlpha (0.16f), r.getX(), r.getBottom(), t.withAlpha (0.03f), r.getRight(), r.getY(), false));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (t.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

    g.setColour (palette.text.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i)
    {
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 4.0f), r.getY(), r.getBottom());
        g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX(), r.getRight());
    }

    g.setFont (labelFont (10.5f));
    g.setColour (palette.dim);
    g.drawText (caps ("Cutoff  >"), r.reduced (10.0f).removeFromBottom (14.0f), juce::Justification::right);
    g.drawText (caps ("^  Resonance"), r.reduced (10.0f).removeFromTop (16.0f), juce::Justification::left);

    const auto dot = r.reduced (14.0f);
    const juce::Point<float> p (dot.getX() + xy.x * dot.getWidth(), dot.getBottom() - xy.y * dot.getHeight());
    // Crosshair.
    g.setColour (t.withAlpha (0.18f));
    g.drawVerticalLine ((int) p.x, r.getY() + 2.0f, r.getBottom() - 2.0f);
    g.drawHorizontalLine ((int) p.y, r.getX() + 2.0f, r.getRight() - 2.0f);
    const float halo = 40.0f + 14.0f * xy.press;
    g.setColour (t.withAlpha (0.2f + 0.1f * xy.press));
    g.fillEllipse (juce::Rectangle<float> (halo, halo).withCentre (p));
    g.setColour (t);
    g.fillEllipse (juce::Rectangle<float> (16, 16).withCentre (p));

    g.setColour (palette.text);
    g.setFont (font (12.0f));
    g.drawText (xy.info, r.reduced (10.0f).removeFromTop (16.0f), juce::Justification::right);
}

// ---------------------------------------------------------------- LookAndFeel hooks

void Skin::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider& s)
{
    drawKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat(), pos, 0.0f, s.findColour (juce::Slider::rotarySliderFillColourId),
              s.isEnabled());
}

void Skin::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    ToggleLook t;
    t.tint = palette.transport;
    t.hover = over ? 1.0f : 0.0f;
    t.press = down ? 1.0f : 0.0f;
    t.enabled = b.isEnabled();
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (palette.padIdle.brighter (over ? 0.12f : 0.0f).darker (down ? 0.2f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (palette.panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
}

void Skin::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (b.isEnabled() ? palette.text : palette.dim);
    g.setFont (font (juce::jmin (14.0f, (float) b.getHeight() * 0.6f), true));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2), juce::Justification::centred, 1, 0.6f);
}

void Skin::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    ToggleLook t;
    t.text = b.getButtonText();
    t.tint = b.findColour (juce::ToggleButton::tickColourId);
    t.on = b.getToggleState() ? 1.0f : 0.0f;
    t.hover = over ? 1.0f : 0.0f;
    t.press = down ? 1.0f : 0.0f;
    t.enabled = b.isEnabled();
    drawToggle (g, b.getLocalBounds().toFloat(), t);
}

juce::Font Skin::getLabelFont (juce::Label& l)
{
    return font (juce::jmin (14.0f, (float) l.getHeight() * 0.75f), true);
}

juce::Font Skin::getTextButtonFont (juce::TextButton&, int h)
{
    return font (juce::jmin (14.0f, (float) h * 0.5f), true);
}

void Skin::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
        return;
    // Slider value boxes use the skin's display font.
    const bool valueBox = dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr;
    g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.4f));
    g.setFont (valueBox ? screenFont (juce::jmin (14.0f, (float) label.getHeight() * 0.8f)) : getLabelFont (label));
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0), label.getJustificationType(), 1);
}

void Skin::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (palette.padIdle.brighter (box.isMouseOver (true) ? 0.1f : 0.0f));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (palette.panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);

    juce::Path arrow;
    const float ax = r.getRight() - 14.0f, ay = r.getCentreY();
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (palette.dim);
    g.fillPath (arrow);
}

juce::Font Skin::getComboBoxFont (juce::ComboBox&) { return font (12.5f, true); }
juce::Font Skin::getPopupMenuFont() { return font (14.0f); }

void Skin::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void Skin::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (palette.panel);
    g.setColour (palette.panelEdge);
    g.drawRect (0, 0, w, h);
}

} // namespace ts
