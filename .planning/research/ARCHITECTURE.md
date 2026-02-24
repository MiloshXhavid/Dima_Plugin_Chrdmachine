# Architecture Research

**Domain:** JUCE VST3 MIDI Effect Plugin — v1.1 Feature Integration
**Researched:** 2026-02-24
**Confidence:** HIGH — based on direct source code analysis of existing codebase

---

## Scope

This document answers five specific architectural integration questions for ChordJoystick v1.1:

1. Full MIDI CC panic sweep — placement and flooding prevention
2. Trigger quantization — where the algorithm lives and thread safety
3. Live vs. post-record quantize — algorithm unification and beat math
4. Looper playback progress bar — jitter-free UI read-out
5. Gamepad name display — surfacing SDL_GameControllerName to UI

---

## System Overview

```
+-----------------------------------------------------------------+
|                     MESSAGE THREAD                              |
|  PluginEditor::timerCallback() — 50ms poll                      |
|  +---------------+  +------------------+  +------------------+ |
|  | Progress Bar  |  | Gamepad Status   |  | Panic/Mute UI    | |
|  | reads atomic  |  | reads string via |  | sets atomic      | |
|  | playbackBeat_ |  | GamepadInput API |  | pendingPanic_    | |
|  +-------+-------+  +--------+---------+  +--------+---------+ |
|          | (read)            | (read)               | (write)  |
+----------+-------------------+----------------------+----------+
|                   ATOMICS / FLAGS LAYER                         |
|  playbackBeat_ (atomic<float>)  controllerName_ (juce::String) |
|  getLoopLengthBeats()           pendingPanic_ (atomic<bool>)   |
+----------+-------------------+----------------------+----------+
|                     AUDIO THREAD                                |
|  PluginProcessor::processBlock()                                |
|  +--------------+  +--------------+  +------------------------+ |
|  | LooperEngine |  | TriggerSystem|  | Panic Sweep Section    | |
|  | .process()   |  | .processBlock|  | 16-channel CC emit     | |
|  | updates      |  | recordGate() |  | after allNotesOff      | |
|  | playbackBeat_|  | (quantize    |  |                        | |
|  |              |  |  gate pos)   |  |                        | |
|  +--------------+  +--------------+  +------------------------+ |
+-----------------------------------------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Thread |
|-----------|----------------|--------|
| `PluginProcessor::processBlock()` | Orchestrates all audio-thread logic; emits MIDI; reads atomics | Audio |
| `LooperEngine` | Records/plays back events; exposes `playbackBeat_` atomic; finalises recording | Audio + UI transport |
| `TriggerSystem` | Gate logic per voice; where live quantize intercepts gate beat position | Audio |
| `GamepadInput` | SDL2 timer at 60 Hz; writes atomics; `SDL_GameControllerName()` accessible on message thread | Message (timer) |
| `PluginEditor::timerCallback()` | 50ms poll of atomics for UI repaint; drives progress bar, status labels | Message |

---

## Feature 1: Full MIDI CC Panic Sweep

### Question
Where in processBlock? Should it extend the existing `pendingPanic_` pattern? How to avoid flooding?

### Recommendation
Extend the existing `pendingPanic_` atomic exchange pattern. Do not add a new mechanism.

### Placement Analysis

The panic block already exists at the correct location in `PluginProcessor.cpp` (after DAW-stop detection, before the `midiMuted_` early-return). This position is correct for two reasons:

1. It runs unconditionally before the `midiMuted_` gate, so allNotesOff always reaches the synth even when muted.
2. The `exchange(false, acq_rel)` fires exactly once per button press, regardless of how many times the message thread set `pendingPanic_ = true` between blocks.

### Current vs. Extended Implementation

Current v1.0 sends allNotesOff on the 4 voice channels only:
```cpp
if (pendingPanic_.exchange(false, std::memory_order_acq_rel))
{
    if (looper_.isPlaying()) looper_.startStop();
    for (int v = 0; v < 4; ++v)
        midi.addEvent(juce::MidiMessage::allNotesOff(voiceChs[v]), 0);
    trigger_.resetAllGates();
    flashPanic_.fetch_add(1, std::memory_order_relaxed);
}
```

v1.1 full sweep sends CC123, CC120, CC64=0, CC121 on all 16 channels:
```cpp
if (pendingPanic_.exchange(false, std::memory_order_acq_rel))
{
    if (looper_.isPlaying()) looper_.startStop();

    // Full CC reset sweep on all 16 channels.
    // CC123 = All Notes Off, CC120 = All Sound Off,
    // CC64 = 0 (sustain pedal off), CC121 = Reset All Controllers.
    for (int ch = 1; ch <= 16; ++ch)
    {
        midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);           // CC123
        midi.addEvent(juce::MidiMessage::allSoundOff(ch), 0);           // CC120
        midi.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0); // sustain off
        midi.addEvent(juce::MidiMessage::controllerEvent(ch, 121, 0), 0); // reset controllers
    }
    trigger_.resetAllGates();
    flashPanic_.fetch_add(1, std::memory_order_relaxed);
}
```

### Flood Prevention Analysis

16 channels x 4 messages = 64 events per panic. At ~5 bytes per MIDI event this is approximately 320 bytes added to the MidiBuffer in a single block. This fires exactly once per button press via the atomic exchange pattern. No spread-across-blocks mechanism is needed or advisable — atomicity of the panic sweep is more important than distribution.

**Thread safety:** Identical to existing pattern. `pendingPanic_` is `std::atomic<bool>`. UI writes `store(true, relaxed)`. Audio thread reads via `exchange(false, acq_rel)`. The `acq_rel` on the exchange ensures all writes made before the store are visible when the audio thread processes the flag.

**Files modified:** `PluginProcessor.cpp` only — ~10 lines changed inside the existing panic block.

---

## Feature 2: Trigger Quantization — Where the Algorithm Lives

### Question
(a) Inside `LooperEngine::recordGate()` before FIFO write, (b) inside `finaliseRecording()` as a pass over `playbackStore_`, or (c) a separate `quantize()` method called from UI?

### Recommendation
Option (a) for live quantize. Option (b) for post-record quantize. They share one static algorithm function. Option (c) is rejected.

### Analysis

**Option (a) — quantize inside `recordGate()` before FIFO write**

`recordGate()` is declared audio-thread-only in `LooperEngine.h` (the `ASSERT_AUDIO_THREAD()` macro pattern is used throughout the file). It receives `beatPos` and writes a `LooperEvent` into the SPSC FIFO before `finaliseRecording()` drains it. Snapping `beatPos` here means the stored value is already grid-aligned — no post-processing needed.

The subdivision parameter must be readable on the audio thread. Add `std::atomic<int> quantizeSubdivIdx_` using the same pattern as the existing `std::atomic<int> subdiv_`. The live quantize arm flag is `std::atomic<bool> liveQuantize_`.

No new concurrency surface: `recordGate()` is already audio-thread-only, and the new atomic reads follow the established `subdiv_.load()` pattern.

**Option (b) — pass over `playbackStore_` inside `finaliseRecording()`**

`finaliseRecording()` is called from `startStop()` or `record()` (transport methods called from message thread). The threading invariant documented in `LooperEngine.h` is explicit: "MUST be called only when `playing_=false` AND `recording_=false`." At this point no concurrent reader exists for `playbackStore_[]`, and the FIFO writer (audio thread) has already stopped because `recording_=false`.

A full pass over `scratchNew_[]` (which is populated from the FIFO during `finaliseRecording()`) to re-snap gate `beatPosition` values is safe: no atomics required, no race conditions.

For the UI QUANTIZE button (post-record re-quantize of an already-finalized loop), expose a new public method `quantizePlayback(double subdivBeats)`. This operates only on the already-stable `playbackStore_[]`. It must be called only when `!isPlaying() && !isRecording()` — the UI must enforce this precondition.

**Option (c) — separate `quantize()` called from UI thread, reading/writing `playbackStore_[]` concurrently**

`playbackStore_[]` is a plain `std::array`, not an atomic container. The audio thread's `process()` reads it every block during playback. A concurrent write from the message thread is an undefined behaviour data race. This option is rejected.

### Thread Safety Summary

| Approach | When Called | Thread | Safety Verdict |
|----------|-------------|--------|----------------|
| Live quantize in `recordGate()` | Every gate event during recording | Audio thread only | Safe — audio-thread-only method, reads atomic subdivIdx |
| Post-record in `finaliseRecording()` | When recording stops | Message thread; invariant: playing_=false AND recording_=false | Safe — no concurrent access to playbackStore_ |
| UI QUANTIZE button calling `quantizePlayback()` | User presses button | Message thread; only when !isPlaying() && !isRecording() | Safe only when UI enforces precondition |
| UI thread writing playbackStore_[] directly | Any time | Message thread concurrent with audio | UNSAFE — undefined behaviour data race |

### New LooperEngine API

```cpp
// Public — quantizes all Gate events in playbackStore_ to nearest grid.
// THREADING: MUST be called only when playing_=false AND recording_=false.
// subdivBeats: 1.0=1/4, 0.5=1/8, 0.25=1/16, 0.125=1/32
void quantizePlayback(double subdivBeats);

