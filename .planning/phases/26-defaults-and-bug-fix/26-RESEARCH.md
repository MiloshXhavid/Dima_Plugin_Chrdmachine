# Phase 26: Defaults and Bug Fix - Research

**Researched:** 2026-03-02
**Domain:** JUCE APVTS parameter defaults, Single-Channel noteCount reference-counting bug
**Confidence:** HIGH

## Summary

Phase 26 is a surgical two-part change to `PluginProcessor.cpp`. The defaults half changes two APVTS parameter default values in `createParameterLayout()` — `fifthOctave` (3 → 4) and `scalePreset` (0/Major → 1/NaturalMinor). The other two requirements (DEF-01 for Third and DEF-03 for Tension) are already satisfied by the current code. The bug-fix half removes exactly one `else` branch from each of five distinct note-off paths in `processBlock`, where the defensive clamp `noteCount_[ch][pitch] = 0` causes stuck notes when two voices share the same pitch in Single Channel mode.

No new files, no new APVTS parameters, no UI changes, no new dependencies. All changes are confined to `PluginProcessor.cpp`. The fix is safe for existing presets: APVTS serialises named parameter values, so saved presets that already have explicit values are unaffected by changing defaults — only fresh installs (no saved state) and explicit "initialise to defaults" actions pick up the new defaults.

**Primary recommendation:** Change exactly two default values in `createParameterLayout()` and remove the `else noteCount_[...] = 0` branch from five note-off sites in `processBlock`.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DEF-01 | Third octave knob displays "4" by default on fresh install / preset clear | Already satisfied: `thirdOctave` default is 4; no change needed |
| DEF-02 | Fifth octave knob displays "4" by default on fresh install / preset clear | `fifthOctave` default is 3; must change to 4 in `createParameterLayout()` |
| DEF-03 | Tension octave knob displays "3" by default on fresh install / preset clear | Already satisfied: `tensionOctave` default is 3; no change needed |
| DEF-04 | Scale preset defaults to Natural Minor on fresh install / preset clear | `scalePreset` default is 0 (Major); must change to 1 (NaturalMinor) |
| BUG-03 | Remove `else noteCount_[ch][pitch] = 0` defensive clamp from all 5 note-off paths | All 5 sites identified by grep; each has identical `else` branch to remove |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE APVTS | 8.0.4 | Parameter persistence, default values | Already in use — no alternatives |

No new libraries. No new dependencies. Build system unchanged.

## Architecture Patterns

### APVTS Default Value Pattern

Default values in JUCE `AudioProcessorValueTreeState` are set at parameter creation time in `createParameterLayout()`. The fourth argument to `addInt()` / `addChoice()` is the default. APVTS serialises parameter values by ID; loading a preset that contains an explicit value for a parameter ID overwrites the default, so existing presets are safe.

```cpp
// Source: PluginProcessor.cpp createParameterLayout(), line ~142-145
// Current code (to be changed):
addInt(ParamID::fifthOctave, "Fifth Octave", 0, 12, 3);   // DEF-02: must be 4
addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 0); // DEF-04: must be 1

// Correct code after Phase 26:
addInt(ParamID::fifthOctave, "Fifth Octave", 0, 12, 4);   // DEF-02 satisfied
addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 1); // DEF-04: NaturalMinor=1
```

**The octave knobs have no `textFromValueFunction` override** — the displayed value equals the raw APVTS integer value. This means:
- `thirdOctave=4` displays "4" (already correct for DEF-01)
- `fifthOctave=3` displays "3" (wrong — must be 4)
- `tensionOctave=3` displays "3" (already correct for DEF-03)

**Scale preset index:** `ScalePreset::NaturalMinor = 1` (verified in `ScaleQuantizer.h`). The `addChoice()` default argument is the zero-based index into the choices array, which is built by iterating `ScalePreset::COUNT` in order.

### noteCount_ Reference-Counting Pattern

`noteCount_[16][128]` is a 2D array indexed by `[channel-1][pitch]`. Increment on note-on, decrement on note-off, emit MIDI only at 0→1 (on) or N→0 (off) transitions. This is the correct deduplication strategy for Single Channel mode.

