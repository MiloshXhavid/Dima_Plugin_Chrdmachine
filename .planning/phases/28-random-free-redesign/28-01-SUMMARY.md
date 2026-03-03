---
phase: 28-random-free-redesign
plan: 01
subsystem: audio
tags: [cpp, juce, midi, trigger, random, poisson, probability, population, subdivision-pool]

# Dependency graph
requires:
  - phase: 27-triplet-subdivisions
    provides: RandomSubdiv enum with triplet values (WholeT..ThirtySecondT) used in subdivBeatsFor()
provides:
  - Probability-driven Poisson rate for RND SYNC OFF (0=silence, 1=64 events/bar)
  - Population as upward modulator (SYNC OFF) and subdivision pool expander (SYNC ON)
  - activeSubdiv_[4] per-voice active subdivision tracking
  - Three-branch sync matrix: Poisson / DAW-ppq / internal-free-tempo (unchanged)
affects: [29-looper-perimeter-bar, 30-distribution]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Probability-driven Poisson: effProb = min(1.0, prob * (1 + rand * normPop)); mean = spBar / (effProb * 64)
    - Subdivision pool expansion (SYNC ON): radius = floor(normPop * 7); pool = [selIdx-radius .. selIdx+radius]
    - activeSubdiv_[v] tracks per-voice active subdivision; updated on each Poisson fire (SYNC OFF) or pool pick (SYNC ON)
    - Three-branch clock: !randomClockSync (Poisson) / isDawPlaying+ppq>=0 (grid) / else (internal counter)

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp

key-decisions:
  - "randomProbability (0-1.0) drives Poisson rate: effProb * 64 events/bar — 0=silence, 1=maximum density"
  - "randomPopulation (1-64) is upward-only modulator (SYNC OFF) or subdivision pool radius (SYNC ON)"
  - "Burst mechanics removed entirely — each trigger event fires exactly 1 note"
  - "SYNC ON pool expands both directions from UI-selected index; radius = floor(normPop * 7) in 0..7"
  - "hitsPerBarToProbability() helper removed as it became unused after branch rewrites"

requirements-completed: [RND-08, RND-09, RND-10]

# Metrics
duration: 15min
completed: 2026-03-03
---

# Phase 28 Plan 01: Random Free Redesign Summary (Revised)

**Probability drives Poisson rate (0=silence to 64 events/bar); population acts as upward random modulator (SYNC OFF) or subdivision pool expander (SYNC ON); burst mechanics removed; 1 note per trigger event**

## Note

This SUMMARY supersedes the previous 28-01-SUMMARY written by the initial plan execution. The behaviour described in the original summary (burst drain, population-as-lambda) was incorrect per the design spec. This redesign was executed 2026-03-03 per user direction.

## Performance

- **Duration:** ~15 min
- **Completed:** 2026-03-03
- **Tasks:** 1
- **Files modified:** 2
- **Commit:** 84ccc12

## What Was Fixed

### Previous behaviour (incorrect)
- `randomPopulation` was used as the Poisson lambda directly (events/bar = population)
- `randomProbability` was used as a burst size multiplier: N = round(probability * 64) notes per event
- Burst drain loop fired multiple notes per clock event via `burstNotesRemaining_` / `burstPhase_` arrays

### New behaviour (correct)

**SYNC OFF (Poisson free-tempo):**
- `randomProbability` (0.0-1.0) drives the Poisson rate: `effProb * 64` events/bar
- `randomPopulation` (1-64) is an upward-only random modulator applied at each draw:
  - `normPop = max(0, (population - 1) / 63)` — 0.0 at min (1), 1.0 at max (64)
  - `boost = nextRandom() * normPop`
  - `effProb = min(1.0, prob * (1 + boost))`
  - Population=1: no modulation (deterministic at prob rate)
  - Population=64: effProb varies randomly between prob and min(2*prob, 1.0)
- Each Poisson event fires exactly 1 note
- `activeSubdiv_[v]` kept in sync with the UI-selected subdivision on each fire

**SYNC ON (DAW ppq grid):**
- Each grid slot fires with probability = `randomProbability` (per-slot gate, 0=never, 1=always)
- `randomPopulation` expands the subdivision pool in both directions from the selected index:
  - `radius = floor(normPop * 7)` — 0 at pop=1, 7 at pop=64
  - Pool = `[max(0, selIdx - radius) .. min(14, selIdx + radius)]`
  - At each trigger: one subdivision picked randomly from pool; `activeSubdiv_[v]` updated; `prevSubdivIndex_[v]` reset to -1 to force fresh ppq index
  - Population=1 (normPop=0): radius=0, always use selected subdivision

## Structural Changes

**TriggerSystem.h:**
- Removed: `std::array<int, 4> burstNotesRemaining_` and `std::array<double, 4> burstPhase_`
- Added: `std::array<RandomSubdiv, 4> activeSubdiv_` (currently active subdivision per voice)

**TriggerSystem.cpp:**
- Transport section: removed `justStopped` variable and burst-drain block on transport stop
- Clock block: uses `activeSubdiv_[v]` for beats/samplesPerSubdiv; Poisson formula uses `effProb * 64` rate
- Mode-switch block: removed burst reset (only gate-close remains)
- RandomFree branch: full replacement — 1 trigger per event; SYNC OFF triggers unconditionally; SYNC ON gates on probability and updates subdivision pool
- RandomHold branch: full replacement — same 1-trigger semantics, pad-held guard preserved, same pool update logic
- Gate length calc (in `if (trigger)` block): uses `activeSubdiv_[v]` instead of `p.randomSubdiv[v]`
- `resetAllGates()`: removed burst field zeroing, added `activeSubdiv_[v] = RandomSubdiv::Quarter`
- Removed unused `hitsPerBarToProbability()` static helper function

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Cleanup] Removed unused hitsPerBarToProbability helper**
- **Found during:** Task 1, after rewriting all callers
- **Issue:** The static helper became unreferenced after the RandomFree and RandomHold branches were replaced. MSVC emits C4505 unused-function warning for unreferenced static functions.
- **Fix:** Deleted the function definition entirely
- **Files modified:** Source/TriggerSystem.cpp
- **Commit:** 84ccc12

## Self-Check: PASSED

- `Source/TriggerSystem.h` exists: FOUND
- `activeSubdiv_` declared in header: CONFIRMED
- `burstNotesRemaining_` absent from header: CONFIRMED
- `burstPhase_` absent from header: CONFIRMED
- `Source/TriggerSystem.cpp` exists: FOUND
- Clock Poisson branch uses `effProb * 64` rate: CONFIRMED (line 133-135)
- RandomFree fires 1 note per event (no drain loop): CONFIRMED
- RandomHold fires 1 note per event (no drain loop): CONFIRMED
- SYNC ON subdivision pool logic present (both-direction expansion): CONFIRMED
- Commit 84ccc12 exists: CONFIRMED
- Build: 0 errors, 0 new warnings; VST3 installed to Program Files/Common Files/VST3

---
*Phase: 28-random-free-redesign | Revised: 2026-03-03*
