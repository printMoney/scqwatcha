#include "PluginEditor.h"

namespace ts
{

namespace
{
constexpr int editorWidth = 1100;
constexpr int editorHeight = 720;
const char* const rateNames[] = { "1/1", "1/2", "1/4", "1/8", "1/16" };

juce::String formatTime (double seconds)
{
    const int s = (int) seconds;
    return juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2);
}
} // namespace

TurntableEditor::TurntableEditor (TurntableProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      skin (Skin::create (p.getSkinName().isNotEmpty() ? p.getSkinName() : Skin::defaultName)),
      waveform (p),
      platter (p),
      xyPad (*p.apvts.getParameter (ParamID::cutoff), *p.apvts.getParameter (ParamID::resonance),
             *p.apvts.getParameter (ParamID::filterBypass))
{
    setLookAndFeel (skin.get());
    hotkeyProblems = hotkeys.loadOrCreateUserFile();

    // Header.
    for (auto* b : { &loadButton, &halfButton, &doubleButton, &detectButton, &helpButton })
        addAndMakeVisible (*b);
    loadButton.onPress = [this] (const juce::ModifierKeys&) { openFileChooser(); };
    halfButton.onPress = [this] (const juce::ModifierKeys&) { proc.setParam (ParamID::sampleBpm, proc.getParam (ParamID::sampleBpm) * 0.5f); };
    doubleButton.onPress = [this] (const juce::ModifierKeys&) { proc.setParam (ParamID::sampleBpm, proc.getParam (ParamID::sampleBpm) * 2.0f); };
    detectButton.onPress = [this] (const juce::ModifierKeys&) { proc.redetectBpm(); };
    helpButton.onPress = [this] (const juce::ModifierKeys&) { toggleHelp(); };

    addAndMakeVisible (sampleBpm);
    sampleBpm.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 74, 26);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::sampleBpm, sampleBpm));
    sampleBpm.setRange (40.0, 250.0, 0.1);
    sampleBpm.setNumDecimalPlacesToDisplay (2);

    attachKnob (gainKnob, ParamID::gain);

    addAndMakeVisible (skinMenu);
    skinMenu.addItemList (Skin::names(), 1);
    skinMenu.setSelectedItemIndex (juce::jmax (0, Skin::names().indexOf (skin->name())), juce::dontSendNotification);
    skinMenu.onChange = [this] {
        const auto name = skinMenu.getText();
        if (name.isNotEmpty() && name != skin->name())
        {
            proc.setSkinName (name);
            applySkin (name, true);
        }
    };

    addAndMakeVisible (waveform);
    addAndMakeVisible (platter);

    // Filter.
    addAndMakeVisible (xyPad);
    addAndMakeVisible (bypassToggle);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::filterBypass, bypassToggle));

    // Repeater.
    for (int i = 0; i < DeckEngine::numRepeatRates; ++i)
    {
        rateButtons[i] = std::make_unique<PadButton> (rateNames[i], PadButton::Tint::Repeater);
        rateButtons[i]->onPress = [this, i] (const juce::ModifierKeys&) { proc.repeat (i, true); };
        rateButtons[i]->onRelease = [this, i] { proc.repeat (i, false); };
        addAndMakeVisible (*rateButtons[i]);
    }
    attachKnob (mixKnob, ParamID::repeaterMix);

    // Tempo.
    attachKnob (bpmKnob, ParamID::systemBpm);
    bpmKnob.onDragStart = [this] {
        if (proc.getParam (ParamID::overrideBpm) < 0.5f)
            proc.setParam (ParamID::overrideBpm, 1.0f);
    };
    for (auto* t : { &overrideToggle, &syncToggle, &keepPitchToggle })
        addAndMakeVisible (*t);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::overrideBpm, overrideToggle));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::sync, syncToggle));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::keepPitch, keepPitchToggle));
    addAndMakeVisible (pitchSlider);
    pitchSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 24);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::pitch, pitchSlider));
    attachKnob (tapeKnob, ParamID::tapeStopTime);
    tapeKnob.setTextValueSuffix (" s");

    // Transport.
    for (auto* b : { &cueButton, &playButton, &tapeButton, &loopButton, &inButton, &outButton, &setCueButton, &undoButton, &redoButton })
        addAndMakeVisible (*b);
    cueButton.setSubLabel ("restart");
    tapeButton.setSubLabel ("hold");
    cueButton.onPress = [this] (const juce::ModifierKeys&) { proc.hardRestart(); };
    playButton.onPress = [this] (const juce::ModifierKeys&) { proc.togglePlay(); };
    tapeButton.onPress = [this] (const juce::ModifierKeys&) { proc.tapeStop (true); };
    tapeButton.onRelease = [this] { proc.tapeStop (false); };
    loopButton.onPress = [this] (const juce::ModifierKeys&) { proc.toggleLoop(); };
    inButton.onPress = [this] (const juce::ModifierKeys&) { proc.setLoopInHere(); };
    outButton.onPress = [this] (const juce::ModifierKeys&) { proc.setLoopOutHere(); };
    setCueButton.onPress = [this] (const juce::ModifierKeys&) { proc.setCueHere(); };
    undoButton.onPress = [this] (const juce::ModifierKeys&) { proc.undoManager.undo(); };
    redoButton.onPress = [this] (const juce::ModifierKeys&) { proc.undoManager.redo(); };
    addAndMakeVisible (snapToggle);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::snap, snapToggle));

    for (int i = 0; i < TurntableProcessor::numPads; ++i)
    {
        pads[i] = std::make_unique<PadButton> (juce::String (i + 1), PadButton::Tint::Loop);
        pads[i]->onPress = [this, i] (const juce::ModifierKeys& m) {
            if (m.isShiftDown())
                proc.storePad (i);
            else
                proc.recallPad (i);
        };
        addAndMakeVisible (*pads[i]);
    }

    addChildComponent (overlay);
    overlay.onClose = [this] { overlay.setVisible (false); grabFocusSoon(); };
    overlay.onReload = [this] { reloadHotkeys(); };
    addChildComponent (fadeLayer);

    // Keys must reach the editor, not a slider or button.
    for (auto* c : getChildren())
    {
        c->setWantsKeyboardFocus (false);
        c->setMouseClickGrabsKeyboardFocus (false);
    }
    setWantsKeyboardFocus (true);
    addMouseListener (this, true);

    proc.onSampleChanged = [this] { waveform.sampleChanged(); platter.repaint(); repaint(); };

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

