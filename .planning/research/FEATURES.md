# Feature Landscape — ChordJoystick v1.5

**Domain:** MIDI generator VST3 plugin — live performance / composition tool
**Researched:** 2026-02-28
**Milestone:** v1.5 Routing + Expression
**Confidence:** HIGH (codebase directly inspected; MIDI design claims verified against MIDI specification and KVR community knowledge)

---

> **Scope note:** This document covers the NEW features for v1.5.
> Previous milestone research (v1.4 LFO + Clock) is preserved at the bottom as a historical appendix.

---

## Context

The seven v1.5 features plus two bug fixes are analysed here through the lens of:
- Table stakes vs differentiator vs anti-feature classification
- User-facing behavior expectations
- Critical edge cases per feature
- Dependencies on existing subsystems (LooperEngine, LfoEngine, TriggerSystem, GamepadInput, PluginProcessor)

**Existing subsystem summary relevant to v1.5:**
- `LooperEngine` — lock-free SPSC FIFO, beat-timestamped events, AbstractFifo double-buffer, 2048 capacity
- `LfoEngine` — stateless processBlock() DSP, ProcessParams-driven, 7 waveforms, no APVTS coupling
- `TriggerSystem` — 4-voice gate, pad/joystick/random sources, NoteCallback per voice
- `GamepadInput` — SDL2 60 Hz timer, optionMode_ (0/1/2), R3 = rightStickTrig_, held state per voice
- `PluginProcessor` — arpeggiator state (arpPhase_, arpStep_, etc.), heldPitch_[4], looperActivePitch_[4]
- Filter mode boxes — filterXMode/filterYMode APVTS enums, currently 2–3 items each

---

## Table Stakes

Features that must work correctly once they exist. Wrong behavior makes the feature feel broken.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Single-channel mode: all 4 voices to one MIDI channel | Mono synths and non-MPE targets require single-channel output; users expect a clean toggle | Medium | Requires note-collision guard — see edge cases |
| Single-channel note collision: same pitch on same channel → extend gate, not retrigger | Standard MIDI behavior expectation; sending note-on for a sounding pitch on same channel causes stuck notes or envelope reset in most synths | Medium | Reference counting per active pitch is the safe implementation |
| Sub octave: fires exactly -12 semitones below the parent voice pitch | "Sub octave" has a precise meaning to musicians — exactly one octave lower, not user-adjustable | Low | Fixed -12 offset; not a tunable interval |
| Sub octave: follows the same gate (on/off timing) as the parent voice | Lower note must sound and stop together with its parent | Low | Use the same sampleOffset in the note-on/off callback |
| Sub octave: persists across state save/load | APVTS bool per voice | Low | Standard APVTS bool parameter |
| LFO recording: records exactly 1 loop cycle | Gesture automation must match the looper's loop length; arbitrary buffer length is confusing | Medium | Tie to looper's `getLoopLengthBeats()` |
| LFO recording: Distort stays live during playback | Distort is a post-waveform noise shaper; users expect to sculpt the playback in real time | Low | Distort is a ProcessParams field, never stored in the recording buffer |
| LFO playback: shape/freq/phase/level controls grayed out (not Distort) | Only params that define the recorded waveform should be locked | Low | UI-only: gray out 4 controls; leave Distort and on/off toggle enabled |
| Arpeggiator DAW sync: already built | Arp already exists with subdivisions and random order; gamepad control is the only new part | Low | Gamepad routing change only — no new DSP |
| Looper wrong start position: bug fix | Looper must replay events aligned to the beat boundary where REC started; wrong offset is a correctness regression | Medium | loopStartPpq_ anchor re-entrancy after record cycle end |

---

## Differentiators

