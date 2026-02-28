# Architecture Patterns: v1.5 Feature Integration

**Project:** ChordJoystick v1.5 — Routing, Expression, LFO Recording, Arpeggiator Expansion
**Domain:** JUCE VST3 MIDI Plugin — audio-thread-safe feature additions
**Researched:** 2026-02-28
**Overall confidence:** HIGH — based on direct source inspection of all relevant files

---

## Scope

This document answers the architectural integration question for ChordJoystick v1.5:

> How do the 6 new feature areas integrate with the existing architecture? What new components are needed vs. modifications to existing ones? What is the safest build order given dependencies?

Feature areas:

1. Single-channel routing mode
2. Sub octave per voice
3. LFO recording (arm/record/playback state machine in LfoEngine)
4. Left joystick X/Y target expansion
5. Arpeggiator (already mostly in-processor; expansion for Option Mode 1)
6. Option Mode 1 gamepad arp control + R3 removal

---

## Existing Architecture Baseline (v1.4)

```
processBlock() execution order — v1.4
────────────────────────────────────────────────────────────────
1.  gamepad_.setDeadZone(...)
2.  [GAMEPAD]  Poll voice gates / looper buttons / D-pad / option-mode
3.  [DAW]      getPlayHead() → ppqPos, dawBpm, isDawPlaying
4.  [LOOPER]   looper_.process(lp)   — may override joystickX/Y atomics
5.  [CHORD]    buildChordParams()    — reads joystick atomics + LFO ramp outputs
6.  [LFO]      lfoX_.process() / lfoY_.process() + ramp filter
               modulatedJoyX_/Y_ stored for UI display
               chordP.joystickX/Y clamped ±1 after LFO addition
7.  [BEAT]     beat-clock floor-crossing detection → beatOccurred_
8.  [CHORD]    ChordEngine::computePitch(v, chordP) × 4 → freshPitches[]
9.  [ROUTING]  Read voiceChs[] from APVTS (per-voice channels 1-4)
10. [EVENTS]   DAW stop/start detection — allNotesOff, LFO reset
11. [LOOPER]   looper start/stop detection — note-offs for hanging looper notes
12. [LOOPER]   looper reset flag — note-offs
13. [PANIC]    pendingPanic_ — 48 CC events, resetAllGates
14. [LOOPER]   emit looper gateOn/gateOff MIDI (bypasses TriggerSystem)
15. [TRIGGER]  trigger_.processBlock(tp) — pad/joy/random gate logic
16. [ARP]      arm-and-wait logic, then arpeggiator step engine
17. [REC]      looper_.activateRecordingNow() on joystick movement
18. [REC]      looper_.recordJoystick() + looper_.recordFilter()
19. [FILTER]   filter CC section — CC74/CC71/CC12/CC1/CC76 per filterXMode/YMode
────────────────────────────────────────────────────────────────
```

Key invariants that must not change:

- No allocation, no mutex on the audio thread.
- All cross-thread data via `std::atomic` or the params-struct pattern.
- `buildChordParams()` is `const` — LFO ramp outputs are plain floats, readable const.
- `joystickX`/`joystickY` atomics hold raw (pre-LFO) position; LFO applies only in local scope.
- Looper gate MIDI bypasses TriggerSystem entirely (step 14 fires before TriggerSystem at step 15).

---

## Feature 1: Single-Channel Routing Mode

### What It Does

One new APVTS `bool` (`singleChanMode`) and one `int` (`singleChanTarget`, 1..16) route all four voice note-on/off to the same MIDI channel. When two voices play the same pitch on the same channel, a naive approach sends noteOn twice and noteOff once — the second noteOff silences a note that is still logically held. This requires a per-pitch reference-count map.

### Integration Point

Step 9 (routing) and steps 14-16 (note emission).

The channel selection logic currently runs in three places:

- **Step 14** (looper gate MIDI): `voiceChs[v] - 1` used directly.
- **Step 15** (TriggerSystem callback `tp.onNote`): `voiceChs[voice] - 1` used directly.
- **Step 16** (arp note-on/off): `voiceChs[arpActiveVoice_] - 1` used directly.
- **processBlockBypassed**: `voiceChs[v]` used directly.

The cleanest insertion is an inline helper called `effectiveChannel(int voice)` that returns the single-channel target when the mode is active, or `voiceChs[voice]` otherwise. This is called in exactly those four spots, replacing the raw `voiceChs[v]` read.

### Note-Collision Tracking

A `std::array<std::unordered_map<int,int>, 16>` per-channel pitch refcount is too expensive for audio-thread construction and would require heap allocation.

The correct audio-thread solution is a fixed-size flat array. The plugin has 4 voices at most. On any single channel, at most 4 distinct pitches can be playing simultaneously. A 16×128 boolean matrix (`bool activePitchOnChannel_[16][128]`) is 2048 bytes of stack/member storage — acceptable. Rather than a refcount, use a voice-keyed struct: track per-voice the actual channel and pitch that were sent, and send noteOff using exactly those values.

