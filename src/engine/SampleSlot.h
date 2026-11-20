#pragma once

#include "SampleData.h"
#include <atomic>
#include <memory>
#include <vector>

namespace ts
{

// Hands samples from the message thread to the audio thread without locks,
// and makes sure the audio thread never frees memory.
//
// The message thread owns every sample. The audio thread picks up the newest
// one and acknowledges its sequence number; the message thread frees only
// samples older than the acknowledged one.
class SampleSlot
{
public:
    // Message thread.
    void submit (std::unique_ptr<SampleData> sample);
    void collectGarbage();
    const SampleData* latest() const noexcept { return owned.empty() ? nullptr : owned.back().get(); }
    size_t numOwned() const noexcept { return owned.size(); }

    // Audio thread. Call once per block. Returns the sample to play (may be null).
    const SampleData* acquire() noexcept;

private:
    std::atomic<SampleData*> incoming { nullptr };
    std::atomic<uint64_t> acknowledged { 0 };
    const SampleData* current = nullptr; // audio thread only

    std::vector<std::unique_ptr<SampleData>> owned; // message thread only
    uint64_t nextSeq = 1;
};

} // namespace ts