Features that add expressive value beyond user expectation.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Per-voice sub octave (not a global octave shift) | Lets user add bass doubling on the root voice without affecting the third — useful for bass+chord splits | Low | 4 APVTS bools subOctVoice0..3; UI: right-half of Hold button becomes SubOct toggle |
| Sub octave + single-channel collision avoidance | When sub oct note = another voice's note on same channel, extend rather than retrigger — prevents mud | Medium | Requires cross-voice pitch comparison at note-on time in processBlock |
| LFO recording arm/record/play/clear workflow | Captures a custom LFO shape from live performance without leaving the plugin; gesture automation | High | Largest new subsystem; beat-indexed float buffer per axis, new state machine |
| Left joystick target expansion: LFO freq/shape/level/arp gate | Turns the left stick into a live modulation surface for internal plugin parameters beyond CC74/CC71 | Medium | Extend filterXMode/filterYMode APVTS enum by 4–6 values; new processBlock routing switch cases |
| Gamepad Option Mode 1: arp controls on face buttons | Enables arp on/off, rate, and order changes during performance without touching the mouse | Low | Pure GamepadInput + processBlock routing change; no new DSP |
| R3 + held pad = sub oct toggle for that voice | Ergonomic: hold a voice with one hand, toggle its sub oct with R3 + that pad | Low | Additive condition on existing gamepad held-state check; evaluate before R3-standalone |

---

## Anti-Features

Features to explicitly NOT build in v1.5.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Sub octave as a tunable interval (not fixed -12) | "Sub octave" has a precise meaning; tunable offset creates UI scope creep for marginal benefit | Fix at -12 semitones; add a named "voice octave offset" knob separately if needed |
| LFO recording: selectable multi-cycle length | Recording exactly 1 loop cycle matches the looper's mental model; multi-cycle adds sync complexity | Lock to 1 cycle; if longer is needed, extend the looper loop length first |
| LFO recording: record the Distort parameter | Distort is a live-shaping overlay; recording it breaks the "Distort stays live" contract | Keep Distort as a real-time overlay; never include it in the recording buffer |
| Arp + looper: force looper to stop when arp turns on | Arp and looper are designed to run simultaneously; muting one on the other's activation is surprising | Let them run independently; handle note-collision cases with single-channel guard |
| Single-channel mode: remove multi-channel mode | Multi-channel is the v1.0 default; it must remain the default; single-channel is opt-in | Toggle in UI + APVTS bool singleChannelMode, default false |
| Remove R3 panic globally | Panic is safety-critical; removing it entirely is dangerous for live performance | Remove R3 standalone panic only when in Option Mode 1; panic remains available via UI button and via modes 0 and 2 |
| Gamepad Option Mode 1: D-pad mapping for LFO params | D-pad is already used for octave/transpose control in modes 1 and 2; adding more sub-modes creates a menu jungle | Stick to face-button remapping only in v1.5; revisit D-pad extensions in v2 |

---

## Edge Cases by Feature

### Feature 1: Single-Channel Routing Mode

**Edge case A — Note collision (same pitch, same channel):**
Standard MIDI does not define behavior when a note-on arrives for a pitch already sounding on the same channel. Most synths either retrigger (resetting the envelope, which is usually unwanted) or create a stuck note (never hearing the second note-off that matches this note-on). The safe implementation: maintain a `singleChActiveCount[128]` reference count array for the target channel in processBlock. On note-on: if count > 0, skip the note-on but increment the count. On note-off: decrement count; only send MIDI note-off when count reaches zero.

Confidence: MEDIUM — derived from KVR community consensus ("reference counting is the safe approach") and MIDI specification's undefined behavior for duplicate note-ons.

**Edge case B — Sub octave + single-channel collision:**
Voice 0 sounds MIDI 48; its sub octave is MIDI 36. Voice 1 happens to also compute MIDI 36. In single-channel mode both would emit note-on 36. Use the same reference-count guard to prevent double note-on.

**Edge case C — Mode toggle with notes held:**
If the user switches single-channel mode while a note is sounding, the note-off must be sent on the original channel used at note-on time (not the new routing). Track per-voice "channel used at last note-on" alongside activePitch_ — same pattern already used for looperActivePitch_.

**Edge case D — Channel selection in single-channel mode:**
Use voiceCh0 (root voice APVTS parameter, already exists) as the single output channel when singleChannelMode is active. Avoids adding a new APVTS parameter and is intuitive (voice 0 = the "main" voice).

---

### Feature 2: Sub Octave Per Voice

**Edge case A — Sub note MIDI range clamping:**
If voice pitch is 0–11 (MIDI notes 0–11), subtracting 12 produces a negative value. Clamp to MIDI note 0 minimum. No user interaction needed; silent floor clamping is correct.

