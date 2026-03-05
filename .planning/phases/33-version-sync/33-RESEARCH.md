# Phase 33: Version Sync — Research

**Researched:** 2026-03-05
**Domain:** Version bumping, CMake configure_file, Inno Setup installer, gh CLI GitHub releases, desktop backup
**Confidence:** HIGH (all findings from direct file reads and live CLI queries)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- v1.7 is a full stable release — mark as Latest on GitHub (replaces v1.6 installer as current)
- Tag: `v1.7`
- Release title: `DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.7`
- Installer filename: `DIMEA-ChordJoystickMK2-v1.7-Setup.exe`
- v1.6 GitHub release: not published (skipped — superseded by v1.7)
- Release notes style: same format as v1.5/v1.6 — short narrative intro (1–3 sentences) then feature bullets grouped by category
- v1.7 feature groupings:
  1. Visual Overhaul — space background, milky way particle field, BPM-synced glow ring, semitone grid on joystick
  2. Joystick Physics — spring-damper cursor with inertia settle, perimeter arc indicator, note-label compass at cardinal positions
  3. Gamepad SWAP/INV — stick swap (left/right) and axis invert routing via APVTS params, visual indicators in gamepad UI
  4. Bug Fixes & Triplets — 6-bug batch (arp step counting, gate length on joystick triggers, CC mode expansion, left-stick deadzone rescaling, looper reset syncs random phases, population knob deterministic), triplet subdivisions in random trigger and quantize selectors
- Version bump locations: CMakeLists.txt VERSION field, installer/DimaChordJoystick-Setup.iss.in (AppVersion, AppComments, OutputBaseFilename)
- Installer: AppVersion 1.6 → 1.7; AppComments updated to v1.7 feature list; OutputBaseFilename → DIMEA-ChordJoystickMK2-v1.7-Setup; all other fields unchanged
- VST3 bundle source path: confirmed as the current PRODUCT_NAME value (see Version Strings section)
- Build Requirements: clean Release build required; verify .vst3 timestamp newer than last commit before publishing
- Desktop backup target: %USERPROFILE%\Desktop\Dima_plug-in\v1.7\ — contents: installer .exe, .vst3 bundle, source-v1.7.zip
- TRIP-01 and TRIP-02 must be marked complete in REQUIREMENTS.md
- DIST-05 is already complete (Phase 30-01); DIST-06 superseded/skipped
- New DIST-07 + DIST-08 pair should be minted for v1.7 GitHub release and desktop backup

### Claude's Discretion
(None specified — all decisions locked)

### Deferred Ideas (OUT OF SCOPE)
- Phase 34: not yet defined
- v1.6 GitHub release (DIST-06): permanently skipped — v1.7 supersedes it
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DIST-07 | GitHub v1.7 release created with built installer binary and release notes | gh CLI pattern established in Phase 30-02; plugin remote confirmed as `plugin` pointing to MiloshXhavid/Dima_Plugin_Chrdmachine |
| DIST-08 | Full plugin copy backed up to Desktop/Dima_plug-in/v1.7/ | Desktop backup pattern established in Phase 30-02; copy commands and folder structure documented |
| TRIP-01 | Mark complete — random trigger subdivision selector includes triplet values | Status: Pending in REQUIREMENTS.md; code confirmed implemented in Phase 27 per STATE.md |
| TRIP-02 | Mark complete — quantize trigger subdivision selector includes triplet values | Status: Pending in REQUIREMENTS.md; code confirmed implemented in Phase 27 per STATE.md |
</phase_requirements>

---

## Version Strings

### CMakeLists.txt
**File:** `C:/Users/Milosh Xhavid/get-shit-done/CMakeLists.txt`
**Line 2:** `project(ChordJoystick VERSION 1.7.0)`

The VERSION field is already at `1.7.0`. No change needed to CMakeLists.txt for version number.