// Arms live quantize — audio thread reads this in recordGate().
void setLiveQuantize(bool b)   { liveQuantize_.store(b);         }
bool isLiveQuantize()  const   { return liveQuantize_.load();    }

// Quantize subdivision (shared by live and post-record).
void setQuantizeSubdiv(int idx) { quantizeSubdivIdx_.store(idx); }
int  getQuantizeSubdiv()  const { return quantizeSubdivIdx_.load(); }

// Private new members:
// std::atomic<bool> liveQuantize_  { false };
// std::atomic<int>  quantizeSubdivIdx_ { 1 };  // default 1/8
```

---

## Feature 3: Live vs. Post-Record Quantize — Shared Algorithm

### Question
Do they use the same algorithm? How to implement "round to nearest subdivision" given a beat position and BPM?

### Answer
Yes. They use identical math. BPM is not needed — the algorithm operates entirely in beats.

### Beat-Domain Snap Algorithm

```cpp
// Static helper — usable from audio thread and message thread.
// beatPos:    beat offset within loop (0..loopLengthBeats)
// subdivBeats: grid size in beats (e.g. 0.5 = 1/8th note)
// loopBeats:  total loop length in beats (for wrap clamping)
// Returns: snapped beat position
static double snapToGrid(double beatPos, double subdivBeats, double loopBeats)
{
    if (subdivBeats <= 0.0) return beatPos;

    const double grid = std::round(beatPos / subdivBeats) * subdivBeats;

    // Clamp to [0, loopBeats) — rounding may push past loop end.
    return std::fmod(std::max(0.0, grid), loopBeats);
}
```

**Why BPM is not needed:** `beatPos` is already expressed in beat units (quarter notes). Subdivision size is expressed in beats. 1/4 note = 1.0 beat, 1/8 note = 0.5 beats, 1/16 note = 0.25 beats, 1/32 note = 0.125 beats. This mapping is purely metric and tempo-independent.

### Subdivision Index to Beats Mapping

Consistent with the existing `randomSubdiv` parameter index scheme already in the codebase:

| Index | Subdivision | Beats |
|-------|-------------|-------|
| 0 | 1/4 | 1.0 |
| 1 | 1/8 | 0.5 |
| 2 | 1/16 | 0.25 |
| 3 | 1/32 | 0.125 |

```cpp
static double subdivIndexToBeats(int idx)
{
    static const double kTable[] = { 1.0, 0.5, 0.25, 0.125 };
    return kTable[juce::jlimit(0, 3, idx)];
}
```

### Live Quantize Path

```cpp
void LooperEngine::recordGate(double beatPos, int voice, bool on)
{
    if (liveQuantize_.load())
    {
        const double subdiv = subdivIndexToBeats(quantizeSubdivIdx_.load());
        beatPos = snapToGrid(beatPos, subdiv, getLoopLengthBeats());
    }
    // ... existing FIFO write code unchanged ...
}
```

### Post-Record Quantize Path

```cpp
void LooperEngine::quantizePlayback(double subdivBeats)
{
    // Caller must have asserted playing_=false AND recording_=false.
    const double loopBeats = getLoopLengthBeats();
    const int    count     = playbackCount_.load();

    for (int i = 0; i < count; ++i)
    {
        auto& ev = playbackStore_[i];
        if (ev.type == LooperEvent::Type::Gate)
            ev.beatPosition = snapToGrid(ev.beatPosition, subdivBeats, loopBeats);
    }

    // Re-sort after quantization — two events may have converged to the same beat.
    // stable_sort preserves relative order of events at the same position (on before off).
    std::stable_sort(playbackStore_.begin(), playbackStore_.begin() + count,
        [](const LooperEvent& a, const LooperEvent& b) {
            return a.beatPosition < b.beatPosition;
        });
}
```

### Gate-Pair Handling Decision

When gate-on events are snapped to grid, gate-off events retain their original position. This preserves original gate duration (the "feel" of the note length). The snap only affects when the note starts, not how long it sustains. This is the simpler and more musical implementation. A future option to also quantize gate-off events (for a strict "quantize to 1/16 grid" feel) can be a v1.2 feature.

### New APVTS Parameter

```cpp
// In createParameterLayout():
const juce::StringArray kQuantSubdivChoices { "1/4", "1/8", "1/16", "1/32" };
layout.add(std::make_unique<juce::AudioParameterChoice>(
    "quantizeSubdiv", "Quantize Grid", kQuantSubdivChoices, 1));  // default 1/8
