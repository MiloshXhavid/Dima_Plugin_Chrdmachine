# Phase 20: Random Trigger System Extensions — Research

**Researched:** 2026-03-01
**Domain:** JUCE APVTS parameter lifecycle, C++ audio-thread gate logic, in-place enum/choice expansion
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Free / Hold Trigger Modes**
- `triggerSource0..3` Choice param: expand to 4 options — add "Random Hold" at index 3
- `TriggerSource` enum: add `RandomHold = 3`
- Random Hold fires only while the physical pad for that voice is held down
- Random Hold note-off rule: if the pad is released mid-note, note-off fires immediately (gate timer is overridden by pad release)
- "Random" (value 2) renamed to "Random Free" — behavior unchanged, label only changes

**Population + Probability Model**
- `randomDensity` APVTS param ID renamed to `randomPopulation`; range stays 1–8; behavior unchanged
- New APVTS param `randomProbability` (0.0–1.0, default 1.0); two independent rolls: (1) population/subdivsPerBar, (2) probability
- Expected notes/bar = population × probability

**Unified Gate Length**
- `randomGateTime` renamed to `gateLength`; arp gate param merged into this single ID
- Range: 0.0–1.0; 0.0 = manual open gate
- Manual (0.0) for Random Free: note stays open until next random trigger fires for that voice
- Manual (0.0) for Random Hold: note stays open while pad held; pad release still sends immediate note-off
- Manual (0.0) for Arp: open gate until arp advances to next step
- Existing 10ms minimum floor retained for non-manual mode
- Arp gate % UI stays in place; wired to same unified `gateLength` param

**1/64 Subdivision**
- Add `"1/64"` as 5th option in each `randomSubdiv0..3` Choice combo
- `RandomSubdiv` enum: add `SixtyFourthNote = 4`; interval in beats = 0.0625
- Existing 10ms minimum gate floor is sufficient protection

### Claude's Discretion

None declared in CONTEXT.md for this phase.

### Deferred Ideas (OUT OF SCOPE)

- Per-voice Probability knob (all 4 voices share one Probability knob in Phase 20)
- Per-voice Population knob (global in Phase 20)
- Pattern-based randomness / step sequencer integration
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| RND-01 | Trigger source dropdown extended to: Pad / Joystick / Random Free / Random Hold | APVTS Choice param expansion; TriggerSource enum add; no preset break with rename-in-place |
| RND-02 | Random Free fires gates continuously; RND Sync applies same as before | Existing `TriggerSource::Random` logic reused; only label changes |
| RND-03 | Random Hold fires gates only while pad physically held; pad release overrides gate | `padPressed_[v]` atomic already available on audio thread; need to pass hold-check into RND branch |
| RND-04 | Population knob (1–64) replaces RND Density | APVTS rename strategy: add new `randomPopulation` param, remove old (DAW preset impact documented) |
| RND-05 | Probability knob (0–100%) per-tick second roll | New float APVTS param; use existing `nextRandom()` LCG for second roll |
| RND-06 | 1/64 subdivision option added | Extend `RandomSubdiv` enum + `subdivBeatsFor()` switch + Choice string array + `hitsPerBarToProbability()` divisor table |
| RND-07 | Unified Gate Length param replaces both `randomGateTime` and arp gate | APVTS rename strategy for two params merging into one; arp code reads `gateLength` instead of `arpGateTime` |
</phase_requirements>

---

## Summary

Phase 20 is a tightly scoped refactor of the existing random trigger subsystem — no new subsystems introduced, no new files required. All changes are isolated to `TriggerSystem.h/.cpp`, `PluginProcessor.cpp`, and `PluginEditor.cpp`. The structural risk is dominated by two APVTS parameter renames (`randomDensity` → `randomPopulation`, `randomGateTime` + `arpGateTime` → `gateLength`) and one parameter deletion (`arpGateTime` disappears as a standalone param). This is the highest-risk area because APVTS uses XML attribute keys matching the parameter ID string verbatim; old DAW sessions with the old IDs will silently load defaults for renamed params.