TurntableEditor::~TurntableEditor()
{
    releaseHeldKeys (true);
    proc.onSampleChanged = nullptr;
    removeMouseListener (this);
    setLookAndFeel (nullptr);
}

void TurntableEditor::attachKnob (Knob& k, const char* paramID)
{
    addAndMakeVisible (k);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, paramID, k));
    if (auto* param = proc.apvts.getParameter (paramID))
        k.resetTo (param->convertFrom0to1 (param->getDefaultValue()));
}

void TurntableEditor::applySkin (const juce::String& name, bool animate)
{
    if (animate && isShowing() && getWidth() > 0)
        fadeLayer.start (createComponentSnapshot (getLocalBounds(), true));

    auto next = Skin::create (name);
    setLookAndFeel (next.get());
    skin = std::move (next);
    skinMenu.setSelectedItemIndex (juce::jmax (0, Skin::names().indexOf (skin->name())), juce::dontSendNotification);
    fadeLayer.toFront (false);
    repaint();
}

// ---------------------------------------------------------------- layout

void TurntableEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 10);

    headerArea = r.removeFromTop (56);
    {
        auto h = headerArea;
        titleArea = h.removeFromLeft (200).reduced (0, 6);
        loadButton.setBounds (h.removeFromLeft (80).reduced (0, 12));
        h.removeFromLeft (24);
        sampleBpmLabelArea = h.removeFromLeft (84);
        sampleBpm.setBounds (h.removeFromLeft (150).reduced (0, 14));
        h.removeFromLeft (6);
        halfButton.setBounds (h.removeFromLeft (40).reduced (2, 13));
        doubleButton.setBounds (h.removeFromLeft (40).reduced (2, 13));
        detectButton.setBounds (h.removeFromLeft (74).reduced (2, 13));

        helpButton.setBounds (h.removeFromRight (36).reduced (0, 12));
        h.removeFromRight (8);
        auto skinArea = h.removeFromRight (150);
        skinLabelArea = skinArea.removeFromTop (14);
        skinMenu.setBounds (skinArea.reduced (0, 4).withTrimmedBottom (4));
        h.removeFromRight (16);
        meterArea = h.removeFromRight (18).reduced (0, 8);
        h.removeFromRight (6);
        gainKnob.setBounds (h.removeFromRight (50).reduced (0, 2));
        gainLabelArea = h.removeFromRight (36);
    }

    r.removeFromTop (8);
    waveform.setBounds (r.removeFromTop (64));
    r.removeFromTop (8);

    auto middle = r.removeFromTop (360);
    platter.setBounds (middle.removeFromLeft (360));
    middle.removeFromLeft (12);
    filterPanel = middle.removeFromLeft (330);
    {
        auto f = filterPanel.reduced (12);
        auto top = f.removeFromTop (22);
        bypassToggle.setBounds (top.removeFromRight (90));
        f.removeFromTop (8);
        xyPad.setBounds (f);
    }
    middle.removeFromLeft (12);

    repeaterPanel = middle.removeFromTop (162);
    {
        auto rp = repeaterPanel.reduced (12);
        rp.removeFromTop (24);
        auto knob = rp.removeFromRight (80);
        mixLabelArea = knob.removeFromTop (14);
        mixKnob.setBounds (knob);
        rp.removeFromRight (8);
        const int w = rp.getWidth() / DeckEngine::numRepeatRates;
        for (int i = 0; i < DeckEngine::numRepeatRates; ++i)
            rateButtons[i]->setBounds (rp.removeFromLeft (w).reduced (2, 6));
    }
    middle.removeFromTop (12);

    tempoPanel = middle;
    {
        auto t = tempoPanel.reduced (12);
        t.removeFromTop (22);
        auto left = t.removeFromLeft (112);
        bpmKnob.setBounds (left.removeFromTop (84).reduced (4, 0));
        tempoInfoArea = left.withTrimmedTop (8);

        auto right = t.withTrimmedLeft (10);
        auto tape = right.removeFromRight (86);
        tapeLabelArea = tape.removeFromTop (16);
        tapeKnob.setBounds (tape.removeFromTop (104));

        overrideToggle.setBounds (right.removeFromTop (30));
        right.removeFromTop (6);
        auto row = right.removeFromTop (30);
        syncToggle.setBounds (row.removeFromLeft (row.getWidth() / 2 - 3));
        row.removeFromLeft (6);
        keepPitchToggle.setBounds (row);
        right.removeFromTop (10);
        auto pitchRow = right.removeFromTop (26);
        pitchLabelArea = pitchRow.removeFromLeft (44);
        pitchSlider.setBounds (pitchRow);
    }

    r.removeFromTop (8);
    auto transport = r.removeFromTop (64);
    {
        PadButton* buttons[] = { &cueButton, &playButton, &tapeButton, &loopButton, &inButton, &outButton, &setCueButton };
        const int widths[] = { 110, 140, 140, 110, 80, 80, 110 };
        for (int i = 0; i < 7; ++i)
        {
            buttons[i]->setBounds (transport.removeFromLeft (widths[i]).reduced (3, 0));
            if (i == 2 || i == 5)
                transport.removeFromLeft (14);
        }
        transport.removeFromLeft (6);
        snapToggle.setBounds (transport.removeFromLeft (76).reduced (3, 16));
        redoButton.setBounds (transport.removeFromRight (90).reduced (3, 0));
        undoButton.setBounds (transport.removeFromRight (90).reduced (3, 0));
    }

    r.removeFromTop (8);
    statusArea = r.removeFromBottom (24);
    r.removeFromBottom (6);
    {
        auto padRow = r;
        const int w = padRow.getWidth() / TurntableProcessor::numPads;
        for (int i = 0; i < TurntableProcessor::numPads; ++i)
            pads[i]->setBounds (padRow.removeFromLeft (w).reduced (4, 0));
    }

    overlay.setBounds (getLocalBounds());
    fadeLayer.setBounds (getLocalBounds());
}

