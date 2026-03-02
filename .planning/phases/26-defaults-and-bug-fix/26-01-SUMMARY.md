---
phase: 26-defaults-and-bug-fix
plan: 01
subsystem: audio-engine
tags: [juce, vst3, apvts, midi, notecount, defaults, bug-fix]

# Dependency graph
requires:
  - phase: 25-distribution
    provides: stable v1.5 build baseline — PluginProcessor.cpp as shipped

provides:
  - APVTS default values corrected (fifthOctave=4, scalePreset=NaturalMinor)
  - noteCount_ reference-counting fix — else-clamp branches removed from all 13 note-off paths
  - Stuck-notes bug (BUG-03) eliminated from Single Channel mode

affects:
  - 27-triplet-subdivisions (builds on stable PluginProcessor.cpp base)
  - 29-looper-perimeter-bar (same base)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "APVTS defaults set exclusively in createParameterLayout() — never in constructor or prepareToPlay()"
    - "noteCount_ reference counting: decrement only inside guard (noteCount > 0 && --noteCount == 0); no else-clamp fallback"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.cpp

key-decisions:
  - "fifthOctave default changed from 3 to 4 so fresh plugin shows Fifth=4 matching the musical intent (Fifth above root)"
  - "scalePreset default changed from 0 (Major) to 1 (NaturalMinor) — more typical use-case starting point"
  - "All 13 else-noteCount_=0 defensive clamps removed; the guard condition already prevents double-decrement, so the else branch was silently zeroing the counter on legitimate shared-pitch scenarios, preventing note-off emission"

patterns-established:
  - "noteCount_ pattern: if (count > 0 && --count == 0) emit noteOff — no else branch anywhere in processBlock"
  - "APVTS default-only location: createParameterLayout() fourth argument — single source of truth"

requirements-completed: [DEF-01, DEF-02, DEF-03, DEF-04, BUG-03]

# Metrics
duration: ~20min
completed: 2026-03-02
---

# Phase 26 Plan 01: Defaults and Bug Fix Summary

**APVTS octave/scale defaults corrected and 13 else-noteCount_=0 stuck-notes clamp branches removed from PluginProcessor.cpp**

## Performance

- **Duration:** ~20 min
- **Started:** 2026-03-02
- **Completed:** 2026-03-02
- **Tasks:** 4 (3 auto + 1 human-verify)
- **Files modified:** 1

## Accomplishments

- Fifth Octave default corrected from 3 to 4 — fresh plugin now opens with a voiced fifth interval
- Scale Preset default changed from Major (0) to Natural Minor (1) — more musically useful starting point for most users
- 13 defensive `else noteCount_[...] = 0` clamp branches removed from all note-off paths in processBlock() — root cause of stuck notes in Single Channel mode when two voices shared the same MIDI pitch
- Build confirmed clean (0 errors) and human-verified in DAW: defaults correct, stuck-note bug resolved, v1.5 preset regression-free

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix APVTS defaults in createParameterLayout()** - `ffad2b8` (feat)
2. **Task 2: Remove all else noteCount_ = 0 clamp branches** - `2bdff1a` (fix)
3. **Task 3: Build and verify compilation** - (build-only, no commit)
4. **Task 4: Human verify checkpoint** - Approved

**Plan metadata:** (this commit)

## Files Created/Modified

- `Source/PluginProcessor.cpp` — Two default values changed in createParameterLayout(); 13 else-clamp branches removed from all noteCount_ note-off paths in processBlock()

## Decisions Made

- APVTS defaults set only in `createParameterLayout()` (fourth argument to addInt/addChoice). Setting them elsewhere would be overridden by `setStateInformation()` and break preset loading — this is the confirmed single source of truth.
- The `else noteCount_[...] = 0` branches were originally written as defensive clamps against underflow. They are incorrect: the `if (count > 0 && --count == 0)` guard already handles the underflow case. The else branch was silently zeroing the counter when another voice held the same note (Single Channel mode), preventing the note-off from ever being emitted.
- The `std::fill` at the channel-switch flush (~line 1070) was intentionally left untouched — it is a legitimate allNotesOff reset on routing change, not a clamp.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None — both edits applied cleanly, build succeeded first attempt, DAW verification passed.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- PluginProcessor.cpp is now at a stable, regression-free baseline
- Phase 27 (Triplet Subdivisions) can proceed: APVTS enum changes land on this stable base
- Phase 29 (Looper Perimeter Bar) can also proceed independently from this base

---
*Phase: 26-defaults-and-bug-fix*
*Completed: 2026-03-02*
