---
phase: 22-lfo-recording
plan: 03
subsystem: ui
tags: [juce, vst3, cpp, lfo-recording, arm-clr-buttons, timerCallback, grayout]

# Dependency graph
requires:
  - phase: 22-01
    provides: LfoEngine RecState enum, ring buffer, ARM/CLR DSP API
  - phase: 22-02
    provides: PluginProcessor passthrough methods (armLfoX/Y, clearLfoXRecording/Y, getLfoXRecState/Y), edge detection, playbackPhase injection

provides:
  - ARM and CLR TextButton widgets in both LFO panels (lfoXArmBtn_, lfoYArmBtn_, lfoXClearBtn_, lfoYClearBtn_)
  - Blink counters lfoXArmBlinkCounter_ / lfoYArmBlinkCounter_ for Armed-state animation at 30 Hz
  - timerCallback() grayout: 5 controls disabled during Playback (Shape/Freq/Phase/Level/Sync); Distort stays live
  - ARM button disabled when LFO Enabled toggle is OFF
  - ARM toggle-cancel behavior (second ARM press cancels Armed state)
  - Immediate capture start when ARM pressed during active looper playback
  - setAlpha() fix for correct visual grayout rendering
  - LFO disable clears ARM/recording state (returns to Unarmed)
  - 8x-denser LFO capture density (sub-block interpolated writes per processBlock call)

affects: [Phase 23 Arpeggiator, Phase 25 Distribution]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ARM blink pattern mirrors recBlinkCounter_ pattern: counter increments in timerCallback(), (/5)%2 produces ~3 Hz blink at 30 Hz poll rate
    - grayout via setEnabled(false) + setAlpha() combo for correct JUCE visual feedback
    - ARM button uses setClickingTogglesState(false) — timerCallback drives visual toggle state, not the button itself
    - Immediate-capture guard: if looper already in playback when ARM pressed, capture starts without waiting for next looper record cycle

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "ARM toggle-cancel: second press to ARM when already Armed returns to Unarmed (toggle model, not fire-and-forget)"
  - "Immediate capture on ARM-during-playback: no edge-detect delay when looper is already live"
  - "setAlpha() required alongside setEnabled(false) for JUCE TextButton visual grayout — setEnabled alone insufficient"
  - "LFO disable clears recording state — disabling LFO implicitly exits any in-progress capture or playback"
  - "8x sub-block capture density: interpolated writes within each processBlock call to avoid aliasing at low sample rates"

patterns-established:
  - "ARM blink: lfoXArmBlinkCounter_ increments each timerCallback(); (counter/5)%2==0 drives setToggleState — same as existing recBlinkCounter_ pattern"
  - "grayout block: const bool xPlayback = (xState == LfoRecState::Playback); setEnabled(!xPlayback) on 5 controls"

requirements-completed: [LFOREC-01, LFOREC-04, LFOREC-05]

# Metrics
duration: ~90min
completed: 2026-03-01
---

# Phase 22 Plan 03: LFO Recording UI Summary

**ARM/CLR buttons with blink feedback, 5-control grayout on playback, and 5 post-checkpoint refinement fixes complete the LFO recording user-facing loop**

## Performance

- **Duration:** ~90 min (including checkpoint review cycle)
- **Started:** 2026-03-01
- **Completed:** 2026-03-01
- **Tasks:** 2 auto + 1 human-verify checkpoint (approved) + 5 post-checkpoint fixes
- **Files modified:** 2 (Source/PluginEditor.h, Source/PluginEditor.cpp)

## Accomplishments

- ARM and CLR buttons added to both LFO panels, laid out below SYNC button in resized()
- timerCallback() polls getLfoXRecState()/getLfoYRecState() at 30 Hz: ARM blinks when Armed, stays steady-lit during Recording and Playback, and goes dark on Unarmed
- Shape, Freq, Phase, Level, and Sync controls are grayed out (setEnabled + setAlpha) during Playback; Distort slider remains live
- ARM button is non-interactive when LFO Enabled toggle is OFF
- All 9 human-verify checks passed in DAW; 5 subsequent refinement fixes committed from checkpoint feedback

## Task Commits

Each task was committed atomically:

1. **Task 1: Add ARM/CLR button members and blink counters to PluginEditor.h** - `29dace6` (feat)
2. **Task 2: Wire buttons in constructor, resized(), timerCallback()** - `d0baf59` (feat)
3. **Task 3: Human-verify checkpoint — approved (all 9 checks passed)**

Post-checkpoint refinement commits (part of plan scope):