The existing `TriggerSystem::activePitch_[v]` and `looperActivePitch_[v]` already serve this purpose for their respective sources. Single-channel mode does not change what pitch was sent; it only changes which channel it was sent on. The note-off must use the channel that was used at note-on time.

**Safe implementation:** Store the effective channel alongside the pitch at note-on time:

```cpp
// In PluginProcessor private:
int sentChannel_[4] = {1, 2, 3, 4};  // channel used for last note-on, per voice (audio-thread only)
```

At every note-on (looper, TriggerSystem callback, arp), write `sentChannel_[voice] = effectiveChannel(voice)`. At note-off, read `sentChannel_[voice]` instead of computing `effectiveChannel(voice)` again. This guarantees note-off goes to the same channel as note-on regardless of mode changes mid-note.

This same pattern must apply to the arp's `arpActiveVoice_` path.

### New Components

None. Pure modification to PluginProcessor.

### Modified Components

| Component | Change |
|-----------|--------|
| `PluginProcessor.cpp` `createParameterLayout()` | Add `singleChanMode` (bool, false) and `singleChanTarget` (int, 1..16, default 1) |
| `PluginProcessor.h` | Add `sentChannel_[4]` array; add `effectiveChannel(int voice) const` inline |
| `PluginProcessor.cpp` processBlock steps 14/15/16 | Replace `voiceChs[v]` with `effectiveChannel(v)`; write `sentChannel_[v]` at every note-on |
| `PluginProcessor.cpp` processBlockBypassed | Same replacement |
| `PluginEditor` | Add toggle + channel selector UI (message thread, APVTS attachment) |

### Audio-Thread Safety

`singleChanMode` and `singleChanTarget` are APVTS params read via `getRawParameterValue()` — atomic read, safe on audio thread. `sentChannel_` is audio-thread-only, no atomic needed.

---

## Feature 2: Sub Octave Per Voice

### What It Does

Four APVTS bools (`subOct0..3`). When enabled for voice V, every note-on for that voice sends a second note-on 12 semitones lower on the same channel. Every note-off sends a matching note-off at the sub-octave pitch. The sub-octave note-off must match the sub-octave pitch that was sent at note-on — not recomputed from current `heldPitch_[v]`.

### Integration Point

Steps 14, 15, and 16 (all note-emission sites).

The pattern is identical to `sentChannel_`: store the sub-octave pitch that was sent, emit the matching note-off.

```cpp
// In PluginProcessor private (audio-thread only):
int sentSubOctPitch_[4] = {-1, -1, -1, -1};  // -1 = no sub note sounding
```

At note-on for voice V (when `subOct[V]` is enabled):

```cpp
const int subPitch = pitch - 12;
if (subPitch >= 0) {
    midi.addEvent(MidiMessage::noteOn(ch, subPitch, velocity), sampleOff);
    sentSubOctPitch_[voice] = subPitch;
}
```

At note-off for voice V:

```cpp
if (sentSubOctPitch_[voice] >= 0) {
    midi.addEvent(MidiMessage::noteOff(ch, sentSubOctPitch_[voice], 0), sampleOff);
    sentSubOctPitch_[voice] = -1;
}
```

This must be added at all three emission sites: looper gate (step 14), TriggerSystem callback (step 15), and arp (step 16).

The looper `looperActivePitch_[v]` already tracks the looper note-on pitch for matching note-offs. A parallel `looperActiveSubPitch_[v]` array is needed for the looper path.

### MIDI Panic Interaction

The panic handler must also clear `sentSubOctPitch_[]` and `looperActiveSubPitch_[]` to prevent them from re-emitting note-offs after the panic 48-CC flood.

### New Components

None. Pure PluginProcessor modification.

### Modified Components

| Component | Change |
|-----------|--------|
| `PluginProcessor.cpp` `createParameterLayout()` | Add `subOct0..3` (bool × 4, default false) |
| `PluginProcessor.h` | Add `sentSubOctPitch_[4]`, `looperActiveSubPitch_[4]` arrays |
| `PluginProcessor.cpp` steps 14/15/16 | Add sub-octave note-on/off at each emission site |
| `PluginProcessor.cpp` panic handler | Reset `sentSubOctPitch_[]`, `looperActiveSubPitch_[]` |
| `PluginEditor` | Add Sub Oct toggle button per pad (with Hold split UI) |

### Audio-Thread Safety

All new arrays are audio-thread-only. Sub oct bools read from APVTS via `getRawParameterValue()` — atomic, safe.

---

## Feature 3: LFO Recording

### What It Does

