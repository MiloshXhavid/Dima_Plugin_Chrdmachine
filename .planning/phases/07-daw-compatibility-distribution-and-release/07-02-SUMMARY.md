---
phase: 07-daw-compatibility-distribution-and-release
plan: "02"
subsystem: distribution
tags: [inno-setup, installer, vst3, windows, packaging, gumroad, release]

# Dependency graph
requires:
  - phase: 07-daw-compatibility-distribution-and-release
    plan: "01"
    provides: pluginval level-5 PASSED Release VST3 DLL at build/ChordJoystick_artefacts/Release/VST3/
provides:
  - "Inno Setup 6.7.1 script installer/DimaChordJoystick-Setup.iss"
  - "Distributable installer EXE installer/Output/DimaChordJoystick-Setup.exe (3.5 MB)"
  - "Installer places ChordJoystick.vst3 at C:\\Program Files\\Common Files\\VST3\\"
  - "Installer registers ChordJoystick VST3 uninstaller in Windows Add/Remove Programs"
  - "Clean-machine test approved — plugin installs and loads without dev tools"
affects: [gumroad-upload, end-users, v1-release]

# Tech tracking
tech-stack:
  added: [Inno Setup 6.7.1 (ISCC.exe)]
  patterns:
    - "Inno Setup [Files] Source with recursesubdirs+createallsubdirs flags to copy entire .vst3 bundle tree"
    - "[UninstallDelete] filesandordirs for clean VST3 bundle removal"
    - "DisableDirPage=yes — standard VST3 path, no user browsing"
    - "PrivilegesRequired=admin with PrivilegesRequiredOverridesAllowed=dialog for system-wide vs per-user choice"
    - "LicenseFile directive produces license agreement wizard page from LICENSE.txt"
    - "OutputBaseFilename=DimaChordJoystick-Setup — consistent artifact naming for Gumroad"

key-files:
  created:
    - installer/DimaChordJoystick-Setup.iss
    - LICENSE.txt
  modified: []

key-decisions:
  - "Installer EXE not committed to git — binary artifact distributed via Gumroad upload only"
  - "Code signing deferred to v2 — SmartScreen 'More info / Run anyway' flow is acceptable for v1; placeholder comment left in .iss for future cert"
  - "DisableDirPage=yes — VST3 system path standardised, no installer path browsing"
  - "Static CRT (MultiThreaded) set in CMakeLists.txt — plugin DLL carries no MSVC runtime dependency; clean-machine install requires no Visual C++ Redistributable"
  - "PrivilegesRequired=admin with dialog override — admin install is default (system VST3 path), per-user available as fallback"

patterns-established:
  - "Inno Setup .iss at installer/ subdirectory; Source paths use .. to reference project-root build output"
  - "DimaChordJoystick-Setup as canonical artifact name prefix for all distribution files"

requirements-completed: [DIST-03, DIST-04]

# Metrics
duration: ~60min (includes Inno Setup install, compile, smoke test, clean-machine test)
completed: 2026-02-23
---

# Phase 7 Plan 02: Inno Setup Installer Summary

**Inno Setup 6.7.1 script producing DimaChordJoystick-Setup.exe (3.5 MB) — 4-page wizard installer for ChordJoystick.vst3, dev-machine smoke tested and clean-machine approved, ready for Gumroad upload**

## Performance

- **Duration:** ~60 min
- **Started:** 2026-02-23
- **Completed:** 2026-02-23
- **Tasks:** 2 (1 auto + 1 checkpoint)
- **Files modified:** 2 (installer/DimaChordJoystick-Setup.iss, LICENSE.txt)

## Accomplishments

- LICENSE.txt created at project root (copied from extension-less LICENSE file) — required for Inno Setup LicenseFile directive
- installer/DimaChordJoystick-Setup.iss written with all locked decisions: AppVersion=1.0.0, DisableDirPage=yes, static CRT, recursesubdirs bundle copy, clean uninstall via [UninstallDelete]
- ISCC.exe compiled installer to installer/Output/DimaChordJoystick-Setup.exe (3.5 MB) successfully — Inno Setup 6.7.1
- Dev-machine smoke test PASSED: installed to C:\Program Files\Common Files\VST3\ChordJoystick.vst3, "ChordJoystick VST3" in Windows Apps, Ableton MIDI Effects browser confirmed, uninstall clean, reinstall successful
- Clean-machine test APPROVED by user — plugin installs and loads on a machine without Visual Studio, JUCE, or MSVC build tools

