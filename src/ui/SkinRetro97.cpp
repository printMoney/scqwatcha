#include "Skins.h"

namespace ts
{

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

const juce::Colour ink (0xff2b2925);        // printed text on plastic
const juce::Colour rubber (0xff3a393b);     // pads
const juce::Colour rubberText (0xffdedad2);
const juce::Colour key (0xffe4e0d6);        // light plastic keys
const juce::Colour amber (0xffffa733);
const juce::Colour lcdBack (0xff1b1309);

Palette retroPalette()
{
    Palette p;
    p.background = juce::Colour (0xffc9c4b8);
    p.panel = juce::Colour (0xffbbb5a8);
    p.panelEdge = juce::Colour (0xff8d877a);
    p.text = ink;
    p.dim = juce::Colour (0xff6b665c);
    p.transport = juce::Colour (0xffff6f1a);
    p.loop = juce::Colour (0xff36a3e8);
    p.repeater = juce::Colour (0xffd85bc4);
    p.filter = juce::Colour (0xff2fc28d);
    p.danger = juce::Colour (0xffe8382c);
    p.padIdle = rubber;
    p.screen = lcdBack;
    p.screenText = amber;
    return p;
}

juce::String katakanaSubtitle()
{
    return juce::String (juce::CharPointer_UTF8 (
        "\xe3\x82\xb5\xe3\x83\xb3\xe3\x83\x97\xe3\x83\xaa\xe3\x83\xb3\xe3\x82\xb0\xe3\x83\xbb\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\xb3"
        "\xe3\x83\x86\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xab\xe3\x83\xbb\xe3\x82\xb7\xe3\x82\xb9\xe3\x83\x86\xe3\x83\xa0"));
}
} // namespace

Retro97Skin::Retro97Skin() : Skin (retroPalette())
{
    setColour (juce::Slider::textBoxTextColourId, amber);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::PopupMenu::backgroundColourId, key);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, palette.transport);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ComboBox::textColourId, amber);
}

juce::Font Retro97Skin::font (float size, bool) const
{
    return juce::Font (juce::FontOptions ("DIN Alternate", "Bold", size));
}

juce::Font Retro97Skin::labelFont (float size) const
{
    return juce::Font (juce::FontOptions ("DIN Alternate", "Bold", size)).withExtraKerningFactor (0.1f);
}

juce::Font Retro97Skin::screenFont (float size, bool bold) const
{
    return juce::Font (juce::FontOptions ("Menlo", bold ? "Bold" : "Regular", size));
}

// ---------------------------------------------------------------- helpers

void Retro97Skin::lcdTexture (juce::Graphics& g, juce::Rectangle<float> r) const
{
    // Dot-matrix grid and scan lines on top of the text.
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (lcdBack.withAlpha (0.45f));
    for (float x = r.getX(); x < r.getRight(); x += 2.0f)
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    g.setColour (lcdBack.withAlpha (0.3f));
    for (float y = r.getY(); y < r.getBottom(); y += 2.0f)
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    // Glass reflection.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),
                                             juce::Colours::white.withAlpha (0.0f), r.getX(), r.getCentreY(), false));
    g.fillRect (r);
}

void Retro97Skin::lcd (juce::Graphics& g, juce::Rectangle<float> r) const
{
    // Bezel: dark above, light below, so the screen looks set into the plastic.
    g.setColour (juce::Colour (0xff6f695e));
    g.fillRoundedRectangle (r.expanded (2.0f), 4.0f);
    g.setColour (juce::Colour (0xffe6e1d6));
    g.drawLine (r.getX(), r.getBottom() + 2.0f, r.getRight(), r.getBottom() + 2.0f, 1.0f);
    g.setGradientFill (juce::ColourGradient (lcdBack.brighter (0.15f), r.getCentreX(), r.getCentreY(), lcdBack, r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, 3.0f);
}

void Retro97Skin::screw (juce::Graphics& g, juce::Point<float> c, float angle) const
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9e6df), c.x - 3, c.y - 3, juce::Colour (0xff8c877c), c.x + 3, c.y + 3, false));
    g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre (c));
    g.setColour (juce::Colour (0xff5d584f));
    g.drawEllipse (juce::Rectangle<float> (8, 8).withCentre (c), 0.7f);
    g.drawLine ({ c.getPointOnCircumference (3.2f, angle), c.getPointOnCircumference (3.2f, angle + pi) }, 1.2f);
}

