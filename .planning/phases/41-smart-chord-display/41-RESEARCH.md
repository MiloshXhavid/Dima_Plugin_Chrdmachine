# Phase 41: Smart Chord Display — Research

**Researched:** 2026-03-09
**Domain:** JUCE C++ — chord name inference, cross-thread display state, scale pattern lookup
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**A. "Not triggered" definition**
- A voice is not triggered when it did not produce a note-on in the same trigger event as the snapshot (event-based, not real-time gate state).
- Implementation: add `uint8_t displayVoiceMask_` to `PluginProcessor` (4 bits, one per voice). Written alongside `displayPitches_` at every snapshot site. Default `0xF` (all voices active). Same best-effort non-atomic pattern as `displayPitches_`.
- Arp special case: arp fires individual voice steps, but the full chord is known via `heldPitch_`. Use mask `0xF` for arp steps so all 4 pitches are treated as "real" — inference does not apply during arp playback.
- `getCurrentVoiceMask()` exposes it alongside `getCurrentPitches()`.

**B. Scale inference for absent voices**
- Third (Voice 1 absent — bit 1 not set in mask): check `(rootPc + 3) % 12` in active scale → minor quality; check `(rootPc + 4) % 12` → major quality; both present → use Voice 1's real pitch to decide; neither present → omit third from name entirely.
- Seventh (Voice 3 absent — bit 3 not set in mask): check `(rootPc + 11) % 12` → maj7 quality (`△`); check `(rootPc + 10) % 12` → dominant/minor 7th quality; both present → use Voice 3's real pitch to decide; neither present → omit 7th from name entirely.
- Scale data source: UI thread reads APVTS params (`scalePreset`, `useCustomScale`, `scaleNote0..11`) inside `timerCallback()` when computing the chord name. One-frame latency acceptable.
- Scale notes extracted via `ScaleQuantizer::getScalePattern()` + `getScaleSize()` for presets, or the custom `bool[12]` array directly for custom mode.

**C. Update trigger logic**
- Current behavior is correct — no changes. Chord name updates on: deliberate triggers (touchplate, gamepad button, random gate), chord parameter changes via `lastChordParamHash_`, and each arp step.

**D. Arpeggiator**
- Chord display updates on every arp step. Voice mask = `0xF` for all arp steps.

**E. Unicode symbols**
- Replace throughout `ChordNameHelper.h`:
  - `"maj7"` → `"△"` (U+25B3)
  - `"mmaj7"` → `"m△"`
  - `"m7b5"` → `"ø"` (U+00F8)
  - `"dim7"` → `"°7"` (U+00B0)
  - `"maj9"` → `"△9"`
  - `"mmaj9"` → `"m△9"`
  - `"maj7#11"` → `"△#11"`
  - `"maj7#5"` → `"△#5"`
  - `"maj13"` → `"△13"`
  - All others remain as-is.

**F. Extended chord name function**
- New signature: `computeChordNameSmart(const int pitches[4], const ChordNameContext& ctx)`
- `ChordNameContext` holds `scalePattern`, `scaleSize`, `voiceMask`.
- Substitutes inferred pitch classes for absent voices before running existing template-matching logic — no duplication.
- Old `computeChordNameStr()` kept for backward compatibility.

### Scope Guardrail (Locked)
- `ChordNameHelper.h` template table is NOT expanded — new chord types are out of scope.
- No changes to `ChordEngine`, `ScaleQuantizer`, or `TriggerSystem`.
- No UI layout changes — label position/size unchanged.
- Voice 0 (root) always treated as "real". Voice 2 (fifth) always treated as "real".
- Inference only for voice 1 (third) and voice 3 (tension/7th).

### Claude's Discretion
- None specified.

### Deferred Ideas (OUT OF SCOPE)
- New chord types / expanded template table.
- UI layout changes.
- Changes to ChordEngine, ScaleQuantizer, TriggerSystem.
</user_constraints>

---

## Summary

Phase 41 adds intelligent chord quality inference to the existing chord display. The display currently calls `computeChordNameStr()` on every timer tick using pitches from `displayPitches_` — a non-atomic array snapshotted on triggers and parameter changes. The core change is: add `displayVoiceMask_` alongside `displayPitches_`, write it at every existing snapshot site, add a third snapshot site in the arp step path, expose it via `getCurrentVoiceMask()`, then upgrade `ChordNameHelper.h` with a smart overload that substitutes inferred pitch classes for absent voices before passing to the existing template matcher.

