# ChordJoystick — Roadmap

## Milestones

- ✅ **v1.0 MVP** — Phases 01-07 (shipped 2026-02-23)
- ✅ **v1.3 Polish & Quantization** — Phases 09-11 (shipped 2026-02-25)
- ✅ **v1.4 LFO + Clock** — Phases 12-16 (shipped 2026-02-26)
- ✅ **v1.5 Routing + Expression** — Phases 17-25 (shipped 2026-03-02)
- ✅ **v1.6 Triplets & Fixes** — Phases 26-30 (shipped 2026-03-03)
- ✅ **v1.7 Space Joystick** — Phases 31-33.1 (shipped 2026-03-05)
- ✅ **v1.8 Modulation Expansion + Arp/Looper Fixes** — Phases 34-37, 44 (shipped 2026-03-07)
- 🔲 **v1.9 Living Interface** — Phases 38-43 (planned)

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

<details>
<summary>✅ v1.4 LFO + Clock (Phases 12-16) — SHIPPED 2026-02-26</summary>

- [x] **Phase 12: LFO Engine Core** — Isolated LFO DSP class: all 7 waveforms, dual-axis phase accumulators, LCG RNG, distortion, beat detection, audio-thread safety (completed 2026-02-26)
- [x] **Phase 13: processBlock Integration and APVTS** — Wire LfoEngine into the processor: 16 APVTS params, process() call in processBlock, LFO output applied as additive offset in buildChordParams(), phase reset on DAW start/stop (completed 2026-02-26)
- [x] **Phase 14: LFO UI and Beat Clock** — Full LFO panel left of joystick + beat clock dot on Free BPM knob face, all controls attached to APVTS (completed 2026-02-26)
- [x] **Phase 15: Gamepad Preset Control** — Option button toggles preset-scroll mode; D-pad Up/Down send MIDI Program Change; UI shows OPTION indicator (completed 2026-02-26)
- [x] **Phase 16: Distribution** — GitHub v1.4 release with installer binary + release notes; desktop backup (completed 2026-02-26)

Full details: Phase Details section below (Phases 12–16).

</details>

<details>
<summary>✅ v1.5 Routing + Expression (Phases 17-25) — SHIPPED 2026-03-02</summary>

- [x] Phase 17: Bug Fixes (3/3 plans) — completed 2026-02-28
- [x] Phase 18: Single-Channel Routing (3/3 plans) — completed 2026-03-01
- [x] Phase 19: Sub Octave Per Voice (2/2 plans) — completed 2026-03-01
- [x] Phase 20: Random Trigger System Extensions (3/3 plans) — completed 2026-03-01
- [x] Phase 21: Left Joystick Modulation Expansion (2/2 plans) — completed 2026-03-01
- [x] Phase 22: LFO Recording (3/3 plans) — completed 2026-03-01
- [x] Phase 23: Arpeggiator (2/2 plans) — completed 2026-03-01
- [x] Phase 24: Gamepad Option Mode 1 (3/3 plans) — completed 2026-03-01
- [x] Phase 24.1: LFO Joystick Visual Tracking (2/2 plans) — completed 2026-03-02
- [x] Phase 25: Distribution (2/2 plans) — completed 2026-03-02

Full details: `.planning/milestones/v1.5-ROADMAP.md`

</details>

<details>
<summary>✅ v1.6 Triplets &amp; Fixes (Phases 26-30) — SHIPPED 2026-03-03</summary>

- [x] Phase 26: Defaults and Bug Fix — octave defaults + Natural Minor + noteCount_ clamp removal (completed 2026-03-02)
- [x] Phase 27: Triplet Subdivisions — 1/1T–1/32T in random trigger and quantize selectors (completed 2026-03-03)
- [x] Phase 28: Random Free Redesign — Poisson/internal-clock/DAW-grid three-branch sync matrix (completed 2026-03-03)
- [x] Phase 29: Looper Perimeter Bar — clockwise rectangular bar, ghost ring, label exclusion (completed 2026-03-03)
- [x] Phase 30: Distribution — GitHub v1.6 release + installer + desktop backup (completed 2026-03-03)

</details>

<details>
<summary>✅ v1.7 Space Joystick (Phases 31-33.1) — SHIPPED 2026-03-05</summary>

