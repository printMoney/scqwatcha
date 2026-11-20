#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SampleLoader.h"
#include "engine/BpmDetect.h"

namespace ts
{

namespace
{
constexpr double minLoopFrames = 16.0;

// Small per-user settings file (remembers the last skin). Opened only when needed.
juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "settings";
    o.folderName = "scqwatcha";
    o.filenameSuffix = ".xml";
    o.osxLibrarySubFolder = "Application Support";
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = 0;
    return o;
}

juce::String readDefaultSkin()
{
    juce::PropertiesFile file (settingsOptions());
    return file.getValue ("skin", "Studio Ink");
}

void writeDefaultSkin (const juce::String& name)
{
    juce::PropertiesFile file (settingsOptions());
    file.setValue ("skin", name);
    file.saveIfNeeded();
}
}

juce::AudioProcessorValueTreeState::ParameterLayout TurntableProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    NormalisableRange<float> cutoffRange (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (632.0f); // geometric centre: feels logarithmic
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::cutoff, 1 }, "Cutoff", cutoffRange, 20000.0f,
        AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float v, int) {
                return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String ((int) v) + " Hz";
            })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::resonance, 1 }, "Resonance", NormalisableRange<float> (0.0f, 1.0f), 0.2f));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::filterBypass, 1 }, "Filter Bypass", true));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::repeaterMix, 1 }, "Repeater Mix", NormalisableRange<float> (0.0f, 1.0f), 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; })));

    NormalisableRange<float> tapeRange (0.0f, 4.0f, 0.01f); // 0 = instant (10 ms brake)
    tapeRange.setSkewForCentre (1.0f);
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::tapeStopTime, 1 }, "Tape Stop Time", tapeRange, 1.0f,
        AudioParameterFloatAttributes().withLabel ("s")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::systemBpm, 1 }, "System BPM", NormalisableRange<float> (20.0f, 300.0f, 0.1f), 120.0f,
        AudioParameterFloatAttributes().withLabel ("BPM")));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::overrideBpm, 1 }, "Override Host BPM", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::sampleBpm, 1 }, "Sample BPM", NormalisableRange<float> (40.0f, 250.0f, 0.01f), 120.0f,
        AudioParameterFloatAttributes().withLabel ("BPM")));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::sync, 1 }, "BPM Sync", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::keepPitch, 1 }, "Keep Pitch", false));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::pitch, 1 }, "Pitch", -12, 12, 0,
                                                     AudioParameterIntAttributes().withLabel ("st")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::gain, 1 }, "Gain", NormalisableRange<float> (-36.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::snap, 1 }, "Snap Loops To Beat", true));

    return layout;
}

TurntableProcessor::TurntableProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pCutoff = apvts.getRawParameterValue (ParamID::cutoff);
    pResonance = apvts.getRawParameterValue (ParamID::resonance);
    pFilterBypass = apvts.getRawParameterValue (ParamID::filterBypass);
    pMix = apvts.getRawParameterValue (ParamID::repeaterMix);
    pTapeTime = apvts.getRawParameterValue (ParamID::tapeStopTime);
    pSystemBpm = apvts.getRawParameterValue (ParamID::systemBpm);
    pOverride = apvts.getRawParameterValue (ParamID::overrideBpm);
    pSampleBpm = apvts.getRawParameterValue (ParamID::sampleBpm);
    pSync = apvts.getRawParameterValue (ParamID::sync);
    pKeepPitch = apvts.getRawParameterValue (ParamID::keepPitch);
    pPitch = apvts.getRawParameterValue (ParamID::pitch);
    pGain = apvts.getRawParameterValue (ParamID::gain);

    deck.setProperty (DeckID::samplePath, "", nullptr);
    deck.setProperty (DeckID::cue, 0.0, nullptr);
    deck.setProperty (DeckID::loopIn, 0.0, nullptr);
    deck.setProperty (DeckID::loopOut, 0.0, nullptr);
    deck.setProperty (DeckID::loopOn, false, nullptr);
    for (int i = 0; i < numPads; ++i)
    {
        deck.setProperty (DeckID::padIn (i), -1.0, nullptr);
        deck.setProperty (DeckID::padOut (i), -1.0, nullptr);
    }
    deck.setProperty (DeckID::skin, readDefaultSkin(), nullptr);
    deck.addListener (this);
    updateSnapshot();

    startTimerHz (30);
}

