# Stack Research — v1.1 Feature APIs

**Domain:** JUCE VST3 MIDI Generator Plugin — v1.1 additions only
**Researched:** 2026-02-24
**Confidence:** HIGH (all JUCE APIs verified directly from JUCE 8.0.4 source in `build/_deps/juce-src`; all SDL2 APIs verified directly from SDL2 2.30.9 headers in `build/_deps/sdl2-src/include`)

> **Scope:** This file covers ONLY the four new-feature API questions for v1.1.
> The full existing stack (JUCE 8.0.4, SDL2 2.30.9 static, CMake, C++17, MSVC, Catch2,
> Inno Setup) is validated, locked, and documented in the v1.0 STACK.md.
> No new external libraries are needed for any v1.1 feature.

---

## Decision: No New Dependencies

All four v1.1 features can be implemented entirely with APIs already present in the locked
stack. Rationale per feature:

| Feature | Why No New Library Needed |
|---------|--------------------------|
| MIDI panic sweep | `juce::MidiBuffer::addEvent` + 3 JUCE static factory methods already in `juce_audio_basics` |
| Trigger quantization | Integer rounding on `beatPosition` field of existing `LooperEvent` struct |
| Looper position bar | `juce::Component::paint()` + `juce::Graphics::fillRect()` + existing 30 Hz `juce::Timer` |
| Controller name/type | `SDL_GameControllerName()` + `SDL_GameControllerGetType()` in SDL2 2.30.9 — already linked |

---

## Feature 1: MIDI Panic — Full CC Reset Sweep

### Confirmed JUCE 8 Static Factory Methods

Verified from `build/_deps/juce-src/modules/juce_audio_basics/midi/juce_MidiMessage.cpp`:

| Method | CC Number | MIDI Meaning |
|--------|-----------|--------------|
| `MidiMessage::allSoundOff(ch)` | CC 120 = 0 | All Sound Off — cuts active notes immediately |
| `MidiMessage::allControllersOff(ch)` | CC 121 = 0 | Reset All Controllers — resets pitch bend, mod wheel, etc. |
| `MidiMessage::allNotesOff(ch)` | CC 123 = 0 | All Notes Off — note-off for all held notes (allows release) |
| `MidiMessage::controllerEvent(ch, 64, 0)` | CC 64 = 0 | Sustain Pedal off — explicit; allControllersOff does NOT guarantee CC64=0 on all synths |

All four methods return `juce::MidiMessage` objects and are `noexcept`.

### Correct Sweep Order

Send in this order per channel to achieve maximum compatibility:

```
1. allSoundOff(ch)         — CC 120: kills sustaining audio immediately
2. allControllersOff(ch)   — CC 121: resets mod wheel, pitch bend register
3. controllerEvent(ch,64,0)— CC  64: explicit sustain off (some synths ignore CC121 for sustain)
4. allNotesOff(ch)         — CC 123: note-off for all voices
```

### Integration with Existing pendingPanic_ Pattern

The current `pendingPanic_` atomic flag fires in `processBlock`, sending `allNotesOff` on the
4 configured voice channels only. The v1.1 upgrade sends the full sweep on **all 16 MIDI
channels**, not just voice channels. This ensures synths hold notes on channels other than
1–4 are silenced.

```cpp
// In processBlock, replacing the current pendingPanic_ handler:
if (pendingPanic_.exchange(false, std::memory_order_acq_rel))
{
    if (looper_.isPlaying()) looper_.startStop();

    for (int ch = 1; ch <= 16; ++ch)
    {
        midi.addEvent(juce::MidiMessage::allSoundOff(ch),        0);
        midi.addEvent(juce::MidiMessage::allControllersOff(ch),  0);
        midi.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0);
        midi.addEvent(juce::MidiMessage::allNotesOff(ch),        0);
    }

    trigger_.resetAllGates();
    flashPanic_.fetch_add(1, std::memory_order_relaxed);
}
```

**Thread safety:** `midi.addEvent` is audio-thread-only and called only within `processBlock`.
`pendingPanic_` is `std::atomic<bool>` — already used this way. No changes to threading model.