- [x] Phase 31: Paint-Only Visual Foundation — deep-space background, milky way, starfield, semitone grid, BPM glow ring (completed 2026-03-04)
- [x] Phase 32: Spring-Damper Inertia and Angle Indicator — cursor physics, perimeter arc, note-label compass (completed 2026-03-04)
- [x] Phase 33.1: Cursor/INV/LFO Fixes — 7 UI bug fixes: cursor snap, spring damping, log LFO rate, INV label/attachment swap, piano hit test, battery icon (completed 2026-03-05)
- [x] Phase 33: Version Sync — GitHub v1.7 Latest release, installer v1.7, desktop backup (completed 2026-03-05)

</details>

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
- [x] 12-01-PLAN.md — LfoEngine DSP class: all 7 waveforms, LCG, sync/free modes, S&H boundary detection (completed 2026-02-26)
- [x] 12-02-PLAN.md — Catch2 tests and CMakeLists update: build test target, prove all success criteria (completed 2026-02-26)

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
- [x] 13-01-PLAN.md — PluginProcessor.h/cpp: 16 APVTS params, lfoX_/lfoY_ members, processBlock LFO injection with ramp, prepareToPlay + transport resets (completed 2026-02-26)

### Phase 14: LFO UI and Beat Clock
**Goal**: The player can control both LFOs and see beat timing through the plugin UI without opening the DAW
**Depends on**: Phase 13
**Requirements**: LFO-11, CLK-01, CLK-02
**Success Criteria** (what must be TRUE):
  1. Two LFO panels are visible to the left of the joystick pad; the joystick pad is shifted right to accommodate — no controls overlap or are clipped at the editor boundary
  2. Each LFO panel contains a shape dropdown, Rate slider (Hz in free mode / subdivision selector in sync mode), Phase slider, Distortion slider, Level slider, Enabled toggle, and Sync button — all controls function and save with the preset
  3. A small dot next to the Free BPM knob flashes once per beat when free tempo is active, and switches to flashing on DAW beats when DAW sync is active
  4. Toggling Sync on an LFO panel switches the Rate control label and behavior between Hz display and subdivision steps — no UI glitch or layout shift
**Plans**: 3 plans
Plans:
- [x] 14-01-PLAN.md — PluginProcessor.h/cpp: beatOccurred_, modulatedJoyX_/Y_ atomics + processBlock beat detection and modulated position stores (completed 2026-02-26)
- [x] 14-02-PLAN.md — PluginEditor.h/cpp: 1120px layout, LFO X/Y panel columns, all controls + APVTS attachments, Sync toggle Rate swap (completed 2026-02-26)
- [x] 14-03-PLAN.md — PluginEditor.cpp: JoystickPad LFO tracking dot via modulatedJoyX_/Y_ + 7-issue layout overhaul (completed 2026-02-26)

### Phase 15: Gamepad Preset Control
**Goal**: The Option button on the PS/Xbox controller switches the BPM±1 controls into MIDI Program Change mode, with a subtle "OPTION" indicator in the gamepad status area of the plugin UI
**Depends on**: Phase 14 (UI framework in place)
**Requirements**: CTRL-01, CTRL-02, CTRL-03
**Success Criteria** (what must be TRUE):
  1. Pressing the Option button activates preset-scroll mode and a visible indicator appears in the plugin UI; pressing again deactivates it
  2. In preset-scroll mode, the gamepad controls that normally increment/decrement BPM instead send MIDI Program Change +1 / −1 on the configured MIDI channel — verified in DAW MIDI monitor
  3. The plugin UI shows an "OPTION" suffix on the gamepad status label in highlight color when preset-scroll mode is active
**Plans**: 2 plans
Plans:
- [x] 15-01-PLAN.md — GamepadInput.h/cpp: Option/Guide button toggle, presetScrollActive_, consumePcDelta(), one-frame lockout (completed 2026-02-26)
- [x] 15-02-PLAN.md — PluginProcessor.h/cpp: program counter + PC message routing; PluginEditor.cpp: OPTION indicator in gamepadStatusLabel_ (completed 2026-02-26)

### Phase 16: Distribution
**Goal**: v1.4 is publicly released on GitHub and backed up locally
**Depends on**: Phase 15
**Requirements**: DIST-01, DIST-02
**Success Criteria** (what must be TRUE):
  1. A GitHub release tagged `v1.4` exists with the installer binary attached and a release notes body listing the LFO + beat clock features
  2. A copy of the built plugin and installer is present at `Desktop/Dima_Plug-in/v1.4/` — confirmed by directory listing