TurntableProcessor::~TurntableProcessor()
{
    alive->store (false);
    stopTimer();
    deck.removeListener (this);
    loaderPool.removeAllJobs (true, 10000);
}

void TurntableProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate;
    engine.prepare (sampleRate);
}

bool TurntableProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void TurntableProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* s = slot.acquire(); s != engineSample)
    {
        engine.setSample (s);
        engineSample = s;
        status.sampleSeq.store (s != nullptr ? s->seq : 0);
    }

    double hostBpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;
    const bool hostValid = hostBpm > 1.0 && hostBpm < 1000.0;

    EngineParams p;
    p.sampleBpm = pSampleBpm->load();
    const bool useOverride = pOverride->load() > 0.5f || ! hostValid;
    p.effectiveBpm = useOverride ? (double) pSystemBpm->load() : hostBpm;
    p.sync = pSync->load() > 0.5f;
    p.keepPitch = pKeepPitch->load() > 0.5f;
    p.pitchSemitones = pPitch->load();
    p.tapeStopSeconds = pTapeTime->load();
    p.repeaterMix = pMix->load();
    p.cutoffHz = pCutoff->load();
    p.resonance = pResonance->load();
    p.filterBypass = pFilterBypass->load() > 0.5f;
    p.gainDb = pGain->load();

    const uint64_t seq = engineSample != nullptr ? engineSample->seq : 0;
    Command c;
    while (commands.peek (c))
    {
        if (c.forSeq > seq)
            break; // meant for a sample the audio thread has not picked up yet
        commands.pop (c);
        engine.handle (c, p);
    }

    buffer.clear();
    const int numOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    if (numOut > 0 && n > 0)
        engine.process (buffer.getWritePointer (0), numOut > 1 ? buffer.getWritePointer (1) : nullptr, n, p,
                        scratchOffset.load (std::memory_order_relaxed));

    status.playhead.store (engine.getPlayhead());
    status.speed.store (engine.getSpeed());
    status.playing.store (engine.isPlaying());
    status.scratching.store (engine.isScratching());
    status.repeatIndex.store (engine.getRepeatIndex());
    status.hostBpm.store (hostBpm);
    status.hostBpmValid.store (hostValid);
    status.effectiveBpm.store (p.effectiveBpm);
    status.tempoRatio.store (engine.getTempoRatio());
    status.peakL.store (juce::jmax (status.peakL.load(), engine.getPeakL()));
    status.peakR.store (juce::jmax (status.peakR.load(), engine.getPeakR()));

    midi.clear();
}

juce::AudioProcessorEditor* TurntableProcessor::createEditor()
{
    return new TurntableEditor (*this);
}

// ---------------------------------------------------------------- state

void TurntableProcessor::updateSnapshot()
{
    const juce::ScopedLock sl (snapshotLock);
    deckSnapshot = deck.createCopy();
}

void TurntableProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree root ("TurntableSampler");
    root.setProperty ("version", 1, nullptr);
    root.appendChild (apvts.copyState(), nullptr);

    juce::ValueTree d;
    {
        const juce::ScopedLock sl (snapshotLock);
        d = deckSnapshot.createCopy();
    }
    d.setProperty (DeckID::playhead, status.playhead.load(), nullptr);
    root.appendChild (d, nullptr);

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void TurntableProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    const auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("TurntableSampler"))
        return;

    const auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid())
        apvts.replaceState (params.createCopy());

    const auto saved = root.getChildWithName (DeckID::deck).createCopy();
    if (! saved.isValid())
        return;

    {
        const juce::ScopedLock sl (snapshotLock);
        deckSnapshot = saved.createCopy();
    }

    if (juce::MessageManager::existsAndIsCurrentThread())
        applyDeckState (saved);
    else
        juce::MessageManager::callAsync ([this, flag = alive, saved] {
            if (flag->load())
                applyDeckState (saved);
        });
}

