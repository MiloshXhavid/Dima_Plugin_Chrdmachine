# Phase 30: Distribution — Context

**Gathered:** 2026-03-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Publish v1.6 of ChordJoystick MK2 as the official stable release on GitHub (replacing v1.4 as Latest), upgrade v1.5 from pre-release to a finished release, build the installer, write release notes, and back up to desktop. No new features.

</domain>

<decisions>
## Implementation Decisions

### GitHub Release Designation
- v1.6 is a **full stable release** — mark as Latest on GitHub (replaces v1.4)
- v1.5 must be **upgraded from pre-release to finished release** before or during this phase (no longer pre-release)
- Tag: `v1.6`
- Release title: `DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6`
- Installer filename: `DIMEA-ChordJoystickMK2-v1.6-Setup.exe`

### Release Notes Style
- Same format as v1.5: short narrative intro (1–3 sentences) followed by feature bullets grouped by category
- v1.6 feature groupings:
  1. **Defaults & Bug Fix** — correct octave/scale defaults on fresh install; noteCount stuck-notes fix
  2. **Triplet Subdivisions** — triplet options in random trigger and quantize selectors
  3. **Random Free Redesign** — Poisson density, probability knob, population modulator
  4. **Looper Visualization** — clockwise perimeter progress bar, ghost ring when stopped
- No SmartScreen warning in release notes (handled inside installer)

### Installer Changes
- **Version bump only**: `AppVersion` 1.5 → 1.6 and `AppComments` updated to v1.6 feature list
- All other fields (AppName, AppPublisher, AppPublisherURL, welcome text, disclaimer, no LicenseFile) stay exactly as set in Phase 25

### Build Requirements
- Clean Release build required — verify `.vst3` binary timestamp is newer than last commit before publishing
- Installer rebuilt with updated `.iss` file

### Desktop Backup
- Target: `%USERPROFILE%\Desktop\Dima_plug-in\v1.6\` (create `v1.6.1\` if folder already exists)
- Contents: installer `.exe`, `.vst3` bundle, `source-v1.6.zip`

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `installer/DimaChordJoystick-Setup.iss` — Inno Setup script from Phase 25; only AppVersion + AppComments need updating
- `installer/Output/` — build output directory for installer binary
- Prior release workflow established in Phase 25 (build → installer → GitHub release → desktop backup)

### Established Patterns
- GitHub release via `gh release create` CLI
- Desktop backup is manual copy (robocopy or PowerShell)
- Git tag applied before GitHub release creation

### Integration Points
- `gh` CLI for GitHub release create/edit (upgrading v1.5 from pre-release uses `gh release edit v1.5 --prerelease=false`)
- CMake Release build: `cmake --build build --config Release`

</code_context>

<specifics>
## Specific Requirements

- v1.5 must be promoted to finished release (not just left as pre-release) as part of this phase
- v1.6 should become the new Latest stable release on GitHub
- Release notes narrative should highlight that v1.6 closes out the v1.6 milestone (Triplets & Fixes)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 30-distribution*
*Context gathered: 2026-03-03*