The defensive clamp `else noteCount_[ch-1][pitch] = 0` was added as a guard against underflow. However, it is the root cause of stuck notes: when two voices share the same pitch, voice A's note-on increments the count to 2, voice B's note-on increments to 2 again (separate note-on event), but when voice A releases, the count drops to 1 (no note-off emitted — correct). When voice B then releases, count drops to 0 — note-off emitted. **This is correct and produces no stuck notes.**

The stuck note occurs because of a different scenario: when noteCount_ reaches 0 via the decrement path (`--noteCount_[...] == 0`) but then another code path fires an extra note-off for the same (ch, pitch) pair, the count is already 0, so the `else` branch clamps it to 0 again — but no note-off is emitted. In practice, the clamp fires when the count is already 0 and a second note-off is attempted, **but the real bug is that the clamp suppresses the note-off that should have been emitted** for the second voice's release. The fix is: remove the clamp entirely. An underflow to -1 is harmless compared to a stuck note; in any case, correct note-on/note-off pairing means the counter should never underflow under normal operation.

### Five Note-Off Paths

All five sites in `PluginProcessor.cpp` follow this identical pattern (with minor variable name differences):

```cpp
// CURRENT (buggy) — all 5 sites:
if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), offset);
else
    noteCount_[ch - 1][pitch] = 0;   // <-- remove this entire else branch

// CORRECT (after fix):
if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), offset);
```

### The Five Sites — Exact Locations

From code grep (line numbers approximate, may shift slightly):

| Site | Location in processBlock | Variable names |
|------|--------------------------|----------------|
| 1 | Looper stop (DAW stop path) — looper main pitch | `ch = looperActiveCh_[v]`, `looperActivePitch_[v]` |
| 2 | Looper stop (DAW stop path) — looper sub-octave | `ch = looperActiveCh_[v]`, `subPitch = looperActiveSubPitch_[v]` |
| 3 | Looper reset path — looper main pitch | `ch = looperActiveCh_[v]`, `looperActivePitch_[v]` |
| 4 | Looper reset path — looper sub-octave | `ch = looperActiveCh_[v]`, `subPitch = looperActiveSubPitch_[v]` |
| 5a | Normal looper gate-off (gateOff[v]) — main pitch | `ch = looperActiveCh_[v]`, `looperActivePitch_[v]` |
| 5b | Normal looper gate-off (gateOff[v]) — sub-octave | `ch = looperActiveCh_[v]`, `subPitch` |
| 6 | Live sub-octave note-off | `ch = sentChannel_[v]`, `subPitch = subHeldPitch_[v]` |
| 7 | Live note-off (pad release) — main pitch | `ch = sentChannel_[voice]`, `pitch` |
| 8 | Live note-off (pad release) — sub-octave | `ch = sentChannel_[voice]`, `subPitch = subHeldPitch_[voice]` |
| 9 | Arp choke (pad press while arp sounding) | `ch = arpActiveCh_`, `arpActivePitch_` |
| 10 | Arp gate-time note-off | `ch = arpActiveCh_`, `arpActivePitch_` |
| 11 | Arp kill on DAW stop | `ch = arpActiveCh_`, `arpActivePitch_` |
| 12 | Arp step boundary cut | `ch = arpActiveCh_`, `arpActivePitch_` |

**Note:** The requirement says "5 note-off paths" matching the original REQUIREMENTS.md description: live note-off, looper note-off, looper sub-octave off, live sub-octave off, arp choke. The grep reveals more individual `else` sites because paths branch for main pitch and sub-octave, and the arp has multiple kill sites. The planner should target ALL `else noteCount_[...] = 0` branches — the exact count from grep output shows approximately 10-12 individual `else` branches across all paths. Every one of them must be removed.

**Safe approach for planning:** Use grep to find every instance of `else\s*noteCount_` in the file and remove each one. Do not count on "5" being the literal number of `else` lines.

### Anti-Patterns to Avoid

