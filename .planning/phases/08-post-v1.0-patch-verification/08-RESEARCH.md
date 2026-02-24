# Phase 08: Post-v1.0 Patch Verification - Research

**Researched:** 2026-02-24
**Domain:** JUCE VST3 MIDI-effect C++ plugin — code audit, manual verification protocol, test artifact design
**Confidence:** HIGH

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PATCH-01 | Plugin sends CC64=127 (sustain on) before each note-on so sustain-pedal-aware synths don't cut notes | Code audit reveals this is NOT implemented — gap found; must be added before verification |
| PATCH-02 | Per-voice hold mode — pad press sends note-off (mute), pad release sends note-on with current pitch (inverse gate) | Fully implemented; verified in PluginEditor.cpp TouchPlate and PluginProcessor.cpp gamepad path |
| PATCH-03 | Filter Mod button gates all left-stick CC output; mode dropdowns and atten knobs visually disabled when Filter Mod is off | Fully implemented; verified in timerCallback grey-out logic and processBlock filterModOn guard |
| PATCH-04 | MIDI Panic is a one-shot action — press sends allNotesOff and button immediately returns to pressable state | PARTIALLY correct: pendingPanic_ is one-shot but the UI wires panic as a toggle-mute, not a one-shot; conflicts with v1.1 design intent |
| PATCH-05 | Filter looper recording always active (recFilter_=true) — left-stick gestures recorded when looper is in record mode | Fully implemented; recFilter_ default true in LooperEngine.h, recordFilter() called in processBlock |
| PATCH-06 | Joystick threshold applied to filter CC dedup — changing atten knobs does not retrigger filter CC | Fully implemented; filterMoved guard uses fThresh before emitting CC |
</phase_requirements>

---

## Summary

This is a verification phase, not an implementation phase — with one critical exception. A full code audit of the six source files reveals that five of the six patches (PATCH-02 through PATCH-06) are correctly implemented in the current codebase. PATCH-01 (CC64=127 sustain-on before every note-on) is the single outlier: there is no `controllerEvent(ch, 64, 127)` call anywhere in TriggerSystem.cpp or PluginProcessor.cpp before note-on events. The REQUIREMENTS.md describes this as "already implemented," which is inaccurate for PATCH-01. The phase plan must include a small code task to add the CC64=127 before addressing verification.

A second issue concerns PATCH-04. The MIDI Panic button is currently wired as a persistent toggle-mute: pressing it calls `proc_.setMidiMuted(true)` which permanently blocks all MIDI output until pressed again ("MUTED" state). The REQUIREMENTS.md and STATE.md locked decision describe panic as "one-shot action — press sends allNotesOff and button immediately returns to pressable state with no persistent blocking." The current UI implementation is a toggle, not a one-shot. This is an architectural conflict that the phase plan must address — either the UI needs rework, or the requirement needs a clarification. The planner should treat PATCH-04 as requiring a UI change from toggle-mute to one-shot, consistent with the STATE.md locked decision.

Verification approach for this phase: all six patches are fundamentally observable via MIDI monitor (a virtual MIDI cable to a MIDI Monitor app like MIDI-OX or MIDIView). Four are binary pass/fail with a MIDI monitor. Two (PATCH-02, PATCH-03) are confirmed by both MIDI monitor and visual UI inspection. None of the six can be meaningfully Catch2 unit-tested without mocking the full processBlock audio thread — manual verification protocol with a signed-off checklist is the correct artifact.

**Primary recommendation:** Fix PATCH-01 (add CC64=127 before each note-on) and PATCH-04 UI (replace toggle-mute with one-shot panic) first, then run the 6-patch manual verification checklist and produce a VALIDATION.md sign-off.

---

## Standard Stack

### Core (already in project)
| Component | Version | Purpose | Status |
|-----------|---------|---------|--------|
| JUCE | 8.0.4 | VST3 plugin framework, MIDI buffer, AudioProcessor | In project |
| SDL2 | release-2.30.9 | Gamepad input (GamepadInput.cpp) | In project |
| CMake + FetchContent | — | Build system | In project |
| VS 2026 / C++17 | — | Compiler | In project |

