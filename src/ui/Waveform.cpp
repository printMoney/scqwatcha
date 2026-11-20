#include "Waveform.h"
#include "../PluginProcessor.h"

namespace ts
{

Waveform::Waveform (TurntableProcessor& p) : proc (p)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

void Waveform::sampleChanged()
{
    cacheValid = false;
    repaint();
}

float Waveform::xFor (double frames) const
{
    const auto* s = proc.getUiSample();
    if (s == nullptr || s->numFrames() == 0)
        return 0.0f;
    return (float) (frames / s->numFrames() * getWidth());
}

void Waveform::rebuildCache()
{
    cacheValid = true;
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const float scale = 2.0f;
    cache = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (cache);
    g.addTransform (juce::AffineTransform::scale (scale));

    const auto* s = proc.getUiSample();
    if (s == nullptr || s->peakMax.empty())
        return;

    const float mid = getHeight() * 0.5f;
    const float amp = getHeight() * 0.46f;
    const int bins = (int) s->peakMax.size();
    g.setColour (Skin::of (*this).waveColour());
    for (int x = 0; x < getWidth(); ++x)
    {
        const int b0 = x * bins / getWidth();
        const int b1 = juce::jmax (b0 + 1, (x + 1) * bins / getWidth());
        float lo = 0.0f, hi = 0.0f;
        for (int b = b0; b < b1 && b < bins; ++b)
        {
            lo = juce::jmin (lo, s->peakMin[(size_t) b]);
            hi = juce::jmax (hi, s->peakMax[(size_t) b]);
        }
        g.fillRect ((float) x, mid - hi * amp, 1.0f, juce::jmax (1.0f, (hi - lo) * amp));
    }
}

void Waveform::refresh()
{
    const double ph = proc.status.playhead.load();
    const double in = proc.getLoopIn(), out = proc.getLoopOut(), cue = proc.getCue();
    const bool on = proc.isLoopOn();
    const int rep = proc.status.repeatIndex.load();
    if (std::abs (xFor (ph) - xFor (lastPlayhead)) >= 0.5f || in != lastIn || out != lastOut || cue != lastCue
        || on != lastLoopOn || rep != lastRepeat)
    {
        lastPlayhead = ph;
        lastIn = in;
        lastOut = out;
        lastCue = cue;
        lastLoopOn = on;
        lastRepeat = rep;
        repaint();
    }
}

void Waveform::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto& skin = Skin::of (*this);
    const auto& pal = skin.pal();
    skin.drawWaveformFrame (g, r);

    const auto* s = proc.getUiSample();
    if (s == nullptr)
    {
        g.setColour (pal.dim);
        g.setFont (skin.font (13.0f));
        g.drawText ("Waveform appears here after you load a sample", r, juce::Justification::centred);
        return;
    }

    if (! cacheValid)
        rebuildCache();

    // Loop region (dim when the loop is off).
    if (proc.getLoopOut() > proc.getLoopIn())
    {
        const float x0 = xFor (proc.getLoopIn()), x1 = xFor (proc.getLoopOut());
        g.setColour (pal.loop.withAlpha (proc.isLoopOn() ? 0.28f : 0.10f));
        g.fillRect (x0, 0.0f, juce::jmax (1.0f, x1 - x0), r.getHeight());
        g.setColour (pal.loop.withAlpha (proc.isLoopOn() ? 1.0f : 0.4f));
        g.fillRect (x0, 0.0f, 2.0f, r.getHeight());
        g.fillRect (x1 - 2.0f, 0.0f, 2.0f, r.getHeight());
    }

    if (cache.isValid())
        g.drawImage (cache, r.reduced (0.0f, 4.0f), juce::RectanglePlacement::stretchToFit);

    // Cue marker.
    const float cx = xFor (proc.getCue());
    g.setColour (pal.screenText.withAlpha (0.7f));
    juce::Path tri;
    tri.addTriangle (cx - 5.0f, 0.0f, cx + 5.0f, 0.0f, cx, 7.0f);
    g.fillPath (tri);

    // Playhead.
    const float px = xFor (proc.status.playhead.load());
    g.setColour (proc.status.repeatIndex.load() >= 0 ? pal.repeater : pal.screenText);
    g.fillRect (px - 1.0f, 0.0f, 2.0f, r.getHeight());
}

void Waveform::mouseDown (const juce::MouseEvent& e)
{
    if (const auto* s = proc.getUiSample(); s != nullptr && getWidth() > 0)
        proc.jumpTo (juce::jlimit (0.0, 1.0, (double) e.position.x / getWidth()) * s->numFrames());
}

void Waveform::mouseDrag (const juce::MouseEvent& e)
{
    mouseDown (e);
}

} // namespace ts
