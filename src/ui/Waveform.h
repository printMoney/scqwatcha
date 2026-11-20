#pragma once

#include "LookAndFeel.h"

namespace ts
{

class TurntableProcessor;

// Overview of the whole sample with playhead, loop region, cue and pads.
// Click to jump.
class Waveform final : public juce::Component
{
public:
    explicit Waveform (TurntableProcessor& p);

    void refresh();
    void sampleChanged();

    void paint (juce::Graphics&) override;
    void resized() override { cacheValid = false; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    float xFor (double frames) const;
    void rebuildCache();

    TurntableProcessor& proc;
    juce::Image cache;
    bool cacheValid = false;
    double lastPlayhead = -1.0, lastIn = -1.0, lastOut = -1.0, lastCue = -1.0;
    bool lastLoopOn = false;
    int lastRepeat = -2;
};

} // namespace ts
