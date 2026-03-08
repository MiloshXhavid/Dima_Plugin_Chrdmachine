# Phase 44 (38.3): Sister LFO Attenuation Inline Slider — Research

**Researched:** 2026-03-08
**Domain:** JUCE VST3 / PluginEditor UI — APVTS parameter addition + inline slider reveal
**Confidence:** HIGH — all findings verified directly from source code

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
1. New APVTS parameters: `lfoXSisterAtten` (float, −1.0..+1.0, default +1.0) and `lfoYSisterAtten` (float, −1.0..+1.0, default +1.0).
2. Inline horizontal `juce::Slider` (LinearHorizontal) per LFO channel, right 50% of Row 1c, 22px height. `addChildComponent()` at construction, `setVisible(true/false)` on combo selection change.
3. Appearance: "ATT" label inside slider track (same pattern as RATE/LEVEL/etc.), center-out bipolar fill, tooltip for current numeric value, no always-visible text box, no center snap.
4. Sister combo items stay as-is: "Sister: None" / "Sister: Rate" / "Sister: Phase" / "Sister: Level" / "Sister: Dist".
5. 50/50 split — combo takes left 50%, slider takes right 50% of Row 1c width.
6. Processor: multiply `modSignal` by `sisterAtten` param value before passing into `applySisterMod` (or inside it). Negative values invert.

### Claude's Discretion
- None defined in CONTEXT.md.

### Deferred Ideas (OUT OF SCOPE)
- Popup/vertical fader variant.
- Label animation/transition when switching dest.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| REQ-01 | Add `lfoXSisterAtten` + `lfoYSisterAtten` APVTS float params (−1..+1, default +1) | addFloat pattern in createParameterLayout(); sister params at PluginProcessor.cpp:372-376 |
| REQ-02 | Add `lfoXSisterAttenSlider_` + `lfoYSisterAttenSlider_` juce::Slider members (LinearHorizontal, NoTextBox) | Existing LFO slider members in PluginEditor.h:468-472; exact style pattern at PluginEditor.cpp:3108-3160 |
| REQ-03 | Add `lfoXSisterAttenAtt_` + `lfoYSisterAttenAtt_` SliderAttachment unique_ptrs | SliderAtt pattern at PluginEditor.h:549; standard `std::make_unique<SliderAtt>(p.apvts, "lfoXSisterAtten", slider)` |
| REQ-04 | `addChildComponent()` slider at construction; combo onChange sets `setVisible(dest != 1)` + calls `resized()` | Custom CC label pattern at PluginEditor.cpp:3196-3207 |
| REQ-05 | Row 1c layout split: left 50% = combo, right 50% = attenuation slider (when visible) | Current Row 1c at PluginEditor.cpp:3999-4001 and 4098-4100 |
| REQ-06 | "ATT" label painted left of slider track via `drawSliderLabel` pattern | drawSliderLabel lambda at PluginEditor.cpp:4543-4551; called at 4555-4565 |
| REQ-07 | Center-out bipolar fill in `drawLinearSlider` — detect slider range min=−1/max=+1, fill from center | Bipolar knob pattern at PluginEditor.cpp:76-101; drawLinearSlider at PluginEditor.cpp:263-317 |
| REQ-08 | Multiply `modSignal` by atten param before `applySisterMod` calls at PluginProcessor.cpp:1084 and 1144 | applySisterMod lambda at PluginProcessor.cpp:1003-1029 |
</phase_requirements>

---

## Summary

Phase 38.3 adds a bipolar attenuation slider to each Sister LFO combo row. The change spans three files: PluginProcessor.cpp (two new APVTS float params + atten read before two `applySisterMod` calls), PluginEditor.h (two new `juce::Slider` members + two `SliderAtt` unique_ptrs), and PluginEditor.cpp (construction, layout, paint).

The codebase already has exact precedents for every required change. The custom CC label pattern from Phase 38.2 provides the `addChildComponent` / `setVisible` / `resized()` combo-change flow. The `drawLinearSlider` in `PixelLookAndFeel` already implements a center-out delta fill (the "modAnchor" feature) and already has the red/blue color vocabulary; the bipolar fill for the atten slider follows the same existing rotary-knob bipolar arc logic. The `drawSliderLabel` lambda in `paint()` paints "ATT" in the 34px gap left of the slider bounds with zero new infrastructure.

**Primary recommendation:** Follow the custom CC label pattern (Phase 38.2) for visibility toggling and the existing `drawLinearSlider` bipolar pattern for fill — no new infrastructure required.

