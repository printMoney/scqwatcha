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
      waveform (p),
      platter (p),
      xyPad (*p.apvts.getParameter (ParamID::cutoff), *p.apvts.getParameter (ParamID::resonance),
             *p.apvts.getParameter (ParamID::filterBypass))
{
    setLookAndFeel (&lnf);
    hotkeyProblems = hotkeys.loadOrCreateUserFile();

    // Header.
    for (auto* b : { &loadButton, &halfButton, &doubleButton, &detectButton, &helpButton })
        addAndMakeVisible (*b);
    loadButton.setColour (juce::TextButton::buttonColourId, Colours::accent.withAlpha (0.85f));
    loadButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
    loadButton.onClick = [this] { openFileChooser(); };
    halfButton.onClick = [this] { proc.setParam (ParamID::sampleBpm, proc.getParam (ParamID::sampleBpm) * 0.5f); };
    doubleButton.onClick = [this] { proc.setParam (ParamID::sampleBpm, proc.getParam (ParamID::sampleBpm) * 2.0f); };
    detectButton.onClick = [this] { proc.redetectBpm(); };
    helpButton.onClick = [this] { toggleHelp(); };

    addAndMakeVisible (sampleBpm);
    sampleBpm.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 70, 26);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::sampleBpm, sampleBpm));
    sampleBpm.setRange (40.0, 250.0, 0.1);
    sampleBpm.setNumDecimalPlacesToDisplay (2);

    setupKnob (gainKnob, Colours::text);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::gain, gainKnob));

    addAndMakeVisible (waveform);
    addAndMakeVisible (platter);

    // Filter.
    addAndMakeVisible (xyPad);
    setupToggle (bypassToggle, Colours::filter);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::filterBypass, bypassToggle));

    // Repeater.
    for (int i = 0; i < DeckEngine::numRepeatRates; ++i)
    {
        rateButtons[i] = std::make_unique<PadButton> (rateNames[i], Colours::repeater);
        rateButtons[i]->onPress = [this, i] (const juce::ModifierKeys&) { proc.repeat (i, true); };
        rateButtons[i]->onRelease = [this, i] { proc.repeat (i, false); };
        addAndMakeVisible (*rateButtons[i]);
    }
    setupKnob (mixKnob, Colours::repeater);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::repeaterMix, mixKnob));

    // Tempo.
    setupKnob (bpmKnob, Colours::accent);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::systemBpm, bpmKnob));
    bpmKnob.onDragStart = [this] {
        if (proc.getParam (ParamID::overrideBpm) < 0.5f)
            proc.setParam (ParamID::overrideBpm, 1.0f);
    };
    setupToggle (overrideToggle, Colours::accent);
    setupToggle (syncToggle, Colours::accent);
    setupToggle (keepPitchToggle, Colours::accent);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::overrideBpm, overrideToggle));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::sync, syncToggle));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, ParamID::keepPitch, keepPitchToggle));
    addAndMakeVisible (pitchSlider);
    pitchSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 24);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::pitch, pitchSlider));
    setupKnob (tapeKnob, Colours::accent);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, ParamID::tapeStopTime, tapeKnob));
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

    for (int i = 0; i < TurntableProcessor::numPads; ++i)
    {
        pads[i] = std::make_unique<PadButton> (juce::String (i + 1), Colours::pad);
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
    startTimerHz (60);
}

TurntableEditor::~TurntableEditor()
{
    releaseHeldKeys (true);
    proc.onSampleChanged = nullptr;
    removeMouseListener (this);
    setLookAndFeel (nullptr);
}

void TurntableEditor::setupKnob (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::rotarySliderFillColourId, c);
    if (s.getTextBoxPosition() != juce::Slider::NoTextBox)
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    s.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (s);
}

void TurntableEditor::setupToggle (juce::ToggleButton& b, juce::Colour c)
{
    b.setColour (juce::ToggleButton::tickColourId, c);
    addAndMakeVisible (b);
}

// ---------------------------------------------------------------- layout

void TurntableEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 10);

    headerArea = r.removeFromTop (56);
    {
        auto h = headerArea;
        h.removeFromLeft (200); // title
        loadButton.setBounds (h.removeFromLeft (80).reduced (0, 12));
        h.removeFromLeft (24);
        h.removeFromLeft (84); // "SAMPLE BPM" label
        sampleBpm.setBounds (h.removeFromLeft (150).reduced (0, 14));
        h.removeFromLeft (6);
        halfButton.setBounds (h.removeFromLeft (40).reduced (2, 14));
        doubleButton.setBounds (h.removeFromLeft (40).reduced (2, 14));
        detectButton.setBounds (h.removeFromLeft (70).reduced (2, 14));

        helpButton.setBounds (h.removeFromRight (36).reduced (0, 12));
        h.removeFromRight (16);
        meterArea = h.removeFromRight (18).reduced (0, 8);
        h.removeFromRight (6);
        gainArea = h.removeFromRight (100);
        gainKnob.setBounds (gainArea.removeFromRight (56));
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
        bypassToggle.setBounds (top.removeFromRight (84));
        f.removeFromTop (8);
        xyPad.setBounds (f);
    }
    middle.removeFromLeft (12);

    repeaterPanel = middle.removeFromTop (162);
    {
        auto rp = repeaterPanel.reduced (12);
        rp.removeFromTop (24);
        auto knob = rp.removeFromRight (78);
        mixKnob.setBounds (knob.withTrimmedTop (4));
        rp.removeFromRight (8);
        const int w = rp.getWidth() / DeckEngine::numRepeatRates;
        for (int i = 0; i < DeckEngine::numRepeatRates; ++i)
            rateButtons[i]->setBounds (rp.removeFromLeft (w).reduced (3, 6));
    }
    middle.removeFromTop (12);

    tempoPanel = middle;
    {
        auto t = tempoPanel.reduced (12);
        t.removeFromTop (22);
        auto left = t.removeFromLeft (110);
        bpmKnob.setBounds (left.removeFromTop (100));
        tempoInfoArea = left;

        auto right = t.withTrimmedLeft (10);
        auto tape = right.removeFromRight (84);
        tape.removeFromTop (16);
        tapeKnob.setBounds (tape.removeFromTop (100));

        overrideToggle.setBounds (right.removeFromTop (28));
        right.removeFromTop (6);
        auto row = right.removeFromTop (28);
        syncToggle.setBounds (row.removeFromLeft (row.getWidth() / 2 - 3));
        row.removeFromLeft (6);
        keepPitchToggle.setBounds (row);
        right.removeFromTop (8);
        pitchSlider.setBounds (right.removeFromTop (26).withTrimmedLeft (44));
    }

    r.removeFromTop (8);
    auto transport = r.removeFromTop (64);
    {
        PadButton* buttons[] = { &cueButton, &playButton, &tapeButton, &loopButton, &inButton, &outButton, &setCueButton, &undoButton, &redoButton };
        const int widths[] = { 110, 140, 140, 110, 80, 80, 110, 0, 0 };
        for (int i = 0; i < 7; ++i)
        {
            buttons[i]->setBounds (transport.removeFromLeft (widths[i]).reduced (3, 0));
            if (i == 2 || i == 5)
                transport.removeFromLeft (14);
        }
        redoButton.setBounds (transport.removeFromRight (90).reduced (3, 0));
        undoButton.setBounds (transport.removeFromRight (90).reduced (3, 0));
    }

    r.removeFromTop (8);
    statusArea = r.removeFromBottom (22);
    r.removeFromBottom (4);
    {
        auto padRow = r;
        const int w = padRow.getWidth() / TurntableProcessor::numPads;
        for (int i = 0; i < TurntableProcessor::numPads; ++i)
            pads[i]->setBounds (padRow.removeFromLeft (w).reduced (4, 0));
    }

    overlay.setBounds (getLocalBounds());
}

void TurntableEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    // Title.
    {
        auto t = headerArea.withWidth (200);
        g.setColour (Colours::accent);
        g.setFont (TSLookAndFeel::font (19.0f, true));
        g.drawText ("TURNTABLE", t.removeFromTop (30).withTrimmedTop (6), juce::Justification::bottomLeft);
        g.setColour (Colours::dim);
        g.setFont (TSLookAndFeel::font (12.0f, true));
        g.drawText ("SAMPLER", t, juce::Justification::topLeft);
    }

    g.setColour (Colours::dim);
    g.setFont (TSLookAndFeel::font (11.0f, true));
    g.drawText ("SAMPLE BPM", headerArea.withX (headerArea.getX() + 304).withWidth (84), juce::Justification::centredLeft);
    g.drawText ("GAIN", gainArea, juce::Justification::centredRight);

    // Meter.
    {
        auto m = meterArea.toFloat();
        g.setColour (Colours::panelEdge);
        g.fillRoundedRectangle (m, 3.0f);
        auto drawBar = [&] (juce::Rectangle<float> bar, float level) {
            const float db = juce::Decibels::gainToDecibels (level, -60.0f);
            const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
            auto filled = bar.removeFromBottom (bar.getHeight() * frac);
            g.setColour (level > 0.99f ? Colours::danger : (db > -6.0f ? Colours::accent : Colours::filter));
            g.fillRect (filled);
        };
        auto left = m.reduced (2.0f);
        auto right = left.removeFromRight (left.getWidth() * 0.5f);
        drawBar (left.withTrimmedRight (1.0f), meterL);
        drawBar (right.withTrimmedLeft (1.0f), meterR);
    }

    drawPanel (g, filterPanel.toFloat(), "Filter", Colours::filter);
    drawPanel (g, repeaterPanel.toFloat(), "Repeater", Colours::repeater);
    drawPanel (g, tempoPanel.toFloat(), "Tempo", Colours::accent);

    g.setFont (TSLookAndFeel::font (11.0f, true));
    g.setColour (Colours::dim);
    g.drawText ("MIX", mixKnob.getBounds().withHeight (12).translated (0, -10), juce::Justification::centred);
    g.drawText ("TAPE STOP", tapeKnob.getBounds().withHeight (12).translated (0, -14), juce::Justification::centred);
    g.drawText ("PITCH", pitchSlider.getBounds().withX (pitchSlider.getX() - 44).withWidth (44), juce::Justification::centredLeft);

    // Tempo readout.
    {
        auto t = tempoInfoArea.withTrimmedTop (4);
        const bool hostValid = proc.status.hostBpmValid.load();
        const bool overridden = proc.getParam (ParamID::overrideBpm) > 0.5f || ! hostValid;
        g.setFont (TSLookAndFeel::font (12.0f));
        g.setColour (overridden ? Colours::dim : Colours::text);
        g.drawText (hostValid ? "Host " + juce::String (proc.status.hostBpm.load(), 1) : juce::String ("Host: none"),
                    t.removeFromTop (16), juce::Justification::centred);
        g.setColour (Colours::accent);
        g.setFont (TSLookAndFeel::font (13.0f, true));
        g.drawText ("Using " + juce::String (proc.effectiveBpm(), 1), t.removeFromTop (18), juce::Justification::centred);
        g.setFont (TSLookAndFeel::font (11.0f));
        g.setColour (Colours::dim);
        g.drawText ("speed x" + juce::String (proc.status.tempoRatio.load(), 3), t.removeFromTop (16), juce::Justification::centred);
    }

    // Status line.
    g.setFont (TSLookAndFeel::font (12.0f));
    const auto err = proc.getLastError();
    g.setColour (err.isNotEmpty() ? Colours::danger : Colours::dim);
    g.drawText (lastStatus, statusArea, juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.drawText ("Press ? for hotkeys", statusArea, juce::Justification::centredRight);
}

// ---------------------------------------------------------------- timer

void TurntableEditor::timerCallback()
{
    platter.refresh();
    waveform.refresh();
    xyPad.refresh();

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
        pads[i]->setSubLabel (proc.padHasRegion (i) ? "" : "shift+click to store");
    }
    undoButton.setFilled (proc.undoManager.canUndo());
    redoButton.setFilled (proc.undoManager.canRedo());
    pitchSlider.setEnabled (proc.getParam (ParamID::keepPitch) > 0.5f);

    // Meter with simple decay.
    const float pl = proc.status.peakL.exchange (0.0f);
    const float pr = proc.status.peakR.exchange (0.0f);
    meterL = juce::jmax (pl, meterL * 0.88f);
    meterR = juce::jmax (pr, meterR * 0.88f);

    // Status text.
    juce::String status;
    if (proc.isLoading())
        status = "Loading sample...";
    else if (proc.getLastError().isNotEmpty())
        status = proc.getLastError();
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

    if (status != lastStatus)
        lastStatus = status;

    const uint64_t seq = proc.status.sampleSeq.load();
    if (seq != shownSeq)
    {
        shownSeq = seq;
        waveform.sampleChanged();
    }

    repaint (headerArea);
    repaint (tempoInfoArea);
    repaint (statusArea);
}

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

    const auto binding = HotkeyMap::fromKeyPress (key);
    auto action = hotkeys.find (binding);
    if (! action)
        return false; // let the host have it

    const auto* info = HotkeyMap::info (*action);
    const int code = key.getKeyCode();
    const bool isRepeat = keysDown.contains (code);
    if (isRepeat && ! info->autoRepeat)
        return true;

    keysDown.addIfNotAlreadyThere (code);
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
