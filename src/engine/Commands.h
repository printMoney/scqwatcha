#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cstdint>

namespace ts
{

enum class Cmd : uint8_t
{
    TogglePlay,
    Play,
    Pause,
    HardRestart,  // jump to cue and play at full speed immediately
    TapeStopDown,
    TapeStopUp,
    RepeatDown,   // index = rate 0..4 (1/1 .. 1/16 bar)
    RepeatUp,
    ScratchBegin,
    ScratchEnd,
    SetLoop,      // a = in, b = out (source frames), flag = enabled
    Jump,         // a = position (source frames)
    SetCue        // a = position (source frames)
};

struct Command
{
    Cmd type = Cmd::Play;
    int index = 0;
    double a = 0.0;
    double b = 0.0;
    bool flag = false;
    uint64_t forSeq = 0; // if set, wait until the engine plays this sample
};

// Many writers (UI, host state restore), one reader (audio thread).
// Writers serialise on a spin lock; the audio thread never locks.
class CommandQueue
{
public:
    bool push (const Command& c) noexcept
    {
        const juce::SpinLock::ScopedLockType lock (writeLock);
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 + n2 < 1)
            return false;
        items[(size_t) (n1 > 0 ? s1 : s2)] = c;
        fifo.finishedWrite (1);
        return true;
    }

    bool peek (Command& c) const noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);
        if (n1 + n2 < 1)
            return false;
        c = items[(size_t) (n1 > 0 ? s1 : s2)];
        return true;
    }

    bool pop (Command& c) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);
        if (n1 + n2 < 1)
            return false;
        c = items[(size_t) (n1 > 0 ? s1 : s2)];
        fifo.finishedRead (1);
        return true;
    }

private:
    static constexpr int capacity = 512;
    juce::AbstractFifo fifo { capacity };
    std::array<Command, (size_t) capacity> items {};
    juce::SpinLock writeLock;
};

// Values the audio thread publishes for the UI. All lock-free atomics.
struct EngineStatus
{
    std::atomic<double> playhead { 0.0 };    // source frames
    std::atomic<double> speed { 0.0 };       // 1.0 = normal platter speed
    std::atomic<bool> playing { false };
    std::atomic<bool> scratching { false };
    std::atomic<int> repeatIndex { -1 };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool> hostBpmValid { false };
    std::atomic<double> effectiveBpm { 120.0 };
    std::atomic<double> tempoRatio { 1.0 };
    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };
    std::atomic<uint64_t> sampleSeq { 0 };
};

} // namespace ts
