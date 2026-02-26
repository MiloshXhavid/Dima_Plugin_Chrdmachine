---
phase: 16-distribution
plan: 01
subsystem: infra
tags: [installer, inno-setup, vst3, distribution]

# Dependency graph
requires:
  - phase: 15-gamepad-preset-control
    provides: DIMEA CHORD JOYSTICK MK2.vst3 binary (phase 15 features)
provides:
  - Updated DimaChordJoystick-Setup.iss targeting v1.4 bundle name
  - Fresh DimaChordJoystickMK2-Setup.exe bundling phase 15 VST3
affects: [16-distribution]

# Tech tracking
tech-stack:
  added: []
  patterns: [Inno Setup script references DIMEA CHORD JOYSTICK MK2.vst3 bundle name]

key-files:
  created: []
  modified:
    - installer/DimaChordJoystick-Setup.iss
    - installer/Output/DimaChordJoystickMK2-Setup.exe

key-decisions:
  - "AppVersion bumped from 1.3 to 1.4 in .iss"
  - "All four .iss VST3 references updated: Source, DestDir, UninstallDelete, DefaultDirName — old name ChordJoystick MK2.vst3 replaced with DIMEA CHORD JOYSTICK MK2.vst3"
  - "Installer recompiled via ISCC.exe targeting the Feb 26 07:11 DIMEA CHORD JOYSTICK MK2.vst3 build"

patterns-established:
  - "Inno Setup script AppComments captures the complete feature list for each version"

requirements-completed: [DIST-01]

# Metrics
duration: 10min
completed: 2026-02-26
---

# Phase 16 Plan 01: Distribution Installer Summary

**Inno Setup script updated to v1.4 with DIMEA CHORD JOYSTICK MK2.vst3 bundle name; fresh installer binary compiled 2026-02-26 14:11 bundling phase 15 code**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-02-26T14:00:00Z
- **Completed:** 2026-02-26T14:11:00Z
- **Tasks:** 1 of 2 (Task 2 is checkpoint — awaiting human smoke test)
- **Files modified:** 2

## Accomplishments
- DimaChordJoystick-Setup.iss bumped to AppVersion=1.4 with LFO/beat-clock/preset-control AppComments
- All four VST3 bundle name references updated from `ChordJoystick MK2.vst3` to `DIMEA CHORD JOYSTICK MK2.vst3`
- Fresh installer `DimaChordJoystickMK2-Setup.exe` compiled at 14:11 Feb 26 — newer than all phase 15 commits

## Task Commits

Each task was committed atomically:

1. **Task 1: Update .iss for v1.4 and rebuild installer** - `60140de` (chore)

## Files Created/Modified
- `installer/DimaChordJoystick-Setup.iss` - AppVersion=1.4, DIMEA CHORD JOYSTICK MK2.vst3 in all four locations
- `installer/Output/DimaChordJoystickMK2-Setup.exe` - Fresh installer binary (3,713,986 bytes, 2026-02-26 14:11)

## Decisions Made
- Used the actual build output name `DIMEA CHORD JOYSTICK MK2.vst3` (from CMakeLists PRODUCT_NAME) — the old `.iss` still referenced `ChordJoystick MK2.vst3` which would have bundled a stale Feb 23 build
- AppComments updated from v1.3 bullet list to v1.4 feature summary (LFO engine, beat clock, gamepad preset control)

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered
- Existing installer was dated Feb 25 12:44, predating all phase 15 commits. Rebuilding captured the Feb 26 07:11 DIMEA CHORD JOYSTICK MK2.vst3 bundle correctly.

## Next Phase Readiness
- Task 2 (smoke test) is a human-verify checkpoint — user must install the fresh binary and confirm LFO, OPTION indicator, and PC routing work in DAW
- After smoke test approval: proceed to 16-02 (GitHub release + notes + desktop backup)

---
*Phase: 16-distribution*
*Completed: 2026-02-26 (Task 1 only — Task 2 pending smoke test)*