**Plans**: 2 plans
Plans:
- [x] 16-01-PLAN.md — Update .iss for v1.4, rebuild installer, smoke test checkpoint (completed 2026-02-26)
- [x] 16-02-PLAN.md — Retag v1.4 to HEAD, create GitHub release, desktop backup (completed 2026-02-26)

### Phase 44: 38.3 Sister LFO attenuation inline slider

**Goal:** When a Sister LFO cross-modulation target is selected, shrink the Sister combo to half-width and show a bipolar attenuator slider (-1..+1) in the freed right half. "None" selected = full-width combo, slider hidden.
**Requirements**: REQ-01, REQ-02, REQ-03, REQ-04, REQ-05, REQ-06, REQ-07, REQ-08
**Depends on:** Phase 38.2
**Plans:** 1 plan

Plans:
- [ ] 44-38-3-01-PLAN.md — Processor params + Editor slider construction/layout/paint + drawLinearSlider bipolar fill + Catch2 tests + UAT checkpoint

---

### Phase 17: Bug Fixes
**Goal**: Two known correctness defects are eliminated so that the codebase is a stable foundation for all v1.5 feature work — the looper no longer drifts after a record cycle, and the plugin no longer crashes when a PS4 reconnects via Bluetooth
**Depends on**: Phase 16
**Requirements**: BUG-01, BUG-02
**Success Criteria** (what must be TRUE):
  1. After recording a loop and letting it play back, the first beat of every subsequent cycle aligns exactly with the DAW grid — no audible phase offset or late-start drift
  2. Disconnecting and reconnecting a PS4 controller via Bluetooth during a live session does not crash the plugin and the controller resumes normal operation within one reconnect cycle
  3. Both fixes pass a pluginval level 5 run with no new failures introduced
**Plans**: 3 plans
Plans:
- [x] 17-01-PLAN.md — LooperEngine.cpp: overshoot-corrected loopStartPpq_ anchor fix; TC 13 regression test (BUG-01) (completed 2026-02-28)
- [x] 17-02-PLAN.md — GamepadInput.h/cpp: !controller_ guard on ADDED, instance-ID guard on REMOVED, pendingReopenTick_ deferred open (BUG-02) (completed 2026-02-28)
- [x] 17-03-PLAN.md — pluginval level 5 run + manual smoke test checkpoint (both bugs) (completed 2026-02-28)

### Phase 18: Single-Channel Routing
**Goal**: Users can route all four voices to a single configurable MIDI channel so that any monophonic or paraphonic synthesizer can be driven by the plugin without per-voice MIDI channel setup in the DAW
**Depends on**: Phase 17
**Requirements**: ROUT-01, ROUT-02, ROUT-03, ROUT-04, ROUT-05
**Success Criteria** (what must be TRUE):
  1. A "Multi-Channel / Single Channel" dropdown is visible in the bottom-right of the plugin UI; switching to Single Channel reveals a second channel selector (1–16)
  2. In Single Channel mode all four voices, CC74, and CC71 messages arrive on the same MIDI channel in the DAW MIDI monitor — no messages appear on other channels
  3. Pressing two pads simultaneously that produce the same pitch in Single Channel mode results in exactly one note-on and one note-off — no doubled velocity and no stuck note
  4. Looper playback in Single Channel mode sends notes on the currently selected channel even if the channel was different at record time
**Plans**: 3 plans
Plans:
- [x] 18-01-PLAN.md — PluginProcessor.h/cpp: singleChanMode + singleChanTarget APVTS params, effectiveChannel lambda, noteCount_[16][128] deduplication, channel snapshots, flush logic at all allNotesOff paths (completed 2026-02-28)
- [x] 18-02-PLAN.md — PluginEditor.h/cpp: Routing panel with routingModeBox_, singleChanTargetBox_, voiceChBox_[4], APVTS attachments, resized() layout, timerCallback() show/hide (completed 2026-02-28)
- [x] 18-03-PLAN.md — Deploy VST3 + manual smoke test checkpoint (all 5 ROUT requirements verified in DAW MIDI monitor) (completed 2026-03-01)

