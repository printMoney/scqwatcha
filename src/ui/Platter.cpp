#include "Platter.h"
#include "../PluginProcessor.h"
#include "../SampleLoader.h"

namespace ts
{

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;
}

Platter::Platter (TurntableProcessor& p) : proc (p)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

juce::Point<float> Platter::centre() const { return getLocalBounds().toFloat().getCentre(); }

float Platter::discRadius() const
{
    return juce::jmin (getWidth(), getHeight()) * 0.5f - 14.0f;
}

void Platter::refresh()
{
    const auto* s = proc.getUiSample();
    const double secs = s != nullptr && s->sampleRate > 0 ? proc.status.playhead.load() / s->sampleRate : 0.0;
    const double a = std::fmod (secs / DeckEngine::revolutionSeconds, 1.0) * twoPi;
    if (std::abs (a - angle) > 0.002)
    {
        angle = a;
        repaint();
    }
}

void Platter::paint (juce::Graphics& g)
{
    const auto c = centre();
    const float R = discRadius();
    const auto* s = proc.getUiSample();

    // Plinth.
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);

    // Platter rim.
    g.setColour (juce::Colour (0xff3a3d45));
    g.fillEllipse (juce::Rectangle<float> ((R + 8) * 2, (R + 8) * 2).withCentre (c));
    g.setColour (scratchActive ? Colours::accent : juce::Colour (0xff4a4e58));
    g.drawEllipse (juce::Rectangle<float> ((R + 7) * 2, (R + 7) * 2).withCentre (c), scratchActive ? 3.0f : 1.0f);

    // Vinyl with grooves.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1b1e), c.x, c.y, juce::Colour (0xff09090a), c.x + R, c.y + R, true));
    g.fillEllipse (juce::Rectangle<float> (R * 2, R * 2).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (float r = R * 0.42f; r < R * 0.97f; r += 3.0f)
        g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 0.6f);

    // Static light reflection (does not rotate).
    juce::Path sheen;
    sheen.addPieSegment (juce::Rectangle<float> (R * 2, R * 2).withCentre (c), -0.6f, 0.3f, 0.42f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillPath (sheen);
    sheen.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi, c.x, c.y));
    g.fillPath (sheen);

    // Rotating part: label and marker.
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation ((float) angle, c.x, c.y));

        const float L = R * 0.36f;
        g.setColour (s != nullptr ? Colours::accent : Colours::panelEdge);
        g.fillEllipse (juce::Rectangle<float> (L * 2, L * 2).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (juce::Rectangle<float> (L * 1.6f, L * 1.6f).withCentre (c), 1.0f);

        g.setColour (juce::Colours::black);
        g.setFont (TSLookAndFeel::font (juce::jmax (10.0f, L * 0.18f), true));
        const auto name = s != nullptr ? s->name : juce::String ("NO SAMPLE");
        g.drawFittedText (name, juce::Rectangle<float> (L * 1.5f, L * 0.5f).withCentre (c.translated (0, -L * 0.42f)).toNearestInt(),
                          juce::Justification::centred, 2);

        // Marker stripe so rotation is visible.
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRoundedRectangle (juce::Rectangle<float> (4.0f, R * 0.5f).withPosition (c.x - 2.0f, c.y - R * 0.97f), 2.0f);
    }

    // Spindle.
    g.setColour (juce::Colour (0xffc9ccd3));
    g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre (c));

    if (s == nullptr || proc.isLoading())
    {
        g.setColour (Colours::text.withAlpha (0.85f));
        g.setFont (TSLookAndFeel::font (15.0f, true));
        const auto msg = proc.isLoading() ? juce::String ("Loading...") : juce::String ("Drop a sample here\nor press O");
        g.drawFittedText (msg, juce::Rectangle<float> (R * 1.4f, 50.0f).withCentre (c.translated (0, R * 0.6f)).toNearestInt(),
                          juce::Justification::centred, 2);
    }

    if (dropHover)
    {
        g.setColour (Colours::accent);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 14.0f, 3.0f);
    }
}

void Platter::track (juce::Point<float> pos)
{
    const auto d = pos - centre();
    if (d.getDistanceFromOrigin() < 8.0f)
        return;

    const float a = std::atan2 (d.y, d.x);
    float delta = a - lastAngle;
    while (delta > juce::MathConstants<float>::pi) delta -= twoPi;
    while (delta < -juce::MathConstants<float>::pi) delta += twoPi;
    lastAngle = a;

    if (! scratchActive)
        return;
    if (const auto* s = proc.getUiSample())
        proc.moveScratch ((double) delta / twoPi * DeckEngine::revolutionSeconds * s->sampleRate);
}

void Platter::updateScratchState()
{
    const bool want = (mouseHeld || keyHeld) && proc.getUiSample() != nullptr;
    if (want == scratchActive)
        return;
    scratchActive = want;
    if (want)
        proc.beginScratch();
    else
        proc.endScratch();
    repaint();
}

void Platter::setScratchKey (bool held)
{
    keyHeld = held;
    const auto d = getMouseXYRelative().toFloat() - centre();
    lastAngle = std::atan2 (d.y, d.x);
    updateScratchState();
}

void Platter::releaseAll()
{
    mouseHeld = keyHeld = false;
    updateScratchState();
}

void Platter::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.getDistanceFrom (centre()) > discRadius() + 8.0f)
        return;
    const auto d = e.position - centre();
    lastAngle = std::atan2 (d.y, d.x);
    mouseHeld = true;
    updateScratchState();
}

void Platter::mouseDrag (const juce::MouseEvent& e)
{
    if (mouseHeld || keyHeld)
        track (e.position);
}

void Platter::mouseMove (const juce::MouseEvent& e)
{
    if (keyHeld)
        track (e.position);
}

void Platter::mouseUp (const juce::MouseEvent&)
{
    mouseHeld = false;
    updateScratchState();
}

bool Platter::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (isSupportedAudioFile (juce::File (f)))
            return true;
    return false;
}

void Platter::filesDropped (const juce::StringArray& files, int, int)
{
    dropHover = false;
    for (auto& f : files)
        if (isSupportedAudioFile (juce::File (f)))
        {
            proc.loadFile (juce::File (f));
            break;
        }
    repaint();
}

} // namespace ts
