# Phase 35: Arp Subdivision Expansion - Research

**Researched:** 2026-03-06
**Domain:** JUCE APVTS AudioParameterChoice, arpeggiator timing logic, gamepad Option Mode 1
**Confidence:** HIGH

## Summary

The arpeggiator subdivision is currently an isolated 6-item APVTS `AudioParameterChoice` with its
own hardcoded `kSubdivBeats[6]` lookup table inside `processBlock`. The Random Trigger system uses
a parallel but independent 17-item APVTS `AudioParameterChoice` (the `randomSubdivN` params) with
the `subdivBeatsFor(RandomSubdiv)` lambda in TriggerSystem.cpp.

Phase 35 replaces the arp's 6-item param with a 17-item param whose string labels and index order
exactly match the `randomSubdivN` params. The arp's private `kSubdivBeats[6]` table is replaced
with a new 17-entry table aligned to the new index ordering. The gamepad Option Mode 1 Triangle
button step range `(0, 5)` must be updated to `(0, 16)`. The APVTS default index changes from
2 (1/8) to 8 (1/8 in new ordering), and the UI ComboBox is repopulated with the full 17-item list.

The biggest implementation risk is preset backward compatibility. Old presets encode `arpSubdiv`
as a normalised float (`index / (N-1)`). When the param count changes from 6 to 17, the JUCE
APVTS denormalization arithmetic maps the old float to a different new index. The requirement
says "default to 1/8 if out of range" — the clamped jlimit in processBlock handles crash
prevention, but the loaded value will be silently wrong, not silently defaulted. The planner
needs to decide: accept wrong-but-harmless (no crash) or add a migration guard in
`setStateInformation`. Crashing is impossible once the `jlimit(0, 16, subdivIdx)` guard is in
place.

**Primary recommendation:** Align arpSubdiv to the randomSubdivN 17-item param exactly. Replace
kSubdivBeats[6] with an inline table matching RandomSubdiv enum ordering. Update gamepad step
range from `(0,5)` to `(0,16)`. Accept that old preset arp rates will be silently remapped
(no crash, no user-visible error beyond a different arp rate on first load).

---

## Exact Current State

### APVTS Parameter: `arpSubdiv`

Defined in `PluginProcessor.cpp` line 261:
```cpp
addChoice(ParamID::arpSubdiv, "Arp Subdivision",
          { "1/4", "1/8T", "1/8", "1/16T", "1/16", "1/32" }, 2);  // default: 1/8
```
- **6 items**, index 0-5
- Default index: 2 (= "1/8")
- Order: 1/4, 1/8T, 1/8, 1/16T, 1/16, 1/32

### processBlock kSubdivBeats (arp)

Defined at line 1727 (audio-thread local, same block that reads the param):
```cpp
// Indices: 0=1/4, 1=1/8T, 2=1/8, 3=1/16T, 4=1/16, 5=1/32
static const double kSubdivBeats[] = { 1.0, 1.0/3.0, 0.5, 1.0/6.0, 0.25, 0.125 };
const int subdivIdx = juce::jlimit(0, 5,
    static_cast<int>(*apvts.getRawParameterValue(ParamID::arpSubdiv)));
const double subdivBeats = kSubdivBeats[subdivIdx];
```
The `jlimit(0, 5, ...)` is the crash-prevention guard. It also functions as the out-of-range
clamp for old presets once the limit changes to `(0, 16)`.

### UI ComboBox population (PluginEditor.cpp lines 2876-2887)

```cpp
arpSubdivBox_.addItem("1/4",  1);
arpSubdivBox_.addItem("1/8T", 2);
arpSubdivBox_.addItem("1/8",  3);
arpSubdivBox_.addItem("1/16T",4);
arpSubdivBox_.addItem("1/16", 5);
arpSubdivBox_.addItem("1/32", 6);
arpSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "arpSubdiv", arpSubdivBox_);
```
Uses `ComboBoxAttachment` (index-based: item IDs 1..N map to APVTS choice indices 0..N-1).

### Gamepad Option Mode 1 Triangle step

Line 735:
```cpp
{ const int d = gamepad_.consumeArpRateDelta(); if (d) stepWrappingParam(apvts, "arpSubdiv", 0, 5, d); }
```
The `maxVal=5` hardcodes the 6-item upper bound. **This must become 16.**

---

## Target State: The 17-Item randomSubdiv Param

### APVTS definition for randomSubdivN (lines 184-203)

The target param ordering (what arpSubdiv must match):
```
"4/1", "2/1",          // indices 0-1
"1/1", "1/1T",         // indices 2-3
"1/2", "1/2T",         // indices 4-5
"1/4", "1/4T",         // indices 6-7
"1/8", "1/8T",         // indices 8-9
"1/16", "1/16T",       // indices 10-11
"1/32", "1/32T",       // indices 12-13
"1/64",                // index 14
"2/1T", "4/1T"         // indices 15-16
```
17 items. Default for randomSubdivN is index 8 (= "1/8").