---

## Standard Stack

### Core
| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| `juce::AudioParameterFloat` | JUCE 8.0.4 | APVTS float param, −1..+1 | Same type used for lfoXLevel, lfoXPhase, etc. |
| `juce::AudioProcessorValueTreeState::SliderAttachment` | JUCE 8.0.4 | Binds slider to APVTS param | Aliased as `SliderAtt` throughout PluginEditor.h |
| `juce::Slider` (LinearHorizontal / NoTextBox) | JUCE 8.0.4 | Inline attenuation fader | Exact same style as lfoXRateSlider_, lfoXPhaseSlider_, etc. |
| `addChildComponent` + `setVisible` | JUCE 8.0.4 | Hidden-by-default component reveal | Established pattern for lfoXCustomCcLabel_ (Phase 38.2) |

---

## Architecture Patterns

### Pattern 1: APVTS float parameter addition (PluginProcessor.cpp)

**Location:** After the existing sister `AudioParameterChoice` declarations at PluginProcessor.cpp:372-376.

The `addFloat` lambda uses `NormalisableRange<float>(min, max, 0.01f)`. For −1..+1 with step 0.01:

```cpp
// Source: PluginProcessor.cpp:104-108 (addFloat lambda definition)
// After line 376 (after lfoYSister choice param):
addFloat("lfoXSisterAtten", "LFO X Sister Atten", -1.0f, 1.0f, 1.0f);
addFloat("lfoYSisterAtten", "LFO Y Sister Atten", -1.0f, 1.0f, 1.0f);
```

The `addFloat` lambda (PluginProcessor.cpp:104-108) internally calls:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    id, name, juce::NormalisableRange<float>(min, max, 0.01f), def));
```

Note: `lfoXSister` / `lfoYSister` use raw `layout.add(std::make_unique<AudioParameterChoice>(...))` at lines 374-375 — they were not added via `addChoice` because they used a local StringArray. The new atten params CAN use `addFloat` directly.

No new `ParamID::` namespace entry is required — the existing sister params `"lfoXSister"` / `"lfoYSister"` are also inline string literals (not in ParamID namespace). Use plain string literals `"lfoXSisterAtten"` / `"lfoYSisterAtten"` for consistency.

### Pattern 2: PluginEditor.h member declarations

**Location:** PluginEditor.h in the LFO panels section (around line 467) and the APVTS attachments section (around lines 548-560).

Slider members go alongside the existing LFO sliders:
```cpp
// After lfoXSisterBox_, lfoYSisterBox_ at line 467:
juce::Slider     lfoXSisterAttenSlider_, lfoYSisterAttenSlider_;
```

SliderAttachment unique_ptrs go in the "LFO X" and "LFO Y" attachment blocks:
```cpp
// After lfoXSisterAtt_ at line 548:
std::unique_ptr<SliderAtt> lfoXSisterAttenAtt_;