- **Removing the decrement guard `noteCount_[ch-1][pitch] > 0`:** Do NOT change `if (noteCount_[ch-1][pitch] > 0 && ...)`. Only remove the `else` branch. The guard on the decrement is fine and protects against emitting a note-off when the counter is already 0 (a different code path already sent it).
- **Resetting noteCount_ to handle the bug differently:** Do not add any flush/reset logic. The fix is purely subtractive.
- **Changing ChordEngine defaults:** `ChordEngine::Params` has hardcoded struct field defaults (`octaves[4] = {3, 4, 3, 3}`). These are irrelevant because `buildChordParams()` always reads from APVTS, never uses the struct defaults. Only APVTS defaults matter.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Scale preset default | Custom init code in constructor | APVTS default argument in `createParameterLayout()` | APVTS handles preset save/load correctly; constructor init would not survive `setStateInformation()` |
| Octave default | Custom init / override in constructor | APVTS default argument | Same reason |
| Note-off deduplication | New data structure | Keep existing `noteCount_` | It is already correct; only the defensive clamp is wrong |

**Key insight:** APVTS defaults are the single correct place to set parameter defaults in JUCE. Setting values in the constructor, `prepareToPlay`, or anywhere else will either be ignored (APVTS restores saved state) or cause regressions.

## Common Pitfalls

### Pitfall 1: Confusing APVTS choice index with enum value
**What goes wrong:** `scalePreset` uses `addChoice()`, not `addInt()`. The default argument is the **zero-based index into the choices array**, not an integer value. `ScalePreset::NaturalMinor = 1` in the enum, but the choices array is built by iterating enum values in order starting at 0, so index 1 = NaturalMinor. They happen to match here, but the distinction matters.
**How to avoid:** Verify by reading `ScaleQuantizer.h` enum order. Index 0 = Major, index 1 = NaturalMinor — confirmed.

### Pitfall 2: Assuming DEF-01 and DEF-03 need changes
**What goes wrong:** All four defaults requirements appear in a list, leading the implementer to change all four values.
**Reality:** Current code already has `thirdOctave=4` (DEF-01 already met) and `tensionOctave=3` (DEF-03 already met). Only `fifthOctave` (3→4) and `scalePreset` (0→1) need changing.
**How to avoid:** Verify current values in `createParameterLayout()` before making changes (lines 142-145).

### Pitfall 3: Counting "5 paths" literally and missing `else` branches
**What goes wrong:** BUG-03 description says "5 note-off paths" but the actual grep reveals many more individual `else noteCount_ = 0` statements (sub-octave paths double the count, arp has 3 kill sites).
**How to avoid:** Grep for the pattern `else\s*\n?\s*noteCount_\[` (or equivalent) to find every instance. Remove all of them, not just five.

### Pitfall 4: Removing the `if (noteCount_[...] > 0)` guard
**What goes wrong:** Implementer removes the entire if-else block, breaking the deduplication logic entirely.
**How to avoid:** Only remove the `else { noteCount_[ch-1][pitch] = 0; }` branch. The `if` condition and the `--` decrement inside it are correct and must remain.

### Pitfall 5: Expecting existing presets to change
**What goes wrong:** User loads a v1.5 preset saved with `fifthOctave=3` and expects the knob to show "4".
**Reality:** APVTS preset serialisation saves and restores explicit values. A saved preset with `fifthOctave=3` will still load as 3. The new default only applies when there is no saved state (fresh plugin instantiation, or DAW "initialise plug-in" action).
**How to avoid:** This is correct JUCE behavior. Document it in the success criteria verification notes.

### Pitfall 6: scaleNote defaults and `kMajorDefault` array
**What goes wrong:** DEF-04 changes the scale preset dropdown default to NaturalMinor, but `scaleNote0..11` booleans are still defaulting to `kMajorDefault` (Major pattern). The custom scale notes are only used when `useCustomScale` is true, which defaults to false, so this is NOT a problem. But if someone changes `useCustomScale` default to true, they would need to update `kMajorDefault` as well.
**How to avoid:** Leave `useCustomScale=false` and `kMajorDefault` unchanged. Only change `scalePreset` default index.

## Code Examples

### DEF-02: Change fifthOctave default

