# Feature Research

**Domain:** MIDI Chord Generator / Performance Controller VST Plugin — v1.1 Additions
**Researched:** 2026-02-24
**Confidence:** HIGH for MIDI spec items (CC numbers are standardized); MEDIUM for UI patterns and quantize algorithms (DAW conventions, verified by multiple sources); HIGH for SDL2 API (verified against SDL2 wiki)

---

> **Scope note:** This document supersedes the v1.0 FEATURES.md for the purpose of v1.1 planning.
> It covers ONLY the four new feature domains requested: MIDI panic, trigger quantization,
> looper position visualization, and gamepad type detection.
> v1.0 table-stakes / differentiator analysis is preserved at the bottom for roadmap continuity.

---

## Feature Domain 1: MIDI Panic Full Reset

### What Professional Implementations Do

**Standard MIDI channel mode messages (MIDI 1.0 spec, HIGH confidence):**

| CC | Name | Value | Effect |
|----|------|-------|--------|
| 120 | All Sound Off | 0 | Immediate silence — cuts active audio regardless of release envelopes or hold pedal. Use for instant silence. |
| 121 | Reset All Controllers | 0 | Resets pitch bend to 0, mod wheel (CC1) to 0, sustain pedal (CC64) to 0, expression (CC11) to 127, and all other continuous controllers to default values. Does NOT stop notes. |
| 123 | All Notes Off | 0 | Transitions all notes to released state. Notes with held sustain (CC64 still on) will continue to ring until sustain is released. Respects release envelopes. |
| 64 | Sustain Pedal | 0 | Explicitly releases hold pedal — must precede or accompany CC123 to guarantee note silence when synth has sustain active. |

**Key distinction (HIGH confidence, MIDI spec):**
- CC 120 (All Sound Off) = immediate cut, ignores release tails and hold pedal. Most aggressive.
- CC 123 (All Notes Off) = polite release, respects release envelopes and sustain. Some synths ignore it while CC64 is on.
- CC 121 (Reset All Controllers) = does not stop notes — only resets controller state.

**Professional panic sequence (MEDIUM confidence — triangulated from Cubase docs, iConnectivity MIDI Panic App, Logic Pro support, multiple forum sources):**

The de-facto industry sequence, sent on EVERY channel (1–16):

```
Step 1: CC 64  = 0    (sustain off — unlock any hold-pedal-held notes)
Step 2: CC 120 = 0    (all sound off — immediate silence)
Step 3: CC 123 = 0    (all notes off — set note state to off)
Step 4: CC 121 = 0    (reset all controllers — zero pitch bend, mod, expression)
Step 5: Pitch Bend = 8192  (center pitch bend to 0 — belt and suspenders)
```

**Why 16 channels, not just 4 voice channels:**
- Stuck notes from previous plugin instances or external gear may be on any channel.
- Real panic must be unconditional and cover the full MIDI channel space.
- Cubase "Reset" and iConnectivity MIDI Panic both default to sending on all 16 channels.

**Current v1.0 implementation (from source):**
- `pendingPanic_` flag is set from UI button or R3 gamepad button.
- On audio thread: sends `allNotesOff()` on the 4 configured voice channels only, resets TriggerSystem gates, stops looper.
- Does NOT send CC 121, CC 64, pitch bend reset, or cover all 16 channels.
- Filter CCs (CC74, CC71) are reset separately via `pendingCcReset_` on disconnect.

**Gap for v1.1:** The current panic is a `juce::MidiMessage::allNotesOff()` on 4 channels (which sends CC123 per JUCE implementation). v1.1 should expand this to a full sweep: CC64=0, CC120=0, CC123=0, CC121=0, pitch bend center — on all 16 channels. Additionally, it should zero out filter CCs (CC74, CC71, CC12, CC1, CC76) on the filter channel, and reset pitch bend on all 4 voice channels.

### Table Stakes vs. Differentiator