// After lfoYSisterAtt_ at line 557:
std::unique_ptr<SliderAtt> lfoYSisterAttenAtt_;
```

### Pattern 3: PluginEditor.cpp constructor — slider setup + addChildComponent

**Location:** After the `lfoXSisterBox_` onChange block (around PluginEditor.cpp:3218), mirror the custom CC label pattern:

```cpp
// Source pattern: PluginEditor.cpp:3180-3207 (custom CC label + onChange)
// Attenuation slider setup:
lfoXSisterAttenSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
lfoXSisterAttenSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
lfoXSisterAttenSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
lfoXSisterAttenSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
lfoXSisterAttenSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
lfoXSisterAttenSlider_.setTooltip("LFO X Sister Atten  -  scales the sister modulation depth (-1..+1); negative inverts");
addChildComponent(lfoXSisterAttenSlider_);
lfoXSisterAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXSisterAtten", lfoXSisterAttenSlider_);
```

**CRITICAL:** `addAndMakeVisible(lfoXSisterBox_)` at line 3217 must be changed to `addChildComponent` is NOT correct for the existing combo — the combo must remain `addAndMakeVisible`. Only the new slider uses `addChildComponent`.

The existing `lfoXSisterBox_` has no `onChange` handler currently. One must be added:

```cpp
// Add immediately after lfoXSisterAtt_ creation (line 3218), before the Sync button block:
lfoXSisterBox_.onChange = [this]() {
    const bool hasTarget = (lfoXSisterBox_.getSelectedId() != 1);  // 1 = "Sister: None"
    lfoXSisterAttenSlider_.setVisible(hasTarget);
    resized();
};
// Trigger once at load to sync initial visibility from saved APVTS state:
lfoXSisterBox_.onChange();
```

Same pattern for LFO Y around line 3404.

**Load-time initialisation:** The `.onChange()` call at the end (like `lfoYSyncBtn_.onClick()` at line 3446) is mandatory — without it, the slider visibility will be wrong after preset load.

### Pattern 4: resized() — Row 1c layout split

**Current code** (PluginEditor.cpp:3999-4001 for LFO X, 4098-4100 for LFO Y):
```cpp
// Row 1c: Sister dest ComboBox
lfoXSisterBox_.setBounds(col.removeFromTop(22));
col.removeFromTop(4);
```

**Replace with** a conditional split, matching the Row 1b custom-CC-label approach at lines 3989-3996:
```cpp
// Row 1c: Sister dest ComboBox (left 50%) + attenuation slider (right 50%, when visible)
{
    auto row = col.removeFromTop(22);
    if (lfoXSisterAttenSlider_.isVisible())
    {
        const int half = row.getWidth() / 2;
        lfoXSisterAttenSlider_.setBounds(row.removeFromRight(half));
    }
    else
    {
        lfoXSisterAttenSlider_.setBounds({});
    }
    lfoXSisterBox_.setBounds(row);
}
col.removeFromTop(4);
```

Note: `removeFromRight(half)` shrinks the `row` rect in-place, so the remaining `row` passed to `lfoXSisterBox_.setBounds(row)` is the left 50%.

### Pattern 5: paint() — "ATT" label + bipolar center-out fill

**"ATT" label** uses the existing `drawSliderLabel` lambda (PluginEditor.cpp:4543-4551), which draws text 34px left of the slider bounds. Call it where the other LFO slider labels are drawn (lines 4553-4566):

```cpp
// Source: PluginEditor.cpp:4543-4566 (drawSliderLabel calls)
if (!lfoXPanelBounds_.isEmpty())
{
    // ... existing calls ...
    if (lfoXSisterAttenSlider_.isVisible())
        drawSliderLabel(lfoXSisterAttenSlider_.getBounds(), "Att");
}
```

**Bipolar center-out fill in `drawLinearSlider`:** The existing implementation (PluginEditor.cpp:263-317) currently draws a unipolar left-to-zero fill. The atten slider range will be −1..+1, which `drawLinearSlider` receives as `sliderPos` in pixel coordinates. Add bipolar detection analogous to the existing rotary logic (PluginEditor.cpp:76-101):

```cpp
// Add after background track fill (after line 283), replacing unipolar fill section:
const float sMin = (float)slider.getMinimum();
const float sMax = (float)slider.getMaximum();
const bool isBipolar = (sMin < 0.0f && sMax > 0.0f &&
                        std::abs(sMin + sMax) < 0.01f);
