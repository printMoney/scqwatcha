#include "Platter.h"
#include "../PluginProcessor.h"
#include "../SampleLoader.h"

namespace ts
{

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;
}

Platter::Platter (TurntableProcessor& p) : Animated (static_cast<juce::Component&> (*this)), proc (p)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

juce::Point<float> Platter::centre() const { return getLocalBounds().toFloat().getCentre(); }

float Platter::discRadius() const
{
    return (float) juce::jmin (getWidth(), getHeight()) * 0.5f - 18.0f;
}

bool Platter::animate (float dt)
{
    bool moved = false;
    const auto* s = proc.getUiSample();
    const double secs = s != nullptr && s->sampleRate > 0 ? proc.status.playhead.load() / s->sampleRate : 0.0;
    const double a = std::fmod (secs / DeckEngine::revolutionSeconds, 1.0) * twoPi;
    if (std::abs (a - angle) > 0.0005)
    {
        angle = a;
        moved = true;
    }
    const float sp = (float) proc.status.speed.load();
    if (std::abs (sp - speed) > 0.01f)
    {
        speed = sp;
        moved = true;
    }
    scratchGlow.target = scratchActive ? 1.0f : 0.0f;
    moved |= scratchGlow.update (dt, 18.0f);
    return moved;
}

void Platter::paint (juce::Graphics& g)
{
    const auto* s = proc.getUiSample();
    PlatterLook look;
    look.angle = angle;
    look.hasSample = s != nullptr;
    look.name = s != nullptr ? s->name : juce::String();
    look.message = proc.isLoading() ? juce::String ("Loading...") : (s == nullptr ? juce::String ("Drop a sample here\nor press Cmd+O") : juce::String());
    look.scratch = scratchGlow.value;
    look.dropHover = dropHover;
    look.speed = speed;
    Skin::of (*this).drawPlatter (g, getLocalBounds().toFloat(), look);
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