### Phase 19: Sub Octave Per Voice
**Goal**: Players can add a parallel bass note exactly one octave below any individual voice, controllable per-pad from both the UI and the gamepad, enabling bass-doubling without affecting the other voices
**Depends on**: Phase 18
**Requirements**: SUBOCT-01, SUBOCT-02, SUBOCT-03, SUBOCT-04
**Success Criteria** (what must be TRUE):
  1. Each of the four voice pads displays a Sub Oct toggle (right half of the existing Hold button); activating it causes a second note at exactly -12 semitones to sound simultaneously on every trigger for that voice
  2. Releasing a pad with Sub Oct active sends a note-off for the sub-octave note at the exact pitch that was sent at note-on time — no stuck sub-octave notes even if the joystick moved between press and release
  3. Holding a pad button on the gamepad (R1/R2/L1/L2) and pressing R3 toggles Sub Oct for that voice — confirmed by the UI toggle updating visually
  4. Sub Oct state survives preset save and load — a preset saved with Sub Oct on for voice 1 reloads with Sub Oct on for voice 1
**Plans**: 2 plans
Plans:
- [x] 19-01-PLAN.md — Processor backend: APVTS params, note-on/off emission, mid-note toggle, looper gate sub path, R3 combo (completed 2026-03-01)
- [x] 19-02-PLAN.md — UI: SUB8 button split, ButtonParameterAttachment, timerCallback coloring, smoke-test checkpoint (completed 2026-03-01)

### Phase 20: Random Trigger System Extensions
**Goal**: The random trigger source becomes expressive and composable — players can choose between continuous free-firing and held-pad burst modes, set a population ceiling and per-slot probability, access 1/64 subdivision, and gate length is available as a unified parameter shared with the arpeggiator
**Depends on**: Phase 18
**Requirements**: RND-01, RND-02, RND-03, RND-04, RND-05, RND-06, RND-07
**Success Criteria** (what must be TRUE):
  1. The trigger source dropdown for each voice shows four options: Pad / Joystick / Random Free / Random Hold — and existing saved presets using the old "Random" option load as "Random Free" without breaking
  2. In Random Free mode, gates fire continuously; in Random Hold mode, gates fire only while the pad is physically held down — verifiable by pressing and releasing the touchplate
  3. The Population knob (1–64) limits how many gate slots are generated per bar; the Probability knob (0–100%) sets the chance each slot actually fires — both are automatable and save with the preset
  4. Subdivision 1/64 is available in the random subdivision selector alongside 1/4, 1/8, 1/16, 1/32
  5. A single Gate Length knob (0–100% of subdivision) controls note hold duration for both Random and Arpeggiator sources; the knob is present in the UI and automatable
**Plans**: 3 plans
Plans:
- [x] 20-01-PLAN.md — TriggerSystem backend: RandomHold enum, double-roll probability, 1/64 subdivision, manual-gate sentinel (completed 2026-03-01)
- [x] 20-02-PLAN.md — PluginProcessor APVTS params: randomPopulation, randomProbability, gateLength; ProcessParams forwarding; arp gate migration (completed 2026-03-01)
- [x] 20-03-PLAN.md — PluginEditor UI: 4-option trigger combos, 5-option subdiv combos, Population + Probability knobs, smoke-test checkpoint (completed 2026-03-01)

### Phase 21: Left Joystick Modulation Expansion
**Goal**: The left joystick X and Y axes can be pointed at LFO frequency, phase, level, or arp gate length in addition to the existing CC74/CC71 filter targets, so players can modulate expressively from the gamepad without touching the mouse
**Depends on**: Phase 20
**Requirements**: LJOY-01, LJOY-02, LJOY-03, LJOY-04
**Success Criteria** (what must be TRUE):
  1. The Left Joystick X dropdown offers at least six target options including Filter Cutoff (CC74), LFO-X Frequency, LFO-X Phase, LFO-X Level, and Gate Length; the Left Joystick Y dropdown offers the equivalent Y-axis variants
  2. Selecting LFO-X Frequency as the Left Joystick X target and moving the left stick left-right produces audible LFO rate changes — confirmed by LFO waveform speed changing in the UI
  3. When LFO Sync mode is active and LFO Frequency is the stick target, the stick scales the sync subdivision rate (0.25x–4x multiplier) — stays grid-locked but speed changes expressively
  4. Switching a stick target from CC74 to any LFO parameter stops CC74 from being emitted — no leftover CC messages on the old target after a target change