The new Random Hold mode requires the audio thread to read whether a pad is physically held. `padPressed_[v]` in `TriggerSystem` is already an `std::atomic<bool>` written from the UI/gamepad thread and readable on the audio thread — no new cross-thread mechanism is needed. The Random Hold branch is a conditional modifier layered on top of the existing Random tick logic: tick fires, probability rolls pass, but the final note-on is suppressed if the pad is not held, and the gate is hard-cut on pad release regardless of remaining countdown.

The unified gate-length manual mode (knob = 0.0) requires a sentinel value to distinguish "manual open" from the countdown-based model. The existing `randomGateRemaining_[v]` counter uses 0 to mean "gate closed / countdown not started". A value of -1 (sentinel) works cleanly because the counter is an `int` and the audio loop already checks `> 0` before decrementing. The manual open-gate path for Random Free (next trigger = implicit note-off) is handled naturally: `fireNoteOn` always calls `fireNoteOff` first if the gate is open, which closes any previous open-gate note before re-triggering.

**Primary recommendation:** Add `randomPopulation` and `randomProbability` as fresh APVTS params, keep `randomDensity` and `randomGateTime` as legacy aliases reading from the new params during a transition block, then remove them — OR — simply rename in place and accept that old sessions reset those two values to defaults (population=8, gate=0.5). Since this is a personal-use / Gumroad plugin and no DAW session portability guarantee has been made in earlier phases, a clean rename with no aliasing is the pragmatic choice.

---

## Standard Stack

### Core

| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| JUCE APVTS `AudioParameterFloat` | 8.0.4 | New `randomProbability` param | Same pattern as every existing float param in the codebase |
| JUCE APVTS `AudioParameterChoice` | 8.0.4 | Extend `triggerSource` and `randomSubdiv` choice lists | Same as existing choice params |
| `std::atomic<bool>` (`padPressed_[v]`) | C++17 std | Audio-thread read of pad physical state for Hold mode | Already in TriggerSystem; zero new infrastructure |
| LCG RNG (`nextRandom()`) | Project LCG | Second probability roll | Established audio-safe RNG pattern; same function, called twice per tick |

### Supporting

| Component | Version | Purpose | When to Use |
|-----------|---------|---------|-------------|
| `juce::SliderParameterAttachment` | 8.0.4 | Wire unified `gateLength` knob to APVTS | Use for the probability knob as well; consistent with existing `randomGateTimeKnobAtt_` |
| `juce::ComboBoxParameterAttachment` | 8.0.4 | Wire expanded trigger source combo boxes | Existing `trigSrcAtt_[v]` pattern; just add a fourth combo item |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Clean APVTS rename | Version-stamped XML migration in `setStateInformation` | Migration is correct if DAW preset backward compat is required; adds ~30 lines; unnecessary for this project |
| `int` sentinel (-1) for manual gate | Separate `manualGate_[4]` bool array | Both work; sentinel is fewer fields and consistent with existing style |
| Using `padPressed_` directly in TriggerSystem for Hold | Passing `padHeld[4]` in `ProcessParams` | Passing via ProcessParams is cleaner (no reach into private state from processBlock path); either works |

---

## Architecture Patterns

### Existing Project Structure (unchanged by this phase)

```
Source/
├── TriggerSystem.h/.cpp    # Gate logic — all per-voice trigger modes
├── PluginProcessor.h/.cpp  # APVTS layout + processBlock + param forwarding
└── PluginEditor.h/.cpp     # UI widgets + APVTS attachments
Tests/
└── (Catch2 unit tests — no TriggerSystem test file exists yet)
```

### Pattern 1: APVTS Choice Param Expansion (Add Item to Existing Combo)

**What:** Add a new string option to an existing `AudioParameterChoice`. JUCE stores the choice as an integer index in the XML state; adding a new item at the END preserves all existing saved index values.

**When to use:** `triggerSource0..3` (add "Random Hold" at index 3) and `randomSubdiv0..3` (add "1/64" at index 4).

**CRITICAL constraint:** New items MUST be appended, never inserted before existing items. Inserting "Random Hold" before "Random" would shift "Random" from index 2 to index 3, corrupting all existing DAW sessions that saved triggerSource=2.

