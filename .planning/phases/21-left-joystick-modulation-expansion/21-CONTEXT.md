# Phase 21: Left Joystick Modulation Expansion — Context

**Gathered:** 2026-03-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Extend the Left Stick X and Y target menus from 2–3 CC-only options to 6 options each, adding direct LFO parameter modulation (Freq, Phase, Level) and unified Gate Length control. CC targets remain; LFO targets write APVTS params directly instead of emitting MIDI CC.

</domain>

<decisions>
## Implementation Decisions

### X Axis Target List (6 items, in order)

| Index | Label | Dispatch |
|-------|-------|----------|
| 0 | Cutoff (CC74) | MIDI CC74 — existing behavior |
| 1 | VCF LFO (CC12) | MIDI CC12 — existing behavior |
| 2 | LFO-X Freq | Direct APVTS write to lfoXRate |
| 3 | LFO-X Phase | Direct APVTS write to lfoXPhase |
| 4 | LFO-X Level | Direct APVTS write to lfoXLevel |
| 5 | Gate Length | Direct APVTS write to gateLength |

Note: Mod Wheel (CC1) dropped from list — replaced by LFO targets.

### Y Axis Target List (6 items, symmetric)

| Index | Label | Dispatch |
|-------|-------|----------|
| 0 | Resonance (CC71) | MIDI CC71 — existing behavior |
| 1 | LFO Rate (CC76) | MIDI CC76 — existing behavior |
| 2 | LFO-Y Freq | Direct APVTS write to lfoYRate |
| 3 | LFO-Y Phase | Direct APVTS write to lfoYPhase |
| 4 | LFO-Y Level | Direct APVTS write to lfoYLevel |
| 5 | Gate Length | Direct APVTS write to gateLength |

### LFO Sync + Freq Target Behavior

When LFO Sync is ON and the stick targets LFO Freq:
- The stick **scales the sync subdivision** (multiplier applied to the current sync rate)
- Stick fully left = slower (e.g. half-speed), center = unmodified, fully right = faster (e.g. double)
- Remains grid-locked — does NOT switch to free mode
- Feels expressive ("analog feel") without breaking beat alignment
- SC3 requirement ("no effect") is replaced by this subdivision-scaling behavior

When LFO Sync is OFF and target is LFO Freq: stick drives the free Hz rate directly.

### Attenuation Knob — Relabeled Per Target

The existing Atten knob stays visible for all target modes but its label changes to match the target:

| Target | Atten label | Meaning |
|--------|-------------|---------|
| Cutoff (CC74) | Atten | CC output scale 0–127% (current) |
| VCF LFO (CC12) | Atten | CC output scale 0–127% (current) |
| LFO Rate (CC76) | Atten | CC output scale 0–127% (current) |
| Resonance (CC71) | Atten | CC output scale 0–127% (current) |
| LFO-X/Y Freq | Hz | How many Hz of the LFO rate range the stick sweeps |
| LFO-X/Y Phase | Phase | How much of the phase range (0–2π) the stick sweeps |
| LFO-X/Y Level | Level | How much of the level range (0–1) the stick sweeps |
| Gate Length | Gate | How much of the gate range (0–1) the stick sweeps |

Claude's Discretion: exact scaling ranges for each LFO target (e.g. Freq sweep 0–20Hz, Level sweep 0–1.0). Choose sensibly based on existing APVTS param ranges.

### CC Cleanup on Target Switch

When switching from a CC target to an LFO target: stop emitting that CC immediately (prevCcCut_ / prevCcRes_ reset to -1 on mode change — existing pattern already handles this for X/Y mode changes, extend to cover all 6 targets).

### Dispatch Architecture

- Indices 0–1 (X) and 0–1 (Y): existing CC dispatch path, unchanged
- Indices 2–5: "pending-atomic dispatch" — write stick value to an atomic float that the audio thread reads each processBlock and applies to the target APVTS param (same pattern as modulatedJoyX_/Y_ atomics used for LFO display)
- Gate Length target: writes to the unified `gateLength` APVTS param (shared with Arp + Random)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- `filterXModeBox_` / `filterYModeBox_` (PluginEditor.h:298-299): existing ComboBox widgets — extend item lists from 3/2 to 6 each
- `filterXAttenKnob_` / `filterYAttenKnob_` (PluginEditor.h:274): existing Atten sliders — keep widgets, add timerCallback logic to relabel based on current mode
- `filterXAttenAtt_` / `filterYAttenAtt_` (PluginEditor.h:346): existing SliderAtt — keep as-is (still wired to filterXAtten / filterYAtten APVTS params)
- `prevXMode_` / `prevYMode_` + `prevCcCut_.store(-1)` reset (PluginProcessor.cpp:1477-1478): existing mode-change CC reset pattern — extend to cover all 6 indices
- `modulatedJoyX_` / `modulatedJoyY_` atomics: established pattern for inter-thread float passing — use same pattern for pending stick→LFO dispatch

### Established Patterns

- CC dispatch: `ccXnum` computed from `xMode` int (PluginProcessor.cpp:1474) — extend this switch for 6 items; indices 2–5 branch to LFO write path instead of CC
- `prevCcCut_.store(-1)` on mode change: already resets CC dedup on target switch — extend logic
- APVTS param write from processBlock: exists for all LFO params — stick target writes go to the same APVTS addresses

### Integration Points

- `filterXMode` / `filterYMode` APVTS params (PluginProcessor.cpp:219-220): currently 3/2 choice items — extend StringArrays to 6 items each
- `filterXAtten` / `filterYAtten` APVTS params: unchanged — Atten knob stays wired to these; only label changes in UI
- `lfoXRate`, `lfoXPhase`, `lfoXLevel`, `lfoYRate`, `lfoYPhase`, `lfoYLevel` APVTS params: these are the write targets for LFO dispatch (planner to confirm exact param IDs from PluginProcessor.cpp APVTS layout)
- `gateLength` APVTS param: Gate Length target writes here (registered in Phase 20)
- timerCallback() in PluginEditor: already handles conditional show/hide per mode — add label update for Atten knob here

</code_context>

<specifics>
## Specific Ideas

- "Analog feel" in sync mode: subdivision scaling should feel continuous/smooth, not stepped — a multiplier float (0.25x–4x?) applied per-block rather than snapping to discrete subdivisions
- Atten knob relabeling happens in timerCallback() by reading the current combo index and calling `styleLabel(filterXAttenLabel_, newText)` — same pattern already used for show/hide

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 21-left-joystick-modulation-expansion*
*Context gathered: 2026-03-01*
