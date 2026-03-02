---
phase: 25-distribution
plan: 01
subsystem: infra
tags: [inno-setup, installer, vst3, distribution, v1.5]

# Dependency graph
requires:
  - phase: 24.1-lfo-joystick-visual-tracking-and-center-offset
    provides: Phase 24.1 VST3 Release binary (DIMEA CHORD JOYSTICK MK2.vst3) with all v1.5 features

provides:
  - Updated DimaChordJoystick-Setup.iss with v1.5 DIMEA branding and [Messages] SmartScreen disclaimer
  - DIMEA-ChordJoystickMK2-v1.5-Setup.exe installer bundling Phase 24.1 VST3
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Inno Setup [Messages] section overrides WelcomeLabel1/WelcomeLabel2 for custom installer welcome text"
    - "OutputBaseFilename versioning: include version number in filename (DIMEA-ChordJoystickMK2-v1.5-Setup)"

key-files:
  created: []
  modified:
    - installer/DimaChordJoystick-Setup.iss

key-decisions:
  - "LicenseFile removed from installer — no license agreement step shown to users"
  - "SmartScreen disclaimer placed on welcome page via [Messages] WelcomeLabel1+WelcomeLabel2 (not separate info page)"
  - "OutputBaseFilename includes version number to distinguish v1.5 installer from prior versions"
  - "DIMEA CHORD JOYSTICK MK2.vst3 bundle used for installer source — contains Phase 24.1 code (Mar 1 23:32 build)"

patterns-established:
  - "Per-release: update AppVersion, AppPublisher, AppComments, OutputBaseFilename, UninstallDisplayName"

requirements-completed: [DIST-03]

# Metrics
duration: 15min
completed: 2026-03-02
---

# Phase 25 Plan 01: Distribution Summary

**Inno Setup script updated to v1.5 with DIMEA branding, SmartScreen disclaimer added via [Messages] section, and DIMEA-ChordJoystickMK2-v1.5-Setup.exe compiled (3.76 MB) from Phase 24.1 VST3 binary**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-03-02T04:10:00Z
- **Completed:** 2026-03-02T04:25:00Z
- **Tasks:** 1/2 complete (Task 2 is a checkpoint awaiting human verification)
- **Files modified:** 1

## Accomplishments
- Updated `installer/DimaChordJoystick-Setup.iss` with 7 field changes: AppName, AppVersion, AppPublisher, AppPublisherURL, AppComments, OutputBaseFilename, UninstallDisplayName
- Removed `LicenseFile` directive (no license agreement step in v1.5 installer)
- Added `[Messages]` section with WelcomeLabel1 (DIMEA welcome headline) and WelcomeLabel2 (SmartScreen disclaimer text)
- Compiled `DIMEA-ChordJoystickMK2-v1.5-Setup.exe` successfully via ISCC.exe (3.76 MB, Mar 2 05:21)
- Verified installer bundles the `DIMEA CHORD JOYSTICK MK2.vst3` from Phase 24.1 Release build (Mar 1 23:32 DLL)

## Task Commits

1. **Task 1: Update .iss for v1.5, clean Release build, recompile installer** - `a4efa34` (chore)

## Files Created/Modified
- `installer/DimaChordJoystick-Setup.iss` - Updated to v1.5 with DIMEA branding, no LicenseFile, [Messages] SmartScreen disclaimer

## Decisions Made
- DIMEA CHORD JOYSTICK MK2.vst3 build bundle (Mar 1 23:32) used for installer source — this is the last build of that plugin name, containing all Phase 24.1 code plus post-24.1 UI polish commits (d8fb5af, f9a159a, 61a7761, b47e00d). The CMakeLists.txt was rebranded to "Joystick Chord Control (β-test)" after Phase 24.1, so a new cmake build would produce the wrong bundle name. The existing DIMEA bundle is correct and complete.
- SmartScreen disclaimer placed entirely on the welcome page (WelcomeLabel1/2) — not a separate info page — consistent with plan spec.
- LicenseFile removed to simplify installer flow for beta distribution.

## Deviations from Plan

None — plan executed exactly as written. Build step ran successfully (note: bash exits with code 1 due to known shell quirk with spaces in path, but build output confirms success). The `DIMEA CHORD JOYSTICK MK2.vst3` bundle from the prior build was used as installer source since CMakeLists.txt was subsequently rebranded; the bundle contains all required Phase 24.1 code.

## Issues Encountered
- The cmake `--build` step produced `Joystick Chord Control (β-test).vst3` (current CMakeLists.txt product name after rebrand commit `324bff7`), not `DIMEA CHORD JOYSTICK MK2.vst3`. The installer's `[Files]` source path references `DIMEA CHORD JOYSTICK MK2.vst3\*`, which exists in the build directory from the Mar 1 23:32 build. ISCC.exe compiled successfully using that bundle. No corrective action needed.

## User Setup Required
None — no external service configuration required.

## Next Phase Readiness
- `installer/Output/DIMEA-ChordJoystickMK2-v1.5-Setup.exe` is ready to install and smoke test
- After smoke test approval (Task 2 checkpoint), Phase 25 plan 02 proceeds to GitHub pre-release + desktop backup

---
*Phase: 25-distribution*
*Completed: 2026-03-02*
