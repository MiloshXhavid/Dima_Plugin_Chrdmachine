# ChordJoystick — Roadmap

**Milestone:** v1.0 — Paid Windows VST3 Release

---

## Phase 01: Build Foundation and JUCE Version Lock

**Goal:** Get the plugin compiling and loading in a DAW with pinned dependencies and correct build configuration.

**Delivers:**
- Reproducible build: JUCE pinned to `8.0.4` (not `origin/master`)
- Plugin loads in Reaper without crashes
- All APVTS parameters save/restore correctly
- Static CRT configured for distribution (no MSVC runtime dependency)
- `COPY_PLUGIN_AFTER_BUILD` set for dev builds

**Status:** COMPLETE (2/2 plans done — Ableton crash fixed in 05-03, all criteria met)

Plans:
- [x] 01-01: Fix CMakeLists.txt (pin JUCE tag, static CRT, validate plugin target) — DONE: build verified, VST3 installed
- [x] 01-02: Smoke test — plugin loads in Ableton, APVTS round-trip verified — DONE: retroactively resolved (Ableton crash fixed 05-03, verified 06-03)

---

## Phase 02: Core Engine Validation

**Goal:** Verify ChordEngine and ScaleQuantizer produce correct output via unit tests before wiring into the audio thread.

**Delivers:**
- Catch2 test suite: scale quantization (all presets, edge cases, tie-breaking), ChordEngine pitch computation across all 4 voices, custom 12-key scale entry
- All tests passing in CI/build

**Status:** COMPLETE (2/2 plans done — checkpoint approved)

**Plans:** 2/2 plans complete

Plans:
- [x] 02-01-PLAN.md — Add Catch2 v3.8.1 FetchContent + ChordJoystickTests CMake target + ScaleQuantizer test suite — DONE: 218 assertions green
- [x] 02-02-PLAN.md — ChordEngine test suite (axis routing, transpose, octave offsets, clamping, scale quantization) — DONE: 9 TEST_CASEs, 15 combined tests, 0 failures

---

## Phase 03: Core MIDI Output and Note-Off Guarantee

**Goal:** Wire TriggerSystem into processBlock to produce sample-accurate MIDI note-on/off. Guarantee note-off on all exit paths.

**Delivers:**
- All 4 voices producing MIDI note-on/off via TouchPlate triggers
- Note-off flushed on `releaseResources()`, transport stop, and bypass
- Per-voice MIDI channel routing verified
- Visual gate LED feedback in PluginEditor updating at 30 Hz

**Status:** in-progress (1/2 plans done)

**Plans:** 2/2 plans complete

Plans:
- [x] 03-01-PLAN.md — Note-off guarantee and LED fix (TriggerSystem::resetAllGates, processBlockBypassed, releaseResources, green gate LED, channel conflict warning) — DONE: build clean, 15/15 tests pass
- [x] 03-02-PLAN.md — Build + DAW verification in Reaper (6-test structured verification, checkpoint) — DONE: all 6 Reaper tests passed

---

## Phase 04: Per-Voice Trigger Sources and Random Gate

**Goal:** Complete the trigger source model — add joystick-motion and random subdivision sources on top of the working TouchPlate trigger.

**Delivers:**
- Per-voice trigger source selector (TouchPlate / Joystick Motion / Random) fully functional
- Random gate: DAW-synced 1/4–1/32 subdivision with density control
- Joystick motion trigger with configurable threshold
- AudioPlayHead integration for ppqPosition-based timing

**Status:** COMPLETE (2/2 plans done — 04-01 + 04-02 both approved in Reaper)

**Plans:** 2/2 plans complete

Plans:
- [x] 04-01-PLAN.md — Joystick motion trigger: continuous gate model, joystickThreshold APVTS param, THRESHOLD slider, TouchPlate dimming in JOY/RND mode, legato note-on/note-off tracking — DONE: build clean, VST3 installed, DAW verified
- [x] 04-02-PLAN.md — Random gate: per-voice subdivision clock (ppqPosition sync + sync/free toggle), density 1–8 hits/bar, randomGateTime knob, randomClockSync + randomFreeTempo params, 4-column DENS/GATE/FREE BPM/SYNC row — DONE: all 5 Reaper tests passed, human checkpoint APPROVED

---

## Phase 05: LooperEngine Hardening and DAW Sync

**Goal:** Replace the std::mutex in LooperEngine with a lock-free design. Validate beat-locked record/playback in Ableton and Reaper.

