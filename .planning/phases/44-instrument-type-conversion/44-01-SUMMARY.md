---
phase: 44-instrument-type-conversion
plan: 01
subsystem: infra
tags: [juce, cmake, vst3, plugin-type, instrument, midi-effect]

# Dependency graph
requires: []
provides:
  - VST3 plugin classified as instrument type (IS_SYNTH TRUE, IS_MIDI_EFFECT FALSE)
  - Stereo silent output bus enabled — hosts see audio output port
  - isBusesLayoutSupported accepts numIn=0, numOut=0 or 2 — instrument layout
  - isMidiEffect() returns false — Ableton places in instrument slot
affects: [build, distribution, phase-45-if-any]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Instrument-type JUCE plugin: IS_SYNTH TRUE + IS_MIDI_EFFECT FALSE + enabled output bus + isBusesLayoutSupported(numIn==0)"

key-files:
  created: []
  modified:
    - CMakeLists.txt
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "IS_MIDI_EFFECT FALSE in CMakeLists required — JUCE VST3 wrapper ignores C++ isMidiEffect() for bus configuration"
  - "Output bus enabled (true) in BusesProperties — instrument slot visibility in FL Studio, Cakewalk, Logic requires active output bus"
  - ".withInput() removed entirely — instruments don't consume audio input; keeping it would invite host audio routing confusion"
  - "isBusesLayoutSupported accepts numOut=0 — some DAWs query with 0 output channels during instrument discovery"
  - "audio.clear() in processBlock already present — silent output guaranteed, no audio engine changes needed"

patterns-established:
  - "Instrument-type conversion: 4 surgical changes only (CMake flags, isMidiEffect, BusesProperties, isBusesLayoutSupported) — all MIDI/LFO/looper/arp subsystems untouched"

requirements-completed: []

# Metrics
duration: 5min
completed: 2026-03-07
---

# Phase 44 Plan 01: Instrument Type Conversion — Code Changes Summary

**Converted ChordJoystick from MIDI effect to VST3 instrument type via 4 surgical changes: CMake flags, isMidiEffect() false, enabled stereo output bus, updated isBusesLayoutSupported — all MIDI/LFO/looper subsystems unaffected**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-07T00:00:00Z
- **Completed:** 2026-03-07T00:05:00Z
- **Tasks:** 4 code changes + 1 commit
- **Files modified:** 3

## Accomplishments

- CMakeLists.txt: `IS_MIDI_EFFECT FALSE`, `IS_SYNTH TRUE`, `VST3_CATEGORIES "Instrument" "Fx"`
- PluginProcessor.h: `isMidiEffect()` now returns `false`
- PluginProcessor.cpp constructor: removed `.withInput(...)`, changed `.withOutput(...)` to `true` (enabled)
- PluginProcessor.cpp `isBusesLayoutSupported`: rewritten to accept `numIn==0 && (numOut==0 || numOut==2)`

## Task Commits

All changes committed atomically in a single task commit per plan spec:

1. **Tasks 1-4: All four code changes** - `9ad14bf` (feat)

## Files Created/Modified

- `CMakeLists.txt` - Plugin type flags: IS_MIDI_EFFECT FALSE, IS_SYNTH TRUE, VST3_CATEGORIES Instrument|Fx
- `Source/PluginProcessor.h` - isMidiEffect() returns false
- `Source/PluginProcessor.cpp` - Constructor output bus enabled; isBusesLayoutSupported updated for instrument layout

## Decisions Made

- Removed `.withInput()` entirely rather than keeping it disabled — instruments don't take audio input and an inactive input bus could confuse routing in some hosts
- `isBusesLayoutSupported` now uses `numIn == 0 && (numOut == 0 || numOut == 2)` — the `numOut==0` branch keeps compatibility with DAWs that probe the plugin with no outputs during discovery
- `audio.clear()` in processBlock already existed — no change needed; plugin outputs silence

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

Build and install required after this plan:
- Run `& "C:\Users\Milosh Xhavid\get-shit-done\build32.ps1"` (reconfigures CMake + builds)
- Run `& "C:\Users\Milosh Xhavid\get-shit-done\do-reinstall.ps1"` (installs VST3)
- Verify plugin appears in instrument slot (not MIDI FX slot) in target DAWs

## Next Phase Readiness

- All code changes complete and committed
- Awaiting build + manual verification that plugin loads in instrument slot
- Ableton/Reaper existing presets remain compatible (PLUGIN_CODE "DCJM" unchanged)

---
*Phase: 44-instrument-type-conversion*
*Completed: 2026-03-07*