**Plans**: 2 plans
Plans:
- [x] 21-01-PLAN.md — Processor backend: extend APVTS 6-item choice params, lfoXSubdivMult_/lfoYSubdivMult_ atomics, processBlock LFO/Gate dispatch, subdivision multiplier wiring
- [x] 21-02-PLAN.md — UI: 6-item ComboBoxes, X-above-Y layout swap, timerCallback Atten label relabeling, smoke-test checkpoint

### Phase 22: LFO Recording
**Goal**: Players can capture one loop cycle of live LFO output into the LFO and replay it in perfect sync with the looper — enabling repeatable LFO shapes from live performance without programming — while Distort stays adjustable during playback
**Depends on**: Phase 17
**Requirements**: LFOREC-01, LFOREC-02, LFOREC-03, LFOREC-04, LFOREC-05, LFOREC-06
**Success Criteria** (what must be TRUE):
  1. An Arm button appears next to the LFO Sync button; pressing it arms recording — the LFO does not begin capturing until the looper starts its next record cycle
  2. After arming and starting a looper record cycle, LFO output is captured for exactly one loop length; recording stops automatically and the LFO enters playback mode replaying the captured shape in sync with the looper
  3. During LFO playback mode, the Shape, Freq, Phase, and Level controls are visually grayed out and non-interactive; the Distort slider remains fully adjustable and audibly changes the playback output
  4. Pressing the Clear button returns the LFO to normal (unarmed) mode — all grayed-out controls become interactive again and the LFO resumes real-time generation
**Plans**: 3 plans
Plans:
- [x] 22-01-PLAN.md — LfoEngine DSP: RecState enum, ring buffer capture, playback branch
- [x] 22-02-PLAN.md — PluginProcessor: passthrough methods, edge detection, playbackPhase injection
- [x] 22-03-PLAN.md — PluginEditor: ARM/CLR buttons, blink, grayout, smoke-test checkpoint (completed 2026-03-01)

### Phase 23: Arpeggiator
**Goal**: Players can arpeggiate the current four-voice chord at a selected rate and order, synchronized to the DAW or looper, so the plugin produces melodic patterns without external MIDI tools
**Depends on**: Phase 20
**Requirements**: ARP-01, ARP-02, ARP-03, ARP-04, ARP-05, ARP-06
**Success Criteria** (what must be TRUE):
  1. Enabling the arpeggiator causes the four chord voices to play as a sequence of individual notes at the selected Rate (1/4, 1/8, 1/16, 1/32) and Order (Up, Down, Up-Down, Random) — confirmed in DAW MIDI monitor
  2. The arpeggiator steps fire on the active MIDI channel routing (Single Channel or Multi-Channel) — no notes appear on unintended channels
  3. With DAW sync active, turning the arp on arms it and stepping begins exactly on the next bar boundary — no early or late first note
  4. Turning the arp off stops all notes immediately and the step counter resets to 0 — the next arp-on starts from the first voice
  5. Arp note durations follow the unified Gate Length parameter — setting Gate Length to 50% causes each arp note to hold for half its subdivision slot
**Plans**: 2 plans
Plans:
- [x] 23-01-PLAN.md — PluginProcessor.cpp: timeSigNumer extraction + bar-boundary arpWaitingForPlay_ release; build verification (ARP-05 fix) (completed 2026-03-01)
- [x] 23-02-PLAN.md — Deploy VST3 + DAW smoke test checkpoint (all 6 ARP requirements) (completed 2026-03-01)

### Phase 24: Gamepad Option Mode 1
**Goal**: Option Mode 1 on the gamepad provides one-handed arp and random trigger control during live performance — Circle toggles arp, Triangle steps arp rate, Square steps arp order, X toggles RND Sync — and R3 alone no longer triggers MIDI Panic
**Depends on**: Phase 19, Phase 23
**Requirements**: OPT1-01, OPT1-02, OPT1-03, OPT1-04, OPT1-05, OPT1-06, OPT1-07
**Success Criteria** (what must be TRUE):
  1. In Option Mode 1, pressing Circle toggles the arp on/off — turning it on triggers a looper reset to beat 0 and starts playback (or arms if DAW sync is active); the arp UI reflects the state change
  2. In Option Mode 1, pressing Triangle once steps the Arp Rate up (1/4 → 1/8 → 1/16 → 1/32 → wraps); pressing it twice quickly steps it down — the Rate dropdown in the UI updates visually
  3. In Option Mode 1, pressing Square steps Arp Order with the same two-press architecture; pressing X toggles RND Sync — the RND Sync state in the UI reflects the change
  4. Holding any pad button (R1/R2/L1/L2) and pressing R3 toggles Sub Oct for that voice in any mode — the Sub Oct toggle in the UI updates visually
  5. Pressing R3 alone no longer sends MIDI Panic — panic is only accessible via the UI Panic button; Option Mode 2 (D-pad Program Change scrolling) continues to work unchanged