**Buffer capacity:** 64 messages (4 msgs × 16 channels) added in a single block. `juce::MidiBuffer`
is a heap-allocated growable container; this is not a stack-size concern.

**Performance:** 64 `addEvent` calls is negligible — each is a small memcpy. Panic fires at most
once per button press, not every block.

### What NOT to Do

- Do NOT call `MidiMessage::isResetAllControllers()` — that is a query, not a factory.
- Do NOT assume `allControllersOff` resets CC64 sustain — some hardware synths (Roland,
  Korg) require an explicit `controllerEvent(ch, 64, 0)`. Send it explicitly.
- Do NOT send panic from the message thread — only `pendingPanic_.store(true)` from message
  thread; let `processBlock` execute the actual `midi.addEvent` calls.
- Do NOT use a loop over `voiceChs[]` only — that sends panic to 4 channels. Full panic means
  all 16 channels so externally held notes from other sources are also cleared.

---

## Feature 2: Trigger Quantization (Live and Post-Record)

### What Needs to Happen

**Live quantization:** When a gate event is recorded, snap its `beatPosition` to the nearest
grid subdivision boundary before storing it in the FIFO.

**Post-record quantization:** Iterate `playbackStore_[0..playbackCount_-1]`, snapping each
Gate-type event's `beatPosition` to the grid. Called from the UI thread after recording stops.

### Quantization Math

The looper timeline unit is already beats (quarter-note = 1.0). Subdivisions map to beat
fractions:

| Subdivision | Beats per grid step |
|-------------|---------------------|
| 1/4         | 1.0 |
| 1/8         | 0.5 |
| 1/16        | 0.25 |
| 1/32        | 0.125 |

Snap formula (no new API needed — pure arithmetic):

```cpp
double snapBeatToGrid(double beatPos, double gridStep, double loopLen)
{
    const double snapped = std::round(beatPos / gridStep) * gridStep;
    return std::fmod(snapped, loopLen);   // wrap at loop boundary
}
```

`std::round` is in `<cmath>`, already included in `LooperEngine.cpp`. `std::fmod` is also
already used in `LooperEngine::recordGate`.

### Live Quantization Integration Point

In `LooperEngine::recordGate()`, after computing `pos = std::fmod(beatPos, loopLen)`, apply:

```cpp
if (liveQuantizeEnabled_.load())
    pos = snapBeatToGrid(pos, gridStepBeats_.load(), loopLen);
```

Two new `std::atomic` members are needed:
- `std::atomic<bool> liveQuantize_ { false }`
- `std::atomic<float> quantizeGrid_ { 0.25f }` — grid step in beats (0.25 = 1/16)

These follow the exact same pattern as existing `recGates_`, `recJoy_`, etc. atomics.

### Post-Record Quantization Integration Point

A new `quantizeLoop(float gridStepBeats)` public method on `LooperEngine`, called from the
UI thread (message thread). Threading invariant: this method MUST only be called when
`isPlaying() == false && isRecording() == false` — the same invariant as `finaliseRecording()`.

```cpp
void LooperEngine::quantizeLoop(float gridStepBeats)
{
    // THREADING INVARIANT: call only when playing_=false AND recording_=false
    const double loopLen = getLoopLengthBeats();
    const int count = playbackCount_.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i)
    {
        auto& ev = playbackStore_[i];
        if (ev.type == LooperEvent::Type::Gate)
            ev.beatPosition = snapBeatToGrid(ev.beatPosition, (double)gridStepBeats, loopLen);
    }
}
```

`playbackStore_` is a `std::array` class member — no allocation. Direct array write is safe
because the threading invariant guarantees no concurrent reader (audio thread early-returns
when `playing_=false`).

### What NOT to Do

- Do NOT quantize JoystickX/JoystickY/FilterX/FilterY events — only Gate events should snap.
  Joystick position events are continuous and quantizing them would create audible jumps.
- Do NOT call `quantizeLoop` while the looper is playing — this writes to `playbackStore_`
  while the audio thread reads it. Enforce the stop-first invariant in the UI button handler.