LfoEngine gains a recording state machine: `Unarmed → Armed → Recording → Playback`. During Recording, the engine stores one float sample per `process()` call into a fixed-capacity circular buffer. After one loop cycle (duration = looper loop length in beats / BPM), it transitions to Playback and reads back the stored samples instead of evaluating the waveform. Distort parameter applies in all states on top of the playback sample (exactly as it applies on the live waveform in normal mode).

### LfoEngine State Machine

```
Unarmed  ──(arm())──►  Armed
Armed    ──(first process() call after arm)──►  Recording
Recording──(cycle complete)──►  Playback
Playback ──(clear())──►  Unarmed
Armed    ──(arm() again / cancel)──►  Unarmed
```

State is communicated across the audio/message boundary via atomics:

```cpp
// In LfoEngine private:
enum class RecState { Unarmed = 0, Armed, Recording, Playback };
std::atomic<int> recState_ { (int)RecState::Unarmed };  // written audio thread, read UI
```

`arm()` and `clear()` are called from the message thread (UI button). `arm()` does a CAS from Unarmed→Armed. `clear()` stores Unarmed unconditionally. Both are atomic stores — no mutex.

The recording buffer is a fixed-capacity array of floats:

```cpp
static constexpr int kLfoRecCapacity = 8192;  // ~170 seconds at 1 Hz / 48 Hz block rate, far more than needed
float recBuf_[kLfoRecCapacity] {};
int   recWritePos_ = 0;   // audio-thread only
int   recLength_   = 0;   // set at end of recording; read during playback
int   recReadPos_  = 0;   // audio-thread only, wraps at recLength_
```

The capacity must hold at least one full loop cycle worth of blocks. At 44100 Hz with 512-sample blocks, that is 86 blocks/second. At 120 BPM with a 16-bar loop at 4/4, that is 32 beats = 16 seconds = ~1376 blocks. 8192 entries is sufficient for loop lengths up to 95 seconds at 86 blocks/sec.

### Cycle Duration Tracking

PluginProcessor already computes `looper_.getLoopLengthBeats()` and `effectiveBpm_`. LfoEngine needs to know the cycle length to know when to stop recording. Pass it via `ProcessParams::maxCycleBeats` (this field already exists in `LfoEngine::ProcessParams`). The recording stops when accumulated beats >= `maxCycleBeats`.

Beat accumulation in recording mode:

```cpp
const double beatsThisBlock = p.bpm * p.blockSize / (p.sampleRate * 60.0);
recordedBeats_ += beatsThisBlock;
if (recordedBeats_ >= p.maxCycleBeats) {
    recLength_   = recWritePos_;
    recWritePos_ = 0;
    recReadPos_  = 0;
    recState_.store((int)RecState::Playback, std::memory_order_release);
}
```

### Distort in Playback Mode

The existing `distortion` param applies post-waveform noise. In Playback mode, read from `recBuf_[recReadPos_]` instead of calling `evaluateWaveform()`, then apply the same distort noise addition. The distort path is already written as a separate additive noise step, so the refactor is localized.

### processBlock Integration (PluginProcessor)

No changes to processBlock beyond what already exists for LFO. The `ProcessParams` passed to `lfoX_`/`lfoY_` already includes `maxCycleBeats`. PluginProcessor needs to:

1. Forward `looper_.getLoopLengthBeats()` into `xp.maxCycleBeats` and `yp.maxCycleBeats` (this is already set as 16.0 hardcoded — replace with the live value).
2. Expose `lfoX_.getRecState()` / `lfoY_.getRecState()` accessors for UI read.
3. Expose `lfoX_.arm()` / `lfoX_.clear()` wrappers callable from message thread.

### New Components

None — modification to `LfoEngine` only.

### Modified Components

| Component | Change |
|-----------|--------|
| `LfoEngine.h` | Add `RecState` enum, `recState_` atomic, `recBuf_[]`, record/playback state; add `arm()`, `clear()`, `getRecState()` |
| `LfoEngine.cpp` `process()` | State machine: in Armed→start recording; in Recording→write to buf, check cycle; in Playback→read from buf |
| `PluginProcessor.cpp` processBlock | Replace hardcoded `maxCycleBeats = 16.0` with `looper_.getLoopLengthBeats()` |
| `PluginProcessor.h` | Add `lfoXArm()`, `lfoXClear()`, `lfoYArm()`, `lfoYClear()` forwarding wrappers; add `getLfoXRecState()`, `getLfoYRecState()` |
| `PluginEditor` | Add ARM button + CLEAR button per LFO axis; read rec state in timerCallback for UI blink |

### Audio-Thread Safety

`recState_` is `std::atomic<int>`. Writes from message thread (`arm()`, `clear()`) use `store(relaxed)` since no data the UI publishes is guarded by this atomic. Writes from audio thread use `store(release)` to ensure `recLength_` is visible when Playback state is observed. The buffer arrays `recBuf_[]`, `recWritePos_`, `recLength_`, `recReadPos_` are audio-thread-only. No mutex anywhere.

