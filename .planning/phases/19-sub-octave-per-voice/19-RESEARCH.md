# Phase 19: Sub Octave Per Voice — Research

**Researched:** 2026-03-01
**Domain:** JUCE VST3 MIDI generation — parallel sub-octave note emission, APVTS bool params, UI button layout split, gamepad combo detection
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Decision 1: SUB8 Button Layout**
Split the existing 18px HOLD strip 50/50 — HOLD on the left half, SUB8 on the right half.
- Label: `"SUB8"` (not SUB, -8, OCT, or 8VB)
- Color when active: orange/amber — distinct from green HOLD, unused in current palette
- Color when inactive: same dim style as HOLD inactive
- When HOLD and SUB8 are both active: sub-octave note obeys hold behavior (stays on after release; press = note-off, release = note-on) — exactly the same hold-invert logic as the main note
- Always visible regardless of trigger source (Pad / Joystick / Random)
- `padHoldBtn_[v]` resized to left 50%; `padSubOctBtn_[v]` added as new TextButton at right 50%

**Decision 2: Mid-Note Toggle Behavior**
- Enable SUB8 while gate is open → immediate note-on for the sub-octave pitch
- Disable SUB8 while sub-octave is sounding → immediate note-off
- Sub-octave pitch is locked to trigger-time — always exactly -12 semitones from the main note's snapshot pitch, never from the live joystick position
- Looper playback uses the live SUB8 state at emission time (not baked into the loop)

**Decision 3: Gamepad R3 Behavior**
- R3 alone: PANIC REMOVED in this phase. R3 standalone no longer triggers all-notes-off. Panic is UI-button only from Phase 19 onward.
- Combo: hold any pad button (L1/L2/R1/R2) + press R3 → toggle SUB8 for that voice
- Works in any Option Mode (Mode 0 and Mode 1)
- Multiple pads held simultaneously + R3 → toggle SUB8 for all held voices
- No special feedback for the combo — state updates via APVTS param, SUB8 button reflects new state on next timerCallback tick (30Hz)

**Decision 4: Visual Feedback**
- SUB8 button lit up (orange/amber) = sufficient. No changes to the TouchPlate body.
- Brightening when sounding: SUB8 button uses a brighter orange when sub-octave is actively sounding. Condition: `SUB8 enabled AND gate open for that voice`.
- Read via: `*apvts.getRawParameterValue("subOct" + String(v)) > 0.5f && proc_.isGateOpen(v)`
- No dedicated `subOctActive_` atomic needed — same pattern as how HOLD reads `padHold_[v]` in `timerCallback`.

**Decision 5: APVTS Parameters (new)**
4 bool parameters, one per voice:
- `"subOct0"` — Sub Oct voice 0 (Root), default `false`
- `"subOct1"` — Sub Oct voice 1 (Third), default `false`
- `"subOct2"` — Sub Oct voice 2 (Fifth), default `false`
- `"subOct3"` — Sub Oct voice 3 (Tension), default `false`

Pattern: `addBool("subOct0", "Sub Oct Root", false)` — consistent with `arpEnabled`, `lfoXEnabled` etc.

### Claude's Discretion

None specified — all decisions locked.

### Deferred Ideas (OUT OF SCOPE)

- Sub-octave per voice in arpeggiator steps → Phase 23
- Sub-octave indicator in looper position bar → not requested
- Sub-octave CC/pitch-bend tracking → not requested
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SUBOCT-01 | Each of the 4 voice pads has a Sub Oct toggle — right half of the existing Hold button | UI layout pattern: resize padHoldBtn_[v] to 50% width; add padSubOctBtn_[v] sibling at right 50% in resized() |
| SUBOCT-02 | When Sub Oct is active for a voice, a second note-on fires exactly 12 semitones below the voice pitch on every trigger | Emit note-on inside tp.onNote isOn block using `pitch - 12`; snapshot `subHeldPitch_[v] = pitch - 12` at gate-open |
| SUBOCT-03 | Sub Oct note-off always matches the pitch used at note-on time (snapshotted, not recomputed at release) | Use `subHeldPitch_[v]` array (like `looperActivePitch_[v]` pattern); send note-off using stored value, not `heldPitch_[v] - 12` |
| SUBOCT-04 | User can toggle Sub Oct for a pad via gamepad by holding the pad button (R1/R2/L1/L2) and pressing R3 | Remove panic from `consumeRightStickTrigger()` path; check `gamepad_.getVoiceHeld(v)` across all 4 voices when R3 consumed; toggle APVTS param via `setValue()` on AudioParameterBool |
</phase_requirements>