// ---------------------------------------------------------------- window pieces

void Retro97Skin::drawBackground (juce::Graphics& g, juce::Rectangle<float> r)
{
    const int w = juce::roundToInt (r.getWidth()), h = juce::roundToInt (r.getHeight());
    if (! plastic.isValid() || plastic.getWidth() != w || plastic.getHeight() != h)
    {
        // Textured plastic, rendered once.
        plastic = juce::Image (juce::Image::RGB, juce::jmax (1, w), juce::jmax (1, h), true);
        juce::Graphics pg (plastic);
        pg.setGradientFill (juce::ColourGradient (palette.background.brighter (0.06f), 0.0f, 0.0f,
                                                  palette.background.darker (0.08f), 0.0f, (float) h, false));
        pg.fillAll();
        juce::Random rng (97);
        for (int i = 0; i < w * h / 6; ++i)
        {
            pg.setColour (rng.nextBool() ? juce::Colours::white.withAlpha (0.05f) : juce::Colours::black.withAlpha (0.05f));
            pg.fillRect ((float) rng.nextInt (w), (float) rng.nextInt (h), 1.0f, 1.0f);
        }
        // Moulding seams.
        pg.setColour (juce::Colours::black.withAlpha (0.18f));
        pg.drawHorizontalLine (78, 0.0f, (float) w);
        pg.setColour (juce::Colours::white.withAlpha (0.4f));
        pg.drawHorizontalLine (79, 0.0f, (float) w);
        // Case edge.
        pg.setColour (juce::Colour (0xff57534b));
        pg.drawRect (0, 0, w, h, 3);
    }
    g.drawImageAt (plastic, juce::roundToInt (r.getX()), juce::roundToInt (r.getY()));

    screw (g, { r.getX() + 9.0f, r.getY() + 9.0f }, 0.6f);
    screw (g, { r.getRight() - 9.0f, r.getY() + 9.0f }, 2.1f);
    screw (g, { r.getX() + 9.0f, r.getBottom() - 9.0f }, 1.3f);
    screw (g, { r.getRight() - 9.0f, r.getBottom() - 9.0f }, 0.2f);
}

void Retro97Skin::drawTitle (juce::Graphics& g, juce::Rectangle<float> a)
{
    auto top = a.removeFromTop (a.getHeight() * 0.6f);
    const auto nameFont = juce::Font (juce::FontOptions ("DIN Alternate", "Bold", 24.0f)).withExtraKerningFactor (0.03f);
    const float nameW = juce::GlyphArrangement::getStringWidth (nameFont, "SCQWATCHA") + 8.0f;
    g.setColour (ink);
    g.setFont (nameFont);
    g.drawText ("SCQWATCHA", top.removeFromLeft (nameW), juce::Justification::bottomLeft);
    // Model badge.
    auto badge = top.removeFromLeft (38.0f).withTrimmedBottom (4.0f).removeFromBottom (13.0f);
    g.setColour (palette.transport);
    g.fillRoundedRectangle (badge, 2.0f);
    g.setColour (juce::Colours::black);
    g.setFont (labelFont (9.5f));
    g.drawText ("TT-97", badge, juce::Justification::centred);
    g.setColour (palette.dim);
    g.setFont (juce::Font (juce::FontOptions ("Hiragino Sans", "W3", 8.5f)));
    g.drawText (katakanaSubtitle(), a, juce::Justification::topLeft);
}