if (isBipolar)
{
    const float centerPix = trackL + (float)width * 0.5f;
    const juce::Colour posClr(0xFFE03030);  // red
    const juce::Colour negClr(0xFF3060FF);  // blue
    if (sliderPos > centerPix + 1.0f)
    {
        g.setColour(posClr.withAlpha(0.75f));
        g.fillRect(juce::Rectangle<float>(centerPix, trackY, sliderPos - centerPix, trackH));
    }
    else if (sliderPos < centerPix - 1.0f)
    {
        g.setColour(negClr.withAlpha(0.75f));
        g.fillRect(juce::Rectangle<float>(sliderPos, trackY, centerPix - sliderPos, trackH));
    }
}
else
{
    // existing unipolar fill (line 286-287):
    g.setColour(slider.findColour(juce::Slider::trackColourId));
    g.fillRect(juce::Rectangle<float>(trackL, trackY, sliderPos - trackL, trackH));
}
```

This is safe: the only existing `LinearHorizontal` sliders with negative minimums are the atten sliders (all others have min ≥ 0). The `modAnchor` indicator block can remain unchanged beneath this.

### Pattern 6: Processor — multiply modSignal by atten before applySisterMod

**Current calls** (PluginProcessor.cpp:1084 and 1144):
```cpp
// Line 1084:
applySisterMod(xp, ySisterDest, prevYRampOut);
// Line 1144:
applySisterMod(yp, xSisterDest, xTarget);
```

**Read atten params** immediately after the existing sister dest reads at lines 995-996:
```cpp
// After line 996:
const float xSisterAtten = *apvts.getRawParameterValue("lfoXSisterAtten");
const float ySisterAtten = *apvts.getRawParameterValue("lfoYSisterAtten");
```

**Modify calls** to multiply:
```cpp
// Line 1084 replacement:
applySisterMod(xp, ySisterDest, prevYRampOut * ySisterAtten);
// Line 1144 replacement:
applySisterMod(yp, xSisterDest, xTarget * xSisterAtten);
```

Note the parameter naming: `xSisterDest` modulates Y's parameters (LFO X signal routes to LFO Y), so the atten for "LFO X Sister" is `xSisterAtten` applied at line 1144 where `xSisterDest` is used. Similarly `ySisterAtten` applies at line 1084. This is consistent with CONTEXT.md decision 6.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Param save/recall | Custom state serialisation | APVTS float param | JUCE handles XML state automatically |
| Combo→slider visibility link | Timer polling | `combo.onChange` + `setVisible` + `resized()` | Synchronous, zero overhead |
| Bipolar fill drawing | Separate LookAndFeel subclass | Add detection branch inside existing `drawLinearSlider` | Same LookAndFeel is already the global LAF |

---

## Common Pitfalls

### Pitfall 1: Load-time visibility not initialised
**What goes wrong:** Slider is always hidden (or always shown) after plugin load from preset because `onChange` is never called during APVTS state restore.
**Why it happens:** APVTS restores values by calling `setValue` on the parameter, not by calling `onChange` on the combo.
**How to avoid:** Call `lfoXSisterBox_.onChange()` once after `lfoXSisterAtt_` is created, like `lfoYSyncBtn_.onClick()` at PluginEditor.cpp:3446.
**Warning signs:** Slider visible when None is selected after DAW project reload.

### Pitfall 2: `addAndMakeVisible` vs `addChildComponent` confusion
**What goes wrong:** Slider is always visible (ignores setVisible) or combo disappears.
**Why it happens:** The combo must use `addAndMakeVisible` (it already does at line 3217). Only the new atten slider uses `addChildComponent`.
**How to avoid:** Do not change the existing `addAndMakeVisible(lfoXSisterBox_)` call.
**Warning signs:** Combo missing from UI, or atten slider always showing.

### Pitfall 3: Row width consumed before conditional split
**What goes wrong:** Slider bounds set to empty but combo gets wrong width, or vice versa.
**Why it happens:** `col.removeFromTop(22)` returns a copy of the row rect. The split must use `removeFromRight` on that row copy before assigning bounds.
**How to avoid:** Follow the exact pattern of the Row 1b custom-CC-label split (lines 3989-3996): `removeFromRight` then use the leftover `row` for the combo.
**Warning signs:** Combo takes full width when slider is visible; combo and slider overlap.

### Pitfall 4: Modulation signal pairing (which atten goes with which signal)
**What goes wrong:** Inverting LFO X Sister modulation instead of LFO Y Sister modulation.
**Why it happens:** Variable naming is cross-routing: `xSisterDest` means "LFO X's output modulates LFO Y", called at line 1144 — the correct atten is `xSisterAtten` (read from `"lfoXSisterAtten"`).
**How to avoid:** Pair `xSisterAtten` with the `xSisterDest` call (line 1144), and `ySisterAtten` with the `ySisterDest` call (line 1084).

### Pitfall 5: `drawLinearSlider` bipolar check affects other sliders
**What goes wrong:** Existing sliders (Rate 0.01..20, Phase 0..360, Level 0..1, Dist 0..1) accidentally get bipolar fill because the check is wrong.
**Why it happens:** The check `sMin < 0 && sMax > 0 && |sMin+sMax| < 0.01` only matches symmetric bipolar ranges. All existing LFO sliders have min ≥ 0. This is safe.
**How to avoid:** Keep the symmetry check `std::abs(sMin + sMax) < 0.01f`.

---

## Code Examples

### Full: existing addFloat call pattern (PluginProcessor.cpp:104-108, 341-346)
```cpp
// addFloat lambda definition (line 104):
auto addFloat = [&](const juce::String& id, const juce::String& name,
                    float min, float max, float def)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        id, name, juce::NormalisableRange<float>(min, max, 0.01f), def));
};
// Example usage (line 341):
addFloat(ParamID::lfoXLevel, "LFO X Level", 0.0f, 1.0f, 0.0f);
```

### Full: existing lfoXSisterAtt_ construction (PluginEditor.cpp:3218)
```cpp
lfoXSisterAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXSister", lfoXSisterBox_);
```

### Full: addChildComponent + onChange pattern (PluginEditor.cpp:3196-3207)
```cpp
// Custom CC label — exact model for atten slider:
lfoXCustomCcLabel_.setVisible(false);
addChildComponent(lfoXCustomCcLabel_);