---

## Summary

Phase 19 adds a parallel sub-octave note (-12 semitones) to each of the 4 voices, controlled by a new SUB8 toggle button and a gamepad combo. The technical domain is entirely within the existing codebase — no new libraries, no new subsystems. All patterns (APVTS bool params, note-on/off snapshot, UI button layout, gamepad combo detection, timerCallback visual update) already exist in the project and are being extended, not invented.

The most critical correctness concern is the note-off matching: the sub-octave pitch must be snapshotted at gate-open time and stored in a `subHeldPitch_[v]` array (mirroring the `looperActivePitch_[v]` pattern), then used verbatim at gate-close. If the gate closes and the sub-octave note-off uses `heldPitch_[v] - 12` instead of the stored snapshot, pitch drift from joystick movement between trigger and release will produce stuck notes. The looper gate callback path needs a parallel `looperActiveSubPitch_[v]` snapshot for the same reason.

The mid-note toggle (enable/disable SUB8 while gate open) requires reading APVTS bool state inside `processBlock` and comparing against a `subOctSounding_[v]` flag. When the bool flips true while `isGateOpen(v)`, emit an immediate note-on; when it flips false, emit an immediate note-off using `subHeldPitch_[v]`. This is the only novel logic in the phase — everything else is straightforward application of existing patterns.

**Primary recommendation:** Implement in four tasks: (1) APVTS params, (2) processor note-on/off + mid-note toggle + R3 gamepad, (3) UI layout split + button coloring, (4) smoke-test verification. The mid-note toggle detection is the highest-risk step — test it explicitly.

---

## Standard Stack

### Core (no new dependencies)

| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| JUCE AudioParameterBool | JUCE 8.0.4 | `subOct0..3` APVTS bool params | Already used for `arpEnabled`, `lfoXEnabled`, `padHold_` backing |
| juce::TextButton | JUCE 8.0.4 | `padSubOctBtn_[4]` — SUB8 toggle buttons | Same as `padHoldBtn_[4]` — identical overlay pattern |
| juce::ButtonParameterAttachment | JUCE 8.0.4 | Wire `padSubOctBtn_[v]` to `subOctN` APVTS params | Same as hold button attachments |
| std::atomic<bool> | C++17 | `subOctSounding_[4]` — track whether sub note is currently on | Same as `padHold_[v]` — audio thread writes, timer thread reads |

**No new libraries. No new CMakeLists.txt changes needed.**

---

## Architecture Patterns

### Pattern 1: APVTS Bool Parameter Declaration

**What:** Declare 4 bool parameters using the `addBool` lambda at PluginProcessor.cpp line 108.

**Where to add:** After existing `arpEnabled` / `lfoXEnabled` / `lfoYEnabled` declarations.

```cpp
// PluginProcessor.cpp — createParameterLayout()
// Source: existing pattern at line 108, mirroring arpEnabled
addBool("subOct0", "Sub Oct Root",    false);
addBool("subOct1", "Sub Oct Third",   false);
addBool("subOct2", "Sub Oct Fifth",   false);
addBool("subOct3", "Sub Oct Tension", false);
```

### Pattern 2: Sub-Pitch Snapshot Arrays

**What:** Two int arrays in PluginProcessor.h to snapshot sub-octave pitch at gate-open time.

**Pattern:** Mirrors `looperActivePitch_[4]` and `heldPitch_[4]`.

