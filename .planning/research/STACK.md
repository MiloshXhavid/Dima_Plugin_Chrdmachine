# Technology Stack

**Project:** ChordJoystick v1.5 — Routing + Expression
**Domain:** JUCE VST3 MIDI effect plugin — single-channel routing, sub octave, arpeggiator, LFO recording, left-stick target expansion, SDL2 BT fix
**Researched:** 2026-02-28
**Confidence:** HIGH (all claims verified against repo source, JUCE 8.0.4 API docs, SDL2 2.30 wiki, and SDL2 issue tracker)

> **Scope:** This file covers ONLY the new v1.5 features. The full locked stack
> (JUCE 8.0.4, SDL2 2.30.9 static, CMake FetchContent, C++17, MSVC, Catch2, Inno Setup 6)
> is documented in prior milestone STACK files and is NOT re-researched here.
> No new external libraries are needed for any v1.5 feature.

---

## Decision: No New Dependencies

All v1.5 features are implemented with the existing locked stack. Zero new libraries, zero new
JUCE modules, zero new CMake targets.

| Feature | Why No New Library Needed |
|---------|--------------------------|
| Note collision tracking (single-channel) | `int[128]` ref-count array — O(1), zero allocation, audio-thread-safe |
| Sub Octave per voice | Second `MidiMessage::noteOn/Off` call with `pitch - 12` — `juce_audio_basics` already in use |
| Arpeggiator DAW sync | `AudioPlayHead::PositionInfo::getPpqPosition()` — already used by LfoEngine and LooperEngine |
| LFO recording ring buffer | `std::array<float, N>` with audio-thread-only read/write indices — no SPSC FIFO needed |
| Left stick target routing | APVTS dispatch switch — pattern identical to existing D-pad option mode |
| SDL2 BT reconnect crash | Instance-ID guard using `SDL_GameControllerGetJoystick` / `SDL_JoystickInstanceID` — both in SDL2 2.30.9 |
| Option Mode 1 arp remap | Code-only button mapping change in existing dispatch logic |
| Looper start-position fix | Logic defect in `loopStartPpq_` anchoring — no new API needed |

---

## Feature-by-Feature Stack Analysis

### Feature 1: Single-Channel MIDI Routing + Note Collision Tracking

**Problem statement:** When all 4 voices route to one MIDI channel, two voices can emit a
`noteOn` for the same pitch number simultaneously. Most downstream instruments treat a second
`noteOn` for an already-sounding pitch as a re-trigger and decrement their internal reference
count on the first `noteOff`, leaving a ghost note sounding indefinitely.

**Stack decision:** Plain `int noteRefCount_[128]` array on `PluginProcessor`, audio-thread-only.
Before any `noteOn` emission on the single channel: increment `noteRefCount_[pitch]`. Before any
`noteOff` emission: decrement `noteRefCount_[pitch]` and only emit the MIDI `noteOff` when the
count reaches zero. Reset the table on panic, DAW stop, and single-channel-mode toggle.

**JUCE APIs used:**
- `MidiBuffer::addEvent(MidiMessage, sampleOffset)` — already in use in processBlock
- `MidiMessage::noteOn(channel, pitch, velocity)` — already in use
- `MidiMessage::noteOff(channel, pitch, velocity)` — already in use

**Why not `juce::MPEInstrument`:** MPE is an input-side polyphony tracker for synthesiser voice
assignment. This plugin generates MIDI; it does not receive or track incoming notes. MPE solves
the wrong problem and would add dead weight.

**Why not `std::map<int,int>`:** Dynamic allocation is prohibited on the audio thread. A
128-element `int` array costs 512 bytes, is O(1), and needs no destructor.

**Confidence:** HIGH — the `int[N]` note-reference-count pattern is the standard MIDI generator
technique confirmed in JUCE forum discussions on stuck notes (multiple threads, 2018–2024).

---

### Feature 2: Per-Voice Sub Octave (Parallel Note 1 Octave Lower)

**Problem statement:** Each voice pad must optionally emit a second note one octave below the
primary pitch on the same channel at the same time, with an independent note-off.

