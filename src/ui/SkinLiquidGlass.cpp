#include "Skins.h"

namespace ts
{

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

Palette glassPalette()
{
    Palette p;
    p.background = juce::Colour (0xff0b1026);
    p.panel = juce::Colours::white.withAlpha (0.07f);
    p.panelEdge = juce::Colours::white.withAlpha (0.2f);
    p.text = juce::Colour (0xfff8f9ff);
    p.dim = juce::Colour (0xffb3b9d1);
    p.transport = juce::Colour (0xffff9f0a);
    p.loop = juce::Colour (0xff0a84ff);
    p.repeater = juce::Colour (0xffbf5af2);
    p.filter = juce::Colour (0xff64d2ff);
    p.danger = juce::Colour (0xffff453a);
    p.padIdle = juce::Colours::white.withAlpha (0.09f);
    p.screen = juce::Colours::black.withAlpha (0.25f);
    p.screenText = p.text;
    return p;
}
} // namespace

LiquidGlassSkin::LiquidGlassSkin() : Skin (glassPalette())
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1d2342));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, palette.loop);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

juce::Font LiquidGlassSkin::font (float size, bool bold) const
{
    return juce::Font (juce::FontOptions ("Helvetica Neue", bold ? "Medium" : "Regular", size));
}

juce::Font LiquidGlassSkin::labelFont (float size) const
{
    return juce::Font (juce::FontOptions ("Helvetica Neue", "Medium", size + 1.0f));
}

juce::Font LiquidGlassSkin::screenFont (float size, bool bold) const
{
    return juce::Font (juce::FontOptions ("Helvetica Neue", bold ? "Light" : "Regular", bold ? size * 1.15f : size));
}

// The frosted look: a translucent fill, a bright edge along the top and a
// soft highlight that fades downwards.
void LiquidGlassSkin::glass (juce::Graphics& g, juce::Rectangle<float> r, float corner, float strength) const
{
    // Soft shadow underneath.
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.06f * strength));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f * (float) i).expanded (0.5f * (float) i), corner + 0.5f * (float) i);
    }

    g.setColour (juce::Colours::white.withAlpha (0.075f * strength));
    g.fillRoundedRectangle (r, corner);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.13f * strength), r.getX(), r.getY(),
                                             juce::Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.45f, false));
    g.fillRoundedRectangle (r, corner);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.42f * strength), r.getX(), r.getY(),
                                             juce::Colours::white.withAlpha (0.06f * strength), r.getX(), r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
}

void LiquidGlassSkin::drawBackground (juce::Graphics& g, juce::Rectangle<float> r)
{
    const int w = juce::roundToInt (r.getWidth()), h = juce::roundToInt (r.getHeight());
    if (! backdrop.isValid() || backdrop.getWidth() != w || backdrop.getHeight() != h)
    {
        // A soft field of colour for the glass to sit on, rendered once.
        backdrop = juce::Image (juce::Image::RGB, juce::jmax (1, w), juce::jmax (1, h), true);
        juce::Graphics bg (backdrop);
        bg.fillAll (palette.background);
        struct Blob { float x, y, rad; juce::uint32 c; float a; };
        const Blob blobs[] = {
            { 0.12f, 0.18f, 0.55f, 0xff5e4bff, 0.55f },
            { 0.88f, 0.12f, 0.50f, 0xff17b5ff, 0.45f },
            { 0.70f, 0.95f, 0.60f, 0xffff4f9a, 0.35f },
            { 0.20f, 0.95f, 0.45f, 0xff21d4a8, 0.25f },
        };
        for (auto& b : blobs)
        {
            const juce::Point<float> c (b.x * (float) w, b.y * (float) h);
            const float rad = b.rad * (float) juce::jmax (w, h);
            juce::ColourGradient grad (juce::Colour (b.c).withAlpha (b.a), c.x, c.y, juce::Colour (b.c).withAlpha (0.0f), c.x + rad, c.y, true);
            bg.setGradientFill (grad);
            bg.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c));
        }
    }
    g.drawImageAt (backdrop, juce::roundToInt (r.getX()), juce::roundToInt (r.getY()));
}