```cpp
// PluginProcessor.h — member declarations
int subHeldPitch_[4]        = { -1, -1, -1, -1 };  // sub pitch at live gate-open
int looperActiveSubPitch_[4] = { -1, -1, -1, -1 };  // sub pitch at looper gate-on
std::atomic<bool> subOctSounding_[4] = {};           // true while sub note is on
```

`subHeldPitch_[v]` is written only on gate-open (inside `tp.onNote isOn`), read at gate-close. Never re-derived from `heldPitch_[v] - 12` at note-off time.

### Pattern 3: Note-On/Off Inside tp.onNote Lambda

**What:** Inside the existing `tp.onNote` lambda (PluginProcessor.cpp ~line 913), emit sub-octave MIDI events alongside the main note events.

**When to add:** After the existing `noteCount_` dedup + `midi.addEvent(noteOn)` block.

```cpp
// Inside tp.onNote lambda — isOn branch (after existing note-on emission ~line 957-958)
if (isOn)
{
    // ... existing: sentChannel_[voice] = ch; noteCount_ dedup; noteOn ...

    // Sub-octave: emit only if enabled
    const bool subEnabled = (*apvts.getRawParameterValue("subOct" + juce::String(voice)) > 0.5f);
    if (subEnabled)
    {
        const int subPitch = juce::jlimit(0, 127, pitch - 12);
        subHeldPitch_[voice] = subPitch;
        if (noteCount_[ch - 1][subPitch]++ == 0)
            midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), sampleOff);
        subOctSounding_[voice].store(true, std::memory_order_relaxed);
    }
    else
    {
        subHeldPitch_[voice] = -1;
    }
}
else  // gate-close
{
    // ... existing: noteOff using sentChannel_[voice] + pitch ...

    // Sub-octave note-off — use snapshot, not heldPitch_[voice] - 12
    const int subPitch = subHeldPitch_[voice];
    if (subPitch >= 0)
    {
        const int ch = sentChannel_[voice];
        if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
            midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), sampleOff);
        else
            noteCount_[ch - 1][subPitch] = 0;
        subHeldPitch_[voice] = -1;
        subOctSounding_[voice].store(false, std::memory_order_relaxed);
    }
}
```

### Pattern 4: Mid-Note Toggle Detection

**What:** In `processBlock`, before the `tp.onNote` processing loop, detect SUB8 param transitions while a gate is already open and emit immediate note-on or note-off.

**Where:** After the looper gate section (~line 860), before the TriggerSystem process call.

```cpp
// processBlock — mid-note SUB8 toggle detection
// Run once per block, before TriggerSystem::process()
for (int v = 0; v < 4; ++v)
{
    const bool subWanted  = (*apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
    const bool subSounding = subOctSounding_[v].load(std::memory_order_relaxed);
    const bool gateOpen   = isGateOpen(v);

    if (gateOpen && subWanted && !subSounding)
    {
        // SUB8 toggled ON while gate open — emit immediate note-on
        const int ch       = sentChannel_[v];
        const int subPitch = juce::jlimit(0, 127, heldPitch_[v] - 12);
        subHeldPitch_[v]   = subPitch;
        if (noteCount_[ch - 1][subPitch]++ == 0)
            midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), 0);
        subOctSounding_[v].store(true, std::memory_order_relaxed);
    }
    else if (subSounding && (!subWanted || !gateOpen))
    {
        // SUB8 toggled OFF (or gate closed by other means) — emit immediate note-off
        const int ch       = sentChannel_[v];
        const int subPitch = subHeldPitch_[v];
        if (subPitch >= 0)
        {
            if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
            else
                noteCount_[ch - 1][subPitch] = 0;
            subHeldPitch_[v] = -1;
        }
        subOctSounding_[v].store(false, std::memory_order_relaxed);
    }
}
```

### Pattern 5: Looper Gate Sub-Octave Snapshot

**What:** In the looper gate section (~line 840-858), emit sub-octave notes alongside looper main-voice notes using live SUB8 param state.

**Key rule:** Do NOT bake SUB8 state into the loop recording. Use live param at emission time.

