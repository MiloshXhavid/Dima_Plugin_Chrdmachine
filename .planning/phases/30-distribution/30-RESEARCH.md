# Phase 30: Distribution - Research

**Researched:** 2026-03-03
**Domain:** GitHub release publishing, Inno Setup installer, desktop backup workflow
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**GitHub Release Designation**
- v1.6 is a full stable release — mark as Latest on GitHub (replaces v1.4)
- v1.5 must be upgraded from pre-release to finished release before or during this phase (no longer pre-release)
- Tag: `v1.6`
- Release title: `DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6`
- Installer filename: `DIMEA-ChordJoystickMK2-v1.6-Setup.exe`

**Release Notes Style**
- Same format as v1.5: short narrative intro (1–3 sentences) followed by feature bullets grouped by category
- v1.6 feature groupings:
  1. Defaults & Bug Fix — correct octave/scale defaults on fresh install; noteCount stuck-notes fix
  2. Triplet Subdivisions — triplet options in random trigger and quantize selectors
  3. Random Free Redesign — Poisson density, probability knob, population modulator
  4. Looper Visualization — clockwise perimeter progress bar, ghost ring when stopped
- No SmartScreen warning in release notes (handled inside installer)

**Installer Changes**
- Version bump only: `AppVersion` 1.5 → 1.6 and `AppComments` updated to v1.6 feature list
- All other fields (AppName, AppPublisher, AppPublisherURL, welcome text, disclaimer, no LicenseFile) stay exactly as set in Phase 25

**Build Requirements**
- Clean Release build required — verify `.vst3` binary timestamp is newer than last commit before publishing
- Installer rebuilt with updated `.iss` file

**Desktop Backup**
- Target: `%USERPROFILE%\Desktop\Dima_plug-in\v1.6\` (create `v1.6.1\` if folder already exists)
- Contents: installer `.exe`, `.vst3` bundle, `source-v1.6.zip`

### Claude's Discretion

None defined in CONTEXT.md — all decisions are locked.

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DIST-05 | GitHub v1.6 release created with built installer binary and release notes listing defaults fix, noteCount bug fix, triplet subdivisions, Random Free redesign, and looper perimeter bar | Workflow verified against Phase 25 precedent; gh CLI commands documented; tag + release steps confirmed |
| DIST-06 | Full plugin copy backed up to Desktop | Desktop backup path convention established; v1.4 and v1.5 backups already exist at Desktop/Dima_plug-in/; three-artifact pattern confirmed |
</phase_requirements>

---

## Summary

Phase 30 is a pure distribution phase — no code changes. The work consists of four sequential activities: (1) promote v1.5 from pre-release to finished release on GitHub, (2) build a clean Release binary, (3) bump the Inno Setup `.iss` to v1.6 and recompile the installer, and (4) publish v1.6 as the new stable/Latest GitHub release and copy artifacts to the desktop backup folder.

The entire workflow was executed once before in Phase 25 (v1.5 distribution). That phase established all conventions: remote name `plugin`, repo `MiloshXhavid/Dima_Plugin_Chrdmachine`, `.iss` field names, desktop backup path structure, and `gh` CLI patterns. Phase 30 repeats those patterns with only two changes: version numbers (1.5 → 1.6) and GitHub release designation (pre-release → Latest instead of pre-release).

The critical new element versus Phase 25 is the v1.5 promotion step. v1.5 currently shows as "Pre-release" on GitHub with v1.4 still holding "Latest". Phase 30 must flip v1.5 to a finished release and then publish v1.6 as Latest — a two-step release-state operation that Phase 25 did not need.

**Primary recommendation:** Execute in this order: promote v1.5 first, then build, then bump .iss, then create v1.6 release as Latest. Promoting v1.5 before publishing v1.6 avoids any gap where neither v1.5 nor v1.6 is a finished release.

---

## Standard Stack

### Core

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| `gh` CLI | installed | GitHub release create/edit/view | Used in Phase 25; authenticated to MiloshXhavid account |
| Inno Setup ISCC.exe | 6.7.1 | Compile `.iss` script into installer `.exe` | Used in Phase 25; confirmed at `C:\Program Files (x86)\Inno Setup 6\ISCC.exe` |
| CMake Release build | project version | Produce clean VST3 binary | Used in all prior distribution phases |
| git | project version | Create annotated tag, push to plugin remote | Used in Phase 25 with `plugin` remote |

### Supporting

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| PowerShell / bash `cp -r` | OS built-in | Copy .vst3 bundle to desktop | Desktop backup step |
| `git archive` | built-in | Create source-v1.6.zip at tag | Source archive in desktop backup |
| `mkdir -p` | OS built-in | Create versioned desktop backup folder | Before copying artifacts |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `gh release edit` to promote v1.5 | Manual GitHub web UI | CLI is reproducible and verifiable; web UI is acceptable fallback if CLI fails |
| `cp -r` for .vst3 bundle | `robocopy` | Either works on Windows under bash; `cp -r` is simpler and consistent with Phase 25 |

---

## Architecture Patterns

### Distribution Step Sequence

```
1. Promote v1.5 pre-release → finished release
2. Clean Release build (CMake)
3. Verify VST3 binary timestamp
4. Update installer/DimaChordJoystick-Setup.iss (AppVersion + AppComments only)
5. Recompile installer via ISCC.exe
6. Smoke test: run installer, verify in DAW
7. Create v1.6 annotated git tag at HEAD
8. Push v1.6 tag to plugin remote
9. Write release notes to temp file
10. gh release create v1.6 --latest (attaches installer .exe)
11. Verify v1.6 is Latest, v1.5 is finished release, v1.4 is historical
12. Create desktop backup folder (v1.6 or v1.6.1 if exists)
13. Copy installer .exe, .vst3 bundle, source-v1.6.zip to backup folder
14. Verify all three desktop artifacts exist
```

### Pattern 1: Promote Pre-Release to Finished Release

**What:** Change v1.5 from pre-release to finished release without altering its title, notes, or attached binary.
**When to use:** Before publishing a new Latest release — ensures v1.5 is a proper historical release, not pre-release.

```bash
# Source: gh CLI documentation
gh release edit v1.5 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --prerelease=false