- Do NOT add a mutex — the existing lock-free invariant (stop first) is sufficient and keeps
  the audio thread free of blocking calls.

---

## Feature 3: Looper Playback Position Bar (Animated)

### Existing Infrastructure

The looper already exposes what the progress bar needs:

| API | Location | What It Returns |
|-----|----------|-----------------|
| `looper_.getPlaybackBeat()` | `LooperEngine::getPlaybackBeat()` | `double` — current beat offset within loop (0..loopLengthBeats) via `std::atomic<float>` |
| `looper_.getLoopLengthBeats()` | `LooperEngine::getLoopLengthBeats()` | `double` — total loop length in beats |
| `looper_.isPlaying()` | `LooperEngine::isPlaying()` | `bool` — whether looper is running |

These are already accessible through `PluginProcessor`:
- `proc_.looperIsPlaying()` — already called in `timerCallback()`
- The looper is accessible via `proc_` — add accessor methods for beat/length if needed.

### Recommended Implementation Pattern

Create a simple `LooperPositionBar` custom `juce::Component` that:
1. Receives a normalized position `[0..1]` each timer tick
2. Draws a filled rectangle scaled to that fraction of its width
3. Is repainted at the existing 30 Hz rate in `timerCallback()`

```cpp
class LooperPositionBar : public juce::Component
{
public:
    void setPosition(float normalizedPos)  // 0..1
    {
        if (pos_ != normalizedPos) { pos_ = normalizedPos; repaint(); }
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        // Background
        g.setColour(juce::Colour(0xFF252843));   // matches Clr::gateOff
        g.fillRect(b);
        // Progress fill
        if (pos_ > 0.0f)
        {
            g.setColour(juce::Colour(0xFF1E3A6E));  // matches Clr::accent
            g.fillRect(b.withWidth(b.getWidth() * pos_));
        }
        // Playhead cursor line
        g.setColour(juce::Colour(0xFFFF3D6E));   // matches Clr::highlight
        const float x = b.getWidth() * pos_;
        g.drawLine(x, b.getY(), x, b.getBottom(), 2.0f);
    }

private:
    float pos_ = 0.0f;
};
```

In `PluginEditor::timerCallback()`, add after the existing looper button state updates:

```cpp
{
    const double beat = proc_.looper_.getPlaybackBeat();    // or via accessor
    const double len  = proc_.looper_.getLoopLengthBeats();
    const float  norm = (len > 0.0) ? (float)(beat / len) : 0.0f;
    looperPositionBar_.setPosition(proc_.looperIsPlaying() ? norm : 0.0f);
}
```

**No additional timer needed.** The existing `startTimerHz(30)` in `PluginEditor` is
sufficient — 30 Hz gives ~33 ms updates, imperceptible as lag for a visual position bar.

### Why NOT Use `juce::ProgressBar`

`juce::ProgressBar` monitors a `double&` value and runs its own internal timer. It was
designed for one-shot loading bars, not real-time looping playback position. Using it
would introduce a second internal timer and provide no useful behavior (no loop wrap,
no playhead cursor). A custom `Component` with `paint()` is 15 lines and does exactly
what's needed.

### Direct Access to LooperEngine from Editor

`LooperEngine` is a `private` member of `PluginProcessor`. The existing pattern is to
expose needed state through forwarding methods on `PluginProcessor` (e.g.,
`looperIsPlaying()`, `looperIsRecording()`). Add:

```cpp
// In PluginProcessor.h:
double looperGetPlaybackBeat()     const { return looper_.getPlaybackBeat(); }
double looperGetLoopLengthBeats()  const { return looper_.getLoopLengthBeats(); }
```

These are read-only queries — no threading concern. `getPlaybackBeat()` reads
`std::atomic<float>` internally.

---

## Feature 4: Gamepad Controller Name / Type Detection

### Confirmed SDL2 2.30.9 API

Verified from `build/_deps/sdl2-src/include/SDL_gamecontroller.h`:

```c
// Returns the human-readable name of the controller.
// Available since SDL 2.0.0. Returns NULL if controller is NULL.
const char* SDL_GameControllerName(SDL_GameController* gamecontroller);

// Returns the type enum of the controller.
// Available since SDL 2.0.12. Returns SDL_CONTROLLER_TYPE_UNKNOWN (0) on failure.
SDL_GameControllerType SDL_GameControllerGetType(SDL_GameController* gamecontroller);
```

### SDL_GameControllerType Enum (verified from SDL2 2.30.9 header)

```c
typedef enum {
    SDL_CONTROLLER_TYPE_UNKNOWN            = 0,
    SDL_CONTROLLER_TYPE_XBOX360            = 1,
    SDL_CONTROLLER_TYPE_XBOXONE            = 2,
    SDL_CONTROLLER_TYPE_PS3                = 3,
    SDL_CONTROLLER_TYPE_PS4                = 4,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO = 5,
    SDL_CONTROLLER_TYPE_VIRTUAL            = 6,
    SDL_CONTROLLER_TYPE_PS5                = 7,
    SDL_CONTROLLER_TYPE_AMAZON_LUNA        = 8,
    SDL_CONTROLLER_TYPE_GOOGLE_STADIA      = 9,
    SDL_CONTROLLER_TYPE_NVIDIA_SHIELD      = 10,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT  = 11,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT = 12,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR  = 13,
    SDL_CONTROLLER_TYPE_MAX                = 14
} SDL_GameControllerType;
```

### Integration Pattern

Both functions are called on the message thread (inside `timerCallback()` in `GamepadInput`,
or in the UI callback). They take `SDL_GameController*` — the same `controller_` pointer
already held by `GamepadInput`.

Add a method to `GamepadInput`:

```cpp
// Returns display string: "DualShock 4", "Xbox One", etc.
// Falls back to raw SDL name if type is UNKNOWN or unrecognized.
juce::String getControllerDisplayName() const
{
    if (!controller_) return "No controller";

    const SDL_GameControllerType type = SDL_GameControllerGetType(controller_);
    switch (type)
    {
        case SDL_CONTROLLER_TYPE_PS4:                return "DualShock 4";
        case SDL_CONTROLLER_TYPE_PS5:                return "DualSense";
        case SDL_CONTROLLER_TYPE_PS3:                return "DualShock 3";
        case SDL_CONTROLLER_TYPE_XBOX360:            return "Xbox 360";
        case SDL_CONTROLLER_TYPE_XBOXONE:            return "Xbox One";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:return "Switch Pro";
        default: break;
    }
    // Fall back to raw SDL name string for unrecognized types
    const char* name = SDL_GameControllerName(controller_);
    return name ? juce::String(name) : "Controller";
}
```

`SDL_GameControllerGetType` and `SDL_GameControllerName` are both called inside
`timerCallback()` which already runs on the JUCE message thread. `controller_` is only
written in `tryOpenController()` and `closeController()`, both also on the message thread.
No threading concern.

**Important:** `SDL_GameControllerName` returns a `const char*` pointing to an internal
SDL buffer. Copy it to a `juce::String` immediately — do not store the raw pointer.

### Display in PluginEditor

The existing gamepad status label pattern is:

```cpp
// In GamepadInput — triggered via onConnectionChangeUI callback:
if (onConnectionChangeUI) onConnectionChangeUI(connected);
```

In `PluginEditor`, the `onConnectionChangeUI` lambda already updates a status label.
Change the label text in that lambda to call `getControllerDisplayName()`:

```cpp
proc_.getGamepad().onConnectionChangeUI = [this](bool connected)
{
    if (connected)
    {
        const juce::String name = proc_.getGamepad().getControllerDisplayName();
        gamepadStatusLabel_.setText("PAD: " + name, juce::dontSendNotification);
    }
    else
    {
        gamepadStatusLabel_.setText("PAD: none", juce::dontSendNotification);
    }
};
```

---

## No New Libraries Required — Verification

