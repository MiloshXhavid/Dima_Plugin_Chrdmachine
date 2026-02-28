# Phase 18: Single-Channel Routing — Research

**Researched:** 2026-02-28
**Domain:** JUCE MIDI routing, APVTS parameter change detection, note deduplication, ComboBox UI show/hide
**Confidence:** HIGH (all key findings verified against live codebase; no external libraries needed)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

1. **UI Layout**: ComboBox labeled `Routing:` with `Multi-Channel` / `Single Channel` options. Placed in a separate labeled panel directly under the quantize/trigger section (not bottom-right column). Multi-Channel shows four per-voice ch dropdowns (voiceCh0..3). Single Channel hides those four and shows one `Channel:` ComboBox (1-16).
2. **Note deduplication**: `noteCount_[16][128]` reference counter (audio-thread only). note-on increments, emits only if count was 0. note-off decrements, emits only if count reaches 0.
3. **sentChannel_[4]** snapshot per voice: stores `effectiveChannel(v)` at note-on so note-off always sends to the correct channel even if the setting changed mid-hold.
4. **Filter CC**: `effectiveChannel()` also overrides `filterMidiCh` in Single Channel mode. The filterMidiCh UI selector stays visible and editable; its value is ignored for CC emission while singleChanMode is true.
5. **Mode switch flush**: Toggle (either direction) → allNotesOff all 16 channels. Target change while in Single mode → allNotesOff on old target channel only.
6. **Looper stores no channel**: channel resolved at emission time via `effectiveChannel()`. No changes needed to LooperEngine.
7. **New APVTS params**: `singleChanMode` (bool, default false) + `singleChanTarget` (int 1-16, default 1).
8. **Change detection for flush triggers**: Detected in processBlock by comparing previous-value snapshot to current APVTS value.

### Claude's Discretion

- Exact placement of the Routing panel within the left column (within the constraint: under the quantize/trigger section).
- Whether `effectiveChannel()` is a method on PluginProcessor or a lambda/inline block in processBlock.

### Deferred Ideas (OUT OF SCOPE)

- Per-voice channel memory in looper events (explicitly rejected — channel always resolved at emission time).
- Any additional routing modes beyond Multi-Channel and Single Channel.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| ROUT-01 | User can select Multi-Channel or Single Channel routing via a dropdown | UI show/hide pattern, APVTS bool param + ComboBox attachment |
| ROUT-02 | In Single Channel mode, user can select target MIDI channel (1-16) | ComboBox int param attachment, conditional visibility |
| ROUT-03 | Same-pitch note collisions in Single Channel mode handled via reference counting | noteCount_[16][128] pattern, note-on/off gate logic |
| ROUT-04 | In Single Channel mode, CC74 and CC71 messages go to selected channel | effectiveChannel() overrides filterMidiCh at emission site |
| ROUT-05 | Looper playback in Single Channel mode uses current channel (live setting) | No LooperEngine changes needed; effectiveChannel() called at emission |
</phase_requirements>

---

## Summary