---

## Feature 4: Left Joystick X/Y Target Expansion

### Current State

`filterXMode` and `filterYMode` APVTS choice params already exist (added in v1.4/current):

```cpp
juce::StringArray yModes { "Resonance", "LFO Rate" };         // 0=CC71, 1=CC76
juce::StringArray xModes { "Cutoff", "VCF LFO", "Mod Wheel" }; // 0=CC74, 1=CC12, 2=CC1
```

These route the left stick to different CC numbers. The filter CC section in step 19 already reads `filterXMode`/`filterYMode` and selects `ccXnum`/`ccYnum` accordingly.

### New Targets

The v1.5 requirement is to add **LFO freq / shape / level / arp gate len** as left-stick targets. These are not CC-routed — they modify APVTS parameters directly from the stick position.

This is architecturally different from CC emission. CC emission is a per-block output operation. APVTS modification is a per-event UI-thread operation. The audio thread must NOT call `setValueNotifyingHost()`.

### Correct Pattern: Stick Values as Param Hints via Atomics

The existing `filterCutDisplay_` / `filterResDisplay_` atomics demonstrate the established cross-thread pattern: audio thread writes a float atomic, UI timer reads it and calls `setValueNotifyingHost()`.

For stick-to-param targets, the same pattern applies:

```cpp
// In PluginProcessor private:
std::atomic<float> pendingLStickX_ { -999.f };  // -999 = no pending update
std::atomic<float> pendingLStickY_ { -999.f };
```

In processBlock step 19 (filter CC), when `filterXMode` indicates an APVTS-target mode (index >= 3 for new targets), write `pendingLStickX_` with the scaled stick value instead of emitting a CC. In `PluginEditor::timerCallback()`, read `pendingLStickX_`, if it changed, call `setValueNotifyingHost()` on the target APVTS param.

### APVTS Choice Param Extension

The choice lists must be extended:

```
filterYMode:  { "Resonance", "LFO Rate", "LFO Level", "LFO Freq", "Arp Gate" }
filterXMode:  { "Cutoff", "VCF LFO", "Mod Wheel", "LFO Shape", "LFO Level", "LFO Freq", "Arp Gate" }
```

Extending a `juce::AudioParameterChoice` with more options changes the choice count. In JUCE, `AudioParameterChoice` saves its value as a normalized float (index / (count-1)). Adding options changes the normalization and can corrupt saved state for mode indices already in use. The only safe approach is to add new choices at the END of the list, preserving all existing indices.

The existing indices (0, 1, 2 for X and 0, 1 for Y) must remain at the same positions. New entries are appended.

### processBlock Impact

In step 19, the existing `ccXnum`/`ccYnum` logic already has a mode switch. Add new branches:

```cpp
if (xMode >= 3) {
    // APVTS-param target: write pendingLStickX_ scaled to param range
    // No CC emitted
} else {
    // existing CC logic
}
```

The `filterModOn` guard already gates the whole block, so the new branches inherit the same enable/disable logic.

### New Components

None. PluginProcessor + PluginEditor modification only.

### Modified Components

| Component | Change |
|-----------|--------|
| `PluginProcessor.cpp` `createParameterLayout()` | Extend `filterXMode` / `filterYMode` choice lists (append new entries) |
| `PluginProcessor.h` | Add `pendingLStickX_`, `pendingLStickY_` atomics |
| `PluginProcessor.cpp` step 19 filter CC | Add mode branches for APVTS-param targets; write pending atomics |
| `PluginEditor.cpp` `timerCallback()` | Read pending atomics, call `setValueNotifyingHost()` on target params |
| `PluginEditor` | Update filterMode dropdown labels |

---

## Feature 5: Arpeggiator — Current State and Gap Analysis

### What Already Exists

The arpeggiator is **not a separate class**. It lives entirely in `PluginProcessor.cpp` processBlock step 16. APVTS parameters `arpEnabled`, `arpSubdiv`, `arpOrder`, `arpGateTime` are already registered and fully functional. State variables `arpPhase_`, `arpStep_`, `arpActivePitch_`, `arpActiveVoice_`, `arpCurrentVoice_`, `arpNoteOffRemaining_`, `arpRandSeed_`, `arpRandomOrder_[4]`, `arpWaitingForPlay_`, `prevArpOn_` are all declared in `PluginProcessor.h`.

The arp supports 7 order modes (Up, Down, Up+Down, Down+Up, Outer-In, Inner-Out, Random), DAW-sync and looper-sync clock modes, arm-and-wait logic, and pad-priority choking.

### What v1.5 Adds

The only arp gap for v1.5 is **gamepad Option Mode 1 control** (Feature 6 below). No new arp logic is needed beyond the gamepad bindings. The arpeggiator architecture itself is complete.