**Stack decision:** Add `bool subOctEnabled_[4]` backed by four new APVTS bool parameters. In the
existing note-on/off emission path, after computing `pitch` for a voice, conditionally emit a
second pair:
```
subPitch = juce::jlimit(0, 127, pitch - 12);
noteOn  at subPitch, same channel, same sampleOffset
noteOff at subPitch, same channel
```
In single-channel mode the collision tracker (Feature 1) handles ref counting for `subPitch`
independently of `pitch`.

**JUCE APIs used:** Same as Feature 1, plus `juce::jlimit<int>()` from `juce_core`.

**Confidence:** HIGH — trivial extension of existing note-emit logic; no new API surface.

---

### Feature 3: Arpeggiator — Rate, Order, Gate Length, DAW Sync

**Problem statement:** The existing arpeggiator skeleton in `PluginProcessor` has `arpPhase_`,
`arpStep_`, `arpActivePitch_`, and `arpActiveVoice_`. The v1.5 spec adds configurable rate
(1/4 to 1/32), order (up/down/random), gate length as a 0..1 fraction of subdivision, and
stable DAW clock sync without drift.

**Stack decision — timing model:**

The JUCE `ArpeggiatorPluginDemo` uses a sample-counter approach that accumulates drift at
non-integer BPMs (e.g. 130 BPM, where `noteDuration` is a fractional number of samples).
Community reports confirm audible timing drift with this approach.

The correct DAW-synced pattern uses `AudioPlayHead::PositionInfo::getPpqPosition()` to compute
the exact sample offset within the block where the next note boundary falls:

```cpp
const double subdivBeats = arpSubdivToBeats(arpRate_);   // e.g. 0.25 for 1/16
const double blockBeats  = (numSamples / sampleRate_) * (bpm / 60.0);
const double nextBoundary = std::ceil(ppqPos / subdivBeats) * subdivBeats;

if (nextBoundary < ppqPos + blockBeats)
{
    const int sampleOffset = static_cast<int>(
        (nextBoundary - ppqPos) / blockBeats * numSamples);
    // emit noteOn at sampleOffset in MidiBuffer
}
```

This is identical to the `ppqPosition`-based sync already implemented in `LfoEngine::process()`
and `LooperEngine::process()`. No new algorithm is needed — extend the existing pattern.

**Gate length:** `gateBeats = subdivBeats * gateFraction`. Use existing `arpNoteOffRemaining_`
counter (already a `double` tracking beats until note-off) to emit the `noteOff` at the correct
sample offset within the same or a later block.

**Free-tempo mode:** When DAW is not playing, use `effectiveBpm_` and `sampleCounter_` (already
on `PluginProcessor`) to synthesize a `ppqPos` equivalent. This is the existing pattern used by
`TriggerSystem` for random gates.

**JUCE APIs used:**
- `AudioPlayHead::PositionInfo::getPpqPosition()` — returns `std::optional<double>`, JUCE 8
- `AudioPlayHead::PositionInfo::getBpm()` — returns `std::optional<double>`, JUCE 8
- `AudioPlayHead::PositionInfo::getIsPlaying()` — returns `std::optional<bool>`, JUCE 8
- `MidiBuffer::addEvent()` with computed `sampleOffset`

All three `PositionInfo` methods confirmed against the JUCE 8 official API documentation.
All are already called in `processBlock` for LFO and looper sync.

**New APVTS parameters needed:**
| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `arpRate` | Choice | 0=1/4, 1=1/8, 2=1/16, 3=1/32 | 2 (1/16) |
| `arpOrder` | Choice | 0=Up, 1=Down, 2=Random | 0 |
| `arpGateLen` | Float | 0.05..0.99 | 0.5 |
| `arpEnabled` | Bool | false/true | false |

**Confidence:** HIGH — ppqPosition-based note scheduling is confirmed by JUCE's own
`AudioPlayHead::PositionInfo` API reference and is the documented approach for sample-accurate
MIDI scheduling in processBlock.

---

### Feature 4: LFO Recording into a Ring Buffer (Arm → Capture 1 Cycle → Locked Playback)