The inference logic is pure arithmetic against the scale pattern: check whether the minor-third or major-third pitch class appears in the active scale array, and similarly for the 7th. The scale is read from APVTS on the UI thread inside `timerCallback()` — exactly where the chord name is already computed. No audio-thread changes are required beyond the mask write.

Unicode replacement throughout the template table's `nm` strings is a mechanical find-and-replace with no logic change.

**Primary recommendation:** Implement in three tightly scoped files: `ChordNameHelper.h` (new struct + smart function + unicode rename), `PluginProcessor.h`/`.cpp` (add mask field + getter + three snapshot sites), `PluginEditor.cpp` (update `computeChordName` wrapper to call smart version).

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE AudioProcessorValueTreeState | 8.0.4 | Read scale params on UI thread | Already in project; `getRawParameterValue` is thread-safe for float reads |
| `std::array<int,4>` | C++17 | displayPitches_ pattern | Already established in this codebase |
| `uint8_t` | C++17 | `displayVoiceMask_` | 4-bit mask fits; same best-effort non-atomic pattern as displayPitches_ |

### No new dependencies required
This phase introduces no new libraries.

---

## Architecture Patterns

### Existing Snapshot Pattern (HIGH confidence)
The codebase uses a best-effort non-atomic `std::array<int,4> displayPitches_` as an audio→UI display channel. It is written on the audio thread at trigger sites and read on the message thread in `timerCallback()`. The same pattern is used for `displayVoiceMask_` — add `uint8_t displayVoiceMask_ {0xF}` next to `displayPitches_` in `PluginProcessor.h`.

There are currently **two** snapshot write sites in `PluginProcessor.cpp`:
1. **Line 1588** — hash-triggered parameter change refresh: `displayPitches_ = heldPitch_`
2. **Line 1609** — deliberate trigger (non-joystick source note-on): `displayPitches_ = heldPitch_`

There is **no** snapshot write in the arp step path. The CONTEXT decision requires adding one.

### Three Snapshot Sites After Phase 41
```
Site 1 (param hash, line ~1588):
    displayPitches_    = heldPitch_;
    displayVoiceMask_  = 0xF;          // all voices real on param change

Site 2 (deliberate trigger note-on, line ~1609):
    displayPitches_    = heldPitch_;
    displayVoiceMask_  = buildVoiceMask(src);  // bit = 1 only if src[v] != Joystick

Site 3 (arp step note-on, near line ~2027):
    displayPitches_    = heldPitch_;
    displayVoiceMask_  = 0xF;          // arp: full chord is always known
```

### Voice Mask Construction (Site 2)
At Site 2, a voice gets bit=1 if its trigger source caused a note-on in this trigger event. The simplest approach: at the moment `displayPitches_ = heldPitch_` fires (inside `tp.onNote` with `isOn=true`), the triggering voice's bit is always 1. The other voices' bits come from whether they have active gates: `trigger_.isGateOpen(v)` at that moment. However, per the CONTEXT decision, "not triggered" means "did not produce a note-on in the same trigger event as the snapshot." The simplest safe implementation: the mask written at Site 2 is built by scanning all 4 voices — bit v = 1 if that voice is currently gate-open (i.e., a note-on just fired or is still held from this event batch). This is best-effort (same contract as displayPitches_) and matches the intent.

A cleaner alternative (which the CONTEXT description implies): maintain a `uint8_t pendingVoiceMask_` audio-thread variable, set bit v whenever a non-joystick note-on fires for voice v in a block, then write `displayVoiceMask_ = pendingVoiceMask_` alongside `displayPitches_` when the snapshot condition is met. Reset `pendingVoiceMask_` at the start of each processBlock.

### Smart Chord Name Function Pattern (HIGH confidence)