---

## Feature 6: Option Mode 1 Gamepad Arp Control + R3 Removal

### Current Option Mode 1 Behaviour

In GamepadInput, `optionMode_` cycles 0→1→2→0 on each START button press. In processBlock step 2:

- Mode 1: D-pad controls octave params via `consumeOptionDpadDelta(dir)`.
- Mode 2: D-pad controls transpose + intervals.
- Mode 0: D-pad controls BPM and looper rec state.

R3 (right stick press) currently triggers MIDI panic unconditionally (step 2, `consumeRightStickTrigger()`).

### New Option Mode 1 Bindings

The requirement is to repurpose the face buttons (Circle/Triangle/Square/X) in Option Mode 1 to control arp parameters. Currently, face buttons in all modes are hardwired to looper transport: Cross=start/stop, Square=reset, Triangle=record, Circle=delete.

### Architecture Decision: Where to Implement

Option A — GamepadInput handles mode-sensitive button mapping:
- GamepadInput.timerCallback() checks `optionMode_` before setting looper/arp trigger atomics.
- Clean separation: GamepadInput translates hardware → intent.
- Problem: GamepadInput currently has no knowledge of arp state. Adding APVTS or arp coupling to GamepadInput breaks its isolation (it currently has no APVTS dependency).

Option B — PluginProcessor handles mode-sensitive dispatch:
- GamepadInput continues to expose raw consume flags for all buttons.
- processBlock step 2 reads `getOptionMode()` and dispatches accordingly.
- This is the **existing pattern**: processBlock already reads `optMode` and dispatches D-pad differently per mode. The same `if (optMode == 1)` branch can be extended to intercept face buttons.

Option B is correct. It requires zero changes to GamepadInput.

### Implementation in processBlock Step 2

```cpp
const int optMode = gamepad_.getOptionMode();

// Face buttons — mode-sensitive dispatch
if (gamepad_.consumeLooperStartStop()) {
    if (optMode == 1) { /* X → toggle RND Sync */ }
    else               looper_.startStop();
}
if (gamepad_.consumeLooperRecord()) {
    if (optMode == 1) { /* Triangle → arp rate step */ }
    else               looper_.record();
}
if (gamepad_.consumeLooperReset()) {
    if (optMode == 1) { /* Square → arp order step */ flashLoopReset_.fetch_add(1, ...); }
    else             { looper_.reset(); flashLoopReset_.fetch_add(1, ...); }
}
if (gamepad_.consumeLooperDelete()) {
    if (optMode == 1) { /* Circle → arp on/off toggle */ }
    else             { looper_.deleteLoop(); flashLoopDelete_.fetch_add(1, ...); }
}
```

For "arp rate step" and "arp order step", use the existing `stepWrappingParam()` helper (already in PluginProcessor.h as a static method). For "arp on/off", call `setValueNotifyingHost()` via `apvts.getParameter("arpEnabled")` — this is safe from the audio thread because JUCE's `setValueNotifyingHost()` is thread-safe for AudioProcessorParameter.

For "RND Sync" toggle, the same pattern: read `randomClockSync` APVTS param, invert, call `setValueNotifyingHost()`.

### R3 Removal

Currently in processBlock step 2:
```cpp
if (gamepad_.consumeRightStickTrigger())
    triggerPanic();   // R3 -> MIDI panic
```

This line is deleted unconditionally. R3 standalone panic is removed entirely.

The `consumeRightStickTrigger()` method in GamepadInput remains (no header change needed) but is simply not called in processBlock. It will drain silently.

### R3 + Held Pad = Sub Oct Toggle

The requirement "R3 + held pad (any mode) = toggle Sub Oct for that voice" needs R3 as a held-state modifier, not a rising-edge trigger. GamepadInput currently exposes `consumeRightStickTrigger()` (rising edge only) and `btnRightStick_` (private).

Two options:

Option A — Add `getRightStickHeld()` accessor to GamepadInput:

```cpp
// In GamepadInput.h public:
bool getRightStickHeld() const { return rightStickHeld_.load(std::memory_order_relaxed); }
// In GamepadInput private:
std::atomic<bool> rightStickHeld_ { false };
// In timerCallback(), alongside existing btnRightStick_ debounce:
rightStickHeld_.store(debounced_held_state, std::memory_order_relaxed);
```

Option B — Detect in processBlock via the existing rising-edge flag plus a local held-state tracker (mirror of the `gamepadVoiceWasHeld_` pattern). This requires the timerCallback to set the flag each tick, which it already does for voice buttons.

Option A is cleaner and symmetric with `getVoiceHeld()` / `getAllNotesHeld()` which both exist. It is one new atomic + one line in timerCallback — minimal GamepadInput change.

In processBlock:

