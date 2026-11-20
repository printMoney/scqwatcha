#pragma once

#include "LookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace ts
{

// X = cutoff, Y = resonance. Touching the pad switches the filter on.
class XYPad final : public juce::Component
{
public:
    XYPad (juce::RangedAudioParameter& cutoff, juce::RangedAudioParameter& resonance, juce::RangedAudioParameter& bypass);

    void refresh();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> padArea() const;
    void setFrom (juce::Point<float> p);

    juce::RangedAudioParameter& cutoff;
    juce::RangedAudioParameter& resonance;
    juce::RangedAudioParameter& bypass;
    float lastX = -1.0f, lastY = -1.0f;
    bool lastBypass = false;
    bool dragging = false;
};

} // namespace ts