void TurntableProcessor::applyDeckState (const juce::ValueTree& saved)
{
    const auto path = saved[DeckID::samplePath].toString();
    const double playhead = saved[DeckID::playhead];

    deck.copyPropertiesFrom (saved, nullptr);
    deck.removeProperty (DeckID::playhead, nullptr);
    undoManager.clearUndoHistory();
    sendDeckToEngine();

    const auto* current = getUiSample();
    if (path.isNotEmpty() && juce::File::isAbsolutePath (path))
    {
        if (current == nullptr || current->path != path)
            startLoad (juce::File (path), true, playhead);
        else
            jumpTo (playhead);
    }
}

// ---------------------------------------------------------------- loading

void TurntableProcessor::loadFile (const juce::File& file)
{
    startLoad (file, false, 0.0);
}

void TurntableProcessor::startLoad (const juce::File& file, bool restoring, double restorePlayhead)
{
    const int generation = ++loadGeneration;
    ++loadsInFlight;
    lastError.clear();

    loaderPool.addJob ([this, file, restoring, restorePlayhead, generation, flag = alive] {
        juce::String error;
        auto sample = decodeSample (file, error);

        if (flag->load())
        {
            std::lock_guard<std::mutex> lock (loadMutex);
            if (generation == loadGeneration.load())
            {
                auto r = std::make_unique<LoadResult>();
                r->sample = std::move (sample);
                r->error = error.isNotEmpty() ? error : juce::String();
                r->restoring = restoring;
                r->restorePlayhead = restorePlayhead;
                r->generation = generation;
                pendingResult = std::move (r);
            }
        }
        --loadsInFlight;
    });
}

void TurntableProcessor::redetectBpm()
{
    if (const auto* s = getUiSample())
        setParam (ParamID::sampleBpm, (float) detectBpm (s->audio, s->sampleRate));
}

void TurntableProcessor::timerCallback()
{
    slot.collectGarbage();

    std::unique_ptr<LoadResult> r;
    {
        std::lock_guard<std::mutex> lock (loadMutex);
        r = std::move (pendingResult);
    }
    if (r == nullptr)
        return;

    if (r->sample == nullptr)
    {
        lastError = r->error;
        if (onSampleChanged)
            onSampleChanged();
        return;
    }

    const auto path = r->sample->path;
    const double bpm = r->sample->detectedBpm;
    slot.submit (std::move (r->sample));
    const uint64_t seq = slot.latest()->seq;

    deck.setProperty (DeckID::samplePath, path, nullptr);

    if (! r->restoring)
    {
        setParam (ParamID::sampleBpm, (float) bpm);
        deck.setProperty (DeckID::cue, 0.0, nullptr);
        deck.setProperty (DeckID::loopIn, 0.0, nullptr);
        deck.setProperty (DeckID::loopOut, 0.0, nullptr);
        deck.setProperty (DeckID::loopOn, false, nullptr);
        for (int i = 0; i < numPads; ++i)
        {
            deck.setProperty (DeckID::padIn (i), -1.0, nullptr);
            deck.setProperty (DeckID::padOut (i), -1.0, nullptr);
        }
        undoManager.clearUndoHistory();
    }

    sendDeckToEngine (seq);
    if (r->restoring && r->restorePlayhead > 0.0)
    {
        Command c { Cmd::Jump };
        c.a = r->restorePlayhead;
        c.forSeq = seq;
        send (c);
    }

    if (onSampleChanged)
        onSampleChanged();
}

// ---------------------------------------------------------------- deck / loops

void TurntableProcessor::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& id)
{
    if (id == DeckID::loopIn || id == DeckID::loopOut || id == DeckID::loopOn || id == DeckID::cue)
        sendDeckToEngine();
    updateSnapshot();
}

void TurntableProcessor::sendDeckToEngine (uint64_t forSeq)
{
    Command loop { Cmd::SetLoop };
    loop.a = getLoopIn();
    loop.b = getLoopOut();
    loop.flag = isLoopOn();
    loop.forSeq = forSeq;
    send (loop);

    Command cue { Cmd::SetCue };
    cue.a = getCue();
    cue.forSeq = forSeq;
    send (cue);
}