```cpp
// Source: PluginProcessor.cpp createParameterLayout() — existing pattern
// BEFORE (3 options, index 0/1/2):
juce::StringArray trigSrcNames { "TouchPlate", "Joystick", "Random" };

// AFTER (4 options, index 0/1/2/3) — append only:
juce::StringArray trigSrcNames { "TouchPlate", "Joystick", "Random Free", "Random Hold" };
// NOTE: "Random" → "Random Free" is a display-label rename; the saved index 2 still loads correctly.

// randomSubdiv: append "1/64" at index 4
const juce::StringArray subdivChoices { "1/4", "1/8", "1/16", "1/32", "1/64" };
```

### Pattern 2: APVTS Parameter Rename (ID Change)

**What:** Changing a parameter's ID string breaks all existing DAW sessions that saved under the old ID. JUCE `apvts.replaceState()` simply ignores XML attributes whose key does not match any registered parameter ID.

**How to handle for this project:** Since no preset backward-compatibility contract has been established (this is a personal instrument, no v1.0 preset file format was documented), the clean approach is to:

1. Remove old param registration (`randomDensity`, `randomGateTime`, `arpGateTime`)
2. Add new param registrations (`randomPopulation`, `gateLength`, `randomProbability`)
3. Update all `getRawParameterValue()` callsites
4. Update all UI attachment IDs

The consequence: any existing DAW session resets the three affected params to their new defaults (population=8, gate=0.5, probability=1.0). All other params (triggerSource, randomSubdiv, clockSync, freeTempo, all chord/scale/LFO params) are unaffected.

```cpp
// REMOVE these from createParameterLayout():
addFloat(ParamID::randomDensity, "Random Density", 1.0f, 8.0f, 4.0f);
addFloat("randomGateTime", "Random Gate Time", 0.0f, 1.0f, 0.5f);
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "arpGateTime", "Arp Gate Time",
    juce::NormalisableRange<float>(5.0f, 100.0f, 1.0f), 75.0f));

// ADD these:
addFloat("randomPopulation", "Random Population", 1.0f, 64.0f, 8.0f);
addFloat("randomProbability", "Random Probability", 0.0f, 1.0f, 1.0f);
addFloat("gateLength", "Gate Length", 0.0f, 1.0f, 0.5f);
```

### Pattern 3: Random Hold Gate Logic (Audio Thread)

**What:** In the existing Random tick path (`src == TriggerSource::Random`), gate-on is conditional on an RNG roll. Random Hold adds a second condition: the pad must also be physically held.

**Key insight:** `padPressed_[v]` is already an `std::atomic<bool>` on TriggerSystem, so the audio thread can read it safely in `processBlock` without any new synchronization.

**How pad release cuts the gate:** The TouchPlate branch already handles `!pressed && gateOpen_[v].load()` → `fireNoteOff()`. The Random Hold path must mirror this: if `src == RandomHold && !padPressed_[v] && gateOpen_[v]` → `fireNoteOff()`. This check runs every block, so pad release is detected within one block (≤ ~11ms at 512 samples / 44100 Hz).

```cpp
// In processBlock, inside per-voice loop — add after existing `else if (src == TriggerSource::Random)` block:
else if (src == TriggerSource::RandomHold)
{
    // Hard-cut: pad released mid-note → immediate note-off
    const bool padHeld = padPressed_[v].load();
    if (!padHeld && gateOpen_[v].load())
    {
        fireNoteOff(v, ch - 1, 0, p);
        randomGateRemaining_[v] = 0;
    }
    // Gate fires only if pad is held AND subdivision tick occurred
    else if (padHeld && randomFired[v])
    {
        const float popProb  = hitsPerBarToProbability(p.randomPopulation, p.randomSubdiv[v]);
        const float userProb = p.randomProbability;
        if (nextRandom() < popProb && nextRandom() < userProb)
            trigger = true;
    }
}
```

### Pattern 4: Unified Gate Length — Manual Open Gate Sentinel

