# Architecture Research: ChordJoystick VST3 MIDI Generator

**Domain:** JUCE VST3 MIDI-only plugin (no audio, 4-voice chord generator)
**Researched:** 2026-02-22
**Confidence:** HIGH (analysis based directly on existing source files; JUCE patterns verified from codebase)

---

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        UI THREAD (Message Thread)                    │
│                                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐    │
│  │  JoystickPad  │  │  TouchPlate  │  │  PluginEditor (Timer)  │    │
│  │  (mouse drag) │  │ (mouseDown)  │  │  (30Hz repaint / sync) │    │
│  └──────┬────────┘  └──────┬───────┘  └──────────┬─────────────┘   │
│         │ atomic write     │ setPadState()        │ read atomics     │
├─────────┼──────────────────┼──────────────────────┼─────────────────┤
│         ▼ joystickX/Y      ▼ padPressed_[]         ▼ gateOpen_[]    │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │              juce::AudioProcessorValueTreeState (APVTS)       │   │
│  │   SliderAttachment / ComboBoxAttachment / ButtonAttachment    │   │
│  │   getRawParameterValue() → std::atomic<float>*               │   │
│  └──────────────────────────────┬───────────────────────────────┘   │
│                                 │ lock-free param reads              │
├─────────────────────────────────┼────────────────────────────────────┤
│                                 │                                    │
│            GAMEPAD THREAD (juce::Timer, 60Hz, main thread)          │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  GamepadInput (SDL2 poll) → atomic writes (pitchX/Y, trigs)  │   │
│  └──────────────────────────────┬───────────────────────────────┘   │
│                                 │ consume atomics in processBlock    │
├─────────────────────────────────┼────────────────────────────────────┤
│                                 ▼                                    │
│                        AUDIO THREAD (processBlock)                   │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  PluginProcessor::processBlock()                             │    │
│  │  1. consume GamepadInput edge-flags (atomic::exchange)       │    │
│  │  2. LooperEngine::process() → BlockOutput (joystick/gates)   │    │
│  │  3. buildChordParams() → read APVTS atomics + joystickX/Y   │    │
│  │  4. ChordEngine::computePitch() per voice (pure function)    │    │
│  │  5. TriggerSystem::processBlock() → fires NoteCallback       │    │
│  │  6. NoteCallback → MidiBuffer::addEvent() (note-on/off)     │    │
│  │  7. looper_.recordJoystick/Gate() [mutex — critical path]    │    │
│  │  8. Filter CC events → MidiBuffer::addEvent()               │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                 │                                    │
│                                 ▼                                    │
│                        DAW / VST3 HOST                               │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  AudioPlayHead (ppqPosition, bpm, isPlaying)                 │    │
│  │  MidiBuffer → downstream synth/instrument track             │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Thread | Responsibility | Communicates With |
|-----------|--------|----------------|-------------------|
| `PluginProcessor` | Audio (owns processBlock), all (owns APVTS) | Orchestrates audio-thread processing; owns APVTS, coordinates all subsystems | All components |
| `ChordEngine` | Audio (called from processBlock) | Pure stateless pitch computation: joystick + intervals + octave + scale → MIDI note 0-127 | ScaleQuantizer (inline), called by PluginProcessor |
| `ScaleQuantizer` | Audio (called by ChordEngine) | Stateless: maps raw MIDI pitch to nearest scale note; 2-octave search, ties go down | None (pure functions) |
| `TriggerSystem` | Straddled: UI writes atomics, audio reads | Gate state machine per voice; fires NoteCallback at sample offset; handles TouchPlate / Joystick / Random sources | PluginProcessor (callback), padPressed_ atomics from UI |
| `LooperEngine` | Straddled: UI writes atomics, audio calls process() | Beat-timestamped event recording and playback; DAW-synced or internal clock | PluginProcessor (process call, record calls); mutex for event vector |
| `GamepadInput` | Main (juce::Timer at 60Hz) | SDL2 poll loop; normalises axes; edge-detects buttons; writes atomics | PluginProcessor (consumeVoiceTrigger etc.), PluginEditor (isConnected) |
| `PluginEditor` | UI (message thread) | All JUCE Component rendering; APVTS attachments for knobs/combos; 30Hz Timer repaint for gate LEDs | PluginProcessor (joystickX/Y, isGateOpen, looper state) |
| `JoystickPad` | UI | Mouse → atomic joystickX/Y write | PluginProcessor.joystickX/Y (std::atomic<float>) |
| `TouchPlate` | UI | Mouse down/up → PluginProcessor.setPadState() | PluginProcessor via setPadState() |
| `ScaleKeyboard` | UI | 12 toggle buttons → APVTS scaleNote0..11 parameters | APVTS directly |

