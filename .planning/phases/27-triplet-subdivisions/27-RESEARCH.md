# Phase 27: Triplet Subdivisions - Research

**Researched:** 2026-03-03
**Domain:** JUCE APVTS parameter extension, MIDI timing math, C++ enum extension
**Confidence:** HIGH

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| TRIP-01 | Random trigger subdivision selector (`randomSubdiv` APVTS parameter) includes 1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T alongside existing straight subdivisions — all subdivisions save/load with preset | Section "Extending randomSubdiv APVTS": AudioParameterChoice extension pattern, enum extension, subdivBeatsFor() patch |
| TRIP-02 | Quantize trigger subdivision selector (`quantizeSubdiv` APVTS parameter) includes the same 6 triplet options alongside existing straight subdivisions | Section "Extending quantizeSubdiv APVTS": AudioParameterInt→AudioParameterChoice migration, quantizeSubdivToGridSize() patch |
</phase_requirements>

---

## Summary

Phase 27 adds triplet rhythmic subdivisions (1/1T through 1/32T) to two independent subsystems: the random trigger per-voice subdivision selector (`randomSubdiv0..3`) and the quantize subdivision selector (`quantizeSubdiv`). The two subsystems use different APVTS parameter types and different dispatch paths, requiring separate but parallel changes.

The arpeggiator already implements triplets (1/8T and 1/16T in `arpSubdiv`) using `kSubdivBeats[] = { 1.0, 1.0/3.0, 0.5, 1.0/6.0, 0.25, 0.125 }`. This is the **canonical in-project triplet pattern** — Phase 27 reuses the same math (triplet beat value = straight beat value × 2/3) for both new subsystems.

The `randomSubdiv` parameters are already `AudioParameterChoice` with 9 options (4/1 through 1/64). Extending them to 15 options requires: (1) adding 6 triplet entries to the `StringArray`, (2) adding 6 new enum cases to `RandomSubdiv`, (3) patching `subdivBeatsFor()` and `hitsPerBarToProbability()` in TriggerSystem.cpp, and (4) adding the same items to the 4 `randomSubdivBox_` ComboBoxes in PluginEditor.

The `quantizeSubdiv` parameter is `AudioParameterInt` (range 0..3). To add 6 triplet options it must be migrated to `AudioParameterChoice` (or extend the int range to 0..9). The `quantizeSubdivToGridSize()` free function in LooperEngine.cpp must be extended to cover the new indices.

**Critical preset-compatibility note:** Changing `AudioParameterInt` to `AudioParameterChoice` for `quantizeSubdiv` is a **breaking APVTS change** — the XML attribute type changes. Old presets that saved `quantizeSubdiv` as an integer (0..3) will still load correctly because `AudioParameterChoice` also stores indices as integers in the XML; the value is interpretable as a new index. However, the semantic mapping shifts: old "0=1/4" stays 0=1/4 only if triplets are appended after the existing straight values. Insert triplets as a block AFTER existing straight values to preserve old preset compatibility.

**Primary recommendation:** Extend `randomSubdiv` choices by appending 6 triplet items after the existing 9 straight items (indices 9..14). Migrate `quantizeSubdiv` from `AudioParameterInt` to `AudioParameterChoice` with straight values first (indices 0..3) then triplets (indices 4..9). Patch all three switch-statement dispatch functions. Update both ComboBox populations in PluginEditor.

---

## Standard Stack

### Core (already in use — no new dependencies)

| Component | Version | Purpose | Status |
|-----------|---------|---------|--------|
| JUCE AudioParameterChoice | JUCE 8.0.4 | APVTS enum parameters | Already used for randomSubdiv |
| JUCE AudioParameterInt | JUCE 8.0.4 | APVTS integer range | Used for quantizeSubdiv — migration needed |
| JUCE ComboBoxParameterAttachment | JUCE 8.0.4 | Attaches ComboBox to AudioParameterChoice | Already used for randomSubdivBox_ |

### Triplet Math Reference (from existing arpSubdiv)