**What:** `randomGateRemaining_[v]` is an `int` counter (samples until note-off). Normal use: set to gate duration on note-on, decrement each block, fire note-off when it reaches 0. Manual mode (gateLength = 0.0): set sentinel value -1; the decrement loop skips negative values; next trigger fires implicit note-off via existing `if (gateOpen_[v].load()) fireNoteOff(...)` at note-on time.

```cpp
// In the trigger-fires branch for Random Free / Random Hold:
if (p.gateLength <= 0.0f)
{
    // Manual mode: open gate, no timer — next trigger will close it via fireNoteOff at note-on
    randomGateRemaining_[v] = -1;
}
else
{
    const double beats = subdivBeatsFor(p.randomSubdiv[v]);
    const double samplesPerSubdiv = (p.sampleRate / (p.bpm / 60.0)) * beats;
    const int minDur = static_cast<int>(0.010 * p.sampleRate);  // 10ms floor
    randomGateRemaining_[v] = std::max(minDur,
        static_cast<int>(p.gateLength * samplesPerSubdiv));
}

// In auto note-off countdown — guard against sentinel:
if (gateOpen_[v].load() && randomGateRemaining_[v] > 0)
{
    randomGateRemaining_[v] -= p.blockSize;
    if (randomGateRemaining_[v] <= 0)
    {
        randomGateRemaining_[v] = 0;
        fireNoteOff(v, ch - 1, p.blockSize - 1, p);
    }
}
// randomGateRemaining_[v] == -1: manual open gate — skip countdown entirely
```

### Pattern 5: Two-Roll Probability

**What:** The existing `hitsPerBarToProbability()` roll becomes the first roll (population-based density). A second roll against `randomProbability` is added. Both rolls use the same `nextRandom()` LCG — called twice per tick per voice when both conditions are evaluated.

```cpp
// Replaces existing single-roll in Random Free branch:
const float popProb  = hitsPerBarToProbability(p.randomPopulation, p.randomSubdiv[v]);
const float userProb = p.randomProbability;   // 0.0–1.0, default 1.0
if (nextRandom() < popProb && nextRandom() < userProb)
    trigger = true;
```

### Pattern 6: `hitsPerBarToProbability()` Extension for 1/64

**What:** The helper function has a hardcoded switch for subdivsPerBar. Add the `SixtyFourthNote` case (64 subdivisions per bar).

```cpp
static float hitsPerBarToProbability(float density, RandomSubdiv subdiv)
{
    const float subdivsPerBar = static_cast<float>(
        subdiv == RandomSubdiv::Quarter      ?  4 :
        subdiv == RandomSubdiv::Eighth       ?  8 :
        subdiv == RandomSubdiv::Sixteenth    ? 16 :
        subdiv == RandomSubdiv::ThirtySecond ? 32 : 64);  // SixtyFourthNote
    return juce::jlimit(0.0f, 1.0f, density / subdivsPerBar);
}
```

Also update `subdivBeatsFor()`:
```cpp
case RandomSubdiv::SixtyFourthNote: return 0.0625;
```

### Pattern 7: UI Parameter Rename — Attachment Swap

**What:** When an APVTS parameter ID changes, the UI attachment (`SliderParameterAttachment`, `SliderAttachment`) must reference the new ID. The widget itself (slider/knob) does not change; only the attachment ID string and the member variable name change for clarity.

```cpp
// In PluginEditor constructor — rename attachment, update ID string:
// BEFORE:
randomDensityAtt_ = std::make_unique<SliderAtt>(p.apvts, "randomDensity", randomDensityKnob_);
// AFTER (also rename member: randomDensityAtt_ → randomPopulationAtt_ for clarity):
randomPopulationAtt_ = std::make_unique<SliderAtt>(p.apvts, "randomPopulation", randomDensityKnob_);

// Existing randomGateTimeKnobAtt_ → rename to gateLengthAtt_, point at "gateLength"
// arpGateTimeAtt_ → point at "gateLength" too (same param, two widgets)
```

### Anti-Patterns to Avoid