# Verify
gh release view v1.5 --repo MiloshXhavid/Dima_Plugin_Chrdmachine --json isPrerelease,isLatest
# Expected: {"isPrerelease":false,"isLatest":false}
```

### Pattern 2: Publish Latest Release

**What:** Create v1.6 release and designate it as Latest (replacing v1.4).
**When to use:** v1.6 is stable and ready for public distribution.

```bash
# Source: gh CLI documentation
gh release create v1.6 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --title "DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6" \
  --latest \
  --notes-file "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/release-notes-v1.6.md" \
  "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe#DIMEA-ChordJoystickMK2-v1.6-Setup.exe"

# Verify
gh release list --repo MiloshXhavid/Dima_Plugin_Chrdmachine
# v1.6 should show "Latest", v1.5 should show no badge, v1.4 should show no badge
```

### Pattern 3: .iss Version Bump (Minimal Changes)

**What:** Only two fields change from the Phase 25 .iss. All structural elements (branding, [Messages], [Files] path, [UninstallDelete]) are preserved verbatim.

| Field | Current (v1.5) | New (v1.6) |
|-------|----------------|------------|
| `AppVersion` | `1.5` | `1.6` |
| `AppComments` | v1.5 feature list | v1.6 feature list |
| `OutputBaseFilename` | `DIMEA-ChordJoystickMK2-v1.5-Setup` | `DIMEA-ChordJoystickMK2-v1.6-Setup` |

Note: `OutputBaseFilename` must also be bumped (it controls the output filename), though CONTEXT.md only mentions AppVersion and AppComments — the filename must match `DIMEA-ChordJoystickMK2-v1.6-Setup.exe`.

### Pattern 4: Desktop Backup Folder Convention

```
Desktop/Dima_plug-in/
├── v1.4/        (existing — installer + VST3 + source zip)
├── v1.5/        (existing — installer + VST3 + source zip)
└── v1.6/        (new — create if not exists; create v1.6.1/ if v1.6/ already exists)
    ├── DIMEA-ChordJoystickMK2-v1.6-Setup.exe
    ├── DIMEA CHORD JOYSTICK MK2.vst3/    (full bundle directory from build output)
    └── source-v1.6.zip                    (git archive at v1.6 tag)