**Problem statement:** Arm a capture mode, record one full LFO cycle of output values into a
fixed-size buffer, then play back the recorded values in a loop instead of recomputing the
live LFO. `distortion` must remain adjustable during playback.

**Stack decision:** Extend `LfoEngine` with a fixed-size float ring buffer:

```cpp
// In LfoEngine.h (new private members)
static constexpr int LFO_REC_CAPACITY = 8192;   // 8192 blocks @ 512 samples = ~95 sec @ 44100
std::array<float, LFO_REC_CAPACITY> recBuf_ {};
int   recWritePos_  = 0;   // audio-thread-only
int   recReadPos_   = 0;   // audio-thread-only
int   recLength_    = 0;   // audio-thread-only; set when capture ends

// Cross-thread (UI sets, audio reads/sets)
std::atomic<bool> recArmed_    { false };
std::atomic<bool> recPlaying_  { false };
```

**Recording logic:** While `recArmed_` is true, write one float per `process()` call into
`recBuf_[recWritePos_++ % LFO_REC_CAPACITY]`. Detect one complete cycle using the existing
`totalCycles_` counter: when `floor(totalCycles_)` advances by 1, the cycle is complete. Set
`recLength_ = recWritePos_`, set `recPlaying_.store(true)`, clear `recArmed_`.

**Playback logic:** When `recPlaying_` is true, return `recBuf_[recReadPos_++ % recLength_]`
directly. Apply `distortion` post-playback identically to the live path so distortion
remains adjustable without rewriting the buffer.

**Clear button:** Sets `recPlaying_.store(false)` and `recArmed_.store(false)` from the
message thread. Audio thread resumes live LFO computation on the next `process()` call.

**Why not `juce::AbstractFifo`:** `AbstractFifo` is a SPSC FIFO for communicating data
between two threads (one writer, one reader on different threads). The LFO record buffer
has a single thread touching it (the audio thread writes during record, then reads during
playback). SPSC synchronization overhead is unnecessary and incorrect for a single-thread
pattern. A plain `std::array<float>` with `int` indices is simpler and sufficient.

**Why not extending `LooperEngine`:** `LooperEngine` stores sparse beat-timestamped events
(joystick X/Y, gate on/off). LFO recording produces a dense stream of one float per block —
a fundamentally different data model. Storing it in `LooperEngine` would require a new event
type, a new playback mode, and cross-class coupling. Keeping it inside `LfoEngine` is
self-contained and mirrors the existing "audio-thread-only private state" design of `LfoEngine`.

**Thread boundary:**
- `recArmed_` written by message thread, read by audio thread — `std::atomic<bool>` correct
- `recPlaying_` written by audio thread (on cycle completion), read by message thread (UI blink) — `std::atomic<bool>` correct
- `recWritePos_`, `recReadPos_`, `recLength_` are audio-thread-only — no atomic needed

**Capacity math:** 8192 entries at one entry per block at 44100 Hz / 512 samples/block = ~95
seconds of LFO recording. At the slowest LFO sync (8 bars at 60 BPM = 32 seconds), this
comfortably fits two full cycles, ensuring the capture always completes.

**Confidence:** HIGH — pattern is a textbook single-producer-single-consumer ring buffer that
degrades to a single-thread pattern (no sync primitives needed). Mirrors the `AbstractFifo`
design in `LooperEngine` at the conceptual level.

---

### Feature 5: Left Joystick X/Y Modulation Target Expansion

**Problem statement:** Left stick axes currently route to CC74 (cutoff) and CC71 (resonance).
v1.5 adds LFO freq / shape / level / arp gate len as additional selectable targets per axis.

**Stack decision:** Add two APVTS int parameters `leftStickXTarget` / `leftStickYTarget`
(enum: 0=Filter CC, 1=LFO Freq, 2=LFO Shape, 3=LFO Level, 4=Arp Gate Len). In
`processBlock`'s left-stick section, dispatch on the target using a switch and call
`setValueNotifyingHost` on the appropriate APVTS parameter.