void Retro97Skin::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint)
{
    // Recessed area.
    g.setColour (palette.panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (r.getX() + 4, r.getY() + 0.5f, r.getRight() - 4, r.getY() + 0.5f, 1.5f);
    g.drawLine (r.getX() + 0.5f, r.getY() + 4, r.getX() + 0.5f, r.getBottom() - 4, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawLine (r.getX() + 4, r.getBottom() - 0.5f, r.getRight() - 4, r.getBottom() - 0.5f, 1.0f);
    g.drawLine (r.getRight() - 0.5f, r.getY() + 4, r.getRight() - 0.5f, r.getBottom() - 4, 1.0f);

    if (title.isNotEmpty())
    {
        auto t = r.reduced (12.0f, 7.0f).removeFromTop (16.0f);
        g.setColour (tint);
        g.fillRect (t.removeFromLeft (8.0f).withSizeKeepingCentre (8.0f, 8.0f));
        t.removeFromLeft (6.0f);
        g.setColour (ink);
        g.setFont (labelFont (12.0f));
        const auto text = title.toUpperCase();
        g.drawText (text, t, juce::Justification::centredLeft);
        // Printed rule after the title.
        const float tw = juce::GlyphArrangement::getStringWidth (labelFont (12.0f), text) + 8.0f;
        g.setColour (ink.withAlpha (0.35f));
        g.drawHorizontalLine ((int) t.getCentreY(), t.getX() + tw, t.getRight());
    }
}

void Retro97Skin::drawSmallLabel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Justification j)
{
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (labelFont (10.5f));
    g.drawText (text.toUpperCase(), r, j);
}

void Retro97Skin::drawScreen (juce::Graphics& g, juce::Rectangle<float> r, const juce::StringArray& lines, juce::Justification j,
                              bool warning)
{
    lcd (g, r);
    auto inner = r.reduced (6.0f, 4.0f);
    const float heights[] = { 13.0f, 22.0f, 13.0f };
    const float sizes[] = { 10.0f, 18.0f, 10.0f };
    for (int i = 0; i < lines.size() && i < 3; ++i)
    {
        auto row = inner.removeFromTop (heights[i]);
        const auto col = warning && i == 1 ? palette.danger : amber;
        // Glow behind the characters.
        g.setColour (col.withAlpha (0.25f));
        g.setFont (screenFont (sizes[i], i == 1));
        g.drawText (lines[i].toUpperCase(), row.translated (0.0f, 0.6f), j);
        g.setColour (col.withAlpha (i == 1 ? 1.0f : 0.75f));
        g.drawText (lines[i].toUpperCase(), row, j);
    }
    lcdTexture (g, r);
}

void Retro97Skin::drawStatusBar (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& left, const juce::String& right,
                                 bool warning)
{
    lcd (g, r);
    g.setFont (screenFont (11.5f));
    g.setColour (warning ? palette.danger : amber);
    g.drawText (left.toUpperCase(), r.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (amber.withAlpha (0.6f));
    g.drawText (right.toUpperCase(), r.reduced (8.0f, 0.0f), juce::Justification::centredRight);
    lcdTexture (g, r);
}

void Retro97Skin::drawMeter (juce::Graphics& g, juce::Rectangle<float> m, float left, float right)
{
    lcd (g, m);
    auto col = [&] (juce::Rectangle<float> b, float level) {
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
        constexpr int segs = 12;
        const float segH = b.getHeight() / segs;
        for (int i = 0; i < segs; ++i)
        {
            const bool on = (float) (i + 1) / segs <= frac + 0.0001f;
            const auto c = i >= 10 ? palette.danger : (i >= 8 ? amber : palette.filter);
            g.setColour (on ? c : c.withAlpha (0.12f));
            g.fillRect (b.getX(), b.getBottom() - (float) (i + 1) * segH + 1.0f, b.getWidth(), segH - 1.5f);
        }
    };
    auto l = m.reduced (2.0f);
    auto r = l.removeFromRight (l.getWidth() * 0.5f);
    col (l.withTrimmedRight (1.0f), left);
    col (r.withTrimmedLeft (1.0f), right);
}

void Retro97Skin::drawOverlay (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (lcdBack.withAlpha (0.97f));
    g.fillRect (r);
    lcdTexture (g, r);
}

// ---------------------------------------------------------------- controls

void Retro97Skin::drawPad (juce::Graphics& g, juce::Rectangle<float> r, const PadLook& p)
{
    const bool small = p.kind == PadLook::Kind::Small;
    r = r.reduced (2.0f);

    if (small)
    {
        // Light plastic key.
        g.setColour (juce::Colour (0xff6f695e));
        g.fillRoundedRectangle (r.translated (0, 1.5f), 4.0f);
        auto face = r.translated (0, p.press * 1.2f);
        g.setGradientFill (juce::ColourGradient (key.brighter (0.05f + p.hover * 0.05f), face.getX(), face.getY(),
                                                 key.darker (0.12f), face.getX(), face.getBottom(), false));
        g.fillRoundedRectangle (face, 4.0f);
        if (p.lit > 0.001f || p.glow > 0.001f)
        {
            g.setColour (p.tint.withAlpha (juce::jmax (p.lit, p.glow)));
            g.fillRoundedRectangle (face.removeFromTop (3.0f).reduced (6.0f, 0.0f), 1.5f);
        }
        g.setColour (ink);
        g.setFont (labelFont (juce::jmin (12.0f, r.getHeight() * 0.5f)));
        g.drawFittedText (p.label, face.toNearestInt(), juce::Justification::centred, 1, 0.6f);
        return;
    }

    // Rubber pad with backlight bleeding out around the edges.
    const float lit = juce::jmax (p.lit, p.glow * 0.8f);
    fillGlow (g, r, p.tint, lit, 6.0f, 7.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 7.0f);
    auto face = r.translated (0, p.press * 1.5f);
    g.setGradientFill (juce::ColourGradient (rubber.brighter (0.12f + p.hover * 0.08f), face.getX(), face.getY(),
                                             rubber.darker (0.25f), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 7.0f);

    if (lit > 0.001f)
    {
        // Translucent rubber lit from below: brightest in the middle.
        g.setGradientFill (juce::ColourGradient (p.tint.brighter (0.3f).withAlpha (0.95f * lit), face.getCentreX(), face.getCentreY(),
                                                 p.tint.withAlpha (0.55f * lit), face.getX(), face.getY(), true));
        g.fillRoundedRectangle (face, 7.0f);
    }
    else if (p.filled)
    {
        g.setColour (p.tint.withAlpha (0.28f));
        g.fillRoundedRectangle (face.reduced (3.0f), 5.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (face.getX() + 6, face.getY() + 1, face.getRight() - 6, face.getY() + 1, 1.0f);

    const auto tc = lit > 0.5f ? juce::Colour (0xff1a1714) : rubberText;
    auto text = face.reduced (4.0f);
    if (p.sub.isNotEmpty())
    {
        g.setColour (tc.withAlpha (0.6f));
        g.setFont (labelFont (9.5f));
        g.drawText (p.sub.toUpperCase(), text.removeFromBottom (15.0f), juce::Justification::centred);
    }
    g.setColour (p.enabled ? tc : tc.withAlpha (0.4f));
    g.setFont (labelFont (juce::jmin (18.0f, text.getHeight() * 0.4f, text.getWidth() * 0.5f)));
    g.drawFittedText (p.label.toUpperCase(), text.toNearestInt(), juce::Justification::centred, 2, 0.6f);
}

void Retro97Skin::drawToggle (juce::Graphics& g, juce::Rectangle<float> r, const ToggleLook& t)
{
    r = r.reduced (1.5f);
    g.setColour (juce::Colour (0xff6f695e));
    g.fillRoundedRectangle (r.translated (0, 1.5f), 4.0f);
    auto face = r.translated (0, t.press * 1.2f);
    g.setGradientFill (juce::ColourGradient (key.brighter (0.05f + t.hover * 0.05f), face.getX(), face.getY(),
                                             key.darker (0.12f), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 4.0f);

    // LED.
    auto led = face.removeFromLeft (16.0f).withSizeKeepingCentre (7.0f, 7.0f);
    if (t.on > 0.001f)
        fillGlow (g, led, t.tint, t.on, 5.0f, 3.5f);
    g.setColour (t.tint.withAlpha (0.18f + 0.82f * t.on));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (led, 0.8f);

    g.setColour (t.enabled ? ink : ink.withAlpha (0.4f));
    g.setFont (labelFont (juce::jmin (11.0f, r.getHeight() * 0.45f)));
    g.drawFittedText (t.text.toUpperCase(), face.toNearestInt().withTrimmedRight (4), juce::Justification::centred, 2, 0.8f);
}

void Retro97Skin::drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float pos, float hover, juce::Colour tint, bool enabled)
{
    const float start = pi * 1.25f, end = pi * 2.75f;
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);

    // Printed tick ring.
    g.setColour (ink.withAlpha (0.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const float len = (i % 5 == 0) ? 5.0f : 3.0f;
        g.drawLine ({ c.getPointOnCircumference (radius - len, a), c.getPointOnCircumference (radius, a) }, i % 5 == 0 ? 1.6f : 1.0f);
    }
    // Small LED at the current value.
    g.setColour ((enabled ? tint : palette.dim).withAlpha (0.9f));
    g.fillEllipse (juce::Rectangle<float> (4.5f, 4.5f).withCentre (c.getPointOnCircumference (radius - 9.0f, angle)));

    // Ridged cap.
    const float capR = radius - 12.0f;
    auto cap = juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (cap.translated (0, 2.0f));
    g.setColour (juce::Colour (0xff2c2b2d).brighter (hover * 0.1f));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    for (int i = 0; i < 28; ++i)
    {
        const float a = angle + twoPi * (float) i / 28.0f;
        g.drawLine ({ c.getPointOnCircumference (capR * 0.82f, a), c.getPointOnCircumference (capR, a) }, 1.0f);
    }
    auto top = cap.reduced (capR * 0.22f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a494c), top.getX(), top.getY(), juce::Colour (0xff232224), top.getRight(),
                                             top.getBottom(), false));
    g.fillEllipse (top);
    g.setColour (juce::Colour (0xfff2efe8));
    g.drawLine ({ c.getPointOnCircumference (capR * 0.15f, angle), c.getPointOnCircumference (capR * 0.78f, angle) }, 2.2f);
}

void Retro97Skin::drawPlatter (juce::Graphics& g, juce::Rectangle<float> bounds, const PlatterLook& look)
{
    const auto c = bounds.getCentre();
    const float R = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 22.0f;

    drawPanel (g, bounds, {}, palette.transport);
    screw (g, bounds.getTopLeft().translated (11, 11), 0.3f);
    screw (g, bounds.getTopRight().translated (-11, 11), 1.9f);
    screw (g, bounds.getBottomLeft().translated (11, -11), 2.6f);
    screw (g, bounds.getBottomRight().translated (-11, -11), 0.9f);

    // Brushed aluminium platter with two rows of strobe dots.
    const auto plate = juce::Rectangle<float> ((R + 14) * 2, (R + 14) * 2).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillEllipse (plate.translated (0, 3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9e8e4), plate.getX(), plate.getY(), juce::Colour (0xff8e8c86),
                                             plate.getRight(), plate.getBottom(), false));
    g.fillEllipse (plate);
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 60; ++i)
        {
            const float a = (float) look.angle + twoPi * ((float) i + 0.5f * (float) row) / 60.0f;
            g.setColour (juce::Colour (0xff4b4944));
            g.fillRect (juce::Rectangle<float> (2.2f, 2.2f).withCentre (c.getPointOnCircumference (R + 6.5f + 4.5f * (float) row, a)));
        }

    if (look.scratch > 0.001f)
    {
        g.setColour (palette.transport.withAlpha (look.scratch));
        g.drawEllipse (plate, 3.0f);
    }

    drawVinyl (g, c, R, look, palette.transport, ink, labelFont (12.0f));

    g.setColour (juce::Colour (0xffd8d6d0));
    g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre (c));

    // Strobe lamp in the corner.
    const auto lamp = juce::Point<float> (bounds.getX() + 26.0f, bounds.getBottom() - 30.0f);
    fillGlow (g, juce::Rectangle<float> (10, 10).withCentre (lamp), juce::Colour (0xffff4a2a), juce::jlimit (0.0f, 1.0f, look.speed), 8.0f, 5.0f);
    g.setColour (juce::Colour (0xffff4a2a).withAlpha (0.25f + 0.75f * juce::jlimit (0.0f, 1.0f, look.speed)));
    g.fillEllipse (juce::Rectangle<float> (10, 10).withCentre (lamp));

    if (look.message.isNotEmpty())
    {
        auto box = juce::Rectangle<float> (R * 1.3f, 40.0f).withCentre (c.translated (0, R * 0.62f));
        lcd (g, box);
        g.setColour (amber);
        g.setFont (screenFont (12.5f, true));
        g.drawFittedText (look.message.toUpperCase(), box.toNearestInt().reduced (4), juce::Justification::centred, 2, 0.6f);
        lcdTexture (g, box);
    }
    if (look.dropHover)
    {
        g.setColour (palette.transport);
        g.drawRoundedRectangle (bounds.reduced (2.0f), 6.0f, 3.0f);
    }
}