---

## Thread Boundary Map

This is the most critical architectural concern for a JUCE plugin. Every inter-thread access must be through a safe mechanism.

### Thread Inventory

| Thread | Who Runs It | Timing | Real-time Constraint |
|--------|-------------|--------|----------------------|
| Audio thread | DAW host | Every block (~5ms at 44.1kHz/256 samples) | YES — no blocking allowed |
| Message thread | JUCE message loop | Event-driven | No |
| Timer thread (juce::Timer) | JUCE schedules on message thread | ~16ms (60Hz) | No |

Important: `juce::Timer` callbacks run on the **message thread**, not a separate thread. GamepadInput and PluginEditor both use juce::Timer — they run sequentially on the same message thread.

### Cross-Thread Communication Mechanisms

| Data Path | Mechanism | Correct? | Notes |
|-----------|-----------|----------|-------|
| UI mouse → joystickX/Y → audio | `std::atomic<float>` | YES | Verified in PluginProcessor.h (line 43-44) |
| UI setPadState → padPressed_ | `std::atomic<bool>` per voice | YES | TriggerSystem.h (line 72-73) |
| Gamepad timer → pitchX/Y/filterX/Y | `std::atomic<float>` | YES | GamepadInput.h (line 65-68) |
| Gamepad timer → button flags | `std::atomic<bool>` + exchange() | YES | GamepadInput.h (line 70-75); consume pattern avoids lost events |
| Gamepad → looper transport | Consumed in processBlock via exchange() | YES | PluginProcessor.cpp (lines 224-227); audio thread reads edge flags |
| LooperEngine transport (play/record) | `std::atomic<bool>` | YES | LooperEngine.h (line 83-84) |
| LooperEngine event storage | `std::mutex` + `std::vector` | RISK — see below | LooperEngine.cpp (line 143) — mutex locked in processBlock |
| APVTS parameters → audio thread | `getRawParameterValue()` returns `std::atomic<float>*` | YES | JUCE-documented as audio-thread safe |
| Audio thread → UI (gate state) | `std::atomic<bool>` per voice (gateOpen_) | YES | Read by isGateOpen() from UI timer |
| Audio thread → UI (playback beat) | `std::atomic<double>` | YES | getPlaybackBeat() in LooperEngine |

---

## Recommended Project Structure

```
Source/
├── PluginProcessor.h/.cpp   # AudioProcessor: orchestration, processBlock, APVTS
├── PluginEditor.h/.cpp      # AudioProcessorEditor: all UI, APVTS attachments
├── ChordEngine.h/.cpp       # Stateless pitch computation (audio thread)
├── ScaleQuantizer.h/.cpp    # Stateless scale quantization (audio thread)
├── TriggerSystem.h/.cpp     # Gate state machine, 4-voice (straddled)
├── LooperEngine.h/.cpp      # Beat event looper (straddled, has mutex risk)
└── GamepadInput.h/.cpp      # SDL2 poll on Timer, atomic output (message thread)
```

This flat structure is correct for a single-module JUCE plugin. No subfolders needed at this scale.

---

## Architectural Patterns

### Pattern 1: Atomic Boundary for UI-to-Audio Communication

**What:** Every piece of data written by the UI or gamepad thread and read by the audio thread is wrapped in `std::atomic<T>`. Reads in processBlock use `.load()`, writes use `.store()` or `.exchange()`.

**When to use:** All scalar values that cross thread boundaries: joystick position, pad state, gate state, playback beat position.

**Trade-offs:** Zero blocking, zero allocations, audio-thread safe. Limited to scalar types (float, bool, int, double). Not suitable for variable-length data like the event vector.

**Example from codebase:**
```cpp
// PluginProcessor.h
std::atomic<float> joystickX {0.0f};
std::atomic<float> joystickY {0.0f};

// JoystickPad (UI thread) — writes:
proc_.joystickX.store(normX);

// processBlock (audio thread) — reads:
const float jx = joystickX.load();
```