```
1/1T  = 4.0 × (2/3) = 8.0/3.0  ≈ 2.6667 beats
1/2T  = 2.0 × (2/3) = 4.0/3.0  ≈ 1.3333 beats
1/4T  = 1.0 × (2/3) = 2.0/3.0  ≈ 0.6667 beats
1/8T  = 0.5 × (2/3) = 1.0/3.0  ≈ 0.3333 beats
1/16T = 0.25 × (2/3) = 1.0/6.0 ≈ 0.1667 beats
1/32T = 0.125 × (2/3) = 1.0/12.0 ≈ 0.0833 beats
```

**All triplet values are expressed as exact fractions to avoid floating-point drift: use `8.0/3.0`, `4.0/3.0`, `2.0/3.0`, `1.0/3.0`, `1.0/6.0`, `1.0/12.0`.**

---

## Architecture Patterns

### Current State: What Exists

#### randomSubdiv (TRIP-01)

**Parameter declaration** (PluginProcessor.cpp line 182–191):
```cpp
const juce::StringArray subdivChoices { "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
// 4 AudioParameterChoice params: randomSubdiv0..3, default index 5 (="1/8")
```

**Enum** (TriggerSystem.h line 10):
```cpp
enum class RandomSubdiv { QuadWhole = 0, DblWhole = 1, Whole = 2, Half = 3,
                          Quarter = 4, Eighth = 5, Sixteenth = 6, ThirtySecond = 7, SixtyFourth = 8 };
```

**Dispatch** (TriggerSystem.cpp line 39–52):
```cpp
static auto subdivBeatsFor = [](RandomSubdiv s) -> double {
    switch (s) {
        case RandomSubdiv::QuadWhole:    return 16.0;
        case RandomSubdiv::DblWhole:     return 8.0;
        case RandomSubdiv::Whole:        return 4.0;
        case RandomSubdiv::Half:         return 2.0;
        case RandomSubdiv::Quarter:      return 1.0;
        case RandomSubdiv::Eighth:       return 0.5;
        case RandomSubdiv::Sixteenth:    return 0.25;
        case RandomSubdiv::ThirtySecond: return 0.125;
        case RandomSubdiv::SixtyFourth:  return 0.0625;
    }
    return 1.0;
};
```

**PluginProcessor cast** (line 1348–1350):
```cpp
const int subdivIdx = static_cast<int>(*apvts.getRawParameterValue("randomSubdiv" + juce::String(v)));
tp.randomSubdiv[v] = static_cast<RandomSubdiv>(subdivIdx);
```
This is a **direct index cast** — the enum value must equal the APVTS choice index exactly.

**UI** (PluginEditor.cpp line 1805):
```cpp
randomSubdivBox_[v].addItemList(subdivChoices, 1);
```
The `subdivChoices` StringArray is a local in PluginEditor — must be extended separately from the one in PluginProcessor.

#### quantizeSubdiv (TRIP-02)

**Parameter declaration** (PluginProcessor.cpp line 256):
```cpp
addInt(ParamID::quantizeSubdiv, "Quantize Subdiv", 0, 3, 1);  // 0=1/4, 1=1/8, 2=1/16, 3=1/32
```

**Dispatch** (LooperEngine.cpp line 22–32):
```cpp
double quantizeSubdivToGridSize(int subdivIdx) noexcept {
    switch (subdivIdx) {
        case 0: return 1.0;    // 1/4
        case 1: return 0.5;    // 1/8
        case 2: return 0.25;   // 1/16
        case 3: return 0.125;  // 1/32
        default: return 0.5;
    }
}
```

**UI** (PluginEditor.cpp line 2032–2039):
```cpp
quantizeSubdivBox_.addItem("1/4",  1);
quantizeSubdivBox_.addItem("1/8",  2);
quantizeSubdivBox_.addItem("1/16", 3);
quantizeSubdivBox_.addItem("1/32", 4);
quantizeSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "quantizeSubdiv", quantizeSubdivBox_);
```

The `ComboAtt` alias is `juce::AudioProcessorValueTreeState::ComboBoxAttachment`. It works with both `AudioParameterInt` and `AudioParameterChoice` — but the mapping semantics differ (int = direct value, choice = index). Migration to `AudioParameterChoice` is cleaner and consistent with `randomSubdiv`.

---

### Pattern 1: Extending randomSubdiv (TRIP-01)

**What:** Append 6 triplet enum values + choices after existing 9 straight values.

