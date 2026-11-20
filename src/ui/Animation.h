#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <functional>

namespace ts
{

// A value that glides towards a target. rate = how fast (1/seconds).
struct Smooth
{
    float value = 0.0f;
    float target = 0.0f;

    // Returns true while still moving.
    bool update (float dt, float rate) noexcept
    {
        if (value == target)
            return false;
        value += (target - value) * (1.0f - std::exp (-dt * rate));
        if (std::abs (target - value) < 0.002f)
            value = target;
        return true;
    }

    void snap (float v) noexcept { value = target = v; }
};

// Ticks at the display refresh rate while the component is on screen and
// repaints only when animate() reports that something moved.
class Animated
{
public:
    explicit Animated (juce::Component& c)
        : owner (c),
          vblank (&c, [this] (double t) { tick (t); })
    {
    }

    virtual ~Animated() = default;

protected:
    // dt in seconds. Return true if a repaint is needed.
    virtual bool animate (float dt) = 0;

private:
    void tick (double timestampSec)
    {
        const float dt = lastTime > 0.0 ? (float) juce::jlimit (0.0, 0.1, timestampSec - lastTime) : 1.0f / 60.0f;
        lastTime = timestampSec;
        if (animate (dt))
            owner.repaint();
    }

    juce::Component& owner;
    juce::VBlankAttachment vblank;
    double lastTime = 0.0;
};

} // namespace ts
