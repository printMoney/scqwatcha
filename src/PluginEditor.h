#pragma once

#include "HotkeyMap.h"
#include "PluginProcessor.h"
#include "ui/Animation.h"
#include "ui/HotkeyOverlay.h"
#include "ui/PadButton.h"
#include "ui/Platter.h"
#include "ui/Skin.h"
#include "ui/Waveform.h"
#include "ui/XYPad.h"
#include <map>

namespace ts
{

// Snapshot of the old skin that fades away after a skin change.
class FadeLayer final : public juce::Component, private Animated
{
public:
    FadeLayer() : Animated (static_cast<juce::Component&> (*this))
    {
        setInterceptsMouseClicks (false, false);
        setVisible (false);
    }

    void start (juce::Image img)
    {
        image = std::move (img);
        alpha.value = 1.0f;
        alpha.target = 0.0f;
        setVisible (true);
        toFront (false);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setOpacity (alpha.value);
        g.drawImage (image, getLocalBounds().toFloat());
    }

private:
    bool animate (float dt) override
    {
        if (! isVisible())
            return false;
        const bool moved = alpha.update (dt, 16.0f);
        if (alpha.value <= 0.001f)
        {
            setVisible (false);
            image = {};
        }
        return moved;
    }

    juce::Image image;
    Smooth alpha;
};

class TurntableEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TurntableEditor (TurntableProcessor&);
    ~TurntableEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    void mouseDown (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    void applySkin (const juce::String& name, bool animate);

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void perform (Action a, bool down);
    void flashFor (Action a);
    void releaseHeldKeys (bool all);
    void bumpBpm (float delta);
    void openFileChooser();
    void toggleHelp();
    void reloadHotkeys();
    void grabFocusSoon();
    void attachKnob (Knob& k, const char* paramID);

    TurntableProcessor& proc;
    std::unique_ptr<Skin> skin;
    HotkeyMap hotkeys;
    juce::StringArray hotkeyProblems;

    // Header.
    PadButton loadButton { "LOAD", PadButton::Tint::Transport, PadLook::Kind::Small },
        halfButton { "/2", PadButton::Tint::Neutral, PadLook::Kind::Small },
        doubleButton { "x2", PadButton::Tint::Neutral, PadLook::Kind::Small },
        detectButton { "DETECT", PadButton::Tint::Neutral, PadLook::Kind::Small },
        helpButton { "?", PadButton::Tint::Neutral, PadLook::Kind::Small };
    juce::Slider sampleBpm { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    Knob gainKnob { Knob::Tint::Neutral, false };
    juce::ComboBox skinMenu;

    Waveform waveform;
    Platter platter;

    // Filter.
    XYPad xyPad;
    SkinToggle bypassToggle { "BYPASS", SkinToggle::Tint::Filter };

    // Repeater.
    std::unique_ptr<PadButton> rateButtons[DeckEngine::numRepeatRates];
    Knob mixKnob { Knob::Tint::Repeater };

    // Tempo.
    Knob bpmKnob { Knob::Tint::Transport, false };
    SkinToggle overrideToggle { "OVERRIDE HOST", SkinToggle::Tint::Transport }, syncToggle { "SYNC", SkinToggle::Tint::Transport },
        keepPitchToggle { "KEEP PITCH", SkinToggle::Tint::Transport };
    juce::Slider pitchSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    Knob tapeKnob { Knob::Tint::Transport };

    // Transport and pads.
    PadButton cueButton { "CUE", PadButton::Tint::Transport, PadLook::Kind::Transport },
        playButton { "PLAY", PadButton::Tint::Transport, PadLook::Kind::Transport },
        tapeButton { "TAPE STOP", PadButton::Tint::Transport, PadLook::Kind::Transport },
        loopButton { "LOOP", PadButton::Tint::Loop, PadLook::Kind::Transport },
        inButton { "IN", PadButton::Tint::Loop, PadLook::Kind::Transport },
        outButton { "OUT", PadButton::Tint::Loop, PadLook::Kind::Transport },
        setCueButton { "SET CUE", PadButton::Tint::Transport, PadLook::Kind::Transport },
        undoButton { "UNDO", PadButton::Tint::Neutral, PadLook::Kind::Transport },
        redoButton { "REDO", PadButton::Tint::Neutral, PadLook::Kind::Transport };
    SkinToggle snapToggle { "SNAP", SkinToggle::Tint::Loop };
    std::unique_ptr<PadButton> pads[TurntableProcessor::numPads];

    HotkeyOverlay overlay;
    FadeLayer fadeLayer;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::unique_ptr<juce::FileChooser> chooser;

    // Layout rectangles used for painted parts.
    juce::Rectangle<int> headerArea, titleArea, sampleBpmLabelArea, filterPanel, repeaterPanel, tempoPanel, meterArea, statusArea,
        gainLabelArea, tempoInfoArea, mixLabelArea, tapeLabelArea, pitchLabelArea, skinLabelArea;

    // Keyboard state: physical keys currently down, and which momentary action each one started.
    juce::Array<int> keysDown;
    std::map<int, Action> heldActions;

    float meterL = 0.0f, meterR = 0.0f;
    uint64_t shownSeq = 0;
    juce::String statusLeft;
    bool statusWarning = false;
    juce::StringArray tempoLines;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurntableEditor)
};

} // namespace ts