void TurntableEditor::paint (juce::Graphics& g)
{
    auto& s = *skin;
    s.drawBackground (g, getLocalBounds().toFloat());
    s.drawTitle (g, titleArea.toFloat());

    s.drawSmallLabel (g, sampleBpmLabelArea.toFloat(), "Sample BPM", juce::Justification::centredLeft);
    s.drawSmallLabel (g, gainLabelArea.toFloat(), "Gain", juce::Justification::centredRight);
    s.drawSmallLabel (g, skinLabelArea.toFloat(), "Skin", juce::Justification::centredLeft);
    s.drawMeter (g, meterArea.toFloat(), meterL, meterR);

    s.drawPanel (g, filterPanel.toFloat(), "Filter", s.pal().filter);
    s.drawPanel (g, repeaterPanel.toFloat(), "Repeater", s.pal().repeater);
    s.drawPanel (g, tempoPanel.toFloat(), "Tempo", s.pal().transport);

    s.drawSmallLabel (g, mixLabelArea.toFloat(), "Mix", juce::Justification::centred);
    s.drawSmallLabel (g, tapeLabelArea.toFloat(), "Tape stop", juce::Justification::centred);
    s.drawSmallLabel (g, pitchLabelArea.toFloat(), "Pitch", juce::Justification::centredLeft);

    s.drawScreen (g, tempoInfoArea.toFloat(), tempoLines, juce::Justification::centred);
    s.drawStatusBar (g, statusArea.toFloat(), statusLeft, "Press ? or H for hotkeys", statusWarning);
}