```cpp
// Looper gate-on (~line 847 area) — after existing main note-on
if (loopOut.gateOn[v])
{
    looperActivePitch_[v] = heldPitch_[v];
    const int ch = effectiveChannel(v);
    looperActiveCh_[v] = ch;
    sentChannel_[v] = ch;
    if (noteCount_[ch - 1][looperActivePitch_[v]]++ == 0)
        midi.addEvent(juce::MidiMessage::noteOn(ch, looperActivePitch_[v], (uint8_t)100), 0);

    // Sub-octave: check live param at emission time
    const bool subEnabled = (*apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
    if (subEnabled)
    {
        const int subPitch = juce::jlimit(0, 127, looperActivePitch_[v] - 12);
        looperActiveSubPitch_[v] = subPitch;
        if (noteCount_[ch - 1][subPitch]++ == 0)
            midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), 0);
    }
    else
    {
        looperActiveSubPitch_[v] = -1;
    }
}

// Looper gate-off — use snapshot
if (loopOut.gateOff[v] && looperActivePitch_[v] >= 0)
{
    const int ch = looperActiveCh_[v];
    if (noteCount_[ch - 1][looperActivePitch_[v]] > 0 &&
        --noteCount_[ch - 1][looperActivePitch_[v]] == 0)
        midi.addEvent(juce::MidiMessage::noteOff(ch, looperActivePitch_[v], (uint8_t)0), 0);
    else
        noteCount_[ch - 1][looperActivePitch_[v]] = 0;
    looperActivePitch_[v] = -1;

    // Sub-octave note-off
    const int subPitch = looperActiveSubPitch_[v];
    if (subPitch >= 0)
    {
        if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
            midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
        else
            noteCount_[ch - 1][subPitch] = 0;
        looperActiveSubPitch_[v] = -1;
    }
}
```

### Pattern 6: Gamepad R3 Combo — Remove Panic, Add SUB8 Toggle

**What:** In processBlock gamepad section (~line 491-492), replace the existing `consumeRightStickTrigger() → triggerPanic()` with conditional logic.

**Pattern:** Check `gamepad_.getVoiceHeld(v)` for all 4 voices. If any held, toggle SUB8. If none held, do nothing (panic removed).

```cpp
// processBlock — replace lines ~491-492
if (gamepad_.consumeRightStickTrigger())
{
    bool anyHeld = false;
    for (int v = 0; v < 4; ++v)
    {
        if (gamepad_.getVoiceHeld(v))
        {
            anyHeld = true;
            // Toggle subOctN APVTS bool
            const juce::String paramID = "subOct" + juce::String(v);
            auto* param = dynamic_cast<juce::AudioParameterBool*>(
                apvts.getParameter(paramID));
            if (param != nullptr)
                param->setValueNotifyingHost(param->get() ? 0.0f : 1.0f);
        }
    }
    // If !anyHeld: R3 alone — no action (panic removed per Phase 19 Decision 3)
}
```

### Pattern 7: UI Layout Split + Button Coloring

**What:** In `PluginEditor::resized()`, resize `padHoldBtn_[v]` to left 50% and position `padSubOctBtn_[v]` at right 50%. In `timerCallback()`, set SUB8 button color based on enabled + sounding state.

```cpp
// PluginEditor.cpp — resized() (near ~line 1796)
// The 18px strip at the top of each pad rect:
auto holdStrip = padRect.removeFromTop(18);
auto holdLeft  = holdStrip.removeFromLeft(holdStrip.getWidth() / 2);
padHoldBtn_[v].setBounds(holdLeft);
padSubOctBtn_[v].setBounds(holdStrip);   // remainder = right 50%

// PluginEditor.cpp — timerCallback() (30Hz visual update)
for (int v = 0; v < 4; ++v)
{
    const bool subEnabled = (*proc_.apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
    const bool gateOpen   = proc_.isGateOpen(v);

    if (subEnabled && gateOpen)
        padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFFF8C00)); // bright orange
    else if (subEnabled)
        padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFB36200)); // dim orange
    else
        padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
}
```