lfoXCcDestBox_.onChange = [this]() {
    const bool customActive = (lfoXCcDestBox_.getSelectedId() == 20);
    lfoXCustomCcLabel_.setVisible(customActive);
    // ...
    resized();
};
```

### Full: Row 1b conditional split (PluginEditor.cpp:3989-3996)
```cpp
{
    auto row = col.removeFromTop(22);
    if (lfoXCustomCcLabel_.isVisible())
        lfoXCustomCcLabel_.setBounds(row.removeFromRight(60));
    else
        lfoXCustomCcLabel_.setBounds({});
    lfoXCcDestBox_.setBounds(row);
}
```

### Full: applySisterMod call sites (PluginProcessor.cpp:1083-1085 and 1143-1145)
```cpp
// LFO X: modulated by Y's signal (line 1084)
applySisterMod(xp, ySisterDest, prevYRampOut);   // ← multiply by ySisterAtten
xTarget = lfoX_.process(xp);

// LFO Y: modulated by X's signal (line 1144)
applySisterMod(yp, xSisterDest, xTarget);          // ← multiply by xSisterAtten
yTarget = lfoY_.process(yp);
```

---

## State of the Art

| Current | After Phase 38.3 | Impact |
|---------|-----------------|--------|
| Sister combo full-width, no depth control | Sister combo 50% + atten slider 50% when active | Bipolar attenuation without opening any popup |
| `modSignal` passed raw (×1.0 implied) | `modSignal * sisterAtten` passed | Negative values invert direction; zero kills modulation |

---

## Open Questions

None — all required code paths are fully verified in source.

---

## Validation Architecture

> `workflow.nyquist_validation` key is absent from `.planning/config.json` — treated as enabled.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 (CMake FetchContent, see `build/ChordJoystickTests*.cmake`) |
| Config file | `Tests/` directory; `CMakeLists.txt` target `ChordJoystickTests` |
| Quick run command | `cmake --build "C:/Users/Milosh Xhavid/get-shit-done/build" --config Release --target ChordJoystickTests && "C:/Users/Milosh Xhavid/get-shit-done/build/Release/ChordJoystickTests.exe"` |
| Full suite command | Same (all tests in one binary) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| REQ-01 | `lfoXSisterAtten` param exists, range −1..+1, default +1.0 | unit (APVTS param layout) | Catch2 suite | ❌ Wave 0 |
| REQ-02–03 | Slider + attachment constructed without crash | smoke (plugin instantiation) | Catch2 suite | ❌ Wave 0 |
| REQ-06 | `applySisterMod` scales signal by atten | unit (processor logic) | Catch2 suite | ❌ Wave 0 |
| REQ-04–05 | Visibility + layout correctness | manual-only | N/A — requires UI render | manual |
| REQ-07 | Bipolar fill in drawLinearSlider | manual-only | N/A — requires UI render | manual |

### Wave 0 Gaps
- [ ] `Tests/LfoSisterAttenTests.cpp` — covers REQ-01 (param range/default) and REQ-06 (modSignal scaling)
- [ ] REQ-02/03 covered by plugin instantiation test if one exists; check `Tests/ChordEngineTests.cpp` for existing fixture

---

## Sources

### Primary (HIGH confidence)
- `Source/PluginProcessor.cpp` lines 7-90 (ParamID namespace), 94-389 (createParameterLayout), 994-1145 (applySisterMod and call sites)
- `Source/PluginEditor.h` lines 458-570 (LFO member declarations and attachment typedefs)
- `Source/PluginEditor.cpp` lines 263-317 (drawLinearSlider), 3108-3160 (LFO X slider setup), 3180-3218 (custom CC label + sister combo), 3989-4001 (Row 1b/1c LFO X layout), 4060-4101 (Row 1b/1c LFO Y layout), 4542-4566 (drawSliderLabel calls)
- `Source/PluginEditor.cpp` lines 60-110 (bipolar rotary arc logic)

### Secondary (MEDIUM confidence)
- None needed — all findings from primary source.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all patterns verified from live source
- Architecture: HIGH — exact line numbers, exact code
- Pitfalls: HIGH — derived from actual code structure (addAndMakeVisible vs addChildComponent, load-time onChange)
- Processor integration: HIGH — both call sites found and naming verified

**Research date:** 2026-03-08
**Valid until:** Until any refactor of PluginEditor.cpp Row 1c layout or drawLinearSlider — stable otherwise.