**PRODUCT_NAME (line 118):** `"Arcade Chord Control (BETA-Test)"`
**COMPANY_NAME (line 119):** `"Dimea Arcade"`

This means the current VST3 bundle folder name is `Arcade Chord Control (BETA-Test).vst3` — NOT the old `DIMEA CHORD JOYSTICK MK2.vst3` referenced in prior Phase 30 plans. This is a critical breaking change from previous distribution assumptions.

**Key discovery — configure_file pattern (lines 4-9):**
```cmake
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimaChordJoystick-Setup.iss.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/DimaChordJoystick-Setup.iss"
    @ONLY
)
```
The .iss file is auto-generated from .iss.in at cmake configure time. The .iss.in uses `@PROJECT_VERSION@` as the placeholder. **This means the version in .iss is already at 1.7.0** because cmake was configured with VERSION 1.7.0 — the generated .iss already shows `AppVersion=1.7.0`.

### Installer .iss.in Template
**File:** `C:/Users/Milosh Xhavid/get-shit-done/installer/DimaChordJoystick-Setup.iss.in`

This is the authoritative source. It uses `@PROJECT_VERSION@` in three places:
- `AppVersion=@PROJECT_VERSION@` (generates `1.7.0`)
- `AppComments=v@PROJECT_VERSION@` (generates `v1.7.0` — a stub, needs real feature list)
- `OutputBaseFilename=DimeaArcade-ChordControl-v@PROJECT_VERSION@-Setup` (generates `DimeaArcade-ChordControl-v1.7.0-Setup`)

**Current generated .iss state** (`installer/DimaChordJoystick-Setup.iss`):
- AppName=`Arcade Chord Control (BETA-Test)`
- AppVersion=`1.7.0`
- AppPublisher=`Dimea Arcade`
- AppPublisherURL=`https://www.dimea.com`
- AppComments=`v1.7.0` (stub only — needs v1.7 feature list)
- OutputBaseFilename=`DimeaArcade-ChordControl-v1.7.0-Setup`
- VST3 Source path: `..\build\ChordJoystick_artefacts\Release\VST3\Arcade Chord Control (BETA-Test).vst3\*`
- DestDir: `{commoncf64}\VST3\Arcade Chord Control (BETA-Test).vst3`

**CRITICAL MISMATCH vs CONTEXT.md decisions:**
- CONTEXT.md says installer filename should be `DIMEA-ChordJoystickMK2-v1.7-Setup.exe`
- The .iss.in generates `DimeaArcade-ChordControl-v1.7.0-Setup.exe`
- CONTEXT.md says AppName/branding is `DIMEA CHORD JOYSTICK MK2` — but CMakeLists.txt now has PRODUCT_NAME `Arcade Chord Control (BETA-Test)` and COMPANY_NAME `Dimea Arcade`
- The plugin has been renamed since the CONTEXT.md was written

**Resolution:** The CONTEXT.md locked decisions reference old branding. The planner must use the actual current values from the codebase. The .iss.in template controls the generated output. To match CONTEXT.md's desired filename `DIMEA-ChordJoystickMK2-v1.7-Setup.exe`, either:
  - (a) Update OutputBaseFilename in .iss.in to `DIMEA-ChordJoystickMK2-v1.7-Setup` (hardcoded, ignoring version template)
  - (b) Edit the generated .iss directly after cmake configure (but it says "generated — edit .iss.in instead")
  - (c) Accept the new branding filename `DimeaArcade-ChordControl-v1.7.0-Setup.exe`

**AppComments** needs updating from `v1.7.0` stub to a real v1.7 feature list. This requires editing .iss.in (since AppComments is not templated from cmake — it just echoes the version number stub).

---

## Installer .iss State