**ButtonParameterAttachment:** Add in PluginEditor constructor (or `connectParams()`) using the same pattern as HOLD:

```cpp
// PluginEditor.h
std::unique_ptr<juce::ButtonParameterAttachment> subOctAttach_[4];

// PluginEditor.cpp — constructor / connectParams()
for (int v = 0; v < 4; ++v)
{
    padSubOctBtn_[v].setButtonText("SUB8");
    subOctAttach_[v] = std::make_unique<juce::ButtonParameterAttachment>(
        *proc_.apvts.getParameter("subOct" + juce::String(v)),
        padSubOctBtn_[v]);
    addAndMakeVisible(padSubOctBtn_[v]);
}
```

### Anti-Patterns to Avoid

- **Re-deriving sub pitch at note-off:** `heldPitch_[v] - 12` at gate-close is wrong if the joystick moved. Always use `subHeldPitch_[v]` snapshot.
- **Baking SUB8 state into looper:** The looper records gate events only, not pitch/SUB8 state. SUB8 is read live at emission time — not recorded.
- **Separate MIDI channel for sub:** Sub-octave note goes on the same channel as the main voice (same `sentChannel_[v]`). Single-Channel mode is handled automatically by `effectiveChannel(v)`.
- **Forgetting noteCount_ for sub pitch:** The sub pitch goes through the same `noteCount_[ch-1][subPitch]` deduplication used by all other notes. Skipping this causes stuck notes in Single-Channel mode when two voices land on the same sub pitch.
- **setValueNotifyingHost on audio thread:** The R3+combo toggle calls `param->setValueNotifyingHost()` from processBlock (audio thread). This is technically non-RT-safe per JUCE docs. Safer alternative: write a pending-toggle atomic and process it in timerCallback. However, the existing codebase already calls APVTS parameter changes from processBlock in multiple places (checked: Option Mode octave changes ~line 496-530), so this is an established project pattern. Document and proceed consistently.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Button↔APVTS wiring | Custom listener/callback | `juce::ButtonParameterAttachment` | Handles undo, preset save/load, host automation — already used for padHoldBtn_ |
| Sub pitch clamping | Manual range check | `juce::jlimit(0, 127, pitch - 12)` | MIDI spec allows 0–127 only; voice 0 at very low pitch could go below 0 without clamp |
| noteCount_ deduplication | Skip for sub note | Same `noteCount_[ch-1][subPitch]` | Single-Channel mode: two voices can share the same sub pitch; skip dedup = stuck notes |

---

## Common Pitfalls

### Pitfall 1: Stuck Sub-Octave Notes on SUB8 Toggle-Off
**What goes wrong:** Toggling SUB8 off while gate is open sends no note-off → synth has stuck note.
**Why it happens:** The mid-note toggle detection loop runs before TriggerSystem; if the gate-close path doesn't check `subOctSounding_`, the note-off is never sent.
**How to avoid:** The mid-note detection loop (Pattern 4) handles this: `subSounding && !subWanted → emit note-off`. The `subOctSounding_[v]` flag is the guard.
**Warning signs:** Testing toggle-off while holding a pad produces stuck MIDI note in DAW.

### Pitfall 2: Stuck Sub-Octave Notes After Panic / allNotesOff
**What goes wrong:** `allNotesOff()` / DAW stop sends note-off for main pitches but not sub pitches if the flush only covers `voiceCh(v)` note range.
**Why it happens:** The allNotesOff flush (Phase 18 fix) sends CC123 (all notes off) on all 16 channels — which should cover sub pitches automatically. But `subHeldPitch_[v]` and `looperActiveSubPitch_[v]` arrays should also be reset to -1 inside `resetNoteCount()` or the flush path.
**How to avoid:** After calling `allNotesOff()`, also reset `subHeldPitch_[v] = -1`, `looperActiveSubPitch_[v] = -1`, and `subOctSounding_[v].store(false)` for all voices.
**Warning signs:** After DAW stop or panic, re-pressing SUB8 causes double note-on for sub pitch.

