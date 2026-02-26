---
phase: 14-lfo-ui-and-beat-clock
plan: 01
subsystem: audio-processor
tags: [juce, vst3, atomic, beat-clock, lfo, cross-thread, processblock]

# Dependency graph
requires:
  - phase: 13-processblock-integration-and-apvts
    provides: LFO injection in processBlock — chordP.joystickX/Y post-LFO values available
  - phase: 12-lfo-engine-core
    provides: LfoEngine standalone engine (evaluateWaveform, process, reset)
provides:
  - beatOccurred_ atomic<bool> — audio thread writes true on each beat, UI timer reads+clears
  - modulatedJoyX_ atomic<float> — post-LFO joystick X position for JoystickPad visual tracking
  - modulatedJoyY_ atomic<float> — post-LFO joystick Y position for JoystickPad visual tracking
  - sampleCounter_ int64_t — free-tempo beat accumulator reset on transport events
  - prevBeatCount_ double — promoted from static local for transport-reset safety
affects:
  - 14-02 (PluginEditor timerCallback reads beatOccurred_/modulatedJoyX_/Y_ to drive UI)
  - 14-03 (JoystickPad LFO tracking dot uses modulatedJoyX_/Y_)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Cross-thread atomic pattern: audio thread writes with memory_order_relaxed, UI timer reads"
    - "Beat clock: floor-crossing detection on ppqPos (DAW) or sampleCounter-derived count (free tempo)"
    - "prevBeatCount_ promoted from static local to member for transport-reset capability"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "beatOccurred_ is a sticky flag (audio writes true, UI timer clears it) — not a counter — simplest correct pattern for 30Hz polling"
  - "prevBeatCount_ promoted to private member (vs static local) so transport stop/start can reset it cleanly"
  - "blockSize2 naming collision avoided by using existing const int blockSize = audio.getNumSamples() already declared at line 427"
  - "effectiveBpm_ (already stored each block) drives free-tempo beat detection — no additional BPM read needed"

patterns-established:
  - "Transport reset sites (prepareToPlay, justStopped, dawJustStarted) must reset all phase-tracking state together: lfo.reset(), sampleCounter_=0, prevBeatCount_=-1.0"

requirements-completed: [CLK-01, CLK-02]

# Metrics
duration: 15min
completed: 2026-02-26
---

# Phase 14 Plan 01: LFO UI Beat Clock — Processor Atomics Summary

**Three cross-thread atomics (beatOccurred_, modulatedJoyX_, modulatedJoyY_) wired into processBlock with DAW/free-tempo beat floor-crossing detection and post-LFO joystick position broadcasting**

## Performance

- **Duration:** 15 min
- **Started:** 2026-02-26T03:37:16Z
- **Completed:** 2026-02-26T03:52:00Z
- **Tasks:** 2 of 2
- **Files modified:** 2

## Accomplishments
- Added three public atomics to PluginProcessor.h readable from UI message thread at 30 Hz
- Added private sampleCounter_ (int64_t) and prevBeatCount_ (double) for beat detection state
- Implemented modulated joystick position stores immediately after LFO injection block in processBlock
- Implemented beat floor-crossing detection for both DAW-playing (ppqPos) and free-tempo (sampleCounter) modes
- Reset all transport-sensitive state (sampleCounter_, prevBeatCount_) in prepareToPlay, DAW stop, and DAW start
- Build: 0 errors; LfoEngine 15/15 Catch2 tests pass (2222 assertions)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add cross-thread atomics to PluginProcessor.h** - `b2dc1b1` (feat)
2. **Task 2: Beat detection and modulated position stores in processBlock** - `c262098` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified
- `Source/PluginProcessor.h` - Three new public atomics (beatOccurred_, modulatedJoyX_, modulatedJoyY_) + two private members (sampleCounter_, prevBeatCount_)
- `Source/PluginProcessor.cpp` - modulatedJoyX_/Y_ stores after LFO block, beat floor-crossing detection block, reset calls in prepareToPlay/justStopped/dawJustStarted

## Decisions Made
- **beatOccurred_ is a sticky bool flag** — audio writes true, UI timer reads and clears it. Simple and correct for 30 Hz polling without event queues.
- **prevBeatCount_ promoted to private member** (not static local) so transport stop/start can reset it. Static locals cannot be reset portably.
- **Used existing blockSize variable** (declared at line 427 as `const int blockSize = audio.getNumSamples()`). Plan template used `buffer.getNumSamples()` but the function parameter is `audio`, not `buffer` — auto-fixed during Rule 1 (bug fix).
- **effectiveBpm_ reused for free-tempo beat detection** — already stored each block from looper's effective BPM calculation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Wrong audio buffer variable name in beat detection block**
- **Found during:** Task 2 (beat detection implementation)
- **Issue:** Plan template used `buffer.getNumSamples()` but processBlock parameter is named `audio`, and `blockSize` is already declared as a local const at line 427
- **Fix:** Used existing `blockSize` variable instead of `buffer.getNumSamples()` — no new variable needed
- **Files modified:** Source/PluginProcessor.cpp
- **Verification:** Build succeeded 0 errors after fix
- **Committed in:** c262098 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 — variable name bug from plan template)
**Impact on plan:** Fix essential for compilation. No scope change.

## Issues Encountered
- First build attempt failed with C2065 ("buffer": undeclared identifier) and C2737 (blockSize2 const not initialized). Root cause: plan template used wrong variable names. Fixed inline by using the existing `blockSize` local variable. No additional fix attempts needed.
- 5 pre-existing LooperEngineTests failures confirmed unchanged — unrelated to this plan's changes.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Processor side of UI cross-thread contract is complete
- PluginEditor timerCallback can now read beatOccurred_ (clear after read), modulatedJoyX_, modulatedJoyY_ at 30 Hz
- Phase 14 Plan 02 (PluginEditor timerCallback + beat pulse dot + JoystickPad LFO tracking) can proceed immediately

---
*Phase: 14-lfo-ui-and-beat-clock*
*Completed: 2026-02-26*