```cpp
const bool r3Held = gamepad_.getRightStickHeld();
if (r3Held) {
    for (int v = 0; v < 4; ++v) {
        if (gamepad_.getVoiceHeld(v)) {
            // rising-edge detected by prevR3HeldAndVoice_[v] tracking
            // toggle subOct[v] APVTS param via setValueNotifyingHost
        }
    }
}
```

Edge-detect per voice requires a `prevR3WithVoice_[4]` bool array (audio-thread only) tracking whether R3+voice[v] was held last block.

### Modified Components

| Component | Change |
|-----------|--------|
| `GamepadInput.h` | Add `rightStickHeld_` atomic + `getRightStickHeld()` accessor |
| `GamepadInput.cpp` timerCallback | Set `rightStickHeld_` from debounced R3 state |
| `PluginProcessor.cpp` processBlock step 2 | Mode-sensitive face-button dispatch; R3 panic removal; R3+pad sub-oct toggle |
| `PluginProcessor.h` | Add `prevR3WithVoice_[4]` bool array (audio-thread only) |

---

## Unified Data Flow Diagram — v1.5

```
[UI drag / gamepad right stick]
         |
         v
   joystickX/Y (atomic<float>)   ← raw pitch joystick position
         |
         |  [looper playback may override — step 4]
         |
         v
   buildChordParams()
   + LFO ramp output (lfoXRampOut_/lfoYRampOut_, plain floats)
         |
         v
   ChordEngine::Params { joystickX, joystickY } (clamped ±1)
         |
         v
   freshPitches[4]
         |
         +──► TriggerSystem.processBlock → tp.onNote callback
         |         └──► note-on: sentChannel_[v] written
         |              note-off: sentChannel_[v] read
         |              sub-oct: sentSubOctPitch_[v] written/read
         |
         +──► looper gateOn/gateOff
         |         └──► looperActivePitch_[v] / looperActiveSubPitch_[v]
         |              effectiveChannel(v) used for MIDI channel
         |
         +──► arp step engine
                   └──► arpActivePitch_ / arpActiveVoice_
                        effectiveChannel(arpActiveVoice_) for channel

[gamepad left stick]
         |
         v
   filterX_/filterY_ (atomics in GamepadInput)
         |
         v
   processBlock step 19:
     filterXMode 0-2: emit CC74/CC12/CC1
     filterXMode 3+:  write pendingLStickX_ atomic
         |
         v
   PluginEditor.timerCallback():
     read pendingLStickX_ → setValueNotifyingHost on LFO/arp APVTS param

[LfoEngine X / Y]
         |
   RecState: Unarmed/Armed/Recording/Playback
         |
   process(): evaluateWaveform OR recBuf_[readPos] + distort
         |
         v
   lfoXRampOut_ / lfoYRampOut_ (plain floats, ramp-smoothed)
   → applied in buildChordParams() local scope
```

---

## Component Summary Table

### New Components

None required. All features integrate into existing component structure.

### Modified Components

| Component | Feature Areas | What Changes |
|-----------|--------------|--------------|
| `LfoEngine.h` / `.cpp` | F3 (LFO recording) | RecState enum + atomic; recBuf_[]; arm()/clear(); process() state machine |
| `GamepadInput.h` / `.cpp` | F6 (R3 held state) | rightStickHeld_ atomic; getRightStickHeld() accessor; timerCallback sets it |
| `PluginProcessor.h` | F1/F2/F4/F6 | sentChannel_[4]; sentSubOctPitch_[4]; looperActiveSubPitch_[4]; pendingLStickX_/Y_; prevR3WithVoice_[4]; effectiveChannel() inline |
| `PluginProcessor.cpp` createParameterLayout | F1/F2/F4 | singleChanMode, singleChanTarget, subOct0-3; extend filterXMode/YMode choices |
| `PluginProcessor.cpp` processBlock | F1/F2/F4/F5/F6 | effectiveChannel() at note-on/off sites; sub-oct emission; filter mode new branches; mode-1 face-button dispatch; R3 removal; R3+pad sub-oct toggle |
| `PluginEditor.h` / `.cpp` | F1/F2/F3/F4 | Single-chan UI; sub-oct buttons; LFO arm/clear UI; filter mode dropdown extension |

### Unmodified Components

| Component | Reason |
|-----------|--------|
| `ChordEngine` | Pure pitch computation; channel/sub-oct are post-ChordEngine |
| `ScaleQuantizer` | No dependency on any v1.5 feature |
| `LooperEngine` | No changes required; existing event types sufficient |
| `TriggerSystem` | No changes required; sub-oct and channel remapping happen in the onNote callback in processBlock, outside TriggerSystem |

---

## Dependency Graph and Build Order