### RandomSubdiv enum (TriggerSystem.h lines 11-29)

The enum values are _integers_ that match APVTS indices exactly:
```
QuadWhole=0 (4/1), DblWhole=1 (2/1),
Whole=2 (1/1), WholeT=3 (1/1T),
Half=4 (1/2), HalfT=5 (1/2T),
Quarter=6 (1/4), QuarterT=7 (1/4T),
Eighth=8 (1/8), EighthT=9 (1/8T),
Sixteenth=10 (1/16), SixteenthT=11 (1/16T),
ThirtySecond=12 (1/32), ThirtySecondT=13 (1/32T),
SixtyFourth=14 (1/64),
DblWholeT=15 (2/1T), QuadWholeT=16 (4/1T)
```

### subdivBeatsFor() in TriggerSystem.cpp (lines 39-58)

The authoritative beat values (quarter note = 1.0 beat):
```
QuadWhole=16.0, DblWhole=8.0,
Whole=4.0, WholeT=8.0/3.0,
Half=2.0, HalfT=4.0/3.0,
Quarter=1.0, QuarterT=2.0/3.0,
Eighth=0.5, EighthT=1.0/3.0,
Sixteenth=0.25, SixteenthT=1.0/6.0,
ThirtySecond=0.125, ThirtySecondT=1.0/12.0,
SixtyFourth=0.0625,
DblWholeT=16.0/3.0, QuadWholeT=32.0/3.0
```

---

## Architecture Patterns

### Pattern 1: JUCE ComboBoxAttachment index mapping

`AudioProcessorValueTreeState::ComboBoxAttachment` maps ComboBox item IDs 1..N to APVTS choice
indices 0..N-1. When you call `addItem("label", id)`, the attachment interprets `id-1` as the
APVTS parameter index. This means the ComboBox items must be added in the same order as the
APVTS `StringArray` choices, and `addItem` IDs must be sequential starting from 1.

The existing pattern for `randomSubdivBox_` (line 2412) uses `addItemList(subdivChoices, 1)`
which correctly populates all items with IDs starting at 1. The arp ComboBox currently uses
manual `addItem` calls — both approaches work identically.

### Pattern 2: Inline vs. shared beat lookup table

The arp's `kSubdivBeats[6]` is a `static const double[]` local to the arp section of
`processBlock`. The 17-item replacement table should follow the same pattern (static const local)
with values matching `subdivBeatsFor()` from TriggerSystem.cpp indexed by APVTS position.

The `subdivBeatsFor()` lambda is not accessible from PluginProcessor.cpp (it's file-scope in
TriggerSystem.cpp). Do not refactor the architecture — duplicate the 17 values inline. Verified
values:
```cpp
// Index = APVTS choice index = RandomSubdiv cast
static const double kSubdivBeats[17] = {
    16.0,        // 0: 4/1
    8.0,         // 1: 2/1
    4.0,         // 2: 1/1
    8.0/3.0,     // 3: 1/1T
    2.0,         // 4: 1/2
    4.0/3.0,     // 5: 1/2T
    1.0,         // 6: 1/4
    2.0/3.0,     // 7: 1/4T
    0.5,         // 8: 1/8  ← new default
    1.0/3.0,     // 9: 1/8T
    0.25,        // 10: 1/16
    1.0/6.0,     // 11: 1/16T
    0.125,       // 12: 1/32
    1.0/12.0,    // 13: 1/32T
    0.0625,      // 14: 1/64
    16.0/3.0,    // 15: 2/1T
    32.0/3.0     // 16: 4/1T
};
```

### Pattern 3: addChoice / addItemList symmetry

For the APVTS definition, copy the exact same StringArray used by randomSubdivN. To keep them
in sync long-term, share a const StringArray. The simplest approach: define the StringArray once
in a local scope in `createParameterLayout()` and pass it to both arpSubdiv and a comment
pointing to randomSubdivN.

### Anti-patterns to avoid

- **Don't reorder the 17 items** from the randomSubdivN ordering. The APVTS index must be
  identical so the RandomSubdiv enum cast works the same way across both systems.
- **Don't use `juce::ComboBoxParameterAttachment` with non-sequential IDs.** Keep IDs 1..17.
- **Don't update jlimit to (0,16) in only one place.** There are two guards: (1) the
  processBlock subdivision index clamp and (2) the gamepad `stepWrappingParam` maxVal.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Beat-value lookup for triplets | Custom fraction math per block | Inline table (same as kLfoSubdivBeats pattern) | Already proven in LFO code; table is trivially verifiable |