### Verification Tools (external, no build change)
| Tool | Purpose | Platform |
|------|---------|---------|
| MIDI-OX or MIDIView | Capture plugin's MIDI output stream for PATCH-01, PATCH-04, PATCH-05, PATCH-06 | Windows free |
| loopMIDI (Tobias Erichsen) | Virtual MIDI cable: plugin output → MIDI monitor | Windows free |
| A sustain-pedal-aware soft synth (e.g., Surge XT, Vital) | Verify PATCH-01 CC64 behavior aurally | Free |
| Plugin host (Ableton Live or VST3 host) | Required to instantiate the plugin | Already owned |

No new libraries required. No CMakeLists.txt changes required for verification itself.

---

## Architecture Patterns

### Pattern 1: Pre-Note-On CC64=127 Inject (for PATCH-01 fix)

PATCH-01 requires emitting `CC64=127` on the voice's MIDI channel immediately before every `noteOn()` event. The correct insertion point is in `TriggerSystem::fireNoteOn()` — the single call site that produces all note-on events across all trigger sources.

**What:** Emit `controllerEvent(ch0+1, 64, 127)` via the existing `p.onNote`-equivalent mechanism before the note-on fires.

**Problem:** `fireNoteOn` only has access to `p.onNote` which is a `std::function<void(int voice, int pitch, bool isOn, int sampleOff)>`. There is no CC-emission callback in TriggerSystem's ProcessParams.

**Correct approach:** Emit CC64=127 in the `tp.onNote` lambda inside `processBlock` in PluginProcessor.cpp, before calling `midi.addEvent(noteOn(...))`:

```cpp
// In tp.onNote lambda (PluginProcessor.cpp ~line 516):
if (isOn)
{
    // PATCH-01: CC64=127 sustain-on before every note-on
    midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 64, 127), sampleOff);
    midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), sampleOff);
}
```

This also must be added to looper playback note-on (PluginProcessor.cpp ~line 490):

```cpp
if (loopOut.gateOn[v])
{
    midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 64, 127), 0);  // PATCH-01
    midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, heldPitch_[v], (uint8_t)100), 0);
}
```

**Source:** Direct code audit of TriggerSystem.cpp, PluginProcessor.cpp — confirmed no CC64 before noteOn anywhere.

### Pattern 2: One-Shot Panic UI (for PATCH-04 fix)

The current panicBtn_ is wired as `setClickingTogglesState(true)` with persistent midiMuted_ blocking. The REQUIREMENTS.md and STATE.md locked decision require: "press sends allNotesOff and button immediately returns to pressable state."

**What:** Change panic to non-toggling. Remove the midiMuted_ connection from the panic button. Keep midiMuted_ for the R3 gamepad path if desired, but the UI PANIC button must be one-shot.

```cpp
// In PluginEditor constructor (PluginEditor.cpp ~line 1082):
panicBtn_.setClickingTogglesState(false);  // one-shot, not toggle
panicBtn_.onClick = [this]
{
    proc_.triggerPanic();   // fires allNotesOff this block, button stays pressable
    // flashPanic_ incremented by audio thread — timerCallback handles visual flash
};
```

The `flashPanic_` counter already exists and drives a 167ms highlight. After the fix, the panicBtn_ visual cycle is: unpressed → brief red flash → back to unpressed. No persistent MUTED state from UI button. The R3 gamepad still calls `triggerPanic()` via `pendingPanic_` — that path is already one-shot.

**Note:** The midiMuted_/MUTED toggle from R3 press (PluginProcessor.cpp ~line 379) is separate from the UI panic button and is out of scope for Phase 08. Phase 08 only covers the UI button wiring per PATCH-04.

### Pattern 3: Manual Verification Protocol

For a JUCE VST3 MIDI-effect, the verification artifact is a filled-in VALIDATION.md checklist, not Catch2 tests. Each patch has a specific observable signal:

| Patch | Tool | Observable Signal |
|-------|------|-------------------|
| PATCH-01 | MIDI monitor | Every note-on preceded by CC64=127 on same channel, same tick |
| PATCH-02 | Ear + MIDI monitor | Press silences (note-off), release sounds (note-on); MIDI monitor confirms order |
| PATCH-03 | Plugin UI visual | Dropdowns and knobs grey out when Filter Mod is OFF |
| PATCH-04 | Plugin UI + MIDI monitor | One press → allNotesOff burst → button returns to normal, no MUTED state |
| PATCH-05 | MIDI monitor playback | Loop replay contains filter CC events when left-stick was moved during record |
| PATCH-06 | MIDI monitor | Wiggling attenuator knob emits zero filter CC if stick is still |

### Project Structure (unchanged)

```
Source/
├── PluginProcessor.cpp   # PATCH-01: add CC64=127 before noteOn (2 locations)
│                         # PATCH-04: remove midiMuted_ from panicBtn_ onClick
├── PluginEditor.cpp      # PATCH-04: set setClickingTogglesState(false) on panicBtn_
├── TriggerSystem.cpp     # No changes needed
├── LooperEngine.cpp      # No changes needed
├── GamepadInput.cpp      # No changes needed
```

### Anti-Patterns to Avoid

- **Don't add CC64 in TriggerSystem::fireNoteOn:** `fireNoteOn` has no MIDI buffer reference; adding one would require threading the buffer reference through ProcessParams — unnecessary complexity.
- **Don't add Catch2 tests for PATCH-01 through PATCH-06:** These patches interact with SDL2, JUCE audio thread, MIDI buffer, and real-time state. Mocking the full chain produces false confidence. Manual MIDI monitor verification is correct.
- **Don't add CC64 only to the TriggerSystem path:** Looper playback note-ons bypass TriggerSystem entirely — both emission sites need the CC64 injection.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| MIDI output capture | Custom MIDI logging in plugin | loopMIDI + MIDI-OX | Real external tool sees exactly what the DAW sees |
| CC64 value type | Complex sustain state machine | Single controllerEvent(ch, 64, 127) before each noteOn | Sustain-pedal-aware synths just need the flag high at note-on time |
| Visual flash animation | New timer / thread | Existing `flashPanic_` counter + `startTimerHz(30)` already in editor | STATE.md: "Single 30 Hz timer only" |

**Key insight:** PATCH-01 is two lines of code (one per note-on emission site). PATCH-04 UI is removing three lines and simplifying to one. The complexity in this phase is verification procedure and documentation, not code.

---

## Common Pitfalls

### Pitfall 1: Believing All 6 Patches Are Already Complete
**What goes wrong:** Phase description says "patches are ALREADY implemented." Taking that at face value and skipping code audit.
**Why it happens:** Natural assumption from phase framing.
**How to avoid:** PATCH-01 (CC64=127) is confirmed ABSENT from the code. Audit before writing tests.
**Warning signs:** No `controllerEvent.*64` in any grep of Source/ — confirmed absent.

### Pitfall 2: PATCH-04 Toggle vs. One-Shot Confusion
**What goes wrong:** Verifying that "MIDI Panic works" without noticing the current behavior is a toggle-mute (persistent MUTED state) rather than the one-shot required by STATE.md.
**Why it happens:** The button does fire allNotesOff — it appears to work — but the persistent block violates the locked requirement.
**How to avoid:** Test explicitly: press once, observe allNotesOff, then immediately try to trigger a new note. In the current code it will be silenced. In the fixed code it will sound normally.
**Warning signs:** After pressing PANIC, the button shows "MUTED" and subsequent note triggers produce no MIDI output.

