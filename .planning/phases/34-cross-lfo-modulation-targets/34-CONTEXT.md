# Phase 34: Cross-LFO Modulation Targets - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Extend `filterXMode` and `filterYMode` APVTS choice params from 8 to 11 items each.
New modes 8–10 on each axis route the left joystick to the **opposite** LFO's Freq, Phase, and Level.
No new UI panels, no new APVTS params beyond the extended choice list. Pure processBlock dispatch extension.

</domain>

<decisions>
## Implementation Decisions

### APVTS Extension
- `filterXMode` extends to 11 items: append "LFO-Y Freq", "LFO-Y Phase", "LFO-Y Level" (indices 8, 9, 10)
- `filterYMode` extends to 11 items: append "LFO-X Freq", "LFO-X Phase", "LFO-X Level" (indices 8, 9, 10)
- Old indices 0–7 are unchanged — no preset mapping shift, backward-compatible

### Dispatch Pattern (cross-LFO cases 8–10)
- Cross-LFO cases are **direct mirrors** of the corresponding same-LFO cases (4/5/6), just targeting the opposite LFO's parameters
- Do NOT change the existing case formulas — replicate them verbatim, pointing at the opposite LFO's APVTS params and display/override atomics
- X axis modes 8/9/10 write to: `lfoYRateDisplay_`/`lfoYSubdivMult_`/`lfoYRateOverride_`, `lfoYPhaseDisplay_`/`lfoYPhaseOverride_`, `lfoYLevelDisplay_`/`lfoYLevelOverride_`
- Y axis modes 8/9/10 write to: `lfoXRateDisplay_`/`lfoXSubdivMult_`/`lfoXRateOverride_`, `lfoXPhaseDisplay_`/`lfoXPhaseOverride_`, `lfoXLevelDisplay_`/`lfoXLevelOverride_`

### Sync Mode Behavior (cross-LFO Freq in sync)
- When xMode==8 ("LFO-Y Freq") and LFO-Y sync is ON: stick controls `lfoYSubdivMult_`, base = LFO-Y's current subdivIdx (not MOD FIX X)
- When yMode==8 ("LFO-X Freq") and LFO-X sync is ON: stick controls `lfoXSubdivMult_`, base = LFO-X's current subdivIdx
- Both use ±6 step range, same as existing cases 4

### Subdivision Multiplier Reset Guard
- Current code: `if (xMode != 4) lfoXSubdivMult_.store(1.0f, ...)` and `if (yMode != 4) lfoYSubdivMult_.store(1.0f, ...)`
- Update to: `if (xMode != 4 && xMode != 8) lfoXSubdivMult_.store(1.0f, ...)` — xMode==8 cross-drives lfoYSubdivMult_, but xMode still means lfoXSubdivMult_ should reset (no change needed for lfoX)
- Actually: xMode==8 drives lfoYSubdivMult_, so the guard for lfoYSubdivMult_ becomes: `if (yMode != 4 && xMode != 8) lfoYSubdivMult_.store(1.0f, ...)` — protect lfoYSubdivMult_ from reset when xMode==8
- Symmetric: protect lfoXSubdivMult_ from reset when yMode==8

### Playback Mode Guard
- Cross-LFO targets **fully respect** the target LFO's Playback mode state
- If target LFO (opposite axis) is in `LfoRecState::Playback`, the cross-LFO dispatch block must be skipped entirely: no override written, no display atomic updated
- Symmetric: applies to both X→LFO-Y and Y→LFO-X directions

### CC Suppression
- For modes 8–10 on either axis, `ccXnum`/`ccYnum` must resolve to -1 (no CC output)
- Existing logic already handles this via the ternary `(xMode==0)?74:...-1` — modes 8–10 naturally fall through to -1 since they are not in the 0–3 range

### Visual Tracking in PluginEditor
- When stick is routed to a cross-LFO target, the **opposite LFO's** slider thumb must track visually in the `timerCallback` at 30 Hz
- Obeys the existing Phase 24.1 display-atomic read pattern — `timerCallback` already reads `lfoYLevelDisplay_` etc. for LFO-Y sliders; no new polling required if the dispatch writes the correct display atomics

### Claude's Discretion
- Exact code placement and structure within the existing switch blocks
- Whether to use `else if` chains or extend the existing switch with new cases for 8/9/10
- Any minor refactoring needed to avoid code duplication between same-LFO and cross-LFO cases

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing dispatch block (PluginProcessor.cpp ~line 2025): `if (xMode >= 4 && xMode <= 7)` switch — extend to `>= 4 && <= 10`, add cases 8/9/10 mirroring Y-axis cases 4/5/6
- Override floats: `lfoXRateOverride_`, `lfoXPhaseOverride_`, `lfoXLevelOverride_`, `lfoYRateOverride_`, `lfoYPhaseOverride_`, `lfoYLevelOverride_` — all reset unconditionally at top of dispatch block (line 2010–2011), set only by active mode
- Display atomics: `lfoXRateDisplay_`, `lfoYRateDisplay_`, `lfoXPhaseDisplay_`, `lfoYPhaseDisplay_`, `lfoXLevelDisplay_`, `lfoYLevelDisplay_` — read by timerCallback for slider visual tracking
- `lfoXSubdivMult_` / `lfoYSubdivMult_` — reset logic at lines 2147–2148 needs updating

### Established Patterns
- `addChoice("filterXMode", ..., xModes, N)` in `createParameterLayout()` — just extend `xModes[]` array with 3 more strings
- `LfoRecState::Playback` check already used elsewhere in processBlock for grayout logic — same check needed in cross-LFO dispatch
- NormalisableRange `kLfoRange(0.01f, 20.0f, 0.0f, 0.35f)` defined as local static in existing cases — reuse

### Integration Points
- `createParameterLayout()` (PluginProcessor.cpp ~line 229–236): extend xModes/yModes arrays
- processBlock dispatch block (~line 2025–2148): extend range checks and add cases 8/9/10
- Subdivision mult reset logic (~line 2147–2148): add cross-axis guard conditions
- PluginEditor: Left Stick X/Y ComboBox options auto-populate from APVTS — no manual UI string changes needed if APVTS strings are set correctly

</code_context>

<specifics>
## Specific Ideas

- Cases 8/9/10 are exact copies of Y-axis cases 4/5/6 (for X→LFO-Y) and X-axis cases 4/5/6 (for Y→LFO-X)
- "Don't change the existing case" — user explicitly wants existing formulas preserved verbatim

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 34-cross-lfo-modulation-targets*
*Context gathered: 2026-03-06*
