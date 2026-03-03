---
phase: 29-looper-perimeter-bar
plan: 01
subsystem: ui
tags: [juce, paint, perimeter-bar, looper, animation, clockwise]

# Dependency graph
requires:
  - phase: 26-defaults-and-bug-fix
    provides: stable APVTS base and looperPanelBounds_ layout infrastructure
provides:
  - clockwise perimeter bar animation around LOOPER section panel (2px green, 30 Hz)
  - ghost ring affordance when looper is stopped
  - LOOPER label exclusion from clip region in all stroke calls
  - looperPositionBarBounds_ zeroed — horizontal strip completely removed
affects: [30-distribution]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "perimPoint lambda: maps 1D perimeter distance to 2D Point on rectangle boundary, CW from top-left"
    - "ScopedSaveState + excludeClipRegion for label protection during strokePath"
    - "Tail+head two-pass rendering: tail at 0.28 alpha, head at full alpha overdraw"

key-files:
  created: []
  modified:
    - Source/PluginEditor.cpp

key-decisions:
  - "perimPoint lambda handles negative dist via fmod+conditional add — wraps correctly for tail segment starting before position 0"
  - "labelExclude computed fresh inside paint() using same drawLfoPanel constants (font 12pt bold, textX=b.getX()+6, textH=14, textY=b.getY()-7)"
  - "Ghost ring uses 1px ghostStroke, dim gateOff.brighter(0.3f); animated bar uses 2px stroke, kTailPx=40 perimeter-px tail at 0.28 alpha"
  - "looperPanelBounds_.reduced(1.0f) used so 2px stroke stays inside the panel border"
  - "looperPositionBarBounds_ kept as member but zeroed = {} in resized(); no header change needed"
  - "8px removeFromTop preserved in resized() to keep subdiv/length controls at same vertical position"

patterns-established:
  - "Perimeter bar pattern: perimPoint lambda + ScopedSaveState/excludeClipRegion reusable for any panel label"

requirements-completed: [LOOP-01, LOOP-02, LOOP-03, LOOP-04]

# Metrics
duration: ~15min
completed: 2026-03-03
---

# Phase 29 Plan 01: Looper Perimeter Bar Summary

**Clockwise 2px green perimeter bar replaces horizontal looper strip — circuits LOOPER panel at loop speed, 30 Hz via existing timer, with ghost ring at idle and LOOPER label always excluded from clip**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-03-03T~T00:00Z
- **Completed:** 2026-03-03
- **Tasks:** 2/2 (1 automated + 1 human-verify checkpoint — approved 2026-03-03)
- **Files modified:** 1

## Accomplishments
- Replaced 4px horizontal looper strip with clockwise perimeter bar using `perimPoint` lambda
- Ghost ring (1px, dim) shown when looper is idle/stopped as visual affordance
- Animated tail (40px, 0.28 alpha) + head (12px, full alpha) segments travel CW at loop speed
- LOOPER label fully protected via `excludeClipRegion(labelExclude)` in all three stroke paths
- `repaint(looperPanelBounds_)` drives 30 Hz updates; old `repaint(looperPositionBarBounds_)` removed
- `looperPositionBarBounds_` zeroed in `resized()`, 8px gap preserved to keep controls in place

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement perimeter bar in PluginEditor paint/resized/timerCallback** - `e917a47` (feat)

2. **Task 2: Deploy VST3 and visually verify perimeter bar** - Human checkpoint approved (no separate commit — verification only)

**Plan metadata:** (docs commit — this summary update)

## Files Created/Modified
- `Source/PluginEditor.cpp` - resized() strip removed, timerCallback repaint target updated, paint() block replaced with perimPoint-based perimeter bar

## Decisions Made
- `perimPoint` lambda uses `std::fmod` + conditional add so negative dist (tail before position 0) wraps correctly
- Label exclusion zone computed inline inside paint() to match `drawLfoPanel` constants exactly
- `kTailPx = 40.0f`, `kHeadPx = 12.0f` — tail at 0.28 alpha, head overdraw at full alpha
- `reduced(1.0f)` on panel bounds keeps 2px stroke inside the drawn border

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None. Build succeeded on first attempt (compiler output confirmed no errors). The spurious bash exit code 1 from the known shell/space-in-path issue does not affect the build.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All verification criteria confirmed by human:
  - No horizontal strip visible below Rec gate buttons
  - Ghost ring visible when looper is idle
  - Clockwise green bar circuits the panel at loop speed
  - LOOPER label always legible
- Phase 30 (Distribution) is fully unblocked — all v1.6 feature phases complete

## Self-Check: PASSED

- Commit e917a47 (Task 1): FOUND in git log
- Source/PluginEditor.cpp modified: FOUND
- Task 2 human-verify: APPROVED by user (2026-03-03)

---
*Phase: 29-looper-perimeter-bar*
*Completed: 2026-03-03*