### Pitfall 3: PATCH-05 Looper Filter Recording Confusion
**What goes wrong:** Assuming "filter looper always active" means filters are always recorded. It means `recFilter_=true` by default, but recording only happens when the looper is in record mode (recording_=true).
**Why it happens:** Ambiguous phrasing.
**How to avoid:** PATCH-05 verification step: arm REC, move left stick, stop recording, play back — the filter CC should replay. If REC is not armed, no recording occurs even with recFilter_=true. This is correct behavior.
**Warning signs:** Trying to verify by moving the stick without having pressed REC first.

### Pitfall 4: CC64 on Wrong Channel
**What goes wrong:** Sending CC64=127 on a fixed channel (e.g., ch 1) instead of the voice's configured MIDI channel.
**Why it happens:** Copying a pattern from the filter CC section which uses filterMidiCh.
**How to avoid:** CC64 must use `voiceChs[voice]` — the same channel as the note-on it precedes. Each voice can be on a different channel (voiceCh0..3 default 1-4).
**Warning signs:** MIDI monitor shows CC64 on channel 1 but note-on on channel 3 — they must match.

### Pitfall 5: Only Adding CC64 for TriggerSystem Path (Missing Looper Playback)
**What goes wrong:** Adding CC64 only in the `tp.onNote` lambda, not before looper playback `gateOn[v]` note-ons.
**Why it happens:** Looper note-on emission (PluginProcessor.cpp ~line 487-498) is a separate path that bypasses TriggerSystem.
**How to avoid:** There are exactly two note-on emission sites: (1) `tp.onNote` lambda and (2) looper `gateOn[v]` section. Both need CC64=127.
**Warning signs:** CC64 appears before pad-triggered notes but not before looper-triggered notes.

---

## Code Examples

### Current PATCH-01 Gap (confirmed absent)

```cpp
// PluginProcessor.cpp ~line 516 — tp.onNote lambda (CURRENT — missing CC64):
tp.onNote = [&](int voice, int pitch, bool isOn, int sampleOff)
{
    if (isOn) anyNoteOnThisBlock = true;
    const int ch0 = voiceChs[voice] - 1;
    if (isOn)
    {
        // *** NO CC64=127 HERE — PATCH-01 gap ***
        midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), sampleOff);
    }
    // ...
};
```

### PATCH-01 Fix (to be applied)

```cpp
// PluginProcessor.cpp tp.onNote lambda — add CC64 before noteOn:
if (isOn)
{
    midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 64, 127), sampleOff);
    midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), sampleOff);
}

// PluginProcessor.cpp looper playback section (~line 490) — add CC64 before looper noteOn:
if (loopOut.gateOn[v])
{
    midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 64, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, heldPitch_[v], (uint8_t)100), 0);
}
```

### PATCH-04 Fix (to be applied)

```cpp
// PluginEditor.cpp constructor — replace toggle-mute with one-shot:
panicBtn_.setClickingTogglesState(false);   // was: true
panicBtn_.onClick = [this]
{
    proc_.triggerPanic();  // pendingPanic_.store(true) — one-shot, no persistent mute
    // flashPanic_ incremented from audio thread; timerCallback handles 167ms flash
};
```

### PATCH-02 Implementation (VERIFIED CORRECT)

```cpp
// TouchPlate::mouseDown (PluginEditor.cpp ~line 334):
if (proc_.padHold_[voice_].load())
    proc_.setPadState(voice_, false);  // hold mode: press = note-off (mute)
else
    proc_.setPadState(voice_, true);   // normal: press = note-on

// TouchPlate::mouseUp:
if (proc_.padHold_[voice_].load())
    proc_.setPadState(voice_, true);   // hold mode: release = note-on (resume)
else
    proc_.setPadState(voice_, false);  // normal: release = note-off
```

### PATCH-03 Implementation (VERIFIED CORRECT)

```cpp
// PluginEditor::timerCallback (~line 1511):
const bool filterOn = proc_.isFilterModActive();
filterYModeBox_.setEnabled(filterOn);
filterXModeBox_.setEnabled(filterOn);
filterXAttenKnob_.setEnabled(filterOn);
filterYAttenKnob_.setEnabled(filterOn);

// PluginProcessor.cpp processBlock filter CC section (~line 646):
const bool filterModOn = filterModActive_.load(std::memory_order_relaxed);
const bool looperDriving = filterModOn && (loopOut.hasFilterX || loopOut.hasFilterY);
const bool liveGamepad   = filterModOn && gamepad_.isConnected() && gamepadActive_.load(...);
if (looperDriving || liveGamepad) { /* emit CC */ }
```