### Pattern 2: Edge-Flag Consume Pattern for Button Events

**What:** A button press is a rising-edge event. Set an `atomic<bool>` to true on the rising edge; the audio thread reads it with `exchange(false)`, consuming the flag in one atomic operation. This guarantees each button press fires exactly once even if the audio block runs after multiple timer callbacks.

**When to use:** Any discrete event (button press, looper transport command) where missing an event is wrong and double-firing is also wrong.

**Trade-offs:** Correct for single-press events. Does not queue multiple presses between audio blocks — second press before audio block would be lost. For this plugin's use case (human button presses at ~60Hz, audio blocks at ~175Hz) the timing margin makes this safe in practice.

**Example from codebase:**
```cpp
// GamepadInput.cpp (timer, 60Hz):
if (curSS && !prevStartStop_) looperStartStop_.store(true);

// PluginProcessor.cpp (processBlock, audio thread):
if (gamepad_.consumeLooperStartStop()) looper_.startStop();

// GamepadInput.cpp (consume):
bool GamepadInput::consumeLooperStartStop() {
    return looperStartStop_.exchange(false);
}
```

### Pattern 3: APVTS getRawParameterValue for Audio-Thread Parameter Access

**What:** `apvts.getRawParameterValue(paramID)` returns a `std::atomic<float>*`. This pointer is stable for the plugin's lifetime. Calling `.load()` on it in processBlock is the correct, lock-free way to read parameter values on the audio thread.

**When to use:** All parameter reads inside processBlock. Never call `apvts.getParameter()` or access `ValueTree` objects on the audio thread.

**Trade-offs:** Safe and zero-cost. The returned pointer must be cached or looked up by string ID — string lookup has overhead if done per-block (acceptable for 40 parameters called once per block).

**Current implementation concern:** `buildChordParams()` calls `apvts.getRawParameterValue(ParamID::...)` 20+ times per block using string IDs. These string comparisons are technically allocation-free (JUCE uses identifier maps internally) but are not zero-cost. Optimization: cache the raw pointers as member variables in prepareToPlay(). Not a correctness issue, minor performance improvement.

**Example from codebase:**
```cpp
// PluginProcessor.cpp — correct pattern:
p.xAtten = apvts.getRawParameterValue(ParamID::joystickXAtten)->load();
p.globalTranspose = (int)apvts.getRawParameterValue(ParamID::globalTranspose)->load();
```

### Pattern 4: MidiBuffer::addEvent with Sample Offset

**What:** MIDI events in processBlock are added to the `juce::MidiBuffer` with a sample offset (0 to blockSize-1) indicating when within the block the event fires. The DAW uses this for sample-accurate MIDI timing.

**When to use:** All MIDI note-on, note-off, and CC events. Always specify the correct sample offset. Offset 0 means "start of block."

**Trade-offs:** Sample-accurate timing for touchplate and random triggers requires computing the exact sample offset of each subdivision crossing. The current implementation uses offset=0 for all events (fires at block start). This is a known simplification — see architectural risks.

**Example from codebase:**
```cpp
// PluginProcessor.cpp:
midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), sampleOff);
midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, pitch), sampleOff);
midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCut), 0);
```

### Pattern 5: AudioPlayHead for DAW Sync

**What:** Call `getPlayHead()->getCurrentPosition(pos)` in processBlock to get the DAW's current `ppqPosition` (quarter-note position from song start), `bpm`, `isPlaying`, and `timeSigNumerator`. This drives the LooperEngine's beat clock.

**When to use:** Any time the plugin must sync to DAW transport. Always check the return value — playhead can be null (standalone, no DAW).

**Trade-offs:** ppqPosition advances linearly while playing. For looper sync, `fmod(ppqPosition, loopLengthBeats)` gives position within the current loop. The plugin correctly falls back to an internal beat clock when `isDawPlaying` is false.

**Example from codebase:**
```cpp
// PluginProcessor.cpp:
juce::AudioPlayHead::CurrentPositionInfo pos {};
const bool hasDaw = getPlayHead() && getPlayHead()->getCurrentPosition(pos);

lp.bpm = (hasDaw && pos.bpm > 0.0) ? pos.bpm : 120.0;
lp.ppqPosition = (hasDaw && pos.isPlaying) ? pos.ppqPosition : -1.0;
lp.isDawPlaying = hasDaw && pos.isPlaying;
```

