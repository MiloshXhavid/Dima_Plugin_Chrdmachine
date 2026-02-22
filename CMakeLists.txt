cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

# Static CRT — must precede all FetchContent calls so JUCE internals also get /MT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# ─── JUCE ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4        # commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a — Nov 18 2024
    GIT_SHALLOW    TRUE
)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(juce)

# ─── SDL2 (gamepad support) ──────────────────────────────────────────────────
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.9
)
set(SDL_SHARED          OFF CACHE BOOL "" FORCE)
set(SDL_STATIC          ON  CACHE BOOL "" FORCE)
set(SDL_JOYSTICK        ON  CACHE BOOL "" FORCE)
set(SDL_GAMECONTROLLER  ON  CACHE BOOL "" FORCE)
set(SDL_HAPTIC          OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO           OFF CACHE BOOL "" FORCE)
set(SDL_VIDEO           OFF CACHE BOOL "" FORCE)
set(SDL_RENDER          OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR          OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# ─── Plugin target ───────────────────────────────────────────────────────────
juce_add_plugin(ChordJoystick
    PRODUCT_NAME            "ChordJoystick"
    FORMATS                 VST3
    PLUGIN_MANUFACTURER_CODE "MxCJ"
    PLUGIN_CODE             "CJoy"
    VST3_CATEGORIES         "Instrument"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

set_target_properties(ChordJoystick PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED ON
)

target_sources(ChordJoystick PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ScaleQuantizer.cpp
    Source/ChordEngine.cpp
    Source/TriggerSystem.cpp
    Source/LooperEngine.cpp
    Source/GamepadInput.cpp
)

target_compile_definitions(ChordJoystick PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordJoystick PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    SDL2-static
)
