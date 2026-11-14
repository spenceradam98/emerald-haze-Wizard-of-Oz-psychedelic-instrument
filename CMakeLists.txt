cmake_minimum_required(VERSION 3.22)

# macOS 11 (Big Sur) on Intel is the minimum target; universal binary (Intel + Apple Silicon).
# No CPU-specific flags anywhere (plain SSE2 baseline, no AVX/AVX2), so older Intel/AMD machines are fine.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()
# Windows: static MSVC runtime so nobody needs the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(EmeraldHaze VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: -DJUCE_DIR=/path/to/JUCE, a ./JUCE checkout, or fetch 8.0.9
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(EmeraldHaze
    COMPANY_NAME "Emerald Haze Audio"
    BUNDLE_ID com.emeraldhaze.emeraldhaze
    PLUGIN_MANUFACTURER_CODE EmHz
    PLUGIN_CODE EmH1
    FORMATS VST3 AU Standalone
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "Emerald Haze"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(EMHAZE_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FactoryPresets.cpp
    Source/PresetManager.cpp)

target_sources(EmeraldHaze PRIVATE ${EMHAZE_SOURCES})

target_compile_definitions(EmeraldHaze PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(EmeraldHaze PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---- Offline render / screenshot tool (dev only): renders every preset to WAV,
# flags silence/NaN/clipping, snapshots the editor in every theme, and with
# --calibrate writes Source/FactoryPresetLevels.h (per-preset loudness match).
option(EMHAZE_BUILD_TOOLS "Build the offline render/snapshot tool" OFF)
if(EMHAZE_BUILD_TOOLS)
    juce_add_console_app(EmHazeRender PRODUCT_NAME "EmHazeRender")
    target_sources(EmHazeRender PRIVATE tools/Render.cpp ${EMHAZE_SOURCES})
    target_compile_definitions(EmHazeRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 EMHAZE_NO_PLUGIN_ENTRY=1)
    target_link_libraries(EmHazeRender PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