```cpp
const float stickVal = gamepad_.getFilterX();  // normalized -1..+1
switch (leftXTarget)
{
    case 0: /* existing CC74 code */ break;
    case 1: stepClampingParam(apvts, "lfoXFreq", ...); break;
    case 2: stepWrappingParam(apvts, "lfoXWave", 0, 6, ...); break;
    case 3: stepClampingParam(apvts, "lfoXDepth", 0, 127, ...); break;
    case 4: stepClampingParam(apvts, "arpGateLen", 0, 99, ...); break;
}
```

`stepClampingParam` and `stepWrappingParam` are already defined as static helpers on
`PluginProcessor` (confirmed in `PluginProcessor.h` lines 274–295).

**Why `setValueNotifyingHost` and not direct atomic write:** APVTS parameters must be changed
via `setValueNotifyingHost` so DAW automation tracks changes, state save/load works, and the
UI reflects changes in real time. This is the existing pattern for all parameter mutations in
this plugin.

**New APVTS parameters needed:**
| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `leftStickXTarget` | Choice | 0..4 | 0 (Filter CC) |
| `leftStickYTarget` | Choice | 0..4 | 0 (Filter CC) |

**Confidence:** HIGH — pattern is identical to the existing D-pad option mode parameter dispatch
(confirmed at `PluginProcessor.h` lines 271–295 and `PluginProcessor.cpp` processBlock section).

---

### Feature 6: SDL2 Bluetooth Reconnect Crash Fix

**Root cause analysis (MEDIUM confidence — inferred from SDL2 issue tracker):**

The crash on PS4 BT reconnect is caused by the Windows Bluetooth HID stack firing
`SDL_CONTROLLERDEVICEREMOVED` and `SDL_CONTROLLERDEVICEADDED` events in rapid succession during
a BT re-pair. The current `GamepadInput::timerCallback()` calls `closeController()` on ANY
`SDL_CONTROLLERDEVICEREMOVED` event and `tryOpenController()` on ANY `SDL_CONTROLLERDEVICEADDED`
event. Two failure modes:

1. **Double-open:** A spurious `SDL_CONTROLLERDEVICEADDED` fires while `controller_` is
   non-null (BT re-pair without a preceding valid disconnect). `tryOpenController()` opens a
   second handle to the same HID device. Both handles share SDL2's internal device state.
   When either is closed or the BT stack flushes, the shared state is freed and the surviving
   handle is a use-after-free.

2. **Wrong-device close:** `SDL_CONTROLLERDEVICEREMOVED`'s `event.cdevice.which` is the
   **joystick instance ID**, not the device index. On systems with a USB hub or composite
   device, a different device's removal can trigger `closeController()` on the wrong handle.
   This is documented in the SDL2 wiki for `SDL_ControllerDeviceEvent`.

**Fix: two guards in `GamepadInput::timerCallback()`**

```cpp
// ADDED event: guard against double-open
if (ev.type == SDL_CONTROLLERDEVICEADDED && controller_ == nullptr)
    tryOpenController();

// REMOVED event: verify instance ID matches our open handle before closing
if (ev.type == SDL_CONTROLLERDEVICEREMOVED && controller_ != nullptr)
{
    SDL_Joystick* joy = SDL_GameControllerGetJoystick(controller_);
    if (joy && SDL_JoystickInstanceID(joy) == ev.cdevice.which)
        closeController();
}
```

**SDL2 APIs used (all in SDL2 2.30.9):**
- `SDL_GameControllerGetJoystick(SDL_GameController*)` — documented in SDL2 wiki, returns the
  underlying `SDL_Joystick*` pointer for an open game controller handle
- `SDL_JoystickInstanceID(SDL_Joystick*)` — returns the joystick instance ID as `SDL_JoystickID`
- `ev.cdevice.which` — in `SDL_CONTROLLERDEVICEREMOVED` events this is the joystick instance ID
  (confirmed in SDL2/SDL_ControllerDeviceEvent wiki documentation)

**No SDL version upgrade needed.** Both `SDL_GameControllerGetJoystick` and `SDL_JoystickInstanceID`
have been present since SDL2 2.0.0. SDL2 2.30.9 is the pinned version; no change to
`CMakeLists.txt` required.

