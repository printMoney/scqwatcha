#pragma once

#include "engine/Commands.h"
#include "engine/DeckEngine.h"
#include "engine/SampleSlot.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <mutex>

namespace ts
{

namespace ParamID
{
inline constexpr const char* cutoff = "cutoff";
inline constexpr const char* resonance = "resonance";
inline constexpr const char* filterBypass = "filterBypass";
inline constexpr const char* repeaterMix = "repeaterMix";
inline constexpr const char* tapeStopTime = "tapeStopTime";
inline constexpr const char* systemBpm = "systemBpm";
inline constexpr const char* overrideBpm = "overrideBpm";
inline constexpr const char* sampleBpm = "sampleBpm";
inline constexpr const char* sync = "sync";
inline constexpr const char* keepPitch = "keepPitch";
inline constexpr const char* pitch = "pitch";
inline constexpr const char* gain = "gain";
inline constexpr const char* snap = "snap";
} // namespace ParamID

namespace DeckID
{
inline const juce::Identifier deck { "DECK" };
inline const juce::Identifier samplePath { "samplePath" };
inline const juce::Identifier cue { "cue" };
inline const juce::Identifier loopIn { "loopIn" };
inline const juce::Identifier loopOut { "loopOut" };
inline const juce::Identifier loopOn { "loopOn" };
inline const juce::Identifier playhead { "playhead" };
inline const juce::Identifier skin { "skin" };
inline juce::Identifier padIn (int i) { return juce::Identifier ("pad" + juce::String (i) + "In"); }
inline juce::Identifier padOut (int i) { return juce::Identifier ("pad" + juce::String (i) + "Out"); }
} // namespace DeckID

class TurntableProcessor final : public juce::AudioProcessor,
                                 private juce::Timer,
                                 private juce::ValueTree::Listener
{
public:
    static constexpr int numPads = 8;

    TurntableProcessor();
    ~TurntableProcessor() override;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Turntable Sampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Message-thread API used by the editor ----
    juce::AudioProcessorValueTreeState apvts;
    EngineStatus status;
    juce::UndoManager undoManager;

    void loadFile (const juce::File& file);
    void redetectBpm();
    const SampleData* getUiSample() const noexcept { return slot.latest(); }
    juce::String getLastError() const { return lastError; }
    bool isLoading() const noexcept { return loadsInFlight.load() > 0; }
    std::function<void()> onSampleChanged;

    void togglePlay()    { send ({ Cmd::TogglePlay }); }
    void hardRestart()   { send ({ Cmd::HardRestart }); }
    void tapeStop (bool down) { send ({ down ? Cmd::TapeStopDown : Cmd::TapeStopUp }); }
    void repeat (int index, bool down);
    void beginScratch();
    void moveScratch (double sourceFrames);
    void endScratch();
    void jumpTo (double sourceFrames);

    void toggleLoop();
    void setLoopInHere();
    void setLoopOutHere();
    void setCueHere();
    void recallPad (int index); // empty pad: saves a new 1-bar loop at the playhead and loops it
    void storePad (int index);
    bool padHasRegion (int index) const;
    bool padIsCurrentLoop (int index) const;
    bool isLoopOn() const;
    double getLoopIn() const;
    double getLoopOut() const;
    double getCue() const;

    // Look of the window: saved with the set; the last choice is the default for new instances.
    juce::String getSkinName() const;
    void setSkinName (const juce::String& name);

    double barFrames() const;   // one 4/4 bar of the current sample, in source frames
    // Snaps a position to the beat grid (beats counted from the sample start) when Snap is on.
    double snapToBeat (double frames, bool roundDown) const;
    double effectiveBpm() const { return status.effectiveBpm.load(); }

    void nudgeParam (const char* id, float delta); // delta in the parameter's own units
    void toggleParam (const char* id);
    void setParam (const char* id, float value);
    float getParam (const char* id) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void send (Command c) { commands.push (c); }
    void sendDeckToEngine (uint64_t forSeq = 0);
    void timerCallback() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void applyDeckState (const juce::ValueTree& saved);
    void startLoad (const juce::File& file, bool restoring, double restorePlayhead);
    void setLoopRegion (double in, double out, bool on);
    void updateSnapshot();

    DeckEngine engine;
    SampleSlot slot;
    CommandQueue commands;
    std::atomic<double> scratchOffset { 0.0 };
    const SampleData* engineSample = nullptr; // audio thread
    double hostSampleRate = 44100.0;

    juce::ValueTree deck { DeckID::deck };

    // Parameter value pointers (atomic floats owned by apvts).
    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pFilterBypass = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pTapeTime = nullptr;
    std::atomic<float>* pSystemBpm = nullptr;
    std::atomic<float>* pOverride = nullptr;
    std::atomic<float>* pSampleBpm = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pKeepPitch = nullptr;
    std::atomic<float>* pPitch = nullptr;
    std::atomic<float>* pGain = nullptr;

    // Background loading.
    struct LoadResult
    {
        std::unique_ptr<SampleData> sample;
        juce::String error;
        bool restoring = false;
        double restorePlayhead = 0.0;
        int generation = 0;
    };
    juce::ThreadPool loaderPool { juce::ThreadPoolOptions{}.withThreadName ("TS loader").withNumberOfThreads (1) };
    std::mutex loadMutex;
    std::unique_ptr<LoadResult> pendingResult;
    std::atomic<int> loadGeneration { 0 };
    std::atomic<int> loadsInFlight { 0 };
    juce::String lastError;

    // State snapshot readable from any thread (hosts may save off the message thread).
    juce::CriticalSection snapshotLock;
    juce::ValueTree deckSnapshot { DeckID::deck };
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurntableProcessor)
};

} // namespace ts