void LiquidGlassSkin::drawTitle (juce::Graphics& g, juce::Rectangle<float> a)
{
    g.setColour (palette.text);
    g.setFont (juce::Font (juce::FontOptions ("Helvetica Neue", "Bold", 21.0f)));
    g.drawText ("Turntable", a.removeFromTop (a.getHeight() * 0.58f), juce::Justification::bottomLeft);
    g.setColour (palette.dim);
    g.setFont (juce::Font (juce::FontOptions ("Helvetica Neue", "Light", 15.0f)));
    g.drawText ("Sampler", a, juce::Justification::topLeft);
}

void LiquidGlassSkin::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour tint)
{
    glass (g, r, 22.0f);
    if (title.isNotEmpty())
    {
        auto t = r.reduced (18.0f, 9.0f).removeFromTop (18.0f);
        g.setColour (tint);
        g.fillEllipse (t.removeFromLeft (7.0f).withSizeKeepingCentre (7.0f, 7.0f));
        t.removeFromLeft (8.0f);
        g.setColour (palette.text);
        g.setFont (juce::Font (juce::FontOptions ("Helvetica Neue", "Bold", 13.5f)));
        g.drawText (title, t, juce::Justification::centredLeft);
    }
}

void LiquidGlassSkin::drawPad (juce::Graphics& g, juce::Rectangle<float> r, const PadLook& p)
{
    const bool small = p.kind == PadLook::Kind::Small;
    const float corner = small ? r.getHeight() * 0.5f - 2.0f : 16.0f;
    r = r.reduced (2.5f);

    const float lit = juce::jmax (p.lit, p.glow * 0.8f);
    fillGlow (g, r, p.tint, juce::jmax (p.lit * 0.9f, p.glow), small ? 2.5f : 6.5f, corner);

    glass (g, r, corner, 0.9f + p.hover * 0.5f);

    if (lit > 0.001f)
    {
        g.setGradientFill (juce::ColourGradient (p.tint.withAlpha (0.9f * lit), r.getCentreX(), r.getY(),
                                                 p.tint.withAlpha (0.55f * lit), r.getCentreX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        // Specular highlight on the lit glass.
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f * lit), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.5f, false));
        g.fillRoundedRectangle (r.reduced (2.0f), corner - 2.0f);
    }
    else if (p.filled)
    {
        g.setColour (p.tint.withAlpha (0.22f));
        g.fillRoundedRectangle (r, corner);
        g.setColour (p.tint.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.2f);
    }

    if (p.press > 0.001f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.1f * p.press));
        g.fillRoundedRectangle (r, corner);
    }

    auto text = r.reduced (4.0f);
    if (p.sub.isNotEmpty() && ! small)
    {
        g.setColour (palette.text.withAlpha (0.65f));
        g.setFont (font (10.5f));
        g.drawText (p.sub, text.removeFromBottom (16.0f), juce::Justification::centred);
    }
    g.setColour (p.enabled ? palette.text : palette.text.withAlpha (0.4f));
    g.setFont (small ? font (juce::jmin (13.5f, text.getHeight() * 0.55f), true)
                     : juce::Font (juce::FontOptions ("Helvetica Neue", "Medium", juce::jmin (21.0f, text.getHeight() * 0.42f, text.getWidth() * 0.5f))));
    g.drawFittedText (p.label, text.toNearestInt(), juce::Justification::centred, 2, 0.6f);
}

