---
phase: 30-distribution
plan: 01
subsystem: infra
tags: [inno-setup, installer, github-release, vst3, cmake]

requires:
  - phase: 29-looper-perimeter-bar
    provides: Final Phase 29 Release VST3 binary with looper perimeter bar feature
provides:
  - v1.5 GitHub release promoted to finished historical release (isPrerelease=false)
  - installer/DimaChordJoystick-Setup.iss updated to v1.6 (AppVersion, AppComments, OutputBaseFilename)
  - DIMEA-ChordJoystickMK2-v1.6-Setup.exe compiled fresh from Phase 26-29 VST3 binary
  - User smoke-test approved — v1.6 features verified in DAW
affects: [30-02-plan]

tech-stack:
  added: []
  patterns: []

key-files:
  created:
    - installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe
  modified:
    - installer/DimaChordJoystick-Setup.iss

key-decisions:
  - "VST3 bundle source path is Chord Joystick Control (BETA).vst3 — PRODUCT_NAME in CMakeLists.txt is 'Chord Joystick Control (BETA)', not 'DIMEA CHORD JOYSTICK MK2'. .iss [Files] Source updated to match actual build output; DestDir still installs to DIMEA CHORD JOYSTICK MK2.vst3 (branded public-facing name unchanged)"
  - "v1.5 promoted to finished release before v1.6 published — prevents two releases competing for Latest badge"

requirements-completed:
  - DIST-05

duration: 15min
completed: 2026-03-03
---

# Phase 30 Plan 01: v1.5 Promotion + v1.6 Installer Build Summary

**v1.5 promoted from pre-release to finished GitHub release; .iss bumped to v1.6; DIMEA-ChordJoystickMK2-v1.6-Setup.exe compiled from Phase 29 VST3 and smoke-tested approved**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-03-03T10:11:54Z
- **Completed:** 2026-03-03T10:26:11Z
- **Tasks:** 2 (1 auto + 1 checkpoint:human-verify)
- **Files modified:** 2 (installer/DimaChordJoystick-Setup.iss, installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe)

## Accomplishments

- Promoted v1.5 from pre-release to finished historical GitHub release (`isPrerelease=false`) via `gh release edit --prerelease=false`
- Updated `installer/DimaChordJoystick-Setup.iss`: `AppVersion=1.6`, `AppComments=v1.6: Defaults & Bug Fix...`, `OutputBaseFilename=DIMEA-ChordJoystickMK2-v1.6-Setup`; fixed [Files] Source path to match actual build output folder `Chord Joystick Control (BETA).vst3`
- Performed clean Release build (DLL timestamp: Mar 3 03:58) and compiled `DIMEA-ChordJoystickMK2-v1.6-Setup.exe` (3.77 MB) via ISCC.exe successfully
- User smoke-tested installer: DIMEA welcome page confirmed, no license step, v1.6 features verified in DAW — approved

## Task Commits

1. **Task 1: Promote v1.5, update .iss to v1.6, build + compile installer** - `7e4869f` (feat)
2. **Task 2: Smoke test checkpoint** - approved by user (no code commit — checkpoint gate)

## Files Created/Modified

- `installer/DimaChordJoystick-Setup.iss` — Updated to v1.6: AppVersion, AppComments, OutputBaseFilename, Source path corrected to actual VST3 bundle name
- `installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe` — Freshly compiled 3.77 MB installer bundling Phase 26–29 VST3 binary

## Decisions Made

- **Source path fix:** The current `PRODUCT_NAME` in CMakeLists.txt is `"Chord Joystick Control (BETA)"`, so the build output VST3 bundle is `Chord Joystick Control (BETA).vst3`. The .iss [Files] Source was updated to use this actual path while DestDir continues to install to `DIMEA CHORD JOYSTICK MK2.vst3` — preserving the branded public-facing install directory name.
- The `isLatest` JSON field is not supported by the installed `gh` CLI version; verification used `isPrerelease` field only (sufficient to confirm promotion).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] VST3 bundle folder name mismatch between .iss template and actual build output**
- **Found during:** Task 1, Step 4 (VST3 bundle folder name verification)
- **Issue:** Plan template had `DIMEA CHORD JOYSTICK MK2.vst3` in the .iss [Files] Source path, but `PRODUCT_NAME = "Chord Joystick Control (BETA)"` in CMakeLists.txt produces `Chord Joystick Control (BETA).vst3` as the actual build output bundle. ISCC would have failed with "Source file not found" if left uncorrected.
- **Fix:** Updated [Files] Source path to `Chord Joystick Control (BETA).vst3\*` while keeping DestDir as `DIMEA CHORD JOYSTICK MK2.vst3` (installer still deploys under the branded name)
- **Files modified:** `installer/DimaChordJoystick-Setup.iss`
- **Verification:** ISCC.exe compiled successfully in 1.532 seconds, output EXE confirmed 3.77 MB
- **Committed in:** `7e4869f` (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 bug — source path mismatch)
**Impact on plan:** Auto-fix was necessary for ISCC compilation to succeed. No scope creep. Plan's Step 4 explicitly anticipated this scenario and instructed the fix.

## Issues Encountered

- `gh release view --json isLatest` failed (field not supported in installed gh CLI version). Used `isPrerelease` field instead — sufficient to confirm v1.5 promotion. No impact on outcome.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- v1.5 finished release and v1.6 installer ready — plan 30-02 can proceed to create the v1.6 git tag, push to plugin remote, publish GitHub Latest release, and archive desktop backup
- Actual VST3 bundle folder name for plan 30-02 desktop backup step: `Chord Joystick Control (BETA).vst3`

---
*Phase: 30-distribution*
*Completed: 2026-03-03*

## Self-Check: PASSED
