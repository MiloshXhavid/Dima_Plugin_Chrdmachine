# Phase 38.3 — Sister LFO Attenuation Inline Slider — CONTEXT

**Phase goal:** When a Sister LFO cross-modulation dest is non-"None", shrink the Sister
combo to 50% width and reveal a bipolar attenuator slider (−1..+1) in the right 50%.
"None" = full-width combo, slider hidden.

---

## Decisions

### 1. New APVTS parameters
- `lfoXSisterAtten` — float, −1.0..+1.0, default **+1.0**
- `lfoYSisterAtten` — float, −1.0..+1.0, default **+1.0**
- Default +1.0 so effect is immediately audible at full depth when a dest is first selected.

### 2. Inline horizontal slider (no popup)
- Single horizontal `juce::Slider` (LinearHorizontal) per LFO channel.
- Lives in the right 50% of Row 1c (Sister row), same 22px height as the combo.
- Always visible (not hidden behind a click) when dest ≠ None.
- `addChildComponent()` at construction (starts hidden); `setVisible(true/false)` when
  combo selection changes.

### 3. Slider appearance
- **"ATT" label** painted inside the slider track (consistent with RATE/LEVEL/etc. labels).
- **Center-out bipolar fill** — fill drawn from center-zero outward (like a pan bar), making
  the bipolar range visually obvious.
- **Tooltip** for current numeric value — no always-visible text box (saves space).
- **Free-run** — no center snap, no special behavior at 0.0.

### 4. Combo label — keep "Sister:" prefix
- Items remain: `"Sister: None"`, `"Sister: Rate"`, `"Sister: Phase"`, `"Sister: Level"`,
  `"Sister: Dist"`.
- Rationale: at 50% panel width the combo is narrow; dropping the prefix risks ambiguity.

### 5. Split ratio
- **50/50** — combo takes left 50%, slider takes right 50% of Row 1c width.

### 6. Processor integration
- `applySisterMod()` lambda already multiplies the modulation signal before applying it.
- Change: multiply `modSignal` by `sisterAtten` param value before passing into the lambda
  (or inside the lambda). Negative values invert modulation direction.

---

## Code context

| Symbol | File | Notes |
|--------|------|-------|
| `lfoXSisterBox_`, `lfoYSisterBox_` | PluginEditor.h:467 | Existing combo boxes |
| `lfoXSisterAtt_`, `lfoYSisterAtt_` | PluginEditor.h:548,557 | Existing APVTS combo attachments |
| `applySisterMod` lambda | PluginProcessor.cpp:1001 | Multiply modSignal by atten here |
| `lfoXSister` / `lfoYSister` APVTS | PluginProcessor.cpp:373 | Existing params |
| Row 1c bounds | PluginEditor.cpp:~4000,4099 | `col.removeFromTop(22)` |
| `addChildComponent` pattern | PluginEditor.cpp | Used for custom CC labels in Phase 38.2 |

---

## Out of scope (deferred)
- Popup/vertical fader variant — considered, deferred, inline is sufficient.
- Label animation / transition when switching dest.