### Pattern 6: juce::Timer for 60Hz Gamepad Poll

**What:** `GamepadInput` inherits `juce::Timer` privately and calls `startTimerHz(60)`. The `timerCallback()` runs on the message thread, not a dedicated background thread. SDL2's `SDL_PollEvent()` is called here to drain the SDL event queue.

**When to use:** Polling at rates up to ~100Hz where message-thread latency is acceptable. For true real-time input, a background thread would be needed.

**Trade-offs:**
- Correct choice for this plugin. 60Hz gamepad poll latency is ~16ms, which is imperceptible for chord triggering.
- Message thread can be delayed by heavy UI painting. If the LooperEngine UI renders complex waveforms, timer jitter could cause occasional 30-50ms latency on button press detection.
- A dedicated background thread (inheriting `juce::Thread`) would give more consistent timing but adds synchronization complexity. Not needed here.
- SDL must be initialized without `SDL_INIT_VIDEO` to avoid conflicting with JUCE's window management. The codebase correctly uses `SDL_INIT_GAMECONTROLLER` only.

### Pattern 7: APVTS Serialization via XML

**What:** `getStateInformation()` copies the APVTS ValueTree to XML and serializes to binary. `setStateInformation()` deserializes and calls `apvts.replaceState()`. This is the standard JUCE pattern for DAW preset save/recall.

**When to use:** Always for JUCE plugins with APVTS. Do not manually serialize parameters.

**Trade-offs:** Works correctly for all APVTS-managed parameters (40+ in this plugin). Does NOT persist non-APVTS state: the LooperEngine's recorded event buffer is not serialized and will be lost on DAW project reload. This is likely intentional (loops are transient performance data) but should be documented.

**Example from codebase:**
```cpp
void PluginProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
```

---

## MIDI-Only Plugin Configuration

The plugin declares itself correctly as a MIDI generator:

```cpp
bool acceptsMidi()  const override { return false; }  // does not process incoming MIDI
bool producesMidi() const override { return true; }   // generates MIDI
bool isMidiEffect() const override { return false; }  // NOT a MIDI effect (no audio passthrough)

bool isBusesLayoutSupported(const BusesLayout& layouts) const override
{
    if (layouts.getMainInputChannelCount()  != 0) return false;
    if (layouts.getMainOutputChannelCount() != 0) return false;
    return true;
}
```

**Important nuance:** `isMidiEffect() = false` is intentional here. A true MIDI effect (returns true) causes some DAWs (Ableton) to route it differently — as an inline MIDI transformer. This plugin is a generator inserted on an instrument track, so `false` is correct. However, some DAWs (Logic, Ableton on instrument tracks) may route MIDI output differently. This warrants DAW-specific testing.

**AudioProcessor constructor:** `AudioProcessor(BusesProperties())` with empty BusesProperties is correct for no-audio plugins. Do not call `BusesProperties().withInput(...).withOutput(...)`.

---

## Data Flow

### Primary Flow: Joystick → MIDI Note

```
[Mouse drag / Gamepad right stick]
         |
         | (atomic write: joystickX/Y, pitchX/Y)
         v
[processBlock begins]
         |
         | (LooperEngine::process → may override joystick from playback)
         | (buildChordParams → reads atomics + APVTS)
         v
[ChordEngine::computePitch(voice, params)]  ← pure function, no state
         |
         | (rawPitch = baseNote + joystick*atten + interval + octave*12)
         | (ScaleQuantizer::quantize → nearest scale note)
         v
[heldPitch_[voice] = computed MIDI pitch]
         |
         | (TriggerSystem::processBlock → on note-on event:)
         v
[MidiBuffer::addEvent(noteOn, pitch, channel, sampleOffset)]
         |
         v
[DAW receives MIDI → synth plays chord]
```

### Secondary Flow: Looper Record → Playback

```
[User presses Record (UI or gamepad)]
         | (atomic: recording_.store(true))
         v
[processBlock: looper_.recordJoystick() + looper_.recordGate()]
         | (mutex lock → push_back to events_ vector)
         v
[User presses Stop]
         | (atomic: playing_.store(false))
         v
[User presses Play again]
         | (atomic: playing_.store(true))
         v
[processBlock: looper_.process() → scans events_ in beat window]
         | (mutex lock for read scan — critical path concern)
         | (returns BlockOutput: joystickX/Y overrides + gateOn/gateOff)
         v
[processBlock: applies overrides → TriggerSystem fires MIDI]
```

