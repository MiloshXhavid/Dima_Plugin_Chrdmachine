cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.9.0)

# Generate Inno Setup installer script from template (Windows only — configure_file fails on Mac)
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimeaArcade-ChordControl-Setup.iss.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimeaArcade-ChordControl-Setup.iss"
        @ONLY
    )
endif()

# MAC: Universal binary — MUST be set before FetchContent so JUCE/SDL2 static libs are built fat
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

# Static CRT — Windows/MSVC only; must precede FetchContent so JUCE internals also get /MT
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
        Tests/CustomCcRoutingTests.cpp
        Tests/LfoSisterAttenTests.cpp
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
            Resources/SpaceBackground.png
)

# Single source of truth for name strings — used by juce_add_plugin AND the RC template.
set(PLUGIN_PRODUCT_NAME "Arcade Chord Control Beta-Test")
set(PLUGIN_COMPANY_NAME "Dimea Arcade")

juce_add_plugin(ChordJoystick
    PRODUCT_NAME            "${PLUGIN_PRODUCT_NAME}"
    COMPANY_NAME            "${PLUGIN_COMPANY_NAME}"
    BUNDLE_ID               "com.dimeaarcade.chordcontrol"
    FORMATS                 VST3 AU
    PLUGIN_MANUFACTURER_CODE "MxCJ"
    PLUGIN_CODE             "DCJM"
    VST3_CATEGORIES         "Instrument" "Fx"
    AU_MAIN_TYPE            "kAudioUnitType_MusicDevice"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    HARDENED_RUNTIME_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Regenerate the Windows VERSIONINFO resource from the template so Reaper always
# sees the correct ProductName / FileDescription in the DLL. configure_file() runs
# at cmake-configure time and overwrites JUCE's auto-generated RC file.
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordJoystick_resources.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ChordJoystick_artefacts/JuceLibraryCode/ChordJoystick_resources.rc"
        @ONLY
    )
endif()

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

if(WIN32)
    # After every successful build: wipe Reaper's VST cache so it always does a
    # full fresh rescan on next Reaper startup. Without this, Reaper uses the
    # cached checksum and never reloads the updated plugin factory info.
    set(REAPER_VST_CACHE
        "C:/Users/Milosh Xhavid/AppData/Roaming/REAPER/reaper-vstplugins64.ini")
    add_custom_command(TARGET ChordJoystick_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f "${REAPER_VST_CACHE}"
        COMMENT "Cleared Reaper VST cache — restart Reaper to load the new build"
        VERBATIM)

    # Also copy to the user-local VST3 scan path so both locations stay in sync.
    set(LOCAL_VST3_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    add_custom_command(TARGET ChordJoystick_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:ChordJoystick_VST3>/../../"
            "${LOCAL_VST3_DIR}/${PLUGIN_PRODUCT_NAME}.vst3"
        COMMENT "Copied to local VST3 path: ${LOCAL_VST3_DIR}"
        VERBATIM)
elseif(APPLE)
    # Copy VST3 to system VST3 path and AU to user AU path after build.
    add_custom_command(TARGET ChordJoystick_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:ChordJoystick_VST3>"
            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/${PLUGIN_PRODUCT_NAME}.vst3"
        COMMENT "Copied VST3 to ~/Library/Audio/Plug-Ins/VST3/"
        VERBATIM)
    add_custom_command(TARGET ChordJoystick_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:ChordJoystick_AU>"
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components/${PLUGIN_PRODUCT_NAME}.component"
        COMMENT "Copied AU to ~/Library/Audio/Plug-Ins/Components/"
        VERBATIM)
endif()
