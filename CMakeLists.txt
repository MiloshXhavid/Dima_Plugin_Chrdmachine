cmake_minimum_required(VERSION 3.22)

project(ChordJoystick VERSION 1.0.0)

# JUCE CMake setup
include(FetchContent)

# Fetch JUCE 8.x
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG origin/master
)

# Configure JUCE - Disable AUv3/Standalone for now (not needed for current task)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_AUDIO_PLUGINS_AS_VST3 ON CACHE BOOL "" FORCE)
set(JUCE_AUDIO_PLUGINS_AS_AUV3 OFF CACHE BOOL "" FORCE)
set(JUCE_AUDIO_PLUGINS_AS_STANDALONE OFF CACHE BOOL "" FORCE)
set(JUCE_AUDIO_PLUGINS_AS_VST2 OFF CACHE BOOL "" FORCE)
set(JUCE_FORCE_USE_LEGACY_PARAM_IDS ON CACHE BOOL "" FORCE)

# Add explicit compile definition for VST3
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)

FetchContent_MakeAvailable(juce)

# Plugin configuration - MIDI Controller (not Synth)
juce_add_plugin(ChordJoystick
  PRODUCT_NAME "ChordJoystick"
  FORMATS VST3
  PLUGIN_MANUFACTURER_CODE "ChJo"
  PLUGIN_CODE "ChJo"
  VST3_CATEGORIES Effect
  COPY_PLUGIN_AFTER_BUILD TRUE
)

# C++17 standard
set_target_properties(ChordJoystick PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Source files
target_sources(ChordJoystick PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)

# JUCE modules
target_link_libraries(ChordJoystick PRIVATE
  juce::juce_audio_processors
  juce::juce_gui_basics
)