### Current Generated .iss (full content)
```ini
; DimaChordJoystick-Setup.iss  (generated — edit DimaChordJoystick-Setup.iss.in instead)
; Inno Setup 6.7.1 installer for Arcade Chord Control (BETA-Test) VST3 v1.7.0

[Setup]
AppName=Arcade Chord Control (BETA-Test)
AppVersion=1.7.0
AppPublisher=Dimea Arcade
AppPublisherURL=https://www.dimea.com
AppComments=v1.7.0
OutputBaseFilename=DimeaArcade-ChordControl-v1.7.0-Setup
OutputDir=Output
DefaultDirName={commoncf64}\VST3\Arcade Chord Control (BETA-Test).vst3
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
DisableDirPage=yes
Compression=lzma
SolidCompression=yes
UninstallDisplayName=Arcade Chord Control (BETA-Test) VST3

[Files]
Source: "..\build\ChordJoystick_artefacts\Release\VST3\Arcade Chord Control (BETA-Test).vst3\*"; \
    DestDir: "{commoncf64}\VST3\Arcade Chord Control (BETA-Test).vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Arcade Chord Control (BETA-Test).vst3"

[Messages]
WelcomeLabel1=Welcome to DIMEA — congratulations for unlocking musical skills. Let's play some chords! Check out the Website: www.DIMEA.com
WelcomeLabel2=Hi, I am DIMEA. I am currently no certified developer by Microsoft nor MAC OS and this Plugin is currently in Beta-Phase...
```

### What needs editing in .iss.in
The .iss.in only needs the `AppComments` line updated (it is currently a stub — just `v@PROJECT_VERSION@`). The AppComments line in .iss.in is not a cmake template variable — it is a literal value. The planner can replace the stub with the full v1.7 feature list.

For OutputBaseFilename, the planner must decide: use the current auto-generated `DimeaArcade-ChordControl-v1.7.0-Setup` (matches current branding) or hardcode `DIMEA-ChordJoystickMK2-v1.7-Setup` (matches CONTEXT.md decision but conflicts with new COMPANY_NAME/PRODUCT_NAME).

### Current installer/Output/ contents
```
DIMEA-ChordJoystickMK2-v1.5-Setup.exe    (old)
DIMEA-ChordJoystickMK2-v1.6-Setup.exe    (old — built from old branding era)
DimaChordJoystick-Setup.exe              (old)
DimaChordJoystickMK2-Setup.exe           (old)
DimeaChordJoystickMK2-v1.4-Setup.exe    (old)
release-notes-v1.4.md
release-notes-v1.5.md
```
No v1.7 installer binary yet. No v1.6 release notes file (that plan was never executed — DIST-06 skipped).

---

## Previous Distribution Steps

### Exact workflow from Phase 30-02 (last completed distribution plan pattern)

**Step 1 — Git tag**
```bash
git -C "C:/Users/Milosh Xhavid/get-shit-done" tag -a v1.6 -m "v1.6 — Triplets & Fixes"
git -C "C:/Users/Milosh Xhavid/get-shit-done" push plugin v1.6
```
Verify:
```bash
gh api repos/MiloshXhavid/Dima_Plugin_Chrdmachine/git/refs/tags/v1.6
```

**Step 2 — Write release notes to file**
Write to: `C:/Users/Milosh Xhavid/get-shit-done/installer/Output/release-notes-v1.6.md`

**Step 3 — Create GitHub Latest release**
```bash
gh release create v1.6 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --title "DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6" \
  --latest \
  --notes-file "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/release-notes-v1.6.md" \
  "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe#DIMEA-ChordJoystickMK2-v1.6-Setup.exe"
```
Fallback if `--latest` flag not supported:
```bash
gh release edit v1.6 --repo MiloshXhavid/Dima_Plugin_Chrdmachine --latest
```

**Step 4 — Verify release**
```bash
gh release view v1.6 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --json title,isPrerelease,isLatest,assets
```
Note: CONTEXT.md flags that `isLatest` JSON field may not be supported in installed gh CLI. Use `isPrerelease` as the primary verification field.