### Filter CC Flow (Gamepad Only)

```
[Gamepad left stick X/Y]
         | (SDL2 poll → atomic write filterX_/filterY_)
         v
[processBlock: gamepad_.getFilterX/Y() → scale by APVTS attenuators]
         v
[MidiBuffer::addEvent(CC74 cutoff + CC71 resonance, offset=0)]
         v
[DAW → synth filter responds]
```

---

## Architectural Risks

### Risk 1: Mutex in Audio Thread (CRITICAL)

**Location:** `LooperEngine::process()` (line 143), `recordJoystick()` (line 87), `recordGate()` (line 101)

**What happens:** `std::mutex` is locked inside `processBlock()`. This is a real-time violation. If the UI thread or any other thread holds the mutex when the audio thread calls `lock()`, the audio thread blocks. This causes audio glitches (buffer underruns in the DAW).

**Current exposure:** The UI calls `reset()` and `deleteLoop()` (which also lock the mutex) from button callbacks on the message thread. If a button press coincides with a processBlock call, the audio thread spins waiting for the mutex.

**Why it hasn't manifested:** At 256-sample block size with 44.1kHz, processBlock runs every ~5.8ms. Message-thread mutex hold time for `events_.clear()` is microseconds. The race window is narrow, making this rarely audible but not correct.

**Recommended fix:** Replace `std::mutex + std::vector` with a lock-free single-producer single-consumer queue for recording, and an atomic swap for playback event buffer. A double-buffer approach (record into back-buffer, swap atomically on loop wrap) is the standard pattern.

**Phase implication:** This is a phase 2 fix. The initial "get it to compile and play notes" phase can ship with the mutex. The looper stabilization phase must address this before any public release.

### Risk 2: isMidiEffect() DAW Compatibility

**Location:** `PluginProcessor.h` (line 28)

**What happens:** `isMidiEffect() = false` with `producesMidi() = true` is unusual. Most DAWs expect generators to be on instrument tracks. Ableton Live requires the plugin to be on a MIDI track with MIDI output routing enabled. Reaper handles this more flexibly.

**Recommended action:** Test in both Ableton and Reaper as the first DAW integration step. If Ableton doesn't pass the generated MIDI downstream, the output routing or isMidiEffect() may need adjustment.

### Risk 3: APVTS Parameter ID String Lookups in processBlock

**Location:** `PluginProcessor.cpp::buildChordParams()` and trigger/looper param reads — called every processBlock

**What happens:** `apvts.getRawParameterValue(ParamID::someString)` performs a hash map lookup by string ID every call. With 20+ calls per block this is not free, though JUCE's implementation uses `HashMap` which is O(1). It is not allocation-heavy but does do string comparisons.

**Recommended fix:** Cache the `std::atomic<float>*` pointers in `prepareToPlay()`. Pattern:
```cpp
// In PluginProcessor.h:
std::atomic<float>* pGlobalTranspose_ = nullptr;

// In prepareToPlay():
pGlobalTranspose_ = apvts.getRawParameterValue(ParamID::globalTranspose);

// In buildChordParams():
p.globalTranspose = (int)pGlobalTranspose_->load();
```
This is a performance optimization, not a correctness issue. The current code is safe.

### Risk 4: GamepadInput Consumed in processBlock, Not in Timer Callback

**Location:** `PluginProcessor.cpp` (lines 217-227) — gamepad edge flags consumed at start of processBlock

**What happens:** The consume pattern (exchange false) happens on the audio thread. The gamepad timer sets flags on the message thread. This is a correct use of atomics. However, looper transport commands (`consumeLooperStartStop()` etc.) are consumed on the audio thread and then call `looper_.startStop()` — which writes to `playing_` atomic. This is correct because startStop() only touches atomics.

**No fix needed.** Pattern is sound.

### Risk 5: LooperEngine eventsMutex_ Locked on const_cast in hasContent()

**Location:** `LooperEngine.cpp` (line 33)

**What happens:** `hasContent()` is `const` but must lock the mutex. The implementation uses `const_cast<std::mutex&>(eventsMutex_)`. This is technically correct but indicates the mutex should have been `mutable`. Minor code smell, not a correctness issue.