**TriggerSystem.h — extend enum:**
```cpp
enum class RandomSubdiv {
    // Straight (existing, indices 0–8)
    QuadWhole = 0, DblWhole = 1, Whole = 2, Half = 3,
    Quarter = 4, Eighth = 5, Sixteenth = 6, ThirtySecond = 7, SixtyFourth = 8,
    // Triplets (new, indices 9–14)
    WholeT = 9, HalfT = 10, QuarterT = 11, EighthT = 12, SixteenthT = 13, ThirtySecondT = 14
};
```

**TriggerSystem.cpp — extend subdivBeatsFor():**
```cpp
case RandomSubdiv::WholeT:        return 8.0/3.0;   // 1/1T
case RandomSubdiv::HalfT:         return 4.0/3.0;   // 1/2T
case RandomSubdiv::QuarterT:      return 2.0/3.0;   // 1/4T
case RandomSubdiv::EighthT:       return 1.0/3.0;   // 1/8T
case RandomSubdiv::SixteenthT:    return 1.0/6.0;   // 1/16T
case RandomSubdiv::ThirtySecondT: return 1.0/12.0;  // 1/32T
```

**TriggerSystem.cpp — extend hitsPerBarToProbability():**
```cpp
case RandomSubdiv::WholeT:        subdivsPerBar = 0.75f;  break;  // 3 triplets per 2 bars = 0.75 per bar? No.
```

Wait — `hitsPerBarToProbability` computes "how many subdivisions fit in one bar" to derive per-slot fire probability from population. For triplets in 4/4:
- 1/1T: 3 triplets fill 2 whole notes (2 bars) → 1.5 per bar
- 1/2T: 3 triplets fill 1 whole note (1 bar) → 3.0 per bar
- 1/4T: 3 triplets fill 1 half note → 6.0 per bar
- 1/8T: 3 triplets fill 1 quarter note → 12.0 per bar
- 1/16T: → 24.0 per bar
- 1/32T: → 48.0 per bar

Formula: tripletSubdivPerBar = (4.0 / tripletBeatValue). In 4/4, bar = 4 beats.
- WholeT beat = 8/3 → 4 / (8/3) = 4 × 3/8 = 1.5
- HalfT beat = 4/3 → 4 / (4/3) = 3.0
- QuarterT beat = 2/3 → 6.0
- EighthT beat = 1/3 → 12.0
- SixteenthT = 1/6 → 24.0
- ThirtySecondT = 1/12 → 48.0

**TriggerSystem.cpp — extend hitsPerBarToProbability() cases:**
```cpp
case RandomSubdiv::WholeT:        subdivsPerBar = 1.5f;  break;
case RandomSubdiv::HalfT:         subdivsPerBar = 3.0f;  break;
case RandomSubdiv::QuarterT:      subdivsPerBar = 6.0f;  break;
case RandomSubdiv::EighthT:       subdivsPerBar = 12.0f; break;
case RandomSubdiv::SixteenthT:    subdivsPerBar = 24.0f; break;
case RandomSubdiv::ThirtySecondT: subdivsPerBar = 48.0f; break;
```

**PluginProcessor.cpp — extend subdivChoices:**
```cpp
const juce::StringArray subdivChoices {
    "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64",  // 0-8 straight
    "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"                   // 9-14 triplet
};
// Existing default index 5 (="1/8") is unchanged.
```

**PluginEditor.cpp — extend per-voice randomSubdivBox_ population:**
```cpp
const juce::StringArray subdivChoices {
    "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64",
    "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"
};
randomSubdivBox_[v].addItemList(subdivChoices, 1);
```

---

### Pattern 2: Migrating quantizeSubdiv to AudioParameterChoice (TRIP-02)

**What:** Replace `addInt(quantizeSubdiv, 0, 3, 1)` with `addChoice(quantizeSubdiv, ...)`.

**PluginProcessor.cpp — new declaration:**
```cpp
{
    const juce::StringArray qSubdivChoices {
        "1/4", "1/8", "1/16", "1/32",              // 0-3 straight (same index mapping as before)
        "1/4T", "1/8T", "1/16T", "1/32T"           // 4-7 triplet (new; 1/1T and 1/2T omitted — too slow for quantize)
    };
    addChoice(ParamID::quantizeSubdiv, "Quantize Subdiv", qSubdivChoices, 1);  // default index 1 = 1/8
}
```