```

---

## Feature 4: Looper Playback Progress Bar — Jitter-Free UI Read-out

### Question
`LooperEngine` exposes `getPlaybackBeat()` and `getLoopLengthBeats()`. How should `PluginEditor::timerCallback()` use these to drive a progress bar without jitter?

### Existing Infrastructure (confirmed from LooperEngine.h)

- `playbackBeat_` is `std::atomic<float>` — written by audio thread each block in `process()`, read by message thread.
- The header includes a compile-time assertion: `static_assert(sizeof(float) == 4)` plus the comment "lock-free guarantee on 32-bit builds". This is safe to read from timerCallback.
- `getLoopLengthBeats()` is a pure computation from `subdiv_` and `loopBars_` atomics — no state mutation.
- `getPlaybackBeat()` returns `static_cast<double>(playbackBeat_.load())` — already provided.

### Normalized Position Pattern

```cpp
// In PluginEditor::timerCallback():
if (proc_.looperIsPlaying())
{
    const double beat  = proc_.looperGetPlaybackBeat();
    const double total = proc_.looperGetLoopLengthBeats();
    const float  t     = (total > 0.0)
                          ? static_cast<float>(beat / total)
                          : 0.0f;

    // jlimit guards against stale read at loop-wrap boundary.
    looperProgressBar_.setProgress(juce::jlimit(0.0f, 1.0f, t));
    looperProgressBar_.repaint();
}
else
{
    looperProgressBar_.setProgress(0.0f);
    looperProgressBar_.repaint();
}
```

### Jitter Sources and Mitigations

| Source | Cause | Mitigation |
|--------|-------|-----------|
| Atomic float read at loop wrap | `playbackBeat_` resets to 0 mid-cycle; timer may read stale ~0.99 then see 0.0 next tick | `jlimit(0, 1)` clamp; the 0.99→0.0 visual jump is correct wrap behaviour |
| 50ms timer vs audio block rate | Position appears to jump by ~0.1 beats per tick at 120 BPM | Acceptable — 50ms at 120 BPM = 0.1 beats; over an 8-bar loop that is <1% per tick |
| `getLoopLengthBeats()` returning 0 | Division by zero before first config | Guard with `total > 0.0` check |
| Timer fires while looper stopped | Shows stale non-zero position | Return `0.0f` unconditionally when `!looperIsPlaying()` |

### Progress Bar Widget Choice

Use a custom `juce::Component` subclass with a `float progress_` member rather than `juce::ProgressBar`. Rationale:

- `juce::ProgressBar` has a built-in animation model (spinning indeterminate mode) that conflicts with audio-driven position updates.
- A custom paintable component calling `repaint()` from the timer gives full control over appearance (consistent with the PixelLookAndFeel aesthetic).
- `paint()` simply draws a filled rectangle proportional to `progress_`.

### Forwarding Methods on PluginProcessor

`LooperEngine` is a private member of `PluginProcessor`. Add forwarding methods consistent with the existing pattern:

```cpp
// In PluginProcessor.h:
double looperGetPlaybackBeat()    const { return looper_.getPlaybackBeat();    }
double looperGetLoopLengthBeats() const { return looper_.getLoopLengthBeats(); }
```

---

## Feature 5: Gamepad Name Display — SDL_GameControllerName()

### Question
SDL2 provides `SDL_GameControllerName()` — how to surface this in UI?

### API (HIGH confidence — SDL2 official wiki)

```c
const char* SDL_GameControllerName(SDL_GameController *gamecontroller);
```

Returns a UTF-8 encoded static string (controller owns it, do not free), or NULL if `gamecontroller` is NULL. Available since SDL 2.0.0.

### Threading Analysis

`GamepadInput` owns `SDL_GameController* controller_` as a private member. Both `tryOpenController()` and `closeController()` are called from `timerCallback()`, which runs on the message thread. `PluginEditor::timerCallback()` also runs on the message thread. Since both timers run on the same thread (JUCE `juce::Timer` callbacks always run on the message thread), `controllerName_` can be a plain `juce::String` with no atomics required.

`SDL_GameControllerName()` must be called on the message thread. It is not documented as thread-safe in the SDL2 wiki. Calling it from the audio thread would be unsafe; the message-thread-only approach is the conservative correct choice.

### Implementation Pattern

```cpp
// GamepadInput.h — add public accessor:
juce::String getControllerName() const { return controllerName_; }

