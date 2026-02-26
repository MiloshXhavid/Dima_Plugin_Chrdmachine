---
phase: 14-lfo-ui-and-beat-clock
plan: 02
subsystem: ui
tags: [juce, vst3, lfo, apvts, editor, layout, beat-clock, sync-toggle]

# Dependency graph
requires:
  - phase: 14-01
    provides: beatOccurred_ atomic + modulatedJoyX_/Y_ atomics in PluginProcessor
  - phase: 13-processblock-integration-and-apvts
    provides: lfoXWaveform/Rate/Phase/Level/Distortion/Sync/Subdiv/Enabled APVTS params
provides:
  - PluginEditor 1120px wide with fixed 448px left column + two 130px LFO panel columns
  - LFO X and LFO Y panels — shape ComboBox, Rate/Phase/Level/Dist sliders, SYNC button, enabled LED
  - SYNC toggle swaps Rate slider attachment between lfoXRate (Hz) and lfoXSubdiv (step index)
  - Beat pulse dot drawn adjacent to BPM label, decays over ~300ms via beatPulse_ float
  - mouseDown() routes LED area clicks to hidden ToggleButton carrying lfoXEnabled/lfoYEnabled ButtonAttachment
affects:
  - 14-03 (JoystickPad LFO tracking — reads modulatedJoyX_/Y_ from processor)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "SliderParameterAttachment swap: reset() then re-create with different param ID on button click — no layout shift"
    - "Hidden ToggleButton pattern: zero-alpha button carries APVTS attachment; mouseDown on editor routes LED hit area to it"
    - "beatPulse_ float: set to 1.0 on beat, decremented by 0.11/tick at 30 Hz — ~9 frames / ~300ms fade"
    - "Fixed-column resized(): constexpr kLeftColW=448 then removeFromLeft for each column — deterministic layout regardless of total window width"

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Editor widened to 1120px (from 920px) — 200px added to the right of the left column for two 130px LFO panels"
  - "Left column width fixed at 448px (constexpr kLeftColW) — was previously colW/2-4 which would have expanded with the wider window"
  - "dividerX_ now points to lfoXCol.getX() (~464px) — the vertical divider line in paint() remains between the left column and LFO panels"
  - "Sync toggle uses SliderParameterAttachment swap (reset + recreate) not range reconfigure — preserves APVTS value traceability"
  - "beatPulse_ drawn in paint() adjacent to bpmDisplayLabel_ bounds, not inside it — avoids touching label widget"
  - "LFO panel paint fill uses Clr::panel.brighter(0.12f) matching LOOPER/FILTER MOD panel style from Phase 11"

patterns-established:
  - "LFO panel layout pattern: 14px header clearance, 22px ComboBox, 4 x (18px slider with 30px left label offset + 4px gap), 22px SYNC button"
  - "Panel bounds computed as union of first/last control bounds, withX/withWidth from column, expanded(0,10) for header padding"

requirements-completed: [LFO-11]

# Metrics
duration: 35min
completed: 2026-02-26
---

# Phase 14 Plan 02: LFO UI — Editor Panels Summary

**1120px PluginEditor with two 130px LFO panels (shape, rate, phase, level, dist, sync), APVTS-attached sync-mode rate swap, enabled LED via hidden ToggleButton, and beat pulse dot decaying over 300ms**

## Performance

- **Duration:** 35 min
- **Started:** 2026-02-26T04:00:00Z
- **Completed:** 2026-02-26T04:35:00Z
- **Tasks:** 2 of 2
- **Files modified:** 2

## Accomplishments
- Expanded editor from 920px to 1120px; left column fixed at 448px so existing controls are unchanged width
- Added full LFO X and LFO Y panels: ComboBox (7 waveforms), Rate/Phase/Level/Dist sliders, SYNC button — all APVTS-attached
- Implemented Sync toggle Rate-swap: lfoXSyncBtn_.onClick resets lfoXRateAtt_ and re-creates it against lfoXSubdiv (6 discrete steps with subdivision label text) or lfoXRate (Hz display)
- Implemented enabled LED: hidden ToggleButton (alpha=0) carries ButtonAttachment; editor mouseDown() routes clicks on lfoXLedBounds_/lfoYLedBounds_ to toggle it
- Beat pulse dot: timerCallback reads proc_.beatOccurred_ atomic, sets beatPulse_=1.0, then decrements by 0.11/tick; paint() draws 8px circle adjacent to BPM label, alpha=beatPulse_
- Build: 0 C++ errors; VST3 installed to Common Files/VST3

## Task Commits

Each task was committed atomically:

1. **Task 1: Declare LFO panel members in PluginEditor.h** - `c33fc7a` (feat)
2. **Task 2: Implement LFO panel construction, layout, paint, and Sync toggle** - `6968771` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified
- `Source/PluginEditor.h` - LFO X/Y member declarations (sliders, combobox, buttons, attachment unique_ptrs, panel/LED bounds, beatPulse_, mouseDown override)
- `Source/PluginEditor.cpp` - setSize(1120), LFO panel construction in constructor, mouseDown impl, resized() layout refactor, paint() panel drawing + beat dot, timerCallback beat pulse

## Decisions Made
- **Left column fixed at kLeftColW=448** — previous `colW/2-4` formula would have over-widened the left column at 1120px; fixing at 448 preserves all existing control proportions exactly
- **dividerX_ points to lfoXCol.getX()** — the paint() divider line stays between left column and LFO panels rather than at the joystick edge; cosmetically correct
- **SliderParameterAttachment swap for Sync mode** — reset() + recreate with different param ID is the correct JUCE pattern; setRange() alone would decouple the slider from APVTS
- **beatPulse_ drawn in paint() outside label bounds** — positioning adjacent to bpmDisplayLabel_.getRight() avoids modifying the label widget and works regardless of label text length
- **Panel fill Clr::panel.brighter(0.12f)** — matches existing LOOPER/FILTER MOD panel style established in Phase 11; consistent visual language

## Deviations from Plan

None — plan executed exactly as written. Minor additions (initial textFromValueFunction on Rate sliders set in constructor for correct Hz display before first sync toggle) were trivially in scope.

## Issues Encountered
None. Build succeeded on first attempt with 0 errors.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- LFO UI panels are fully visible and APVTS-attached; automation lanes in DAW will show all 7 params per panel
- beatPulse_ wired and decaying correctly in timerCallback
- Phase 14 Plan 03 (JoystickPad LFO dot tracking via modulatedJoyX_/Y_) can proceed immediately
- One remaining item from CONTEXT.md deferred list: JoystickPad dot reads modulatedJoyX_/Y_ when LFO active (Plan 03)

---
*Phase: 14-lfo-ui-and-beat-clock*
*Completed: 2026-02-26*
