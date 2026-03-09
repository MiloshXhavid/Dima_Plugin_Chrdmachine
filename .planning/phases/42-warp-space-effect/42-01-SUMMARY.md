---
phase: 42-warp-space-effect
plan: 01
subsystem: ui
tags: [juce, animation, particle-system, looper, joystickpad]

# Dependency graph
requires:
  - phase: 41-smart-chord-display
    provides: livePitch_ atomics and Phase 40 crosshair members — last JoystickPad additions before Phase 42
  - phase: 40-pitch-axis-crosshair
    provides: livePitch_[4] array in JoystickPad private section (insertion anchor for Phase 42 members)
provides:
  - WarpStar struct nested in JoystickPad private section
  - warpStars_ vector (warp star pool)
  - warpRamp_ float (0..1 linear, smoothstepped at use sites)
  - timerCallback warp ramp step (4 s at 30 Hz) reading proc_.looperIsPlaying()
  - timerCallback warp star animate loop with acceleration (+=0.0008*warpT) + respawn at dist>1.2f
  - Starfield large-star freeze guard (r > 1.2f skips drift during warp)
  - resized() warpStars_ pool init with scattered angles
affects:
  - 42-02-warp-space-effect (draw pass — reads warpRamp_ and warpStars_ from this plan)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Ramp state (warpRamp_) stored as member float, smoothstepped at each use site — not stored as member"
    - "warpT = warpRamp_ * warpRamp_ * (3.0f - 2.0f * warpRamp_) smoothstep computed inline at use site"
    - "Pool never shrinks mid-session to avoid deallocation on timer thread — grow-only via resize()"
    - "Static juce::Random with fixed 64-bit seed for deterministic per-session scatter"

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "warpT is a local variable computed from warpRamp_ at each use site — NOT a member — consistent with project style"
  - "Large-star freeze (r > 1.2f) uses warpRamp_ > 0.0f guard — activates immediately on looper play, not on smoothstep threshold"
  - "warpStars_ pool grows when capacity increases but never shrinks mid-session — avoids deallocation on timer thread"
  - "Seed 0xBEEF42CAFE0000LL is a valid 64-bit literal (no UB) — replaces pseudocode 0xWARPINIT from research notes"
  - "warpRamp_ step = 1/(4*30) = 4 s ramp at 30 Hz timer — matches plan spec exactly"

patterns-established:
  - "Ramp-based animation: linear ramp in member, smoothstep computed inline at draw/animate sites"

requirements-completed: [WARP-01, WARP-02, WARP-03, WARP-04, WARP-05]

# Metrics
duration: 10min
completed: 2026-03-09
---

# Phase 42 Plan 01: Warp Space Effect Summary

**WarpStar particle engine (ramp, pool, animation) wired into JoystickPad timerCallback and resized() — no draw changes, logic-only foundation for Plan 02 draw pass**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-03-09T18:26:04Z
- **Completed:** 2026-03-09T18:36:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- WarpStar struct (angle, dist, speed) added to JoystickPad private section in PluginEditor.h
- warpRamp_ float + warpStars_ vector added as JoystickPad members
- Three insertion sites in PluginEditor.cpp: large-star freeze, warp ramp update, warp star animation loop, and resized() pool init
- VST3 built and installed clean with 0 errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Add WarpStar struct and new members to JoystickPad header** - `05a2866` (feat)
2. **Task 2: Wire warp ramp + star pool animation in timerCallback and resized()** - `94c539f` (feat)

**Plan metadata:** (docs commit — see below)

## Files Created/Modified
- `Source/PluginEditor.h` - WarpStar struct, warpStars_ vector, warpRamp_ float in JoystickPad private section
- `Source/PluginEditor.cpp` - Large-star freeze guard, warp ramp step logic, warp star animate loop, resized() pool init

## Decisions Made
- warpT is a local variable computed from warpRamp_ at each use site — NOT stored as a member, consistent with project style for derived animation values
- Seed `0xBEEF42CAFE0000LL` is a valid 64-bit literal (the research notes had pseudocode `0xWARPINIT...` which would not compile)
- Pool grows but never shrinks mid-session to avoid deallocation calls on the timer thread
- Large-star freeze uses `warpRamp_ > 0.0f` (not `warpT > 0.0f`) — activates immediately on looper play, not at some smoothstep threshold

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Build succeeded first attempt.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- warpRamp_ ramps 0→1 on looper play, 1→0 on stop (silently, no visual yet)
- warpStars_ pool is initialised in resized() with scattered angles, animated in timerCallback
- Plan 02 draw pass can read warpRamp_ and warpStars_ directly — no additional state changes needed

## Self-Check: PASSED

- Source/PluginEditor.h: FOUND
- Source/PluginEditor.cpp: FOUND
- .planning/phases/42-warp-space-effect/42-01-SUMMARY.md: FOUND
- Commit 05a2866 (Task 1): FOUND
- Commit 94c539f (Task 2): FOUND

---
*Phase: 42-warp-space-effect*
*Completed: 2026-03-09*