| Feature | Checked Against | Result |
|---------|----------------|--------|
| CC sweep (allSoundOff/allControllersOff/allNotesOff) | `juce_MidiMessage.cpp` source | All 3 methods confirmed in JUCE 8.0.4 |
| CC64 explicit sustain off | `juce_MidiMessage.h` — `controllerEvent(int, int, int)` | Confirmed |
| Quantize math | `<cmath>` — `std::round`, `std::fmod` | Already in `LooperEngine.cpp` includes |
| Progress bar paint | `juce::Component::paint()`, `juce::Graphics::fillRect()` | Core JUCE, no extra module |
| SDL controller name | `SDL_gamecontroller.h` — `SDL_GameControllerName` | In SDL 2.0.0+ |
| SDL controller type | `SDL_gamecontroller.h` — `SDL_GameControllerGetType` | In SDL 2.0.12+ (project uses 2.30.9) |

---

## Alternatives Considered

| Recommended | Alternative | Why Not |
|-------------|-------------|---------|
| Sweep all 16 channels | Sweep only voiceChs[4] | Does not silence notes held on other channels from external triggers or previous routing |
| Custom `LooperPositionBar` Component | `juce::ProgressBar` | ProgressBar has no looping behavior, no playhead cursor, runs its own internal timer; custom component is simpler |
| `std::round` for quantize snap | Nearest-grid-down (floor) | Nearest-to-grid (round) matches user expectation: notes slightly after the beat land on-beat; notes far before the beat land on previous beat |
| `SDL_GameControllerGetType` for display name | `SDL_GameControllerName` only | Raw SDL names are vendor-specific strings like "Wireless Controller" (PS4) or "Xbox 360 Controller"; type enum gives clean human-readable names with a fallback |

---

## What NOT to Add

| Do Not Add | Why |
|------------|-----|
| Any quantization library (e.g., third-party beat quantizer) | The math is four lines of `std::round` / `std::fmod` — a library adds zero value |
| `juce::ProgressBar` widget | It runs its own timer, has no playhead cursor concept, and was not designed for looping progress |
| SDL3 upgrade for controller type | SDL3 has a fully different API; `SDL_GameControllerGetType` is in SDL2 2.0.12+, already satisfied by 2.30.9 |
| New APVTS parameters for quantize grid | Quantize grid should be a simple UI state selector (ComboBox mapping to a float step), not a DAW-automatable parameter — it is a one-shot action, not a continuous value |
| A new thread for quantize processing | Post-record quantization is a fast linear scan (~2048 events max, each a double write) — runs in microseconds on the message thread when looper is stopped |

---

## Sources

### PRIMARY — HIGH Confidence (verified from source in repo)

- `build/_deps/juce-src/modules/juce_audio_basics/midi/juce_MidiMessage.cpp` lines 643–674 — exact CC numbers for `allNotesOff` (123), `allSoundOff` (120), `allControllersOff` (121) confirmed
- `build/_deps/juce-src/modules/juce_audio_basics/midi/juce_MidiMessage.h` lines 535–546 — method signatures confirmed
- `build/_deps/sdl2-src/include/SDL_gamecontroller.h` lines 63–441 — `SDL_GameControllerType` enum and `SDL_GameControllerName`/`SDL_GameControllerGetType` signatures confirmed from SDL 2.30.9
- `Source/LooperEngine.h` — `getPlaybackBeat()`, `getLoopLengthBeats()` accessor signatures confirmed
- `Source/PluginEditor.cpp` line 1100 — `startTimerHz(30)` existing refresh rate confirmed
- `Source/PluginProcessor.cpp` lines 471–477 — existing `pendingPanic_` pattern confirmed

### SECONDARY — MEDIUM Confidence

- SDL2 Wiki `SDL_GameControllerGetType` — availability since SDL 2.0.12 confirmed via web search
- SDL2 Wiki `SDL_GameControllerName` — return type `const char*` and availability since SDL 2.0.0 confirmed via web search

---

*Stack research for: ChordJoystick v1.1 — JUCE VST3 MIDI Generator Plugin*
*Researched: 2026-02-24*
