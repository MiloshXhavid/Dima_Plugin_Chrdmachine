cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.7.0)

# Generate Inno Setup installer script from template (single version source of truth)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimaChordJoystick-Setup.iss.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimaChordJoystick-Setup.iss"
    @ONLY
)

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

# ─── Catch2 (unit tests, opt-in) ─────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(ChordJoystickTests
        Source/ScaleQuantizer.cpp
        Source/ChordEngine.cpp
        Source/LooperEngine.cpp
        Source/LfoEngine.cpp
        Tests/ScaleQuantizerTests.cpp
        Tests/ChordEngineTests.cpp
        Tests/LooperEngineTests.cpp
        Tests/LfoEngineTests.cpp
        Tests/ChordNameTests.cpp
    )

    set_target_properties(ChordJoystickTests PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
    )

    target_compile_definitions(ChordJoystickTests PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_audio_processors=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISABLE_ASSERTIONS=1
    )

    target_include_directories(ChordJoystickTests PRIVATE Source)

    target_link_libraries(ChordJoystickTests PRIVATE
        juce::juce_audio_processors
        Catch2::Catch2WithMain
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    enable_testing()
    include(CTest)
    include(Catch)
    catch_discover_tests(ChordJoystickTests)

    # ─── ThreadSanitizer (TSAN) option — Linux/Clang only, silently skipped on MSVC ──
    option(ENABLE_TSAN "Enable ThreadSanitizer — Linux/Clang only, ignored on MSVC" OFF)

    if(ENABLE_TSAN)
        if(MSVC)
            message(WARNING "ENABLE_TSAN=ON ignored: ThreadSanitizer is not available on MSVC/Windows. Use Debug-mode ASSERT_AUDIO_THREAD() macro as fallback.")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(ChordJoystickTests PRIVATE -fsanitize=thread -g)
            target_link_options(ChordJoystickTests PRIVATE -fsanitize=thread)
            message(STATUS "ThreadSanitizer enabled for ChordJoystickTests")
        endif()
    endif()
endif()

# ─── Plugin target ───────────────────────────────────────────────────────────
juce_add_binary_data(ChordJoystickFonts
    SOURCES Fonts/PressStart2P-Regular.ttf
            Resources/DimeaLogo.png
)

juce_add_plugin(ChordJoystick
    PRODUCT_NAME            "Joystick Chord Control (β-test)"
    COMPANY_NAME            "Dimea Arcade"
    FORMATS                 VST3
    PLUGIN_MANUFACTURER_CODE "MxCJ"
    PLUGIN_CODE             "DCJM"
    VST3_CATEGORIES         "Fx" "MIDI"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          TRUE
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
    Source/LfoEngine.cpp
    Source/GamepadInput.cpp
    Source/SdlContext.cpp
)

target_compile_definitions(ChordJoystick PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordJoystick PRIVATE
    ChordJoystickFonts
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    SDL2-static
)

# After every successful build: wipe Reaper's VST cache so it always does a
# full fresh rescan on next Reaper startup. Without this, Reaper uses the
# cached checksum and never reloads the updated plugin factory info.
set(REAPER_VST_CACHE
    "C:/Users/Milosh Xhavid/AppData/Roaming/REAPER/reaper-vstplugins64.ini")
add_custom_command(TARGET ChordJoystick_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f "${REAPER_VST_CACHE}"
    COMMENT "Cleared Reaper VST cache — restart Reaper to load the new build"
    VERBATIM)
