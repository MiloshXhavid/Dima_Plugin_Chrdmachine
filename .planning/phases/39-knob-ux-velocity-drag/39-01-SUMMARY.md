---
phase: 39-knob-ux-velocity-drag
plan: "01"
subsystem: ui
tags: [juce, slider, velocity-drag, EMA, look-and-feel, rotary-knob]

requires: []
provides:
  - VelocityKnob class (EMA-smoothed velocity-sensitive rotary drag)
  - VelocitySlider class (same EMA logic for horizontal linear sliders)
  - ScaleSnapSlider now inherits VelocityKnob
  - All 13 in-scope rotary members upgraded to VelocityKnob
  - All 10 in-scope LFO linear sliders upgraded to VelocitySlider
  - drawRotarySlider: 12-dot snap indicators for 0..11 range knobs
  - drawRotarySlider: hover ring (1.5px, 35% alpha highlight)
affects: [39-02-install-uat]

tech-stack:
  added: []
  patterns:
    - "EMA velocity-sensitive drag: smoothedSpeed_ = kAlpha*|delta| + (1-kAlpha)*smoothedSpeed_; mult = jlimit(1,3, 1+(speed-2)*(2/8))"
    - "isDotMode detection: min==0 && max==11 && interval==1 triggers 12-dot ring instead of fill arc"
    - "Hover ring drawn last in drawRotarySlider so it renders on top of all knob body elements"

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "VelocityKnob/VelocitySlider defined before PluginEditor class in header — VelocityKnob visible at ScaleSnapSlider declaration site"
  - "Shift+drag bypasses EMA and falls back to JUCE built-in fine-tune on both VelocityKnob and VelocitySlider"
  - "kBasePx=300 gives full-range sweep at 1x sensitivity; mult range 1x-3x via EMA threshold 2px-10px"
  - "isDotMode: interval==1.0 check prevents false positive from non-integer-step 0..11 sliders if any exist"

patterns-established:
  - "EMA velocity drag pattern: override mouseDown/mouseDrag/mouseUp, track lastDragDist_ and smoothedSpeed_"

requirements-completed: []

duration: 20min
completed: 2026-03-09
---

# Phase 39 Plan 01: VelocityKnob, dot indicators, hover ring Summary

**EMA velocity-sensitive drag classes (VelocityKnob + VelocitySlider) wired to 23 UI controls, plus 12-dot snap indicators and hover ring added to drawRotarySlider**

## Performance

- **Duration:** ~20 min
- **Started:** 2026-03-09T00:00:00Z
- **Completed:** 2026-03-09T00:20:00Z
- **Tasks:** 7
- **Files modified:** 2

## Accomplishments
- `VelocityKnob` and `VelocitySlider` classes added to PluginEditor.h with full EMA velocity drag logic (1x-3x sensitivity, Shift=fine-tune bypass)
- `ScaleSnapSlider` rebased from `juce::Slider` to `VelocityKnob` — inherits velocity drag without any other changes needed
- All 13 in-scope rotary members and 10 LFO linear sliders upgraded to the new types
- `drawRotarySlider`: dot mode renders 12 evenly-spaced snap dots for 0..11 range knobs, suppressing fill arc
- `drawRotarySlider`: hover ring (1.5px pink-red, 35% alpha) drawn last so it appears on top of the knob body
- Build clean with zero errors, VST3 installed

## Task Commits

Each task was committed atomically:

1. **Tasks 1-5: Header type changes** - `09f0ac8` (feat)
2. **Tasks 6-7: drawRotarySlider updates** - `792a521` (feat)

## Files Created/Modified
- `Source/PluginEditor.h` - Added VelocityKnob, VelocitySlider classes; ScaleSnapSlider rebased; 23 member type changes
- `Source/PluginEditor.cpp` - drawRotarySlider: isDotMode guard + 12 dots + hover ring

## Decisions Made
- Tasks 1-5 grouped in a single commit because they are all header declaration changes with no behavior impact independently — grouping is cleaner and the plan describes them as sequential declaration edits
- Tasks 6-7 grouped in a single commit because both modify `drawRotarySlider` in the same function — splitting would leave a partially-modified function in the tree

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - build passed on first attempt with zero new errors.

## Next Phase Readiness
- Plan 39-01 complete; plan 39-02 (install + UAT) can proceed
- VST3 already built and installed by cmake post-build step

## Self-Check: PASSED
- FOUND: .planning/phases/39-knob-ux-velocity-drag/39-01-SUMMARY.md
- FOUND: Source/PluginEditor.h
- FOUND: Source/PluginEditor.cpp
- FOUND: commit 09f0ac8 (feat: header type changes)
- FOUND: commit 792a521 (feat: drawRotarySlider updates)

---
*Phase: 39-knob-ux-velocity-drag*
*Completed: 2026-03-09*
