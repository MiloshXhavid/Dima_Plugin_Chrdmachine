---
phase: 04-per-voice-trigger-sources-and-random-gate
plan: 01
subsystem: ui
tags: [juce, midi, trigger, joystick, continuous-gate, vst3, cpp]

# Dependency graph
requires:
  - phase: 03-core-midi-output-and-note-off-guarantee
    provides: TriggerSystem with resetAllGates(), processBlockBypassed(), note-off guarantee

provides:
  - joystickThreshold APVTS param (0.001..0.1, default 0.015) controlling gate sensitivity
  - Continuous joystick gate model in TriggerSystem (Chebyshev magnitude, 50ms floor)
  - joystickStillSamples_[4] per-voice stillness counter, cleared in resetAllGates()
  - Horizontal THRESHOLD slider in PluginEditor right column with SliderParameterAttachment
  - TouchPlate dimming and mouseDown/mouseUp no-op guard in JOY and RND modes

affects:
  - 04-02 (random gate — shares TriggerSystem per-voice infrastructure)
  - 06-sdl2-gamepad (notifyJoystickMoved stub may be revisited)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Chebyshev distance (max|dx|, |dy|) for joystick motion magnitude — avoids diagonal bias
    - 50ms minimum gate via per-voice sample counter (joystickStillSamples_)
    - getRawParameterValue() load() from paint() message thread — safe for atomic<float>*

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Chebyshev distance (max|dx|, |dy|) for threshold comparison — avoids diagonal asymmetry vs Manhattan"
  - "50ms gate floor via joystickStillSamples_[v] += blockSize per block — simple, allocation-free"
  - "joystickTrig_ atomic kept (not removed) since resetAllGates() clears it; continuous gate logic bypasses it"
  - "THRESH label painted via PluginEditor::paint() direct g.drawText — no extra juce::Label member needed"
  - "TouchPlate dimming reads APVTS from paint() via getRawParameterValue()->load() — atomic, safe on message thread"

patterns-established:
  - "Per-voice stillness counter pattern: joystickStillSamples_[v] cleared on movement, accumulated on stillness"
  - "Mode-aware TouchPlate: check triggerSource param in paint()/mouseDown()/mouseUp() before acting"

requirements-completed: [TRIG-01, TRIG-02]

# Metrics
duration: 15min
completed: 2026-02-22
---

# Phase 4 Plan 1: Joystick Continuous Gate + Threshold Slider Summary

**Chebyshev-magnitude continuous gate model in TriggerSystem with 50ms note-off floor, APVTS joystickThreshold param, horizontal THRESHOLD slider in UI, and mode-aware TouchPlate dimming/disabling for JOY and RND modes**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-02-22T16:56:47Z
- **Completed:** 2026-02-22T17:12:00Z (paused at human-verify checkpoint)
- **Tasks:** 2/3 complete (Task 3 is human verification checkpoint — awaiting Reaper test)
- **Files modified:** 5

## Accomplishments

- Replaced one-shot delta-based joystick trigger with sustained continuous gate model (open on threshold-crossing, close after 50ms stillness)
- Added `joystickThreshold` APVTS parameter (0.001..0.1, default 0.015) and horizontal THRESHOLD slider in right column
- TouchPlate pads dim visually and become non-functional in JOY and RND trigger modes
- Build clean, 15/15 Catch2 tests pass after both task changes

## Task Commits

Each task was committed atomically:

1. **Task 1: Add joystickThreshold APVTS param + continuous gate model** - `afc4ba0` (feat)
2. **Task 2: Add threshold slider and TouchPlate dimming to PluginEditor** - `0a2e023` (feat)
3. **Task 3: Human verify in Reaper** - PENDING CHECKPOINT

**Plan metadata:** TBD (created after checkpoint approval)

## Files Created/Modified

- `Source/TriggerSystem.h` - Added `joystickThreshold` to ProcessParams; added `joystickStillSamples_[4]` private member
- `Source/TriggerSystem.cpp` - Replaced delta-based joystick logic with Chebyshev magnitude gate; 50ms floor via sample counter; cleared in resetAllGates()
- `Source/PluginProcessor.cpp` - Added `joystickThreshold` ParamID and APVTS float param; assigned to `tp.joystickThreshold` in processBlock
- `Source/PluginEditor.h` - Added `thresholdSlider_` and `thresholdSliderAtt_` members
- `Source/PluginEditor.cpp` - Constructor: slider init + APVTS attachment; resized(): threshold row below filter atten; paint(): THRESH label; TouchPlate: mode-aware paint()/mouseDown()/mouseUp()

## Decisions Made

- Chebyshev distance (max|dx|, |dy|) over Manhattan — avoids diagonal bias where combined X+Y movement double-triggers
- 50ms floor implemented via `joystickStillSamples_[v] += p.blockSize` per processBlock call — simple, no heap allocation
- `joystickTrig_` atomic kept (per plan note) since `resetAllGates()` still clears it; the new gate model doesn't use it
- THRESH label drawn directly in `paint()` rather than adding a `juce::Label` member — keeps header simpler

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - both builds were clean on first attempt, all 15 tests passing after Task 1.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Task 3 (Reaper verification) pending human approval: load ChordJoystick.vst3 in Reaper, test pad dimming, JOY gate behavior, and 50ms minimum gate floor
- After approval: Phase 04 Plan 02 (random gate per-voice infrastructure)
- Known blocker: Ableton crash unresolved (Reaper used for verification)

---
*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Completed: 2026-02-22*