**Edge case B — Sub note with Hold mode:**
The existing padHold_ flag keeps the parent note sounding when mouseUp fires. Sub octave uses the same gate-open state as its parent voice — the hold behavior is automatic with no separate tracking.

**Edge case C — Sub note with Arpeggiator:**
The arpeggiator cycles individual voices, firing note-on via the TriggerSystem NoteCallback. Sub-octave emission must occur in the same callback branch as the main arp note-on, not as a separate later step. Order: fire main note, then immediately fire sub-oct note with the same sampleOffset.

**Edge case D — Sub note with Looper playback:**
Looper replays gate events from playbackStore_ which contain voice index and beatPosition — but not sub-octave state. Sub octave is a real-time APVTS parameter. Apply sub-octave doubling at the point of looper gate-on emission in processBlock, reading subOctVoiceN at playback time (not recording time). This means changing the sub-oct APVTS knob mid-playback takes effect immediately — which is the expected live behavior.

**Edge case E — R3 + held pad toggle mid-gate:**
When R3 is pressed while a voice pad is physically held, toggle subOctVoiceN for that voice. Do NOT retroactively add or remove a currently sounding sub-octave note; that would produce a stuck note or orphan note-off. The toggle takes effect on the next note-on only. This is acceptable because the user is changing behavior for the next hit, not the current one.

---

### Feature 3: LFO Recording

**Behavior contract (state machine per LFO axis):**
- IDLE → ARM: arm button pressed; LFO output monitoring begins; no buffer write yet
- ARM → RECORDING: looper enters playing state (or manual trigger if looper not running); LFO process() output sampled into a beat-indexed buffer for exactly 1 loop cycle
- RECORDING → PLAYBACK: 1 cycle elapsed; buffer locked; LfoEngine.process() bypassed; buffer provides LFO value at each block via beat-position lookup
- PLAYBACK → IDLE: Clear button pressed; buffer discarded; LfoEngine.process() resumes free-running

**Edge case A — Buffer size calculation:**
The recording resolution is one sample per processBlock (block-rate, not sample-rate). LFO values do not need sample-accurate timestamping. At 120 BPM, 16-bar 4/4 loop = 64 beats = 32 seconds. At 512-sample blocks and 44100 Hz, that is ~2755 blocks. A fixed buffer of 4096 `(float beatPosition, float lfoValue)` pairs (two float per entry = 32 KB per axis) covers all practical loop lengths. This is the same pattern as LooperEngine's playbackStore_ but for LFO values instead of gate events.

Use a `std::array<std::pair<float,float>, 4096>` or two parallel float arrays. Store as (beatPosition, value) pairs sorted by beatPosition so playback can binary-search or linearly scan.

**Edge case B — Playback interpolation:**
Recorded samples are spaced at processBlock intervals — approximately every 5–12 ms. At slow LFO rates (0.1 Hz) this is fine. At fast rates (10 Hz) with small blocks (64 samples), the recording resolution may be coarser than the LFO output. Linear interpolation between adjacent (beatPos, value) pairs eliminates zipper artifacts. This is low-cost and should be implemented from the start.

**Edge case C — Tempo change during playback:**
The buffer stores beat positions, not sample positions. Playback reads the buffer by mapping current playbackBeat_ (from LooperEngine) to the nearest recorded beat position. If the DAW BPM changes, the beat-position mapping stays correct — the playback speed just changes in musical time, which is correct behavior (same as how the looper itself behaves).

**Edge case D — Gray-out scope during playback:**
Gray out (disable UI): LFO shape combo, rate slider, phase slider, level slider.
Leave active (not grayed out): Distort slider, LFO on/off toggle, arm/record/clear buttons.
Rationale: Distort is explicitly stated to remain live. The on/off toggle must stay active so the user can bypass the whole LFO (including the recording) if needed.