The existing fallback poll (`SDL_GameControllerGetAttached()` every tick) remains in place and
handles silent USB disconnects that do not generate events.

**Confidence:** MEDIUM — root cause is inferred from SDL2 GitHub issues #3697 and #3468 (both
confirmed reports of crash on reconnect and wrong-index on REMOVED events), the raylib PR #4724
(reference implementation of instance-ID guard), and the SDL2 wiki documentation on
`SDL_ControllerDeviceEvent.which`. The specific stack trace for this project's PS4 BT crash
has not been captured in a debugger. The fix addresses the two most common root causes; if the
crash persists post-fix, a debug build with SDL_SetHint("SDL_LOGGING", "all") should capture
the event sequence.

---

### Feature 7: Gamepad Option Mode 1 Re-mapping to Arp Control

**Problem statement:** Option Mode 1 currently sets octave offsets via D-pad. v1.5 changes it
to: Circle=Arp on/off, Triangle=Rate, Square=Order, X (Cross)=RND Sync. R3 (right stick click)
panic is also removed.

**Stack decision:** Pure code change. Two files:

1. `GamepadInput.cpp`: In the `SDL_CONTROLLER_BUTTON_B/Y/X/A` looper-button section, add a
   mode check — when `optionMode_ == 1`, these buttons fire new `atomic<bool>` trigger flags
   (`arpToggleTrig_`, `arpRateTrig_`, `arpOrderTrig_`, `arpRndSyncTrig_`) instead of their
   normal looper functions.

2. `PluginProcessor.cpp`: In the Option Mode 1 D-pad dispatch block, consume the new trigger
   flags and call `stepWrappingParam` / `setValueNotifyingHost` on the arp parameters.

Remove the `rightStickTrig_` consumer from processBlock (single line deletion).

**New SDL2 APIs needed:** None. Existing `SDL_CONTROLLER_BUTTON_B/Y/X/A` polling,
`debounce()` helper, and `atomic<bool>` trigger pattern are reused.

**Confidence:** HIGH — mechanical remapping of existing dispatch logic.

---

### Feature 8: Looper Start Position Bug Fix

**Problem statement:** After the record cycle completes and playback begins, the looper plays
back from a wrong beat offset (typically mid-loop rather than beat 0).

**Root cause hypothesis:** `loopStartPpq_` in `LooperEngine` is either:
(a) Set to the ppqPosition at record *stop* instead of record *start*, or
(b) Set to the next bar boundary instead of the actual start ppqPosition, producing an off-by-one
    when the DAW ppqPosition at record start is not exactly on a bar boundary.

The relevant field `loopStartPpq_` is initialized to `-1.0` and set in `anchorToBar()`. The
`process()` method computes playback beat as `(currentPpq - loopStartPpq_) % loopLen`. If
`loopStartPpq_` is set to the bar boundary *before* the actual record start ppqPosition, the
offset is negative until the playhead crosses that boundary, then jumps to positive — producing
the observed wrong-start behavior.

**Stack decision:** No new API. The fix is a logic correction in `LooperEngine.cpp`:
`anchorToBar()` must be called at record *start* (not record *stop*) and must use the
*current ppqPosition* (not the next bar boundary) as the anchor when recording in free-running
(non-bar-aligned) mode.

**Confidence:** MEDIUM — structural diagnosis is correct based on code inspection. The exact
line requires adding a `DBG()` trace to `anchorToBar()` to confirm which scenario applies.

---

## Consolidated New APVTS Parameters

All parameters are added to `createParameterLayout()` following the existing helper pattern.