```

### Anti-Patterns to Avoid

- **Publishing v1.6 before promoting v1.5:** Would leave v1.5 as a pre-release forever, confusing users who see it alongside a new Latest release.
- **Using `--prerelease` flag on v1.6:** v1.6 is full stable; `--latest` is the correct flag (GitHub sets Latest automatically when `--latest` is passed or when it's the newest non-pre-release).
- **Copying .vst3 from system VST3 path instead of build output:** The build output at `build/ChordJoystick_artefacts/Release/VST3/` is the canonical source — system path may have an older version still installed.
- **Skipping the smoke test:** Always install the freshly compiled installer and verify in a DAW before pushing the GitHub release.
- **Tagging at a stale commit:** Ensure all Phase 26–29 commits (and Phase 30 doc commits) are in before tagging v1.6.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| GitHub release promotion | Manual GitHub web steps embedded in plan | `gh release edit --prerelease=false` | One-line, idempotent, verifiable with `gh release view` |
| Release notes formatting | Generating markdown dynamically in code | Write to temp `.md` file, pass via `--notes-file` | Avoids shell escaping nightmares with long multi-line strings |
| Asset upload | Separate `gh release upload` step | Pass asset directly to `gh release create` as positional arg | Single command, atomic |

---

## Common Pitfalls

### Pitfall 1: OutputBaseFilename Not Updated
**What goes wrong:** `.iss` has `AppVersion=1.6` but `OutputBaseFilename` still says `v1.5-Setup`. Installer compiles but outputs file named `DIMEA-ChordJoystickMK2-v1.5-Setup.exe` overwriting the v1.5 installer.
**Why it happens:** CONTEXT.md says "version bump only" for AppVersion + AppComments — easy to forget OutputBaseFilename is also version-bearing.
**How to avoid:** Update AppVersion, AppComments, AND OutputBaseFilename in the same edit. Verify output file name before proceeding.
**Warning signs:** After ISCC compilation, the output file is named v1.5-Setup instead of v1.6-Setup.

### Pitfall 2: v1.6 Tag Already Exists Locally from GSD Framework
**What goes wrong:** `git tag -a v1.6` fails with "tag already exists" because the GSD framework repo (origin) has tags like `v1.6.0`, `v1.6.1`, etc. — but `v1.6` (exact) may or may not already exist.
**Why it happens:** This repo's origin (get-shit-done framework) has many version tags that could collide. The plugin remote (`plugin`) is separate and clean.
**How to avoid:** Before tagging, check `git tag -l v1.6` to see if the exact tag exists locally. If it does, check whether it already points to the correct commit for the plugin (it likely doesn't — these are GSD framework tags). Use `git -C ... tag -a v1.6 ...` which scopes to the project. The `v1.6` tag does NOT appear in the tag list output — the framework has `v1.6.0`, `v1.6.1`, etc. but not bare `v1.6`. Safe to create.
**Warning signs:** `git tag -a v1.6` returns "fatal: tag 'v1.6' already exists".

### Pitfall 3: Wrong Repo in gh Commands
**What goes wrong:** `gh release create v1.6` without `--repo` publishes to the GSD framework repo (glittercowboy/get-shit-done) instead of the plugin repo.
**Why it happens:** `gh` CLI defaults to the repo of the git remote named `origin`, which is the GSD framework.
**How to avoid:** Always explicitly pass `--repo MiloshXhavid/Dima_Plugin_Chrdmachine` to every `gh release` command.
**Warning signs:** `gh release list` shows releases on glittercowboy/get-shit-done instead of MiloshXhavid/Dima_Plugin_Chrdmachine.

### Pitfall 4: Build Output VST3 Name Mismatch
**What goes wrong:** The PRODUCT_NAME changed to "Chord Joystick Control (BETA)" (see MEMORY.md). If the .iss [Files] Source still references "DIMEA CHORD JOYSTICK MK2.vst3", installer build fails because the file at that path doesn't exist.
**Why it happens:** Product name was renamed 2026-03-02. The .iss Source path must match the actual built VST3 folder name.
**How to avoid:** Before running ISCC, verify the exact VST3 bundle folder name in `build/ChordJoystick_artefacts/Release/VST3/`. Confirm it matches the Source path in the .iss.
**Warning signs:** ISCC returns error "Source file not found" or similar. Current .iss has `DIMEA CHORD JOYSTICK MK2.vst3` — verify this still matches the build output.

### Pitfall 5: Desktop v1.6 Folder Already Exists
**What goes wrong:** A `v1.6/` folder was created manually for testing, causing overwrite of existing artifacts.
**Why it happens:** Developer may have done a test run or partial backup earlier.
**How to avoid:** Always check `ls Desktop/Dima_plug-in/v1.6` before `mkdir`. If it exists, create `v1.6.1/` per CONTEXT.md.
**Warning signs:** `mkdir -p` succeeds silently but the folder already contained files.

### Pitfall 6: v1.5 Promotion Doesn't Remove Pre-Release Badge
**What goes wrong:** `gh release edit v1.5 --prerelease=false` appears to succeed but the GitHub UI still shows "Pre-release" due to caching or API delay.
**Why it happens:** Rare GitHub API caching; usually resolves within seconds.
**How to avoid:** After editing, run `gh release view v1.5 --repo ... --json isPrerelease` and confirm `false` before proceeding.

---

## Code Examples

### v1.5 Promotion

```bash
# Promote v1.5 from pre-release to finished release
gh release edit v1.5 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --prerelease=false

