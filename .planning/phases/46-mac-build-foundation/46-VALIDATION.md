---
phase: 46
slug: mac-build-foundation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-11
---

# Phase 46 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CLI smoke commands (cmake, lipo, auval) — no Catch2 unit tests for build/infra work |
| **Config file** | CMakeLists.txt |
| **Quick run command** | `cmake -G Xcode -B build-mac -S . 2>&1 \| tail -5` |
| **Full suite command** | `lipo -info <vst3-binary> && lipo -info <au-binary> && auval -v aumu DCJM MxCJ` |
| **Estimated runtime** | ~2 minutes (cmake configure) + ~20 min (Release build) |

---

## Sampling Rate

- **After every task commit:** Run `cmake -G Xcode -B build-mac -S . 2>&1 | tail -5` (confirm configure succeeds)
- **After every plan wave:** Run `lipo -info` on both binaries + `auval -v aumu DCJM MxCJ`
- **Before `/gsd:verify-work`:** All CLI validations pass + manual DAW smoke tests documented
- **Max feedback latency:** ~2 seconds for cmake exit code check

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 46-01-01 | 01 | 1 | MAC-05 | smoke | `cmake -G Xcode -B build-mac -S . 2>&1; echo "Exit: $?"` | ✅ CMakeLists.txt | ⬜ pending |
| 46-01-02 | 01 | 1 | MAC-01 | smoke | `cmake -G Xcode -B build-mac -S . -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" 2>&1; echo "Exit: $?"` | ✅ CMakeLists.txt | ⬜ pending |
| 46-02-01 | 02 | 2 | MAC-01 | smoke | `xcodebuild -project build-mac/ChordJoystick.xcodeproj -scheme ChordJoystick_All -configuration Release ONLY_ACTIVE_ARCH=NO 2>&1 \| tail -10` | ❌ W0 — build artifact | ⬜ pending |
| 46-02-02 | 02 | 2 | MAC-01 | smoke | `lipo -info "build-mac/ChordJoystick_artefacts/Release/VST3/Arcade Chord Control (BETA-Test).vst3/Contents/MacOS/Arcade Chord Control (BETA-Test)"` | ❌ W0 — build artifact | ⬜ pending |
| 46-02-03 | 02 | 2 | MAC-01 | smoke | `lipo -info "$HOME/Library/Audio/Plug-Ins/Components/Arcade Chord Control (BETA-Test).component/Contents/MacOS/Arcade Chord Control (BETA-Test)"` | ❌ W0 — build artifact | ⬜ pending |
| 46-03-01 | 03 | 2 | MAC-02 | smoke | `killall -9 AudioComponentRegistrar 2>/dev/null; sleep 2; auval -a \| grep -i DCJM` | ❌ W0 — AU must be registered | ⬜ pending |
| 46-03-02 | 03 | 2 | MAC-02 | smoke | `auval -v aumu DCJM MxCJ; echo "Exit: $?"` | ❌ W0 — AU must be registered | ⬜ pending |
| 46-04-01 | 04 | 3 | MAC-03 | manual | N/A — requires DAW UI interaction | ❌ manual-only | ⬜ pending |
| 46-04-02 | 04 | 3 | MAC-04 | manual | N/A — requires physical USB controller + UI | ❌ manual-only | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

No new test files needed — this phase has no Catch2-testable logic. All validations are CLI smoke commands or manual DAW tests run against build artifacts produced during the phase.

*Existing Catch2 infrastructure covers pre-existing unit-testable logic (ScaleQuantizer, ChordEngine, LooperEngine). Phase 46 is pure build/configuration work.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Logic Pro (AU) loads plugin in Software Instrument slot without error, UI visible | MAC-03 | kAudioUnitType_MusicDevice cannot route MIDI output through Logic's MIDI monitor — architectural limitation | Open Logic, add Software Instrument track, insert "Arcade Chord Control (BETA-Test)" AU, confirm UI appears and no crash occurs |
| Reaper (VST3) emits MIDI output to MIDI monitor | MAC-03 | Requires Reaper + MIDI monitor UI, physical playback | Open Reaper, insert plugin as VST3 instrument, enable MIDI output, play notes, confirm chord MIDI output in Reaper MIDI monitor |
| Ableton Live (VST3) emits MIDI output | MAC-03 | Requires Ableton + MIDI routing setup | Open Ableton, insert plugin on MIDI track, play notes, confirm chord MIDI output visible |
| PS4 or Xbox controller detected (SDL_NumJoysticks() > 0) | MAC-04 | Requires physical USB controller + plugin UI inspection | Connect PS4 or Xbox controller via USB, load plugin in any DAW, confirm controller type label visible in plugin UI |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
