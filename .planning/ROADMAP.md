# ChordJoystick — Roadmap

## Milestones

- ✅ **v1.0 MVP** — Phases 01-07 (shipped 2026-02-23)
- ✅ **v1.3 Polish & Quantization** — Phases 09-11 (shipped 2026-02-25)
- ✅ **v1.4 LFO + Clock** — Phases 12-16 (shipped 2026-02-26)
- ✅ **v1.5 Routing + Expression** — Phases 17-25 (shipped 2026-03-02)
- ✅ **v1.6 Triplets & Fixes** — Phases 26-30 (shipped 2026-03-03)
- 🔲 **v1.7 Space Joystick** — Phases 31-38 (active)

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

### v1.6 Triplets & Fixes

**Milestone Goal:** Fix the noteCount stuck-notes bug and default param values, add triplet subdivisions to both random and quantize selectors, correct Random Free to fire truly outside the tempo grid when RND SYNC is off, and replace the linear looper progress bar with a rectangular perimeter visualization.

- [x] **Phase 26: Defaults and Bug Fix** — Default octave values (Third=4, Fifth=4, Tension=3) + Natural Minor default + noteCount_ clamp removal from all 13 note-off paths (completed 2026-03-02)
- [x] **Phase 27: Triplet Subdivisions** — 1/1T through 1/32T added to random trigger and quantize subdivision selectors; APVTS enum extended; UI updated (completed 2026-03-03)
- [x] **Phase 28: Random Free Redesign** — Poisson clock + 64-note burst model; three-branch sync matrix: Poisson (SYNC OFF), DAW ppq (SYNC ON + playing), internal counter (SYNC ON + stopped) (completed 2026-03-03)
- [x] **Phase 29: Looper Perimeter Bar** — Clockwise 2px green perimeter bar replaces horizontal strip; ghost ring at idle; LOOPER label excluded from clip at all times (completed 2026-03-03)
- [ ] **Phase 30: Distribution** — GitHub v1.6 release with installer binary + release notes; desktop backup

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

---

### Phase 26: Defaults and Bug Fix
**Goal**: The plugin loads with musically sensible default octave values and Natural Minor scale on a fresh install, and the noteCount_ reference-counting clamp is removed so that single-channel mode no longer produces stuck notes when two voices land on the same pitch
**Depends on**: Phase 25
**Requirements**: DEF-01, DEF-02, DEF-03, DEF-04, BUG-03
**Success Criteria** (what must be TRUE):
  1. Loading the plugin with no saved state (fresh install or initialized preset) shows Third octave = 4, Fifth octave = 4, Tension octave = 3 in the UI — verified by checking the knob display values before any user interaction
  2. Loading the plugin with no saved state defaults the scale preset dropdown to Natural Minor — the scale keyboard reflects the Natural Minor pattern immediately on load
  3. In Single Channel mode, pressing two pads that produce the same MIDI pitch simultaneously and then releasing both results in exactly one note-off with no stuck note — verified in DAW MIDI monitor across all 5 note-off paths (live, looper, looper sub-octave, live sub-octave, arp choke)
  4. All existing preset files (v1.5 and earlier) load without regression — octave and scale defaults only apply when no saved value is present
**Plans**: 1 plan
Plans:
- [x] 26-01-PLAN.md — PluginProcessor.cpp: fifthOctave default 3→4, scalePreset default 0→1, remove all 13 else noteCount_ = 0 clamp branches, build + smoke test checkpoint (completed 2026-03-02)

### Phase 27: Triplet Subdivisions
**Goal**: Players can select triplet rhythms (1/1T through 1/32T) in both the random trigger and quantize subdivision selectors, enabling triplet-feel gate patterns and quantize snapping without any workarounds
**Depends on**: Phase 26
**Requirements**: TRIP-01, TRIP-02
**Success Criteria** (what must be TRUE):
  1. The random trigger subdivision selector for each voice shows 1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T alongside the existing straight subdivisions — all 12 options save and reload correctly with a preset
  2. The quantize subdivision selector shows the same 6 triplet options alongside existing straight subdivisions — quantize snapping at 1/8T produces gate events that land on the correct triplet grid when verified in a DAW piano roll
  3. Selecting a triplet subdivision in Random Free mode with RND SYNC ON produces gate onsets spaced at exactly 2/3 of the equivalent straight note duration — confirmed against the DAW MIDI event timestamps
**Plans**: 1 plan
Plans:
- [ ] 27-01-PLAN.md — Backend enum/APVTS/dispatch extension + UI ComboBox update + human timing verification