**Step 5 — Desktop backup**
```bash
mkdir -p "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6"

cp "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe" \
   "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/"

cp -r "C:/Users/Milosh Xhavid/get-shit-done/build/ChordJoystick_artefacts/Release/VST3/[BUNDLE_NAME].vst3" \
      "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/"

git -C "C:/Users/Milosh Xhavid/get-shit-done" archive \
  --format=zip \
  --prefix=ChordJoystick-v1.6/ \
  v1.6 \
  -o "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/source-v1.6.zip"
```

**Step 6 — CMake Release build**
```bash
cmake --build "C:/Users/Milosh Xhavid/get-shit-done/build" --config Release
```

**Step 7 — ISCC compile**
```bash
"C:/Program Files (x86)/Inno Setup 6/ISCC.exe" "C:/Users/Milosh Xhavid/get-shit-done/installer/DimaChordJoystick-Setup.iss"
```

---

## Git/GitHub State

### Local git tags (relevant subset)
```
v1.6      (exists — already created and pushed in Phase 30-01)
v1.6.0    (GSD framework tag — different from plugin tag)
v1.6.1    (GSD framework tag)
v1.7.0    (GSD framework tag — exists locally)
v1.7.1    (GSD framework tag — exists locally)
```
**Plugin milestone tags on plugin remote:** v1.0, v1.1, v1.2, v1.3, v1.3-planning, v1.4, v1.5, v1.6
**v1.7 tag: NOT yet on the plugin remote** — no `refs/tags/v1.7` found in `gh api repos/MiloshXhavid/Dima_Plugin_Chrdmachine/git/refs/tags`.

Note: The local tag `v1.7.0` is a GSD framework tag (not the plugin milestone tag). The plugin milestone tag to create is bare `v1.7` (matching the pattern v1.0, v1.1, ..., v1.6).

**IMPORTANT:** Check whether bare `v1.7` already exists locally before creating:
```bash
git -C "C:/Users/Milosh Xhavid/get-shit-done" tag -l v1.7
```
If it returns empty, create it fresh. If it exists, verify it points to HEAD.

### Remotes
```
origin   https://github.com/glittercowboy/get-shit-done.git   (GSD framework — do NOT use)
plugin   https://github.com/MiloshXhavid/Dima_Plugin_Chrdmachine (plugin repo — use this)
```

### GitHub Release State (plugin remote)
| Tag | Title | Status |
|-----|-------|--------|
| v1.6 | DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6 | **Latest** |
| v1.5 | DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.5 | Finished (no badge) |
| v1.4 | ChordJoystick v1.4 — LFO + Beat Clock | Finished (no badge) |
| v1.3 | ChordJoystick MK2 v1.3 | Finished (no badge) |
| v1.0-v1.2 | ChordJoystick MK2 v1.x | Finished (no badge) |

v1.7 release does not yet exist on GitHub. v1.6 is the current Latest and will be superseded.

### gh CLI Version
`gh version 2.87.2 (2026-02-20)` — supports `--latest` flag on `gh release create`.

---

## Requirements State

### TRIP-01
**Current status in REQUIREMENTS.md:** `[ ] **TRIP-01**` — marked PENDING (checkbox unchecked)
**Traceability table:** `TRIP-01 | Phase 27 | Pending`
**Reality:** Phase 27 is listed as "Complete" in STATE.md Phase Map, and STATE.md decisions confirm "Triplet subdivisions interleaved with straight counterparts in all selectors (RandomSubdiv enum, APVTS choices, quantizeSubdivToGridSize, PluginEditor ComboBoxes)". The code was implemented; the checkbox in REQUIREMENTS.md was never updated.
**Action:** Change `[ ] **TRIP-01**` to `[x] **TRIP-01**` and update Traceability table from `Pending` to `Complete`.