| Preset migration | Custom XML migration pass | Accept jlimit clamp | Requirement explicitly says "defaults to 1/8 if out of range"; a crash is the only unacceptable outcome |
| ComboBox→APVTS sync | Manual selectedId listener | ComboBoxAttachment | Already established pattern for all other combos |

---

## Common Pitfalls

### Pitfall 1: jlimit upper bound left at 5 after param expansion
**What goes wrong:** If `jlimit(0, 5, subdivIdx)` is not updated to `jlimit(0, 16, subdivIdx)`,
any index above 5 clamps to 1/32. Selecting 1/1 in the UI would sound like 1/32 (very fast).
**Why it happens:** The jlimit and the param size are in two different code locations; easy to
update one and miss the other.
**How to avoid:** Update both on the same line edit pass: (a) createParameterLayout StringArray
size, (b) jlimit bounds in processBlock, (c) kSubdivBeats array size. Treat them as a single
atomic change.
**Warning signs:** Selecting slow subdivisions (4/1, 1/1) produces very fast arping.

### Pitfall 2: Gamepad step range not updated
**What goes wrong:** `stepWrappingParam(apvts, "arpSubdiv", 0, 5, d)` wraps at index 5 instead
of 16. Triangle button can only cycle through the first 6 options.
**Why it happens:** The `maxVal=5` literal is in PluginProcessor.cpp line 735, separate from the
param definition.
**How to avoid:** Search for `arpSubdiv` in the entire codebase before declaring done. There are
exactly two sites: createParameterLayout (the definition) and the gamepad dispatch (line 735).
**Warning signs:** Gamepad Triangle only cycles through 1/4, 1/8T, 1/8, 1/16T, 1/16, 1/32
ignoring all new items.

### Pitfall 3: Default index mismatch (2 vs 8)
**What goes wrong:** Old default is index 2 in the 6-item set (= "1/8"). New 17-item set has
"1/8" at index 8. If the default remains 2, a freshly loaded plugin opens with "1/1" selected
(the third item in the new list).
**Why it happens:** `addChoice(..., 2)` — the default index literal is not obviously wrong.
**How to avoid:** The new `addChoice` call must use default index 8 (= "1/8" in the 17-item set).
Comment it explicitly: `// default: 1/8 (index 8 in 17-item set)`.
**Warning signs:** New plugin instance defaults to a very slow arp rate instead of 1/8.

### Pitfall 4: UI addItem IDs mismatched with APVTS indices
**What goes wrong:** ComboBoxAttachment uses `itemId - 1` as the APVTS choice index internally.
If IDs start at 0, or are non-sequential, the attachment sends wrong normalised values.
**Why it happens:** Manual addItem calls use programmer-chosen IDs, unlike addItemList which
auto-assigns them.
**How to avoid:** Use `arpSubdivBox_.addItemList(subdivChoices, 1)` matching the randomSubdivBox_
pattern, OR keep sequential addItem IDs starting at 1. Verify: IDs must be 1..17.

### Pitfall 5: Preset backward-compat crash vs. silent wrong value
**What goes wrong:** With 6 items, the APVTS stored normalised float = `index / 5.0`. With 17
items, the same float denormalises to `round(float * 16)`. Index 2/5=0.4 becomes index 7 (1/4T).
Index 5/5=1.0 becomes index 16 (4/1T — very slow). There is no crash, just a wrong arp rate.
**Why it happens:** JUCE `AudioParameterChoice` normalisation: stored as `index/(N-1)`.
**How to avoid:** This is acceptable per the requirement ("defaults to 1/8 if out of range" is
satisfied by the jlimit clamp). Document this in the plan so the verifier doesn't flag it as a
bug. If exact 1/8 default is required on preset mismatch, a migration guard in
`setStateInformation` would check if the arpSubdiv value looks "new" (N > 6). This is optional —
the requirement's language is permissive.

---

## Code Examples

### New APVTS parameter definition (createParameterLayout)
```cpp
// Source: matches randomSubdivN StringArray verbatim (PluginProcessor.cpp ~line 184)
const juce::StringArray arpSubdivChoices {
    "4/1", "2/1",
    "1/1", "1/1T",
    "1/2", "1/2T",
    "1/4", "1/4T",
    "1/8", "1/8T",     // index 8 = default
    "1/16", "1/16T",
    "1/32", "1/32T",
    "1/64",
    "2/1T", "4/1T"
};
addChoice(ParamID::arpSubdiv, "Arp Subdivision", arpSubdivChoices, 8);  // default: 1/8
```