### Pitfall 3: Sub Note Below MIDI Note 0
**What goes wrong:** Voice pitch = 10, SUB8 → `10 - 12 = -2`. MIDI note-on with negative pitch is undefined behavior (JUCE will likely clamp or assert in debug).
**Why it happens:** Low-register presets with root octave set to minimum.
**How to avoid:** Always `juce::jlimit(0, 127, pitch - 12)`. If clamped pitch == main pitch, still emit (the note plays in unison instead of octave below — acceptable edge case).
**Warning signs:** Debug assert in MidiMessage constructor; or silent failure in release build.

### Pitfall 4: R3 Toggle Calls setValueNotifyingHost on Audio Thread
**What goes wrong:** JUCE docs warn `setValueNotifyingHost` is not RT-safe — it may notify listeners synchronously.
**Why it happens:** processBlock runs on audio thread; R3 combo is detected there.
**How to avoid:** This is an accepted project pattern (octave changes from gamepad are done the same way). If it causes audio glitches, move to a `pendingSubOctToggle_[4]` atomic int pattern processed in timerCallback. For Phase 19, proceed with the direct call — it matches existing code style.
**Warning signs:** Audio dropout when pressing R3 combo.

### Pitfall 5: HOLD + SUB8 Both Active — Hold Logic for Sub Note
**What goes wrong:** HOLD is on, gate is in hold-inverted state (press = note-off, release = note-on). SUB8 should follow the exact same behavior — if hold means the main note is sustained, sub-octave must also be sustained.
**Why it happens:** The mid-note toggle detection reads `isGateOpen(v)` — which reflects the hold-inverted state correctly. The sub note tracks the main note's gate state via the same flag. No special handling needed.
**How to avoid:** No extra code needed — `isGateOpen(v)` already accounts for hold mode.
**Warning signs:** SUB8 on + HOLD on → releasing pad stops sub note but not main note.

### Pitfall 6: MSVC C2923 from Type Alias in Header
**What goes wrong:** If `ButtonParameterAttachment` is forward-declared using a `using` alias in the same class before it's defined (like the `ComboAtt` issue from Phase 18).
**Why it happens:** MSVC ordering sensitivity in class bodies.
**How to avoid:** Use full type `std::unique_ptr<juce::ButtonParameterAttachment> subOctAttach_[4]` in the header declaration. Do not use a local alias.

---

## Code Examples

### Verified Pattern: AudioParameterBool Declaration
```cpp
// Source: PluginProcessor.cpp line 108-111 (existing addBool lambda)
auto addBool = [&](const juce::String& id, const juce::String& name, bool def)
{
    layout.add(std::make_unique<juce::AudioParameterBool>(id, name, def));
};
addBool("subOct0", "Sub Oct Root",    false);
addBool("subOct1", "Sub Oct Third",   false);
addBool("subOct2", "Sub Oct Fifth",   false);
addBool("subOct3", "Sub Oct Tension", false);
```

### Verified Pattern: Reading APVTS Bool in processBlock
```cpp
// Source: existing pattern (e.g., arpEnabled read, lfoXEnabled read)
const bool subEnabled = (*apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
```

### Verified Pattern: noteCount_ Deduplication for noteOn
```cpp
// Source: PluginProcessor.cpp line 957-958
if (noteCount_[ch - 1][subPitch]++ == 0)
    midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), sampleOff);
```

### Verified Pattern: noteCount_ Deduplication for noteOff
```cpp
// Source: PluginProcessor.cpp line 968-971
if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), sampleOff);
else
    noteCount_[ch - 1][subPitch] = 0;  // clamp (defensive)
```

### Verified Pattern: looperActivePitch_ Snapshot at Looper Gate-On
```cpp
// Source: PluginProcessor.cpp line 841-848
looperActivePitch_[v] = heldPitch_[v];
const int ch = effectiveChannel(v);
looperActiveCh_[v] = ch;
sentChannel_[v] = ch;
if (noteCount_[ch - 1][looperActivePitch_[v]]++ == 0)
    midi.addEvent(juce::MidiMessage::noteOn(ch, looperActivePitch_[v], (uint8_t)100), 0);
```