### TRIP-02
**Current status in REQUIREMENTS.md:** `[ ] **TRIP-02**` — marked PENDING (checkbox unchecked)
**Traceability table:** `TRIP-02 | Phase 27 | Pending`
**Reality:** Same as TRIP-01 — implemented in Phase 27, checkbox never updated.
**Action:** Change `[ ] **TRIP-02**` to `[x] **TRIP-02**` and update Traceability table from `Pending` to `Complete`.

### DIST-05 and DIST-06
**DIST-05:** `[ ]` Pending in REQUIREMENTS.md — but Phase 30-01 SUMMARY is noted as created (per STATE.md "1/2 plans done"). The 30-01-PLAN.md shows DIST-05 as its requirement. However REQUIREMENTS.md still shows it unchecked. The plan may have been executed but REQUIREMENTS.md not updated.
**DIST-06:** `[ ]` Pending — permanently skipped per CONTEXT.md. Leave unchecked or add a note.

**Action for this phase:** The planner should also mark DIST-05 as complete (Phase 30 plan 01 built the v1.6 installer — the requirement was satisfied even if the GitHub release was skipped). DIST-06 can be left as-is or annotated "skipped — superseded by v1.7".

### New requirements to mint: DIST-07, DIST-08
These do not yet exist in REQUIREMENTS.md. They must be added in the v1.6 (or new v1.7) section:
- `[ ] **DIST-07**: GitHub v1.7 release created with built installer binary and release notes`
- `[ ] **DIST-08**: Full plugin copy backed up to Desktop/Dima_plug-in/v1.7/`

Both will be marked complete by the end of Phase 33.

---

## VST3 Bundle Name Discovery

The build output directory contains many stale bundles from name changes over development history:
```
Arcade Chord Control (BETA-Test).vst3     <-- CURRENT (matches PRODUCT_NAME in CMakeLists.txt)
Chord Joystick Control (BETA).vst3        (old)
Chord Joystick Controller (BETA).vst3     (old)
ChordJoystick MK2.vst3                    (old)
DIMEA CHORD JOYSTICK MK2.vst3            (old — used in Phase 30-01 .iss)
Dima_Chord_Joy_MK2.vst3                  (old)
Dimea Arcade - Chord Joystick Controller.vst3 (old)
Joystick Chord Control (β-test).vst3      (old)
```

After a fresh Release build, the correct bundle will be the one matching the current PRODUCT_NAME. To confirm after build:
```bash
ls "C:/Users/Milosh Xhavid/get-shit-done/build/ChordJoystick_artefacts/Release/VST3/"
```
Look for the most recently modified `.vst3` folder — it should be `Arcade Chord Control (BETA-Test).vst3`.

The current generated .iss already references `Arcade Chord Control (BETA-Test).vst3` — it matches. No .iss correction needed for the bundle path.

---

## Gotchas

### Gotcha 1 — CMakeLists.txt VERSION is already 1.7.0
The project VERSION is already at `1.7.0` (line 2 of CMakeLists.txt). This means the generated .iss is already correct for the version number. The planner does NOT need to bump the version in CMakeLists.txt — it was already done when v1.7 development started.

### Gotcha 2 — .iss is auto-generated; edit .iss.in not .iss
The comment on line 1 of DimaChordJoystick-Setup.iss says "generated — edit DimaChordJoystick-Setup.iss.in instead". The `configure_file()` call in CMakeLists.txt regenerates .iss from .iss.in at every `cmake configure` run. Direct edits to .iss survive until the next cmake configure, then get overwritten. For permanent changes (like AppComments), edit .iss.in.

**Implication:** The AppComments stub `v@PROJECT_VERSION@` in .iss.in must be replaced with the actual v1.7 feature description text. Then re-run cmake configure (or just edit the generated .iss directly if cmake will not be re-run before ISCC).

### Gotcha 3 — Branding mismatch: DIMEA vs Dimea Arcade
CONTEXT.md locked decisions reference the old branding:
- Old: AppName=`DIMEA CHORD JOYSTICK MK2`, AppPublisher=`DIMEA`
- Current: AppName=`Arcade Chord Control (BETA-Test)`, AppPublisher=`Dimea Arcade`