| Parameter ID | Type | Range | Default | Feature |
|-------------|------|-------|---------|---------|
| `singleChannelMode` | Bool | false/true | false | F1: single-channel routing |
| `singleChannelNum` | Int | 1..16 | 1 | F1: which MIDI channel |
| `subOct0..3` | Bool x4 | false/true | false | F2: sub octave per voice |
| `arpEnabled` | Bool | false/true | false | F3: arp on/off |
| `arpRate` | Choice | 0=1/4, 1=1/8, 2=1/16, 3=1/32 | 2 | F3: arp rate |
| `arpOrder` | Choice | 0=Up, 1=Down, 2=Random | 0 | F3: arp order |
| `arpGateLen` | Float | 0.05..0.99 | 0.5 | F3: gate length fraction |
| `leftStickXTarget` | Choice | 0..4 | 0 | F5: stick X target |
| `leftStickYTarget` | Choice | 0..4 | 0 | F5: stick Y target |

---

## New C++ Members by File

### `LfoEngine.h` additions
```cpp
static constexpr int LFO_REC_CAPACITY = 8192;
std::array<float, LFO_REC_CAPACITY> recBuf_ {};
int   recWritePos_  = 0;
int   recReadPos_   = 0;
int   recLength_    = 0;
std::atomic<bool> recArmed_   { false };
std::atomic<bool> recPlaying_ { false };
```

### `PluginProcessor.h` additions
```cpp
// Single-channel note collision tracker (audio-thread-only)
int noteRefCount_[128] = {};

// R3 + pad sub-oct toggle tracking (audio-thread-only)
bool r3HeldLastFrame_ = false;
```

### `GamepadInput.h` additions
```cpp
// Option Mode 1 arp control triggers
std::atomic<bool> arpToggleTrig_   { false };
std::atomic<bool> arpRateTrig_     { false };
std::atomic<bool> arpOrderTrig_    { false };
std::atomic<bool> arpRndSyncTrig_  { false };
```

---

## Explicit "Do Not Add" List

| What | Why Not |
|------|---------|
| Ableton Link | `AudioPlayHead::getPpqPosition()` provides sufficient DAW sync; Link adds network sync overhead unnecessary for a single-host MIDI plugin |
| `juce::MPEInstrument` | Input-side polyphony tracker for synths; this plugin generates MIDI, does not process it |
| `std::map` / `std::unordered_map` for note tracking | Dynamic allocation on audio thread is prohibited; `int[128]` array is O(1) and correct |
| `std::mutex` in processBlock path | Zero mutexes on audio thread is an existing invariant in this codebase; `std::atomic` and audio-thread-only state are sufficient |
| SDL3 upgrade | SDL2 2.30.9 has all required APIs; SDL3 breaks ABI and binary compatibility with the static lib build, and is a v2.0 concern |
| Second `juce::Timer` for arp or LFO recording | Existing 60 Hz `GamepadInput` timer and 30 Hz `PluginEditor` timer are sufficient; a third timer adds jitter and complexity |
| `juce_dsp` module | No audio processing; `isMidiEffect()=true` plugin has no `AudioBlock`; the module is dead weight |
| Third-party arpeggiator library | The arp is 40 lines of ppqPosition arithmetic extending the existing skeleton; a library adds build complexity for no benefit |
| Per-sample LFO recording | This plugin has no audio output; block-granular LFO recording (one float per processBlock call) is indistinguishable at MIDI resolution |

---

## JUCE Module Inventory (unchanged — no additions needed)

| Module | v1.5 Usage |
|--------|-----------|
| `juce_audio_processors` | APVTS new params, AudioPlayHead ppqPosition / getBpm / getIsPlaying |
| `juce_audio_basics` | MidiBuffer, MidiMessage, AbstractFifo (LooperEngine, unchanged) |
| `juce_core` | `juce::jlimit`, `juce::String`, `juce::Timer`, `std::atomic` wrappers |
| `juce_gui_basics` | PluginEditor UI controls for new parameters |

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Note collision (int[128]) | HIGH | Standard MIDI generator pattern; confirmed in JUCE forum stuck-note discussions |
| Sub Octave | HIGH | Trivial extension of existing note-on/off emission; no new API surface |
| Arp ppqPosition scheduling | HIGH | JUCE AudioPlayHead PositionInfo API verified against official JUCE 8 docs; pattern mirrors LfoEngine/LooperEngine |
| LFO recording ring buffer | HIGH | Single-thread array + atomics pattern mirrors LooperEngine AbstractFifo design; no SPSC sync needed |
| Left stick target routing | HIGH | Pattern identical to existing D-pad dispatch (confirmed in PluginProcessor.h) |
| SDL2 BT reconnect fix | MEDIUM | Root cause inferred from SDL2 issues #3697/#3468 and SDL2 wiki; not yet debugger-confirmed with a stack trace |
| Looper start-position fix | MEDIUM | Structural diagnosis correct; exact root requires DBG() trace to confirm which anchor scenario |
| Option Mode 1 remap | HIGH | Pure mechanical change to existing dispatch logic |