// GamepadInput.h — add private member:
// juce::String controllerName_;  // message thread only, no atomics needed
```

```cpp
// GamepadInput.cpp — in timerCallback(), after tryOpenController():
if (controller_ != nullptr)
{
    const char* name = SDL_GameControllerName(controller_);
    controllerName_ = (name != nullptr) ? juce::String(name) : "Unknown Controller";
}
else
{
    controllerName_ = "";
}
```

```cpp
// PluginEditor.cpp — in timerCallback():
const auto& gp = proc_.getGamepad();
if (gp.isConnected())
    gamepadStatusLabel_.setText("GAMEPAD: " + gp.getControllerName(),
                                juce::dontSendNotification);
else
    gamepadStatusLabel_.setText("NO GAMEPAD", juce::dontSendNotification);
```

`SDL_GameControllerName()` uses SDL's gamecontroller database. For recognized devices it returns the mapped name (e.g., "PS4 Controller", "Xbox 360 Controller"). For unrecognized devices it may return a generic USB descriptor string — this is acceptable and expected.

---

## Data Flow

### processBlock() Step Order (v1.1)

```
processBlock()
  1. setDeadZone() sync
  2. Gamepad poll — voice gates, looper transport, D-pad, R3 (panic toggle)
  3. DAW playhead query — ppqPos, bpm, isDawPlaying
  4. looper_.process() — returns BlockOutput (joystick, filter, gate overrides)
  5. DAW stop detection — allNotesOff + resetAllGates
  6. MIDI Panic sweep — pendingPanic_ exchange; 16-channel CC sweep [v1.1 extended]
  7. midiMuted_ early-return gate
  8. Looper gate playback — direct MIDI emit (bypasses TriggerSystem)
  9. TriggerSystem::processBlock() — onNote callback -> looper_.recordGate()
     (live quantize: snapToGrid() applied here if liveQuantize_ armed) [v1.1]
  10. looper_.activateRecordingNow() if anyNoteOnThisBlock
  11. Looper joystick + filter recording
  12. Filter CC section — pendingAllNotesOff_, pendingCcReset_, CC emit
  13. Looper config sync (subdiv, length)