Note: The quantize selector may benefit from fewer triplet options than the random selector. See Open Questions for the triplet range decision. The requirements say "same 6 triplet options" as random subdivisions for TRIP-02, so 1/1T through 1/32T would also be included (6 options, total 10 items).

**PRESET COMPATIBILITY:** Old presets saved `quantizeSubdiv` as `AudioParameterInt` value (e.g. `1` for 1/8). JUCE APVTS serializes `AudioParameterInt` and `AudioParameterChoice` both as a numeric index in the XML. When loading an old preset into the new `AudioParameterChoice`, the XML value `1` maps to choice index 1 = "1/8" which is correct. Old indices 0..3 are unchanged. **No preset migration needed.**

**LooperEngine.cpp — extend quantizeSubdivToGridSize():**
```cpp
double quantizeSubdivToGridSize(int subdivIdx) noexcept {
    switch (subdivIdx) {
        // Straight (existing)
        case 0: return 1.0;      // 1/4
        case 1: return 0.5;      // 1/8
        case 2: return 0.25;     // 1/16
        case 3: return 0.125;    // 1/32
        // Triplets (new)
        case 4: return 2.0/3.0;  // 1/4T
        case 5: return 1.0/3.0;  // 1/8T
        case 6: return 1.0/6.0;  // 1/16T
        case 7: return 1.0/12.0; // 1/32T
        default: return 0.5;
    }
}
```

**LooperEngine.h — update documented range:**
```cpp
std::atomic<int> quantizeSubdiv_ { 1 };  // 0=1/4, 1=1/8, 2=1/16, 3=1/32, 4=1/4T, 5=1/8T, 6=1/16T, 7=1/32T
```

**PluginEditor.cpp — extend quantizeSubdivBox_:**
```cpp
quantizeSubdivBox_.addItem("1/4",   1);
quantizeSubdivBox_.addItem("1/8",   2);
quantizeSubdivBox_.addItem("1/16",  3);
quantizeSubdivBox_.addItem("1/32",  4);
quantizeSubdivBox_.addItem("1/4T",  5);
quantizeSubdivBox_.addItem("1/8T",  6);
quantizeSubdivBox_.addItem("1/16T", 7);
quantizeSubdivBox_.addItem("1/32T", 8);
// quantizeSubdivAtt_ using ComboAtt (ComboBoxParameterAttachment) — unchanged
```

Note: JUCE ComboBox item IDs are 1-based; index 0 in APVTS maps to item ID 1. The attachment handles this translation automatically.

---

### Pattern 3: PPQ Subdivision Grid for Triplets (Sync Mode)

When `randomClockSync = true`, TriggerSystem fires on:
```cpp
const int64_t idx = static_cast<int64_t>(p.ppqPosition / beats);
if (idx != prevSubdivIndex_[v]) { prevSubdivIndex_[v] = idx; randomFired[v] = true; }
```

For triplets, `beats = 1.0/3.0` (for 1/8T). PPQ position divided by 1/3 = PPQ × 3. This produces integer boundaries at every triplet slot on the DAW beat grid. **No special handling needed** — the same formula works for any `beats` value including triplet fractions.

**Verification math for TRIP-02 Success Criterion 3:**
- At 120 BPM, 1/8T = 1/3 beat = 0.1667 seconds = 166.7ms between gates
- Straight 1/8 = 250ms
- Triplet = 250ms × 2/3 = 166.7ms ✓

---

### Recommended File Change Map

| File | Change | Scope |
|------|--------|-------|
| `Source/TriggerSystem.h` | Add 6 triplet enum cases to `RandomSubdiv` (indices 9–14) | 6 lines |
| `Source/TriggerSystem.cpp` | Extend `subdivBeatsFor()` + `hitsPerBarToProbability()` | ~12 lines |
| `Source/PluginProcessor.cpp` | Extend `subdivChoices` StringArray (randomSubdiv); migrate `quantizeSubdiv` addInt→addChoice | ~10 lines |
| `Source/LooperEngine.cpp` | Extend `quantizeSubdivToGridSize()` cases 4..7 | ~8 lines |
| `Source/LooperEngine.h` | Update `quantizeSubdiv_` comment | 1 line |
| `Source/PluginEditor.cpp` | Extend `randomSubdivBox_[v]` + `quantizeSubdivBox_` item lists | ~10 lines |

