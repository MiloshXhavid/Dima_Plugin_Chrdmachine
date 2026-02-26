# ChordJoystick — Roadmap

## Milestones

- ✅ **v1.0 MVP** — Phases 01-07 (shipped 2026-02-23)
- ✅ **v1.3 Polish & Quantization** — Phases 09-11 (shipped 2026-02-25)
- 🚧 **v1.4 LFO + Clock** — Phases 12-16 (in progress)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 01-07) — SHIPPED 2026-02-23</summary>

- [x] Phase 01: Build Foundation and JUCE Version Lock (2/2 plans) — completed 2026-02-23
- [x] Phase 02: Core Engine Validation (2/2 plans) — completed 2026-02-23
- [x] Phase 03: Core MIDI Output and Note-Off Guarantee (2/2 plans) — completed 2026-02-23
- [x] Phase 04: Per-Voice Trigger Sources and Random Gate (2/2 plans) — completed 2026-02-23
- [x] Phase 05: LooperEngine Hardening and DAW Sync (3/3 plans) — completed 2026-02-23
- [x] Phase 06: SDL2 Gamepad Integration (4/4 plans) — completed 2026-02-23
- [x] Phase 07: DAW Compatibility, Distribution, and Release (2/2 plans) — completed 2026-02-23

Full details: `.planning/milestones/v1.0-ROADMAP.md`

</details>

<details>
<summary>✅ v1.3 Polish & Quantization (Phases 09-11) — SHIPPED 2026-02-25</summary>

- [x] Phase 09: MIDI Panic and Mute Feedback (2/2 plans) — completed 2026-02-25
- [x] Phase 10: Trigger Quantization Infrastructure (5/5 plans) — completed 2026-02-25
- [x] Phase 11: UI Polish and Installer (4/4 plans) — completed 2026-02-25

Full details: `.planning/milestones/v1.3-ROADMAP.md`

</details>

### 🚧 v1.4 LFO + Clock (In Progress)

**Milestone Goal:** Add dual per-axis LFO modulation to the joystick with a beat clock indicator, then ship a clean GitHub release.

- [x] **Phase 12: LFO Engine Core** — Isolated LFO DSP class (LfoEngine.h/cpp): all 7 waveforms, dual-axis phase accumulators, LCG RNG, distortion, beat detection, audio-thread safety (completed 2026-02-26)
- [ ] **Phase 13: processBlock Integration and APVTS** — Wire LfoEngine into the processor: 16 APVTS params, process() call in processBlock, LFO output applied as additive offset in buildChordParams(), phase reset on DAW start/stop
- [ ] **Phase 14: LFO UI and Beat Clock** — Full LFO panel left of joystick + beat clock dot near Free BPM knob, all controls attached to APVTS
- [ ] **Phase 15: Gamepad Preset Control** — Option button toggles preset-scroll mode; BPM±1 controls send MIDI Program Change; UI shows active mode + current program number
- [ ] **Phase 16: Distribution** — GitHub v1.4 release with installer binary + release notes; desktop backup

## Phase Details

### Phase 12: LFO Engine Core
**Goal**: A fully tested, audio-thread-safe LFO DSP class exists as a standalone unit with all waveforms and performance guarantees met
**Depends on**: Phase 11 (codebase in v1.3 state)
**Requirements**: LFO-03, LFO-04, LFO-05, LFO-06, LFO-07, PERF-01, PERF-02, PERF-03
**Success Criteria** (what must be TRUE):
  1. All 7 waveforms (Sine, Triangle, Saw Up, Saw Down, Square, S&H, Random) produce output normalized to [-1, 1] — verified by Catch2 unit tests
  2. S&H waveform holds a constant value for an entire cycle period and jumps instantaneously at the cycle boundary — no interpolation between steps
  3. LFO phase in sync mode is derived from ppqPos (DAW playing) or a sample counter (free mode) — no free-accumulating phase drift on transport stop/start
  4. S&H and Random waveforms use the project's existing LCG pattern (`seed * 1664525u + 1013904223u`), never `std::rand()` — no CRT mutex acquisition on the audio thread
  5. LFO output is computed as a local float value; no write-back to shared `joystickX`/`joystickY` atomics occurs anywhere in LfoEngine
**Plans**: 2 plans
Plans:
- [ ] 12-01-PLAN.md — LfoEngine DSP class: all 7 waveforms, LCG, sync/free modes, S&H boundary detection
- [ ] 12-02-PLAN.md — Catch2 tests and CMakeLists update: build test target, prove all success criteria