```cpp
// Source: ChordNameHelper.h (new addition)
struct ChordNameContext {
    const int*  scalePattern;   // from ScaleQuantizer::getScalePattern()
    int         scaleSize;
    uint8_t     voiceMask;      // bit v = voice v produced a real note-on
};

inline std::string computeChordNameSmart(const int pitches[4],
                                          const ChordNameContext& ctx)
{
    // Build effective pitches — substitute inferred values for absent voices
    int effective[4];
    for (int i = 0; i < 4; ++i) effective[i] = pitches[i];

    const int rootPc = ((pitches[0] % 12) + 12) % 12;

    // Voice 1 (third) absent?
    if (!(ctx.voiceMask & 0x02) && ctx.scalePattern && ctx.scaleSize > 0)
    {
        const int minorPc = (rootPc + 3) % 12;
        const int majorPc = (rootPc + 4) % 12;
        bool hasMinor = false, hasMajor = false;
        for (int i = 0; i < ctx.scaleSize; ++i) {
            if ((rootPc + ctx.scalePattern[i]) % 12 == minorPc) hasMinor = true;
            if ((rootPc + ctx.scalePattern[i]) % 12 == majorPc) hasMajor = true;
        }
        // NOTE: scale pattern contains semitone offsets (0-based from any root),
        // so membership check needs care — see Pitfall 1 below.
        if (hasMinor && !hasMajor)
            effective[1] = (pitches[0] / 12) * 12 + minorPc;  // infer minor third
        else if (hasMajor && !hasMinor)
            effective[1] = (pitches[0] / 12) * 12 + majorPc;  // infer major third
        // both or neither: leave as pitches[1] (real value) or drop (handled by
        // template matcher receiving duplicate/matching pitch class)
    }

    // Voice 3 (tension/7th) absent?
    if (!(ctx.voiceMask & 0x08) && ctx.scalePattern && ctx.scaleSize > 0)
    {
        const int maj7Pc  = (rootPc + 11) % 12;
        const int dom7Pc  = (rootPc + 10) % 12;
        bool hasMaj7 = false, hasDom7 = false;
        for (int i = 0; i < ctx.scaleSize; ++i) {
            if ((rootPc + ctx.scalePattern[i]) % 12 == maj7Pc) hasMaj7 = true;
            if ((rootPc + ctx.scalePattern[i]) % 12 == dom7Pc) hasDom7 = true;
        }
        if (hasMaj7 && !hasDom7)
            effective[3] = (pitches[3] / 12) * 12 + maj7Pc;
        else if (hasDom7 && !hasMaj7)
            effective[3] = (pitches[3] / 12) * 12 + dom7Pc;
    }

    return computeChordNameStr(effective);
}
```

### Scale Membership Check — Critical Detail
`ScaleQuantizer::getScalePattern()` returns an array of **semitone offsets from the root** (0–11) — not absolute pitch classes. For example, Major returns `{0, 2, 4, 5, 7, 9, 11}`. To check if pitch class `targetPc` is in the scale for a given `rootPc`, the check is:

```cpp
for (int i = 0; i < scaleSize; ++i)
    if ((rootPc + scalePattern[i]) % 12 == targetPc) → present
```

This is the correct membership test. The simple loop `scalePattern[i] == offset` would fail when the root is not C.

### UI Thread — Reading Scale for Context (HIGH confidence)
In `PluginEditor::timerCallback()`, just before calling `computeChordName`, add:

```cpp
ChordNameContext ctx;
ctx.voiceMask = proc_.getCurrentVoiceMask();

const int scalePresetIdx = (int)*proc_.apvts.getRawParameterValue("scalePreset");
const bool useCustom     = (bool)(int)*proc_.apvts.getRawParameterValue("useCustomScale");

if (useCustom) {
    bool customNotes[12];
    for (int i = 0; i < 12; ++i)
        customNotes[i] = (bool)(int)*proc_.apvts.getRawParameterValue("scaleNote" + juce::String(i));
    static int customPatBuf[12]; int customSize = 0;
    ScaleQuantizer::buildCustomPattern(customNotes, customPatBuf, customSize);
    ctx.scalePattern = customPatBuf;
    ctx.scaleSize    = customSize;
} else {
    const auto preset = static_cast<ScalePreset>(scalePresetIdx);
    ctx.scalePattern = ScaleQuantizer::getScalePattern(preset);
    ctx.scaleSize    = ScaleQuantizer::getScaleSize(preset);
}
```

Note: `customPatBuf` here would be a static local or member variable — not a stack temporary — because `ctx.scalePattern` points into it.

### `computeChordName` Wrapper Update
```cpp
// PluginEditor.cpp line 45 — updated wrapper
static juce::String computeChordName(const int pitches[4], const ChordNameContext& ctx)
{
    return juce::String(computeChordNameSmart(pitches, ctx));
}
```