Phase 18 is a pure C++ refactor + UI addition inside the existing JUCE codebase. No new external libraries are required. The core work is: (1) add two APVTS parameters, (2) introduce an `effectiveChannel(v)` helper that all seven emission sites call instead of reading `voiceChs[v]` directly, (3) add `noteCount_[16][128]` + `sentChannel_[4]` to PluginProcessor, (4) detect mode-switch and target-change in processBlock via previous-value snapshots (the project's established pattern), and (5) add UI ComboBoxes with APVTS attachments and a listener-driven show/hide.

All JUCE API calls are verified against the live codebase. The `noteCount_[16][128]` pattern (2048 ints = 8 KB) is audio-thread-only and requires no synchronization beyond the fact that processBlock is single-threaded. The UI show/hide pattern is already established in the project via `lfoXEnabledBtn_`-driven visibility toggles in timerCallback.

**Primary recommendation:** implement `effectiveChannel()` as a private inline method on PluginProcessor that reads `singleChanMode` and `singleChanTarget` from APVTS raw pointers cached at the top of processBlock. All seven `voiceChs[v]` read sites become one-line call replacements.

---

## Standard Stack

### Core
| Component | Source | Purpose | Why Standard |
|-----------|--------|---------|--------------|
| `juce::MidiMessage::allNotesOff(ch)` | JUCE core | CC123 all-notes-off on a channel | Already used at lines 690, 756, 1152 |
| `juce::MidiMessage::noteOn/noteOff` | JUCE core | MIDI note events | Already used throughout |
| `juce::AudioParameterBool` | JUCE APVTS | singleChanMode parameter | Pattern: `addBool(id, name, false)` — identical to arpEnabled, lfoXEnabled |
| `juce::AudioParameterInt` | JUCE APVTS | singleChanTarget parameter | Pattern: `addInt(id, name, 1, 16, 1)` — identical to voiceCh0..3 |
| `juce::ComboBox` + `ComboBoxAttachment` | JUCE GUI | Routing dropdown + target dropdown | Already used for loopSubdivBox_, scalePresetBox_, trigSrc_[4] |
| `juce::TextButton` + `ButtonAttachment` | JUCE GUI | Alternative for singleChanMode | Already used for arpEnabledBtn_, lfoXEnabledBtn_ |

### Supporting
| Component | Source | Purpose | When to Use |
|-----------|--------|---------|-------------|
| `juce::ComboBoxParameterAttachment` | JUCE GUI | Bi-directional ComboBox ↔ APVTS | When ComboBox needs to survive preset load/save (required here) |
| `juce::ButtonParameterAttachment` | JUCE GUI | Bi-directional ToggleButton ↔ APVTS | If singleChanMode is implemented as a ToggleButton instead of ComboBox index |

**Installation:** No new packages. All in existing JUCE 8.0.4 FetchContent dependency.

---

## Architecture Patterns

### Recommended Project Structure Changes

```
PluginProcessor.h
  + private: int noteCount_[16][128] = {};   // audio-thread only
  + private: int sentChannel_[4] = {1,2,3,4}; // channel snapshot per voice
  + private: int prevSingleChanMode_ = 0;     // change detection
  + private: int prevSingleChanTarget_ = 1;   // change detection
  + private: int effectiveChannel(int voice) const; // helper declaration

PluginProcessor.cpp
  + ParamID: singleChanMode, singleChanTarget in namespace
  + createParameterLayout(): addBool + addInt for new params
  + processBlock(): cache singleMode/singleTarget at top; detect changes; update noteCount_
  + effectiveChannel() implementation
  + All 7 emission sites: voiceChs[v] → effectiveChannel(v) or sentChannel_[v]

PluginEditor.h
  + juce::ComboBox routingModeBox_
  + juce::ComboBox singleChanTargetBox_
  + juce::ComboBox voiceChBox_[4]         // NEW — per-voice channel dropdowns
  + std::unique_ptr<ComboAtt> routingModeAtt_
  + std::unique_ptr<ComboAtt> singleChanTargetAtt_
  + std::unique_ptr<ComboAtt> voiceChAtt_[4]
  + juce::Rectangle<int> routingPanelBounds_

PluginEditor.cpp
  + constructor: create + attach routingModeBox_, singleChanTargetBox_, voiceChBox_[4]
  + resized(): place routing panel below trigger section
  + timerCallback(): show/hide voiceChBox_[4] and singleChanTargetBox_ based on routingModeBox_ value
```

### Pattern 1: effectiveChannel() Helper

**What:** A private const method that centralises the single-vs-multi decision. All emission sites call it instead of reading voiceChs[v] directly.

**When to use:** Every MIDI note-on, note-off, allNotesOff, and CC emission. The `filterMidiCh` case uses `v = -1` or a separate `effectiveFilterChannel()` — or inline in the filter block.

**Example:**
```cpp
// PluginProcessor.h (private):
// Returns 1-based MIDI channel for voice v (0-3).
// In Single Channel mode, always returns singleChanTarget.
// In Multi-Channel mode, returns voiceChs_[v] (cached at top of processBlock).
// NOTE: voiceChs_ cache is block-local in processBlock; this method reads
// from cached int[4] that is passed by pointer or captured in a lambda.
// Simpler alternative: inline in processBlock as a lambda.
```

```cpp
// In processBlock, after reading APVTS params:
const bool singleMode = (*apvts.getRawParameterValue("singleChanMode") > 0.5f);
const int  singleTarget = (int)*apvts.getRawParameterValue("singleChanTarget");

// voiceChs[4] already computed at line 671-677 of current code.
// effectiveChannel lambda (captures singleMode, singleTarget, voiceChs):
auto effectiveChannel = [&](int v) -> int {
    return singleMode ? singleTarget : voiceChs[v];
};
// For filter CC:
auto effectiveFilterChannel = [&]() -> int {
    return singleMode ? singleTarget
                      : (int)apvts.getRawParameterValue(ParamID::filterMidiCh)->load();
};
```

### Pattern 2: Change Detection via Previous-Value Snapshot

**What:** Compare current APVTS value to a stored previous value at the top of processBlock. Fire flush when they differ. This is the project's established pattern (see `prevXMode_`, `prevYMode_`, `prevArpOn_` in PluginProcessor.cpp lines 1187, 969).

**When to use:** Detecting `singleChanMode` toggle and `singleChanTarget` change for flush.

**Example:**
```cpp
// At top of processBlock, after computing singleMode and singleTarget:
{
    const bool modeChanged   = (singleMode ? 1 : 0) != prevSingleChanMode_;
    const bool targetChanged = singleMode && (singleTarget != prevSingleChanTarget_);

    if (modeChanged)
    {
        // Nuclear flush: allNotesOff all 16 channels
        for (int ch = 1; ch <= 16; ++ch)
            midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
        trigger_.resetAllGates();
        looperActivePitch_.fill(-1);
        std::fill(std::begin(noteCount_), std::end(noteCount_), 0);  // reset refcounts
        prevSingleChanMode_ = singleMode ? 1 : 0;
        prevSingleChanTarget_ = singleTarget;
    }
    else if (targetChanged)
    {
        // Targeted flush: allNotesOff on the old target channel only
        midi.addEvent(juce::MidiMessage::allNotesOff(prevSingleChanTarget_), 0);
        std::fill(std::begin(noteCount_[prevSingleChanTarget_ - 1]),
                  std::end  (noteCount_[prevSingleChanTarget_ - 1]), 0);
        prevSingleChanTarget_ = singleTarget;
    }
}
```

**IMPORTANT:** Place this change-detection block BEFORE the midiMuted_ early-return guard, because the flush must reach the synth even when muted. The existing panic block at line 749 already demonstrates this pattern — it sends allNotesOff before the mute gate.

### Pattern 3: noteCount_[16][128] Reference Counting

**What:** 2D array tracking how many active note-ons have been sent for each (channel, pitch) pair. Completely audio-thread-only — no atomic or mutex needed.

**When to use:** Every note-on and note-off emission in Single Channel mode. In Multi-Channel mode the counter is irrelevant (each voice is on a different channel) but costs nothing to maintain — the counts will always be 0 or 1 per channel since pitches can't collide when each voice has its own channel.

**Example:**
```cpp
// At note-on (in tp.onNote lambda, isOn == true):
const int ch = effectiveChannel(voice);      // 1-based
const int p  = pitch;                        // 0-127
sentChannel_[voice] = ch;                    // snapshot for matching note-off
if (noteCount_[ch - 1][p]++ == 0)           // first voice on this pitch
    midi.addEvent(juce::MidiMessage::noteOn(ch, p, (uint8_t)100), sampleOff);
// else: another voice is already holding this pitch — suppress duplicate noteOn

// At note-off (in tp.onNote lambda, isOn == false):
const int ch = sentChannel_[voice];         // use snapshot, not current effectiveChannel
const int p  = pitch;
if (--noteCount_[ch - 1][p] <= 0)          // last voice released this pitch
{
    noteCount_[ch - 1][p] = 0;             // clamp to prevent negative (defensive)
    midi.addEvent(juce::MidiMessage::noteOff(ch, p, (uint8_t)0), sampleOff);
}
```

**Important: also apply noteCount gating to looper gate emissions** (lines 769-787) and arp note-on/off (lines 1062-1120). The sentChannel_ snapshot must be stored at every note-on path across all sources (live pad, looper, arp).

For looper, store in `looperActivePitch_` (already exists) and a parallel `looperActiveCh_[4]` array (new — mirrors sentChannel_ for looper-originated notes).

### Pattern 4: UI Show/Hide via timerCallback

**What:** The project's established pattern for conditional visibility is to check state in `timerCallback()` and call `setVisible()`. This avoids APVTS listener threading complexity.

**When to use:** Show/hide the four voiceChBox_[v] dropdowns vs. singleChanTargetBox_ based on current routingModeBox_ selection.

**Example (timerCallback):**
```cpp
// In PluginEditor::timerCallback():
const bool isSingle = (*proc_.apvts.getRawParameterValue("singleChanMode") > 0.5f);
singleChanTargetBox_.setVisible(isSingle);
for (int v = 0; v < 4; ++v)
    voiceChBox_[v].setVisible(!isSingle);
```

**Note:** `timerCallback` runs on the message thread at 30 Hz — no threading issues. The APVTS raw pointer is safe to read from message thread (atomic float). This approach avoids needing a separate `juce::AudioProcessorValueTreeState::Listener` subclass for the visibility switch.

**Alternative (also valid):** Attach a `juce::ValueTree::Listener` or `ComboBox::onChange` callback on the routing dropdown. The timerCallback approach is simpler and consistent with how the project currently drives UI state (see `arpCurrentVoice_` UI updates in timerCallback).

### Pattern 5: singleChanMode as a ComboBox Index Parameter

**What:** `singleChanMode` is declared as a `bool` (`AudioParameterBool`) in APVTS. In the UI it is represented by a ComboBox with two items. This requires using `ComboBox` + a **manual onChange handler** rather than `ComboBoxParameterAttachment` (which expects an int/choice param, not a bool param).

**Alternative approach:** Declare `singleChanMode` as `AudioParameterInt` (0=Multi, 1=Single) or `AudioParameterChoice`. Then a standard `ComboBoxParameterAttachment` works directly.

**Recommendation:** Declare `singleChanMode` as `AudioParameterChoice` with options `{"Multi-Channel", "Single Channel"}`. This lets `ComboBoxParameterAttachment` work directly, consistent with trigSrc_[4] and loopSubdivBox_ patterns. In processBlock, read as `(*apvts.getRawParameterValue("singleChanMode") > 0.5f)` — the raw float will be 0.0 or 1.0 for a 2-choice Choice param, identical to a bool param.

```cpp
// In createParameterLayout():
juce::StringArray routingModes { "Multi-Channel", "Single Channel" };
addChoice("singleChanMode", "Routing Mode", routingModes, 0);   // 0 = Multi-Channel default
addInt   ("singleChanTarget", "Single Channel Target", 1, 16, 1);
```

### Anti-Patterns to Avoid

- **Calling `setValueNotifyingHost` from processBlock for singleChanMode**: Never use this to trigger flush. Detection via previous-value snapshot is audio-safe and consistent with the project pattern.
- **Reading APVTS params individually at every emission site**: Cache singleMode/singleTarget once at the top of processBlock into block-local variables, then pass to effectiveChannel lambda. APVTS raw pointer loads have negligible cost but scattered reads make the code harder to audit.
- **Storing channel in looper events**: Explicitly locked out in CONTEXT.md. Channel is always resolved at emission time.
- **Calling setVisible from paint() or resized()**: Always drive visibility changes from timerCallback or an onChange handler, never from the layout pass.
- **Decrementing noteCount_ without clamping**: A mismatched note-off (e.g., from DAW transport restart) can drive the counter negative, causing note-on suppression. Always clamp to 0 at decrement.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| APVTS ↔ ComboBox sync | Manual onChange + setValueNotifyingHost wiring | `ComboBoxParameterAttachment` | Handles preset save/load, undo, host automation correctly |
| Bool UI toggle | Manual ToggleButton state sync | `ButtonParameterAttachment` (if using TextButton) | Same as above |
| All-notes-off MIDI message | Manual CC123 construction | `juce::MidiMessage::allNotesOff(ch)` | Returns correctly formatted CC123 message — already used at 3 sites |
| MIDI note dedup state reset | Custom reset logic per site | Single `std::fill` + `noteCount_` clear in flush block | Centralised reset prevents per-site omissions |

---

## Common Pitfalls

### Pitfall 1: mode-switch flush arrives after the mute gate

**What goes wrong:** The midiMuted_ guard at line 766 causes an early return. If the flush block is placed after this guard, allNotesOff never reaches the synth when muted.

**Why it happens:** The mute guard was added for the panic button use-case. The mode-switch flush is a different code path that must also bypass mute.

**How to avoid:** Place the change-detection / flush block before line 766 (`if (midiMuted_) return`). The existing panic block at line 749 already demonstrates this placement.

**Warning signs:** Stuck notes after toggling routing mode while muted.

---

### Pitfall 2: note-off uses current effectiveChannel() instead of sentChannel_ snapshot

**What goes wrong:** User holds a pad in Single Channel mode on ch5, then changes target to ch7 before releasing. The note-off fires on ch7 (the new channel), leaving a stuck note on ch5.

**Why it happens:** effectiveChannel(v) returns the current setting; it has no memory of what channel was used at note-on.

**How to avoid:** At every note-on: `sentChannel_[v] = effectiveChannel(v)`. At note-off: use `sentChannel_[v]`, not `effectiveChannel(v)`. The flush (targetChanged path) handles the in-flight notes via allNotesOff on the old channel.

**Warning signs:** Stuck notes after changing target channel while a pad is held.

---

### Pitfall 3: looper gate-off uses wrong channel

**What goes wrong:** Looper emits noteOff at line 784 using `voiceChs[v]` (currently). After refactor this becomes `effectiveChannel(v)`. But `effectiveChannel(v)` reflects the CURRENT setting, which may differ from when gateOn fired.

**How to avoid:** Add `looperActiveCh_[4]` (a new int[4] array in PluginProcessor, audio-thread-only) that snapshots `effectiveChannel(v)` at looper gateOn time, exactly like `looperActivePitch_[v]`. Use `looperActiveCh_[v]` for gateOff.

**Warning signs:** Stuck looper notes after changing channels mid-playback.

---

### Pitfall 4: arp note-off uses wrong channel

**What goes wrong:** The arp keeps `arpActivePitch_` and `arpActiveVoice_` for its pending note-off. If the channel changes between arp note-on and its gate-time note-off, the note-off goes to the wrong channel.

**How to avoid:** Add `arpActiveCh_` (int, audio-thread-only) that snapshots `effectiveChannel(arpActiveVoice_)` at the arp note-on. Use `arpActiveCh_` for all arp note-offs (lines 862, 990, 1025, 1068).

**Warning signs:** Stuck notes after changing routing mode or target channel while arp is sounding.

---

### Pitfall 5: noteCount_ not reset on DAW transport stop or panic

**What goes wrong:** `noteCount_[ch][pitch]` retains a non-zero count after DAW stop or panic fires allNotesOff. Next note-on is suppressed because the counter says a note is already active (even though allNotesOff cleared it on the synth).

**How to avoid:** In every allNotesOff flush path (DAW stop at line 690, panic at line 751, pendingAllNotesOff_ at line 1152, looper stop at line 724), also call `std::fill(&noteCount_[0][0], &noteCount_[0][0] + 16*128, 0)`. Add a helper: `void resetNoteCount() { std::fill(&noteCount_[0][0], &noteCount_[16][0], 0); }`.

**Warning signs:** Notes silently missing after a transport restart or panic event.

---

### Pitfall 6: bypass mode (processBlockBypassed) uses old voiceChs

**What goes wrong:** `processBlockBypassed` (lines 337-346) reads `voiceChs[v]` directly. After the refactor these won't go through `effectiveChannel()`.

**How to avoid:** The bypass path is simple — it only needs to emit note-offs for active pitches. It should also use `sentChannel_[v]` snapshots (or rebuild effectiveChannel inline) to match what channel the note was originally sent on. The cleanest fix: in processBlockBypassed, read `singleChanMode` and `singleChanTarget` and apply the same lambda logic.

---

### Pitfall 7: ComboBox index vs parameter value off-by-one

**What goes wrong:** `ComboBoxParameterAttachment` maps ComboBox items (1-based by default in JUCE: first item added with `addItem("...", 1)`) to the underlying parameter value. For a `Choice` parameter this is 0-based internally. Mismatched indexing causes the displayed item to be off by one from the stored value.

**How to avoid:** Use `addChoice()` consistently (as done for trigSrc_, loopSubdiv_, etc.). The JUCE Choice parameter stores a 0-based index internally. The ComboBoxParameterAttachment handles the mapping automatically when items are added with IDs 1..N. Never add items manually with `comboBox.addItem("name", id)` and then attach — always add items first, then create the attachment. The project pattern (e.g., trigSrc_[i] with addItem calls before `trigSrcAtt_[i] = make_unique<ComboAtt>(...)`) is correct.

---

## Code Examples

Verified patterns from the live codebase:

### Adding a Choice parameter (createParameterLayout)
```cpp
// Source: PluginProcessor.cpp lines 212-215, pattern matches trigSrc_ params
juce::StringArray routingModes { "Multi-Channel", "Single Channel" };
addChoice("singleChanMode",   "Routing Mode",         routingModes, 0);
addInt   ("singleChanTarget", "Single Channel Target", 1, 16, 1);
```

### Reading bool/choice param on audio thread (processBlock)
```cpp
// Source: PluginProcessor.cpp line 835 (arpEnabled), line 562 (lfoXEnabled)
const bool singleMode   = (*apvts.getRawParameterValue("singleChanMode") > 0.5f);
const int  singleTarget = (int)*apvts.getRawParameterValue("singleChanTarget");
```

### allNotesOff on a single channel
```cpp
// Source: PluginProcessor.cpp line 690, 756
midi.addEvent(juce::MidiMessage::allNotesOff(ch),  0);  // ch is 1-based
```

### allNotesOff nuclear flush (all 16 channels)
```cpp
// Source: PluginProcessor.cpp lines 752-757 (panic block)
for (int ch = 1; ch <= 16; ++ch)
    midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
```

### ComboBox + ComboBoxAttachment in PluginEditor constructor
```cpp
// Source: PluginEditor.cpp lines 986-993 (trigSrc_ pattern)
routingModeBox_.addItem("Multi-Channel",  1);
routingModeBox_.addItem("Single Channel", 2);
styleCombo(routingModeBox_);
addAndMakeVisible(routingModeBox_);
routingModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "singleChanMode", routingModeBox_);

singleChanTargetBox_.addItem("Ch 1",  1);  // ... through Ch 16
// (loop for 1-16)
styleCombo(singleChanTargetBox_);
addAndMakeVisible(singleChanTargetBox_);
singleChanTargetAtt_ = std::make_unique<ComboAtt>(p.apvts, "singleChanTarget", singleChanTargetBox_);
```

### Conditional visibility in timerCallback
```cpp
// Source: pattern from timerCallback (PluginEditor.cpp) — modelled on existing visibility logic
const bool isSingle = (*proc_.apvts.getRawParameterValue("singleChanMode") > 0.5f);
singleChanTargetBox_.setVisible(isSingle);
for (int v = 0; v < 4; ++v)
    voiceChBox_[v].setVisible(!isSingle);
// Also hide/show voiceChLabel_[v] labels if added
```

### noteCount reference counter — note-on gate
```cpp
// Audio-thread only, no mutex needed
const int ch = effectiveChannel(voice);   // 1-based
sentChannel_[voice] = ch;
if (noteCount_[ch - 1][pitch]++ == 0)    // was zero before increment
    midi.addEvent(juce::MidiMessage::noteOn(ch, pitch, (uint8_t)100), sampleOff);
```

### noteCount reference counter — note-off gate
```cpp
const int ch = sentChannel_[voice];      // snapshot from note-on
if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), sampleOff);
else
    noteCount_[ch - 1][pitch] = 0;       // clamp (defensive against mismatched events)
```

### Previous-value snapshot change detection (established pattern)
```cpp
// Source: PluginProcessor.cpp lines 1187-1188 (xMode/yMode change detection)
if (singleMode != (prevSingleChanMode_ != 0))
{
    // mode toggled — nuclear flush
    for (int ch = 1; ch <= 16; ++ch)
        midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
    resetNoteCount();
    prevSingleChanMode_ = singleMode ? 1 : 0;
    prevSingleChanTarget_ = singleTarget;
}
else if (singleMode && singleTarget != prevSingleChanTarget_)
{
    // target changed — targeted flush on old channel
    midi.addEvent(juce::MidiMessage::allNotesOff(prevSingleChanTarget_), 0);
    std::fill(std::begin(noteCount_[prevSingleChanTarget_ - 1]),
              std::end  (noteCount_[prevSingleChanTarget_ - 1]), 0);
    prevSingleChanTarget_ = singleTarget;
}
```

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|------------------|--------|
| Per-voice MIDI channel routing (voice 0=ch1, 1=ch2...) | Single-channel collapse via effectiveChannel() helper | Paraphonic/mono synths now playable without DAW channel splitting |
| Direct `voiceChs[v]` read at each emission site | Centralised `effectiveChannel(v)` capture | 7 sites unified; mode logic in one place |
| No note deduplication (each voice is on its own channel, no collision possible) | noteCount_[16][128] refcounting | Collision-safe on any channel topology |

---

## Research Question Answers

### Q1: Correct JUCE pattern for detecting APVTS parameter changes in processBlock

**Answer (HIGH confidence):** Poll with previous-value snapshot. This is the project's established and verified pattern. See `prevXMode_` / `prevYMode_` at PluginProcessor.cpp:1187-1188, `prevArpOn_` at line 969, `prevLooperWasPlaying_` at line 718. A JUCE `AudioProcessorValueTreeState::Listener` fires on the message thread, not the audio thread — it cannot be used to trigger audio-thread flushes directly. The correct audio-thread pattern is: read raw param, compare to previous-block value, act on change, update previous value.

### Q2: Is noteCount_[16][128] the right data structure?

**Answer (HIGH confidence):** Yes. 16 channels × 128 pitches × sizeof(int) = 8 KB. Zero synchronization cost (audio-thread-only). Zero dynamic allocation. `std::fill` reset is O(2048) — negligible in processBlock context. The only enhancement needed is defensive clamping at decrement (see Pitfall 5). No lighter alternative offers the same completeness; the alternatives (bitset per channel, hash map) all have worse access patterns or miss the reference-counting requirement.

### Q3: JUCE ComboBox show/hide based on another ComboBox selection

**Answer (HIGH confidence):** Use `timerCallback()` to poll APVTS state and call `setVisible()`. This is the simplest approach consistent with the project's existing timerCallback-driven UI update pattern. The alternative (ComboBox::onChange lambda + setVisible) also works and gives immediate response, but requires care about calling setVisible during resized() — the timerCallback path avoids this. If instant response is needed (no 33ms lag), add an `onChange` callback on `routingModeBox_` that calls `setVisible` and `repaint`.

### Q4: JUCE APVTS gotchas with bool params for singleChanMode

**Answer (HIGH confidence):** Use `AudioParameterChoice` (not `AudioParameterBool`) if you want a ComboBox with two labeled items. `AudioParameterBool` + `ComboBoxParameterAttachment` does not work reliably in JUCE 8 — ComboBoxParameterAttachment expects a ranged param whose 0-1 float maps to item indices. `ButtonParameterAttachment` works with `AudioParameterBool` + a `TextButton` (project uses this for arpEnabledBtn_, lfoXEnabledBtn_). Since the CONTEXT.md spec says "a ComboBox dropdown labeled `Routing:`", the correct implementation is `addChoice("singleChanMode", ..., {"Multi-Channel","Single Channel"}, 0)` + `ComboBoxParameterAttachment`. Reading as a bool in processBlock: `(*raw > 0.5f)` works for both Bool and Choice-with-2-items.

### Q5: Exact MidiMessage API calls for allNotesOff in JUCE

**Answer (HIGH confidence — verified against live codebase):**
```cpp
// Single channel (ch is 1-based):
juce::MidiMessage::allNotesOff(ch)
// This creates a CC123 (All Notes Off) message.
// Used at PluginProcessor.cpp lines 690, 756, 1152.

// Nuclear flush (all 16 channels):
for (int ch = 1; ch <= 16; ++ch)
    midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
// Used at PluginProcessor.cpp lines 752-756 (panic block).
```
The panic block also sends CC120 (all sound off) and CC64=0 (sustain off). For mode-switch flush, `allNotesOff` alone is sufficient per CONTEXT.md decision ("nuclear option — foolproof").

### Q6: Edge cases the standard refcount pattern misses

**Answer (HIGH confidence from codebase analysis):**

1. **DAW transport restart / allNotesOff flush**: `noteCount_` must be reset to all zeros whenever allNotesOff is sent. Failure to do so causes permanent note-on suppression on that (ch, pitch) pair. Fix: call `resetNoteCount()` in every flush path.

2. **Plugin bypass** (`processBlockBypassed`): This sends note-offs but does not go through the normal note-on path that sets `sentChannel_`. Fix: use `sentChannel_[v]` in the bypass path, or simply send note-offs on all 16 channels (safe overkill in bypass).

3. **Looper stop while notes sounding**: The existing looper-stop detection block (lines 718-731) sends note-offs using `voiceChs[v]`. After refactor this must use `looperActiveCh_[v]` snapshots, not `effectiveChannel(v)` (which may have changed since gateOn). The `looperActivePitch_` + new `looperActiveCh_` pair is the correct approach.

4. **Arp mid-note channel change**: `arpActivePitch_` + `arpActiveVoice_` exist but no `arpActiveCh_`. Must add `int arpActiveCh_ = 1` to processor, snapshot at arp note-on, use for all arp note-offs.

5. **Looper reset (seek to bar 0)**: The looper reset block at lines 734-744 uses `voiceChs[v]` — needs `looperActiveCh_[v]` after refactor.

6. **pendingAllNotesOff_ (gamepad disconnect)**: Line 1151-1153 sends allNotesOff using `voiceChs[v]` — needs to be replaced with all-16-channel flush (or effectiveChannel for all 4 voices) to be correct in Single mode, plus `resetNoteCount()`.

---

## Open Questions

1. **voiceChBox_ label UI real estate**: The left column is 448px wide and the trigger section already uses all four columns. The routing panel below it needs to fit 4 per-voice ch dropdowns (labeled "Root ch:", "Third ch:", "Fifth ch:", "Tension ch:") in the same 4-column grid. This is confirmed feasible at 112px per column — the same layout as trigSrc_[4]. The required height is approximately 14px (label) + 22px (dropdown) = 36px per row, one row for labels + one row for the 4 dropdowns. Total panel height: ~80px (header label + mode dropdown + voice ch row).

2. **sentChannel_ initialization**: `sentChannel_[4]` should be initialised to `{1, 2, 3, 4}` (the multi-channel defaults) to match the APVTS defaults for voiceCh0..3. A zero-initialised sentChannel_ would cause note-offs on ch0 (invalid) before any note-on has fired.

---

## Sources

### Primary (HIGH confidence)
- Live codebase: `Source/PluginProcessor.cpp` — all API calls verified against working implementation
- Live codebase: `Source/PluginEditor.cpp` / `PluginEditor.h` — UI patterns confirmed
- JUCE 8.0.4 (FetchContent in CMakeLists.txt) — library version confirmed
- `18-CONTEXT.md` — all locked decisions read verbatim

### Secondary (MEDIUM confidence)
- JUCE forum consensus (training data, corroborated by codebase patterns): `ComboBoxParameterAttachment` + `AudioParameterChoice` is the standard pattern for labeled ComboBox params. `ButtonParameterAttachment` + `AudioParameterBool` is the standard pattern for toggle buttons.

### Tertiary (LOW confidence — not needed, covered by codebase evidence)
- None required; all findings grounded in the existing working implementation.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs verified in live codebase
- Architecture: HIGH — all patterns derived from existing working code in the same file
- Pitfalls: HIGH (for pitfalls 1-5) / MEDIUM (for pitfall 7) — 1-5 derived from code analysis, 7 from JUCE ComboBox/Choice param behaviour observed in project
- Note dedup pattern: HIGH — derived from CONTEXT.md spec + standard refcount logic
- UI show/hide: HIGH — timerCallback pattern is the project's established approach

**Research date:** 2026-02-28
**Valid until:** 2026-04-28 (stable JUCE 8 API, no external dependencies)