The CONTEXT.md decisions were written before the codebase rename. The actual .iss.in reflects the current CMakeLists.txt PRODUCT_NAME/COMPANY_NAME. The planner must decide which to honor — the locked CONTEXT.md decisions (requiring .iss.in edits to restore old branding) or the current codebase state (accept new branding). Given the code is the ground truth, the research recommends accepting the current branding and updating CONTEXT.md-derived outputs accordingly.

### Gotcha 4 — OutputBaseFilename mismatch
CONTEXT.md decision: `OutputBaseFilename=DIMEA-ChordJoystickMK2-v1.7-Setup`
Current .iss.in generates: `DimeaArcade-ChordControl-v1.7.0-Setup`

These differ. The gh release create command asset path must use whichever filename ISCC actually produces. The planner should use the generated name `DimeaArcade-ChordControl-v1.7.0-Setup.exe` unless the .iss.in is explicitly updated.

### Gotcha 5 — v1.7.0 local GSD tag conflict
Local tag `v1.7.0` and `v1.7.1` exist (GSD framework tags). The plugin milestone tag to create is bare `v1.7` (not `v1.7.0`). These are distinct tags. Check `git tag -l v1.7` (no suffix) to confirm the plugin milestone tag does not already exist locally.

### Gotcha 6 — RC file / Reaper vendor display
`build/ChordJoystick_artefacts/JuceLibraryCode/ChordJoystick_resources.rc` has CompanyName baked at cmake configure time. Reaper reads this for vendor display, not the VST3 API. If cmake was configured when COMPANY_NAME was "Dimea Arcade", the RC file should show "Dimea Arcade". This is not a blocker for GitHub release publication but may affect Reaper scanning UX.

### Gotcha 7 — COPY_PLUGIN_AFTER_BUILD requires elevated process
CMakeLists.txt has `COPY_PLUGIN_AFTER_BUILD TRUE`. The Release build will attempt to copy the .vst3 to the system VST3 folder. If the build process is not elevated, this copy step may fail (non-fatal — build still succeeds, but the system-installed copy is not updated). Use `fix-install.ps1` or run VS as admin for a clean system install alongside the build.

### Gotcha 8 — isLatest JSON field availability
From CONTEXT.md: "GitHub `isLatest` JSON field not supported in installed gh CLI". Verification should rely on `isPrerelease=false` and the release list badge, not on `isLatest`.

### Gotcha 9 — --repo flag is mandatory
All `gh release` commands MUST include `--repo MiloshXhavid/Dima_Plugin_Chrdmachine`. Without this flag, gh defaults to the `origin` remote which is the GSD framework repo (`glittercowboy/get-shit-done`), not the plugin repo.

### Gotcha 10 — No v1.6 smoke test gate needed
Phase 30-01 already completed the v1.6 build and smoke test (STATE.md confirms "1/2 plans done" for Phase 30). Phase 33 skips the v1.6 GitHub release entirely. The v1.7 smoke test should gate before the GitHub release, not before the build.

---

## Open Questions

1. **AppComments branding decision**
   - What we know: .iss.in currently has `AppComments=v@PROJECT_VERSION@` (stub only)
   - What's unclear: Should AppComments use the v1.7 feature list from CONTEXT.md (with old "DIMEA CHORD JOYSTICK MK2" references) or updated text for the new "Arcade Chord Control" branding?
   - Recommendation: The planner should write new AppComments text reflecting the current product name and v1.7 features. The CONTEXT.md feature groupings remain accurate regardless of product name.

