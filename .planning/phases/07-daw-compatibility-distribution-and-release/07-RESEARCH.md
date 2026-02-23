# Phase 07: DAW Compatibility, Distribution, and Release - Research

**Researched:** 2026-02-23
**Domain:** VST3 plugin validation (pluginval), Windows installer (Inno Setup 6), SmartScreen / code-signing
**Confidence:** MEDIUM-HIGH (pluginval mechanics well-documented; Inno Setup patterns confirmed via official docs; SmartScreen behaviour empirically described by community)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Code Signing**
- No certificate yet — ship unsigned for v1; SmartScreen warning is acceptable short-term
- Structure the build + installer so signing can be retrofitted in one step (signing script placeholder)
- When signing is added later: sign both the VST3 DLL and the installer EXE
- Mac: no Apple Developer account yet — ship unnotarized; Gatekeeper warning acceptable for v1

**pluginval (Windows)**
- Target: strictness 5 preferred; level 4 acceptable if level 5 reveals truly obscure edge cases
- Policy: fix everything pluginval reports — zero tolerance, no known-benign exceptions
- Not installed yet — plan must include download + setup steps
- Run against installed path (`%COMMONPROGRAMFILES%\VST3\ChordJoystick.vst3`) — production-like validation

**Windows Installer (Inno Setup)**
- Name: `DimaChordJoystick-Setup.exe`
- Version: 1.0.0
- Install path: offer both system-wide (`%COMMONPROGRAMFILES%\VST3\`) and per-user during install wizard; system-wide as default
- Uninstaller: yes — registered in Windows Add/Remove Programs
- Wizard pages: standard 4-5 page flow: welcome, license agreement, install path, install progress, done
- Post-install: success dialog only — no browser launch, no README popup
- Signing: placeholder `signtool.exe` call commented out, ready to activate when cert acquired

**Distribution**
- Scope: artifact only — produce `DimaChordJoystick-Setup.exe` ready to upload
- Gumroad product listing: manual — out of scope for Phase 07
- Clean install test: required before release — must be tested on a Windows machine without dev tools

### Claude's Discretion
- Exact Inno Setup script structure and section ordering

### Deferred Ideas (OUT OF SCOPE)
- macOS support — explicitly v2
- Windows code signing — deferred post-v1 (infrastructure placeholder only)
</user_constraints>

---

## Summary

Phase 07 has three distinct workstreams: (1) validate the VST3 with pluginval, (2) build an Inno Setup installer, and (3) test the full chain on a clean machine. The Ableton routing concern from the original phase description was already resolved in pre-07 commit `41d75db` — the plugin loads correctly as a MIDI effect in Ableton 11.3.43. No further DAW-routing investigation is needed for v1.

**pluginval** is the industry-standard JUCE plugin validator (Tracktion-maintained, v1.0.5 as of late 2025). At strictness level 5, pluginval additionally runs Steinberg's official VST3 SDK validator binary — this is the most significant difference between levels 4 and 5. Because ChordJoystick is a MIDI effect (`isMidiEffect=true`, zero audio buses), the audio buffer presented to `processBlock` will have zero samples and zero channels. pluginval's own tests handle this gracefully; the primary risk is that the embedded VST3 validator may produce warnings about missing audio I/O. Community evidence suggests JUCE 8.0.x MIDI effect configuration passes the VST3 validator cleanly provided the CMake flags are correctly set (which they already are: `IS_MIDI_EFFECT=TRUE`, `NEEDS_MIDI_INPUT=TRUE`, `NEEDS_MIDI_OUTPUT=TRUE`, `VST3_CATEGORIES="Fx" "MIDI Effect"`, empty `BusesProperties()`). The known pitfall is that `processBlock` may be called with a zero-sample buffer — the plugin must not crash or assert in this condition.

**Inno Setup 6.7.1** (released 2026-02-17) is the current standard for free Windows installers. The Inno Setup script format is well-understood, with `{commoncf64}\VST3` as the VST3 system-wide path and `{autocf64}\VST3` for an auto-switching per-user/system path. The VST3 format requires copying the entire `.vst3` bundle (which is a directory on disk) recursively. Inno Setup handles this with the `Flags: recursesubdirs` or by sourcing `ChordJoystick.vst3\*`. The `PrivilegesRequired=admin` + `PrivilegesRequiredOverridesAllowed=dialog` combination gives the user the per-user/system-wide choice at the first wizard screen.

**SmartScreen** will show an "Unknown Publisher" warning for unsigned installers on Windows 11. Users can bypass it via "More info → Run anyway". This is acceptable per the user's decisions. No code-signing infrastructure is needed for v1, only a commented-out `signtool.exe` placeholder.

**Primary recommendation:** Validate with `pluginval.exe --validate-in-process --strictness-level 5 "C:\Program Files\Common Files\VST3\ChordJoystick.vst3"` (no trailing backslash — trailing slash is a known pluginval bug fixed in v1.0.4+). Fix any reported issues. Then author the Inno Setup script, produce the EXE, and test on a clean machine.

---

## Standard Stack

### Core

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| pluginval | v1.0.5 (latest) | VST3 plugin validation before distribution | Industry standard for JUCE plugins; required for "Verified by pluginval" badge; runs embedded VST3 SDK validator at strictness 5+ |
| Inno Setup | 6.7.1 (2026-02-17) | Windows installer authoring | Free, widely used, native Windows Add/Remove Programs integration, simple Pascal-like scripting |

### Supporting

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| Chocolatey | current | Alternative pluginval install | `choco install pluginval` — convenient if choco already available |
| ISCC.exe | (bundled with Inno Setup) | Command-line script compiler | Allows building the installer EXE from a `.iss` script file without opening the GUI |
| signtool.exe | (bundled with Windows SDK) | Code signing | Out of scope for v1; placeholder only |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Inno Setup | WiX (MSI) | WiX produces .msi — natively trusted format, but steep learning curve and XML verbosity; Inno Setup is faster to write for single-format plugin installers |
| Inno Setup | NSIS | NSIS is scriptable but older; community less active; Inno Setup 6 has better Unicode and privilege model |
| pluginval | VST3 SDK validator (standalone) | VST3 validator tests conformity only; pluginval tests JUCE-specific behaviour and is the de-facto community standard |

---

## Architecture Patterns

### VST3 Bundle Structure on Windows

```
ChordJoystick.vst3\          <- root directory (the "bundle")
  Contents\
    x86_64-win\
      ChordJoystick.vst3     <- actual PE DLL
    moduleinfo.json          <- VST3 manifest (category, sub-categories)
    Resources\               <- optional
```

The entire `ChordJoystick.vst3\` directory tree must be copied to `%COMMONPROGRAMFILES%\VST3\`.

### Inno Setup Script Pattern for VST3

```iss
[Setup]
AppName=ChordJoystick
AppVersion=1.0.0
AppPublisher=Dima Xhavid
AppPublisherURL=https://your-gumroad-page
OutputBaseFilename=DimaChordJoystick-Setup
DefaultDirName={commoncf64}\VST3
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
DisableDirPage=yes
UninstallDisplayName=ChordJoystick VST3

[Files]
; Copy the entire .vst3 bundle recursively
Source: "build\ChordJoystick_artefacts\Release\VST3\ChordJoystick.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ChordJoystick.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; No start menu or desktop shortcuts needed for VST3

[Code]
// Placeholder for future signtool.exe signing step
// procedure CurStepChanged(CurStep: TSetupStep);
// begin
//   if CurStep = ssPostInstall then begin
//     // Exec('signtool.exe', 'sign /a /t http://timestamp.digicert.com "' +
//     //   ExpandConstant('{commoncf64}\VST3\ChordJoystick.vst3\Contents\x86_64-win\ChordJoystick.vst3"'), ...)
//   end;
// end;
```

**Key directives:**
- `PrivilegesRequired=admin` — requests UAC elevation (required for `%COMMONPROGRAMFILES%`)
- `PrivilegesRequiredOverridesAllowed=dialog` — shows a dialog letting user choose per-user install (maps to `{localappdata}\Programs\` area); when non-admin install is selected, `{commoncf64}` automatically remaps to `{usercf}` if `{autocf64}` is used instead
- `DisableDirPage=yes` — VST3 path is standardised; no need for user to browse
- `Flags: recursesubdirs createallsubdirs` — copies the entire `.vst3` bundle tree correctly

### Pattern: System-Wide vs Per-User Install Choice

Use `{autocf64}` instead of `{commoncf64}` if you want automatic path remapping:
- Admin mode: `{autocf64}` → `C:\Program Files\Common Files`
- Non-admin mode: `{autocf64}` → `C:\Users\<name>\AppData\Local\Programs` (or similar `{usercf}` path)

The `PrivilegesRequiredOverridesAllowed=dialog` directive displays a dialog before the wizard pages that lets the user choose "Install for all users (requires admin)" or "Install for me only".

### Pattern: pluginval Command Line (Windows)

```cmd
:: From cmd.exe (NOT PowerShell — pluginval has issues with PowerShell)
pluginval.exe --validate-in-process --strictness-level 5 "C:\Program Files\Common Files\VST3\ChordJoystick.vst3"

:: Check result
if %ERRORLEVEL% neq 0 (
    echo VALIDATION FAILED
    exit /b 1
)
```

**Critical:** No trailing backslash on the `.vst3` path. pluginval has a known bug (fixed in v1.0.4+ via path stripping) where trailing slashes cause "No types found" failure.

**Download script (PowerShell, then run validator from cmd):**
```powershell
# Download
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip -OutFile pluginval.zip

# Extract
Expand-Archive pluginval.zip -DestinationPath .
```

### Anti-Patterns to Avoid

- **Trailing backslash on .vst3 path:** `ChordJoystick.vst3\` instead of `ChordJoystick.vst3` — causes "No types found" in pluginval
- **Running pluginval from PowerShell:** pluginval has documented compatibility issues with PowerShell; use `cmd.exe`
- **Validating the build artefact path:** CONTEXT.md explicitly requires validating the installed path (`%COMMONPROGRAMFILES%\VST3\...`), not the build output
- **Using `{cf}` or `{cf64}` in Inno Setup:** These are deprecated; use `{commoncf64}` or `{autocf64}`
- **Forgetting `recursesubdirs createallsubdirs` flags:** Without these, only the DLL is copied; the `Contents\` directory structure is missing and DAWs cannot load the plugin
- **Sourcing the `.vst3` directory itself (not its contents):** Inno Setup `Source` must end in `\*` to copy the bundle's contents into a `DestDir` named `ChordJoystick.vst3`

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plugin validation | Custom test harness | pluginval | Tests DAW-like loading, parameter fuzzing, state save/restore — not trivial to replicate |
| Windows installer | PowerShell copy script | Inno Setup | Add/Remove Programs registration, uninstaller, UAC elevation, wizard UI, path constants — all built-in |
| VST3 conformity | Custom tests | pluginval --strictness-level 5 (embeds VST3 SDK validator) | Official Steinberg validator is bundled in pluginval v1.0.0+ |

**Key insight:** The VST3 SDK validator is embedded in pluginval v1.0.0+ and runs automatically at strictness level 5. There is no need to separately build or install the Steinberg VST3 SDK validator.

---

## Common Pitfalls

### Pitfall 1: pluginval Run Against Build Artefact (Not Installed Path)

**What goes wrong:** Developer validates `build\ChordJoystick_artefacts\Release\VST3\ChordJoystick.vst3` and passes, but ships a broken installer that places files incorrectly.
**Why it happens:** Convenience — build path is known; installed path requires running the installer first.
**How to avoid:** Install via `DimaChordJoystick-Setup.exe` first, then validate `"C:\Program Files\Common Files\VST3\ChordJoystick.vst3"`. The CONTEXT.md explicitly requires this.
**Warning signs:** If you run pluginval and get "No types found", you may be pointing at the wrong path (or have a trailing slash issue).

### Pitfall 2: Zero-Sample processBlock Crash

**What goes wrong:** pluginval calls `processBlock` with an `AudioBuffer<float>` that has 0 channels and 0 samples (MIDI effect behaviour). Any code that accesses `buffer.getReadPointer(0)` or calls `buffer.getNumSamples()` and uses the result in division will crash or assert.
**Why it happens:** `isMidiEffect()=true` causes JUCE to give the plugin an empty audio buffer, but pluginval still calls `processBlock` for timing/MIDI delivery.
**How to avoid:** The existing `processBlock` code in ChordJoystick should already handle this — it does not index into the audio buffer. Verify by code-reading `PluginProcessor::processBlock` for any `buffer.getReadPointer` or `buffer.getNumSamples()` usage before running pluginval.
**Warning signs:** pluginval crash/assertion inside `processBlock`.

### Pitfall 3: Inno Setup Source Path Wildcard Error

**What goes wrong:** `Source: "ChordJoystick.vst3"` copies only the directory node; `DestDir` receives a folder named `ChordJoystick.vst3` but with wrong internal structure.
**Why it happens:** Inno Setup requires `\*` suffix on directory sources to recurse their contents.
**How to avoid:** Use `Source: "...\ChordJoystick.vst3\*"` with `Flags: recursesubdirs createallsubdirs` and `DestDir: "{commoncf64}\VST3\ChordJoystick.vst3"`.
**Warning signs:** Plugin installs but DAW cannot load it; `moduleinfo.json` or DLL missing from expected path.

### Pitfall 4: SmartScreen Blocks Unsigned Installer on Clean VM

**What goes wrong:** The test machine shows "Windows protected your PC" and the installer cannot be run without clicking "More info → Run anyway". Test pass/fail gets confused because the tester doesn't know this step is required.
**Why it happens:** No code signing certificate; SmartScreen flags all unsigned executables from unrecognised publishers.
**How to avoid:** Document the bypass procedure in test notes. The user right-clicks the installer → Properties → Unblock checkbox; or clicks "More info" then "Run anyway" in the SmartScreen prompt. This is explicitly accepted in the CONTEXT.md.
**Warning signs:** Tester reports "installer won't open" — they need the bypass instructions.

### Pitfall 5: pluginval Parameter Listener Dangling Reference

**What goes wrong:** pluginval at higher strictness levels destroys and recreates the editor multiple times. If `PluginEditor` registers a parameter listener and doesn't call `removeListener` in its destructor, pluginval crashes with a use-after-free.
**Why it happens:** JUCE parameter listeners are raw pointers; editors frequently forget to deregister.
**How to avoid:** Audit `PluginEditor` destructor — ensure every `addListener` call has a matching `removeListener`. Common pattern: `apvts.getParameter("paramId")->removeListener(this)` in destructor.
**Warning signs:** pluginval crash deep inside JUCE parameter notification code at strictness 5+.

### Pitfall 6: ISCC.exe Not on PATH

**What goes wrong:** Running `ISCC.exe script.iss` from cmd fails with "command not found" even though Inno Setup is installed.
**Why it happens:** Inno Setup installer doesn't add ISCC to PATH by default.
**How to avoid:** Use full path: `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" script.iss`
**Warning signs:** "ISCC is not recognised as an internal or external command."

### Pitfall 7: pluginval VST3 Validator Missing (Rare Edge Case)

**What goes wrong:** At strictness level 5, pluginval runs the embedded VST3 validator binary. On rare system configurations the validator subprocess fails silently.
**Why it happens:** pluginval bundles the VST3 validator internally (not a separate SDK installation required); however, runtime DLL dependencies or antivirus interference can block the subprocess.
**How to avoid:** Run with `--validate-in-process` flag. If level 5 still has unexplained failures not reported by level 4, fall back to level 4 per the CONTEXT.md allowance.
**Warning signs:** Level 5 fails with no clear JUCE test name in the output, or fails intermittently.

---

## Code Examples

### Minimal Inno Setup Script for ChordJoystick

```iss
; DimaChordJoystick-Setup.iss
; Place this file at project root or installer/ subfolder

[Setup]
AppName=ChordJoystick
AppVersion=1.0.0
AppPublisher=Dima Xhavid
OutputBaseFilename=DimaChordJoystick-Setup
DefaultDirName={commoncf64}\VST3\ChordJoystick.vst3
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
DisableDirPage=yes
Compression=lzma
SolidCompression=yes
; LicenseFile=LICENSE.txt    ; uncomment if you have a license file

[Files]
; Copies the full .vst3 bundle (directory tree) to the standard VST3 path
Source: "build\ChordJoystick_artefacts\Release\VST3\ChordJoystick.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ChordJoystick.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\ChordJoystick.vst3"

; ── Code-signing placeholder ───────────────────────────────────────────────
; Uncomment and fill in when a certificate is acquired:
; [Code]
; procedure CurStepChanged(CurStep: TSetupStep);
; var ResultCode: Integer;
; begin
;   if CurStep = ssPostInstall then begin
;     Exec('signtool.exe',
;       'sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a ' +
;       '"' + ExpandConstant('{commoncf64}\VST3\ChordJoystick.vst3\Contents\x86_64-win\ChordJoystick.vst3"'),
;       '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
;   end;
; end;
```

### Build the installer from command line

```cmd
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\DimaChordJoystick-Setup.iss
:: Output: installer\Output\DimaChordJoystick-Setup.exe
```

### pluginval validation (cmd.exe)

```cmd
:: Step 1: Download (PowerShell, one-time)
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip -OutFile pluginval.zip"
powershell -Command "Expand-Archive pluginval.zip -DestinationPath ."

:: Step 2: Validate from cmd.exe (NOT PowerShell)
pluginval.exe --validate-in-process --strictness-level 5 "C:\Program Files\Common Files\VST3\ChordJoystick.vst3"

:: Check exit code
if %ERRORLEVEL% neq 0 (
    echo PLUGINVAL FAILED — check output above
    exit /b 1
) else (
    echo ALL TESTS PASSED
)
```

### Verify VST3 version string is consistent (CMakeLists.txt already has this)

```cmake
project(ChordJoystick VERSION 1.0.0)  ; already set
```

The JUCE macro `JUCE_APPLICATION_VERSION_STRING` is derived from `CMake project VERSION` — no separate version string needed.

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| pluginval 0.2.x (tests only) | pluginval 1.0.x (embeds VST3 SDK validator at level 5+) | pluginval v1.0.0 | Level 5 now also runs Steinberg's official validator — failures may differ from 0.2.x runs |
| EV certificate required for instant SmartScreen bypass | Trusted Signing (Microsoft managed) also available | March 2024 | Indie developers have cheaper option for SmartScreen bypass, but it's out of scope for v1 |
| `{cf64}` Inno Setup constant | `{commoncf64}` or `{autocf64}` | Inno Setup 6 | Old names deprecated but still functional; use new names |
| PrivilegesRequired=admin only | PrivilegesRequired + PrivilegesRequiredOverridesAllowed | Inno Setup 6 | Users can now choose per-user install without separate script |

**Deprecated/outdated:**
- Inno Setup constants `{cf}`, `{cf32}`, `{cf64}`: still work but deprecated — use `{commoncf}`, `{commoncf32}`, `{commoncf64}`
- pluginval 0.2.x: superseded; 1.0.5 is current; download from `releases/latest`

---

## Open Questions

1. **Does pluginval's embedded VST3 validator pass for isMidiEffect=true plugins on Windows?**
   - What we know: pluginval's own tests handle zero-sample MIDI effect buffers. The VST3 SDK validator runs as a subprocess at level 5.
   - What's unclear: Whether the VST3 SDK validator raises a warning about missing audio buses for a MIDI-only plugin with empty `BusesProperties()`. Community reports suggest JUCE 8 MIDI effect configuration passes cleanly, but no definitive evidence was found.
   - Recommendation: Run pluginval at level 5 first. If the VST3 validator fails with audio-bus-related errors, fall back to level 4 per the CONTEXT.md policy. The fix (if needed) would be to add an optional inactive stereo bus back — but this was already working per the pre-07 Ableton fix commit.

2. **Exact per-user VST3 install path when non-admin**
   - What we know: `{autocf64}` in Inno Setup auto-maps to `{usercf}` in non-admin mode.
   - What's unclear: Whether `{usercf}` on Windows 11 maps to a location that DAWs (Ableton, Reaper) scan by default.
   - Recommendation: For v1, use `{commoncf64}` (admin/system-wide only) as the default with `PrivilegesRequiredOverridesAllowed=dialog`. Document in release notes that per-user install may not be scanned by all DAWs.

3. **Inno Setup Windows Defender false positives**
   - What we know: Inno Setup EXEs are occasionally flagged because malware authors use Inno Setup. Signing is the definitive fix.
   - What's unclear: Whether an unsigned Inno Setup 6.7.1 EXE will be flagged on a clean Windows 11 VM in 2026.
   - Recommendation: Test on clean VM before releasing. If flagged, submit to Microsoft malware analysis (free) at https://www.microsoft.com/en-us/wdsi/filesubmission. Keep the SmartScreen "More info → Run anyway" bypass documented in the release page instructions.

---

## Sources

### Primary (HIGH confidence)
- [pluginval GitHub README](https://github.com/Tracktion/pluginval/blob/master/README.md) — CLI usage, strictness levels
- [pluginval CHANGELIST.md](https://github.com/Tracktion/pluginval/blob/develop/CHANGELIST.md) — confirmed v1.0.0 added VST3 SDK validator at level 5+; v1.0.5 current; Windows static runtime link
- [pluginval Adding to CI docs](https://github.com/Tracktion/pluginval/blob/develop/docs/Adding%20pluginval%20to%20CI.md) — exact Windows download + validation script
- [pluginval trailing slash issue #20](https://github.com/Tracktion/pluginval/issues/20) — confirmed no trailing slash on .vst3 path
- [Inno Setup constants docs](https://jrsoftware.org/ishelp/topic_consts.htm) — `{commoncf64}`, `{autocf64}`, `{usercf}` definitions
- [Inno Setup PrivilegesRequiredOverridesAllowed](https://jrsoftware.org/is6help/topic_setup_privilegesrequiredoverridesallowed.htm) — dialog mode for per-user/system choice
- [Inno Setup 6.7.1 release](https://jrsoftware.org/isdl.php) — current version confirmed 2026-02-17

### Secondary (MEDIUM confidence)
- [JUCE MIDI effect zero-sample buffer thread](https://forum.juce.com/t/buffer-size-is-zero-in-processblock-for-midi-effect-in/31131) — confirmed zero channels/samples in MIDI effect processBlock; cache samplesPerBlock from prepareToPlay
- [JUCE packaging tutorial](https://juce.com/tutorials/tutorial_app_plugin_packaging/) — VST3 path as `%COMMONPROGRAMFILES%\VST3`, system-wide recommendation
- [HISE Forum Inno Setup template](https://forum.hise.audio/topic/5617/inno-setup-template-for-multiple-locations-and-user-destination/4) — VST3 uses `{cf64}\VST3` (deprecated form); `recursesubdirs` pattern confirmed
- [Advanced Installer SmartScreen article](https://www.advancedinstaller.com/prevent-smartscreen-from-appearing.html) — "More info → Run anyway" bypass; Trusted Signing as alternative to EV cert
- [melatonin.dev pluginval blog](https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/) — parameter listener dangling reference as common failure mode; CI integration patterns
- [JUCE Inno Setup Defender thread](https://forum.juce.com/t/proactive-measures-to-prevent-inno-setup-installer-from-getting-flagged-by-windows-defender/65217) — signing both payload and installer is most effective mitigation; MSI as alternative

### Tertiary (LOW confidence)
- Community reports that JUCE 8 MIDI effect with empty BusesProperties passes pluginval level 5 — not directly verified; treat as hypothesis until the run is done

---

## Metadata

**Confidence breakdown:**
- pluginval mechanics: HIGH — official docs, changelog, CI guide all consistent
- Inno Setup script patterns: HIGH — official docs + multiple community examples converge
- MIDI effect + pluginval level 5 compatibility: LOW-MEDIUM — no definitive pass/fail report found; community inference only
- SmartScreen behaviour: MEDIUM — well-documented; empirically described by multiple sources
- Per-user VST3 DAW scan coverage: LOW — unclear if `{usercf}` path is scanned by Ableton/Reaper

**Research date:** 2026-02-23
**Valid until:** 2026-09-01 (stable tooling; pluginval release cycle is slow)