---

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Note collision tracking | `int[128]` ref count, audio-thread-only | `std::map<int,int>` | Dynamic allocation on audio thread is prohibited by JUCE audio thread safety rules |
| Arp timing | ppqPosition boundary math per block | JUCE `ArpeggiatorPluginDemo` sample-counter | Sample-counter drifts at non-integer BPMs (confirmed in JUCE forum "Inconsistent arpeggiations" thread) |
| LFO recording | `std::array<float, 8192>` inside LfoEngine | Extend LooperEngine with new event type | LooperEngine is designed for sparse timestamped events; a dense float stream requires a fundamentally different data model and would add unnecessary coupling |
| SDL2 BT fix | Instance-ID guard in timerCallback | Upgrading to SDL3 | SDL3 breaks ABI with the static lib build; requires CMakeLists.txt changes, potential API breakage in existing SDL2 calls; the fix is two lines of SDL2 code |
| Left stick targets | APVTS parameter dispatch in processBlock | Direct atomic write to LFO state | Bypassing APVTS means DAW automation, state save/load, and UI synchronization all break |

---

## Sources

- [JUCE AudioPlayHead::PositionInfo API](https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html) — getBpm, getPpqPosition, getIsPlaying, getBarCount methods confirmed (HIGH)
- [JUCE ArpeggiatorPluginDemo source](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/ArpeggiatorPluginDemo.h) — sample-counter timing approach with identified drift limitation (HIGH)
- [JUCE AbstractFifo Class Reference](http://docs.juce.com/master/classAbstractFifo.html) — SPSC design rationale; reason LFO recording uses plain array instead (HIGH)
- [SDL2 SDL_ControllerDeviceEvent wiki](https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent) — `which` = joystick instance ID on REMOVED events, device index on ADDED events (HIGH)
- [SDL2 SDL_GameControllerOpen wiki](https://wiki.libsdl.org/SDL2/SDL_GameControllerOpen) — "index passed to SDL_GameControllerOpen is not the value that will identify the controller in future events" (HIGH)
- [SDL2 issue #3697 — Re-plugging PS4 USB controller crashes SDL](https://github.com/libsdl-org/SDL/issues/3697) — crash on reconnect; duplicate of internal bug 5034 (MEDIUM)
- [SDL2 issue #3468 — Reconnect stops generating events after disconnect/reconnect](https://github.com/libsdl-org/SDL/issues/3468) — instance ID vs device index confusion (MEDIUM)
- [Raylib PR #4724 — Fix gamepad event handling by adding joystick instance id tracking](https://github.com/raysan5/raylib/pull/4724) — reference implementation of instance-ID guard (MEDIUM)
- [JUCE forum: stuck notes in MidiKeyboardState](https://forum.juce.com/t/possible-stuck-notes-in-midikeyboardstate/10316) — manual note-array tracking is the established JUCE plugin pattern (MEDIUM)
- [JUCE forum: MIDI events lost causing stuck notes](https://forum.juce.com/t/midi-events-lost-causing-stucked-notes/65518) — confirms custom ref-count approach (MEDIUM)
- Source code verified: `PluginProcessor.h` lines 274–295 (`stepClampingParam`/`stepWrappingParam`), lines 259–295 (existing arp state), `LfoEngine.h` (full class), `LooperEngine.h` (AbstractFifo double-buffer design), `GamepadInput.cpp` (SDL2 event loop)

---

*Stack research for: ChordJoystick v1.5 — Routing + Expression*
*Researched: 2026-02-28*