```
F1 (Single channel)   ──► No deps on other new features
F2 (Sub octave)       ──► No deps; safe after F1 (both touch note-emission sites)
F3 (LFO recording)    ──► No deps on other new features
F4 (Stick targets)    ──► Requires APVTS choice param extension (do after F1/F2 to consolidate layout changes)
F5 (Arp review)       ──► Already complete; no new arp logic needed
F6 (Gamepad opt mode) ──► Requires GamepadInput R3 held state (GamepadInput change first)
```

### Recommended Phase Order

**Phase 1 — Gamepad R3 Held State (GamepadInput — 1 file change)**

This is the smallest isolated change. Add `rightStickHeld_` to `GamepadInput.h` and set it in `timerCallback()`. Ship nothing else yet. Verify no regression by checking R3-as-panic still fires (it will, nothing changed in processBlock yet). Build and test.

Rationale: GamepadInput changes are independent of APVTS and processBlock. Getting this file change done and tested in isolation avoids having it interleave with the larger processBlock edit.

**Phase 2 — Single-Channel Routing (PluginProcessor — APVTS + processBlock)**

Add `singleChanMode` / `singleChanTarget` APVTS params and `sentChannel_[4]`. Implement `effectiveChannel()`. Replace `voiceChs[v]` at all emission sites. Write `sentChannel_[v]` at all note-on sites.

This phase touches every note-emission path and establishes the channel-tracking infrastructure that sub-octave (Phase 3) will reuse. Completing it first means Phase 3 can follow the established `sentChannel_` pattern.

**Phase 3 — Sub Octave Per Voice (PluginProcessor — APVTS + processBlock)**

Add `subOct0..3` APVTS params. Add `sentSubOctPitch_[4]` and `looperActiveSubPitch_[4]`. Add sub-octave note-on/off at all three emission sites (looper, TriggerSystem callback, arp). Add panic cleanup.

**Phase 4 — LFO Recording (LfoEngine only)**

Add RecState to LfoEngine. Implement recording buffer. Wire processBlock to pass `looper_.getLoopLengthBeats()` as `maxCycleBeats`. Add UI arm/clear buttons.

This is self-contained to LfoEngine and a minor processBlock change. No note-emission path is touched.

**Phase 5 — Left Stick Target Expansion (PluginProcessor APVTS + filter section)**

Extend `filterXMode` / `filterYMode` choice lists. Add `pendingLStickX_`/`Y_` atomics. Add mode branches in step 19. Add timerCallback read in PluginEditor.

Isolating this to Phase 5 avoids changing the APVTS layout while Phases 2/3 are also in progress. After Phase 3 the layout is stable and this can be added cleanly.

**Phase 6 — Gamepad Option Mode 1 Arp Bindings + R3 Removal (processBlock only)**

Replace R3 panic with nothing. Add mode-1 face-button dispatch (arp on/off, rate step, order step, RND sync). Add `prevR3WithVoice_[4]` and sub-oct toggle via R3+pad.

This phase is last because it uses `getRightStickHeld()` (Phase 1), `subOct` params (Phase 3), and arp params (already exist).

**Bug Fixes (can run in parallel with any phase)**

- Looper wrong start position after record cycle: isolated to `LooperEngine.cpp` `finaliseRecording()` / `loopStartPpq_` anchor logic.
- PS4 BT reconnect crash: isolated to `GamepadInput.cpp` `tryOpenController()` / `closeController()` SDL2 handle management.

---

## Phase-Specific Warnings

| Phase | Pitfall | Mitigation |
|-------|---------|------------|
| Phase 1 (single channel) | Note-off goes to wrong channel if `effectiveChannel()` is not memoized at note-on time | Always write `sentChannel_[v]` at every note-on; note-off reads `sentChannel_[v]`, never recomputes |
| Phase 1 (single channel) | processBlockBypassed also sends note-offs using `voiceChs[v]` directly | Update processBlockBypassed to use `sentChannel_[v]` or `effectiveChannel(v)` |
| Phase 2 (sub octave) | Panic clears `sentSubOctPitch_[]` but sub note was already silenced by allNotesOff | Set `sentSubOctPitch_[v] = -1` after panic loop, same as `looperActivePitch_.fill(-1)` pattern |
| Phase 2 (sub octave) | Sub pitch below MIDI 0 (pitch 0..11, sub = -12..-1) | Guard `if (subPitch >= 0)` before emitting; set `sentSubOctPitch_[v] = -1` when guard fails |
| Phase 3 (LFO recording) | RecState::Armed → Recording transition timing: if process() runs with RecState::Armed before the buffer is zeroed, stale data plays back | Zero `recBuf_` and reset `recWritePos_` atomically when transitioning Armed→Recording inside process() on the audio thread |
| Phase 3 (LFO recording) | `arm()` called from message thread while process() is mid-recording | `arm()` does a CAS: `Unarmed → Armed` only. If state is Recording or Playback, arm() is a no-op (cancel requires explicit `clear()` first) |
| Phase 4 (stick targets) | Extending `filterXMode` choices changes normalization of existing saved mode values (0,1,2 map to different floats with more choices) | In JUCE, AudioParameterChoice stores the index as an integer raw value (not normalized to 0..1 like float params). Verify in JUCE 8 source that AudioParameterChoice::getValue() returns index/count-1 or raw index. If raw index, appending is safe. If normalized, must add a value migration path. |
| Phase 4 (stick targets) | `setValueNotifyingHost()` called from audio thread in the APVTS-param branch | This call must NOT be in processBlock. Use the pending-atomic pattern: processBlock writes `pendingLStickX_`, timerCallback reads and calls `setValueNotifyingHost()` |
| Phase 5 (gamepad mode 1) | Face button "consume" flags are consumed even in option mode 1, preventing looper transport in that mode | This is correct by design — option mode 1 intentionally replaces looper transport bindings with arp controls |
| Phase 5 (gamepad mode 1) | R3 + pad sub-oct toggle: R3 held while voice button held fires every block, not once | Use `prevR3WithVoice_[v]` to detect the rising edge (both conditions newly true) — same pattern as `gamepadVoiceWasHeld_[v]` |