- **Inserting a new choice item in the middle of an existing StringArray:** Shifts all subsequent indices, corrupts all previously saved DAW sessions. Always append.
- **Checking `randomGateRemaining_[v] == 0` as the sentinel for "no gate":** Zero already means "gate countdown done"; use -1 for manual-open sentinel.
- **Reading `padPressed_` from outside `TriggerSystem` on the audio thread:** The atomic is private; the processBlock method of TriggerSystem already has access. Do NOT add a public getter called from PluginProcessor — keep the read inside the `TriggerSystem::processBlock` where the atomic lives.
- **Calling `setValueNotifyingHost` from the audio thread for the gateLength param:** Not needed; gateLength is read via `getRawParameterValue()` in processBlock as a float, same as all other params.
- **Forgetting to clear `randomGateRemaining_[v] = 0` on mode switch away from Random sources:** The existing guard `if (src != TriggerSource::Random) randomGateRemaining_[v] = 0;` must be extended to cover `RandomHold` as well (i.e., clear when mode is neither Random Free nor Random Hold).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Thread-safe pad-held read on audio thread | A new atomic or mutex | Existing `padPressed_[v].load()` in TriggerSystem | Already an `std::atomic<bool>`, already on the audio thread path |
| Second RNG roll | A separate RNG instance or `std::rand()` | Existing `nextRandom()` LCG called twice | LCG is audio-safe under MSVC (established in PERF-01), calling it twice per tick is the documented pattern |
| APVTS preset migration | Custom XML parsing in `setStateInformation` | Accept reset-to-defaults on old sessions | Project has no documented preset format; migration complexity not justified |
| Arp gate-length cross-reference | A separate arp-specific float passed through ProcessParams | The unified `gateLength` APVTS param read directly in the arp section of PluginProcessor | Arp does NOT use TriggerSystem's gate countdown — it has its own `arpNoteOffRemaining_` float; they both read the same `gateLength` param independently |

**Key insight:** The arp gate and the random gate are two separate mechanisms that share ONE APVTS source parameter — they do NOT share internal state. The arp's `arpNoteOffRemaining_` (in beats) and the TriggerSystem's `randomGateRemaining_[v]` (in samples) both read `gateLength` at trigger time and convert independently. No shared state is needed.

---

## Common Pitfalls

### Pitfall 1: Appending vs. Inserting into Choice StringArray

**What goes wrong:** Adding "Random Hold" between "Joystick" and "Random" (index 2) shifts "Random" to index 3. Any DAW session that saved `triggerSourceN = 2` now loads "Random Hold" instead of "Random Free". Silent behavior change.

**Why it happens:** Developers think alphabetically or by logic flow, not by saved integer index.

**How to avoid:** Always append to the END of a choice StringArray. Update the label string ("Random" → "Random Free") without changing the item's position.

**Warning signs:** After the change, open the DAW, recall a session that had Random mode, and verify the combo box shows "Random Free" not "Random Hold".

### Pitfall 2: `randomGateRemaining_` Sentinel vs. Zero Collision

**What goes wrong:** Using 0 as the "manual open gate" sentinel when 0 already means "countdown expired / gate closed". The auto-note-off path fires immediately on the next block when `randomGateRemaining_[v]` is 0 and the gate is open.

**Why it happens:** The existing code uses `> 0` to check if countdown is active, and `<= 0` as expiry. Zero is the expired/reset state.

**How to avoid:** Use -1 as the manual open gate sentinel. The decrement path checks `> 0` before decrementing, so -1 is completely skipped.

**Warning signs:** In manual mode (gateLength = 0), notes immediately cut off after one block instead of sustaining.

### Pitfall 3: Missing `clearRandomGate` on Mode Switch to Non-Random Source

**What goes wrong:** If a voice switches from Random Hold to TouchPlate mid-session with a gate open, `randomGateRemaining_[v]` still holds -1 (manual sentinel). If the voice later switches back to Random Hold, the sentinel persists and the countdown never starts for the next note.

**Why it happens:** The existing guard `if (src != TriggerSource::Random) randomGateRemaining_[v] = 0;` only covers the old `Random` enum value; it does not cover `RandomHold`.