**Recommended fix:** Declare `mutable std::mutex eventsMutex_` in LooperEngine.h. This is a one-word change.

### Risk 6: Filter CC Sent Every processBlock Regardless of Gamepad State

**Location:** `PluginProcessor.cpp` (lines 331-344)

**What happens:** CC74 and CC71 messages are added to the MIDI buffer every single processBlock, even when no gamepad is connected (values will be 0). This floods the downstream synth with CC messages at the audio block rate (~175 times/second at 44.1kHz/256).

**Recommended fix:** Only send CC events when the gamepad is connected (`gamepad_.isConnected()`) and only when the filter axis values have changed beyond a threshold (delta > epsilon). Or gate on `gamepad_.isConnected()` as a minimum.

---

## Build / Integration Order

The component dependency graph determines the correct build order:

```
ScaleQuantizer (no deps)
    └── ChordEngine (depends on ScaleQuantizer)
            └── TriggerSystem (no external deps, uses std::atomic)
            └── LooperEngine (no external deps, uses std::atomic + mutex)
            └── GamepadInput (depends on SDL2, juce::Timer)
                    └── PluginProcessor (depends on all above + APVTS)
                            └── PluginEditor (depends on PluginProcessor)
```

**Recommended phase order for testing:**

1. **ScaleQuantizer + ChordEngine** — pure functions, testable without JUCE host. Write unit tests first.
2. **PluginProcessor compiles + loads** — verify CMakeLists, VST3 target, no audio bus crash.
3. **APVTS parameter round-trip** — knobs move, state saves/loads in DAW.
4. **TriggerSystem + basic note output** — touchplates fire MIDI notes in Reaper MIDI monitor.
5. **LooperEngine** — record and replay a simple pattern; verify DAW sync.
6. **GamepadInput** — connect controller, verify pitch joystick and trigger buttons.
7. **Filter CC** — fix the "always send" issue, verify CC74/71 control downstream synth.

---

## Anti-Patterns

### Anti-Pattern 1: Calling Non-Atomic API from processBlock

**What people do:** Call `apvts.getParameter("id")->getValue()` or access `apvts.state` (a `ValueTree`) directly from processBlock.

**Why it's wrong:** `ValueTree` is not thread-safe. It uses internal ref-counting with mutexes. Accessing it from the audio thread can deadlock or corrupt state.

**Do this instead:** Always use `apvts.getRawParameterValue(id)->load()` from the audio thread. The returned `std::atomic<float>*` is safe.

### Anti-Pattern 2: Locking a Mutex on the Audio Thread

**What people do:** Use `std::mutex` to protect shared data accessed from both processBlock and the UI.

**Why it's wrong:** If the UI holds the mutex, the audio thread blocks. This causes glitches (buffer underrun). The audio thread has a hard real-time deadline — it cannot wait.

**Do this instead:** Use lock-free data structures (atomics for scalars, AbstractFifo + fixed-size buffer for queues, double-buffering for event lists). The LooperEngine's `eventsMutex_` is the current violation that needs resolution.

### Anti-Pattern 3: Allocating Memory in processBlock

**What people do:** `new`, `push_back` on an unfixed-size container, `juce::String` construction from char*, dynamic string operations inside processBlock.

**Why it's wrong:** Memory allocation can block (OS heap lock) and is non-deterministic in time. It causes glitches.

**Do this instead:** Pre-allocate in `prepareToPlay()`. Reserve vector capacity (`events_.reserve(4096)` — the LooperEngine already does this correctly). The `loopOut` struct and `chordP` struct are stack-allocated — correct.

### Anti-Pattern 4: Sending MIDI Events With Wrong Channel Index

**What people do:** Confuse 0-based vs 1-based MIDI channel. `MidiMessage::noteOn(channel, ...)` expects 1-based (1-16).

**Why it's wrong:** Channel 0 is invalid in MIDI. Notes route to wrong channel.

**Current codebase status:** The codebase converts correctly: `const int ch0 = voiceChs[voice] - 1;` then uses `ch0 + 1` in `MidiMessage::noteOn(ch0 + 1, ...)`. The `ch` variable in TriggerSystem.cpp is 0-based and passed to the callback — the callback adds 1. Trace carefully to verify no off-by-one remains when all call sites are audited.

### Anti-Pattern 5: UI Components Reading Processor State Without Atomics