**Plans**: 3 plans
Plans:
- [x] 24-01-PLAN.md — GamepadInput.h/cpp: 4 new atomic signals + Mode 1 face-button detection block with double-click for Triangle/Square (completed 2026-03-01)
- [x] 24-02-PLAN.md — PluginProcessor.cpp: Mode 1 arp dispatch + looper consume gate; PluginEditor.cpp: "ARP" label + per-mode control highlight (completed 2026-03-01)
- [x] 24-03-PLAN.md — Deploy VST3 + DAW smoke test checkpoint (all 7 OPT1 requirements + visual feedback) (completed 2026-03-01)

### Phase 24.1: LFO Joystick Visual Tracking and Center Offset (INSERTED)

**Goal**: LFO and Gate Length sliders visually track the left joystick in real time; the slider base value is preserved as the center point; gate length modulation is smooth
**Depends on**: Phase 24
**Requirements**: LJOY-05, LJOY-06, LJOY-07
**Success Criteria** (what must be TRUE):
  1. When the left joystick is routed to an LFO parameter (Freq/Phase/Level) or Gate Length, the corresponding UI slider thumb moves in real time as the joystick moves — at 30 Hz via timerCallback
  2. The LFO slider's pre-joystick value defines the rest position — releasing the stick to center causes the slider to return to that base value, not stay at the last joystick position
  3. Rapidly moving the joystick while Gate Length is the target produces no audible zipper noise — the SmoothedValue ramp eliminates stepped note-off artifacts
  4. During LFO recording Playback mode, grayed-out sliders do NOT visually update even when the joystick is routed to them
**Plans**: 2 plans
Plans:
- [x] 24.1-01-PLAN.md — PluginProcessor.h/cpp: 7 display atomics, 6 override floats, SmoothedValue; LFO block override consumption; dispatch block base+offset refactor; gate smoothing
- [x] 24.1-02-PLAN.md — PluginEditor.cpp timerCallback display poll; deploy VST3; human smoke test checkpoint

### Phase 25: Distribution
**Goal**: v1.5 is publicly released on GitHub and backed up locally
**Depends on**: Phase 24
**Requirements**: DIST-03, DIST-04
**Success Criteria** (what must be TRUE):
  1. A GitHub release tagged `v1.5` exists with the installer binary attached and release notes describing the new routing, sub octave, arp, LFO recording, and bug fix features
  2. A full copy of the built plugin and installer is present on the Desktop backup location — confirmed by directory listing
  3. The installer runs successfully on a clean machine without pre-installed MSVC redistributables (static CRT confirmed)
**Plans**: 2 plans
Plans:
- [x] 25-01-PLAN.md — Update .iss for v1.5 (DIMEA branding, [Messages] section, remove LicenseFile), clean Release build, recompile installer, smoke test checkpoint (completed 2026-03-02)
- [x] 25-02-PLAN.md — Create v1.5 git tag, push to plugin remote, create GitHub pre-release, desktop backup (completed 2026-03-02)

<details>
<summary>✅ v1.8 Modulation Expansion + Arp/Looper Fixes (Phases 34-37, 44) — SHIPPED 2026-03-07</summary>

- [x] Phase 34: Cross-LFO Modulation Targets (2/2 plans) — completed 2026-03-06
- [x] Phase 35: Arp Subdivision Expansion (1/1 plans) — completed 2026-03-07
- [x] Phase 36: Arp + All Trigger Sources (1/1 plans) — completed 2026-03-07
- [x] Phase 37: Looper internalBeat_ Fix (1/1 plans) — completed 2026-03-07
- [x] Phase 44: Instrument Type Conversion (2/2 plans) — completed 2026-03-07

Full details: `.planning/milestones/v1.8-ROADMAP.md`

</details>

### v1.9 Living Interface