void Retro97Skin::drawWaveformFrame (juce::Graphics& g, juce::Rectangle<float> r)
{
    lcd (g, r);
}

void Retro97Skin::drawXYPad (juce::Graphics& g, juce::Rectangle<float> r, const XYLook& xy, juce::Colour tint)
{
    // A green vector-display screen.
    const auto phosphor = xy.on ? tint : juce::Colour (0xff4c6b5c);
    lcd (g, r);
    g.setColour (phosphor.withAlpha (0.16f));
    for (int i = 1; i < 8; ++i)
    {
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 8.0f), r.getY(), r.getBottom());
        g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 8.0f), r.getX(), r.getRight());
    }
    g.setFont (screenFont (10.0f));
    g.setColour (phosphor.withAlpha (0.7f));
    g.drawText ("CUTOFF >", r.reduced (10.0f).removeFromBottom (14.0f), juce::Justification::right);
    g.drawText ("^ RESO", r.reduced (10.0f).removeFromTop (14.0f), juce::Justification::left);

    const auto dot = r.reduced (14.0f);
    const juce::Point<float> p (dot.getX() + xy.x * dot.getWidth(), dot.getBottom() - xy.y * dot.getHeight());
    g.setColour (phosphor.withAlpha (0.45f));
    g.drawVerticalLine ((int) p.x, r.getY() + 3.0f, r.getBottom() - 3.0f);
    g.drawHorizontalLine ((int) p.y, r.getX() + 3.0f, r.getRight() - 3.0f);
    fillGlow (g, juce::Rectangle<float> (12, 12).withCentre (p), phosphor, 0.8f + 0.2f * xy.press, 10.0f + 6.0f * xy.press, 6.0f);
    g.setColour (phosphor.brighter (0.4f));
    g.drawRect (juce::Rectangle<float> (12, 12).withCentre (p), 2.0f);

    g.setColour (phosphor);
    g.setFont (screenFont (11.0f, true));
    g.drawText (xy.info.toUpperCase(), r.reduced (10.0f).removeFromTop (14.0f), juce::Justification::right);
    lcdTexture (g, r);
}