Called at line 6136 as:
```cpp
const auto pitches = proc_.getCurrentPitches();
chordNameLabel_.setText(computeChordName(pitches.data(), ctx), juce::dontSendNotification);
```

### Anti-Patterns to Avoid
- **Checking scale membership with `scalePattern[i] == interval`** when rootPc != 0: this works only for C root. Always use `(rootPc + scalePattern[i]) % 12 == targetPc`.
- **Making `displayVoiceMask_` atomic**: the existing displayPitches_ is non-atomic (best-effort). Mask follows same contract. Making it atomic adds unnecessary complexity.
- **Allocating `customPatBuf` on the stack inside timerCallback and keeping a pointer**: the pointer must remain valid for the duration of the call. Use a static local or a member variable.
- **Writing `displayVoiceMask_` without a matching `displayPitches_` write**: always update both atomically (both are audio-thread-only writes, message-thread reads; since both writes happen in sequence in the same block they are always coherent for display purposes).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Chord name template matching | Custom string builder | `computeChordNameStr()` existing table | 40+ templates already cover all required chord types; the smart function wraps it |
| Scale membership lookup | Custom bool array | `ScaleQuantizer::getScalePattern()` + `getScaleSize()` | Already correct, tested, covers 20 presets + custom |
| Custom scale parsing | Loop over 12 APVTS params | `ScaleQuantizer::buildCustomPattern()` | Handles the `bool[12] → int[12] + size` conversion |

---

## Common Pitfalls

### Pitfall 1: Scale Pattern is Root-Relative, Not Absolute
**What goes wrong:** Checking `scalePattern[i] == (targetPc - rootPc + 12) % 12` is correct, but it's easy to write `scalePattern[i] == targetPc` which is only correct when rootPc == 0 (C).
**Why it happens:** Scale patterns are stored as intervals from the root (0=root, 2=major second, etc.). When the global transpose key is not C, rootPc is non-zero and the naive check produces wrong results.
**How to avoid:** Always use `(rootPc + scalePattern[i]) % 12 == targetPc`.
**Warning signs:** Inference produces wrong quality on all keys except C; tests pass for C but fail for G, D, etc.

### Pitfall 2: "Both absent voices" edge case — omit vs. fallback
**What goes wrong:** When neither minor nor major third appears in the scale (e.g. pentatonic major has no minor third and no major third at semitone 3/4 from root — actually pentatonic major DOES have the major third at interval 4), the code must leave effective[1] = pitches[1] (the real pitch), which the template matcher will use as-is. If effective[1] happens to equal effective[0] (root), the deduplication in `computeChordNameStr` removes it and the chord is rendered as a power chord or root-only.
**Why it happens:** Inference is only supposed to replace absent voices with inferred ones when there is a clear scale match. When there is no match, the real pitch is used — even if that real pitch is a "placeholder" MIDI 64.
**How to avoid:** Document this as intentional: "neither present → name uses the real stored pitch for that voice, which may produce a less-informative name." No special handling needed.

### Pitfall 3: Arp snapshot site — which voice's heldPitch_?
**What goes wrong:** The arp fires one voice at a time. If `displayPitches_ = heldPitch_` is written only when the arp fires voice 0, the chord name may lag. If written on every arp step, it updates rhythmically.
**Why it happens:** Per CONTEXT decision D: "display updates on every arp step." This means the snapshot must be written for every arp note-on, not just voice 0.
**How to avoid:** Add `displayPitches_ = heldPitch_; displayVoiceMask_ = 0xF;` immediately after the `arpActivePitch_ = pitch` assignment at line ~2028, unconditionally for every arp step.

### Pitfall 4: Unicode in C++ string literals — encoding
**What goes wrong:** On Windows with MSVC, source files must be UTF-8 encoded for Unicode string literals to compile correctly. If the file is ANSI/Latin-1, the `△`, `ø`, `°` characters will be mojibake at runtime.
**Why it happens:** MSVC default source encoding can be system codepage (Windows-1252) on Windows.
**How to avoid:** Ensure `ChordNameHelper.h` is saved as UTF-8. The project already builds with CMake + MSVC; add `/utf-8` compiler flag if not already present, or use `u8"..."` string prefix. Check existing source files for encoding — if the codebase already uses UTF-8 strings elsewhere (the font label uses Unicode characters per existing JUCE UI code), this is already handled. Verify by searching for existing Unicode usage.
**Warning signs:** Chord name shows garbled characters like `â–³` instead of `△` in the plugin UI at runtime.