**Milestone Goal:** Ship v1.9 with expressive UI refinements — per-lane recording undo, smart chord display, pitch axis crosshair, velocity-based knob drag, subdivision dot indicators, warp space effect, and a proportionally resizable plugin window.

#### Phase 38: Quick Fixes & Rec Lane Undo
**Goal**: Five targeted fixes: play button stops flashing when DAW sync is armed but transport is stopped; each recording lane (Gates, Joy, Mod Whl) can be individually cleared by pressing its lit button again; LFO cross-mod level slider visually tracks when LFO X drives LFO Y's level parameter; looper is force-stopped when DAW transport stops to prevent note hang on DAW close; live joystick input (mouse drag or gamepad stick) overrides looper joystick playback with equal priority.
**Depends on**: Phase 37
**Success Criteria**:
  1. In DAW Sync mode, the play button is unlit when transport is stopped and lit solid when playing — it never flashes while armed-but-stopped
  2. Pressing Rec Gates while green immediately clears the gate buffer and turns the button gray; same behavior for Rec Joy and Rec Mod Whl independently — no cross-lane clearing
  3. When LFO X is routed to LFO Y's Level parameter, the LFO Y Level slider thumb moves visually in real time at 30 Hz
  4. When DAW transport stops while the looper is playing, the looper automatically stops in the same processBlock — note-offs for any active looper voices are emitted before the looper halts; closing the DAW after stopping transport leaves no stuck notes on the external synth
  5. While actively dragging the mouse on the JoystickPad, the looper's recorded joystick positions do not override the live cursor position — the chord follows the mouse. When the gamepad right stick is deflected, it overrides the looper joystick playback identically. Releasing mouse/stick resumes looper joystick playback. Mouse and gamepad have equal priority.
  6. The cursor burst particle effect fires for every trigger source — pad, joystick, random, looper playback, and arpeggiator — on the correct voice color. Currently looper and arp note-ons bypass voiceTriggerFlash_ and produce no burst.
  7. When filterXMode or filterYMode is set to an LFO Freq target, the LFO Rate slider is the primary base value — MOD FIX adds a normalized offset on top of it (same additive pattern as Phase and Level targets already use). The LFO Rate slider visually tracks the combined value (base + MOD FIX + stick) whenever filterModOn is true, not only when a gamepad is active. Moving MOD FIX with no gamepad connected immediately updates the displayed rate. The fix applies to all four Freq cases: X→LFO-X, X→LFO-Y (cross), Y→LFO-Y, Y→LFO-X (cross).
**Plans**: 1 plan

### Phase 38.1: Modulation Polish and CC Routing (INSERTED)

**Goal:** [Urgent work - to be planned]
**Requirements**: TBD
**Depends on:** Phase 38
**Plans:** 0 plans

Plans:
- [ ] TBD (run /gsd:plan-phase 38.1 to break down)

### Phase 38.2: Custom CC Routing (INSERTED)

**Goal:** Allow users to assign any arbitrary MIDI CC number (0–127) to the LFO CC Dest and Left Stick X/Y Mode dropdowns, instead of being limited to the predefined named list.
**Depends on:** Phase 38.1
**Plans:** 4/4 plans complete

Plans:
- [ ] 38.2-01-PLAN.md — Wave 0: failing Catch2 test stubs for CC-CUSTOM-01/02/03 + CMakeLists registration
- [ ] 38.2-02-PLAN.md — Processor: CustomCcRoutingHelpers.h, 4 APVTS params, StringArray extensions, kFilterCcNums[] OOB guards
- [ ] 38.2-03-PLAN.md — Editor: 4 inline Label members, onChange show/hide, resized() row-split, timerCallback "CC [n]" sync, INV swap extension
- [ ] 38.2-04-PLAN.md — Build + install + human smoke test (5 scenarios: custom entry, MIDI routing, preset round-trip, INV swap, all 4 combos)

### Phase 38.3: Sister LFO Attenuation Inline Slider (INSERTED)

**Goal:** When a Sister LFO cross-modulation target is selected, shrink the Sister combo to half-width and show a bipolar attenuator slider (-1..+1) in the freed right half. "None" selected = full-width combo, slider hidden.
**Requirements**: REQ-01, REQ-02, REQ-03, REQ-04, REQ-05, REQ-06, REQ-07, REQ-08
**Depends on:** Phase 38.2
**Plans:** 1 plan