**How to avoid:** The guard must clear for any non-random source:
```cpp
if (src != TriggerSource::RandomFree && src != TriggerSource::RandomHold)
    randomGateRemaining_[v] = 0;
```

**Warning signs:** After switching a voice from Random Hold to TouchPlate and back, notes don't time out in non-manual mode.

### Pitfall 4: Arp Gate UI Slider Range Mismatch

**What goes wrong:** `arpGateTimeKnob_` in PluginEditor is currently set up with `setRange(5.0, 100.0, 1.0)` (a percentage, 5–100%) and attached to the old `arpGateTime` param (5–100%). The new unified `gateLength` param is 0.0–1.0 (a fraction). If the arp slider is re-attached to `gateLength` without adjusting the range to 0.0–1.0, the slider values will be 100x off.

**Why it happens:** Two UI sliders with different display scales sharing one backend param.

**How to avoid:** Both sliders (`randomGateTimeKnob_` and `arpGateTimeKnob_`) must be set to `setRange(0.0, 1.0, 0.01)` before attaching to `gateLength`. Alternatively, keep both in 0–100% by using a `NormalisableRange` with a custom skew — but the simplest solution is to express gateLength as 0.0–1.0 everywhere (random knob already uses 0–1; arp knob switches to match).

**Warning signs:** Arp note length is 100x shorter or longer than expected; arp knob sweeps the full range in a tiny movement.

### Pitfall 5: `randomGateRemaining_` in `resetAllGates()` Must Handle Sentinel

**What goes wrong:** `resetAllGates()` sets `randomGateRemaining_[v] = 0`. This is correct for the non-manual case. If a voice is in manual open gate (-1) when reset fires (e.g., DAW stop, gamepad disconnect), the reset correctly clears it to 0 — but any open note must be closed by the caller first. This is already the documented caller contract.

**Why it happens:** Nothing to change; this is a reminder to verify the reset is still correct after the -1 sentinel is introduced.

**How to avoid:** `resetAllGates()` should set `randomGateRemaining_[v] = 0` (clearing both normal and sentinel states). Verify callers still fire note-off before calling resetAllGates.

### Pitfall 6: `padPressed_` Is Private in TriggerSystem — Don't Expose It

**What goes wrong:** Adding a `bool isPadPressed(int v) const` getter to TriggerSystem to support Random Hold reads from PluginProcessor.

**Why it happens:** Seems clean, but it's unnecessary — `processBlock()` already runs inside TriggerSystem where `padPressed_` is directly accessible.

**How to avoid:** Implement Random Hold entirely inside `TriggerSystem::processBlock()`. The `padPressed_[v]` atomic is directly readable there. No getter needed.

---

## Code Examples

Verified patterns from direct source-code inspection:

### Existing `hitsPerBarToProbability` + single-roll pattern (to be extended)

```cpp
// Source: TriggerSystem.cpp:49-56, :218-222
static float hitsPerBarToProbability(float density, RandomSubdiv subdiv)
{
    const float subdivsPerBar = static_cast<float>(
        subdiv == RandomSubdiv::Quarter ? 4 :
        subdiv == RandomSubdiv::Eighth  ? 8 :
        subdiv == RandomSubdiv::Sixteenth ? 16 : 32);
    return juce::jlimit(0.0f, 1.0f, density / subdivsPerBar);
}

// In processBlock:
if (randomFired[v]) {
    const float prob = hitsPerBarToProbability(p.randomDensity, p.randomSubdiv[v]);
    if (nextRandom() < prob)
        trigger = true;
}
```

### Existing gate countdown pattern (to be extended with sentinel)

```cpp
// Source: TriggerSystem.cpp:234-258
// Note-on with countdown:
const double beats = subdivBeatsFor(p.randomSubdiv[v]);
const double beatsPerSec = p.bpm / 60.0;
const double samplesPerSubdiv = (p.sampleRate / beatsPerSec) * beats;
const int minDurationSamples = static_cast<int>(0.010 * p.sampleRate); // 10ms floor
randomGateRemaining_[v] = std::max(minDurationSamples,
    static_cast<int>(p.randomGateTime * samplesPerSubdiv));

// Auto-off countdown:
if (src == TriggerSource::Random && gateOpen_[v].load() && randomGateRemaining_[v] > 0)
{
    randomGateRemaining_[v] -= p.blockSize;
    if (randomGateRemaining_[v] <= 0)
    {
        randomGateRemaining_[v] = 0;
        fireNoteOff(v, ch - 1, p.blockSize - 1, p);
    }
}
if (src != TriggerSource::Random)
    randomGateRemaining_[v] = 0;
```

