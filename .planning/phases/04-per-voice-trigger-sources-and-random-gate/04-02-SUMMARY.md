---
phase: 04-per-voice-trigger-sources-and-random-gate
plan: 02
subsystem: midi-trigger
tags: [juce, midi, trigger, random, ppq, daw-sync, vst3]

# Dependency graph
requires:
  - phase: 04-01
    provides: JOY retrigger model, joystickThreshold APVTS param, TriggerSystem::ProcessParams with deltaX/deltaY

provides:
  - Per-voice ppqPosition-synced random subdivision clock (4 independent clocks)
  - Wall-clock sample-count fallback when DAW transport stopped
  - randomDensity 1..8 hits-per-bar model with hitsPerBarToProbability() conversion
  - randomGateTime fraction-of-subdivision note duration with 10ms floor
  - randomSubdiv0..3 APVTS AudioParameterChoice params (replacing single randomSubdiv)
  - randomGateTime APVTS AudioParameterFloat param (0..1, default 0.5)
  - 4 per-voice randomSubdivBox_[4] combo boxes column-aligned with voice trigger source combos
  - randomGateTimeKnob_ Rotary slider in UI adjacent to density knob
  - wasPlaying_ transport-restart edge detection resets prevSubdivIndex_ to -1

affects:
  - 04-03
  - Phase 05 looper hardening (LooperEngine interacts with trigger gate state)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - PPQ subdivision index comparison for DAW-synced random gate (no timer, no accumulator when playing)
    - Sample-accumulator fallback clock for wall-clock rate when transport stopped
    - hitsPerBarToProbability(): density / subdivsPerBar clamped 0..1 for per-event probability
    - randomGateRemaining_[v] countdown: decrements each block, fires noteOff at 0, 10ms floor
    - wasPlaying_ edge detection for transport-restart subdivision reset

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Per-voice ppqPosition subdivision index (int64_t) for DAW sync — fires when index changes, no floating point accumulation"
  - "randomDensity range changed 0..1 → 1..8 hits-per-bar (more musical, matches human expectation)"
  - "hitsPerBarToProbability() converts hits-per-bar to per-event probability: density / subdivsPerBar"
  - "randomGateTime × samplesPerSubdiv with 10ms floor prevents inaudibly-short notes at extreme settings"
  - "wasPlaying_ edge detection resets prevSubdivIndex_[4] to -1 on transport restart (no spurious trigger)"
  - "Transport stopped → sample-count accumulator fallback keeps RND active without DAW (wall-clock rate)"
  - "Auto note-off countdown in per-voice loop, cleared when source != Random (prevents ghost note-off on mode switch)"

patterns-established:
  - "Per-voice state arrays (randomPhase_[4], prevSubdivIndex_[4], randomGateRemaining_[4]) for independent clock"
  - "APVTS per-voice naming: randomSubdiv0..3 for 4-voice array params"
  - "ComboBoxParameterAttachment array std::array<std::unique_ptr<ComboBoxParameterAttachment>, 4>"

requirements-completed:
  - TRIG-03
  - TRIG-04

# Metrics
duration: 25min
completed: 2026-02-22
---

# Phase 04 Plan 02: Per-Voice Random Gate Summary

**ppqPosition-synced per-voice random subdivision clock with hits-per-bar density and gate-time auto note-off, replacing the global free-running random accumulator**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-02-22T~14:30Z
- **Completed:** 2026-02-22 (Tasks 1+2 done; Task 3 checkpoint pending human DAW verification)
- **Tasks:** 2/3 complete (Task 3 = human-verify checkpoint)
- **Files modified:** 5

## Accomplishments

- ppqPosition subdivision clock per voice: fires when int64_t subdivision index changes — exact beat alignment, no float drift
- Wall-clock sample accumulator fallback when transport stopped: RND always active regardless of DAW state
- randomDensity model changed from raw probability 0..1 to hits-per-bar 1..8 (hitsPerBarToProbability converts)
- randomGateTime knob: fraction of subdivision for note duration, 10ms floor prevents inaudible staccato
- 4 per-voice randomSubdiv0..3 APVTS params + 4 per-voice UI combo boxes column-aligned under trigger source combos
- wasPlaying_ edge detection: prevSubdivIndex_[4] reset to -1 on transport restart (no spurious fire on play)
- Ghost note-off prevention: randomGateRemaining_[v] cleared when voice exits RND mode mid-phrase

## Task Commits

Each task was committed atomically:

1. **Task 1: Per-voice random clock + density model in TriggerSystem + APVTS param changes** - `2b256a9` (feat)
2. **Task 2: Update PluginEditor — per-voice subdiv combo boxes and randomGateTime knob** - `448556c` (feat)
3. **Task 3: Verify random gate behavior in Reaper** - PENDING human checkpoint

## Files Created/Modified

- `Source/TriggerSystem.h` - ProcessParams: randomSubdiv[4], randomDensity (1..8), randomGateTime, ppqPosition, isDawPlaying; private: randomPhase_[4], prevSubdivIndex_[4], randomGateRemaining_[4], wasPlaying_
- `Source/TriggerSystem.cpp` - subdivBeatsFor() lambda, hitsPerBarToProbability(), per-voice ppq clock, sample fallback, randomGateRemaining_ countdown, transport restart detection, resetAllGates() updated
- `Source/PluginProcessor.cpp` - randomSubdiv0..3 APVTS params, randomGateTime param, randomDensity range 1..8, processBlock populates tp.ppqPosition, tp.isDawPlaying, tp.randomGateTime, per-voice subdivIdx loop
- `Source/PluginEditor.h` - randomSubdivBox_[4] array, randomSubdivAtt_[4] array, randomGateTimeKnob_, randomGateTimeKnobAtt_; removed old single randomSubdivAtt_
- `Source/PluginEditor.cpp` - 4 per-voice combo boxes with randomSubdiv0..3 param attachments; randomGateTimeKnob_ rotary; resized() column-aligned subdiv combos + GATE label in paint()

## Decisions Made

- Used `int64_t idx = static_cast<int64_t>(p.ppqPosition / beats)` for subdivision index — integer comparison avoids float equality issues and fires exactly once per boundary crossing
- randomDensity 1..8 range: more intuitive than 0..1 probability — "4 hits per bar" communicates directly
- randomGateRemaining_ countdown decrements every block (not per-sample) — block-granular is fine for musical gate timing
- prevSubdivIndex_ resets to -1 on transport restart via wasPlaying_ edge: avoids firing on the same subdivision that was already seen before stop

## Deviations from Plan

None - plan executed exactly as written. The old `subdivisionBeats()` static function was removed as redundant (replaced by `subdivBeatsFor` lambda) — this is cleanup, not deviation.

## Issues Encountered

None. Build clean on first attempt for both tasks. 15/15 Catch2 tests pass throughout.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Tasks 1+2 complete and built. Awaiting human DAW verification (Task 3 checkpoint).
- After checkpoint approval: SUMMARY finalized, STATE.md updated with TRIG-03/TRIG-04 complete, plan 04-03 can begin.
- No blockers beyond the pending checkpoint.

---
*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Completed: 2026-02-22 (Tasks 1+2; Task 3 checkpoint pending)*