### Verified Pattern: consumeRightStickTrigger (current — to be replaced)
```cpp
// Source: PluginProcessor.cpp line 491-492
if (gamepad_.consumeRightStickTrigger())
    triggerPanic();   // <-- REMOVE this entire body; replace with combo logic
```

### Verified Pattern: timerCallback HOLD Button Color
The HOLD button color pattern is the precedent for SUB8 coloring. SUB8 uses the same 30Hz timerCallback check, reading APVTS param directly and calling `setColour()` + `repaint()` if needed.

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|------------------|--------|
| Panic on R3 alone | R3 alone = no-op; R3 + held pad = SUB8 toggle | Frees R3 for musical use; panic is UI-only |
| HOLD strip = full width | HOLD strip split 50/50 HOLD/SUB8 | Adds per-voice sub-octave control without new UI real estate |

---

## Open Questions

1. **setValueNotifyingHost on audio thread for R3 combo**
   - What we know: Project already does this for gamepad octave changes (Mode 1 D-pad). No reported audio dropouts.
   - What's unclear: Whether JUCE 8 improved RT-safety of this call.
   - Recommendation: Match existing pattern. If audio dropout is observed during QA, migrate to pending-toggle atomic processed in timerCallback.

2. **allNotesOff flush covers sub pitches**
   - What we know: Phase 18 fixed allNotesOff to cover all 16 channels via CC123. This kills all notes regardless of pitch.
   - What's unclear: Whether `subHeldPitch_[]` arrays are correctly reset to -1 in the flush path so that subsequent re-triggers don't attempt a double note-off.
   - Recommendation: In the flush path (wherever `noteCount_` is reset via `resetNoteCount()`), add `subHeldPitch_[v] = -1; looperActiveSubPitch_[v] = -1; subOctSounding_[v].store(false)` for all 4 voices.

3. **Sub note velocity**
   - What we know: All main voice notes use fixed velocity 100 (`(uint8_t)100`).
   - What's unclear: Whether sub-octave should use the same velocity or a different (e.g., softer at 80) value for musical balance.
   - Recommendation: Use the same fixed 100. No new parameter needed. Matches existing pattern. If user wants adjustable velocity, that's a future feature.

---

## Sources

### Primary (HIGH confidence)
- Direct code reading: `PluginProcessor.cpp` lines 100-116 (addBool lambda), 440-492 (gamepad processBlock), 840-972 (looper gates + tp.onNote)
- `19-CONTEXT.md` — locked decisions from user discussion
- `REQUIREMENTS.md` — SUBOCT-01 through SUBOCT-04 verbatim

### Secondary (MEDIUM confidence)
- JUCE 8.0.4 API (known from project memory): `ButtonParameterAttachment`, `AudioParameterBool`, `getRawParameterValue`, `jlimit` — patterns verified by existing codebase usage

### Tertiary (LOW confidence)
- RT-safety of `setValueNotifyingHost` from audio thread: project pattern suggests acceptable, but not formally verified against JUCE 8 changelog

---

## Metadata

**Confidence breakdown:**
- APVTS param pattern: HIGH — `addBool` lambda verified at line 108; identical pattern used multiple times
- Note-on/off with snapshot: HIGH — looperActivePitch_[] pattern read directly at lines 841-858; subHeldPitch_[] is a direct copy
- Mid-note toggle detection: HIGH — logic derived from isGateOpen(v) + subOctSounding_ flag; no external unknowns
- UI layout split: HIGH — padHoldBtn_[] at resized() ~line 1796 confirmed by CONTEXT.md; split is mechanical
- R3 gamepad combo: HIGH — consumeRightStickTrigger() location confirmed at line 491; voiceHeld pattern confirmed at lines 451-461
- R3 setValueNotifyingHost thread safety: LOW — project precedent exists but JUCE 8 RT-safety not formally verified

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (stable JUCE 8 codebase; no external dependencies changing)