### New processBlock beat lookup (replaces lines 1727-1730)
```cpp
// Indices match RandomSubdiv enum + randomSubdivN APVTS ordering
static const double kSubdivBeats[17] = {
    16.0, 8.0,              // 4/1, 2/1
    4.0, 8.0/3.0,           // 1/1, 1/1T
    2.0, 4.0/3.0,           // 1/2, 1/2T
    1.0, 2.0/3.0,           // 1/4, 1/4T
    0.5, 1.0/3.0,           // 1/8, 1/8T
    0.25, 1.0/6.0,          // 1/16, 1/16T
    0.125, 1.0/12.0,        // 1/32, 1/32T
    0.0625,                 // 1/64
    16.0/3.0, 32.0/3.0      // 2/1T, 4/1T
};
const int subdivIdx = juce::jlimit(0, 16,
    static_cast<int>(*apvts.getRawParameterValue(ParamID::arpSubdiv)));
const double subdivBeats = kSubdivBeats[subdivIdx];
```

### New UI ComboBox population (replaces lines 2876-2882 in PluginEditor.cpp)
```cpp
const juce::StringArray arpSubdivChoices {
    "4/1", "2/1",
    "1/1", "1/1T",
    "1/2", "1/2T",
    "1/4", "1/4T",
    "1/8", "1/8T",
    "1/16", "1/16T",
    "1/32", "1/32T",
    "1/64",
    "2/1T", "4/1T"
};
arpSubdivBox_.addItemList(arpSubdivChoices, 1);
arpSubdivBox_.setTooltip("ARP Rate  -  time between arpeggiator steps");
styleCombo(arpSubdivBox_);
addAndMakeVisible(arpSubdivBox_);
arpSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "arpSubdiv", arpSubdivBox_);
```

### Gamepad step range fix (line 735)
```cpp
// Change maxVal from 5 to 16:
{ const int d = gamepad_.consumeArpRateDelta(); if (d) stepWrappingParam(apvts, "arpSubdiv", 0, 16, d); }
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| 6-item arpSubdiv (1/4..1/32 + 2 triplets) | 17-item matching randomSubdivN | Phase 35 | Full subdivision parity between arp and random |
| Hardcoded jlimit(0,5) | jlimit(0,16) | Phase 35 | No crash; out-of-range values clamp to slowest subdivision |

**Deprecated after this phase:**
- `kSubdivBeats[6]` (the 6-entry local array): replaced by the 17-entry version
- Gamepad `stepWrappingParam(..., 0, 5, ...)` for arpSubdiv

---

## Open Questions

1. **Exact preset behavior on migration**
   - What we know: old normalised float `= index/5.0`; new param denormalises as `round(float*16)`
   - What's unclear: whether the user wants a hard "1/8 default on bad preset" (requires
     setStateInformation guard) or "whatever it maps to is fine" (jlimit clamp is enough)
   - Recommendation: The roadmap says "arp rate defaults to 1/8 if the old index is out of range"
     — the jlimit clamp satisfies "no crash" but not "defaults to 1/8". If exact 1/8 fallback is
     required, the plan must add a `setStateInformation` migration check. Confirm intent with user
     or treat jlimit as sufficient.

2. **ComboBox width for longer labels (4/1, 2/1T)**
   - What we know: the existing arp ComboBox shows 6 short labels; "1/8" is 3 chars
   - What's unclear: whether "4/1T" (4 chars) or "2/1T" fits in the current narrow arp panel width
   - Recommendation: Check arpBlockBounds_ / arpSubdivBox_ width in resized(). If too narrow,
     use abbreviated labels ("4/1T" is fine; the concern is font rendering in PixelLookAndFeel).
     Verify visually after build.

---

## Sources

### Primary (HIGH confidence)
- Direct code read: `Source/PluginProcessor.cpp` lines 184-203, 261-262, 735, 1727-1730 — exact
  current state of all 4 change sites
- Direct code read: `Source/PluginEditor.cpp` lines 2876-2887 — current arpSubdivBox_ population
- Direct code read: `Source/TriggerSystem.h` lines 11-29 — RandomSubdiv enum values
- Direct code read: `Source/TriggerSystem.cpp` lines 39-57 — subdivBeatsFor() authoritative values
- Direct code read: `Source/PluginProcessor.h` lines 366-375 — arp audio-thread state members

### Secondary (MEDIUM confidence)
- JUCE AudioParameterChoice normalisation behaviour (ComboBoxAttachment index mapping): confirmed
  by pattern used in existing randomSubdivAtt_ (line 2416-2417) which works correctly in
  production

---

## Metadata

**Confidence breakdown:**
- Current param/code state: HIGH — read directly from source files
- Target param ordering: HIGH — read from randomSubdivN definition
- Beat values: HIGH — read from subdivBeatsFor() in TriggerSystem.cpp
- Preset backward compat behaviour: HIGH (mechanically) / MEDIUM (intent) — JUCE normalisation
  is deterministic; what the user expects is ambiguous

**Research date:** 2026-03-06
**Valid until:** Stable — no external dependencies. Valid until any of the 4 change sites are
modified independently.
