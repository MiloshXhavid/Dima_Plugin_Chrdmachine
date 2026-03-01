---
phase: 20-random-trigger-system-extensions
plan: 02
subsystem: audio-engine
tags: [trigger-system, apvts, random-trigger, midi, gate-logic, cpp, juce]

# Dependency graph
requires:
  - phase: 20-01-random-trigger-system-extensions
    provides: TriggerSystem.h/cpp backend with RandomFree/RandomHold enums, randomPopulation/randomProbability/gateLength in ProcessParams

provides:
  - APVTS layout registers randomPopulation (1–64), randomProbability (0.0–1.0), gateLength (0.0–1.0)
  - trigSrcNames has 4 items: TouchPlate, Joystick, Random Free, Random Hold (indices 0-3)
  - subdivChoices has 5 items: 1/4, 1/8, 1/16, 1/32, 1/64 (index 4 added)
  - ProcessParams forwarding: tp.randomPopulation, tp.randomProbability, tp.gateLength from APVTS
  - Arp gateRatio reads gateLength (0.0–1.0) directly — no /100 conversion, no lower clamp
  - tp.joystickGateTime = 1.0f constant (arpGateTime param removed)

affects:
  - 20-03 (PluginEditor must attach Probability knob to "randomProbability", Population to "randomPopulation", gateLength to "gateLength"; triggerSource combo must show 4 choices; subdivChoices must show 5 items)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Unified gateLength param (0.0–1.0) serves both Arp and Random gate-time: single APVTS param, two consumers"
    - "Manual gate (gateRatio == 0.0): gateBeats = 0.0, arpNoteOffRemaining_ stays 0 — step boundary is the implicit note-off"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.cpp

key-decisions:
  - "arpGateTime param (5–100%) removed; unified gateLength (0.0–1.0) replaces it — no /100 conversion needed, gateRatio reads directly"
  - "randomDensity and randomGateTime params removed; randomPopulation (1–64) and randomProbability (0–1) replace them with semantically clearer names"
  - "tp.joystickGateTime hardcoded to 1.0f — arpGateTime was the only source and is gone; dedicated param deferred to future milestone"
  - "trigSrcNames index 2 label changed from 'Random' to 'Random Free'; index 3 'Random Hold' appended — integer values unchanged for DAW session compat"

patterns-established:
  - "Unified gate param pattern: register once in Trigger section, consume in both TriggerSystem forwarding AND arp gateRatio read"

requirements-completed: [RND-01, RND-02, RND-03, RND-04, RND-05, RND-06, RND-07]

# Metrics
duration: 5min
completed: 2026-03-01
---

# Phase 20 Plan 02: Random Trigger System Extensions — PluginProcessor APVTS Bridge Summary

**APVTS param IDs migrated to randomPopulation/randomProbability/gateLength, trigSrcNames expanded to 4, subdivChoices to 5, arp gateRatio unified with random gate — build clean at 0 errors**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-01T01:48:59Z
- **Completed:** 2026-03-01T01:54:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Removed `randomDensity`, `randomGateTime`, and `arpGateTime` from both ParamID namespace and createParameterLayout() — no old IDs remain in live code
- Added `randomPopulation` (1–64, default 8), `randomProbability` (0–1, default 1), `gateLength` (0–1, default 0.5) to APVTS and forwarded into ProcessParams
- trigSrcNames now has 4 items (TouchPlate / Joystick / Random Free / Random Hold) and subdivChoices has 5 items (1/4 through 1/64)
- Arp gateRatio reads `gateLength` directly (no /100 conversion, no jlimit lower clamp) — manual gate (0.0) is now valid

## Task Commits

Each task was committed atomically:

1. **Task 1: Update APVTS parameter layout — rename/add params, expand choice lists** - `bc9eeeb` (feat)
2. **Task 2: Forward new params into ProcessParams and fix downstream references** - `9b71002` (feat)

**Plan metadata:** _(docs commit pending)_

## Files Created/Modified

- `Source/PluginProcessor.cpp` — ParamID namespace updated, createParameterLayout() Trigger section rewritten, arpGateTime removed, arp gateRatio unified, processBlock ProcessParams forwarding updated

## Decisions Made

- **arpGateTime removed entirely** — The param was registered as 5–100% and required a /100 division. The unified gateLength (0.0–1.0) is semantically cleaner and serves both the Arp and Random gate-time paths without conversion. The old value of 75% maps to gateLength=0.75 (preset migration note for future).
- **tp.joystickGateTime = 1.0f constant** — joystickGateTime controls Joystick-mode stillness timeout (in seconds), not gate time. It was incorrectly wired to arpGateTime. Since arpGateTime is gone and no dedicated param exists, 1.0s constant matches the ProcessParams default. A dedicated param can be added in a future phase.
- **Random Free / Random Hold label vs. value** — Only the string label changes (index 2: "Random" → "Random Free"); the integer value 2 is unchanged. Existing DAW sessions storing triggerSource=2 will load RandomFree correctly without migration.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - all changes were mechanical substitutions with no surprises. Task 1 produced the expected downstream build errors (old field references in processBlock) which were immediately resolved by Task 2.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `PluginProcessor.cpp` is fully consistent with `TriggerSystem.h` — all new ProcessParams fields forwarded correctly
- Plan 20-03 (PluginEditor UI) can now proceed: attach UI controls to "randomPopulation", "randomProbability", "gateLength" APVTS IDs; update triggerSource combo to show 4 choices; update subdiv combo to show 5 choices

---
*Phase: 20-random-trigger-system-extensions*
*Completed: 2026-03-01*