**Delivers:**
- Lock-free LooperEngine: AbstractFifo + fixed-size double-buffer design (record FIFO + playbackStore_ array)
- Atomic flags for UI-side destructive ops (reset, delete) executed from audio thread
- ppqPosition anchor recorded at loop start (no drift, bar-boundary snap)
- Hard capacity cap (2048 events) with capReached_ indicator
- [REC JOY], [REC GATES], [SYNC] buttons in PluginEditor
- Sparse joystick recording (threshold-filtered, default 0.02)
- 9 new Catch2 tests (multi-thread stress + loop-wrap + DAW sync anchor)
- Verified 4-bar loop in Reaper; Ableton best-effort

**Status:** COMPLETE (3/3 plans done — all 4 Reaper tests passed, Ableton loads as MIDI effect)

**Plans:** 3 plans

Plans:
- [x] 05-01-PLAN.md — Lock-free LooperEngine rewrite (AbstractFifo + double-buffer) + Catch2 tests + CMake TSAN option — DONE: 26 tests pass, ScopedRead ordering bug fixed
- [x] 05-02-PLAN.md — PluginProcessor DAW sync wiring (JUCE 8 ppqPosition API) + PluginEditor [REC JOY]/[REC GATES]/[SYNC] buttons — DONE
- [x] 05-03-PLAN.md — Release build + full Catch2 suite + Reaper 4-test DAW verification — DONE: all 4 Reaper tests pass, Ableton fix applied (isMidiEffect=true)

---

## Phase 06: SDL2 Gamepad Integration

**Goal:** Fix SDL lifecycle bugs and validate gamepad control end-to-end.

**Delivers:**
- Process-level SDL singleton (reference-counted, no multi-instance SDL_Quit race)
- `SDL_HINT_JOYSTICK_THREAD "1"` set before SDL_Init
- Right stick → joystick XY, buttons → TouchPlate triggers, hot-plug confirmed
- Left stick → CC74/CC71 gated on `isConnected()` (no CC flood when unplugged)
- PS4 and Xbox controller confirmed working
- JOY pitch CV via MIDI pitch bend (RPN 0, +-24 semitones, 1-second gate-close)
- Filter Cut CC and Filter Res CC visible in DAW automation lane
- Gamepad remap: L1=Root, L2=Third, R1=Fifth, R2=Tension
- Looper auto-stop at loop boundary; [REC TOUCH] deferred-start button
- Always-editable scale keyboard with red active note highlights

**Status:** COMPLETE (4/4 plans done — all 8 DAW tests passed, human checkpoint approved)

**Plans:** 4/4 plans complete

Plans:
- [x] 06-01-PLAN.md — SdlContext process-level singleton + GamepadInput refactor (dead zone, sample-and-hold, 20ms button debounce) — DONE: plugin builds, SdlContext singleton + GamepadInput hardening complete
- [x] 06-02-PLAN.md — PluginProcessor CC gating (isConnected + gamepadActive_), CC dedup, disconnect pending flags, joystickThreshold forwarding — DONE
- [x] 06-03-PLAN.md — PluginEditor [GAMEPAD ON/OFF] toggle + status label fix + DAW verification checkpoint — DONE: 7/7 gamepad tests passed
- [x] 06-04-PLAN.md — Commit WIP session work (looper auto-stop, REC TOUCH, scale keyboard, JOY pitch CV, UI polish) + 8-test DAW verification checkpoint — DONE: all 8 tests passed, approved

---

## Phase 07: DAW Compatibility, Distribution, and Release

**Goal:** Pass pluginval at strictness level 5 and produce a signed-ready Windows installer (`DimaChordJoystick-Setup.exe`) for Gumroad distribution.

**Delivers:**
- pluginval passing at strictness level 5 (level 4 acceptable per fallback policy)
- Release build verified and installed at `%COMMONPROGRAMFILES%\VST3\ChordJoystick.vst3`
- Inno Setup 6 installer placing bundle in `%COMMONPROGRAMFILES%\VST3\` with uninstaller
- `DimaChordJoystick-Setup.exe` tested on a clean Windows machine without dev tools
- Code-signing placeholder in installer script (unsigned for v1 per decision)

**Status:** COMPLETE (2/2 plans done — pluginval level-5 PASSED + Inno Setup installer clean-machine approved)

**Plans:** 2/2 plans complete

Plans:
- [x] 07-01-PLAN.md — Release build + pluginval validation at strictness level 5 + code audit — DONE: all 19 test suites green, exit code 0
- [x] 07-02-PLAN.md — Inno Setup installer script + EXE build + clean-machine test checkpoint — DONE: DimaChordJoystick-Setup.exe (3.5 MB) clean-machine approved

---

*Milestone goal: Shippable paid Windows VST3 on Gumroad*
*Created: 2026-02-22*