---

## Audio-Thread Safety Checklist

| Item | Mechanism | Verified |
|------|-----------|---------|
| `singleChanMode` / `singleChanTarget` read on audio thread | APVTS `getRawParameterValue()` atomic read | Safe |
| `sentChannel_[4]` written and read on audio thread only | Plain array, no atomic needed | Safe |
| `subOct0..3` read on audio thread | APVTS `getRawParameterValue()` | Safe |
| `sentSubOctPitch_[4]` audio-thread only | Plain array | Safe |
| `looperActiveSubPitch_[4]` audio-thread only | Plain array | Safe |
| `LfoEngine::recState_` written from message thread (arm/clear), read from audio thread | `std::atomic<int>`, arm uses CAS, clear uses store | Safe |
| `LfoEngine::recBuf_[]` audio-thread only | Plain array | Safe |
| `pendingLStickX_/Y_` written on audio thread, read on message thread | `std::atomic<float>` | Safe |
| `rightStickHeld_` written from SDL timer thread (message thread), read on audio thread | `std::atomic<bool>` | Safe |
| `prevR3WithVoice_[4]` audio-thread only | Plain array | Safe |
| `setValueNotifyingHost()` from arp/RND-sync gamepad bindings (called in processBlock audio thread) | JUCE AudioProcessorParameter::setValueNotifyingHost() posts to message queue — thread-safe by JUCE design | Safe (JUCE guarantee) |

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Single-channel routing integration | HIGH | All emission sites verified by reading full processBlock; `sentChannel_` pattern mirrors existing `looperActivePitch_` |
| Sub-octave emission | HIGH | Same pattern as single-channel; all three emission sites (looper/trigger/arp) confirmed present in code |
| LFO recording state machine | HIGH | LfoEngine interface verified; `ProcessParams::maxCycleBeats` already exists; `recState_` atomic pattern matches existing flash counters |
| Left stick target expansion | MEDIUM | JUCE AudioParameterChoice index storage must be verified — if normalized float, appending changes existing indices; direct inspection of JUCE 8 source needed at implementation time |
| Gamepad mode-1 dispatch | HIGH | Existing D-pad mode-dispatch pattern in processBlock is the exact structure to extend; no new mechanism needed |
| R3 removal | HIGH | Single line deletion in processBlock; no API changes |
| R3 held state | HIGH | `getVoiceHeld()` and `getAllNotesHeld()` in GamepadInput demonstrate exact pattern |
| Build order safety | HIGH | Dependency graph is acyclic; each phase is independently compilable |

---

## Sources

- Direct inspection: `Source/PluginProcessor.h` — full member layout, arp state, looper active pitch, flash atomics
- Direct inspection: `Source/PluginProcessor.cpp` — full processBlock() (all 1300+ lines read); createParameterLayout() confirms existing filterXMode/filterYMode and arpEnabled/arpSubdiv/arpOrder/arpGateTime params
- Direct inspection: `Source/LfoEngine.h` — ProcessParams struct confirms maxCycleBeats field; RecState not yet present
- Direct inspection: `Source/LooperEngine.h` — lock-free design, BlockOutput, all public API
- Direct inspection: `Source/TriggerSystem.h` — ProcessParams, onNote callback signature
- Direct inspection: `Source/GamepadInput.h` — option mode API, getVoiceHeld/getAllNotesHeld pattern, rightStickTrig_ (rising edge, no held state exposed yet)
- Direct inspection: `.planning/PROJECT.md` — v1.5 requirements verbatim
- Confidence: HIGH — all findings based on actual source inspection; no training-data inference used for integration decisions