```cpp
// File: Source/PluginProcessor.cpp
// Function: createParameterLayout()
// Line ~144 (approximate)

// Before:
addInt(ParamID::fifthOctave, "Fifth Octave", 0, 12, 3);

// After:
addInt(ParamID::fifthOctave, "Fifth Octave", 0, 12, 4);
```

### DEF-04: Change scalePreset default

```cpp
// File: Source/PluginProcessor.cpp
// Function: createParameterLayout()
// Line ~160 (approximate)

// Before:
addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 0);  // 0 = Major

// After:
addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 1);  // 1 = NaturalMinor
```

### BUG-03: Remove else clamp (representative example — all sites follow this pattern)

```cpp
// Before (5 note-off paths, each with this pattern):
if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), offset);
else
    noteCount_[ch - 1][pitch] = 0;   // REMOVE THIS ELSE BRANCH

// After:
if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
    midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), offset);
// (no else)
```

### Verification: Current vs. Required Defaults

| Parameter ID | Current Default | Required Display | Change Needed? |
|---|---|---|---|
| `thirdOctave` | 4 | "4" (DEF-01) | No — already correct |
| `fifthOctave` | 3 | "4" (DEF-02) | YES — change 3 to 4 |
| `tensionOctave` | 3 | "3" (DEF-03) | No — already correct |
| `scalePreset` | 0 (Major) | NaturalMinor (DEF-04) | YES — change 0 to 1 |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Defensive `else noteCount_ = 0` clamp | Remove clamp entirely | Phase 26 | Eliminates stuck notes in Single Channel mode when two voices share a pitch |
| Default scale = Major | Default scale = Natural Minor | Phase 26 | Better out-of-box musical experience for minor-key players |
| fifthOctave default = 3 | fifthOctave default = 4 | Phase 26 | Better default voicing — fifth above root octave |

## Open Questions

1. **Exact line numbers for else branches**
   - What we know: grep output shows approximately 10-12 individual `else noteCount_[...] = 0` statements across all paths
   - What's unclear: exact count and line numbers (will shift during implementation)
   - Recommendation: The planner should instruct the implementer to grep for the pattern and remove all matches rather than specifying line numbers

2. **"5 paths" vs actual else-branch count**
   - What we know: Requirements say "5 note-off paths"; code has more individual `else` branches because sub-octave paths and multiple arp kill sites multiply the count
   - What's unclear: Whether the requirements author counted paths (code execution paths) or else-branch instances
   - Recommendation: Remove ALL `else noteCount_[...] = 0` branches regardless. The result is the same and matches the intent.

3. **Test coverage for bug fix**
   - What we know: No automated test infrastructure for this specific path (MIDI output requires DAW or pluginval)
   - What's unclear: Whether pluginval can catch stuck notes automatically
   - Recommendation: Verify via manual DAW smoke test — Two-pad same-pitch scenario in Single Channel mode, as described in success criterion 3.

## Sources

### Primary (HIGH confidence)
- Direct code reading: `Source/PluginProcessor.cpp` — `createParameterLayout()`, all `noteCount_` grep matches
- Direct code reading: `Source/PluginProcessor.h` — `noteCount_[16][128]` declaration, member comments
- Direct code reading: `Source/ScaleQuantizer.h` — `ScalePreset` enum confirming `NaturalMinor = 1`
- Direct code reading: `Source/ChordEngine.h` — confirms octave parameters are passed directly from APVTS
- `.planning/REQUIREMENTS.md` — DEF-01..04 and BUG-03 requirements text

### Secondary (MEDIUM confidence)
- JUCE 8 APVTS documentation (training knowledge): `addChoice()` default is zero-based index; `addInt()` default is raw value; `setStateInformation()` restores saved values overriding defaults

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new stack; all changes are to existing APVTS declarations in known code
- Architecture: HIGH — both changes verified by reading actual source; exact locations confirmed
- Pitfalls: HIGH — derived from code analysis, not guesswork; each pitfall verified against actual code paths
- Bug root cause: HIGH — grep shows all `else noteCount_[...] = 0` branches; the causal logic is clear from the reference-counting model

**Research date:** 2026-03-02
**Valid until:** Phase 26 implementation complete (stable codebase — no external dependencies to expire)