void LiquidGlassSkin::drawToggle (juce::Graphics& g, juce::Rectangle<float> r, const ToggleLook& t)
{
    r = r.reduced (1.5f);
    const float corner = r.getHeight() * 0.5f;
    glass (g, r, corner, 0.8f + t.hover * 0.4f);
    if (t.on > 0.001f)
    {
        g.setColour (t.tint.withAlpha (0.85f * t.on));
        g.fillRoundedRectangle (r, corner);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.3f * t.on), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), r.getX(), r.getCentreY(), false));
        g.fillRoundedRectangle (r.reduced (1.5f), corner - 1.5f);
    }
    g.setColour (t.enabled ? palette.text : palette.text.withAlpha (0.4f));
    g.setFont (labelFont (juce::jmin (12.0f, r.getHeight() * 0.46f)));
    g.drawFittedText (t.text.toLowerCase().replaceSection (0, 1, t.text.substring (0, 1).toUpperCase()),
                      r.toNearestInt().reduced (6, 0), juce::Justification::centred, 1, 0.6f);
}

void LiquidGlassSkin::drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float pos, float hover, juce::Colour tint,
                                bool enabled)
{
    const float start = pi * 1.2f, end = pi * 2.8f;
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto col = enabled ? tint : palette.dim;

    // Thin track with a glowing value arc.
    juce::Path bg;
    bg.addCentredArc (c.x, c.y, radius, radius, 0.0f, start, end, true);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.strokePath (bg, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val;
    val.addCentredArc (c.x, c.y, radius, radius, 0.0f, start, angle, true);
    g.setColour (col.withAlpha (0.25f));
    g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Glass pebble.
    const float capR = radius - 8.0f;
    auto cap = juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c);
    glass (g, cap, capR, 1.0f + hover * 0.6f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), cap.getCentreX() - capR * 0.4f, cap.getY(),
                                             juce::Colours::white.withAlpha (0.0f), cap.getCentreX(), cap.getCentreY(), true));
    g.fillEllipse (cap.reduced (capR * 0.15f).withTrimmedBottom (capR * 0.6f));

    g.setColour (palette.text);
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (c.getPointOnCircumference (capR * 0.68f, angle)));
}

void LiquidGlassSkin::drawPlatter (juce::Graphics& g, juce::Rectangle<float> bounds, const PlatterLook& look)
{
    const auto c = bounds.getCentre();
    const float R = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 20.0f;

    glass (g, bounds, 26.0f);

    // Platter ring of light.
    const auto ring = juce::Rectangle<float> ((R + 9) * 2, (R + 9) * 2).withCentre (c);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillEllipse (ring);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), ring.getX(), ring.getY(),
                                             juce::Colours::white.withAlpha (0.05f), ring.getRight(), ring.getBottom(), false));
    g.drawEllipse (ring, 1.2f);
    if (look.scratch > 0.001f)
    {
        fillGlow (g, ring, palette.transport, look.scratch, 10.0f, ring.getWidth() * 0.5f);
        g.setColour (palette.transport.withAlpha (look.scratch));
        g.drawEllipse (ring, 2.5f);
    }

    drawVinyl (g, c, R, look, juce::Colour (0xfff2f3f8), juce::Colour (0xff1b2040), font (12.0f, true));

    // A soft glossy reflection that drifts a little as the record turns.
    {
        juce::Graphics::ScopedSaveState clip (g);
        juce::Path disc;
        disc.addEllipse (juce::Rectangle<float> (R * 2, R * 2).withCentre (c));
        g.reduceClipRegion (disc);
        const float drift = 0.18f * R * std::sin ((float) look.angle);
        const juce::Point<float> hot (c.x - R * 0.35f + drift, c.y - R * 0.55f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.13f), hot.x, hot.y,
                                                 juce::Colours::white.withAlpha (0.0f), hot.x + R * 0.9f, hot.y, true));
        g.fillEllipse (juce::Rectangle<float> (R * 1.8f, R * 1.2f).withCentre (hot));
    }

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillEllipse (juce::Rectangle<float> (11, 11).withCentre (c));

    if (look.message.isNotEmpty())
    {
        g.setColour (palette.text);
        g.setFont (font (15.0f, true));
        g.drawFittedText (look.message, juce::Rectangle<float> (R * 1.4f, 50.0f).withCentre (c.translated (0, R * 0.6f)).toNearestInt(),
                          juce::Justification::centred, 2, 0.6f);
    }
    if (look.dropHover)
    {
        g.setColour (palette.filter);
        g.drawRoundedRectangle (bounds.reduced (2.0f), 26.0f, 2.5f);
    }
}