2. **Installer filename: legacy vs. current branding**
   - What we know: CONTEXT.md says `DIMEA-ChordJoystickMK2-v1.7-Setup.exe`; current .iss.in generates `DimeaArcade-ChordControl-v1.7.0-Setup.exe`
   - What's unclear: Which should the planner use?
   - Recommendation: Update .iss.in OutputBaseFilename to match CONTEXT.md's decision (`DIMEA-ChordJoystickMK2-v1.7-Setup`) for consistency with prior releases listed on GitHub. But this is a planner judgment call since CONTEXT.md is locked.

3. **DIST-05 mark-complete**
   - What we know: Phase 30-01 was executed and created the v1.6 installer; REQUIREMENTS.md still shows DIST-05 as Pending
   - What's unclear: Should the planner mark DIST-05 complete in REQUIREMENTS.md as part of Phase 33 housekeeping?
   - Recommendation: Yes — mark DIST-05 complete (installer was built and smoke-tested per STATE.md). Leave DIST-06 with a note "skipped — superseded by v1.7".

4. **cmake configure before ISCC**
   - What we know: AppComments in .iss.in needs editing; configure_file() runs at cmake configure time
   - What's unclear: Does the planner need to run `cmake -S . -B build` (configure step) after editing .iss.in, or just edit the generated .iss directly?
   - Recommendation: Edit the generated .iss directly (since cmake was already configured and a rebuild doesn't re-run configure by default). This is safe as long as cmake is not re-configured before ISCC runs. Alternatively, re-run configure then build in one step.

---

## Sources

### Primary (HIGH confidence)
- `C:/Users/Milosh Xhavid/get-shit-done/CMakeLists.txt` — version field (line 2), PRODUCT_NAME (line 118), configure_file pattern (lines 4-9)
- `C:/Users/Milosh Xhavid/get-shit-done/installer/DimaChordJoystick-Setup.iss.in` — template source of truth for all installer fields
- `C:/Users/Milosh Xhavid/get-shit-done/installer/DimaChordJoystick-Setup.iss` — current generated .iss state
- `C:/Users/Milosh Xhavid/get-shit-done/.planning/REQUIREMENTS.md` — TRIP-01, TRIP-02 current status
- `C:/Users/Milosh Xhavid/get-shit-done/.planning/phases/33-version-sync/33-CONTEXT.md` — locked decisions
- `gh release list --repo MiloshXhavid/Dima_Plugin_Chrdmachine` — live GitHub release state
- `gh api repos/MiloshXhavid/Dima_Plugin_Chrdmachine/git/refs/tags` — live remote tag list
- `git tag --list` — local tag inventory
- `git remote -v` — confirmed `plugin` remote points to MiloshXhavid/Dima_Plugin_Chrdmachine
- `ls build/ChordJoystick_artefacts/Release/VST3/` — current bundle name confirmed as `Arcade Chord Control (BETA-Test).vst3`

### Secondary (MEDIUM confidence)
- `.planning/phases/30-distribution/30-01-PLAN.md` — exact build/ISCC/smoke test workflow
- `.planning/phases/30-distribution/30-02-PLAN.md` — exact gh release create and desktop backup workflow
- `.planning/phases/25-distribution/25-01-PLAN.md` — .iss update pattern (historical reference)
- `.planning/STATE.md` — Phase 27 complete status confirming TRIP-01/02 implemented

---

## Metadata

**Confidence breakdown:**
- Version strings: HIGH — direct file read, no inference
- .iss state: HIGH — direct file read of both .iss.in and generated .iss
- Git/GitHub state: HIGH — live CLI queries confirmed
- TRIP-01/02 status: HIGH — REQUIREMENTS.md and STATE.md cross-referenced
- Build workflow: HIGH — exact commands copied from prior executed plans
- Branding mismatch issue: HIGH — confirmed from direct CMakeLists.txt and CONTEXT.md comparison

**Research date:** 2026-03-05
**Valid until:** This research reflects the exact current state of files on disk — valid for the current session. Re-read CMakeLists.txt and .iss if significant time passes or if the codebase changes.
