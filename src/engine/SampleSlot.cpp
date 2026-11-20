#include "SampleSlot.h"
#include <algorithm>

namespace ts
{

void SampleSlot::submit (std::unique_ptr<SampleData> sample)
{
    if (sample == nullptr)
        return;

    sample->seq = nextSeq++;
    auto* raw = sample.get();
    owned.push_back (std::move (sample));
    // An older, never-picked-up sample may come back here. It stays owned
    // until the audio thread acknowledges something newer.
    incoming.exchange (raw, std::memory_order_acq_rel);
}

void SampleSlot::collectGarbage()
{
    const auto ack = acknowledged.load (std::memory_order_acquire);
    if (ack == 0 || owned.size() <= 1)
        return;

    owned.erase (std::remove_if (owned.begin(), owned.end(),
                                 [ack] (const std::unique_ptr<SampleData>& s) { return s->seq < ack; }),
                 owned.end());
}

const SampleData* SampleSlot::acquire() noexcept
{
    if (auto* next = incoming.exchange (nullptr, std::memory_order_acq_rel))
    {
        current = next;
        acknowledged.store (next->seq, std::memory_order_release);
    }
    return current;
}

} // namespace ts