# Verify promotion
gh release view v1.5 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --json isPrerelease,isLatest,name
```

### Clean Build

```bash
cmake --build "C:/Users/Milosh Xhavid/get-shit-done/build" --config Release

# Verify binary exists and check timestamp
ls -la "C:/Users/Milosh Xhavid/get-shit-done/build/ChordJoystick_artefacts/Release/VST3/"
```

### ISCC Compilation

```bash
"C:/Program Files (x86)/Inno Setup 6/ISCC.exe" \
  "C:/Users/Milosh Xhavid/get-shit-done/installer/DimaChordJoystick-Setup.iss"

# Verify output name
ls -la "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe"
```

### Git Tag and Push

```bash
# Create annotated tag at HEAD
git -C "C:/Users/Milosh Xhavid/get-shit-done" tag -a v1.6 -m "v1.6 — Triplets & Fixes"

# Push to plugin remote only (not origin)
git -C "C:/Users/Milosh Xhavid/get-shit-done" push plugin v1.6

# Verify tag on remote
gh api repos/MiloshXhavid/Dima_Plugin_Chrdmachine/git/refs/tags/v1.6
```

### GitHub Release Create (v1.6 as Latest)

```bash
gh release create v1.6 \
  --repo MiloshXhavid/Dima_Plugin_Chrdmachine \
  --title "DIMEA PLUGIN - CHORD JOYSTICK MK2 v1.6" \
  --latest \
  --notes-file "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/release-notes-v1.6.md" \
  "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe#DIMEA-ChordJoystickMK2-v1.6-Setup.exe"

# Verify release state
gh release list --repo MiloshXhavid/Dima_Plugin_Chrdmachine
gh release view v1.6 --repo MiloshXhavid/Dima_Plugin_Chrdmachine --json title,isPrerelease,isLatest,assets
```

### Desktop Backup

```bash
# Check if v1.6 folder exists
ls "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6" 2>/dev/null && echo EXISTS || echo NOT_EXISTS

# Create folder (adjust to v1.6.1 if EXISTS)
mkdir -p "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6"

# Copy installer
cp "C:/Users/Milosh Xhavid/get-shit-done/installer/Output/DIMEA-ChordJoystickMK2-v1.6-Setup.exe" \
   "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/"

# Copy VST3 bundle from build output (not system path)
cp -r "C:/Users/Milosh Xhavid/get-shit-done/build/ChordJoystick_artefacts/Release/VST3/DIMEA CHORD JOYSTICK MK2.vst3" \
      "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/"