### Phase 13: processBlock Integration and APVTS
**Goal**: The LFO modulates joystick X and Y axes during every processBlock call, with all parameters automatable by the DAW host and presets load without breaking v1.3 state
**Depends on**: Phase 12
**Requirements**: LFO-01, LFO-02, LFO-08, LFO-09, LFO-10
**Success Criteria** (what must be TRUE):
  1. Enabling either LFO with level > 0 causes audible pitch variation on the corresponding voices (X-axis LFO affects fifth/tension voices, Y-axis LFO affects root/third voices) — verifiable in DAW MIDI monitor
  2. Setting LFO enabled to Off stops modulation immediately without any note hanging or stuck output
  3. With DAW sync active and Sync ON, LFO phase locks to DAW beat — restarting transport resets LFO to phase 0
  4. Loading a v1.3 preset produces all existing parameters at their saved values and all new LFO parameters at safe defaults (enabled=false, level=0) — no parameter ID collisions
  5. All 16 new LFO parameters appear in the DAW automation lane and respond to automation playback
**Plans**: 1 plan
Plans:
- [ ] 13-01-PLAN.md — PluginProcessor.h/cpp: 16 APVTS params, lfoX_/lfoY_ members, processBlock LFO injection with ramp, prepareToPlay + transport resets

### Phase 14: LFO UI and Beat Clock
**Goal**: The player can control both LFOs and see beat timing through the plugin UI without opening the DAW
**Depends on**: Phase 13
**Requirements**: LFO-11, CLK-01, CLK-02
**Success Criteria** (what must be TRUE):
  1. Two LFO panels are visible to the left of the joystick pad; the joystick pad is shifted right to accommodate — no controls overlap or are clipped at the editor boundary
  2. Each LFO panel contains a shape dropdown, Rate slider (Hz in free mode / subdivision selector in sync mode), Phase slider, Distortion slider, Level slider, Enabled toggle, and Sync button — all controls function and save with the preset
  3. A small dot next to the Free BPM knob flashes once per beat when free tempo is active, and switches to flashing on DAW beats when DAW sync is active
  4. Toggling Sync on an LFO panel switches the Rate control label and behavior between Hz display and subdivision steps — no UI glitch or layout shift
**Plans**: TBD

### Phase 15: Gamepad Preset Control
**Goal**: The Option button on the PS/Xbox controller switches the BPM±1 controls into MIDI Program Change mode, with clear UI feedback showing the active mode and current program number
**Depends on**: Phase 14 (UI framework in place)
**Requirements**: CTRL-01, CTRL-02, CTRL-03
**Success Criteria** (what must be TRUE):
  1. Pressing the Option button activates preset-scroll mode and a visible indicator appears in the plugin UI; pressing again deactivates it
  2. In preset-scroll mode, the gamepad controls that normally increment/decrement BPM instead send MIDI Program Change +1 / −1 on the configured MIDI channel — verified in DAW MIDI monitor
  3. The plugin UI displays the current program number (0–127) while preset-scroll mode is active; number updates on each Program Change sent
**Plans**: TBD

### Phase 16: Distribution
**Goal**: v1.4 is publicly released on GitHub and backed up locally
**Depends on**: Phase 15
**Requirements**: DIST-01, DIST-02
**Success Criteria** (what must be TRUE):
  1. A GitHub release tagged `v1.4` exists with the installer binary attached and a release notes body listing the LFO + beat clock features
  2. A copy of the built plugin and installer is present at `Desktop/Dima_plug-in` — confirmed by directory listing
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 01–07 | v1.0 | 17/17 | ✅ Shipped | 2026-02-23 |
| 09. MIDI Panic | v1.3 | 2/2 | ✅ Shipped | 2026-02-25 |
| 10. Trigger Quantization | v1.3 | 5/5 | ✅ Shipped | 2026-02-25 |
| 11. UI Polish + Installer | v1.3 | 4/4 | ✅ Shipped | 2026-02-25 |
| 12. LFO Engine Core | 2/2 | Complete    | 2026-02-26 | - |
| 13. processBlock Integration + APVTS | v1.4 | 0/1 | Planned | - |
| 14. LFO UI + Beat Clock | v1.4 | 0/? | Not started | - |
| 15. Gamepad Preset Control | v1.4 | 0/? | Not started | - |
| 16. Distribution | v1.4 | 0/? | Not started | - |

---
*v1.0 shipped 2026-02-23 — 7 phases, 17 plans*
*v1.3 shipped 2026-02-25 — 3 phases, 11 plans, 47 files changed*
*v1.4 roadmap created 2026-02-26 — 5 phases, 21 requirements*