```

No step reordering required. The panic sweep at step 6 is a drop-in replacement. Live quantize at step 9 is transparent inside `recordGate()`.

### Quantize Data Flow

```
Live quantize:
  Pad press -> setPadState() -> TriggerSystem gate-on -> onNote callback
           -> looper_.recordGate(beatPos, voice, true)
              -> [if liveQuantize_] snapToGrid(beatPos, subdivBeats, loopBeats)
              -> fifo_.write(snapped LooperEvent)

Post-record quantize (UI QUANTIZE button):
  User presses QUANTIZE -> PluginEditor -> proc_.looperQuantize(subdivBeats)
                        -> [assert !isPlaying && !isRecording]
                        -> looper_.quantizePlayback(subdivBeats)
                           -> pass over playbackStore_[0..count]
                           -> snapToGrid() each Gate event's beatPosition
                           -> stable_sort by beatPosition
```

---

## Build Order and Phase Dependencies

| Phase Topic | Depends On | Notes |
|-------------|------------|-------|
| Full panic sweep | Nothing — isolated change | Extends existing panic block; no new abstractions |
| New APVTS param `quantizeSubdiv` | Nothing | Needed before UI or quantize logic references it |
| `snapToGrid()` static helper + `subdivIndexToBeats()` | Nothing | Pure math, no state |
| Live quantize atomics in `LooperEngine` | APVTS param for grid selection | `liveQuantize_`, `quantizeSubdivIdx_` atomics |
| `LooperEngine::recordGate()` live snap | `snapToGrid()`, live quantize atomics | Audio-thread-only change |
| `LooperEngine::quantizePlayback()` | `snapToGrid()`, APVTS param | Post-record method |
| `PluginProcessor` forwarding methods | LooperEngine new API | `looperQuantize()`, `looperSetLiveQuantize()`, playback beat/length |
| Looper progress bar widget + timerCallback | Forwarding methods exist | Pure UI, no audio impact |
| Gamepad name display | `getControllerName()` accessor | Message thread only, no audio impact |

**Recommended phase build order:**
1. Full panic sweep (isolated, test immediately in DAW)
2. APVTS parameter additions (quantizeSubdiv)
3. LooperEngine quantize methods + atomics
4. PluginProcessor forwarding methods
5. Looper progress bar (UI only)
6. Gamepad name display (UI only)

---

## Anti-Patterns

### Anti-Pattern 1: Calling quantizePlayback() While Looper Is Playing

**What people do:** Bind the QUANTIZE button directly to `looper_.quantizePlayback()` without checking state.

**Why it is wrong:** `playbackStore_[]` is a plain `std::array` read every audio block during playback. Writing to it from the message thread while the audio thread reads it is an undefined behaviour data race. There is no mutex protecting `playbackStore_[]` — the design relies entirely on the "not-playing + not-recording" invariant for all writes to that buffer.

**Do this instead:** Gate the UI button: enable it and call `quantizePlayback()` only when `!looperIsPlaying() && !looperIsRecording()`. Add a `jassert` inside `quantizePlayback()` to catch violations in Debug builds.

### Anti-Pattern 2: Reading SDL_GameControllerName() from Audio Thread

**What people do:** Call `SDL_GameControllerName(controller_)` inside `processBlock()` to display the controller type.

**Why it is wrong:** `SDL_GameControllerName()` is not documented as thread-safe. SDL2's internal state for controller names is managed on the message thread where `SDL_PumpEvents()` runs. Accessing it from the audio thread risks reading partially-updated internal state.

**Do this instead:** Cache the name string in `GamepadInput::timerCallback()` (message thread) as a plain `juce::String`. Read the cached string from `PluginEditor::timerCallback()` (also message thread). Same thread, no atomics needed.

### Anti-Pattern 3: Spreading Panic CCs Across Multiple Blocks

**What people do:** Emit panic CCs one channel per block to avoid "flooding" the MIDI buffer.

**Why it is wrong:** Atomicity of the panic sweep matters more than distribution. If any block is dropped or the DAW discards partial output, some channels will not receive the reset — exactly the failure mode panic is meant to prevent. Additionally, the loop is playing while only partial channels are reset, which can leave hanging notes.

**Do this instead:** Emit all 64 events in a single block. 16 channels x 4 events x ~5 bytes = 320 bytes. This is negligible compared to typical DAW MIDI buffer sizes (commonly 4-64 KB). The `exchange(false)` pattern ensures it fires exactly once.

### Anti-Pattern 4: Using BPM in the Quantize Snap Calculation

**What people do:** Convert beat position to seconds using BPM, snap to the nearest time grid boundary in seconds, then convert back.

**Why it is wrong:** Introduces a dependency on BPM accuracy. When DAW BPM automation is active, BPM changes mid-playback, making the seconds-domain calculation incorrect. Also introduces latency if BPM is read from `effectiveBpm_` with relaxed ordering.

**Do this instead:** Snap directly in beat space. `snapToGrid(beatPos, subdivBeats, loopBeats)` is tempo-independent and always correct. The subdivision table (1.0, 0.5, 0.25, 0.125 beats) is a fixed metric relationship.

### Anti-Pattern 5: Using juce::ProgressBar for the Playback Indicator

**What people do:** Instantiate `juce::ProgressBar` and call `setValue()` from timerCallback.

**Why it is wrong:** `juce::ProgressBar` has a built-in animation mode for indeterminate progress (spinning/pulsing) that activates when the value is negative and cannot be fully disabled through the public API. It also applies its own LookAndFeel rendering that does not match the PixelLookAndFeel aesthetic.

**Do this instead:** A custom `juce::Component` subclass with `paint()` drawing a filled rectangle proportional to the progress float. This gives full visual control and matches the existing pixel-art UI style.

---

## Integration Points

### Files Modified by v1.1 Features

| File | Change | Feature |
|------|--------|---------|
| `PluginProcessor.cpp` | Extend panic block to 16-channel sweep (4 CCs per channel) | Panic sweep |
| `PluginProcessor.h` | Add `looperGetPlaybackBeat()`, `looperGetLoopLengthBeats()`, `looperQuantize()`, `looperSetLiveQuantize()`, `looperSetQuantizeSubdiv()` forwarders | Progress bar, quantization |
| `PluginProcessor.cpp` | Add `quantizeSubdiv` APVTS parameter in `createParameterLayout()` | Quantization |
| `LooperEngine.h` | Add `quantizePlayback()`, `setLiveQuantize()`, `setQuantizeSubdiv()`, `isLiveQuantize()`, `getQuantizeSubdiv()` declarations; `liveQuantize_` and `quantizeSubdivIdx_` atomics | Quantization |
| `LooperEngine.cpp` | Add `snapToGrid()` static helper, `subdivIndexToBeats()`, implement `quantizePlayback()`, modify `recordGate()` for live snap | Quantization |
| `GamepadInput.h` | Add `getControllerName()` accessor; `controllerName_` private member | Gamepad name |
| `GamepadInput.cpp` | Set `controllerName_` in `timerCallback()` after open/close | Gamepad name |
| `PluginEditor.h` | Add looper progress bar component member | Progress bar |
| `PluginEditor.cpp` | Update `timerCallback()` for progress bar read, gamepad name display, QUANTIZE button; add new APVTS attachment for `quantizeSubdiv` | All UI features |

### Internal Boundaries

| Boundary | Communication | Thread Safety |
|----------|---------------|--------------|
| PluginEditor -> PluginProcessor | Forwarding methods only | Maintained — no direct member access |
| PluginProcessor -> LooperEngine (quantize) | `looper_.quantizePlayback()` | Valid only when !playing_ && !recording_; must be asserted at call site |
| Audio thread -> LooperEngine::recordGate() | Live quantize reads `liveQuantize_` and `quantizeSubdivIdx_` atomics with `load()` | Safe — audio-thread-only method |
| GamepadInput timerCallback -> controllerName_ | Plain juce::String write | Safe — message thread only |
| PluginEditor timerCallback -> getControllerName() | Plain juce::String read | Safe — message thread only (same thread as writer) |
| PluginEditor timerCallback -> looperGetPlaybackBeat() | `std::atomic<float>` read via forwarding method | Safe — established pattern |

---

## Confidence Assessment

| Area | Confidence | Basis |
|------|------------|-------|
| Panic sweep placement and buffer size | HIGH | Direct source analysis of processBlock(); MIDI event sizing is deterministic |
| Quantize option (a) thread safety | HIGH | `ASSERT_AUDIO_THREAD()` macro confirmed in LooperEngine.h; audio-thread-only path |
| quantizePlayback() invariant safety | HIGH | LooperEngine.h documents the exact threading invariant; `finaliseRecording()` already follows identical pattern |
| snapToGrid() beat-domain math | HIGH | Beat-unit metric math is independent of BPM; no implementation ambiguity |
| Progress bar atomic float safety | HIGH | `playbackBeat_` is `std::atomic<float>` with explicit lock-free assertion in LooperEngine.h |
| SDL_GameControllerName() thread safety | MEDIUM | SDL2 wiki confirms API signature and availability since SDL 2.0.0; thread-safety not explicitly documented; message-thread-only usage is conservative safe approach |
| JUCE MidiBuffer capacity for 64-event panic | MEDIUM | No official capacity limit documented; 320 bytes is well within all tested DAW implementations; JUCE MidiBuffer grows dynamically |

---

## Sources

### Primary (HIGH confidence)
- `Source/LooperEngine.h` — Threading invariants, SPSC FIFO design, `playbackBeat_` atomic type, `ASSERT_AUDIO_THREAD()` macro, `finaliseRecording()` invariant comment
- `Source/PluginProcessor.cpp` — Full processBlock() structure, existing panic block (lines 471-478), `recordGate()` call sites, `pendingPanic_` exchange pattern
- `Source/PluginProcessor.h` — `pendingPanic_`, flash atomics, `looper_` private member, existing forwarding method pattern
- `Source/GamepadInput.h` — `onConnectionChangeUI` callback, `controller_` private member, `juce::Timer` inheritance
- `Source/PluginEditor.h` — `timerCallback()` declaration, flash counter pattern, `gamepadStatusLabel_` member
- [JUCE AbstractFifo Class Reference](http://docs.juce.com/master/classAbstractFifo.html) — SPSC lock-free design, audio-thread usage

### Secondary (MEDIUM confidence)
- [SDL2 SDL_GameControllerName Wiki](https://wiki.libsdl.org/SDL2/SDL_GameControllerName) — Function signature, return value semantics, availability since SDL 2.0.0
- [JUCE AbstractFifo forum — thread safety discussion](https://forum.juce.com/t/abstractfifo-single-consumer-single-producer-thread-safety/50749) — SPSC usage patterns in JUCE audio plugins

---

*Architecture research for: ChordJoystick v1.1 feature integration*
*Researched: 2026-02-24*