**What people do:** Have the editor directly dereference processor member variables that are written by the audio thread without atomic protection.

**Why it's wrong:** Data races on non-atomic reads are undefined behavior. Can produce torn reads (e.g., reading a float that is mid-write).

**Current status:** The codebase is correctly protected: `isGateOpen()` reads `gateOpen_[voice].load()`, `looperIsPlaying()` reads `playing_.load()`, `getPlaybackBeat()` reads `playbackBeat_.load()`. The PluginEditor Timer reads these via these accessor methods. Correct.

---

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| SDL2 (gamepad) | Static library, `SDL_Init(SDL_INIT_GAMECONTROLLER)` only, `SDL_PollEvent()` in Timer | No video subsystem — JUCE owns window. SDL_Quit() in destructor. |
| DAW host (VST3) | JUCE AudioProcessor virtual methods | getPlayHead(), processBlock MidiBuffer, getStateInformation/setStateInformation |
| JUCE APVTS | Parameter layout created once in constructor, attachments in PluginEditor | getRawParameterValue() for audio-thread reads |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| UI → TriggerSystem | `std::atomic<bool>` padPressed_ + padJustFired_ | Written by UI, read by audio thread. Safe. |
| UI → joystickX/Y | `std::atomic<float>` in PluginProcessor | Written by JoystickPad, read by buildChordParams(). Safe. |
| GamepadInput → processBlock | `atomic::exchange(false)` consume pattern | Flags set at 60Hz, consumed at audio block rate. Safe. |
| processBlock → LooperEngine event recording | `std::mutex` lock | RISK: blocks audio thread if UI contends. See Risk 1. |
| LooperEngine → processBlock event playback | `std::mutex` read scan | Same mutex contention risk. |
| APVTS → processBlock | `getRawParameterValue()->load()` | Correct JUCE pattern. Safe. |
| processBlock → UI (gate LEDs) | `std::atomic<bool>` gateOpen_ per voice | Read by PluginEditor Timer. Safe. |
| processBlock → UI (looper beat display) | `std::atomic<double>` playbackBeat_ | Read by PluginEditor Timer. Safe. |

---

## Scalability Considerations

This is an embedded VST3 plugin, not a server. "Scalability" in this domain means:

| Concern | Current State | Risk if Ignored |
|---------|---------------|-----------------|
| Parameter count (40+) | APVTS handles any count; string lookups per block are O(1) but not free | Minor CPU overhead; fix by caching raw pointers |
| Event vector growth during recording | Pre-reserved to 4096 events; `push_back` reallocates if exceeded | Reallocation in processBlock if recording very long loops. Pre-reserve larger or use circular buffer. |
| Multiple processBlock calls at small block sizes | All per-block work is O(n events + 4 voices) = constant-ish | No issue at typical block sizes (64-2048) |
| SDL2 + JUCE Timer on message thread | Timer at 60Hz; SDL_PollEvent drains queue each callback | Message thread saturation if UI is also rendering heavily. Low risk at 60Hz. |
| Multiple DAW instances | SDL_Init/SDL_Quit per instance; SDL has internal reference counting | In theory safe; in practice multiple plugin instances sharing SDL may conflict. Test with duplicate instances in DAW. |

---

## Sources

- Direct analysis of `Source/PluginProcessor.h/.cpp`, `TriggerSystem.h/.cpp`, `LooperEngine.h/.cpp`, `GamepadInput.h/.cpp`, `ChordEngine.h`, `ScaleQuantizer.h`, `PluginEditor.h` (this session)
- JUCE API knowledge: `AudioProcessorValueTreeState::getRawParameterValue()` returns `std::atomic<float>*` — this is the documented audio-thread-safe access pattern (HIGH confidence, verified from codebase usage)
- `juce::Timer` runs on message thread — documented JUCE behavior, consistent with `GamepadInput`'s design (HIGH confidence)
- MIDI channel conventions (1-based in `MidiMessage` API) — verified from code (HIGH confidence)
- `isMidiEffect()` vs `producesMidi()` — behavior from JUCE plugin format wrappers (MEDIUM confidence — DAW-specific testing required)
- Real-time audio threading rules (no blocking, no allocation in processBlock) — established domain knowledge, verifiable against JUCE documentation (HIGH confidence)

---

*Architecture research for: ChordJoystick VST3 MIDI Generator Plugin*
*Researched: 2026-02-22*
