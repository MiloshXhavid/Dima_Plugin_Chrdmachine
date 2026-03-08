---
phase: 45-arpeggiator-step-patterns-ui-redesign-lfo-heart-preset
plan: 01
subsystem: arp
tags: [apvts, arpeggiator, step-pattern, trigger-system, migration]

# Dependency graph
requires:
  - phase: 44-38-3-sister-lfo-attenuator
    provides: stable APVTS registration pattern and TriggerSystem ProcessParams interface
provides:
  - 10 new APVTS params registered (arpStepState0..7, arpLength, randomSyncMode)
  - randomClockSync bool param removed; replaced by randomSyncMode Choice (FREE/INT/DAW)
  - Arp step loop with ON/TIE/OFF gating and arpLength modulo
  - arpCurrentStep_ atomic for future UI grid highlight
  - setStateInformation migration shim for old preset compat
affects: [45-02, TriggerSystem, PluginProcessor, PluginEditor arp step grid UI]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "APVTS Choice param for 3-state enum (FREE/INT/DAW) replaces bool — same pattern as filterXMode/Y"
    - "Preset migration shim: getChildByAttribute on XML in setStateInformation, removeChildElement + createNewChildElement"
    - "arpCurrentStep_ atomic<int> pattern mirrors arpCurrentVoice_ — audio thread writes, timer/UI reads"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.cpp
    - Source/PluginProcessor.h
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp

key-decisions:
  - "randomSyncMode==0 is FREE (Poisson/countdown timer), ==1 is INT (internal BPM grid), ==2 is DAW (ppq boundary) — maps old bool: false=FREE(0), true=INT(1)"
  - "arpLength APVTS choice index + 1 = actual length (index 0 = length 1, index 7 = length 8) — getRawParameterValue + 1 pattern"
  - "TIE with no prior active note falls through to ON path (treat as ON) — isTie guard requires arpActivePitch_ >= 0"
  - "OFF step: sends immediate noteOff if active pitch exists, then continues — distinct from step-boundary noteOff (which is skipped for TIE)"
  - "gamepad rndSyncToggle cycles 0→1→2→0 via AudioParameterChoice::convertTo0to1(float(next)) — same pattern as other choice cycling"

patterns-established:
  - "Step-state read BEFORE step-boundary noteOff: patStep = arpStep_ % arpLen computed first, then isTie guard wraps the noteOff block"
  - "Migration shim in setStateInformation: check old param presence, extract value, remove, create new PARAM child with mapped value"

requirements-completed: [ARP-01, ARP-02, ARP-03]

# Metrics
duration: 25min
completed: 2026-03-08
---

# Phase 45 Plan 01: Arp Step Patterns — Processor Foundation Summary

**10 new APVTS params (arpStepState0..7, arpLength, randomSyncMode) with ON/TIE/OFF step gating, arpLength modulo wrap, and randomClockSync-to-randomSyncMode migration shim**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-03-08T~19:35Z
- **Completed:** 2026-03-08T~20:00Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Registered 10 new APVTS parameters: arpStepState0..7 (ON/TIE/OFF Choice), arpLength (1-8 Choice, default 8), randomSyncMode (FREE/INT/DAW Choice, default FREE)
- Replaced randomClockSync bool param entirely — TriggerSystem.h, TriggerSystem.cpp (7 branch sites), and PluginProcessor.cpp all updated consistently
- Arp step loop now reads patStep = arpStep_ % arpLen before step-boundary noteOff; TIE suppresses noteOff and re-trigger; OFF fires immediate noteOff and skips note-on
- arpCurrentStep_ atomic declared in PluginProcessor.h for future Editor step-grid highlight
- setStateInformation migration shim converts randomClockSync=1 (old bool preset) to randomSyncMode=1 (INT) cleanly

## Task Commits

Each task was committed atomically:

1. **Task 1: Register 10 new APVTS params, remove randomClockSync bool** - `3518374` (feat)
2. **Task 2: Update TriggerSystem ProcessParams — replace bool with int** - `ac1d78c` (feat)
3. **Task 3: Arp step loop gating + arpLength + arpCurrentStep_ + migration shim** - `23086f3` (feat)

## Files Created/Modified

- `Source/PluginProcessor.cpp` - New param registrations, randomSyncMode fill site + gamepad toggle, arp step loop with state gating, migration shim
- `Source/PluginProcessor.h` - arpCurrentStep_ atomic<int> added after arpCurrentVoice_
- `Source/TriggerSystem.h` - randomClockSync bool replaced with randomSyncMode int in ProcessParams
- `Source/TriggerSystem.cpp` - All 7 randomClockSync branch sites updated to use randomSyncMode comparisons

## Decisions Made

- randomSyncMode==0 is FREE (Poisson/countdown timer), ==1 is INT (internal BPM grid), ==2 is DAW (ppq boundary). Maps old bool: false→FREE(0), true→INT(1)
- arpLength APVTS choice index + 1 = actual step count — getRawParameterValue returns 0..7, +1 gives 1..8
- TIE with no prior active note (arpActivePitch_ < 0) falls through to ON path — isTie guard requires both stepState==1 AND arpActivePitch_ >= 0
- OFF step sends immediate noteOff if active note exists, then continues — this is in addition to the already-skipped step-boundary noteOff (which was skipped because isTie=false and arpActivePitch_ was cleared)
- gamepad rndSyncToggle now cycles 0→1→2→0 via AudioParameterChoice::convertTo0to1(float(next))

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Build succeeded cleanly on all three tasks. The bash shell path issue (`/c/Users/Milosh: Is a directory`) is a known environment quirk — does not affect compilation.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- All 10 APVTS params registered and building — Plan 02 (Editor step-grid UI) can now wire up to arpStepState0..7, arpLength, and randomSyncMode
- arpCurrentStep_ atomic available for step highlighting in the Editor timer callback
- randomSyncMode Choice param ready for RND SYNC button to show 3-state (FREE/INT/DAW)

---
*Phase: 45-arpeggiator-step-patterns-ui-redesign-lfo-heart-preset*
*Completed: 2026-03-08*