void LiquidGlassSkin::drawWaveformFrame (juce::Graphics& g, juce::Rectangle<float> r)
{
    glass (g, r, 16.0f);
}

void LiquidGlassSkin::drawXYPad (juce::Graphics& g, juce::Rectangle<float> r, const XYLook& xy, juce::Colour tint)
{
    const auto t = xy.on ? tint : palette.dim;
    g.setGradientFill (juce::ColourGradient (t.withAlpha (0.28f), r.getX(), r.getBottom(), t.withAlpha (0.04f), r.getRight(), r.getY(), false));
    g.fillRoundedRectangle (r, 18.0f);
    glass (g, r, 18.0f, 0.7f);

    g.setFont (labelFont (11.0f));
    g.setColour (palette.dim);
    g.drawText ("Cutoff  >", r.reduced (14.0f).removeFromBottom (14.0f), juce::Justification::right);
    g.drawText ("^  Resonance", r.reduced (14.0f).removeFromTop (16.0f), juce::Justification::left);

    const auto dot = r.reduced (18.0f);
    const juce::Point<float> p (dot.getX() + xy.x * dot.getWidth(), dot.getBottom() - xy.y * dot.getHeight());
    const float halo = 46.0f + 16.0f * xy.press;
    g.setGradientFill (juce::ColourGradient (t.withAlpha (0.45f), p.x, p.y, t.withAlpha (0.0f), p.x + halo * 0.5f, p.y, true));
    g.fillEllipse (juce::Rectangle<float> (halo, halo).withCentre (p));
    auto pebble = juce::Rectangle<float> (22, 22).withCentre (p);
    glass (g, pebble, 11.0f, 1.4f);
    g.setColour (t);
    g.fillEllipse (pebble.reduced (5.0f));

    g.setColour (palette.text);
    g.setFont (font (12.5f));
    g.drawText (xy.info, r.reduced (14.0f).removeFromTop (16.0f), juce::Justification::right);
}

void LiquidGlassSkin::drawMeter (juce::Graphics& g, juce::Rectangle<float> m, float left, float right)
{
    glass (g, m, m.getWidth() * 0.5f, 0.8f);
    auto bar = [&] (juce::Rectangle<float> b, float level) {
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
        auto filled = b.removeFromBottom (b.getHeight() * frac);
        g.setGradientFill (juce::ColourGradient (palette.filter, 0.0f, b.getBottom(), palette.danger, 0.0f, b.getY(), false));
        g.fillRoundedRectangle (filled, filled.getWidth() * 0.5f);
    };
    auto l = m.reduced (3.0f);
    auto r = l.removeFromRight (l.getWidth() * 0.5f);
    bar (l.withTrimmedRight (1.0f), left);
    bar (r.withTrimmedLeft (1.0f), right);
}

void LiquidGlassSkin::drawOverlay (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0b1026).withAlpha (0.93f));
    g.fillRect (r);
}

void LiquidGlassSkin::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    glass (g, r, juce::jmin (10.0f, r.getHeight() * 0.5f), over ? 1.3f : 0.9f);
    if (down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (r, juce::jmin (10.0f, r.getHeight() * 0.5f));
    }
}

void LiquidGlassSkin::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    glass (g, r, r.getHeight() * 0.5f, box.isMouseOver (true) ? 1.3f : 1.0f);
    juce::Path arrow;
    const float ax = r.getRight() - 15.0f, ay = r.getCentreY();
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (palette.text);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace ts