# Create source archive at v1.6 tag
git -C "C:/Users/Milosh Xhavid/get-shit-done" archive \
  --format=zip \
  --prefix=ChordJoystick-v1.6/ \
  v1.6 \
  -o "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/source-v1.6.zip"

# Verify all three artifacts
ls -la "C:/Users/Milosh Xhavid/Desktop/Dima_plug-in/v1.6/"
```

---

## State of the Art

| Phase 25 Approach | Phase 30 Approach | Change | Impact |
|-------------------|-------------------|--------|--------|
| `--prerelease` flag on `gh release create` | `--latest` flag (no `--prerelease`) | v1.6 is stable | v1.6 becomes Latest; v1.4 loses Latest |
| No v1.x promotion needed | `gh release edit v1.5 --prerelease=false` first | v1.5 was pre-release | v1.5 becomes historical finished release |
| AppName, AppPublisher, AppPublisherURL all changed | Only AppVersion + AppComments + OutputBaseFilename change | Branding already correct | Minimal .iss diff |

**Current GitHub release state (confirmed live):**
- v1.5: Pre-release (published 2026-03-02)
- v1.4: Latest (published 2026-02-26)
- v1.3, v1.2, v1.1, v1.0: historical

**Target state after Phase 30:**
- v1.6: Latest
- v1.5: Finished release (no badge)
- v1.4: Historical (no badge)

---

## Open Questions

1. **VST3 bundle folder name after product rename**
   - What we know: PRODUCT_NAME changed to `"Chord Joystick Control (BETA)"` on 2026-03-02 (per MEMORY.md). Current `.iss` Source references `DIMEA CHORD JOYSTICK MK2.vst3`.
   - What's unclear: Whether the actual built VST3 folder in `build/ChordJoystick_artefacts/Release/VST3/` is named per the old or new PRODUCT_NAME. The CMakeLists.txt PRODUCT_NAME determines the folder name. Need to verify actual folder name at build time.
   - Recommendation: The plan must include a step to `ls` the VST3 output directory BEFORE running ISCC, and confirm the Source path in .iss matches. If the folder name changed, update the .iss Source path accordingly. (Confidence: MEDIUM — confirmed rename happened, actual folder name at HEAD unknown until build runs)

2. **Whether `--latest` is the correct flag for gh release create**
   - What we know: `--prerelease` is documented; `--latest` was added in gh CLI 2.x.
   - What's unclear: Exact flag behavior — `--latest` may be implicit when creating a non-pre-release, or may require explicit flag.
   - Recommendation: Use both `--latest` explicitly and omit `--prerelease`. Alternatively, after `gh release create` (without flags), run `gh release edit v1.6 --latest` to explicitly set Latest. The fallback `gh release edit` approach is safe regardless.

---

## Sources

### Primary (HIGH confidence)
- Phase 25 plans (25-01-PLAN.md, 25-02-PLAN.md) — verified working workflow, all commands proven
- Live GitHub release list (`gh release list --repo MiloshXhavid/Dima_Plugin_Chrdmachine`) — confirmed current release state
- `installer/DimaChordJoystick-Setup.iss` — current file content read directly
- `30-CONTEXT.md` — locked user decisions
- MEMORY.md — product name change, known gotchas

### Secondary (MEDIUM confidence)
- gh CLI `release edit` with `--prerelease=false` — documented behavior, used widely; exact flag verified from `gh release edit --help` pattern matching Phase 25 CONTEXT.md reference
- Desktop backup path — confirmed v1.4 and v1.5 folders exist at `Desktop/Dima_plug-in/`

### Tertiary (LOW confidence)
- VST3 bundle folder name after product rename — inferred from MEMORY.md note; requires runtime verification

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all tools proven in Phase 25
- Architecture: HIGH — exact same workflow, two additions (v1.5 promotion, --latest flag)
- Pitfalls: HIGH — most identified from prior distribution experience; Pitfall 4 (VST3 name) is MEDIUM
- Open questions: MEDIUM — VST3 folder name and --latest flag behavior require runtime checks

**Research date:** 2026-03-03
**Valid until:** 2026-03-10 (GitHub release workflow is stable; gh CLI flags may change on major version bumps)