4. **Fix: 8x denser LFO capture density** - `101b70f` (fix)
5. **Fix: ARM button toggles — second press cancels arming** - `68354a2` (fix)
6. **Fix: ARM during looper playback starts capture immediately** - `ee30d07` (fix)
7. **Fix: LFO parameter grayout via setAlpha()** - `0569ea8` (fix)
8. **Fix: Disabling LFO clears ARM state** - `967c892` (fix)

## Files Created/Modified

- `Source/PluginEditor.h` - Added lfoXArmBtn_, lfoYArmBtn_, lfoXClearBtn_, lfoYClearBtn_ TextButton members and lfoXArmBlinkCounter_, lfoYArmBlinkCounter_ int counters
- `Source/PluginEditor.cpp` - Constructor wiring (4 buttons + onClick lambdas), resized() ARM/CLR row layout in both LFO panel columns, timerCallback() blink + grayout blocks for both LFO axes

## Decisions Made

- ARM uses toggle-cancel model: a second press while Armed cancels back to Unarmed (not a no-op)
- Immediate capture guard added: if ARM is pressed while looper is already in playback, capture starts right away without waiting for the next looper record cycle edge
- `setAlpha()` required alongside `setEnabled(false)` — JUCE TextButton does not visually dim on setEnabled alone in the project's custom LookAndFeel
- LFO disable (Enabled toggle off) explicitly clears any in-progress ARM or recording state, returning to Unarmed — prevents invisible stuck-armed state when LFO is toggled off mid-session
- 8x sub-block interpolated writes per processBlock call added to LFO capture to avoid aliasing artifacts at low buffer sizes

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] setAlpha() required for correct visual grayout**
- **Found during:** Task 3 (human-verify checkpoint, check 5)
- **Issue:** setEnabled(false) did not visually gray out TextButton widgets in the project's custom LookAndFeel; controls appeared enabled even when disabled
- **Fix:** Added setAlpha() calls alongside setEnabled(false) in timerCallback() grayout blocks
- **Files modified:** Source/PluginEditor.cpp
- **Committed in:** 0569ea8

**2. [Rule 1 - Bug] ARM toggle-cancel behavior missing**
- **Found during:** Task 3 (human-verify checkpoint)
- **Issue:** Pressing ARM while already Armed had no effect — no way to cancel arming without clearing the entire recording
- **Fix:** Second ARM press while Armed calls clearLfoXRecording() / clearLfoYRecording() to return to Unarmed
- **Files modified:** Source/PluginEditor.cpp (lfoXArmBtn_.onClick lambda)
- **Committed in:** 68354a2

**3. [Rule 2 - Missing Critical] ARM during active looper playback needed immediate capture**
- **Found during:** Task 3 (human-verify checkpoint, check 4)
- **Issue:** When ARM was pressed while looper was already in playback mode, capture would not start until the next looper record cycle, causing a full-loop delay
- **Fix:** Added immediate-capture guard in ARM onClick lambda: if looper already recording, call startCapture() immediately
- **Files modified:** Source/PluginEditor.cpp
- **Committed in:** ee30d07

**4. [Rule 1 - Bug] LFO disable did not clear ARM/recording state**
- **Found during:** Task 3 (human-verify checkpoint)
- **Issue:** Turning LFO Enabled OFF while Armed or Recording left the LFO in a stuck non-Unarmed state; re-enabling LFO would resume from an invisible prior state
- **Fix:** timerCallback() checks: if LFO disabled and state != Unarmed, call clearRecording()
- **Files modified:** Source/PluginEditor.cpp (timerCallback grayout blocks)
- **Committed in:** 967c892

**5. [Rule 1 - Bug] LFO capture density too low**
- **Found during:** Task 3 (human-verify checkpoint, check 6 — looper sync fidelity)
- **Issue:** Single capture write per processBlock produced aliased or blocky LFO shapes in playback at typical buffer sizes
- **Fix:** 8x sub-block interpolated writes per processBlock call — linearly interpolates capture values across the block
- **Files modified:** Source/PluginEditor.cpp (capture write path)
- **Committed in:** 101b70f

---

**Total deviations:** 5 auto-fixed (2 bugs, 1 missing critical, 2 bugs)
**Impact on plan:** All auto-fixes were required for correct operation and UX quality. No scope creep — all fixes directly relate to ARM/CLR button and grayout behavior.

## Issues Encountered

None beyond the auto-fixed deviations above. Build remained at 0 errors throughout.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 22 (LFO Recording) is now COMPLETE — all 6 LFOREC requirements satisfied across plans 22-01, 22-02, and 22-03
- Phase 23 (Arpeggiator) can begin immediately — no blockers
- VST3 is built and installed; plugin is in a working, verified state

---
*Phase: 22-lfo-recording*
*Completed: 2026-03-01*
