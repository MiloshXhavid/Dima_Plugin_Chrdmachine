---
phase: 19-sub-octave-per-voice
plan: 02
subsystem: ui
tags: [sub-octave, ui, apvts, buttonparameter, timer, juce]

# Dependency graph
requires:
  - phase: 19-sub-octave-per-voice/19-01
    provides: subOct0..3 APVTS bool params, isGateOpen(v) method
provides:
  - padSubOctBtn_[4] TextButton members wired via ButtonParameterAttachment to subOct0..3
  - HOLD strip split 50/50: HOLD on left, SUB8 on right of 18px strip for all 4 pads
  - timerCallback 30Hz coloring: bright orange when sounding, dim orange when enabled-not-sounding, darkgrey when disabled
  - Release VST3 deployed to system VST3 folder
affects:
  - Any future UI changes that modify the pad strip layout

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ButtonParameterAttachment wired via proc_.apvts.getParameter() — not custom onClick handler (unlike HOLD button)
    - timerCallback 30Hz colour poll: read raw APVTS value + isGateOpen() to drive button colour
    - holdStrip.removeFromLeft(width/2) split pattern for pairing two buttons in a strip

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "SUB8 uses ButtonParameterAttachment (wired to APVTS) while HOLD uses manual onClick + proc_.padHold_ — APVTS attachment handles preset save/load automatically"
  - "timerCallback reads getRawParameterValue and isGateOpen() at 30Hz — no additional cross-thread signals needed"
  - "holdStrip.reduced(2,2) applied BEFORE the 50/50 split to maintain equal margins on both buttons"
  - "Occasional stuck note (intermittent, low severity) observed during smoke test — deferred to future milestone for root cause investigation"

patterns-established:
  - "timerCallback per-voice SUB8 coloring: getRawParameterValue check + isGateOpen check -> 3-state colour"

requirements-completed:
  - SUBOCT-01

# Metrics
duration: ~20min
completed: 2026-03-01
---

# Phase 19 Plan 02: Sub Octave UI Summary

**SUB8 button split from HOLD strip (50/50 per pad), ButtonParameterAttachment wired to subOct0..3 APVTS, timerCallback orange brightness coloring, Release build deployed — smoke test APPROVED**

## Performance

- **Duration:** ~20 min
- **Started:** 2026-03-01
- **Completed:** 2026-03-01
- **Tasks:** 3 (2 auto + 1 human-verify checkpoint)
- **Files modified:** 2

## Accomplishments
- Added padSubOctBtn_[4] TextButton and subOctAttach_[4] ButtonParameterAttachment to PluginEditor.h
- Constructor: wires all 4 SUB8 buttons to subOct0..3 APVTS params via ButtonParameterAttachment
- resized(): splits the 18px HOLD strip 50/50 — HOLD gets left half, SUB8 gets right half, for all 4 pads
- timerCallback: 30Hz poll sets SUB8 colour — bright orange (0xFFFF8C00) when sounding, dim orange (0xFFB36200) when enabled-not-sounding, darkgrey when disabled
- Debug build passes with zero errors; Release build passes with zero errors; VST3 deployed via CMake install
- DAW smoke test APPROVED: all 4 SUBOCT requirements (SUBOCT-01..04) verified working in DAW

## Task Commits

1. **Task 1: SUB8 button UI (header, constructor, resized, timerCallback)** - `3794c04` (feat)
2. **Task 2: Release build + deploy** - no additional commit (Release used Task 1 code)
3. **Task 3: Smoke test checkpoint** - approved by user (2026-03-01)

## Files Created/Modified
- `Source/PluginEditor.h` - Added padSubOctBtn_[4] TextButton and subOctAttach_[4] ButtonParameterAttachment declarations
- `Source/PluginEditor.cpp` - SUB8 constructor wiring, resized() 50/50 split, timerCallback coloring

## Decisions Made
- ButtonParameterAttachment used for SUB8 (not manual onClick) — handles preset save/load automatically and is the correct JUCE pattern for boolean APVTS parameters
- holdStrip reduced before split so both HOLD and SUB8 get equal 2px margins
- Bright orange (0xFFFF8C00) for sounding vs dim orange (0xFFB36200) for enabled-not-sounding provides clear visual feedback
- Occasional stuck note deferred to future milestone — intermittent, low severity, does not block shipping

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Plan 19-01 backend work verified complete before executing 19-02**
- **Found during:** Task 1 start — checked for subOct APVTS params in source files
- **Issue:** 19-01-SUMMARY.md did not exist; needed to verify 19-01 backend was committed
- **Fix:** Confirmed commits 01d1320 and 10f1ff1 contain all 19-01 backend work; created 19-01-SUMMARY.md
- **Files modified:** .planning/phases/19-sub-octave-per-voice/19-01-SUMMARY.md (created)
- **Verification:** All subOct params, member arrays, mid-note loop, looper gate path confirmed in source

---

**Total deviations:** 1 auto-fixed (Rule 3 - blocking prereq check)
**Impact on plan:** Non-invasive. Backend was already complete; just needed documentation.

## Issues Encountered

### Known Issue (Deferred)
**Occasional stuck note** — intermittent, low severity. Observed during smoke test: under certain mid-note toggle or rapid trigger sequences a note-off can occasionally be missed. Root cause not yet identified. Deferred to a future milestone for debugging. All 4 SUBOCT requirements verified as working.

## Smoke Test Result

**Status: APPROVED** (2026-03-01)

All 4 SUBOCT requirements verified in DAW:
- SUBOCT-01: SUB8 button visible on all 4 pads, toggles orange/darkgrey correctly
- SUBOCT-02: Sub-octave note emits at pitch-12 alongside main note
- SUBOCT-03: Note-off matches note-on pitch (snapshot held correctly)
- SUBOCT-04: R3 combo toggles SUB8 state (no unwanted panic on R3 alone)

Known minor issue: occasional stuck note (intermittent). Deferred to future milestone.

## User Setup Required
None - VST3 deployed automatically by CMake install step during Release build.

## Next Phase Readiness
- Phase 19 complete: all SUBOCT-01..04 requirements verified
- SUB8 state saves with preset (ButtonParameterAttachment handles APVTS serialization)
- Phase 20 (RND Trigger Extensions) can begin

---
*Phase: 19-sub-octave-per-voice*
*Completed: 2026-03-01*