**Edge case E — Sync button interaction with recording:**
If LFO sync is on, recording should start at the next bar boundary (same as looper's recPendingNextCycle_ pattern). If sync is off, recording starts immediately when ARM transitions to RECORDING.

**Edge case F — Two independent recording instances (LFO X and LFO Y):**
Each axis has its own recording buffer and state machine. Implement as a `LfoRecorder` struct (or nested class) held by PluginProcessor (one instance: lfoXRecorder_, one: lfoYRecorder_). Keep LfoEngine itself stateless-ish and unmodified. PluginProcessor stitches: if recorder is in PLAYBACK state, ignore lfoX_.process() output and use recorder's buffer output instead.

---

### Feature 4: Left Joystick X/Y Modulation Target Expansion

**Current targets (filterXMode / filterYMode APVTS enums):**
- X: Cutoff (CC74), VCF LFO (CC12), Mod Wheel (CC1)
- Y: Resonance (CC71), LFO Rate (CC76)

**New targets to add (6 new enum values, 3 per axis):**
- LFO-X Freq, LFO-X Shape, LFO-X Level (new X axis targets)
- LFO-Y Freq, LFO-Y Shape, LFO-Y Level (new Y axis targets, add to both dropdowns)
- Arp Gate Length (add to both X and Y dropdowns)

**Edge case A — Shape is a discrete enum, not a float:**
LFO shape is Waveform integer (0–6). Left stick output is float -1..+1. Map by dividing the ±1 range into 7 equal bands (each band is 2/7 wide). Apply hysteresis (±0.04) at each boundary to prevent rapid flickering when the stick hovers on a boundary. This is the standard approach for continuous-to-step mapping in hardware synthesizers.

**Edge case B — LFO Freq target when sync is on:**
When LFO sync mode is enabled, the rate parameter is in beat subdivisions (not Hz). Driving the sync subdivision with an analog stick would produce confusing jumps between discrete time signatures. If the target is "LFO-X Freq" and sync is on, suppress the CC output (or display a warning label). The user should turn off sync before using the left stick as LFO rate.

**Edge case C — Arp Gate Length mapping:**
arpGateTime APVTS is 0.0–1.0. Left stick -1..+1 maps to 0..1 via `(value + 1.0f) * 0.5f`. Direct float assignment to the APVTS parameter. No special handling needed; this is the cleanest of the new targets.

**Edge case D — Multi-target conflict:**
Both X and Y dropdowns could select the same target (e.g., both targeting LFO-X Level). Allowed — last write wins within processBlock. No need to prevent it.

---

### Feature 5: Gamepad Option Mode 1 Remapping

**New face-button mapping in Option Mode 1:**
- Circle (B) → Arp on/off toggle
- Triangle (Y) → Arp Rate: cycle subdivision forward (wraps 0→5→0)
- Square (X) → Arp Order: cycle order forward (wraps 0→6→0)
- Cross (A) → RND Sync toggle (intercept before looper start/stop dispatch)
- R3 (right stick btn) → sub oct toggle for held pad (no longer panic in mode 1)

**Edge case A — Circle toggles arp; arp starts waiting for DAW:**
Existing behavior: when arpEnabled is set to true while DAW is not playing, `arpWaitingForPlay_` becomes true (blink state). The existing blink counter handles this. Circle just sets the APVTS arpEnabled bool using `p->setValueNotifyingHost()` — no new state needed.

**Edge case B — R3 standalone panic removal in mode 1 only:**
In processBlock, the current logic fires panic on `consumeRightStickTrigger()` regardless of mode. Add a guard: `if (optionMode != 1 && consumeRightStickTrigger()) { ... panic ... }`. In mode 1, R3 is consumed only in combination with a held pad (for sub oct toggle) and ignored when no pad is held.

**Edge case C — Cross intercept for RND Sync in mode 1:**
Cross (A) in mode 0 = looper start/stop. In mode 1, Cross = RND Sync toggle. Implement as: before the normal looper start/stop branch, check `if (optionMode == 1 && consumeLooperStartStop()) { toggle randomClockSync APVTS bool; return; }`. The `consumeLooperStartStop()` is a destructive consume so it cannot fire twice.

**Edge case D — Rate/Order cycling:**
Triangle → increment `arpSubdiv` APVTS index cyclically within [0, 5]. Square → increment `arpOrder` APVTS index cyclically within [0, 6]. Both use the existing `stepWrappingParam()` helper in PluginProcessor. These consume the option-mode D-pad delta signals — use the same consume pattern: check `consumeOptionDpadDelta()` or add a new option-button-delta mechanism for face buttons in GamepadInput.

Actually: Triangle/Square/Circle are not D-pad buttons — they are face buttons. The D-pad delta mechanism (pendingOptionDpadDelta_) is for D-pad only. For face buttons in option mode, add direct consume flags: `pendingOptionCircle_`, `pendingOptionTriangle_`, `pendingOptionSquare_` as `std::atomic<bool>` in GamepadInput, set on rising edge when optionMode == 1.

---

### Feature 6: Bug Fix — Looper Wrong Start Position After Record Cycle

**Root cause (inferred from LooperEngine.h):**
`loopStartPpq_` is set when the looper begins playing/recording to anchor beat 0. After `finaliseRecording()` runs and the looper transitions from recording to playback, `internalBeat_` is reset but the anchor may not re-snap to the nearest bar boundary. If the PPQ position at the moment of transition is mid-bar, the first playback cycle starts from a fractional beat offset instead of beat 0.

**Expected fix:** After `finaliseRecording()` completes and playback begins, compute the PPQ distance from the current position to the last bar boundary and reset `loopStartPpq_` to that bar boundary. This ensures the first playback pass starts at beat 0 of the recorded material.

**Edge case — DAW loop playback (Ableton clip loop):**
If the DAW loops its own timeline, PPQ position resets backward. The looper must detect backward PPQ jumps (current ppq < previous ppq by more than half a beat) and re-anchor loopStartPpq_ to the new position. This avoids the looper drifting out of sync after a DAW loop point.

---

### Feature 7: Bug Fix — SDL2 Bluetooth Reconnect Crash

**Root cause (inferred from GamepadInput.h):**
The `timerCallback()` calls `tryOpenController()` on SDL_CONTROLLERDEVICEADDED events. If a disconnect and reconnect occur in quick succession (BT pairing oscillation), two ADD events may fire before the first `closeController()` has fully executed. The previous `SDL_GameController*` handle may still be non-null but invalidated, causing a crash in the next SDL_GameControllerOpen call or in axis polling.

**Expected fix:**
1. Add a `lastCloseTimestampMs_` member. In `tryOpenController()`, return early if `SDL_GetTicks() - lastCloseTimestampMs_ < 500` — a 500 ms debounce on reconnect.
2. In `closeController()`, set `lastCloseTimestampMs_ = SDL_GetTicks()` before nulling the pointer.
3. Null-guard all SDL_GameController* accesses at the top of `timerCallback()`.

**Edge case — Multiple controllers connected simultaneously:**
SDL may report multiple ADD events if a second controller is connected. GamepadInput currently tracks only one controller_. Add a check in tryOpenController(): if controller_ is already non-null and valid, do not open a second one.

---

## Feature Dependencies

```
Single-channel mode
  → new: singleChannelMode APVTS bool (default false)
  → new: singleChActiveCount[128] reference count in processBlock (audio thread only)
  → uses: voiceCh0 APVTS param as target channel (existing)
  → used by: sub octave collision guard

Sub octave per voice
  → new: subOctVoice0..3 APVTS bools
  → uses: heldPitch_[4] array (existing)
  → uses: NoteCallback in TriggerSystem (existing — fire sub note in same callback)
  → requires single-channel collision guard when singleChannelMode is active
  → UI: split Hold button right-half into SubOct region (per-voice TouchPlate area)
  → UI: R3 + held pad shortcut in GamepadInput processBlock routing

LFO recording
  → new: LfoRecorder struct (beat-indexed float buffer + state machine, one per axis)
  → uses: LfoEngine.process() output (existing — recorder intercepts it)
  → uses: looper.getLoopLengthBeats() (existing)
  → UI: ARM button near LFO Sync; CLEAR button; gray-out logic for 4 LFO controls

Left joystick target expansion
  → extends: filterXMode / filterYMode APVTS enum values (add 4–6 new values)
  → uses: LfoEngine ProcessParams (existing — new case in processBlock switch)
  → uses: arpGateTime APVTS param (existing)
  → new: hysteresis + step mapping for LFO Shape discrete target

Gamepad Option Mode 1 remapping
  → new: pendingOptionCircle_, pendingOptionTriangle_, pendingOptionSquare_ atomics in GamepadInput
  → uses: stepWrappingParam() (existing)
  → uses: arpEnabled APVTS (existing)
  → changes: R3 routing guard (optionMode != 1 precondition before panic)
  → changes: Cross routing guard (optionMode == 1 intercept for RND Sync)
  → depends on: sub octave (R3+pad gesture)

Bug fix: looper start position
  → changes: LooperEngine — loopStartPpq_ re-anchor logic after finaliseRecording()
  → changes: backward PPQ jump detection

Bug fix: BT reconnect crash
  → changes: GamepadInput — lastCloseTimestampMs_ debounce in tryOpenController()
  → changes: null-guard in timerCallback()
```

---

## MVP Build Order for v1.5

Build in this sequence to respect dependencies and validate foundational changes before layering complexity:

1. **Bug fixes first** — Looper start position and BT crash are live correctness issues. Fix these before adding features that stress these same systems.

2. **Single-channel routing mode** — Foundational routing change. The reference-count collision guard is reused by sub octave. Low risk: it is additive to the existing multi-channel path.

3. **Sub octave per voice** — Depends on single-channel collision guard. The DSP is 4 extra note-on/off calls per trigger. The UI split-button is the highest-effort part; it is self-contained.

4. **Left joystick target expansion** — Extends the existing filterXMode/filterYMode switch statement. Low integration risk. Delivers user-visible expressive value early.

5. **Gamepad Option Mode 1 remapping** — Pure routing change. Depends on sub octave (R3+pad gesture). Requires new atomic flags in GamepadInput for face buttons.

6. **LFO recording** — Highest complexity. Build last so foundational features are stable. Requires the most new state and the most edge-case validation (interpolation, gray-out, sync timing). Can be delivered in a 1.5.1 patch if it blocks the milestone.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Single-channel note collision: reference count approach | MEDIUM | KVR community consensus; MIDI spec is undefined for this case; reference counting is the most widely cited safe approach |
| Sub octave -12 semitone fixed offset | HIGH | "Sub octave" is a well-defined musical term; no ambiguity |
| Sub octave gate timing (same sampleOffset as parent) | HIGH | Direct from TriggerSystem NoteCallback design in codebase |
| LFO recording buffer size (4096 beat-indexed pairs) | HIGH | First-principles calculation from looper max 16 bars, min 30 BPM, 512-sample blocks |
| LFO recording Distort stays live | HIGH | Explicitly stated in milestone spec; Distort is a ProcessParams field not stored in buffer |
| Left joystick shape discrete mapping (7 bands + hysteresis) | MEDIUM | Standard hardware synth pattern; no official VST plugin spec for this |
| Gamepad Option Mode 1 face-button atoms needed | HIGH | Direct consequence of how GamepadInput currently exposes D-pad-only option deltas |
| Looper start-position root cause | MEDIUM | Inferred from LooperEngine.h anchor logic; not confirmed by running code |
| BT reconnect crash root cause | MEDIUM | Inferred from GamepadInput.h + SDL2 timer thread patterns; not confirmed by repro |

---

## Sources

- Codebase inspection: `Source/PluginProcessor.h`, `Source/LooperEngine.h`, `Source/LfoEngine.h`, `Source/TriggerSystem.h`, `Source/GamepadInput.h`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- MIDI same-pitch collision: KVR Audio DSP forum — "Midi events with note on and note off at the same time" (reference count approach is community consensus) — MEDIUM confidence
- MIDI specification undefined duplicate note-on: midi.teragonaudio.com Note-On specification — HIGH confidence
- Arpeggiator loop start-position quirk: Ableton Forum — "Arpeggiator problem" thread — MEDIUM confidence (same symptom as the looper bug)
- Gamepad mode design (dual-purpose buttons): ControllerBuddy design documentation — MEDIUM confidence
- LFO buffer calculation: first-principles from codebase constants (looper 16 bars, 512 block, 30 BPM minimum) — HIGH confidence

---

---

## Appendix: v1.4 Feature Research (Historical — 2026-02-26)

> Preserved for roadmap continuity. All v1.4 features were validated and shipped.

**Domain:** Plugin LFO engine for a JUCE VST3 MIDI performance instrument
**Researched:** 2026-02-26
**Milestone:** ChordJoystick v1.4 — LFO + Clock
**Overall confidence:** HIGH for waveform math and S&H/Random distinction; HIGH for sync/phase reset behavior; MEDIUM for distortion interpretation

### Table Stakes (v1.4)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Sine waveform | Default LFO shape; smooth, organic | Low | `sin(2π * φ)` |
| Triangle waveform | Standard LFO shape; linear | Low | `(φ < 0.5) ? (4φ - 1) : (3 - 4φ)` |
| Sawtooth Up / Down | Classic filter sweeps | Low | `2φ - 1` / `1 - 2φ` |
| Square waveform | Rhythmic gating | Low | `φ < 0.5 ? +1 : -1` |
| Sample & Hold | Stepped random; staircase voltage | Medium | New random value at each cycle boundary |
| Random (smooth) | Interpolated S&H; organic drift | Medium | Linear interpolation between random targets |
| Frequency slider | Fundamental LFO parameter | Low | Hz free mode / BPM subdivision sync mode |
| Level (depth) | Controls modulation amount | Low | Scales output ∈ [-1,+1] |
| On/Off toggle | Bypass without losing state | Low | Outputs 0.0 when off |
| Sync toggle | Lock rate to DAW BPM | Medium | Hard phase reset at transport start; PPQ-derived phase |
| Phase preserved across blocks | Phase is class member | Low | Required to avoid tearing |
| No memory allocation on audio thread | Real-time safety | Low | All state in pre-allocated POD members |
| Beat clock indicator | Visual tempo confirmation | Medium | Flashing dot 30 Hz, reads PPQ or elapsed time |

### Differentiators (v1.4)

Phase slider, Distortion/Jitter slider (additive white noise), Sync subdivisions, Free BPM internal clock.

### Anti-Features (v1.4)

LFO-to-MIDI CC output, custom LFO shape drawing, LFO envelope, variable pulse width, per-voice LFO (4 separate), modulation routing matrix.

### APVTS Parameters Added (v1.4)

lfoXEnabled, lfoXShape (0–6), lfoXFreq, lfoXPhase, lfoXDistortion, lfoXLevel, lfoXSync, lfoXSyncDiv and mirrored lfoY* set — 16 new parameters total.

### Distortion Implementation (v1.4)

Additive white noise: `output = lfoWaveform + distortion * noise` where `noise = lcg() ∈ [-1,+1]`. Clamped to ±1. Matches Ableton M4L LFO "Jitter" documented behavior.

### Sync Behavior (v1.4)

Hard phase reset on DAW play-start (transport stopped → playing transition). PPQ-derived phase during playback for seek/loop robustness: `φ = fmod(ppqPosition / beatsPerCycle, 1.0) + phaseOffset`.

---

## Appendix: v1.1 Feature Research (Historical — 2026-02-24)

> Preserved for roadmap continuity. All v1.1 features shipped in v1.3.

**MIDI Panic:** CC64=0, CC120=0, CC123=0 on all 16 channels. CC121 explicitly excluded (downstream VST3 instruments map CC121 to parameters). One-shot (not persistent mute).

**Trigger/Gate Quantization:** Live (snap on record) and Post (snap existing loop). Algorithm: `round(beatPos / gridSize) * gridSize`. Applied only to Gate events. Subdivisions: 1/4=1.0 beats, 1/8=0.5, 1/16=0.25, 1/32=0.125.

**Looper Playback Position Bar:** Horizontal progress bar. Ratio = `getPlaybackBeat() / getLoopLengthBeats()`. Cyan playing, amber recording. 30 Hz update.

**Gamepad Controller Type Detection:** `SDL_GameControllerGetType()` (SDL 2.0.12+). Returns enum covering PS3/PS4/PS5, Xbox 360/One, Switch Pro. Display string: "{Type} Connected" or "No Controller".

---
*Feature research for: ChordJoystick v1.5 Routing + Expression (primary) and v1.4 LFO + Clock / v1.1 additions (appendix)*
*Primary research date: 2026-02-28*