bool TurntableProcessor::isLoopOn() const { return (bool) deck[DeckID::loopOn]; }
double TurntableProcessor::getLoopIn() const { return (double) deck[DeckID::loopIn]; }
double TurntableProcessor::getLoopOut() const { return (double) deck[DeckID::loopOut]; }
double TurntableProcessor::getCue() const { return (double) deck[DeckID::cue]; }

double TurntableProcessor::barFrames() const
{
    const auto* s = getUiSample();
    if (s == nullptr)
        return 0.0;
    const bool sync = pSync->load() > 0.5f;
    const double bpm = sync ? (double) pSampleBpm->load() : juce::jmax (1.0, effectiveBpm());
    return 4.0 * 60.0 / juce::jmax (1.0, bpm) * s->sampleRate;
}

double TurntableProcessor::snapToBeat (double frames, bool roundDown) const
{
    const double beat = barFrames() / 4.0;
    if (getParam (ParamID::snap) < 0.5f || beat <= minLoopFrames)
        return frames;
    const double beats = frames / beat;
    // Within 1/16 beat counts as "on" the beat, so a slightly late press still rounds down to it.
    return (roundDown ? std::floor (beats + 1.0 / 16.0) : std::round (beats)) * beat;
}

void TurntableProcessor::setLoopRegion (double in, double out, bool on)
{
    deck.setProperty (DeckID::loopIn, in, &undoManager);
    deck.setProperty (DeckID::loopOut, out, &undoManager);
    deck.setProperty (DeckID::loopOn, on, &undoManager);
}

void TurntableProcessor::toggleLoop()
{
    const auto* s = getUiSample();
    if (s == nullptr)
        return;

    undoManager.beginNewTransaction ("Loop on/off");
    if (isLoopOn())
    {
        deck.setProperty (DeckID::loopOn, false, &undoManager);
        return;
    }

    const double length = s->numFrames();
    const double ph = status.playhead.load();
    double in = getLoopIn(), out = getLoopOut();
    if (out - in <= minLoopFrames)
    {
        in = juce::jlimit (0.0, length, snapToBeat (ph, true));
        out = juce::jmin (length, in + barFrames());
    }
    if (out - in <= minLoopFrames)
        return;

    setLoopRegion (in, out, true);
    if (ph < in || ph >= out)
        jumpTo (in);
}

void TurntableProcessor::setLoopInHere()
{
    const auto* s = getUiSample();
    if (s == nullptr)
        return;

    const double length = s->numFrames();
    const double in = juce::jlimit (0.0, length, snapToBeat (status.playhead.load(), false));
    double out = getLoopOut();
    if (out <= in + minLoopFrames)
        out = juce::jmin (length, in + barFrames());
    if (out - in <= minLoopFrames)
        return;

    undoManager.beginNewTransaction ("Loop in");
    setLoopRegion (in, out, isLoopOn());
}

void TurntableProcessor::setLoopOutHere()
{
    const auto* s = getUiSample();
    if (s == nullptr)
        return;

    const double length = s->numFrames();
    const double out = juce::jlimit (0.0, length, snapToBeat (status.playhead.load(), false));
    double in = getLoopIn();
    if (out <= in + minLoopFrames)
        in = juce::jmax (0.0, out - barFrames());
    if (out - in <= minLoopFrames)
        return;

    undoManager.beginNewTransaction ("Loop out");
    setLoopRegion (in, out, isLoopOn());

    // Snapping can put the out point just behind the playhead: wrap back in, keeping the beat.
    const double ph = status.playhead.load();
    if (isLoopOn() && ph >= out)
        jumpTo (in + std::fmod (ph - out, out - in));
}

void TurntableProcessor::setCueHere()
{
    if (getUiSample() == nullptr)
        return;
    undoManager.beginNewTransaction ("Set cue");
    deck.setProperty (DeckID::cue, status.playhead.load(), &undoManager);
}