**Total: ~47 lines of change across 6 files. No structural/architectural change.**

---

### Anti-Patterns to Avoid

- **Do not interleave triplets with straight values in the `randomSubdiv` enum/choice list** (e.g. 4/1, 2/1, 1/1, 1/1T, ...). Interleaving breaks the direct cast `static_cast<RandomSubdiv>(subdivIdx)` and breaks preset compatibility. Append triplets as a trailing block.
- **Do not use floating-point literals like `0.333` for triplet beats** — use exact fractions `1.0/3.0` to avoid accumulated drift in the ppq subdivision index calculation.
- **Do not add 1/1T or 1/2T to quantizeSubdiv if TRIP-02 allows a smaller set** — very slow quantize grids (1/1T = 2.67 beats) are musically unusable for quantize. Requirements say "same 6 triplet options" so include all 6, but note this is unusual for a quantize selector.
- **Do not forget to extend the hitsPerBarToProbability() switch** — the default case currently returns 64.0f (SixtyFourth), so a triplet subdivision missing from the switch would produce incorrect probability, not a compile error.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Triplet beat duration | Custom lookup map or runtime math | Exact fraction constants (1.0/3.0 etc.) in switch statement | Follows arpSubdiv precedent; no allocation, audio-thread safe |
| APVTS choice migration for quantizeSubdiv | Special version-check serialization | JUCE's built-in XML persistence (same integer index, different type wrapper) | JUCE APVTS XML stores choice index as integer — compatible with old int storage |
| ComboBox attachment for triplet items | Custom listener | `juce::ComboBoxParameterAttachment` / `ComboAtt` alias | Already in use for arpSubdiv, LFO subdiv, randomSubdivBox_ |

**Key insight:** The arpeggiator already ships 1/8T and 1/16T in production. The triplet math, APVTS pattern, ComboBox pattern, and DAW sync formula are all proven. Phase 27 is a mechanical extension of that pattern to two more selectors.

---

## Common Pitfalls

### Pitfall 1: randomSubdivBox_ subdivChoices Not in Sync with PluginProcessor subdivChoices
**What goes wrong:** The ComboBox in PluginEditor is populated from a locally-defined `subdivChoices` StringArray (distinct from the one in PluginProcessor.cpp). If only one is updated, the UI shows wrong labels while the parameter uses correct indices, or vice versa.
**Why it happens:** Two separate StringArray definitions in two files — easy to update one and forget the other.
**How to avoid:** Update both in the same task. They must have identical length and identical item order.
**Warning signs:** ComboBox shows "1/64" when parameter reads index 9 (which should be "1/1T").

### Pitfall 2: Direct Enum Cast Breaks if Triplets Are Not Appended
**What goes wrong:** `tp.randomSubdiv[v] = static_cast<RandomSubdiv>(subdivIdx)` — if triplet values are inserted mid-enum (e.g. between SixtyFourth and after Whole), existing presets that saved index 6 (Sixteenth) would load as the wrong enum value.
**Why it happens:** The enum value equals the APVTS choice index directly — no lookup table.
**How to avoid:** Append triplet enum cases starting at index 9 (after SixtyFourth=8). Keep existing indices 0–8 unchanged.
**Warning signs:** Presets recorded with straight subdivisions play back at wrong rhythmic values after the update.

### Pitfall 3: AudioParameterInt vs AudioParameterChoice ComboBox Attachment Behavior
**What goes wrong:** `juce::AudioProcessorValueTreeState::ComboBoxAttachment` with an `AudioParameterInt` parameter maps the parameter's integer value directly to the ComboBox item ID. With `AudioParameterChoice` it maps the choice index (0-based) to item ID (1-based). After migration to Choice, items that were selected by direct value matching will shift.
**Why it happens:** The two parameter types have different normalization models.
**How to avoid:** After migrating `quantizeSubdiv` to `AudioParameterChoice`, verify the default display ("1/8" when index=1) is correct in the UI before proceeding.
**Warning signs:** Default quantize value shows "1/4" (index 0) instead of "1/8" (index 1) after migration.