// ---------------------------------------------------------------- timer

void TurntableEditor::timerCallback()
{
    waveform.refresh();

    // Catch key releases even if the OS did not tell us (e.g. focus moved).
    releaseHeldKeys (false);

    playButton.setLabel (proc.status.playing.load() ? "PAUSE" : "PLAY");
    playButton.setLit (proc.status.playing.load());
    loopButton.setLit (proc.isLoopOn());
    const int rep = proc.status.repeatIndex.load();
    for (int i = 0; i < DeckEngine::numRepeatRates; ++i)
        rateButtons[i]->setLit (rep == i);
    for (int i = 0; i < TurntableProcessor::numPads; ++i)
    {
        pads[i]->setFilled (proc.padHasRegion (i));
        pads[i]->setLit (proc.padIsCurrentLoop (i));
        pads[i]->setSubLabel (proc.padHasRegion (i) ? "shift+tap: replace" : "tap: save loop here");
    }
    undoButton.setFilled (proc.undoManager.canUndo());
    redoButton.setFilled (proc.undoManager.canRedo());
    pitchSlider.setEnabled (proc.getParam (ParamID::keepPitch) > 0.5f);

    // Meter with a gentle fall.
    const float pl = proc.status.peakL.exchange (0.0f);
    const float pr = proc.status.peakR.exchange (0.0f);
    meterL = juce::jmax (pl, meterL * 0.86f);
    meterR = juce::jmax (pr, meterR * 0.86f);

    // Tempo readout.
    {
        const bool hostValid = proc.status.hostBpmValid.load();
        const bool overridden = proc.getParam (ParamID::overrideBpm) > 0.5f || ! hostValid;
        tempoLines.clearQuick();
        tempoLines.add (overridden ? (hostValid ? "Host " + juce::String (proc.status.hostBpm.load(), 1) + " (off)" : juce::String ("No host tempo"))
                                   : juce::String ("Following host"));
        tempoLines.add (juce::String (proc.effectiveBpm(), 1));
        tempoLines.add ("BPM   x" + juce::String (proc.status.tempoRatio.load(), 3));
    }

    // Status text.
    juce::String status;
    statusWarning = false;
    if (proc.isLoading())
        status = "Loading sample...";
    else if (proc.getLastError().isNotEmpty())
    {
        status = proc.getLastError();
        statusWarning = true;
    }
    else if (const auto* s = proc.getUiSample())
    {
        const double ph = proc.status.playhead.load() / s->sampleRate;
        status << s->name << "   " << formatTime (ph) << " / " << formatTime (s->lengthSeconds())
               << "   " << juce::String (proc.getParam (ParamID::sampleBpm), 2) << " BPM sample";
        if (proc.isLoopOn() && proc.barFrames() > 0)
            status << "   loop " << juce::String ((proc.getLoopOut() - proc.getLoopIn()) / proc.barFrames(), 2) << " bars";
        if (proc.status.scratching.load())
            status << "   SCRATCH";
    }
    else
        status = "No sample loaded. Drop a WAV, AIFF or MP3 on the platter, or click LOAD.";
    statusLeft = status;

    const uint64_t seq = proc.status.sampleSeq.load();
    if (seq != shownSeq)
    {
        shownSeq = seq;
        waveform.sampleChanged();
    }

    repaint (meterArea.expanded (2));
    repaint (tempoInfoArea);
    repaint (statusArea);
}