| Aspect | Category | Notes |
|--------|----------|-------|
| Sends allNotesOff on voice channels | Table Stakes | Already in v1.0 |
| Sends on ALL 16 channels | Table Stakes | Any professional panic does this |
| CC 120 (All Sound Off) included | Table Stakes | Required for synths with long release tails |
| CC 121 (Reset All Controllers) | Table Stakes | Required to clear stuck pitch bend / mod wheel |
| CC 64 = 0 before allNotesOff | Table Stakes | Required when synth has hold pedal active |
| Pitch bend reset to center | Table Stakes | Required when pitch bend RPN has been used |
| Filter CC reset (CC74, CC71, etc.) on filter channel | Specific to ChordJoystick | Ensures filter returns to neutral on panic |

### Implementation Notes

- JUCE `MidiMessage::allNotesOff(ch)` sends CC 123 with value 0 on channel `ch`. This is correct but insufficient alone.
- JUCE `MidiMessage::controllerEvent(ch, 120, 0)` sends All Sound Off.
- JUCE `MidiMessage::controllerEvent(ch, 121, 0)` sends Reset All Controllers.
- JUCE `MidiMessage::controllerEvent(ch, 64, 0)` sends Sustain Off.
- JUCE `MidiMessage::pitchWheel(ch, 8192)` sends pitch bend center (8192 = 0 in JUCE's 0–16383 range).
- All of these should be sent at sample offset 0 within the panic block.
- Loop over channels 1–16 for the full sweep; also loop over the 4 voice channels for pitch bend reset (since the plugin sets RPN pitch bend range on note-on).

**Complexity: LOW.** It's a larger loop and more CC messages, but no new architecture. The `pendingPanic_` path in `processBlock` is already in place.

---

## Feature Domain 2: Trigger / Gate Quantization

### How It Works in Professional Tools

**Two quantization modes (HIGH confidence — consistent across MPC, Ableton, Cubase docs):**

**Mode A: Live / Input Quantize (applied during recording)**
- Each gate event (note-on and note-off) is snapped to the nearest subdivision grid point as it is recorded.
- The beat timestamp stored in `LooperEvent` is replaced with the rounded value before it enters the buffer.
- MPC calls this "Time Correct" on the input. Ableton calls it "Record Quantization."
- Subdivision options (standard industry practice): 1/4, 1/8, 1/16, 1/32 — same subdivisions already used for random gate.

**Mode B: Post-Record Quantize (applied to existing loop)**
- A QUANTIZE button iterates over all Gate events in `playbackStore_[]` and re-snaps each timestamp to the nearest grid.
- The loop does not need to re-record; it re-times existing stored events.
- After snapping, `playbackStore_` is updated in-place and playback continues from the corrected data.
- Ableton: Cmd+U (quantize) in clip view. Pro Tools: Event > MIDI > Quantize. Both apply to existing MIDI.

### The Quantization Algorithm (MEDIUM confidence — standard DSP/MIDI practice, multiple sources agree)

**Input:** `beatPosition` (a `double` representing beats elapsed from loop start), `subdivBeats` (the grid cell size in beats), `strength` (0.0–1.0; 1.0 = full snap).

**Steps:**

```
1. Compute grid index: gridIndex = round(beatPosition / subdivBeats)
2. Compute snapped position: snappedBeat = gridIndex * subdivBeats
3. Wrap to loop: snappedBeat = fmod(snappedBeat, loopLengthBeats)
4. Apply strength (optional):
   quantizedBeat = beatPosition + (snappedBeat - beatPosition) * strength
5. Clamp to [0, loopLengthBeats)
```

**For ChordJoystick v1.1 (opinionated recommendation):**
- Use strength = 1.0 (full snap). No partial quantize knob — too complex for a small UI, and the user already controls groove feel via the trigger source (joystick motion vs. pad vs. random).
- Use `round()` not `floor()` — nearest grid, not ahead-of-time snap. This matches MPC "Time Correct" and Ableton record quantize behavior.
- Only quantize Gate events (`LooperEvent::Type::Gate`) — do NOT quantize JoystickX/JoystickY or FilterX/FilterY events. Joystick position is a continuous control; snapping it defeats the expressiveness of the gesture.

**Subdivision storage (grid cell size in beats):**

| Subdivision | Beats (quarter-note = 1.0) |
|-------------|---------------------------|
| 1/4 | 1.0 |
| 1/8 | 0.5 |
| 1/16 | 0.25 |
| 1/32 | 0.125 |

**Quantize subdivision vs. random subdivision:**
These are independent. The quantize subdivision is a separate APVTS parameter (`quantizeSubdiv`). The random subdivision (`randomSubdiv0..3`) controls how often random voices fire — a different concept entirely.

### Live Quantize During Record

**Implementation path:**
- In `processBlock`, when `recordGate()` is called with a `beatPos`, if live quantize is armed, replace `beatPos` with `round(beatPos / subdivBeats) * subdivBeats` before passing it to `LooperEngine::recordGate()`.
- The snap happens in the processor before the event enters the FIFO, so `finaliseRecording()` receives already-quantized timestamps.
- This is cleaner than quantizing inside `LooperEngine` because the quantize parameters live in APVTS (not inside LooperEngine).

**OR** (alternative): Apply quantization inside `LooperEngine::recordGate()` if a `quantizeEnabled_` atomic and `quantizeSubdivBeats_` are added to LooperEngine. This is also valid but couples quantization logic to the engine.

**Recommendation:** Apply in `processBlock` (or in a helper called from there) — keeps LooperEngine focused on timing mechanics, not musical grid decisions.

### Post-Record Quantize

**Implementation path:**
- PluginEditor `[QUANTIZE]` button sets an atomic flag `pendingQuantize_` in PluginProcessor.
- `processBlock` reads the flag; when set, calls `looper_.quantizeGates(subdivBeats)`.
- `LooperEngine::quantizeGates(double subdivBeats)` iterates `playbackStore_[0..playbackCount_-1]`, snaps Gate event `beatPosition` fields in-place, leaves JoystickX/Y and FilterX/Y events unchanged.
- The operation must only run when `recording_ == false` — same invariant as `finaliseRecording()`. If looper is recording, defer until next non-recording block.
- After in-place snap, playback immediately reflects corrected timing on the next loop cycle.

**Complexity: MEDIUM.** The algorithm is simple (one multiply + round per event). The tricky part is thread safety: the audio thread owns `playbackStore_[]` exclusively during playback; the quantize operation must be atomic with respect to the playback read pointer. Because LooperEngine reads events sequentially during playback (no random access), updating timestamps in-place is safe as long as the quantize pass completes before the next `process()` call on the same thread. A simpler approach: copy to scratch, quantize scratch, then swap via `playbackCount_` atomic — same pattern as `finaliseRecording()`.

### Table Stakes vs. Differentiator

| Feature | Category | Notes |
|---------|----------|-------|
| Post-record QUANTIZE button | Table Stakes (for looper products) | MPC, Ableton, Boss RC all have this |
| Live record quantize | Table Stakes | MPC "Time Correct on Input," Ableton record quantize |
| Independent quantize subdivision | Table Stakes | Must be separate from random gate subdivision |
| Quantize only Gate events (not joystick gestures) | ChordJoystick-specific | Gesture continuity is a core value; joystick snapping destroys it |
| Quantize strength knob (partial snap) | Anti-Feature for v1.1 | Adds UI complexity without proportional value; defer to v2 |

---

## Feature Domain 3: Looper Playback Position Visualization

### What Works in Small Plugin UIs

**Industry patterns (MEDIUM confidence — from SooperLooper, hardware loopers, JUCE forum discussions):**

**Pattern 1: Horizontal progress bar (most common)**
- A thin horizontal bar fills left-to-right as the loop plays, resets to 0 at loop boundary.
- Width represents total loop length; fill position = `getPlaybackBeat() / getLoopLengthBeats()`.
- Used by: Boss RC series (LED strip), Ableton Looper device (horizontal bar), SooperLooper (position indicator added in v1.7.9).
- Advantage: immediately readable, zero ambiguity, works at any pixel width (even 8px tall).
- In ChordJoystick: place below the looper transport buttons row. A 6–8px tall bar spanning the looper section width is sufficient.

**Pattern 2: Rotating indicator / clock hand**
- A circular sweep shows position around a clock face.
- Used by: Elektron hardware (Analog Four/Octatrack step indicators), some hardware sequencers.
- Disadvantage: harder to read the absolute position at a glance; requires more horizontal real estate for a circle. Not recommended for a small secondary indicator.

**Pattern 3: Beat tick marks with playhead**
- Vertical tick marks at each beat (or subdivision); a moving cursor sweeps across.
- Used by: DAW timeline view, Roland SP-404.
- Disadvantage: requires enough pixel width to space ticks; at the plugin's current width (likely 600–800px), only 2-bar loops at 4/4 would have adequate tick spacing. For 16-bar loops the ticks compress to noise.

**Recommendation: Horizontal progress bar (Pattern 1).**
- A `juce::Component` with a custom `paint()` that fills a rectangle proportionally.
- Height: 6px. Color: cyan (matching plugin aesthetic).
- The timer callback in PluginEditor already reads `proc_.looper_.getPlaybackBeat()` — add position bar repaint there.
- When loop is stopped: bar is empty (0 fill).
- When loop is recording: bar shows recording progress in red/amber (alternate color), same fill logic.

**API already available:**
- `LooperEngine::getPlaybackBeat()` returns `double` — atomic float cast to double; updates each processBlock.
- `LooperEngine::getLoopLengthBeats()` returns loop length in beats.
- Ratio = `getPlaybackBeat() / getLoopLengthBeats()`, clamped to [0.0, 1.0].

**Update rate:** PluginEditor timer currently fires at some rate (typically 30Hz in JUCE plugins using `startTimerHz(30)`). At 120 BPM, a 4/4 bar is 2 seconds; 30Hz gives ~60 position updates per bar — visually smooth.

**Recording state color coding (MEDIUM confidence — consistent with hardware looper conventions):**
- Stopped/idle: bar hidden or dim outline only
- Playing back: cyan fill
- Recording: red/amber fill (recording progress fills the bar as beats are recorded)
- Record-armed (waiting for trigger): blinking/pulsing outline

**Complexity: LOW.** One custom `juce::Component` with ~20 lines of `paint()` code. No new audio thread logic needed.

### Anti-Pattern to Avoid

**Beat number text display alone ("Bar 2 / Beat 3")** — Cognitive load is high when performing; a visual position bar is scannable peripherally. Do both if space permits, but the bar is primary.

---

## Feature Domain 4: Gamepad Controller Type Detection

### SDL2 API (HIGH confidence — verified against SDL2 Wiki and SDL_gamecontroller.h)

**Primary function: `SDL_GameControllerGetType(controller)`**
- Available since SDL 2.0.12 (ChordJoystick uses SDL2 2.30.9 — well above this floor).
- Returns `SDL_GameControllerType` enum.

**Full enum (HIGH confidence — verified against SDL2 source):**

| Value | Enum Constant | Display String |
|-------|---------------|----------------|
| 0 | SDL_CONTROLLER_TYPE_UNKNOWN | "Controller" |
| 1 | SDL_CONTROLLER_TYPE_XBOX360 | "Xbox 360" |
| 2 | SDL_CONTROLLER_TYPE_XBOXONE | "Xbox One" |
| 3 | SDL_CONTROLLER_TYPE_PS3 | "PS3" |
| 4 | SDL_CONTROLLER_TYPE_PS4 | "PS4" |
| 5 | SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO | "Switch Pro" |
| 6 | SDL_CONTROLLER_TYPE_VIRTUAL | "Virtual" |
| 7 | SDL_CONTROLLER_TYPE_PS5 | "PS5" |
| 8 | SDL_CONTROLLER_TYPE_AMAZON_LUNA | "Luna" |
| 9 | SDL_CONTROLLER_TYPE_GOOGLE_STADIA | "Stadia" |
| 10 | SDL_CONTROLLER_TYPE_NVIDIA_SHIELD | "Shield" |
| 11 | SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT | "Joy-Con L" |
| 12 | SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT | "Joy-Con R" |
| 13 | SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR | "Joy-Con Pair" |

**Secondary function: `SDL_GameControllerName(controller)`**
- Returns implementation-dependent name string (e.g., "PS4 Controller", "Xbox One Controller", "DualSense Wireless Controller").
- Available since SDL 2.0.0.
- Useful as fallback display when type is UNKNOWN — shows the raw device name instead of "Controller".

**Where to call it:**
- `GamepadInput::timerCallback()` opens the controller via `tryOpenController()`.
- At the point where `SDL_GameControllerOpen()` succeeds, call both `SDL_GameControllerGetType()` and `SDL_GameControllerName()`, convert to display string, store in an `std::atomic<int>` (for the type enum) and a `juce::String` (for the name).
- The `juce::String` write from the SDL timer thread is safe as long as the UI reads it via a snapshot protected by a simple flag or reads during the `onConnectionChangeUI` callback (message thread).

**Recommended approach for v1.1:**
- Store type as `std::atomic<int> controllerType_` (default UNKNOWN).
- Store name as a `juce::String controllerName_` written only when controller connects (inside `onConnectionChangeUI` callback on message thread — safe).
- `gamepadStatusLabel_` in PluginEditor currently shows "GAMEPAD: Connected" / "No Controller" — extend to show type: "PS4 Connected", "Xbox One Connected", "Controller (Generic)", "No Controller".

**Format of the display string:**
```
"PS4 Connected"         // SDL_CONTROLLER_TYPE_PS4
"PS5 Connected"         // SDL_CONTROLLER_TYPE_PS5
"Xbox One Connected"    // SDL_CONTROLLER_TYPE_XBOXONE
"Xbox 360 Connected"    // SDL_CONTROLLER_TYPE_XBOX360
"Switch Pro Connected"  // SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO
"Controller Connected"  // SDL_CONTROLLER_TYPE_UNKNOWN (also append name if non-null)
"No Controller"         // not connected
```

**Complexity: LOW.** One function call at connection time, a switch statement to map enum to string. The label is already in place in `PluginEditor`.

---

## v1.1 Feature Landscape

### Table Stakes (Users Expect These in v1.1)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Full MIDI panic (all 16 ch, CC120+121+123+64+PB) | Panic button that only covers 4 channels is incomplete; any stuck note on ch 5-16 from external gear is not cleared | LOW | Extend existing `pendingPanic_` path in processBlock; add CC loop over 16 channels |
| Live record quantize | Expected by any musician who records loops; MPC users especially expect it | MEDIUM | Add `quantizeEnabled_` atomic + subdivision param; apply snap in processBlock before `recordGate()` |
| Post-record QUANTIZE button | Standard looper feature; hardware loopers (Boss, TC Helicon) all have it | MEDIUM | Add `pendingQuantize_` flag, implement `LooperEngine::quantizeGates()` |
| Quantize subdivision selector | Tied to quantize feature — useless without subdivision control | LOW | New APVTS parameter `quantizeSubdiv` (1/4, 1/8, 1/16, 1/32); ComboBox in UI |
| Looper playback position bar | Without position indicator, users cannot see where in the loop they are | LOW | Custom Component, 6px bar, reads `getPlaybackBeat() / getLoopLengthBeats()` |
| Gamepad type in status label | "Connected" without type is ambiguous when multiple controllers exist | LOW | `SDL_GameControllerGetType()` call at connect time |

### Differentiators (v1.1 Polish)

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Animated mute state visual feedback | MIDI mute is invisible without it — user doesn't know if plugin is muted or broken | LOW | Flash/pulse the MUTE button; already have `flashPanic_` pattern for R3 button |
| Section visual grouping (UI panel borders) | Reduces cognitive load; distinguishes chord section vs. trigger section vs. looper section | LOW | Draw rounded rect borders or background fills in `paint()` |
| Quantize only Gate events (not joystick gestures) | Preserves the expressive joystick gesture while tightening the rhythmic grid; no other looper does this | MEDIUM | Explicit Gate-type check in quantize pass |

### Anti-Features (Do NOT Build in v1.1)

| Feature | Why Not | Alternative |
|---------|---------|-------------|
| Quantize strength / percentage knob | Adds a parameter with no clear default; most users want 100% or 0% (on/off); partial quantize is a DAW clip-level concern | Binary on/off is sufficient; defer strength parameter to v2 |
| MIDI output channel expansion beyond 4 voices | Current 4-voice model is the product's identity | Leave 4-voice routing as is |
| Chord name display | Chord analysis engine is significant work; ChordJoystick voices can be non-traditional voicings that don't map to named chords cleanly | Show note names (C3, Eb4) — already derivable from held pitch |
| Separate quantize per voice | Over-complicating for v1.1; one subdivision governs all voices | Single global quantize subdivision |

---

## Feature Dependencies

```
Trigger Quantization (Live)
    └──requires──> Looper Record Gate path (already exists)
    └──requires──> New APVTS param: quantizeSubdiv
    └──requires──> New APVTS param: liveQuantizeEnabled (bool)

Trigger Quantization (Post-Record)
    └──requires──> LooperEngine::quantizeGates() — new method
    └──requires──> pendingQuantize_ atomic in PluginProcessor
    └──requires──> quantizeSubdiv APVTS param (shared with live quantize)
    └──requires──> QUANTIZE button in PluginEditor

Looper Position Bar
    └──requires──> getPlaybackBeat() — already in LooperEngine
    └──requires──> getLoopLengthBeats() — already in LooperEngine
    └──independent of quantize features

Full MIDI Panic
    └──requires──> existing pendingPanic_ flag (already in place)
    └──enhances by──> looping CC over 16 channels
    └──independent of quantize features

Gamepad Type Display
    └──requires──> SDL_GameControllerGetType() call in tryOpenController()
    └──requires──> controllerType_ storage in GamepadInput
    └──requires──> onConnectionChangeUI callback (already wired to PluginEditor)

Live Quantize
    └──enhances──> Post-Record Quantize
        (live quantize reduces need for post-record correction;
         they are complementary, share the same subdivision param)

Full Panic
    └──should trigger──> Filter CC reset (already in pendingCcReset_ path)
    └──should trigger after──> looper stop (already done)
```

### Dependency Notes

- **quantizeSubdiv is shared** between live quantize and post-record quantize — one APVTS parameter, one ComboBox.
- **Live quantize and post-record quantize are independent features** but share the subdivision selector. Plan them in the same phase to avoid adding the subdivision UI twice.
- **Looper position bar has zero dependencies on new features** — it reads existing LooperEngine APIs and can be built in any phase.
- **Full panic has zero external dependencies** — it is a pure processBlock change.
- **Gamepad type detection has zero dependencies** — it is a pure GamepadInput change at connection time.

---

## Feature Prioritization Matrix (v1.1)

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Full MIDI panic (16-channel sweep) | HIGH — fixes real stuck-note problem in live use | LOW | P1 |
| Looper position bar | HIGH — removes "where am I in the loop?" confusion | LOW | P1 |
| Post-record QUANTIZE button | HIGH — most requested looper feature after recording | MEDIUM | P1 |
| Live trigger quantization | MEDIUM — nice for tightening performances | MEDIUM | P2 |
| Quantize subdivision selector | HIGH (required by quantize) | LOW | P1 |
| Gamepad type in status label | MEDIUM — polish, improves discoverability | LOW | P2 |
| Animated mute state visual | MEDIUM — prevents "is it broken?" confusion | LOW | P2 |
| Section visual grouping (UI) | LOW-MEDIUM — polish | LOW | P2 |

**Priority key:**
- P1: Must ship in v1.1
- P2: Should ship in v1.1, include if timeline allows

---

## Implementation Behavior Specifications

### Panic: Full CC Reset Sequence

Send the following on EACH of channels 1–16 (in order):

```cpp
// Per channel ch (1-based):
midi.addEvent(MidiMessage::controllerEvent(ch, 64, 0),   0);  // sustain off
midi.addEvent(MidiMessage::controllerEvent(ch, 120, 0),  0);  // all sound off
midi.addEvent(MidiMessage::allNotesOff(ch),              0);  // CC123 all notes off
midi.addEvent(MidiMessage::controllerEvent(ch, 121, 0),  0);  // reset all controllers
midi.addEvent(MidiMessage::pitchWheel(ch, 8192),         0);  // pitch bend center
```

Then additionally, for the filter MIDI channel (may overlap with voice channels):
```cpp
midi.addEvent(MidiMessage::controllerEvent(filterCh, 74, 0),  0);  // cutoff to 0
midi.addEvent(MidiMessage::controllerEvent(filterCh, 71, 0),  0);  // res to 0
midi.addEvent(MidiMessage::controllerEvent(filterCh, 12, 0),  0);  // VCF LFO to 0
midi.addEvent(MidiMessage::controllerEvent(filterCh,  1, 0),  0);  // mod wheel to 0
midi.addEvent(MidiMessage::controllerEvent(filterCh, 76, 0),  0);  // LFO rate to 0
```

Also: reset `prevCcCut_`, `prevCcRes_` to -1 so next filter CC emission fires fresh.

### Quantize Algorithm (Gate Events Only)

```cpp
double quantizeGateBeat(double beatPosition, double subdivBeats, double loopLengthBeats)
{
    double gridIndex   = std::round(beatPosition / subdivBeats);
    double snapped     = gridIndex * subdivBeats;
    // Wrap to loop boundary
    snapped = std::fmod(snapped, loopLengthBeats);
    if (snapped < 0.0) snapped += loopLengthBeats;
    return snapped;
}
```

Subdivision in beats: 1/4 = 1.0, 1/8 = 0.5, 1/16 = 0.25, 1/32 = 0.125.
Apply only to `LooperEvent::Type::Gate`. Skip JoystickX, JoystickY, FilterX, FilterY.

### Looper Position Bar Paint Logic

```cpp
void LooperPositionBar::paint(juce::Graphics& g)
{
    const double beat       = proc_.looper_.getPlaybackBeat();
    const double loopBeats  = proc_.looper_.getLoopLengthBeats();
    const float  ratio      = (loopBeats > 0.0)
                                ? juce::jlimit(0.0f, 1.0f, (float)(beat / loopBeats))
                                : 0.0f;

    g.fillAll(juce::Colours::black);

    const bool isRecording = proc_.looperIsRecording();
    const juce::Colour barColour = isRecording
        ? juce::Colour(0xFFFF6600)   // amber when recording
        : juce::Colour(0xFF00FFFF);  // cyan when playing

    if (proc_.looperIsPlaying() || isRecording)
        g.setColour(barColour);
    else
        g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));

    g.fillRect(getLocalBounds().toFloat().withWidth(getWidth() * ratio));
}
```

Update: call `looperPositionBar_.repaint()` in `PluginEditor::timerCallback()`.

### Gamepad Type Display

```cpp
// In GamepadInput::tryOpenController(), after SDL_GameControllerOpen() succeeds:
SDL_GameControllerType type = SDL_GameControllerGetType(controller_);
const char* name = SDL_GameControllerName(controller_);

controllerType_.store((int)type, std::memory_order_relaxed);
// controllerName_ written on message thread via onConnectionChangeUI callback;
// pass the name string through the callback argument or via a separate atomic string.
```

```cpp
// Helper: type enum → display string
juce::String controllerTypeToString(int type)
{
    switch (type)
    {
        case SDL_CONTROLLER_TYPE_XBOX360:                      return "Xbox 360";
        case SDL_CONTROLLER_TYPE_XBOXONE:                      return "Xbox One";
        case SDL_CONTROLLER_TYPE_PS3:                          return "PS3";
        case SDL_CONTROLLER_TYPE_PS4:                          return "PS4";
        case SDL_CONTROLLER_TYPE_PS5:                          return "PS5";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:          return "Switch Pro";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:  return "Joy-Con";
        default:                                               return "Controller";
    }
}
// Display: typeStr + " Connected"
```

Note: `SDL_GameControllerGetType` is available since SDL 2.0.12. ChordJoystick uses SDL2 2.30.9. No version gate needed.

---

## Sources

### Primary (HIGH confidence)
- MIDI 1.0 Specification: CC 120 (All Sound Off), CC 121 (Reset All Controllers), CC 123 (All Notes Off) — standardized, unchanging
- SDL2 Wiki — `SDL_GameControllerGetType`: https://wiki.libsdl.org/SDL2/SDL_GameControllerGetType
- SDL2 Wiki — CategoryGameController: https://wiki.libsdl.org/SDL2/CategoryGameController
- SDL2 source — `SDL_gamecontroller.h` via libsdl-org/SDL: confirmed enum values for SDL_GameControllerType
- Project source code — `PluginProcessor.cpp`, `GamepadInput.h`, `LooperEngine.h`, `PluginEditor.h` (v1.0 baseline)

### Secondary (MEDIUM confidence)
- Steinberg Cubase Reset Function docs (archive.steinberg.help): confirms "note-off messages + reset controllers on all MIDI channels" as standard panic
- iConnectivity MIDI Panic Setup App docs: confirms default sequence includes Sustain Off, All Sound Off, Reset Controls, All Notes Off, Pitch Bend Off on all 16 channels
- Logic Pro support (support.apple.com): confirms "Send discrete Note Offs" and "Send Reset Controllers (CC121)" as two separate panic operations
- Ableton forum discussions (forum.ableton.com): confirms Ableton lacks built-in panic button; best practice is CC120 + CC123 + CC121 per channel
- SooperLooper changelog v1.7.9: confirms loop position indicator is a standard expected looper UI feature
- SDL_GameController blog post (blog.rubenwardy.com, Jan 2023): confirms SDL_GameControllerGetType usage pattern

### Tertiary (LOW confidence — for context only)
- KVR Audio forum discussions — looper plugin feature expectations
- Fractal Audio forum — looper quantization behavior descriptions
- MPC forum discussions — quantization terminology ("Time Correct") and live quantize behavior

---

## Appendix: v1.0 Feature Landscape (Preserved for Roadmap Continuity)

All v1.0 table stakes and differentiators remain valid from the 2026-02-22 research. Key competitive observations:

- No competitor (Scaler 2, Chord Prism, Ripchord, Chordjam) offers gamepad support, beat-locked joystick looper, sample-and-hold gate model, or per-voice trigger source — ChordJoystick's core differentiators are intact.
- v1.1 additions are pure polish and UX improvements to an already differentiated product.

---
*Feature research for: JUCE VST3 MIDI Chord Generator / Performance Controller — v1.1 Additions*
*Researched: 2026-02-24*
