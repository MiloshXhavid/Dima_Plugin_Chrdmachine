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

**Status:** in-progress (1/2 plans complete)

Plans:
- [x] 01-01: Fix CMakeLists.txt (pin JUCE tag, static CRT, validate plugin target) — DONE: build verified, VST3 installed
- [ ] 01-02: Smoke test — plugin loads in Reaper, APVTS round-trip verified

---

## Phase 02: Core Engine Validation

**Goal:** Verify ChordEngine and ScaleQuantizer produce correct output via unit tests before wiring into the audio thread.

**Delivers:**
- Catch2 test suite: scale quantization (all presets, edge cases, tie-breaking), ChordEngine pitch computation across all 4 voices, custom 12-key scale entry
- All tests passing in CI/build

**Status:** pending

Plans:
- [ ] 02-01: Add Catch2 CMake target and ScaleQuantizer unit tests
- [ ] 02-02: Add ChordEngine unit tests (all 4 voices, intervals, octave offsets, transpose)

---

## Phase 03: Core MIDI Output and Note-Off Guarantee

**Goal:** Wire TriggerSystem into processBlock to produce sample-accurate MIDI note-on/off. Guarantee note-off on all exit paths.

**Delivers:**
- All 4 voices producing MIDI note-on/off via TouchPlate triggers
- Note-off flushed on `releaseResources()`, transport stop, and bypass
- Per-voice MIDI channel routing verified
- Visual gate LED feedback in PluginEditor updating at 30 Hz

**Status:** pending

Plans:
- [ ] 03-01: Wire TriggerSystem to processBlock (TouchPlate → MIDI note-on/off)
- [ ] 03-02: Implement releaseResources() note-off flush and bypass handling
- [ ] 03-03: Verify per-voice MIDI channel routing in Reaper

---

## Phase 04: Per-Voice Trigger Sources and Random Gate

**Goal:** Complete the trigger source model — add joystick-motion and random subdivision sources on top of the working TouchPlate trigger.

**Delivers:**
- Per-voice trigger source selector (TouchPlate / Joystick Motion / Random) fully functional
- Random gate: DAW-synced 1/4–1/32 subdivision with density control
- Joystick motion trigger with configurable threshold
- AudioPlayHead integration for ppqPosition-based timing

**Status:** pending

Plans:
- [ ] 04-01: Joystick motion trigger (threshold-based, per voice)
- [ ] 04-02: Random gate (ppqPosition subdivisions + density knob, all 4 subdivisions)

---

## Phase 05: LooperEngine Hardening and DAW Sync

**Goal:** Replace the std::mutex in LooperEngine with a lock-free design. Validate beat-locked record/playback in Ableton and Reaper.

**Delivers:**
- Lock-free LooperEngine: AbstractFifo + fixed-size event array
- Atomic flags for UI-side destructive ops (reset, delete)
- ppqPosition anchor recorded at loop start (no drift)
- Hard capacity cap preventing audio-thread allocation
- Verified 1–16 bar loop in Ableton and Reaper

**Research flag:** Needs /gsd:research-phase — lock-free buffer design has multiple valid approaches.

**Status:** pending

Plans:
- [ ] 05-01: Replace LooperEngine mutex with lock-free AbstractFifo design
- [ ] 05-02: DAW sync validation — record/play/stop in Ableton and Reaper

---

## Phase 06: SDL2 Gamepad Integration

**Goal:** Fix SDL lifecycle bugs and validate gamepad control end-to-end.

**Delivers:**
- Process-level SDL singleton (reference-counted, no multi-instance SDL_Quit race)
- `SDL_HINT_JOYSTICK_THREAD "1"` set before SDL_Init
- Right stick → joystick XY, buttons → TouchPlate triggers, hot-plug confirmed
- Left stick → CC74/CC71 gated on `isConnected()` (no CC flood when unplugged)
- PS4 and Xbox controller confirmed working

**Research flag:** Needs /gsd:research-phase — verify SDL_HINT_JOYSTICK_THREAD behavior in SDL2 2.30.9.

**Status:** pending

Plans:
- [ ] 06-01: SDL singleton lifecycle + hint configuration
- [ ] 06-02: Gamepad axis and button wiring + hot-plug test
- [ ] 06-03: Filter CC gating (CC71/CC74 on isConnected() + value-change only)

---

## Phase 07: DAW Compatibility, Distribution, and Release

**Goal:** Resolve Ableton MIDI routing, pass pluginval, and produce a signed installer ready for Gumroad distribution.

**Delivers:**
- Ableton Live 11/12 and Reaper 7 confirmed MIDI routing (VST3 category finalized)
- pluginval passing at strictness level 5
- Inno Setup installer placing bundle in `%COMMONPROGRAMFILES%\VST3\`
- Code signing applied to DLL and installer
- SmartScreen not blocking on clean Windows 11 VM

**Research flag:** Needs /gsd:research-phase — correct IS_SYNTH / IS_MIDI_EFFECT / VST3_CATEGORIES combo for Ableton MIDI output routing is empirically determined.

**Status:** pending

Plans:
- [ ] 07-01: VST3 category configuration and Ableton routing validation
- [ ] 07-02: pluginval validation (strictness 5) and fix any reported issues
- [ ] 07-03: Inno Setup installer + code signing

---

*Milestone goal: Shippable paid Windows VST3 on Gumroad*
*Created: 2026-02-22*