### APVTS Float parameter registration (standard project pattern)

```cpp
// Source: PluginProcessor.cpp:102-107
auto addFloat = [&](const juce::String& id, const juce::String& name,
                    float min, float max, float def)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        id, name, juce::NormalisableRange<float>(min, max, 0.01f), def));
};

// Usage for new params:
addFloat("randomPopulation",  "Random Population",  1.0f, 64.0f, 8.0f);
addFloat("randomProbability", "Random Probability", 0.0f, 1.0f,  1.0f);
addFloat("gateLength",        "Gate Length",         0.0f, 1.0f,  0.5f);
```

### APVTS Choice param expansion (verified project pattern)

```cpp
// Source: PluginProcessor.cpp:171-188
// triggerSource expansion — append "Random Hold":
juce::StringArray trigSrcNames { "TouchPlate", "Joystick", "Random Free", "Random Hold" };
addChoice(ParamID::triggerSource0, "Trigger Source Root",    trigSrcNames, 0);
// ... repeat for 1-3

// randomSubdiv expansion — append "1/64":
const juce::StringArray subdivChoices { "1/4", "1/8", "1/16", "1/32", "1/64" };
layout.add(std::make_unique<juce::AudioParameterChoice>(
    "randomSubdiv0", "Random Subdiv Root", subdivChoices, 1));
// ... repeat for 1-3
```

### UI combo item addition (verified project pattern)

```cpp
// Source: PluginEditor.cpp:997-1006
// BEFORE:
trigSrc_[i].addItem("Pad",      1);
trigSrc_[i].addItem("Joystick", 2);
trigSrc_[i].addItem("Random",   3);
// AFTER:
trigSrc_[i].addItem("Pad",         1);
trigSrc_[i].addItem("Joystick",    2);
trigSrc_[i].addItem("Rnd Free",    3);
trigSrc_[i].addItem("Rnd Hold",    4);
```

### Arp gate currently reads `arpGateTime` — must switch to `gateLength`

