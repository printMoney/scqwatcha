cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) unless the caller overrides it.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(TurntableSampler VERSION 0.1.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TS_BUILD_TESTS "Build the engine test runner" ON)
option(TS_COPY_PLUGINS "Install plugins into ~/Library/Audio/Plug-Ins after building" ON)

# JUCE 8, pinned. Pass -DFETCHCONTENT_SOURCE_DIR_JUCE=<path> to use a local checkout.
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

set(TS_ENGINE_SOURCES
    src/engine/DeckEngine.cpp
    src/engine/SampleSlot.cpp
    src/engine/BpmDetect.cpp)

set(TS_COMMON_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_plugin(TurntableSampler
    PRODUCT_NAME                "Turntable Sampler"
    COMPANY_NAME                "Turntable Sampler Project"
    BUNDLE_ID                   org.turntablesampler.plugin
    PLUGIN_MANUFACTURER_CODE    Tsmp
    PLUGIN_CODE                 Ttbl
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     ${TS_COPY_PLUGINS}
    VST3_CATEGORIES             Instrument Sampler
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VERSION                     ${PROJECT_VERSION})

target_sources(TurntableSampler PRIVATE
    ${TS_ENGINE_SOURCES}
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/HotkeyMap.cpp
    src/KeyboardLayout.mm
    src/SampleLoader.cpp
    src/ui/LookAndFeel.cpp
    src/ui/Platter.cpp
    src/ui/Waveform.cpp
    src/ui/XYPad.cpp
    src/ui/HotkeyOverlay.cpp)

target_include_directories(TurntableSampler PRIVATE src)
target_compile_definitions(TurntableSampler PUBLIC ${TS_COMMON_DEFINITIONS})

target_link_libraries(TurntableSampler
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        "-framework Carbon"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(TS_BUILD_TESTS)
    juce_add_console_app(TurntableTests PRODUCT_NAME "TurntableTests")
    target_sources(TurntableTests PRIVATE tests/TestMain.cpp ${TS_ENGINE_SOURCES} src/HotkeyMap.cpp src/KeyboardLayout.mm)
    target_include_directories(TurntableTests PRIVATE src)
    target_compile_definitions(TurntableTests PRIVATE ${TS_COMMON_DEFINITIONS})
    target_link_libraries(TurntableTests PRIVATE
        juce::juce_audio_basics
        juce::juce_gui_basics
        "-framework Carbon"
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()

# Renders the editor to a PNG (used for screenshots in the README).
option(TS_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(TS_BUILD_SNAPSHOT)
    juce_add_console_app(UISnapshot PRODUCT_NAME "UISnapshot")
    get_target_property(TS_PLUGIN_SOURCES TurntableSampler SOURCES)
    list(FILTER TS_PLUGIN_SOURCES INCLUDE REGEX "^src/")
    target_sources(UISnapshot PRIVATE tests/UISnapshot.cpp ${TS_PLUGIN_SOURCES})
    target_include_directories(UISnapshot PRIVATE src)
    target_compile_definitions(UISnapshot PRIVATE ${TS_COMMON_DEFINITIONS} JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(UISnapshot PRIVATE juce::juce_audio_utils juce::juce_audio_formats "-framework Carbon" juce::juce_recommended_config_flags)
endif()