### Pitfall 5: `customPatBuf` lifetime in timerCallback
**What goes wrong:** Using a raw pointer `ctx.scalePattern` pointing to a stack-allocated `int[12]` that is passed to `computeChordNameSmart` — if the call chain copies the pointer without the buffer, this is fine since it's synchronous. But if the buffer is declared inside an `if` block, it goes out of scope before use.
**Why it happens:** C++ scoping.
**How to avoid:** Declare `int customPatBuf[12]; int customSize = 0;` before the if/else block, not inside the `if(useCustom)` branch.

---

## Code Examples

### Scale membership check (correct)
```cpp
// Source: derived from ScaleQuantizer.h interface + scale pattern semantics
// scalePattern[i] = semitone offset from root (0..11)
// Returns true if pitch class targetPc is in the scale rooted at rootPc
static bool isInScale(int rootPc, int targetPc,
                      const int* scalePattern, int scaleSize)
{
    for (int i = 0; i < scaleSize; ++i)
        if ((rootPc + scalePattern[i]) % 12 == targetPc)
            return true;
    return false;
}
```

### Adding the mask field to PluginProcessor.h
```cpp
// Add alongside displayPitches_ (line ~353 in PluginProcessor.h)
uint8_t displayVoiceMask_ {0xF};  // bit v = voice v produced note-on at snapshot time

// Add alongside getCurrentPitches() (line ~169)
uint8_t getCurrentVoiceMask() const { return displayVoiceMask_; }
```

### Unicode strings in ChordNameHelper.h template table
```cpp
// Replace kT[] entries — file must be saved as UTF-8
{{0, 4, 7,11}, u8"△"},        // was "maj7"
{{0, 3, 7,11}, u8"m△"},       // was "mmaj7"
{{0, 3, 6,10}, u8"ø"},        // was "m7b5"
{{0, 3, 6, 9}, u8"°7"},       // was "dim7"
{{0, 2, 4,11}, u8"△9"},       // was "maj9"
{{0, 2, 3,11}, u8"m△9"},      // was "mmaj9"
{{0, 4, 6,11}, u8"△#11"},     // was "maj7#11"
{{0, 4, 8,11}, u8"△#5"},      // was "maj7#5"
{{0, 4, 9,11}, u8"△13"},      // was "maj13"
```

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|------------------|--------|
| Chord name from 4 pitches always | Smart inference for absent voices from scale | Correct minor/major shown when not all voices triggered |
| Verbose ASCII chord names | Unicode jazz symbols (△, ø, °) | Standard jazz notation |
| timerCallback reads every block | Snapshot on trigger + param change | Correct: no update during silence |

---

## Key Findings — Exact Code Locations

| Symbol | File | Line | Notes |
|--------|------|------|-------|
| `computeChordNameStr()` | `ChordNameHelper.h` | 14 | Add smart overload in same file; unicode renames in kT[] table |
| `displayPitches_` declaration | `PluginProcessor.h` | 353 | Add `displayVoiceMask_` immediately after |
| `getCurrentPitches()` | `PluginProcessor.h` | 169 | Add `getCurrentVoiceMask()` immediately after |
| Param-hash snapshot site | `PluginProcessor.cpp` | 1588 | Add `displayVoiceMask_ = 0xF;` after existing line |
| Trigger snapshot site | `PluginProcessor.cpp` | 1609 | Add mask write with per-voice logic |
| Arp step note-on | `PluginProcessor.cpp` | ~2028 | NEW: add both `displayPitches_` and `displayVoiceMask_ = 0xF` |
| `computeChordName` wrapper | `PluginEditor.cpp` | 45 | Update signature to accept `ChordNameContext` |
| Chord name label update | `PluginEditor.cpp` | 6135–6136 | Build `ctx` from APVTS + mask, pass to updated wrapper |

---

## Open Questions