// Visual feedback for a key press on the matching on-screen control.
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wswitch-enum")
void TurntableEditor::flashFor (Action a)
{
    const int idx = (int) a;
    switch (a)
    {
        case Action::PlayPause:   playButton.flash(); break;
        case Action::HardRestart: cueButton.flash(); break;
        case Action::TapeStop:    tapeButton.flash(); break;
        case Action::LoopToggle:  loopButton.flash(); break;
        case Action::LoopIn:      inButton.flash(); break;
        case Action::LoopOut:     outButton.flash(); break;
        case Action::SetCue:      setCueButton.flash(); break;
        case Action::Undo:        undoButton.flash(); break;
        case Action::Redo:        redoButton.flash(); break;
        case Action::LoadSample:  loadButton.flash(); break;
        case Action::ShowHelp:    helpButton.flash(); break;
        case Action::Repeat1: case Action::Repeat2: case Action::Repeat4: case Action::Repeat8: case Action::Repeat16:
            rateButtons[idx - (int) Action::Repeat1]->flash();
            break;
        case Action::Pad1: case Action::Pad2: case Action::Pad3: case Action::Pad4:
        case Action::Pad5: case Action::Pad6: case Action::Pad7: case Action::Pad8:
            pads[idx - (int) Action::Pad1]->flash();
            break;
        case Action::PadStore1: case Action::PadStore2: case Action::PadStore3: case Action::PadStore4:
        case Action::PadStore5: case Action::PadStore6: case Action::PadStore7: case Action::PadStore8:
            pads[idx - (int) Action::PadStore1]->flash();
            break;
        default: break;
    }
}
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

// ---------------------------------------------------------------- keyboard

void TurntableEditor::grabFocusSoon()
{
    juce::Component::SafePointer<TurntableEditor> safe (this);
    juce::MessageManager::callAsync ([safe] {
        if (safe != nullptr && safe->isShowing() && ! safe->hasKeyboardFocus (true))
            safe->grabKeyboardFocus();
    });
}

void TurntableEditor::visibilityChanged() { grabFocusSoon(); }
void TurntableEditor::parentHierarchyChanged() { grabFocusSoon(); }

void TurntableEditor::mouseDown (const juce::MouseEvent& e)
{
    // Any click in the window (except into a text box) gives the keyboard back to the deck.
    if (dynamic_cast<juce::TextEditor*> (e.eventComponent) == nullptr && dynamic_cast<juce::Label*> (e.eventComponent) == nullptr)
        if (! hasKeyboardFocus (false))
            grabKeyboardFocus();
}

void TurntableEditor::focusLost (FocusChangeType)
{
    releaseHeldKeys (true);
}

bool TurntableEditor::keyPressed (const juce::KeyPress& key)
{
    if (overlay.isVisible() && key == juce::KeyPress::escapeKey)
    {
        overlay.setVisible (false);
        return true;
    }

    auto action = hotkeys.find (key);
    if (! action)
        return false; // let the host have it

    const auto* info = HotkeyMap::info (*action);
    const int code = key.getKeyCode();
    const bool isRepeat = keysDown.contains (code);
    if (isRepeat && ! info->autoRepeat)
        return true;

    keysDown.addIfNotAlreadyThere (code);
    if (! isRepeat)
        flashFor (*action);
    if (info->momentary)
    {
        if (heldActions.count (code) == 0)
        {
            heldActions[code] = *action;
            perform (*action, true);
        }
    }
    else
        perform (*action, true);

    return true;
}

bool TurntableEditor::keyStateChanged (bool isKeyDown)
{
    if (! isKeyDown)
        releaseHeldKeys (false);
    return false;
}

void TurntableEditor::releaseHeldKeys (bool all)
{
    for (int i = keysDown.size(); --i >= 0;)
    {
        const int code = keysDown.getUnchecked (i);
        if (all || ! juce::KeyPress::isKeyCurrentlyDown (code))
        {
            keysDown.remove (i);
            auto it = heldActions.find (code);
            if (it != heldActions.end())
            {
                const auto a = it->second;
                heldActions.erase (it);
                perform (a, false);
            }
        }
    }
}

