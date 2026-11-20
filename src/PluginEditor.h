#pragma once

#include "HotkeyMap.h"
#include "PluginProcessor.h"
#include "ui/HotkeyOverlay.h"
#include "ui/LookAndFeel.h"
#include "ui/PadButton.h"
#include "ui/Platter.h"
#include "ui/Waveform.h"
#include "ui/XYPad.h"
#include <map>

namespace ts
{

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

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void perform (Action a, bool down);
    void releaseHeldKeys (bool all);
    void bumpBpm (float delta);
    void openFileChooser();
    void toggleHelp();
    void reloadHotkeys();
    void grabFocusSoon();
    void setupKnob (juce::Slider& s, juce::Colour c);
    void setupToggle (juce::ToggleButton& b, juce::Colour c);

    TurntableProcessor& proc;
    TSLookAndFeel lnf;
    HotkeyMap hotkeys;
    juce::StringArray hotkeyProblems;

    // Header.
    juce::TextButton loadButton { "LOAD" }, halfButton { "/2" }, doubleButton { "x2" }, detectButton { "DETECT" },
        helpButton { "?" };
    juce::Slider sampleBpm { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Slider gainKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    Waveform waveform;
    Platter platter;

    // Filter.
    XYPad xyPad;
    juce::ToggleButton bypassToggle { "BYPASS" };

    // Repeater.
    std::unique_ptr<PadButton> rateButtons[DeckEngine::numRepeatRates];
    juce::Slider mixKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };

    // Tempo.
    juce::Slider bpmKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::ToggleButton overrideToggle { "OVERRIDE HOST" }, syncToggle { "SYNC" }, keepPitchToggle { "KEEP PITCH" };
    juce::Slider pitchSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Slider tapeKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };

    // Transport and pads.
    PadButton cueButton { "CUE", Colours::accent }, playButton { "PLAY", Colours::accent },
        tapeButton { "TAPE STOP", Colours::accent }, loopButton { "LOOP", Colours::pad }, inButton { "IN", Colours::pad },
        outButton { "OUT", Colours::pad }, setCueButton { "SET CUE", Colours::accent }, undoButton { "UNDO", Colours::dim },
        redoButton { "REDO", Colours::dim };
    std::unique_ptr<PadButton> pads[TurntableProcessor::numPads];
    juce::ToggleButton snapToggle { "SNAP" };

    HotkeyOverlay overlay;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::unique_ptr<juce::FileChooser> chooser;

    // Layout rectangles used for painted labels.
    juce::Rectangle<int> headerArea, filterPanel, repeaterPanel, tempoPanel, meterArea, statusArea, gainArea, tempoInfoArea;

    // Keyboard state: physical keys currently down, and which momentary action each one started.
    juce::Array<int> keysDown;
    std::map<int, Action> heldActions;

    float meterL = 0.0f, meterR = 0.0f;
    uint64_t shownSeq = 0;
    juce::String lastStatus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurntableEditor)
};

} // namespace ts