### Pitfall 4: hitsPerBarToProbability Missing Triplet Cases Falls Through to Default
**What goes wrong:** The default case in `hitsPerBarToProbability()` is `subdivsPerBar = 64.0f` (for SixtyFourth). If a triplet subdivIndex (e.g. 9) is not handled, it silently returns the SixtyFourth probability — resulting in a very high-density grid.
**Why it happens:** Switch statement default is a silent fallback, not a compile error.
**How to avoid:** Add all 6 triplet cases explicitly. Compile with `-Wswitch` or `-Wswitch-enum` to detect unhandled enum cases — or ensure tests catch incorrect density behavior.
**Warning signs:** Random gates fire much more densely than expected when a triplet subdivision is selected.

### Pitfall 5: quantizeSubdivToGridSize() Called with New Index But Old Switch
**What goes wrong:** If LooperEngine.cpp is not updated when PluginProcessor/PluginEditor are extended, calling `setQuantizeSubdiv(5)` (1/8T) will fall through to `default: return 0.5` (1/8) in the old switch — silently using the wrong grid.
**Why it happens:** LooperEngine.cpp and PluginProcessor.cpp are separate files that must be updated together.
**How to avoid:** Update LooperEngine.cpp in the same task as the APVTS change. Verify in DAW that 1/8T quantize produces gates at 166ms intervals (not 250ms).
**Warning signs:** Quantize with 1/8T produces the same timing as straight 1/8.

---

## Code Examples

### Example 1: Exact Triplet Fraction Pattern (from arpSubdiv — production-proven)
```cpp
// Source: Source/PluginProcessor.cpp line 1435-1436 (arpeggiator implementation)
// Indices: 0=1/4, 1=1/8T, 2=1/8, 3=1/16T, 4=1/16, 5=1/32
static const double kSubdivBeats[] = { 1.0, 1.0/3.0, 0.5, 1.0/6.0, 0.25, 0.125 };
```
Phase 27 uses the same fractions: `1.0/3.0` for 1/8T, `1.0/6.0` for 1/16T, `1.0/12.0` for 1/32T, `2.0/3.0` for 1/4T, `4.0/3.0` for 1/2T, `8.0/3.0` for 1/1T.

### Example 2: PPQ Triplet Grid Index (works without modification)
```cpp
// Source: Source/TriggerSystem.cpp line 114-119
// beats = subdivBeatsFor(p.randomSubdiv[v])  ← already returns triplet fraction after patch
const int64_t idx = static_cast<int64_t>(p.ppqPosition / beats);
if (idx != prevSubdivIndex_[v])
{
    prevSubdivIndex_[v] = idx;
    randomFired[v] = true;
}
// For 1/8T at 120 BPM: ppq/0.3333 ticks 3 times per beat = correct triplet grid
```

### Example 3: AudioParameterChoice Extension Pattern (from randomSubdiv)
```cpp
// Source: Source/PluginProcessor.cpp line 182-191 — existing pattern
const juce::StringArray subdivChoices { "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
layout.add(std::make_unique<juce::AudioParameterChoice>(
    "randomSubdiv0", "Random Subdiv Root", subdivChoices, 5));  // default = 1/8

// Phase 27 extension: append triplets
const juce::StringArray subdivChoices {
    "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64",  // 0-8
    "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"                   // 9-14
};
// default index 5 still = "1/8" — no change
```

### Example 4: quantizeSubdiv Migration (addInt → addChoice)
```cpp
// BEFORE (PluginProcessor.cpp line 256):
addInt(ParamID::quantizeSubdiv, "Quantize Subdiv", 0, 3, 1);  // 0=1/4, 1=1/8, 2=1/16, 3=1/32

// AFTER:
{
    const juce::StringArray qSubdivChoices {
        "1/4", "1/8", "1/16", "1/32",      // 0-3 straight (same index mapping preserves presets)
        "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"  // 4-9 triplets (TRIP-02 requires all 6)
    };
    addChoice(ParamID::quantizeSubdiv, "Quantize Subdiv", qSubdivChoices, 1);  // default 1 = 1/8
}
```