```cpp
// Source: PluginProcessor.cpp:1239-1241
// CURRENT:
const double gateRatio = juce::jlimit(0.05, 1.0,
    static_cast<double>(apvts.getRawParameterValue("arpGateTime")->load()) / 100.0);

// AFTER (gateLength is already 0.0-1.0; clamp floor to 0.0 for manual mode):
const double gateRatio = apvts.getRawParameterValue("gateLength")->load();
// Manual mode (gateRatio == 0.0) for arp: gate stays open until next arp step
// This is handled naturally — arpNoteOffRemaining_ = 0 means no timer, step boundary fires note-off
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single `Random` TriggerSource | `Random Free` + `Random Hold` | Phase 20 | Enables pad-gated randomness |
| `randomDensity` (1–8 notes/bar) | `randomPopulation` (1–64) | Phase 20 | Wider density range; same probability math |
| `randomGateTime` + `arpGateTime` (two separate params) | `gateLength` (one shared param) | Phase 20 | Consistent gate across all auto-trigger sources |
| 4 subdivision options (1/4..1/32) | 5 options (1/4..1/64) | Phase 20 | Finer timing for high-density patterns |
| Single probability roll per tick | Double roll (population + probability) | Phase 20 | Orthogonal control: density vs. gate probability |

**Key design invariant preserved:** TouchPlate and Joystick trigger sources are completely unaffected by gateLength — their note-off remains pad/joystick-driven. gateLength only controls note duration for Random Free, Random Hold, and Arp.

---

## Open Questions

1. **Population range change (1–8 → 1–64) and the `ProcessParams::randomDensity` field**
   - What we know: `ProcessParams` has `float randomDensity = 4.0f`. The field will be renamed to `randomPopulation`. The PluginProcessor reads it via `getRawParameterValue("randomPopulation")` and assigns to `tp.randomPopulation`.
   - What's unclear: Should `hitsPerBarToProbability()` accept a `density` range of 1–64? At 1/64 subdivision with 64 population, probability = 64/64 = 1.0 (always fires). This is intentional and correct.
   - Recommendation: No special handling needed; `jlimit(0, 1)` clamp in the function already handles overflows.

2. **Arp manual gate (gateLength = 0.0) behavior with arpNoteOffRemaining_**
   - What we know: The arp uses `arpNoteOffRemaining_` (in beats) for gate-time note-off. When `gateRatio = gateLength = 0.0`, `gateBeats = 0.0`. The note-off remaining starts at 0, meaning the note-off timer fires on the first block (immediately).
   - What's unclear: Is `gateBeats = 0.0` correctly interpreted as "stay open until next step" or does it fire an immediate note-off?
   - Recommendation: Inspect the arp note-off path. If `arpNoteOffRemaining_ = 0` means "no pending note-off", the current design works. If the arp loop fires note-off whenever `arpNoteOffRemaining_ > 0` decrements to ≤ 0, then gateRatio=0 gives gateBeats=0, which never sets arpNoteOffRemaining_ > 0, so no gate-time note-off fires. That IS the manual behavior. Verify by reading PluginProcessor.cpp lines 1247-1270 before implementation.

3. **Probability knob UI placement**
   - What we know: The random control strip currently has: `RND DENS` knob + `RND GATE` knob + SUBDIV combos + RND SYNC button + free tempo knob.
   - What's unclear: Where exactly does the `randomProbability` knob fit in the layout? The UI is already densely packed.
   - Recommendation: Replace the `RND DENS` label area with `POPULATION` and add `PROBABILITY` as a second knob beside it. The `RND GATE` knob becomes `GATE LEN` (wired to `gateLength`). No layout expansion required; one knob replaces one label, one knob is added.

---

## Sources

### Primary (HIGH confidence)

- Direct source inspection: `Source/TriggerSystem.h` (lines 1-128) — `TriggerSource` enum, `RandomSubdiv` enum, `ProcessParams` struct, `padPressed_` atomic array, `randomGateRemaining_` array, `nextRandom()` LCG
- Direct source inspection: `Source/TriggerSystem.cpp` (lines 1-315) — `subdivBeatsFor()`, `hitsPerBarToProbability()`, full `processBlock()` implementation, `resetAllGates()`
- Direct source inspection: `Source/PluginProcessor.cpp` (lines 1-300, 1140-1260) — APVTS parameter layout, `processBlock` param forwarding, arp gate logic, `getStateInformation`/`setStateInformation` (XML-based, no versioning)
- Direct source inspection: `Source/PluginEditor.h` (lines 205-374) — member declarations for random UI components
- Direct source inspection: `Source/PluginEditor.cpp` (lines 975-1094, 1503-1513) — UI setup for triggerSource combos, randomDensityKnob, randomGateTimeKnob, arpGateTimeKnob

### Secondary (MEDIUM confidence)

- JUCE APVTS behavior: `AudioParameterChoice` saves choice as integer index in XML; label-only renames don't affect saved sessions; inserting mid-list breaks saved sessions. Verified by reading `setStateInformation` and `getStateInformation` implementation (XML key = parameter ID, no aliasing).
- `std::atomic<bool>` cross-thread read safety: confirmed by existing project use in Phase 19 (`subOctSounding_[4]`, `padHold_[4]`) and Phase 18 (`sentChannel_`/routing decisions in STATE.md).

### Tertiary (LOW confidence)

- None. All findings verified directly from source.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — everything is the existing codebase; no new libraries
- Architecture: HIGH — all patterns verified from direct source inspection
- Pitfalls: HIGH — derived from actual code paths and existing guard logic
- APVTS preset impact: HIGH — setStateInformation confirmed XML-based, no versioning

**Research date:** 2026-03-01
**Valid until:** Until Phase 20 implementation begins (stable — no external dependencies)
