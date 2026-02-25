# ChordJoystick — Roadmap

## Milestones

- ✅ **v1.0 MVP** — Phases 01-07 (shipped 2026-02-23)
- 🚧 **v1.1 Polish & Quantization** — Phases 08-11 (in progress)

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

### 🚧 v1.1 Polish & Quantization (In Progress)

**Milestone Goal:** Formalize post-v1.0 patches with test coverage, improve MIDI panic to a full 16-channel reset sweep, add live and post-record trigger quantization, and polish the UI with visual grouping and a looper position bar.

- [ ] **Phase 08: Post-v1.0 Patch Verification** - Verify, test, and formally sign off the 6 patches already implemented in code
- [ ] **Phase 09: MIDI Panic and Mute Feedback** - Full 16-channel CC sweep panic (no CC121) and animated MUTED state visual feedback
- [ ] **Phase 10: Trigger Quantization Infrastructure** - LooperEngine quantize backend (pendingQuantize_ deferred pattern, snapToGrid, APVTS param)
- [ ] **Phase 11: UI Polish and Installer** - Section visual grouping, looper position bar, gamepad controller name display, installer v1.1 rebuild

## Phase Details

### Phase 08: Post-v1.0 Patch Verification
**Goal**: All 6 post-v1.0 patches are verified to work correctly under test, documented with REQ-IDs, and formally signed off as shipped v1.1 features
**Depends on**: Phase 07 (v1.0 shipped)
**Requirements**: PATCH-01, PATCH-02, PATCH-03, PATCH-04, PATCH-05, PATCH-06
**Success Criteria** (what must be TRUE):
  1. Sustain-pedal-aware synths receive CC64=127 before every note-on — verifiable with a MIDI monitor capturing the plugin's output stream
  2. Per-voice hold mode inverts the gate: pad press sends note-off, pad release sends note-on with current pitch — verifiable by ear with a held synth note
  3. Filter Mod button correctly gates all left-stick CC output, and knobs/dropdowns are visually disabled when Filter Mod is off — verifiable in the plugin UI
  4. MIDI Panic is a one-shot action — press sends allNotesOff and button immediately returns to pressable state with no persistent blocking — verifiable with a MIDI monitor
  5. Left-stick gestures are recorded to the loop when the looper is in record mode (filter looper always active) — verifiable by replaying a recorded loop that includes filter CC movement
  6. Changing attenuator knobs while the gamepad axis has natural noise does not retrigger filter CCs — verifiable with a MIDI monitor while wiggling an attenuator knob
**Plans**: 2 plans

Plans:
- [x] 08-01-PLAN.md — Fix PATCH-01 (CC64=127 before note-on) and PATCH-04 (one-shot panic button) — done 2026-02-24
- [ ] 08-02-PLAN.md — Build verification, manual 6-patch test protocol, produce VALIDATION.md sign-off

### Phase 09: MIDI Panic and Mute Feedback
**Goal**: Pressing Panic silences all 16 MIDI channels completely as a one-shot action (no persistent mute), with brief button flash feedback confirming the action fired
**Depends on**: Phase 08
**Requirements**: PANIC-01, PANIC-02, PANIC-03
**Success Criteria** (what must be TRUE):
  1. Pressing Panic emits exactly 48 events (16 channels x 3 CCs: CC64=0, CC120, CC123) — verifiable with a MIDI monitor; zero CC121 events appear
  2. After Panic, all notes are silenced on all 16 MIDI channels including channels not assigned to any voice; plugin continues generating MIDI normally on the next trigger
  3. Panic button briefly flashes on press (using existing flashPanic_ counter pattern) and returns to normal state — no persistent toggle or output-blocking state remains
  4. The flash animation reuses the existing 30 Hz editor timer — grep for startTimerHz in PluginEditor.cpp returns exactly 1 result
**Plans**: 2 plans

Plans:
- [x] 09-01-PLAN.md — Expand processor panic sweep to all 16 channels + add PANIC! button to editor with flash feedback — done 2026-02-25
- [ ] 09-02-PLAN.md — Build plugin and human verification of 48-event sweep, button visibility, flash, and MIDI resume

### Phase 10: Trigger Quantization Infrastructure
**Goal**: Recorded gate events can be snapped to a user-selected subdivision grid both during recording (live) and after recording (post-record), without data races on the audio thread
**Depends on**: Phase 09
**Requirements**: QUANT-01, QUANT-02, QUANT-03
**Success Criteria** (what must be TRUE):
  1. With live quantize ON, gate events recorded at arbitrary times within a 1/8-note window land exactly on the nearest 1/8-note boundary — verifiable by inspecting recorded beat positions
  2. Pressing the QUANTIZE button while the looper is stopped re-snaps all existing loop events to the selected subdivision — verifiable by playing back a quantized loop and hearing rhythmically tight events
  3. The QUANTIZE button is disabled (greyed out) while the looper is playing or recording — verifiable in the plugin UI
  4. A gate event recorded near the loop end (beatPosition = loopLen - 0.001) and quantized to a 1-beat grid is stored as beatPosition 0.0 and fires exactly once per loop cycle — verifiable via Catch2 test
  5. The quantize subdivision selector offers 1/4, 1/8, 1/16, and 1/32 note options and operates independently from per-voice random gate subdivision
**Plans**: TBD

### Phase 11: UI Polish and Installer
**Goal**: The plugin UI is visually organized with clear section groupings, the looper shows current playback position, the gamepad status shows the actual controller type, and the v1.1 installer is ready for distribution
**Depends on**: Phase 10
**Requirements**: UI-01, UI-02, UI-03, UI-04
**Success Criteria** (what must be TRUE):
  1. Filter Mod, Looper, and Gamepad sections each have a visible separator or header label — verifiable by opening the plugin in any DAW
  2. Gamepad status label shows the specific controller type ("PS4 Connected", "Xbox One Connected", or "Controller Connected") rather than a plain ON/OFF indicator — verifiable by connecting a PS4 or Xbox controller
  3. A looper position bar sweeps forward continuously during playback and does not jump backward at loop wrap — verifiable by visual inspection during a 1-bar loop at 180 BPM
  4. The installer version string and feature list reflect v1.1 — verifiable by running the installer and checking the information page
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 01. Build Foundation | v1.0 | 2/2 | Complete | 2026-02-23 |
| 02. Core Engine Validation | v1.0 | 2/2 | Complete | 2026-02-23 |
| 03. Core MIDI Output | v1.0 | 2/2 | Complete | 2026-02-23 |
| 04. Trigger Sources & Random Gate | v1.0 | 2/2 | Complete | 2026-02-23 |
| 05. LooperEngine Hardening | v1.0 | 3/3 | Complete | 2026-02-23 |
| 06. SDL2 Gamepad Integration | v1.0 | 4/4 | Complete | 2026-02-23 |
| 07. Distribution & Release | v1.0 | 2/2 | Complete | 2026-02-23 |
| 08. Post-v1.0 Patch Verification | v1.1 | 1/2 | In progress | - |
| 09. MIDI Panic and Mute Feedback | v1.1 | 1/2 | In progress | - |
| 10. Trigger Quantization Infrastructure | v1.1 | 0/TBD | Not started | - |
| 11. UI Polish and Installer | v1.1 | 0/TBD | Not started | - |

---
*v1.0 shipped 2026-02-23 — 7 phases, 17 plans, ~3,966 C++ LOC*
*v1.1 roadmap created 2026-02-24 — 4 phases (08-11), 16 requirements*