void TurntableEditor::bumpBpm (float delta)
{
    if (proc.getParam (ParamID::overrideBpm) < 0.5f)
    {
        // First touch: start from the host tempo, then take over.
        const float base = proc.status.hostBpmValid.load() ? (float) std::round (proc.status.hostBpm.load())
                                                           : proc.getParam (ParamID::systemBpm);
        proc.setParam (ParamID::systemBpm, base + delta);
        proc.setParam (ParamID::overrideBpm, 1.0f);
    }
    else
        proc.nudgeParam (ParamID::systemBpm, delta);
}

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wswitch-enum")
void TurntableEditor::perform (Action a, bool down)
{
    const int idx = (int) a;

    switch (a)
    {
        case Action::ScratchHold: platter.setScratchKey (down); return;
        case Action::TapeStop:    proc.tapeStop (down); return;
        case Action::Repeat1: case Action::Repeat2: case Action::Repeat4: case Action::Repeat8: case Action::Repeat16:
            proc.repeat (idx - (int) Action::Repeat1, down);
            return;
        default: break;
    }

    if (! down)
        return;

    switch (a)
    {
        case Action::PlayPause:   proc.togglePlay(); break;
        case Action::HardRestart: proc.hardRestart(); break;
        case Action::LoopToggle:  proc.toggleLoop(); break;
        case Action::LoopIn:      proc.setLoopInHere(); break;
        case Action::LoopOut:     proc.setLoopOutHere(); break;
        case Action::SetCue:      proc.setCueHere(); break;
        case Action::SnapToggle:  proc.toggleParam (ParamID::snap); break;

        case Action::Pad1: case Action::Pad2: case Action::Pad3: case Action::Pad4:
        case Action::Pad5: case Action::Pad6: case Action::Pad7: case Action::Pad8:
            proc.recallPad (idx - (int) Action::Pad1);
            break;

        case Action::PadStore1: case Action::PadStore2: case Action::PadStore3: case Action::PadStore4:
        case Action::PadStore5: case Action::PadStore6: case Action::PadStore7: case Action::PadStore8:
            proc.storePad (idx - (int) Action::PadStore1);
            break;

        case Action::BpmUp:     bumpBpm (1.0f); break;
        case Action::BpmDown:   bumpBpm (-1.0f); break;
        case Action::BpmUp10:   bumpBpm (10.0f); break;
        case Action::BpmDown10: bumpBpm (-10.0f); break;
        case Action::OverrideToggle:  proc.toggleParam (ParamID::overrideBpm); break;
        case Action::SyncToggle:      proc.toggleParam (ParamID::sync); break;
        case Action::KeepPitchToggle: proc.toggleParam (ParamID::keepPitch); break;

        case Action::PitchDown:
        case Action::PitchUp:
            if (proc.getParam (ParamID::keepPitch) > 0.5f)
                proc.nudgeParam (ParamID::pitch, a == Action::PitchUp ? 1.0f : -1.0f);
            break;

        case Action::FilterBypass:  proc.toggleParam (ParamID::filterBypass); break;
        case Action::CutoffDown:
        case Action::CutoffUp:
        {
            const float factor = std::pow (2.0f, (a == Action::CutoffUp ? 1.0f : -1.0f) / 6.0f);
            proc.setParam (ParamID::cutoff, proc.getParam (ParamID::cutoff) * factor);
            if (proc.getParam (ParamID::filterBypass) > 0.5f)
                proc.setParam (ParamID::filterBypass, 0.0f);
            break;
        }
        case Action::ResonanceDown: proc.nudgeParam (ParamID::resonance, -0.05f); break;
        case Action::ResonanceUp:   proc.nudgeParam (ParamID::resonance, 0.05f); break;
        case Action::MixDown:       proc.nudgeParam (ParamID::repeaterMix, -0.05f); break;
        case Action::MixUp:         proc.nudgeParam (ParamID::repeaterMix, 0.05f); break;
        case Action::TapeTimeDown:  proc.nudgeParam (ParamID::tapeStopTime, -0.1f); break;
        case Action::TapeTimeUp:    proc.nudgeParam (ParamID::tapeStopTime, 0.1f); break;
        case Action::GainDown:      proc.nudgeParam (ParamID::gain, -1.0f); break;
        case Action::GainUp:        proc.nudgeParam (ParamID::gain, 1.0f); break;
        case Action::Undo:          proc.undoManager.undo(); break;
        case Action::Redo:          proc.undoManager.redo(); break;
        case Action::LoadSample:    openFileChooser(); break;
        case Action::ShowHelp:      toggleHelp(); break;
        default: break;
    }
}
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

// ---------------------------------------------------------------- misc

void TurntableEditor::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample", juce::File(),
                                                   "*.wav;*.wave;*.aif;*.aiff;*.aifc;*.mp3;*.flac;*.m4a;*.caf;*.ogg");
    juce::Component::SafePointer<TurntableEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc) {
                              if (safe == nullptr)
                                  return;
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  safe->proc.loadFile (f);
                              safe->grabFocusSoon();
                          });
}

void TurntableEditor::toggleHelp()
{
    if (overlay.isVisible())
        overlay.setVisible (false);
    else
        overlay.show (hotkeys, hotkeyProblems);
}

void TurntableEditor::reloadHotkeys()
{
    releaseHeldKeys (true);
    hotkeyProblems = hotkeys.loadOrCreateUserFile();
    overlay.show (hotkeys, hotkeyProblems);
}

} // namespace ts