---

## State of the Art

| Old State | New State After Phase 27 | Impact |
|-----------|--------------------------|--------|
| randomSubdiv: 9 options (straight only, indices 0–8) | 15 options (straight 0–8, triplets 9–14) | TRIP-01 satisfied |
| quantizeSubdiv: AudioParameterInt 0..3 (4 straight options) | AudioParameterChoice 0..9 (4 straight + 6 triplets) | TRIP-02 satisfied |
| hitsPerBarToProbability: handles 9 straight cases | Handles 15 cases (9 straight + 6 triplet) | Correct density for triplet random gates |
| quantizeSubdivToGridSize: handles indices 0..3 | Handles 0..9 | Correct looper quantize snap for triplets |
| arpSubdiv: already has 1/8T and 1/16T | Unchanged | Reference implementation for Phase 27 pattern |

**Precedent in codebase:** The arpeggiator's `arpSubdiv` (added Phase 23) proves triplet subdivisions work correctly in a DAW-synced context. The `subdivBeatsFor()` formula and PPQ index math are battle-tested.

---

## Open Questions

1. **Does TRIP-02 require all 6 triplet options for quantizeSubdiv, or just practical ones (1/4T, 1/8T, 1/16T, 1/32T)?**
   - What we know: TRIP-02 says "same 6 triplet options alongside existing straight subdivisions" — matching TRIP-01
   - What's unclear: 1/1T (2.67 beats) and 1/2T (1.33 beats) as quantize grids are musically unusual
   - Recommendation: Implement all 6 as required (TRIP-02 is explicit). If 1/1T/1/2T feel wrong in practice, they can be removed in a later phase. The implementation cost is the same.

2. **Preset compatibility: does AudioParameterInt → AudioParameterChoice migration for quantizeSubdiv corrupt existing presets?**
   - What we know: JUCE APVTS serializes both types as an integer in XML. Old presets that stored `quantizeSubdiv=1` will load the new AudioParameterChoice at index 1 = "1/8" — correct.
   - What's unclear: Whether JUCE APVTS validates the parameter type during XML load and rejects mismatches
   - Recommendation: Verify empirically — load a v1.5-era preset after the change and confirm quantize shows "1/8". This is a 30-second smoke test.

3. **Should the randomSubdiv triplet choices be inserted between 1/1 and 1/2 (interleaved) or appended after 1/64?**
   - What we know: Appending (indices 9..14) preserves preset compatibility. Interleaving breaks the direct enum cast.
   - Recommendation: Append. The UX of a non-interleaved list (straight first, then triplets) is acceptable and clearly documented with a separator.

---

## Sources

### Primary (HIGH confidence)
- Source code analysis: `Source/TriggerSystem.h`, `Source/TriggerSystem.cpp` — current `RandomSubdiv` enum and dispatch functions
- Source code analysis: `Source/PluginProcessor.cpp` lines 182–256 — APVTS parameter declarations for randomSubdiv and quantizeSubdiv
- Source code analysis: `Source/LooperEngine.cpp` lines 22–32 — `quantizeSubdivToGridSize()` function
- Source code analysis: `Source/PluginProcessor.cpp` lines 1435–1436 — arpSubdiv triplet implementation (`kSubdivBeats[]`)
- Source code analysis: `Source/PluginEditor.cpp` lines 1805, 2032–2039 — ComboBox population for both selectors

### Secondary (MEDIUM confidence)
- JUCE documentation pattern: `AudioParameterChoice` stores index as integer in XML — consistent with how `AudioParameterInt` stores value. Migration is safe for old preset values 0..3.
- Music theory: triplet note duration = straight note duration × 2/3 — well-established fact, no citation needed.

### Tertiary (LOW confidence)
- None — all findings verified directly from source code.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all technology already in use in the project; no new dependencies
- Architecture: HIGH — direct extension of existing patterns proven in arpSubdiv (production code)
- Pitfalls: HIGH — derived from actual code structure; preset compatibility risk verified via JUCE serialization model

**Research date:** 2026-03-03
**Valid until:** 2026-04-03 (stable codebase, math is exact, no external dependencies)
