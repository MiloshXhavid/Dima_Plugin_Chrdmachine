---
phase: 15-gamepad-preset-control
plan: "01"
subsystem: gamepad
tags: [sdl2, gamepad, preset-scroll, program-change, dpad, guide-button]

# Dependency graph
requires:
  - phase: 14-lfo-ui-and-beat-clock
    provides: complete plugin with LFO, beat clock, all prior gamepad mappings
provides:
  - GamepadInput preset-scroll toggle via Option/Guide button
  - consumePcDelta() API for PluginProcessor to read pending PC delta
  - isPresetScrollActive() state query for UI/processor
  - One-frame D-pad lockout when Option fires (prevents spurious BPM delta)
affects:
  - 15-02 (PluginProcessor consumes consumePcDelta() to send Program Change)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Rising-edge toggle for mode switches (presetScrollActive_ flip on button-down)"
    - "Falling-edge fire for PC delta (button-up ensures exactly one PC per physical press)"
    - "One-frame lockout flag (optionFrameFired_) to prevent same-frame D-pad side effects"
    - "atomic<int> pendingPcDelta_ with fetch_add/exchange pattern for thread-safe delta accumulation"

key-files:
  created: []
  modified:
    - Source/GamepadInput.h
    - Source/GamepadInput.cpp

key-decisions:
  - "SDL_CONTROLLER_BUTTON_GUIDE maps to Option/PS button on DualSense, Share/Options on DS4, Guide/Home on Xbox — correct constant for the intended button"
  - "D-pad Up/Down PC delta fires on falling edge (button-up) to guarantee exactly one PC per physical press — rising-edge would fire immediately on press before user has released"
  - "One-frame lockout (optionFrameFired_) prevents D-pad Up/Down from also registering a BPM delta in the same 16ms frame that Option fires"
  - "D-pad Left/Right always use rising-edge regardless of preset-scroll mode — looper toggle semantics are unaffected by the mode"

patterns-established:
  - "Mode toggle: bool flag flipped on rising-edge, same-frame lockout prevents co-trigger"
  - "PC delta: atomic<int> with fetch_add (producer) / exchange(0) (consumer) for lock-free transfer"

requirements-completed: [CTRL-01, CTRL-02]

# Metrics
duration: 2min
completed: 2026-02-26
---

# Phase 15 Plan 01: Gamepad Preset Control — GamepadInput Layer Summary

**Option/Guide button toggle for preset-scroll mode with falling-edge D-pad PC delta (+1/-1) and one-frame lockout in GamepadInput SDL2 layer**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-26T06:28:24Z
- **Completed:** 2026-02-26T06:30:21Z
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments

- Added `presetScrollActive_` toggle fired by `SDL_CONTROLLER_BUTTON_GUIDE` rising-edge (Option/PS button)
- Added `consumePcDelta()` returning pending Program Change delta (+1/-1) for PluginProcessor consumption in Plan 02
- Added one-frame D-pad lockout (`optionFrameFired_`) preventing spurious BPM delta when Option fires
- D-pad Up/Down produce falling-edge PC deltas in preset-scroll mode; rising-edge BPM deltas in normal mode
- D-pad Left/Right remain always rising-edge (looper toggles, mode-independent)
- Build compiles with zero errors and zero new warnings

## Task Commits

Each task was committed atomically:

1. **Task 1: Add Option-button toggle and PC delta to GamepadInput** - `179e273` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `Source/GamepadInput.h` - Added `presetScrollActive_`, `optionFrameFired_`, `btnOption_`, `pendingPcDelta_` private members; added `isPresetScrollActive()` and `consumePcDelta()` public API
- `Source/GamepadInput.cpp` - Added Option/Guide detection block after R3, replaced old D-pad block with mode-branched version using one-frame lockout

## Decisions Made

- `SDL_CONTROLLER_BUTTON_GUIDE` is the correct SDL2 constant for Option/PS/Guide across all controllers (SDL 2.0.0+, project uses 2.30.9)
- Falling-edge fire for PC delta guarantees exactly one PC per physical press — rising-edge would send immediately on press before intent is confirmed
- One-frame lockout avoids D-pad Up/Down registering as BPM delta in the same 16ms polling frame that Option fires

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `GamepadInput` exposes `isPresetScrollActive()` and `consumePcDelta()` — ready for Plan 02 (PluginProcessor wires these to send Program Change MIDI messages)
- All existing consume helpers (`consumeDpadUp`, `consumeDpadDown`, `consumeDpadLeft`, `consumeDpadRight`) still compile and function in normal mode

---
*Phase: 15-gamepad-preset-control*
*Completed: 2026-02-26*