### Phase 28: Random Free Redesign
**Goal**: Random Free trigger behavior is unambiguously determined by two controls — RND SYNC and DAW Sync — so that musicians get truly stochastic timing when SYNC is off, internal-clock grid when SYNC is on, and DAW grid when DAW Sync is active
**Depends on**: Phase 27
**Requirements**: RND-08, RND-09, RND-10
**Success Criteria** (what must be TRUE):
  1. Random Free + RND SYNC OFF: gate onsets arrive at irregular, non-grid-aligned intervals — consecutive inter-onset intervals differ from one another; no two consecutive gates land on the same beat subdivision when checked in a DAW MIDI monitor
  2. Random Free + RND SYNC ON + DAW Sync OFF: gates fire on the internal free-tempo clock grid at the selected subdivision; onset timing matches the beat clock dot period — verified by comparing onset timestamps to expected subdivision durations at the current free BPM
  3. Random Free + DAW Sync ON: gates fire on the DAW beat grid at the selected subdivision regardless of the RND SYNC toggle state — behavior matches pre-v1.6 RND SYNC ON behavior
**Plans**: 2 plans
Plans:
- [ ] 28-01-PLAN.md — TriggerSystem.h/cpp: burst arrays, Poisson clock block, RandomFree/RandomHold burst rewrite, build verification
- [ ] 28-02-PLAN.md — Deploy VST3 + DAW smoke test checkpoint (all 3 sync matrix behaviors + burst mechanics)

### Phase 29: Looper Perimeter Bar
**Goal**: The looper progress visualization is a clockwise rectangular bar that travels around the perimeter of the Looper section box — immediately readable as a clock-face metaphor — with the section label always fully visible
**Depends on**: Phase 26
**Requirements**: LOOP-01, LOOP-02, LOOP-03, LOOP-04
**Success Criteria** (what must be TRUE):
  1. The linear progress strip below the Rec gates is gone — no horizontal bar renders in that position during looper playback
  2. A colored bar travels clockwise around the outer edge of the Looper section rectangle at 30 Hz, completing exactly one circuit per loop cycle — observable by watching the bar complete a full lap in sync with the DAW transport
  3. The bar starts and ends its circuit at the top-left corner (Looper label area), travels right along the top edge, down the right edge, left along the bottom edge, and up the left edge — direction confirmed by watching a slow loop (e.g. 4-bar at 60 BPM)
  4. The "LOOPER" section label text remains fully legible at all times — the bar does not paint over or occlude the label characters at any point in the circuit
**Plans**: 1 plan
Plans:
- [x] 29-01-PLAN.md — PluginEditor.cpp paint/resized/timerCallback: remove horizontal strip, add ghost ring + clockwise perimeter bar with tail and label exclusion; human visual smoke-test checkpoint (completed 2026-03-03)

### Phase 30: Distribution
**Goal**: v1.6 is publicly released on GitHub with an updated installer and backed up locally
**Depends on**: Phase 29
**Requirements**: DIST-05, DIST-06
**Success Criteria** (what must be TRUE):
  1. A GitHub release tagged `v1.6` exists with the installer binary attached and release notes listing the defaults fix, noteCount bug fix, triplet subdivisions, Random Free redesign, and looper perimeter bar
  2. A full copy of the built plugin and installer is present on the Desktop backup location — confirmed by directory listing
  3. The installer runs successfully on a clean machine and the plugin loads in Ableton Live with no stuck notes on first use in Single Channel mode
**Plans**: 2 plans
Plans:
- [ ] 30-01-PLAN.md — Promote v1.5 to finished release, update .iss to v1.6, clean build, recompile installer, smoke test checkpoint
- [ ] 30-02-PLAN.md — Create v1.6 git tag, push to plugin remote, create GitHub Latest release, desktop backup

### Phase 31: Paint-Only Visual Foundation
**Goal**: The joystick pad has a realistic deep-space visual identity — milky way band, density-driven starfield, heatmap overlay, semitone grid, and a beat-synced breathing cursor glow — implemented as pure paint operations with zero audio/MIDI changes
**Depends on**: Phase 30
**Requirements**: VIS-03, VIS-07, VIS-12
**Plans**: 2 plans
Plans:
- [ ] 31-01-PLAN.md — PluginEditor.h/cpp: add resized() + new members + bake milkyWayCache_/starfield_/heatmapCache_ + rewrite paint() layers 1-6
- [ ] 31-02-PLAN.md — JoystickPad::timerCallback() glowPhase_ advancement + PluginEditor resetGlowPhase() beat wiring + human visual checkpoint


## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 01–07 | v1.0 | 17/17 | Complete | 2026-02-23 |
| 09–11 | v1.3 | 11/11 | Complete | 2026-02-25 |
| 12–16 | v1.4 | 9/9 | Complete | 2026-02-26 |
| 17–25 | v1.5 | 25/25 | Complete | 2026-03-02 |
| 26. Defaults and Bug Fix | v1.6 | Complete    | 2026-03-02 | 2026-03-02 |
| 27. Triplet Subdivisions | v1.6 | Complete    | 2026-03-03 | - |
| 28. Random Free Redesign | v1.6 | 1/1 | Complete | 2026-03-03 |
| 29. Looper Perimeter Bar | v1.6 | Complete    | 2026-03-03 | 2026-03-03 |
| 30. Distribution | 1/2 | In Progress|  | - |
| 31. Paint-Only Visual Foundation | 2/2 | Complete   | 2026-03-04 | - |
