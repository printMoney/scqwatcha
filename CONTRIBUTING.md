# Contributing

Thanks for helping. A few rules keep the plugin stable.

## Before you open a pull request

1. `scripts/build.sh`: must build with no errors, and the engine tests must print `0 failed`.
2. `scripts/validate.sh`: auval and pluginval (strictness 5) must pass for both VST3 and AU.
3. If you changed behaviour, add or update a check in `tests/TestMain.cpp`.

CI (`.github/workflows/build.yml`) runs all of this on macOS for every push.

## Audio-thread rules

Code reachable from `processBlock` or `DeckEngine::process` must not:

- allocate or free memory (no `new`, `std::vector::push_back`, `juce::String`, and so on)
- take locks or wait
- call the file system, logging or any OS API

The test runner overrides `operator new` and fails if `process()` allocates.
Send data from the UI through `CommandQueue` or atomics, and samples through `SampleSlot`.

## Clicks

Any change to the playhead that is not continuous must go through `DeckEngine::jump()`, which crossfades.
The tests measure the largest sample-to-sample step; keep it under the threshold in `tests/TestMain.cpp`.

## Style

- C++17, JUCE style (4-space indent, braces on new lines, space before call parentheses).
- Keep UI code in `src/ui`, engine code in `src/engine` (engine code must not depend on GUI modules).
- New hotkey actions: add them to `Action` and `HotkeyMap::actions()`, handle them in `TurntableEditor::perform`, then regenerate the example file:

  ```bash
  build/TurntableTests_artefacts/Release/TurntableTests --print-hotkeys > config/hotkeys.example.json
  ```

## Licensing

Contributions are accepted under GPLv3. Do not add proprietary dependencies, or libraries whose licence is incompatible with GPLv3.