void Retro97Skin::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff6f695e));
    g.fillRoundedRectangle (r.translated (0, 1.0f), 3.0f);
    auto face = r.translated (0, down ? 1.0f : 0.0f);
    g.setGradientFill (juce::ColourGradient (key.brighter (over ? 0.08f : 0.03f), face.getX(), face.getY(), key.darker (0.12f),
                                             face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 3.0f);
}

void Retro97Skin::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (ink);
    g.setFont (labelFont (juce::jmin (13.0f, (float) b.getHeight() * 0.6f)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1, 0.6f);
}

void Retro97Skin::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
        return;
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) == nullptr)
    {
        Skin::drawLabel (g, label);
        return;
    }
    // Slider values sit in small amber screens.
    auto r = label.getLocalBounds().toFloat().reduced (2.0f, 1.0f);
    lcd (g, r);
    g.setColour (label.isEnabled() ? amber : amber.withAlpha (0.35f));
    g.setFont (screenFont (juce::jmin (12.5f, r.getHeight() * 0.75f), true));
    g.drawFittedText (label.getText(), r.toNearestInt(), juce::Justification::centred, 1, 0.6f);
    lcdTexture (g, r);
}

void Retro97Skin::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (2.0f);
    lcd (g, r);
    juce::Path arrow;
    const float ax = r.getRight() - 12.0f, ay = r.getCentreY();
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (amber);
    g.fillPath (arrow);
}

} // namespace ts