Plans:
- [ ] 44-38-3-01-PLAN.md — Processor params + Editor slider construction/layout/paint + drawLinearSlider bipolar fill + Catch2 tests + UAT checkpoint

#### Phase 39: Knob UX — Velocity Drag & Visual Indicators
**Goal**: Knob interaction feels professional — slow drag gives fine control, fast drag sweeps broadly, hovering shows a subtle highlight, and octave/interval buttons display 12 subdivision dots instead of the red ring indicator.
**Depends on**: Phase 38
**Success Criteria**:
  1. Dragging slowly (< 2 px/frame) gives fine control; dragging fast (> 10 px/frame) sweeps ~3x faster — no jump on direction reversal
  2. Exponential moving average smooths drag values — no stepping artifacts on LFO rate, Cutoff, Resonance
  3. Hovering any knob shows a subtle brightness lift or highlight ring; disappears on mouse leave
  4. All 4 octave and 4 interval buttons show 12 evenly-spaced 2 px filled dots at snap positions; red position ring removed; current value shown as a brighter dot
**Plans**: 2 plans

#### Phase 40: Pitch Axis Crosshair Visualization
**Goal**: Two subtle lines extend from the cursor to the joystick pad edges with quantized note names — giving the player immediate pitch feedback without cluttering the space visual.
**Depends on**: Phase 38
**Success Criteria**:
  1. Horizontal line to Y axis: root note name above the line, third note name below — both update in real time
  2. Vertical line to X axis: fifth note name on left side, tension note name on right — both update in real time
  3. Lines at ~30% alpha in voice accent color; not rendered when cursor is at center or pad inactive
  4. Note names reflect quantized pitches (post-scale quantization)
**Plans**: 1 plan

#### Phase 41: Smart Chord Display
**Goal**: The chord display always shows the current chord quality built from the root up — inferring the third from scale when Voice 1 is not triggered — and only updating on an active trigger.
**Depends on**: Phase 40
**Success Criteria**:
  1. Root from Voice 0 (Y axis); quality built upward through voices 1–3 — always root-relative (e.g. "Cm7", "Gmaj7#11", "D6")
  2. When Voice 1 not triggered, third inferred from active scale pattern relative to root — correct minor/major quality shown
  3. Display retains last chord name during silence — no update until next trigger fires
  4. Correctly identifies: minor, major, dom7, maj7, m7, 6th, sus2/sus4, tension extensions (#11, b9, #9)
**Plans**: 1 plan

#### Phase 42: Warp Space Effect
**Goal**: When the looper enters playback mode, the joystick pad background transforms into a cinematic warp tunnel with 4000ms ease-in ramp.
**Depends on**: Phase 31 (starfield foundation)
**Success Criteria**:
  1. Looper play start → Z-velocity ramps to full warp over 4000ms smooth ease-in
  2. At full warp: stars drawn as stretched lines (length ∝ speed); long edge streaks, short center dashes
  3. Tunnel perspective: dim small at center → bright large at edge; vanishing point at center
  4. Center axis stars shift to blue-white (#aaddff); peripheral stars warm white — radial Doppler blend
  5. Looper stop → ramps back down over 4000ms; no sudden cut
  6. Cursor and all UI on top; warp strictly behind joystick pad bounds
**Plans**: 2 plans

#### Phase 43: Resizable UI
**Goal**: Plugin window resizes proportionally from 0.5x to 2.0x with locked aspect ratio — remembered across sessions.
**Depends on**: Phase 38
**Success Criteria**:
  1. Dragging window corner in DAW resizes with locked aspect ratio — no overlap or distortion at any scale
  2. At 0.5x all controls remain clickable and text readable; at 2.0x nothing overflows
  3. Resizing produces no MIDI output or parameter changes
  4. Scale factor persists across plugin save/load
**Plans**: 2 plans

## Progress

| Phase | Milestone | Status | Completed |
|-------|-----------|--------|-----------|
| 01–07 | v1.0 | Complete | 2026-02-23 |
| 09–11 | v1.3 | Complete | 2026-02-25 |
| 12–16 | v1.4 | Complete | 2026-02-26 |
| 17–25 | v1.5 | Complete | 2026-03-02 |
| 26–30 | v1.6 | Complete | 2026-03-03 |
| 31–33.1 | v1.7 | Complete | 2026-03-05 |
| 34–37, 44 | v1.8 | Complete | 2026-03-07 |
| 38–43 | v1.9 | Planned | — |
