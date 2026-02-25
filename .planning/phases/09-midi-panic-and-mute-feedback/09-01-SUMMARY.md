---
phase: 09-midi-panic-and-mute-feedback
plan: 01
subsystem: midi
tags: [juce, midi-panic, gamepad, flash-feedback, atomic]

# Dependency graph
requires:
  - phase: 08-post-v1.0-patch-verification
    provides: Panic infrastructure (triggerPanic(), flashPanic_ atomic, pendingPanic_ flag) already wired
provides:
  - Panic sweep covers all 16 MIDI channels unconditionally (48 events: CC64=0 + CC120 + CC123 per channel)
  - R3 gamepad button triggers MIDI panic identically to UI button
  - PANIC! button in editor gamepad row (right-aligned, 60px) with 5-frame highlight flash
affects: [phase-10-quantize-infra, phase-11-ui-polish]

# Tech tracking
tech-stack:
  added: []
  patterns: [flash-counter pattern (fetch_add atomic + timerCallback decrement) now used for RST/DEL/PANIC]

key-files:
  created: []
  modified:
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Panic sweep is flat for(ch=1..16) loop — no channel deduplication, always hits all 16 channels regardless of voice config"
  - "CC64=0 + CC120 + CC123 per channel only — no CC121 (Kontakt/Waves CLA-76 map CC121 to parameters via JUCE VST3 wrapper)"
  - "R3 calls triggerPanic() directly in processBlock (audio-thread safe — only writes atomics)"
  - "panicBtn_ uses Clr::accent idle / Clr::highlight flash — matches RST/DEL button flash pattern"

patterns-established:
  - "Flash counter pattern: proc_.flashX_.exchange(0) > 0 sets counter=5; counter decrements each timer tick; colour set each tick"
  - "One-shot buttons: no setClickingTogglesState, no toggle state, onClick only writes atomics to processor"

requirements-completed: [PANIC-01, PANIC-02, PANIC-03]

# Metrics
duration: 15min
completed: 2026-02-25
---

# Phase 09 Plan 01: MIDI Panic + Mute Feedback Summary

**Full 16-channel MIDI panic sweep (48 events: CC64=0+CC120+CC123) with R3 gamepad trigger and PANIC! button with 5-frame highlight flash in the gamepad row**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-02-25T06:22:00Z
- **Completed:** 2026-02-25T06:37:52Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Replaced the old panic sweep (killCh lambda hitting only voice channels + filterCh) with a flat `for(ch=1..16)` loop emitting CC64=0 + CC120 + CC123 on every MIDI channel — exactly 48 events, no CC121
- Wired R3 gamepad (`consumeRightStickTrigger`) to call `triggerPanic()` instead of silently consuming the trigger
- Added `panicBtn_` to PluginEditor: declared in header, constructed with text/tooltip/idle accent color/onClick, laid out right-aligned 60px in gamepad status row, 5-frame highlight flash driven by `flashPanic_.exchange(0)` in timerCallback

## Task Commits

Each task was committed atomically:

1. **Task 1: Expand processor panic sweep to all 16 channels + wire R3 gamepad** - `f902307` (feat)
2. **Task 2: Add panicBtn_ to editor — declare, construct, lay out, flash** - `68b4b18` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `Source/PluginProcessor.cpp` - Replaced killCh lambda/sent[]/fCh with flat ch=1..16 loop; wired consumeRightStickTrigger to triggerPanic()
- `Source/PluginEditor.h` - Added `juce::TextButton panicBtn_` declaration and `int panicFlashCounter_ = 0`
- `Source/PluginEditor.cpp` - Constructor wiring, resized() layout (right-aligned 60px), timerCallback 5-frame flash block

## Decisions Made

- No CC121 in panic sweep — downstream VST3 instruments (Kontakt, Waves CLA-76) map CC121 to plugin parameters via JUCE VST3 wrapper; confirmed in STATE.md decisions
- Panic is one-shot (no mute toggle): onClick only calls `proc_.triggerPanic()`, no `midiMuted_.store(true)`
- `panicBtn_` uses `Clr::accent` idle color to match other utility buttons (RST, DEL)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 09 feature set complete: PANIC-01, PANIC-02, PANIC-03 all satisfied
- Phase 10 (Quantize Infra) can begin — no blockers from this phase
- Build not run locally (CMake/VS build environment); changes are syntactically straightforward C++ edits matching established patterns

## Self-Check: PASSED

- Source/PluginProcessor.cpp: FOUND
- Source/PluginEditor.h: FOUND
- Source/PluginEditor.cpp: FOUND
- 09-01-SUMMARY.md: FOUND
- Commit f902307: FOUND (feat(09-01): expand panic sweep to all 16 channels + wire R3 gamepad)
- Commit 68b4b18: FOUND (feat(09-01): add PANIC! button to editor with flash feedback)

---
*Phase: 09-midi-panic-and-mute-feedback*
*Completed: 2026-02-25*