bool TurntableProcessor::padHasRegion (int i) const
{
    return (double) deck[DeckID::padOut (i)] > (double) deck[DeckID::padIn (i)] && (double) deck[DeckID::padIn (i)] >= 0.0;
}

bool TurntableProcessor::padIsCurrentLoop (int i) const
{
    return isLoopOn() && padHasRegion (i)
        && std::abs ((double) deck[DeckID::padIn (i)] - getLoopIn()) < 1.0
        && std::abs ((double) deck[DeckID::padOut (i)] - getLoopOut()) < 1.0;
}

void TurntableProcessor::recallPad (int i)
{
    const auto* s = getUiSample();
    if (i < 0 || i >= numPads || s == nullptr)
        return;

    if (! padHasRegion (i))
    {
        // Empty pad: capture a new 1-bar loop starting at the current beat and loop it.
        const double length = s->numFrames();
        const double in = juce::jlimit (0.0, length, snapToBeat (status.playhead.load(), true));
        const double out = juce::jmin (length, in + barFrames());
        if (out - in <= minLoopFrames)
            return;
        undoManager.beginNewTransaction ("Save new loop on pad");
        deck.setProperty (DeckID::padIn (i), in, &undoManager);
        deck.setProperty (DeckID::padOut (i), out, &undoManager);
        setLoopRegion (in, out, true);
        return;
    }

    const double in = deck[DeckID::padIn (i)];
    const double out = deck[DeckID::padOut (i)];
    undoManager.beginNewTransaction ("Recall pad");
    setLoopRegion (in, out, true);
    jumpTo (in);
}

void TurntableProcessor::storePad (int i)
{
    const auto* s = getUiSample();
    if (i < 0 || i >= numPads || s == nullptr)
        return;

    double in = getLoopIn(), out = getLoopOut();
    if (out - in <= minLoopFrames)
    {
        in = juce::jlimit (0.0, (double) s->numFrames(), snapToBeat (status.playhead.load(), true));
        out = juce::jmin ((double) s->numFrames(), in + barFrames());
    }
    if (out - in <= minLoopFrames)
        return;

    undoManager.beginNewTransaction ("Store pad");
    deck.setProperty (DeckID::padIn (i), in, &undoManager);
    deck.setProperty (DeckID::padOut (i), out, &undoManager);
}

juce::String TurntableProcessor::getSkinName() const
{
    return deck[DeckID::skin].toString();
}

void TurntableProcessor::setSkinName (const juce::String& name)
{
    deck.setProperty (DeckID::skin, name, nullptr);
    writeDefaultSkin (name);
}

// ---------------------------------------------------------------- transport

void TurntableProcessor::repeat (int index, bool down)
{
    Command c { down ? Cmd::RepeatDown : Cmd::RepeatUp };
    c.index = index;
    send (c);
}

void TurntableProcessor::beginScratch()
{
    scratchOffset.store (0.0);
    send ({ Cmd::ScratchBegin });
}

void TurntableProcessor::moveScratch (double sourceFrames)
{
    scratchOffset.store (scratchOffset.load() + sourceFrames);
}

void TurntableProcessor::endScratch()
{
    send ({ Cmd::ScratchEnd });
}

void TurntableProcessor::jumpTo (double sourceFrames)
{
    Command c { Cmd::Jump };
    c.a = sourceFrames;
    send (c);
}

// ---------------------------------------------------------------- parameters

void TurntableProcessor::setParam (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        const auto range = apvts.getParameterRange (id);
        p->beginChangeGesture();
        p->setValueNotifyingHost (range.convertTo0to1 (range.snapToLegalValue (juce::jlimit (range.start, range.end, value))));
        p->endChangeGesture();
    }
}

float TurntableProcessor::getParam (const char* id) const
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void TurntableProcessor::nudgeParam (const char* id, float delta)
{
    setParam (id, getParam (id) + delta);
}

void TurntableProcessor::toggleParam (const char* id)
{
    setParam (id, getParam (id) > 0.5f ? 0.0f : 1.0f);
}

} // namespace ts

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ts::TurntableProcessor();
}