### PATCH-05 Implementation (VERIFIED CORRECT)

```cpp
// LooperEngine.h line 151:
std::atomic<bool> recFilter_ { true };  // filter recording always on (default true)

// PluginProcessor.cpp processBlock (~line 613):
if (gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed))
    looper_.recordFilter(beatPos, gamepad_.getFilterX(), gamepad_.getFilterY());
```

### PATCH-06 Implementation (VERIFIED CORRECT)

```cpp
// PluginProcessor.cpp processBlock filter CC section (~line 672):
const float fThresh = apvts.getRawParameterValue("joystickThreshold")->load();
const bool filterMoved = looperUpdate
    || (std::abs(gfx - prevFilterX_) > fThresh)
    || (std::abs(gfy - prevFilterY_) > fThresh);
// Only emits CC if filterMoved — atten knob changes alone don't emit
```

---

## Implementation Status (Full Audit)

| Patch | Status | Location | Notes |
|-------|--------|----------|-------|
| PATCH-01 | **MISSING — must implement** | PluginProcessor.cpp tp.onNote + looper gateOn path | No controllerEvent(ch, 64, 127) exists anywhere in Source/ |
| PATCH-02 | Implemented correctly | PluginEditor.cpp TouchPlate::mouseDown/Up, processBlock gamepad voice path | Inverted gate logic confirmed in both UI and gamepad paths |
| PATCH-03 | Implemented correctly | timerCallback (UI grey-out) + processBlock filterModOn guard | Both the CC blocking and the UI disable are wired correctly |
| PATCH-04 | **Partial — UI must be fixed** | PluginEditor.cpp panicBtn_ onClick | Current: toggle-mute. Required: one-shot. triggerPanic() itself is one-shot; only the UI wiring is wrong |
| PATCH-05 | Implemented correctly | LooperEngine.h recFilter_=true, processBlock recordFilter() call | Always-on default confirmed; call is guarded by gamepad connection and gamepadActive_ flag |
| PATCH-06 | Implemented correctly | processBlock filter CC section, fThresh/filterMoved guard | Threshold comes from joystickThreshold APVTS param; atten-only changes do not update prevFilterX_/Y_ |

---

## State of the Art

| Old Approach | Current Approach | Impact on This Phase |
|--------------|------------------|---------------------|
| Panic as persistent mute toggle | Locked decision: panic is one-shot | PATCH-04 UI wiring contradicts locked decision — fix required |
| No CC64 sustain preamble | PATCH-01 requires CC64=127 before each note-on | Must be added — not yet present |
| Hard-coded dead-zone | joystickThreshold APVTS param → GamepadInput::setDeadZone | PATCH-06 uses this param correctly |

---

## Verification Protocol (Manual Checklist Template)

The planner should schedule a VALIDATION.md document as the final deliverable. Template for each patch:

```
PATCH-01: CC64=127 sustain-on before note-on
  Setup: loopMIDI virtual cable → MIDI-OX → Plugin → instrument
  Steps: Press any touchplate or trigger via gamepad
  Pass: Every noteOn in MIDI monitor preceded by CC64=127 on same ch
  Fail: noteOn appears without CC64=127 preceding it

PATCH-02: Hold mode inverts gate
  Setup: Plugin loaded, voice in PAD mode, HOLD button active
  Steps: Press pad (should hear note-off / silence), release pad (should hear note-on)
  Pass: Release sounds the note; press silences it
  Fail: Normal behavior (press=on, release=off)

PATCH-03: Filter Mod gates CC output and greys UI
  Setup: Plugin UI visible, Filter Mod OFF
  Steps: Move gamepad left stick; inspect MIDI monitor and UI
  Pass: Zero filter CC in MIDI monitor; dropdowns and knobs visually dimmed
  Fail: CC emitted while Filter Mod is OFF; UI controls remain active

PATCH-04: Panic is one-shot
  Setup: Plugin loaded with a note held
  Steps: Press PANIC; try to trigger new note immediately after
  Pass: allNotesOff burst in monitor; button returns to normal; next trigger sounds
  Fail: Button shows MUTED; subsequent notes silenced until pressed again

PATCH-05: Filter looper records left-stick gestures
  Setup: Looper REC armed
  Steps: Press REC, move left stick, press REC again to stop; press PLAY
  Pass: Loop playback produces filter CC events matching recorded stick movement
  Fail: Loop playback has no filter CC; only gate events play back

PATCH-06: Atten knob change does not retrigger filter CC
  Setup: loopMIDI → MIDI-OX; gamepad connected with left stick stationary
  Steps: Wiggle Filter Cutoff Atten or Filter Res Atten knobs
  Pass: Zero new CC74/CC71 events in MIDI monitor while stick is still
  Fail: CC74/CC71 events appear when only the knob changed
```

---

## Open Questions

1. **PATCH-04 R3 toggle-mute scope**
   - What we know: R3 on gamepad calls `pendingPanic_` which toggles `midiMuted_` (persistent). STATE.md says panic is one-shot with no persistent blocking.
   - What's unclear: Should the R3 path also be converted to one-shot, or does Phase 08 only cover the UI button?
   - Recommendation: Phase 08 fixes the UI panic button per PATCH-04. The R3 toggle-mute is separate functionality (listed as PANIC-03 territory) and should be deferred to Phase 09's panic overhaul.

2. **CC64 on looper voice channels vs. filter channel**
   - What we know: Looper plays back on `voiceChs[v]`, filter uses `filterMidiCh`.
   - What's unclear: Should CC64 precede looper playback note-ons on voiceChs[v], or only TriggerSystem-fired note-ons?
   - Recommendation: CC64=127 must precede ALL note-on events regardless of source, on the voice's own channel. Both emission sites need it.

---

## Sources

### Primary (HIGH confidence)
- Direct code audit of `Source/PluginProcessor.cpp` (lines 1-736) — confirmed no CC64 before noteOn
- Direct code audit of `Source/TriggerSystem.cpp` (lines 297-314) — confirmed fireNoteOn has no CC injection
- Direct code audit of `Source/PluginEditor.cpp` (lines 1078-1101) — confirmed panic is toggle-mute
- Direct code audit of `Source/LooperEngine.h` (line 151) — confirmed recFilter_ default true (PATCH-05)
- `.planning/REQUIREMENTS.md` — PATCH-01 through PATCH-06 definitions
- `.planning/STATE.md` — Locked decisions: panic is one-shot, single 30 Hz timer

### Secondary (MEDIUM confidence)
- Project MEMORY.md — architecture overview, confirmed isMidiEffect()=true, voiceCh0..3 default 1-4
- ROADMAP.md Phase 08 success criteria — defines exact verifiable conditions for each patch

---

## Metadata

**Confidence breakdown:**
- PATCH-01 gap: HIGH — confirmed absent via grep of all Source/ files
- PATCH-02 implementation: HIGH — full code path traced from UI click to TriggerSystem gate state
- PATCH-03 implementation: HIGH — both CC blocking (processBlock) and UI greying (timerCallback) verified
- PATCH-04 UI conflict: HIGH — setClickingTogglesState(true) and midiMuted_ persistent state confirmed
- PATCH-05 implementation: HIGH — recFilter_ default and recordFilter() call site both confirmed
- PATCH-06 implementation: HIGH — filterMoved guard logic confirmed, uses joystickThreshold param
- Verification protocol: HIGH — standard MIDI monitor approach, well-established for MIDI-effect plugins

**Research date:** 2026-02-24
**Valid until:** 2026-03-24 (stable C++ codebase; changes only when source files change)
