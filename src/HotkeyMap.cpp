#include "HotkeyMap.h"
#include <algorithm>

#if JUCE_MAC
 #include "KeyboardLayout.h"
#endif

namespace ts
{

namespace
{
// US layout, used when the system layout cannot be read.
const char* const shiftedChars = "!@#$%^&*(){}_+?<>:\"|~";
const char* const baseChars    = "1234567890[]-=/,.;'\\`";

const std::map<juce::juce_wchar, juce::juce_wchar>* testLayout = nullptr;
std::map<juce::juce_wchar, juce::juce_wchar> systemLayout;
bool systemLayoutLoaded = false;

const std::map<juce::juce_wchar, juce::juce_wchar>& activeLayout()
{
    if (testLayout != nullptr)
        return *testLayout;
    if (! systemLayoutLoaded)
        HotkeyMap::refreshLayout();
    return systemLayout;
}

int unshift (int c, bool& wasShifted)
{
    const auto& layout = activeLayout();
    if (! layout.empty())
    {
        auto it = layout.find ((juce::juce_wchar) c);
        if (it != layout.end())
        {
            wasShifted = true;
            int base = (int) it->second;
            if (base >= 'a' && base <= 'z')
                base = base - 'a' + 'A';
            return base;
        }
        return c;
    }

    for (int i = 0; shiftedChars[i] != 0; ++i)
        if (c == shiftedChars[i])
        {
            wasShifted = true;
            return baseChars[i];
        }
    return c;
}

bool isSymbol (juce::juce_wchar c)
{
    return c > 32 && ! juce::CharacterFunctions::isLetterOrDigit (c);
}

struct NamedKey
{
    const char* name;
    int code;
};

std::vector<NamedKey> namedKeys()
{
    return {
        { "Space", juce::KeyPress::spaceKey },
        { "Up", juce::KeyPress::upKey },
        { "Down", juce::KeyPress::downKey },
        { "Left", juce::KeyPress::leftKey },
        { "Right", juce::KeyPress::rightKey },
        { "Return", juce::KeyPress::returnKey },
        { "Escape", juce::KeyPress::escapeKey },
        { "Tab", juce::KeyPress::tabKey },
        { "Backspace", juce::KeyPress::backspaceKey },
        { "Delete", juce::KeyPress::deleteKey },
        { "Home", juce::KeyPress::homeKey },
        { "End", juce::KeyPress::endKey },
        { "PageUp", juce::KeyPress::pageUpKey },
        { "PageDown", juce::KeyPress::pageDownKey },
    };
}
} // namespace

const std::vector<ActionInfo>& HotkeyMap::actions()
{
    static const std::vector<ActionInfo> list {
        { Action::PlayPause,       "play_pause",      "Play / pause (tape-stop ramp)",   "Space",       false, false },
        { Action::HardRestart,     "hard_restart",    "Hard stop, restart at cue",       "Shift+Space", false, false },
        { Action::ScratchHold,     "scratch_hold",    "Scratch (hold + move mouse on platter)", "S",    true,  false },
        { Action::TapeStop,        "tape_stop",       "Tape stop (hold)",                "G",           true,  false },
        { Action::Repeat1,         "repeat_1_1",      "Repeater 1/1 (hold)",             "Q",           true,  false },
        { Action::Repeat2,         "repeat_1_2",      "Repeater 1/2 (hold)",             "W",           true,  false },
        { Action::Repeat4,         "repeat_1_4",      "Repeater 1/4 (hold)",             "E",           true,  false },
        { Action::Repeat8,         "repeat_1_8",      "Repeater 1/8 (hold)",             "R",           true,  false },
        { Action::Repeat16,        "repeat_1_16",     "Repeater 1/16 (hold)",            "T",           true,  false },
        { Action::LoopToggle,      "loop_toggle",     "Loop on / off",                   "L",           false, false },
        { Action::LoopIn,          "loop_in",         "Loop in here (snaps to beat)",   "[ or I",         false, false },
        { Action::LoopOut,         "loop_out",        "Loop out here (snaps to beat)",  "] or O",         false, false },
        { Action::SetCue,          "set_cue",         "Set cue point at playhead",       "C",           false, false },
        { Action::SnapToggle,      "snap_toggle",     "Snap loops to the beat on / off", "N",           false, false },
        { Action::Pad1,            "pad_1",           "Pad 1: play loop (empty: save new)", "1",           false, false },
        { Action::Pad2,            "pad_2",           "Pad 2: play loop (empty: save new)", "2",           false, false },
        { Action::Pad3,            "pad_3",           "Pad 3: play loop (empty: save new)", "3",           false, false },
        { Action::Pad4,            "pad_4",           "Pad 4: play loop (empty: save new)", "4",           false, false },
        { Action::Pad5,            "pad_5",           "Pad 5: play loop (empty: save new)", "5",           false, false },
        { Action::Pad6,            "pad_6",           "Pad 6: play loop (empty: save new)", "6",           false, false },
        { Action::Pad7,            "pad_7",           "Pad 7: play loop (empty: save new)", "7",           false, false },
        { Action::Pad8,            "pad_8",           "Pad 8: play loop (empty: save new)", "8",           false, false },
        { Action::PadStore1,       "pad_store_1",     "Store loop in pad 1",             "Shift+1",     false, false },
        { Action::PadStore2,       "pad_store_2",     "Store loop in pad 2",             "Shift+2",     false, false },
        { Action::PadStore3,       "pad_store_3",     "Store loop in pad 3",             "Shift+3",     false, false },
        { Action::PadStore4,       "pad_store_4",     "Store loop in pad 4",             "Shift+4",     false, false },
        { Action::PadStore5,       "pad_store_5",     "Store loop in pad 5",             "Shift+5",     false, false },
        { Action::PadStore6,       "pad_store_6",     "Store loop in pad 6",             "Shift+6",     false, false },
        { Action::PadStore7,       "pad_store_7",     "Store loop in pad 7",             "Shift+7",     false, false },
        { Action::PadStore8,       "pad_store_8",     "Store loop in pad 8",             "Shift+8",     false, false },
        { Action::BpmUp,           "bpm_up",          "System BPM +1",                   "Up",          false, true  },
        { Action::BpmDown,         "bpm_down",        "System BPM -1",                   "Down",        false, true  },
        { Action::BpmUp10,         "bpm_up_10",       "System BPM +10",                  "Shift+Up",    false, true  },
        { Action::BpmDown10,       "bpm_down_10",     "System BPM -10",                  "Shift+Down",  false, true  },
        { Action::OverrideToggle,  "override_toggle", "Override host BPM on / off",      "B",           false, false },
        { Action::SyncToggle,      "sync_toggle",     "BPM sync on / off",               "Y",           false, false },
        { Action::KeepPitchToggle, "keep_pitch",      "Keep pitch on / off",             "K",           false, false },
        { Action::PitchDown,       "pitch_down",      "Pitch -1 semitone (Keep pitch)",  "-",           false, true  },
        { Action::PitchUp,         "pitch_up",        "Pitch +1 semitone (Keep pitch)",  "=",           false, true  },
        { Action::FilterBypass,    "filter_bypass",   "Filter bypass on / off",          "F",           false, false },
        { Action::CutoffDown,      "cutoff_down",     "Filter cutoff down",              "Left",        false, true  },
        { Action::CutoffUp,        "cutoff_up",       "Filter cutoff up",                "Right",       false, true  },
        { Action::ResonanceDown,   "resonance_down",  "Filter resonance down",           "Alt+Down",    false, true  },
        { Action::ResonanceUp,     "resonance_up",    "Filter resonance up",             "Alt+Up",      false, true  },
        { Action::MixDown,         "mix_down",        "Repeater mix down",               "Z",           false, true  },
        { Action::MixUp,           "mix_up",          "Repeater mix up",                 "X",           false, true  },
        { Action::TapeTimeDown,    "tape_time_down",  "Tape stop time shorter",          ",",           false, true  },
        { Action::TapeTimeUp,      "tape_time_up",    "Tape stop time longer",           ".",           false, true  },
        { Action::GainDown,        "gain_down",       "Output gain down",                "9",           false, true  },
        { Action::GainUp,          "gain_up",         "Output gain up",                  "0",           false, true  },
        { Action::Undo,            "undo",            "Undo loop edit",                  "Cmd+Z",       false, false },
        { Action::Redo,            "redo",            "Redo loop edit",                  "Cmd+Shift+Z", false, false },
        { Action::LoadSample,      "load_sample",     "Load a sample",                   "Cmd+O",       false, false },
        { Action::ShowHelp,        "show_help",       "Show / hide this overlay",        "? or H",      false, false },
    };
    return list;
}

const ActionInfo* HotkeyMap::info (Action a)
{
    for (auto& i : actions())
        if (i.action == a)
            return &i;
    return nullptr;
}

void HotkeyMap::refreshLayout()
{
#if JUCE_MAC
    systemLayout = currentLayoutShiftMap();
#endif
    systemLayoutLoaded = true;
}

void HotkeyMap::setLayoutForTesting (const std::map<juce::juce_wchar, juce::juce_wchar>* shiftMap)
{
    testLayout = shiftMap;
}

void HotkeyMap::add (Action a, const Binding& b)
{
    for (auto& [action, binding] : bindings)
        if (action == a && binding == b)
            return;
    bindings.emplace_back (a, b);
}

void HotkeyMap::setDefaults()
{
    bindings.clear();
    for (auto& i : actions())
    {
        juce::StringArray keys;
        keys.addTokens (i.defaultKeys, " or ", "");
        for (auto& k : keys)
            if (auto b = parse (k))
                add (i.action, *b);
    }
}

std::optional<Binding> HotkeyMap::parse (const juce::String& text)
{
    auto t = text.trim();
    if (t.isEmpty())
        return std::nullopt;

    Binding b;

    // "+" alone, or as the last token ("Shift++"), is the key itself.
    juce::StringArray tokens;
    if (t == "+")
        tokens.add ("+");
    else if (t.endsWith ("++"))
    {
        tokens.addTokens (t.dropLastCharacters (2), "+", "");
        tokens.add ("+");
    }
    else
        tokens.addTokens (t, "+", "");
    tokens.trim();
    tokens.removeEmptyStrings();
    if (tokens.isEmpty())
        return std::nullopt;

    for (int i = 0; i < tokens.size() - 1; ++i)
    {
        const auto m = tokens[i].toLowerCase();
        if (m == "shift")                                     b.shift = true;
        else if (m == "cmd" || m == "command" || m == "meta") b.cmd = true;
        else if (m == "alt" || m == "option" || m == "opt")   b.alt = true;
        else if (m == "ctrl" || m == "control")               b.ctrl = true;
        else return std::nullopt;
    }
    const bool hasModifiers = tokens.size() > 1;

    const auto keyPart = tokens[tokens.size() - 1];
    for (auto& nk : namedKeys())
        if (keyPart.equalsIgnoreCase (nk.name))
        {
            b.key = nk.code;
            return b;
        }

    if (keyPart.length() != 1)
        return std::nullopt;

    int c = (int) keyPart[0];

    // A bare symbol matches whatever keys type it on this keyboard.
    if (! hasModifiers && isSymbol ((juce::juce_wchar) c))
    {
        b.ch = (juce::juce_wchar) c;
        return b;
    }

    if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    bool shifted = false;
    c = unshift (c, shifted);
    if (shifted)
        b.shift = true;
    b.key = c;
    return b;
}

juce::String HotkeyMap::format (const Binding& b)
{
    if (b.ch != 0)
        return juce::String::charToString (b.ch);

    juce::String s;
    if (b.cmd)   s << "Cmd+";
    if (b.ctrl)  s << "Ctrl+";
    if (b.alt)   s << "Alt+";
    if (b.shift) s << "Shift+";

    for (auto& nk : namedKeys())
        if (nk.code == b.key)
            return s + nk.name;

    return s + juce::String::charToString ((juce::juce_wchar) b.key);
}

Binding HotkeyMap::fromKeyPress (const juce::KeyPress& k)
{
    Binding b;
    const auto mods = k.getModifiers();
    b.shift = mods.isShiftDown();
    b.cmd = mods.isCommandDown();
    b.alt = mods.isAltDown();
    b.ctrl = mods.isCtrlDown();

    int c = k.getKeyCode();
    if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    bool shifted = false;
    c = unshift (c, shifted);
    if (shifted)
        b.shift = true;
    b.key = c;
    return b;
}

std::optional<Action> HotkeyMap::find (const Binding& b) const
{
    for (auto& [action, binding] : bindings)
        if (binding == b)
            return action;
    return std::nullopt;
}

std::optional<Action> HotkeyMap::find (const juce::KeyPress& k) const
{
    // 1. Exact key + modifiers.
    if (auto a = find (fromKeyPress (k)))
        return a;

    // 2. Typed symbol ("?", "[", "="), however the layout produces it.
    const auto mods = k.getModifiers();
    const auto typed = k.getTextCharacter();
    if (typed != 0 && ! mods.isCommandDown() && ! mods.isCtrlDown())
        for (auto& [action, binding] : bindings)
            if (binding.ch != 0 && binding.ch == typed)
                return action;

    return std::nullopt;
}

std::vector<Binding> HotkeyMap::get (Action a) const
{
    std::vector<Binding> result;
    for (auto& [action, binding] : bindings)
        if (action == a)
            result.push_back (binding);
    return result;
}

juce::String HotkeyMap::describe (Action a) const
{
    juce::StringArray parts;
    for (auto& b : get (a))
        parts.add (format (b));
    return parts.isEmpty() ? juce::String ("-") : parts.joinIntoString (" / ");
}

juce::StringArray HotkeyMap::loadFromJson (const juce::String& json)
{
    juce::StringArray problems;
    setDefaults();

    juce::var root;
    const auto result = juce::JSON::parse (json, root);
    if (result.failed())
    {
        problems.add ("hotkeys.json is not valid JSON: " + result.getErrorMessage());
        return problems;
    }

    auto* obj = root.getProperty ("bindings", juce::var()).getDynamicObject();
    if (obj == nullptr)
    {
        problems.add ("hotkeys.json has no \"bindings\" object");
        return problems;
    }

    for (auto& prop : obj->getProperties())
    {
        const auto id = prop.name.toString();
        const ActionInfo* found = nullptr;
        for (auto& i : actions())
            if (id == i.id)
                found = &i;

        if (found == nullptr)
        {
            problems.add ("Unknown action \"" + id + "\"");
            continue;
        }

        // The file replaces all default keys for this action.
        bindings.erase (std::remove_if (bindings.begin(), bindings.end(),
                                        [found] (const std::pair<Action, Binding>& p) { return p.first == found->action; }),
                        bindings.end());

        juce::StringArray keys;
        if (auto* arr = prop.value.getArray())
            for (auto& v : *arr)
                keys.add (v.toString());
        else
            keys.add (prop.value.toString());

        for (auto& text : keys)
        {
            if (text.isEmpty() || text == "none")
                continue;
            if (auto b = parse (text))
                add (found->action, *b);
            else
                problems.add ("Cannot read key \"" + text + "\" for " + id);
        }
    }

    // Report clashes: two actions on one key means only one of them works.
    for (size_t i = 0; i < bindings.size(); ++i)
        for (size_t j = i + 1; j < bindings.size(); ++j)
            if (bindings[i].second == bindings[j].second && bindings[i].first != bindings[j].first)
                problems.add (format (bindings[i].second) + " is assigned twice (" + juce::String (info (bindings[i].first)->id)
                              + ", " + juce::String (info (bindings[j].first)->id) + ")");

    return problems;
}

juce::String HotkeyMap::toJson() const
{
    auto* bindingsObj = new juce::DynamicObject();
    for (auto& i : actions())
    {
        const auto list = get (i.action);
        if (list.empty())
            bindingsObj->setProperty (i.id, "none");
        else if (list.size() == 1)
            bindingsObj->setProperty (i.id, format (list.front()));
        else
        {
            juce::Array<juce::var> arr;
            for (auto& b : list)
                arr.add (format (b));
            bindingsObj->setProperty (i.id, arr);
        }
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", fileVersion);
    root->setProperty ("_help", "Key names: A-Z, 0-9, Space, Up, Down, Left, Right, Return, Escape, Tab. "
                                "A symbol on its own (\"?\", \"[\", \"=\") matches however your keyboard types it. "
                                "Modifiers: Shift+, Cmd+, Alt+, Ctrl+. Several keys: [\"[\", \"I\"]. Use \"none\" to unassign. "
                                "Press Reload in the ? overlay after editing.");
    root->setProperty ("bindings", juce::var (bindingsObj));
    return juce::JSON::toString (juce::var (root), false);
}

juce::File HotkeyMap::userFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support")
        .getChildFile ("scqwatcha")
        .getChildFile ("hotkeys.json");
}

juce::StringArray HotkeyMap::loadOrCreateUserFile()
{
    refreshLayout();
    auto file = userFile();
    if (! file.existsAsFile())
    {
        setDefaults();
        file.getParentDirectory().createDirectory();
        file.replaceWithText (toJson());
        return {};
    }

    const auto text = file.loadFileAsString();
    const auto parsed = juce::JSON::parse (text);
    if ((int) parsed.getProperty ("version", 1) < fileVersion)
    {
        // Older defaults: keep a backup and write the new defaults.
        file.copyFileTo (file.getSiblingFile ("hotkeys.v1-backup.json"));
        setDefaults();
        file.replaceWithText (toJson());
        return {};
    }

    return loadFromJson (text);
}

} // namespace ts
