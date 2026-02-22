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
  - Wall-clock sample-count fallback when DAW transport stopped (superseded — see deviations)
  - randomDensity 1..8 hits-per-bar model with hitsPerBarToProbability() conversion
  - randomGateTime fraction-of-subdivision note duration with 10ms floor
  - randomSubdiv0..3 APVTS AudioParameterChoice params (replacing single randomSubdiv)
  - randomGateTime APVTS AudioParameterFloat param (0..1, default 0.5)
  - randomClockSync APVTS AudioParameterBool (default true): sync vs free mode
  - randomFreeTempo APVTS AudioParameterFloat (30..240 BPM, default 120): free-running BPM
  - 4 per-voice randomSubdivBox_[4] combo boxes column-aligned with voice trigger source combos
  - randomGateTimeKnob_ Rotary slider + randomDensityKnob_ as NoTextBox rotaries
  - DENS / GATE / FREE BPM / SYNC 4-column row in PluginEditor random controls section
  - wasPlaying_ transport-restart edge detection resets prevSubdivIndex_ to -1

affects:
  - 04-03
  - Phase 05 looper hardening (LooperEngine interacts with trigger gate state)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - PPQ subdivision index comparison for DAW-synced random gate (no timer, no accumulator when playing)
    - Sample-accumulator fallback clock only in free mode (not transport-stopped — wall-clock removed)
    - hitsPerBarToProbability(): density / subdivsPerBar clamped 0..1 for per-event probability
    - randomGateRemaining_[v] countdown: decrements each block, fires noteOff at 0, 10ms floor
    - wasPlaying_ edge detection for transport-restart subdivision reset
    - randomClockSync: bool param selects sync (DAW-only) vs free (internal BPM) mode

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
  - "randomDensity range changed 0..1 to 1..8 hits-per-bar (more musical, matches human expectation)"
  - "hitsPerBarToProbability() converts hits-per-bar to per-event probability: density / subdivsPerBar"
  - "randomGateTime x samplesPerSubdiv with 10ms floor prevents inaudibly-short notes at extreme settings"
  - "wasPlaying_ edge detection resets prevSubdivIndex_[4] to -1 on transport restart (no spurious trigger)"
  - "randomClockSync param (bool, default true): sync mode fires only when isDawPlaying; free mode uses internal BPM accumulator"
  - "randomFreeTempo param (float, 30-240, default 120): BPM for free-running random clock — wall-clock fallback removed from sync mode"
  - "DAW stop = complete silence from RND voices in sync mode — wall-clock fallback intentionally removed (cleaner musical behavior)"
  - "4-column random controls row: DENS / GATE / FREE BPM / SYNC aligned under per-voice subdiv combos"
  - "randomDensityKnob_ changed to NoTextBox rotary to match Gate/FreeTempo knob size (visual consistency)"

patterns-established:
  - "Per-voice state arrays (randomPhase_[4], prevSubdivIndex_[4], randomGateRemaining_[4]) for independent clock"
  - "APVTS per-voice naming: randomSubdiv0..3 for 4-voice array params"
  - "ComboBoxParameterAttachment array std::array<std::unique_ptr<ComboBoxParameterAttachment>, 4>"
  - "Sync/free mode toggle pattern: bool param selects clock source; free BPM param only active in free mode"

requirements-completed:
  - TRIG-03
  - TRIG-04

# Metrics
duration: ~90min
completed: 2026-02-22
---

# Phase 04 Plan 02: Per-Voice Random Gate Summary

**ppqPosition-synced per-voice random subdivision clock with hits-per-bar density, gate-time auto note-off, sync/free mode toggle, and 4-column UI layout — replacing the global free-running random accumulator**

## Performance

- **Duration:** ~90 min
- **Started:** 2026-02-22
- **Completed:** 2026-02-22 (all tasks done including Task 3 human DAW verification — APPROVED)
- **Tasks:** 3/3 complete (Tasks 1, 2, 2a-2d amendments, Task 3 human-verify)
- **Files modified:** 5

## Accomplishments

- ppqPosition subdivision clock per voice: fires when int64_t subdivision index changes — exact beat alignment, no float drift
- randomDensity model changed from raw probability 0..1 to hits-per-bar 1..8 (hitsPerBarToProbability converts)
- randomGateTime knob: fraction of subdivision for note duration, 10ms floor prevents inaudible staccato
- 4 per-voice randomSubdiv0..3 APVTS params + 4 per-voice UI combo boxes column-aligned under trigger source combos
- wasPlaying_ edge detection: prevSubdivIndex_[4] reset to -1 on transport restart (no spurious fire on play)
- Ghost note-off prevention: randomGateRemaining_[v] cleared when voice exits RND mode mid-phrase
- randomClockSync param (bool, default true): sync mode fires only when isDawPlaying; free mode uses internal BPM accumulator at randomFreeTempo BPM
- randomFreeTempo param (30-240 BPM, default 120): controls free-running clock rate independent of DAW
- Wall-clock fallback removed from sync mode: DAW stop = complete silence from RND voices (intentional — cleaner musical behavior)
- 4-column random controls row (DENS / GATE / FREE BPM / SYNC) with consistent NoTextBox rotary knob sizes
- Human checkpoint approved in Reaper — all 5 verification tests passed

## Task Commits

Each task was committed atomically:

1. **Task 1: Per-voice random clock + density model + APVTS params** - `2b256a9`
2. **Task 2: Per-voice subdiv combo boxes and randomGateTime knob in PluginEditor** - `448556c`
3. **Task 2a: Random clock sync/free mode — APVTS params + TriggerSystem logic** - `30f52d4`
4. **Task 2b: Sync toggle + free tempo knob in PluginEditor** - `154bdce`
5. **Task 2c: Align random controls — 4-column row under subdiv combos, all knobs same size** - `43f0724`
6. **Task 2d: Fix gap above random knobs row so DENS/GATE/FREE BPM labels are visible** - `65f069c`
7. **Task 3: Human checkpoint — Reaper DAW verification** - APPROVED (no code commit)

## Files Created/Modified

- `Source/TriggerSystem.h` - ProcessParams: randomSubdiv[4], randomDensity (1..8), randomGateTime, ppqPosition, isDawPlaying, randomClockSync, randomFreeTempo; private: randomPhase_[4], prevSubdivIndex_[4], randomGateRemaining_[4], wasPlaying_
- `Source/TriggerSystem.cpp` - subdivBeatsFor() lambda, hitsPerBarToProbability(), per-voice ppq clock, free-mode BPM accumulator, randomGateRemaining_ countdown, transport restart detection, sync/free branch, resetAllGates() updated
- `Source/PluginProcessor.cpp` - randomSubdiv0..3 APVTS params, randomGateTime param, randomDensity range 1..8, randomClockSync param, randomFreeTempo param, processBlock populates tp.ppqPosition, tp.isDawPlaying, tp.randomGateTime, tp.randomClockSync, tp.randomFreeTempo, per-voice subdivIdx loop
- `Source/PluginEditor.h` - randomSubdivBox_[4] array, randomSubdivAtt_[4] array, randomGateTimeKnob_, randomGateTimeKnobAtt_, syncToggle_, syncToggleAtt_, freeTempKnob_, freeTempKnobAtt_; removed old single randomSubdivAtt_
- `Source/PluginEditor.cpp` - 4 per-voice combo boxes with randomSubdiv0..3 param attachments; randomGateTimeKnob_ rotary; syncToggle_ ToggleButton; freeTempKnob_ rotary; 4-column DENS/GATE/FREE BPM/SYNC row in resized(); DENS/GATE/FREE BPM labels in paint()

## Decisions Made

- Used `int64_t idx = static_cast<int64_t>(p.ppqPosition / beats)` for subdivision index — integer comparison avoids float equality issues and fires exactly once per boundary crossing
- randomDensity 1..8 range: more intuitive than 0..1 probability — "4 hits per bar" communicates directly
- randomGateRemaining_ countdown decrements every block (not per-sample) — block-granular is fine for musical gate timing
- prevSubdivIndex_ resets to -1 on transport restart via wasPlaying_ edge: avoids firing on the same subdivision that was already seen before stop
- randomClockSync=true (sync mode): fires only when isDawPlaying — DAW stop means complete silence; no wall-clock fallback in this mode. This is intentional: free-running triggers while DAW is paused would pollute recordings.
- randomClockSync=false (free mode): uses BPM accumulator at randomFreeTempo; fires regardless of DAW transport state
- Wall-clock fallback from original plan spec removed: replaced with explicit sync/free mode toggle controlled by user

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Feature Addition] randomClockSync + randomFreeTempo params added post-Task 2**

- **Found during:** Task 2 post-implementation review
- **Issue:** Original plan spec included wall-clock fallback when transport stopped (fires at free-running rate when DAW paused). This behavior was found to be musically problematic: random notes would play into recordings whenever the user stopped the DAW to adjust settings.
- **Fix:** Replaced wall-clock fallback with explicit sync/free mode. Added `randomClockSync` APVTS bool (default true = sync) and `randomFreeTempo` APVTS float (30-240 BPM, default 120). Sync mode is silent when DAW stopped. Free mode fires at randomFreeTempo BPM regardless of transport.
- **Files modified:** Source/TriggerSystem.h, Source/TriggerSystem.cpp, Source/PluginProcessor.cpp, Source/PluginEditor.h, Source/PluginEditor.cpp
- **Commits:** 30f52d4, 154bdce, 43f0724, 65f069c

**2. [Rule 2 - UI Consistency] randomDensityKnob_ changed to NoTextBox rotary**

- **Found during:** Task 2c layout alignment
- **Issue:** randomDensityKnob_ was a larger rotary with text box while Gate/FreeTempo knobs were NoTextBox rotaries — visual inconsistency in the 4-column row.
- **Fix:** Changed randomDensityKnob_ to NoTextBox rotary style, matching the other three knobs in the row.
- **Files modified:** Source/PluginEditor.cpp
- **Commit:** 43f0724

## Issues Encountered

None blocking. Build clean on first attempt for Tasks 1 and 2. Tasks 2a-2d were amendments to add the sync/free model after review of the original wall-clock fallback design.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- All tasks complete, human checkpoint approved in Reaper.
- TRIG-03 and TRIG-04 requirements marked complete.
- Plan 04-03 (if any) can begin immediately.
- No blockers.

---
*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Completed: 2026-02-22 (Tasks 1+2+2a-2d+Task 3 human checkpoint APPROVED)*
