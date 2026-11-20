#pragma once

#include "Animation.h"
#include "Skin.h"

namespace ts
{

class TurntableProcessor;

// The spinning record. Drop a file on it to load; drag on it (or hold the
// scratch key and move the mouse over it) to scratch.
class Platter final : public juce::Component, public juce::FileDragAndDropTarget, private Animated
{
public:
    explicit Platter (TurntableProcessor& p);

    void setScratchKey (bool held); // scratch hotkey pressed / released
    void releaseAll();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    bool animate (float dt) override;
    juce::Point<float> centre() const;
    float discRadius() const;
    void track (juce::Point<float> pos);
    void updateScratchState();

    TurntableProcessor& proc;
    double angle = 0.0;
    bool mouseHeld = false, keyHeld = false, scratchActive = false;
    float lastAngle = 0.0f;
    bool dropHover = false;
    Smooth scratchGlow;
    float speed = 0.0f;
};

} // namespace ts