## Task Commits

Each task was committed atomically:

1. **Task 1: Author Inno Setup script and compile installer** - `257f1f8` (chore)
2. **Task 2: Checkpoint — clean-machine test** - checkpoint approved (no code commit; user approval recorded)

## Installer Script Structure

**File:** `installer/DimaChordJoystick-Setup.iss`

| Section | Key settings |
|---------|-------------|
| [Setup] | AppVersion=1.0.0, AppName=ChordJoystick, AppPublisher=Dima Xhavid |
| [Setup] | OutputBaseFilename=DimaChordJoystick-Setup, OutputDir=Output |
| [Setup] | DefaultDirName={commoncf64}\VST3\ChordJoystick.vst3, DisableDirPage=yes |
| [Setup] | LicenseFile=..\LICENSE.txt, PrivilegesRequired=admin |
| [Setup] | Compression=lzma, SolidCompression=yes |
| [Files] | Source: "..\build\ChordJoystick_artefacts\Release\VST3\ChordJoystick.vst3\*" |
| [Files] | Flags: ignoreversion recursesubdirs createallsubdirs |
| [UninstallDelete] | Type: filesandordirs; Name: "{commoncf64}\VST3\ChordJoystick.vst3" |
| [Code] | Code-signing placeholder (commented out — no cert for v1) |

**Wizard pages produced:** Welcome → License Agreement → Install Progress → Done (no directory page)

## Files Created/Modified

- `installer/DimaChordJoystick-Setup.iss` — Inno Setup 6.7.1 script for ChordJoystick v1.0.0 installer
- `LICENSE.txt` — Plain-text copy of project LICENSE file; required by Inno Setup LicenseFile directive; committed to git
- `installer/Output/DimaChordJoystick-Setup.exe` — Compiled installer EXE (3.5 MB); NOT committed to git; distribute via Gumroad upload

## Decisions Made

- **Installer EXE not in git:** Binary artifact at installer/Output/DimaChordJoystick-Setup.exe is .gitignored. The .iss script is committed; the EXE is reproduced from source by running ISCC.exe. Distribution copies go to Gumroad.
- **Code signing deferred:** SmartScreen shows "Windows protected your PC" for unsigned EXE. Users click "More info" → "Run anyway". This is expected and acceptable for v1. Code-signing placeholder comment left in .iss script for v2 when a certificate is acquired.
- **DisableDirPage=yes:** VST3 system path ({commoncf64}\VST3\) is standardised; musicians should not choose an install location. Reduces friction and prevents incorrect placement.
- **Static CRT (MultiThreaded):** CMakeLists.txt sets CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded". The plugin DLL carries no MSVC runtime dependency. Clean-machine install requires no Visual C++ Redistributable — this was the key distribution risk, now validated.
- **PrivilegesRequired=admin with PrivilegesRequiredOverridesAllowed=dialog:** Admin install is default (writes to C:\Program Files\Common Files\VST3\). The dialog override lets users choose per-user install if they cannot elevate, though per-user VST3 paths are less standard.

## Deviations from Plan

None — plan executed exactly as written. Installer compiled on first attempt. Smoke test passed. Clean-machine test approved.

## Issues Encountered

None. SmartScreen warning on unsigned EXE was expected and documented in plan. No runtime DLL issues on clean machine (static CRT confirmed working).

## User Setup Required

None for this plan — Inno Setup was already installed before plan execution.

**Gumroad upload step (post-phase, manual):** Upload `installer/Output/DimaChordJoystick-Setup.exe` to Gumroad product page. This is a distribution action, not a code task.

## Next Phase Readiness

- Phase 07 is complete — all 2 plans done (07-01 pluginval + 07-02 installer)
- Distributable artifact: `installer/Output/DimaChordJoystick-Setup.exe` (3.5 MB)
- Final distribution step: upload EXE to Gumroad
- No further code phases planned — v1.0 release candidate is complete

## Self-Check: PASSED

- FOUND: `installer/DimaChordJoystick-Setup.iss` (committed in 257f1f8)
- FOUND: `LICENSE.txt` (committed in 257f1f8)
- FOUND: commit `257f1f8` (Task 1 — Inno Setup script + LICENSE.txt)
- FOUND: `installer/Output/DimaChordJoystick-Setup.exe` (built artifact, not in git by design)
- Clean-machine test APPROVED by user

---
*Phase: 07-daw-compatibility-distribution-and-release*
*Completed: 2026-02-23*
