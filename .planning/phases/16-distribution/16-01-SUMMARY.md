---
phase: 16-distribution
plan: 01
subsystem: infra
tags: [installer, inno-setup, vst3, distribution, packaging]

# Dependency graph
requires:
  - phase: 15-gamepad-preset-control
    provides: DIMEA CHORD JOYSTICK MK2.vst3 binary (phase 15 features baked in)
  - phase: 14-lfo-ui-and-beat-clock
    provides: LFO engine, beat clock dot, all v1.4 UI features
provides:
  - Updated DimaChordJoystick-Setup.iss targeting v1.4 bundle name (DIMEA CHORD JOYSTICK MK2.vst3)
  - Fresh DimaChordJoystickMK2-Setup.exe bundling all phase 12-15 VST3 features
  - User-verified smoke test: LFO modulation, OPTION indicator, Program Change confirmed working in DAW
affects: [16-02-github-release]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Inno Setup .iss Source/DestDir/UninstallDelete must all reference the same bundle name as CMakeLists PRODUCT_NAME"

key-files:
  created: []
  modified:
    - installer/DimaChordJoystick-Setup.iss
    - installer/Output/DimaChordJoystickMK2-Setup.exe

key-decisions:
  - "AppVersion bumped from 1.3 to 1.4 in .iss"
  - "All four .iss VST3 references updated: Source, DestDir, UninstallDelete, DefaultDirName — old name ChordJoystick MK2.vst3 replaced with DIMEA CHORD JOYSTICK MK2.vst3"
  - "Installer recompiled via ISCC.exe targeting the Feb 26 DIMEA CHORD JOYSTICK MK2.vst3 build"
  - "Smoke test gate passed: user confirmed LFO modulation audible, OPTION indicator visible, Program Change messages sent in DAW"

patterns-established:
  - "Bundle path pattern: verify CMakeLists PRODUCT_NAME matches .iss Source/DestDir on every release cycle"
  - "Inno Setup script AppComments captures the complete feature list for each version"

requirements-completed: [DIST-01]

# Metrics
duration: 30min
completed: 2026-02-26
---

# Phase 16 Plan 01: Distribution Installer Summary

**Inno Setup script updated to v1.4 with DIMEA CHORD JOYSTICK MK2.vst3 bundle name; fresh installer compiled at 14:11 Feb 26 and smoke-tested in DAW — LFO modulation, OPTION indicator, and Program Change all confirmed working.**

## Performance

- **Duration:** ~30 min
- **Started:** 2026-02-26T14:00:00Z
- **Completed:** 2026-02-26T14:45:00Z
- **Tasks:** 2 of 2
- **Files modified:** 2

## Accomplishments
- DimaChordJoystick-Setup.iss bumped to AppVersion=1.4 with LFO/beat-clock/preset-control AppComments
- All four VST3 bundle name references updated from `ChordJoystick MK2.vst3` to `DIMEA CHORD JOYSTICK MK2.vst3`
- Fresh installer `DimaChordJoystickMK2-Setup.exe` compiled at 14:11 Feb 26 — newer than all phase 15 commits
- Smoke test gate passed: user installed fresh binary, verified LFO modulation audible on both axes, OPTION indicator displayed, and MIDI Program Change messages observed in DAW

## Task Commits

Each task was committed atomically:

1. **Task 1: Update .iss for v1.4 and rebuild installer** - `60140de` (chore)
2. **Task 2: Smoke test — verify all v1.4 features in DAW** - checkpoint:human-verify (approved by user — no code commit required)

**Plan metadata:** `e89697d` (docs: complete installer rebuild — checkpoint at smoke test)

## Files Created/Modified
- `installer/DimaChordJoystick-Setup.iss` - AppVersion=1.4, DIMEA CHORD JOYSTICK MK2.vst3 in all four locations, AppComments with v1.4 feature summary
- `installer/Output/DimaChordJoystickMK2-Setup.exe` - Fresh installer binary (3,713,986 bytes, 2026-02-26 14:11)

## Decisions Made
- Used the actual build output name `DIMEA CHORD JOYSTICK MK2.vst3` (from CMakeLists PRODUCT_NAME) — the old `.iss` still referenced `ChordJoystick MK2.vst3` which would have bundled a stale Feb 23 build or failed to compile
- AppComments updated from v1.3 bullet list to v1.4 feature summary: Dual-axis LFO engine (7 waveforms, sync, distortion, level), Beat clock dot on BPM knob, Gamepad Preset Control (Option button + MIDI Program Change)
- Smoke test confirmed all three feature groups (phases 12, 14, 15) are present and functional in the distributed binary

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered
- Existing installer was dated Feb 25 12:44, predating all phase 15 commits. Rebuilding correctly captured the Feb 26 DIMEA CHORD JOYSTICK MK2.vst3 bundle with all v1.4 features.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Fresh installer binary ready at `installer/Output/DimaChordJoystickMK2-Setup.exe`
- All v1.4 features smoke-tested and confirmed working
- Ready for Plan 16-02: GitHub release — tag v1.4, upload installer, write release notes, publish Gumroad listing

## Self-Check: PASSED

- installer/DimaChordJoystick-Setup.iss: exists, contains AppVersion=1.4 and DIMEA CHORD JOYSTICK MK2 references
- installer/Output/DimaChordJoystickMK2-Setup.exe: exists, dated 2026-02-26 14:11 (after phase 15 commits)
- Task 1 commit 60140de: confirmed in git log
- Smoke test: approved by user

---
*Phase: 16-distribution*
*Completed: 2026-02-26*