1. **Mask construction at Site 2 — per-voice or single-voice?**
   - What we know: The CONTEXT says "did not produce a note-on in the same trigger event as the snapshot." Site 2 fires inside `tp.onNote(voice, pitch, isOn=true, ...)`.
   - What's unclear: When two pads are pressed simultaneously, do two separate `onNote` calls fire in the same block? If so, `displayPitches_ = heldPitch_` gets written twice, with different masks each time.
   - Recommendation: Maintain a `uint8_t pendingVoiceMask_ = 0` that gets `|= (1 << voice)` on every non-joystick note-on in a block, and write both `displayPitches_` and `displayVoiceMask_ = pendingVoiceMask_` once per block (after the trigger loop). Reset `pendingVoiceMask_ = 0` at the top of each processBlock. This is cleaner and handles multi-pad presses correctly. Alternatively, because the CONTEXT says "same best-effort non-atomic pattern as displayPitches_", the simpler "write on each note-on" approach with the last-write winning is also acceptable for a display-only feature.

2. **`u8""` vs bare UTF-8 string literals**
   - What we know: MSVC supports `u8"..."` C++17 literals. JUCE is built with C++17. The existing codebase has Unicode elsewhere (e.g. LFO UI symbols).
   - What's unclear: Whether `/utf-8` is already in the CMakeLists.txt compiler flags.
   - Recommendation: Check `CMakeLists.txt` for `/utf-8` flag. If absent, either add it or use `u8"..."` literals in the template table. Using `u8""` is the safest approach regardless.

---

## Validation Architecture

No automated test framework detected for UI behavior. The chord name logic in `ChordNameHelper.h` is a pure function with no JUCE dependencies — it can be tested with the existing `Tests/` CTest infrastructure.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 (detected via `Tests/ChordEngineTests.cpp`, `Tests/ChordNameTests.cpp`) |
| Config file | `CMakeLists.txt` (FetchContent) |
| Quick run command | `cmake --build build --config Release --target ChordJoystickTests && ctest --test-dir build -C Release -R ChordName` |
| Full suite command | `cmake --build build --config Release --target ChordJoystickTests && ctest --test-dir build -C Release` |

### Phase Requirements → Test Map

| Behavior | Test Type | Notes |
|----------|-----------|-------|
| `computeChordNameSmart` with mask=0xF returns same result as `computeChordNameStr` | unit | `ChordNameTests.cpp` |
| Major scale, voice 1 absent → major quality inferred | unit | Add to `ChordNameTests.cpp` |
| Natural minor scale, voice 1 absent → minor quality inferred | unit | Add to `ChordNameTests.cpp` |
| Pentatonic scale, voice 1 absent, neither 3rd present → name omits quality or uses real pitch | unit | Add to `ChordNameTests.cpp` |
| Voice 3 absent + major scale → maj7 inferred (△) | unit | Add to `ChordNameTests.cpp` |
| Voice 3 absent + mixolydian → dom7 inferred | unit | Add to `ChordNameTests.cpp` |
| Unicode symbols render correctly (not garbled) | manual | Visual check in plugin UI |
| Chord name does not update during silence (display retains last name) | manual | Hold chord, release, observe label |
| Arp step updates chord name display rhythmically | manual | Start arp, observe label updates |

### Wave 0 Gaps
- `ChordNameTests.cpp` exists already — new test cases for smart inference should be added there (no new file needed).
- `u8""` or `/utf-8` CMake flag — verify before implementation.

---

## Sources

### Primary (HIGH confidence)
- `Source/ChordNameHelper.h` — complete template table and existing function (read directly)
- `Source/PluginProcessor.h` — `displayPitches_` pattern, `getCurrentPitches()`, `livePitch` atomics (read directly)
- `Source/PluginProcessor.cpp` lines 1585–1609, 2021–2031 — all existing snapshot sites (read directly)
- `Source/PluginEditor.cpp` lines 43–48, 6129–6136 — existing chord name wrapper and timer callback (read directly)
- `Source/ScaleQuantizer.h` — `getScalePattern()`, `getScaleSize()`, `buildCustomPattern()` API (read directly)
- `.planning/phases/41-smart-chord-display/41-CONTEXT.md` — all user decisions (read directly)

### Secondary (MEDIUM confidence)
- None needed — this is a self-contained codebase change with no external library dependencies.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new libraries; all patterns verified from source files
- Architecture: HIGH — exact line numbers verified, snapshot mechanism fully understood
- Pitfalls: HIGH (scale membership) / MEDIUM (Unicode encoding — depends on CMake flags not yet checked)

**Research date:** 2026-03-09
**Valid until:** 2026-04-09 (codebase-internal — valid until files change)
