#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <optional>
#include <vector>

namespace ts
{

enum class Action
{
    PlayPause, HardRestart, ScratchHold, TapeStop,
    Repeat1, Repeat2, Repeat4, Repeat8, Repeat16,
    LoopToggle, LoopIn, LoopOut, SetCue,
    Pad1, Pad2, Pad3, Pad4, Pad5, Pad6, Pad7, Pad8,
    PadStore1, PadStore2, PadStore3, PadStore4, PadStore5, PadStore6, PadStore7, PadStore8,
    BpmUp, BpmDown, BpmUp10, BpmDown10, OverrideToggle, SyncToggle,
    KeepPitchToggle, PitchDown, PitchUp,
    FilterBypass, CutoffDown, CutoffUp, ResonanceDown, ResonanceUp,
    MixDown, MixUp, TapeTimeDown, TapeTimeUp, GainDown, GainUp,
    Undo, Redo, LoadSample, ShowHelp
};

struct ActionInfo
{
    Action action;
    const char* id;          // name used in hotkeys.json
    const char* label;       // shown in the overlay
    const char* defaultKey;
    bool momentary;          // press and release both matter
    bool autoRepeat;         // allow OS key repeat
};

struct Binding
{
    int key = 0; // juce key code; letters upper case, symbols unshifted
    bool shift = false, cmd = false, alt = false, ctrl = false;

    bool operator== (const Binding& o) const noexcept
    {
        return key == o.key && shift == o.shift && cmd == o.cmd && alt == o.alt && ctrl == o.ctrl;
    }
    bool isValid() const noexcept { return key != 0; }
};

class HotkeyMap
{
public:
    HotkeyMap() { setDefaults(); }

    static const std::vector<ActionInfo>& actions();
    static const ActionInfo* info (Action a);

    void setDefaults();
    // Missing actions keep their defaults. Returns problems found (empty = fine).
    juce::StringArray loadFromJson (const juce::String& json);
    juce::String toJson() const;

    std::optional<Action> find (const Binding& b) const;
    juce::String describe (Action a) const;
    Binding get (Action a) const;

    static std::optional<Binding> parse (const juce::String& text);
    static juce::String format (const Binding& b);
    static Binding fromKeyPress (const juce::KeyPress& k);

    static juce::File userFile();
    // Loads the user file, creating it from the defaults if it does not exist.
    juce::StringArray loadOrCreateUserFile();

private:
    std::map<Action, Binding> bindings;
};

} // namespace ts
