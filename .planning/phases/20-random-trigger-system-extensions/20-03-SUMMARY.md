---
phase: 20-random-trigger-system-extensions
plan: 03
subsystem: ui
tags: [plugin-editor, juce, apvts, trigger-system, random-trigger, cpp]

# Dependency graph
requires:
  - phase: 20-02-random-trigger-system-extensions
    provides: APVTS params randomPopulation/randomProbability/gateLength registered; trigSrcNames 4 items; subdivChoices 5 items

provides:
  - trigSrc_[v] combo shows 4 items: Pad / Joystick / Rnd Free / Rnd Hold
  - randomSubdivBox_[v] combo shows 5 items: 1/4 / 1/8 / 1/16 / 1/32 / 1/64
  - POPULATION knob (randomDensityKnob_ widget) wired to "randomPopulation" via randomPopulationAtt_
  - PROBABILITY knob (randomProbabilityKnob_) wired to "randomProbability" via randomProbabilityAtt_
  - GATE LEN knob (randomGateTimeKnob_ widget) wired to unified "gateLength" via gateLengthAtt_
  - Arp GATE LEN slider wired to "gateLength" (range 0.0–1.0, was 5–100%)
  - randomClockSync APVTS default changed to false (off by default)

affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Widget reuse pattern: existing slider widget (randomDensityKnob_) retargeted to new APVTS param by renaming attachment only"
    - "SliderParameterAttachment used directly for new Probability knob (same as Gate knob pattern)"

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp
    - Source/PluginProcessor.cpp
    - Source/TriggerSystem.h

key-decisions:
  - "randomClockSync defaults to false — free-tempo mode is more useful out-of-the-box; sync can be enabled deliberately"
  - "Widget rename pattern: randomDensityKnob_ widget kept, only attachment member renamed to randomPopulationAtt_ — avoids layout churn"
  - "Probability knob added to rndRow as 5th column; column width computed proportionally from row width"

patterns-established:
  - "APVTS param retarget: rename attachment unique_ptr, pass new param ID to constructor — widget state follows automatically"

requirements-completed: [RND-01, RND-04, RND-05, RND-06, RND-07]

# Metrics
duration: 15min
completed: 2026-03-01
---

# Phase 20 Plan 03: Random Trigger System Extensions — PluginEditor UI Summary

**4-item trigger combos, 5-item subdiv combos, Population + Probability knobs wired to APVTS, unified Gate Length in both random strip and arp panel, RND SYNC defaults off — smoke test passed**

## Performance

- **Duration:** ~15 min
- **Completed:** 2026-03-01
- **Tasks:** 2 + 1 post-verify fix
- **Files modified:** 4

## Accomplishments

- Trigger source combos expanded from 3 to 4 items: "Rnd Free" / "Rnd Hold" (existing DAW sessions load correctly — integer values unchanged)
- Subdivision combos expanded from 4 to 5 items: "1/64" appended
- POPULATION knob relabeled and re-wired from `randomDensity` → `randomPopulation` APVTS param
- New PROBABILITY knob added to random controls row, wired to `randomProbability` APVTS param
- GATE LEN knob re-wired from `randomGateTime` → unified `gateLength` APVTS param
- Arp gate slider range changed from 5–100 to 0.0–1.0, re-wired to `gateLength`
- `randomClockSync` default changed to `false` in both APVTS layout and ProcessParams struct

## Files Created/Modified

- `Source/PluginEditor.h` — added `randomProbabilityKnob_`, renamed `randomDensityAtt_` → `randomPopulationAtt_`, renamed `randomGateTimeKnobAtt_` → `gateLengthAtt_`, added `randomProbabilityAtt_`
- `Source/PluginEditor.cpp` — trigger combos 4 items, subdiv combos 5 items, Probability knob setup + attachment, Population/Gate attachment retargets, arp gate range + attachment, resized() 5-column rndRow, paint() drawAbove labels updated
- `Source/PluginProcessor.cpp` — `randomClockSync` default `true` → `false`
- `Source/TriggerSystem.h` — `ProcessParams::randomClockSync` default `true` → `false`

## Decisions Made

- **randomClockSync defaults to false** — free-tempo mode is more immediately useful; DAW sync requires explicit opt-in. Discovered during smoke test verification.

## Deviations from Plan

### Auto-fixed Issues

**1. [User request during verify] randomClockSync default changed to false**
- **Found during:** Human smoke test checkpoint
- **Issue:** RND SYNC was on by default — user noted free-tempo mode is better default
- **Fix:** Changed APVTS default and ProcessParams struct default to false
- **Files modified:** Source/PluginProcessor.cpp, Source/TriggerSystem.h
- **Verification:** Build clean, default state confirmed off

---

**Total deviations:** 1 (user-directed post-verify fix)
**Impact on plan:** Minimal — default value change only, no structural impact.

## Issues Encountered

None during implementation. Build failure on post-build copy step was expected (elevation required); compile succeeded with 0 errors.

## User Setup Required

None — run `copy-vst.bat` or `fix-install.ps1` as admin to deploy the built VST3.

## Next Phase Readiness

- Phase 20 complete — all 3 plans executed, smoke test passed
- Phase 21 (Left Joystick Modulation Expansion) is next: CONTEXT.md exists, ready to plan

---
*Phase: 20-random-trigger-system-extensions*
*Completed: 2026-03-01*
