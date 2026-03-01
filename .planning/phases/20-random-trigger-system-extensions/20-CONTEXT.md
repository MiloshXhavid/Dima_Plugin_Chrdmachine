# Phase 20 — Random Trigger System Extensions
## CONTEXT.md

**Phase goal:** Extend the random trigger system with Free/Hold modes, Population + Probability controls, 1/64 subdivision, and a unified Gate Length parameter.

---

## 1. Free / Hold Trigger Modes

### Decision
The per-voice `triggerSource` dropdown expands from 3 → 4 options:

| Value | Label | Behavior |
|-------|-------|----------|
| 0 | TouchPlate | Existing behavior — note on/off follows pad |
| 1 | Joystick | Existing behavior — note on while joystick active |
| 2 | Random Free | **Renamed from "Random"** — fires autonomously at density/rate without any pad held |
| 3 | Random Hold | **New** — fires only while the physical pad for that voice is held down |

- Scope: **per-voice** (same granularity as existing `triggerSource`)
- UI placement: same per-voice trigger source combo box — just extended with the two new labels
- "Random" (value 2) is renamed to "Random Free" — behavior unchanged, label updated
- **Random Hold note-off rule:** if the pad is released mid-note, note-off fires immediately (gate timer is overridden by pad release)

### APVTS impact
- `triggerSource0..3` Choice param: expand to 4 options (add "Random Hold" at index 3)
- `TriggerSource` enum: add `RandomHold = 3`

---

## 2. Population + Probability Model

### Decision
- **Population** = `randomDensity` **renamed** — same behavior (expected notes per bar, range 1–8)
  - APVTS param ID changes: `randomDensity` → `randomPopulation`
  - UI knob label changes: "Density" → "Population"
  - Behavior unchanged: base probability = `population / subdivsPerBar`
- **Probability** = **new** per-subdivision fire chance (0–100%)
  - APVTS param ID: `randomProbability`, range 0.0–1.0, default 1.0 (100% — fully enabled, matches existing behavior)
  - UI: new knob added alongside Population
  - Interaction: **two independent rolls** — tick fires only if both roll succeeds:
    1. Roll against `population / subdivsPerBar`
    2. Roll against `probability`
    - Expected notes/bar = `population × probability`

### Notes for planner
- `randomDensity` APVTS ID rename may break existing DAW presets — decide on migration or aliasing strategy
- Default `randomProbability = 1.0` preserves existing behavior for loaded sessions

---

## 3. Unified Gate Length

### Decision
- A single `gateLength` parameter replaces both `randomGateTime` and the arpeggiator's gate length parameter
- **Scope:** controls note duration for Random Free, Random Hold, and Arpeggiator triggers
- **TouchPlate / Joystick triggers:** unaffected — their note-off remains pad/joystick-driven
- **Arp gate % UI:** stays in the UI but is wired to the same unified `gateLength` param
- **APVTS:** `randomGateTime` renamed to `gateLength` (or a new shared ID); arp gate param merged into this

### Manual mode (knob = 0)
- When `gateLength = 0.0` (fully left): **manual open gate**
- For **Random Free**: note stays open until the *next* random trigger fires for that voice (creates legato/overlapping effect)
- For **Random Hold**: note stays open while pad is held — pad release still immediately sends note-off (per Area 1 rule)
- For **Arp**: open gate until arp advances to next step (existing arp step boundary acts as implicit note-off)

### Range
- 0.0 = manual (open gate, behavior above)
- 0.0–1.0 = fraction of subdivision duration (existing behavior)
- Existing 10ms minimum floor is retained for non-manual mode

---

## 4. 1/64 Subdivision

### Decision
- Add `"1/64"` as the 5th option in each per-voice `randomSubdiv` combo box
- Label format: `"1/64"` — consistent with existing `1/4 · 1/8 · 1/16 · 1/32`
- `RandomSubdiv` enum: add `SixtyFourthNote` (value 4)
- Interval in beats: `0.0625` (= 1/16 of a beat)
- **Safety guard:** the existing 10ms minimum gate floor is sufficient protection for all devices — no additional guard needed

### APVTS impact
- `randomSubdiv0..3` Choice params: expand from 4 → 5 options

---

## 5. Code Context

### Key files
| File | What changes |
|------|-------------|
| `Source/TriggerSystem.h` | `RandomSubdiv` enum: add `SixtyFourthNote`; `TriggerSource` enum: add `RandomHold`; `ProcessParams` may need `padHeld[4]` passed in for Hold mode |
| `Source/TriggerSystem.cpp` | `subdivBeatsFor()`: add 1/64 case; `processBlock()`: add RandomHold gate logic, add double-roll probability, handle manual gate (open until next trigger), cut on pad release for Hold |
| `Source/PluginProcessor.cpp` | APVTS: rename `randomDensity` → `randomPopulation`, rename `randomGateTime` → `gateLength`, add `randomProbability`, expand `triggerSource` and `randomSubdiv` choices |
| `Source/PluginEditor.cpp` | Add Probability knob, rename Population knob, update combo boxes (trigger source + subdivision), wire arp gate UI to unified `gateLength` |

### Existing patterns to reuse
- `randomGateRemaining_[4]` pattern for auto note-off countdown — extend for manual mode (set to -1 = open)
- `hitsPerBarToProbability()` helper in TriggerSystem.cpp — wrap with the new Probability multiplier
- LCG RNG (`nextRandom()`) — use for Probability second roll (same RNG, two calls per tick)
- `padState_[v]` or similar in TouchPlate handling — reuse/extend for Random Hold gate detection

### Deferred ideas (not in Phase 20)
- Per-voice Probability (all 4 share one Probability knob in Phase 20)
- Per-voice Population (global in Phase 20)
- Pattern-based randomness (step sequencer integration)
