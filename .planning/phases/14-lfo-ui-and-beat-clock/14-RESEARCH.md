# Phase 14: LFO UI and Beat Clock — Research

**Researched:** 2026-02-26
**Domain:** JUCE C++ plugin UI — APVTS attachment, custom component layout, animated LED pulse
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Editor size:** `setSize(1120, 810)` — grows from 920 to 1120px wide; height unchanged at 810px.

**Layout (left → right):**
```
LEFT COLUMN (~460px)  |  LFO X (~130px)  |  LFO Y (~130px)  |  Joystick + knobs (~340px)
```

- Left column: unchanged (scale, chord intervals, octave offsets, trigger sources, looper, etc.)
- Two new LFO panel columns inserted between divider and joystick
- Joystick area shifts right; joystick pad retains 300×300 size
- `dividerX_` remains at ~460px from left edge (between left column and LFO X)

**LFO panel identity:**
- LFO X = left panel (X-axis LFO — fifth/tension voices)
- LFO Y = right panel (Y-axis LFO — root/third voices)
- Panels are top-aligned with the joystick pad; natural height (no fill to bottom)

**LFO panel visual style:**
- Same rounded-rect panel style as LOOPER / FILTER MOD / GAMEPAD (from Phase 11)
- Panel fill: `Clr::panel.brighter(0.12f)`
- Panel border: `Clr::accent.brighter(0.5f)`
- Corner radius: ~6–8px (match existing)
- Header label: knockout style floating on top border (same as `drawSectionPanel`)
- Enabled toggle: 8px LED circle in panel header, right of label text
  - Off: `Clr::gateOff` fill; On: `Clr::gateOn` (0xFF00E676)
  - Click LED or header area to toggle `lfoXEnabled` / `lfoYEnabled` APVTS param

**LFO panel control layout (top to bottom):**

| Row | Control | Notes |
|-----|---------|-------|
| 1 | Shape ComboBox | Full-width; 7 items (see waveform order below) |
| 2 | Rate slider | Horizontal; behavior changes by sync mode |
| 3 | Phase slider | 0.0–360.0; label "Phase" |
| 4 | Level slider | 0.0–2.0; label "Level" |
| 5 | Distortion slider | 0.0–1.0; label "Dist" |
| 6 | [SYNC] button | Full-width TextButton; accent-toggle style |

Row height: ~18–20px per slider row + 4px gap. Total panel height ~160–180px.
Label placement: short label left-aligned (~28px) + horizontal slider filling remainder.

**Rate slider modes:**
- Free: range 0.01–20 Hz, logarithmic (skew 0.2306f), value text = `"2.50 Hz"`
- Sync: same slider, snaps to 6 discrete subdivision steps, text = `"1/4"`, `"1/8"` etc.
- Label stays `"Rate"` in both modes; no layout shift on toggle

**Beat clock dot:**
- 8px circle inline/adjacent to `bpmDisplayLabel_` (implementation: drawn in `paint()` pass)
- On beat: fills `Clr::accent` (cyan-blue, `0xFF1E3A6E` brighter or highlight color per spec)
- Fade: `beatPulse_` float 0.0–1.0, decremented ~0.11 per 30 Hz tick (~9 frames to zero)
- Off: dim grey (e.g. `Clr::textDim` at low alpha)
- Source auto: DAW playing → uses `ppqPos` beat detection; DAW stopped → free BPM
- Signal: `std::atomic<bool> beatOccurred_` on processor — set by `processBlock`, read-and-reset by editor timer

**JoystickPad dot visual tracking:**
- When either LFO active (enabled=true, level>0): dot reads LFO-modulated X/Y
- Processor exposes: `std::atomic<float> modulatedJoyX_`, `modulatedJoyY_`
- JoystickPad::paint() reads these when LFO active, falls back to raw `joystickX/Y` when both LFOs off

### Claude's Discretion

- Exact implementation of beat clock dot: drawn in `PluginEditor::paint()` pass positioned to right of `bpmDisplayLabel_` bounds — OR a custom `juce::Component` overlaid on the label. Use whichever is simpler.
- Beat detection logic in `processBlock`: implementation detail of how `beatOccurred_` is set.

### Deferred Ideas (OUT OF SCOPE)

- LFO panel Enabled LED breathing animation when enabled but no notes playing
- Clicking the shape ComboBox showing a mini waveform preview
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| LFO-11 | LFO section is positioned to the left of the joystick pad; joystick shifted right to accommodate | Editor width increase to 1120px; right column split into [LFO X][LFO Y][joystick] via resized() refactor |
| CLK-01 | A flashing dot next to the Free BPM knob pulses once per beat (internal free tempo) | beatOccurred_ atomic + beatPulse_ float + paint() drawing adjacent to bpmDisplayLabel_ |
| CLK-02 | Beat dot follows DAW clock (pulses on DAW beat) when DAW sync is active | ppqPos integer-crossing detection in processBlock sets beatOccurred_; same dot renders regardless of source |
</phase_requirements>

---

## Summary

Phase 14 is a pure UI and cross-thread signaling phase — no new DSP. The LFO DSP (LfoEngine) and all 16 APVTS parameters were completed in Phases 12–13. Phase 14 wires controls to the existing parameters and adds two new processor atomics for cross-thread communication (`beatOccurred_` and `modulatedJoyX_/Y_`).

The central technical challenge is the layout refactor: `resized()` currently splits the window into a 50/50 left/right split at `colW/2 - 4`. Phase 14 changes the right half into three sub-columns: LFO X (~130px), LFO Y (~130px), and the joystick + controls area (~340px). The `dividerX_` stays at ~460px (left column width unchanged) but the joystick is pushed 260px further right within the right region.

The Rate slider in sync mode uses a `lfoXSubdiv` / `lfoYSubdiv` ComboBox APVTS parameter (not the Rate float parameter). In the UI, the Rate slider is attached to the float `lfoXRate` parameter in free mode. In sync mode, the same slider widget is repurposed: its range is changed to 0–5 (integer steps), its value-to-text function returns subdivision strings, and it is re-attached to `lfoXSubdiv` (a ComboBox parameter) via `SliderParameterAttachment`. The `[SYNC]` button attachment to `lfoXSync` drives this mode switch.

**Primary recommendation:** Implement both LFO panels as a single `LfoPanelComponent` helper class (or a data-driven setup function), not two fully duplicated code blocks. Use JUCE `AudioProcessorValueTreeState::SliderAttachment` / `ComboBoxAttachment` / `ButtonAttachment` throughout — no manual parameter callbacks needed for basic control binding.

---

## Standard Stack

### Core (already in project)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.4 | AudioProcessorEditor, APVTS, Component, Timer | Project standard |
| APVTS | built-in | Parameter attachment, persistence | All parameters use APVTS |
| LfoEngine | Phase 12 | LFO DSP | Already written, standalone |

### Supporting patterns (already established in project)

| Pattern | Purpose | When to Use |
|---------|---------|-------------|
| `juce::AudioProcessorValueTreeState::SliderAttachment` | Bind slider to float APVTS param | Rate, Phase, Level, Distortion sliders |
| `juce::AudioProcessorValueTreeState::ComboBoxAttachment` | Bind combo to choice APVTS param | Shape ComboBox, Subdiv ComboBox |
| `juce::AudioProcessorValueTreeState::ButtonAttachment` | Bind toggle to bool APVTS param | Enabled LED (via clickable component), Sync button |
| `juce::SliderParameterAttachment` | Bind slider to any APVTS param | Rate slider when in sync mode (attaches to ComboBox param) |
| `std::atomic<bool>` | Cross-thread signaling | beatOccurred_ (audio → UI) |
| `std::atomic<float>` | Cross-thread value sharing | modulatedJoyX_, modulatedJoyY_ |

### No new dependencies needed

This phase uses only what is already in the project. No new CMake dependencies, no new header includes beyond what PluginEditor.h already has.

---

## Architecture Patterns

### Existing resized() structure to understand

Current `resized()` (920px):
```cpp
void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(28);    // header bar
    area.removeFromBottom(60); // footer instructions
    const int rowH = area.getHeight();
    const int colW = area.getWidth();

    auto left  = area.removeFromLeft(colW / 2 - 4); // ~(920-16)/2 = ~452px
    area.removeFromLeft(8);
    auto right = area;  // ~(920-16)/2 = ~452px
    dividerX_ = right.getX();  // ~468px from left edge of window

    // RIGHT COLUMN — joystick centered in 'right', then knobs, pads, etc.
    const int padSize = juce::jmin(right.getWidth(), 300);
    // ...joystickPad_ centered in right...
}
```

Phase 14 new `resized()` for right column:
```cpp
// setSize(1120, 810) → area = 1104px wide after reduce(8)
// left = colW/2 - 4 → BUT: left column is still ~452px, not changing
// So: left column must be fixed width, not percentage

// APPROACH: Fix left column at 452px (or compute from colW at 920px baseline)
// left = area.removeFromLeft(452);
// area.removeFromLeft(8);    // gap between left col and LFO X
// auto lfoX = area.removeFromLeft(130);  // LFO X panel column
// area.removeFromLeft(4);
// auto lfoY = area.removeFromLeft(130);  // LFO Y panel column
// area.removeFromLeft(4);
// auto right = area;         // joystick + knobs, ~340px

// dividerX_ = lfoX.getX(); // still ~468px — unchanged for the divider line
```

**Key constraint:** The left column width must be preserved as a fixed pixel value, not `colW/2 - 4`, or the percentage will inflate at 1120px. The simplest approach: compute from the 920-base formula once, or hardcode 452px.

### Pattern 1: LFO Panel Setup (data-driven, no code duplication)

Create a helper struct and setup function rather than duplicating all control declarations:

```cpp
// ── In PluginEditor.h, for each LFO panel ─────────────────────────────────
struct LfoPanel
{
    juce::ComboBox   shapeBox;
    juce::Slider     rateSlider, phaseSlider, levelSlider, distSlider;
    juce::TextButton syncBtn;
    juce::Rectangle<int> panelBounds;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subdivAtt;
    std::unique_ptr<juce::SliderParameterAttachment>                        rateAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   phaseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   distAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   syncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   enabledAtt;
};
LfoPanel lfoXPanel_, lfoYPanel_;
```

Alternatively (and simpler given the existing code style): just declare all members directly with `lfoX_`/`lfoY_` prefix in PluginEditor.h, same pattern as other duplicated controls. The project consistently uses flat member declarations.

### Pattern 2: APVTS Attachments for LFO Controls

```cpp
// Source: PluginProcessor.cpp ParamID namespace (verified from source)
// APVTS parameter IDs established in Phase 13:

// Shape ComboBox — ComboBoxAttachment to "lfoXWaveform" (choice param, 7 items)
lfoXShapeBox_.addItem("Sine",      1);
lfoXShapeBox_.addItem("Triangle",  2);
lfoXShapeBox_.addItem("Saw Up",    3);
lfoXShapeBox_.addItem("Saw Down",  4);
lfoXShapeBox_.addItem("Square",    5);
lfoXShapeBox_.addItem("S&H",       6);
lfoXShapeBox_.addItem("Random",    7);
lfoXShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXWaveform", lfoXShapeBox_);

// Rate slider (free mode) — SliderAttachment to "lfoXRate" (float, NormalisableRange 0.01..20, skew=0.2306f)
// Set range manually to match APVTS param, then attach:
lfoXRateSlider_.setRange(0.01, 20.0);  // display only; attachment will override
lfoXRateAtt_free_ = std::make_unique<SliderAtt>(p.apvts, "lfoXRate", lfoXRateSlider_);

// Phase slider — SliderAttachment to "lfoXPhase" (float 0..360)
lfoXPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXPhase", lfoXPhaseSlider_);

// Level slider — SliderAttachment to "lfoXLevel" (float 0..2)
lfoXLevelAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXLevel", lfoXLevelSlider_);

// Distortion slider — SliderAttachment to "lfoXDistortion" (float 0..1)
lfoXDistAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXDistortion", lfoXDistSlider_);

// Sync button — ButtonAttachment to "lfoXSync" (bool)
lfoXSyncAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXSync", lfoXSyncBtn_);

// Enabled — no ButtonAttachment; LED is a clickable area drawn in paint(),
// backed by manually calling parameter setValueNotifyingHost(), or use a
// hidden ToggleButton + ButtonAttachment.
```

### Pattern 3: Rate Slider Sync Mode Toggle

The Rate slider must behave differently based on `lfoXSync` state. The APVTS has two separate parameters: `lfoXRate` (float, free Hz) and `lfoXSubdiv` (choice, 6 steps).

**Implementation approach: swap attachment on sync toggle**

```cpp
// When SYNC OFF: rate slider attached to "lfoXRate" (float)
// When SYNC ON:  rate slider attached to "lfoXSubdiv" (choice, 0-5 integer)
//                with valueToText mapping 0→"1/1", 1→"1/2", 2→"1/4", etc.

// Subdivision strings (exact, from PluginProcessor.cpp line 278):
// Index: 0="1/1", 1="1/2", 2="1/4", 3="1/8", 4="1/16", 5="1/32"

void onSyncToggled(bool syncOn)
{
    lfoXRateFreeAtt_.reset();    // detach current
    lfoXRateSyncAtt_.reset();

    if (syncOn)
    {
        lfoXRateSlider_.setRange(0.0, 5.0, 1.0);   // 6 integer steps
        lfoXRateSlider_.setNumDecimalPlacesToDisplay(0);
        lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
            static const char* names[] = {"1/1","1/2","1/4","1/8","1/16","1/32"};
            int i = juce::jlimit(0, 5, (int)std::round(v));
            return names[i];
        };
        // Attach to the choice param via SliderParameterAttachment
        // (ComboBoxAttachment not usable on a Slider; must use SliderParameterAttachment)
        if (auto* p = proc_.apvts.getParameter("lfoXSubdiv"))
            lfoXRateSyncAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                *p, lfoXRateSlider_);
    }
    else
    {
        lfoXRateSlider_.setRange(0.01, 20.0);
        lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
            return juce::String(v, 2) + " Hz";
        };
        if (auto* p = proc_.apvts.getParameter("lfoXRate"))
            lfoXRateFreeAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                *p, lfoXRateSlider_);
    }
}
```

This swap is triggered from the `lfoXSync` APVTS parameter listener (via `AudioProcessorValueTreeState::Listener`) or from the Sync button `onClick`.

**Important:** `juce::SliderParameterAttachment` is the lower-level attachment that works with any `AudioProcessorParameter&`. `AudioProcessorValueTreeState::SliderAttachment` (typedef `SliderAtt`) is a convenience wrapper. Both are available in JUCE 8.0.4.

### Pattern 4: Beat Clock Detection

The processor must detect integer PPQ crossings (beat boundaries) and expose the signal to the UI thread.

**New processor members required (PluginProcessor.h):**

```cpp
// Beat-clock cross-thread signals — audio thread writes, UI timer reads
std::atomic<bool>  beatOccurred_       { false };  // set on each beat crossing
std::atomic<float> modulatedJoyX_      { 0.0f  };  // post-LFO X (clamped -1..1)
std::atomic<float> modulatedJoyY_      { -1.0f };  // post-LFO Y (clamped -1..1)
```

**processBlock beat detection logic:**

```cpp
// After LFO section has computed chordP.joystickX/Y (post-LFO):
modulatedJoyX_.store(chordP.joystickX, std::memory_order_relaxed);
modulatedJoyY_.store(chordP.joystickY, std::memory_order_relaxed);

// Beat detection — integer PPQ crossing
// Free tempo path: derive from sampleCount_
// DAW path: ppqPos integer crossing
{
    // DAW playing path:
    if (isDawPlaying && ppqPos >= 0.0)
    {
        const double prevPpq = ppqPos - (lp.bpm * blockSize / sampleRate_) / 60.0;
        if ((long long)ppqPos > (long long)std::max(0.0, prevPpq))
            beatOccurred_.store(true, std::memory_order_relaxed);
    }
    else
    {
        // Free tempo path: detect crossing using free BPM and sample count
        // Use existing effectiveBpm_ and sampleRate_ to compute beat period
        const float bpm = effectiveBpm_.load(std::memory_order_relaxed);
        const double beatsPerSample = bpm / (60.0 * sampleRate_);
        const double prevBeat = sampleCount_ * beatsPerSample;  // sampleCount_ before increment
        const double currBeat = (sampleCount_ + blockSize) * beatsPerSample;
        if ((long long)currBeat > (long long)prevBeat)
            beatOccurred_.store(true, std::memory_order_relaxed);
        // Note: sampleCount_ must be maintained in processBlock (already is for LFO)
    }
}
```

**Editor timerCallback beat pulse:**

```cpp
// In PluginEditor.h:
float beatPulse_ = 0.0f;

// In timerCallback():
if (proc_.beatOccurred_.exchange(false, std::memory_order_relaxed))
    beatPulse_ = 1.0f;
beatPulse_ = juce::jmax(0.0f, beatPulse_ - 0.11f);  // ~9 frames to zero at 30 Hz

// Repaint the bpmDisplayLabel_ area to update the dot:
repaint(bpmDisplayLabel_.getBounds().expanded(12, 4));
```

**paint() beat dot:**

```cpp
// Drawn in PluginEditor::paint(), after bpmDisplayLabel_ is laid out
const auto labelB = bpmDisplayLabel_.getBounds();
const int dotX = labelB.getRight() + 4;   // 4px right of label text area
const int dotY = labelB.getCentreY() - 4;
const juce::Colour dotClr = Clr::accent.brighter(2.5f * beatPulse_)
                                        .withAlpha(0.3f + 0.7f * beatPulse_);
g.setColour(dotClr);
g.fillEllipse((float)dotX, (float)dotY, 8.0f, 8.0f);
```

### Pattern 5: JoystickPad Dot Visual Tracking

Current JoystickPad::paint() (line 353–354):
```cpp
const float cx = (proc_.joystickX.load() + 1.0f) * 0.5f * b.getWidth()  + b.getX();
const float cy = (1.0f - (proc_.joystickY.load() + 1.0f) * 0.5f) * b.getHeight() + b.getY();
```

Phase 14 replacement:
```cpp
// Read LFO-modulated position if either LFO is active
const bool lfoXActive = *proc_.apvts.getRawParameterValue("lfoXEnabled") > 0.5f
                     && *proc_.apvts.getRawParameterValue("lfoXLevel")   > 0.0f;
const bool lfoYActive = *proc_.apvts.getRawParameterValue("lfoYEnabled") > 0.5f
                     && *proc_.apvts.getRawParameterValue("lfoYLevel")   > 0.0f;

const float displayX = (lfoXActive || lfoYActive)
    ? proc_.modulatedJoyX_.load(std::memory_order_relaxed)
    : proc_.joystickX.load(std::memory_order_relaxed);
const float displayY = (lfoXActive || lfoYActive)
    ? proc_.modulatedJoyY_.load(std::memory_order_relaxed)
    : proc_.joystickY.load(std::memory_order_relaxed);

const float cx = (displayX + 1.0f) * 0.5f * b.getWidth()  + b.getX();
const float cy = (1.0f - (displayY + 1.0f) * 0.5f) * b.getHeight() + b.getY();
```

Note: `JoystickPad` already has `PluginProcessor& proc_` so it can access `proc_.apvts` and the new atomics.

### Pattern 6: Panel Drawing in paint()

Existing `drawSectionPanel` lambda draws border-only (no fill) with knockout label on top border. For LFO panels, the CONTEXT.md specifies a fill of `Clr::panel.brighter(0.12f)`. Either extend `drawSectionPanel` to accept a fill flag, or draw the LFO panels explicitly:

```cpp
// LFO panel drawing (in PluginEditor::paint()):
auto drawLfoPanel = [&](juce::Rectangle<int> bounds, const juce::String& title,
                         bool enabled, float ledBrightness)
{
    if (bounds.isEmpty()) return;
    const auto fb = bounds.toFloat();

    // Fill (LFO panels get a fill unlike LOOPER panel)
    g.setColour(Clr::panel.brighter(0.12f));
    g.fillRoundedRectangle(fb, 7.0f);

    // Border
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRoundedRectangle(fb.reduced(0.5f), 7.0f, 1.5f);

    // Knockout header text
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
    const int textW = g.getCurrentFont().getStringWidth(title) + 10;
    const int textH = 12;
    const int textX = (int)fb.getX() + 8;  // left-aligned for narrow panel
    const int textY = (int)fb.getY() - textH / 2;
    g.setColour(Clr::panel.brighter(0.12f));  // erase border behind label
    g.fillRect(textX, textY, textW, textH);
    g.setColour(Clr::textDim);
    g.drawText(title, textX, textY, textW, textH, juce::Justification::centred);

    // Enabled LED (8px circle to right of label)
    const int ledX = textX + textW + 4;
    const int ledY = textY + (textH - 8) / 2;
    g.setColour(enabled ? Clr::gateOn : Clr::gateOff);
    g.fillEllipse((float)ledX, (float)ledY, 8.0f, 8.0f);
};
```

### Anti-Patterns to Avoid

- **Percentage-based left column width at 1120px:** Using `colW / 2 - 4` at 1120px gives ~548px instead of ~452px. Fix: use `460` as a constant or compute from the 920-base formula and clamp.
- **Attaching both rate attachments simultaneously:** Destroy the old attachment before creating the new one. APVTS attachments register as parameter listeners; double-registration corrupts state.
- **Writing to `joystickX`/`joystickY` atomics from audio thread for LFO display:** The decision from Phase 13 was explicit — LFO offsets stay local in `buildChordParams()` scope. Use separate `modulatedJoyX_/Y_` atomics for UI.
- **Calling `apvts.getRawParameterValue()` from audio thread inside JoystickPad::paint():** JoystickPad::paint() runs on the message thread. `getRawParameterValue()` returns `std::atomic<float>*` which is safe to load from message thread. This is acceptable.
- **Using `setSize` arguments that are wrong:** PluginEditor constructor calls `setSize(920, 810)` — must change to `setSize(1120, 810)`.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter persistence | Custom save/load for LFO controls | APVTS attachments | APVTS auto-handles preset save/load |
| Slider-to-subdivision mapping | Custom mapping object | `textFromValueFunction` lambda on slider | JUCE built-in mechanism, already used for other sliders |
| Beat tempo clock | Separate timer / thread | `beatOccurred_` atomic + existing 30 Hz editor timer | Project already has timerCallback at 30 Hz |
| LFO panel component | Full custom `juce::Component` subclass | Draw in `paint()` + member controls laid out in `resized()` | Consistent with rest of editor (LOOPER, ARP sections use same approach) |

---

## Exact APVTS Parameter IDs (verified from PluginProcessor.cpp)

All parameters created in Phase 13, in the `createParameterLayout()` function:

| Parameter ID | Type | Range / Options | Default | Notes |
|---|---|---|---|---|
| `"lfoXEnabled"` | bool | true/false | false | Enabled toggle |
| `"lfoYEnabled"` | bool | true/false | false | Enabled toggle |
| `"lfoXWaveform"` | choice | 7 items (see below) | 0 (Sine) | Shape ComboBox |
| `"lfoYWaveform"` | choice | 7 items | 0 (Sine) | Shape ComboBox |
| `"lfoXRate"` | float | 0.01..20.0, skew=0.2306f | 1.0 | Rate slider (free mode) |
| `"lfoYRate"` | float | 0.01..20.0, skew=0.2306f | 1.0 | Rate slider (free mode) |
| `"lfoXLevel"` | float | 0.0..2.0 | 0.0 | Level slider |
| `"lfoYLevel"` | float | 0.0..2.0 | 0.0 | Level slider |
| `"lfoXPhase"` | float | 0.0..360.0 | 0.0 | Phase slider |
| `"lfoYPhase"` | float | 0.0..360.0 | 0.0 | Phase slider |
| `"lfoXDistortion"` | float | 0.0..1.0 | 0.0 | Distortion slider |
| `"lfoYDistortion"` | float | 0.0..1.0 | 0.0 | Distortion slider |
| `"lfoXSync"` | bool | true/false | false | Sync button |
| `"lfoYSync"` | bool | true/false | false | Sync button |
| `"lfoXSubdiv"` | choice | 6 items (see below) | 2 (1/4) | Rate slider (sync mode) |
| `"lfoYSubdiv"` | choice | 6 items | 2 (1/4) | Rate slider (sync mode) |

**Waveform ComboBox items (exact order from source — MUST match `Waveform` enum):**
```
Index 0 = "Sine"      → Waveform::Sine
Index 1 = "Triangle"  → Waveform::Triangle
Index 2 = "Saw Up"    → Waveform::SawUp
Index 3 = "Saw Down"  → Waveform::SawDown
Index 4 = "Square"    → Waveform::Square
Index 5 = "S&H"       → Waveform::SH
Index 6 = "Random"    → Waveform::Random
```
JUCE ComboBox items are 1-indexed; item 1 maps to APVTS value 0.

**Subdivision choice items (exact order from source):**
```
Index 0 = "1/1"   → kLfoSubdivBeats[0] = 4.0 beats
Index 1 = "1/2"   → kLfoSubdivBeats[1] = 2.0 beats
Index 2 = "1/4"   → kLfoSubdivBeats[2] = 1.0 beats  ← default
Index 3 = "1/8"   → kLfoSubdivBeats[3] = 0.5 beats
Index 4 = "1/16"  → kLfoSubdivBeats[4] = 0.25 beats
Index 5 = "1/32"  → kLfoSubdivBeats[5] = 0.125 beats
```

---

## Common Pitfalls

### Pitfall 1: Left Column Width Inflation at 1120px
**What goes wrong:** Current code uses `colW / 2 - 4` for left column width. At 1120px window width with `reduced(8)`, `colW` = 1104px, so left column becomes 548px — 96px wider than the intended 452px. All scale/chord controls reflow incorrectly.
**Why it happens:** Percentage-based split assumed equal halves; Phase 14 breaks the symmetry.
**How to avoid:** Use a fixed constant `constexpr int kLeftColW = 452;` in `resized()`, or compute as `(920 - 16) / 2 - 4 = 448` (use the old 920-based formula). `dividerX_` remains set to `right.getX()` (which is at ~468px from window left edge).
**Warning signs:** Scale keyboard or chord knobs appear wider/overlapping after resize.

### Pitfall 2: Double APVTS Attachment on Rate Slider
**What goes wrong:** When toggling Sync mode, the new `SliderParameterAttachment` is created without resetting the old one first. Both attachments call `slider.valueChanged()`, causing value conflicts and listener loops.
**Why it happens:** Forgetting to `reset()` the old unique_ptr before constructing the new one.
**How to avoid:** Always `lfoXRateAtt_.reset()` before `lfoXRateAtt_ = std::make_unique<...>(...)`.
**Warning signs:** Rate slider jumps to wrong value on Sync toggle; assertion in debug build from double-listener.

### Pitfall 3: ComboBox Item IDs vs APVTS Choice Index
**What goes wrong:** JUCE ComboBox item IDs start at 1 (not 0). A ComboBoxAttachment maps APVTS integer value `n` to ComboBox item with ID `n+1`. If the ComboBox items are added in the wrong order or with wrong IDs, the shape displayed does not match the waveform rendered by the DSP.
**Why it happens:** LFO waveform enum values are 0-based; ComboBox items are 1-based.
**How to avoid:** Add items with `addItem("Sine", 1)` through `addItem("Random", 7)` in the exact enum order. The ComboBoxAttachment handles the +1 offset automatically.
**Warning signs:** Selecting "Triangle" in the UI produces sine-wave behavior.

### Pitfall 4: JoystickPad Reads Stale Modulated Values at Startup
**What goes wrong:** `modulatedJoyX_` and `modulatedJoyY_` are initialized to `{0.0f}` and `{-1.0f}` but the processor's `joystickX` starts at 0.0f and `joystickY` at -1.0f. If the UI reads modulated values before the first `processBlock`, these match; but if LFOs are enabled at startup with a preset load, the dot may show wrong position for 1–2 frames.
**Why it happens:** Race between preset load and first processBlock call.
**How to avoid:** Initialize `modulatedJoyX_ {0.0f}` and `modulatedJoyY_ {-1.0f}` in PluginProcessor.h (matching joystickX/Y defaults). The 1-frame discrepancy is visually imperceptible.
**Warning signs:** Not a significant problem; no action needed beyond correct initialization.

### Pitfall 5: beatOccurred_ Pulse-On-Every-Block at Low BPM
**What goes wrong:** The beat crossing detection logic fires on every block at very low BPM values if the block-size-to-beat-ratio is larger than 1. e.g., at 30 BPM with a 512-sample block at 44100 Hz, beatsPerBlock ≈ 0.006, so crossings are rare — but if the formula has a bug (e.g., integer division instead of floor), it fires every block.
**Why it happens:** Off-by-one or wrong type in the integer-floor comparison.
**How to avoid:** Use `static_cast<long long>()` for floor, compare previous and current beat integer indices. Test at 60 BPM: should fire exactly once per second (once per ~59 blocks at 512-sample/44100Hz).
**Warning signs:** Beat dot flashes multiple times per beat or continuously.

### Pitfall 6: Phase Slider Range in Degrees vs 0..1
**What goes wrong:** `lfoXPhase` APVTS parameter is defined as 0.0..360.0 (degrees). The slider is attached with `SliderAttachment` which will display in degrees. But `LfoEngine::ProcessParams::phaseShift` expects 0.0..1.0 (normalized). The conversion happens in processBlock: `xp.phaseShift = *apvts.getRawParameterValue(ParamID::lfoXPhase) / 360.0f`. The Phase slider must be set to display 0–360, not 0–1.
**Why it happens:** Confusion between UI range (degrees) and DSP range (normalized). The conversion is in processBlock, already correct.
**How to avoid:** Set slider range to 0..360 via attachment (APVTS defines 0..360). Tooltip: "Phase offset (0–360°)". No conversion needed in the UI — the attachment handles it.

### Pitfall 7: Level Range 0..2 (not 0..1)
**What goes wrong:** The APVTS `lfoXLevel` parameter range is 0.0..2.0 (allows overdrive up to 2x). The slider must be configured for 0..2, not 0..1. At level=1.0 the LFO output equals the full joystick range; at 2.0 it is double (clamped in processBlock).
**Why it happens:** Assuming level is normalized 0..1 when it's actually 0..2.
**How to avoid:** Use `SliderAttachment` which inherits the range from the APVTS parameter definition automatically. Do not manually call `setRange()` on sliders that use `SliderAttachment`.
**Warning signs:** Level slider only goes to halfway position when LFO is at full depth.

---

## Code Examples

### APVTS Attachment setup (verified patterns from existing editor)

```cpp
// Source: PluginEditor.cpp, existing ComboAtt / ButtonAtt / SliderAtt patterns

// ComboBox attachment — exact pattern used for arpSubdiv, loopSubdiv, trigSrc etc.
lfoXShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXWaveform", lfoXShapeBox_);

// SliderAttachment — exact pattern used for arpGateTime, filterXAtten etc.
lfoXPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXPhase", lfoXPhaseSlider_);

// ButtonAttachment — exact pattern used for arpEnabled
lfoXSyncAtt_  = std::make_unique<ButtonAtt>(p.apvts, "lfoXSync",   lfoXSyncBtn_);
```

### SliderParameterAttachment (for rate slider sync mode swap)

```cpp
// Source: JUCE 8.0.4 API — juce_AudioProcessorValueTreeState.h
// juce::SliderParameterAttachment works with AudioProcessorParameter directly

auto* param = proc_.apvts.getParameter("lfoXSubdiv");  // returns AudioProcessorParameter*
if (param)
{
    lfoXRateSyncAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *param, lfoXRateSlider_, nullptr  // nullptr = no UndoManager
    );
}
```

### Existing styleLabel / styleCombo / styleButton helpers (from PluginEditor.cpp)

```cpp
// Source: PluginEditor.cpp lines 769–790 (static helpers)
// These are file-scope static functions, callable anywhere in PluginEditor.cpp

styleLabel(lfoXRateLabel_, "Rate");    // sets text, font=11, textColour=Clr::textDim
styleCombo(lfoXShapeBox_);             // sets background/text/outline colours
styleButton(lfoXSyncBtn_);             // sets buttonColour, buttonOnColour=Clr::highlight
```

### Horizontal slider setup (verified pattern for linear sliders)

```cpp
// Source: PluginEditor.cpp line 1384 (arpGateTimeKnob_ setup)
lfoXRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
lfoXRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
// PixelLAF drawLinearSlider handles rendering — only LinearHorizontal style supported
// (vertical falls back to LookAndFeel_V4 — do not use)
```

### drawSectionPanel lambda (existing, for reference)

```cpp
// Source: PluginEditor.cpp line 1798
// Draws border + knockout title on top edge. Used for LOOPER panel.
// Phase 14 draws LFO panels with fill added (see drawLfoPanel above).
auto drawSectionPanel = [&](juce::Rectangle<int> bounds, const juce::String& title)
{
    if (bounds.isEmpty()) return;
    const auto fb = bounds.toFloat().reduced(1.0f, 0.0f);
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRoundedRectangle(fb.reduced(0.5f), 7.0f, 1.5f);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
    const int textW = g.getCurrentFont().getStringWidth(title) + 10;
    const int textH = 12;
    const int textX = (int)(fb.getCentreX()) - textW / 2;
    const int textY = (int)fb.getY() - textH / 2;
    g.setColour(Clr::bg);
    g.fillRect(textX, textY, textW, textH);
    g.setColour(Clr::textDim);
    g.drawText(title, textX, textY, textW, textH, juce::Justification::centred);
};
```

---

## Layout Geometry Reference

### Current editor (920px) — right column analysis

```
Window: 920 × 810
Area after reduce(8): 904 × 794, from x=8
Header removed (28px): content area starts at y=36
Footer removed (60px): content height = 734px

Left col: area.removeFromLeft(colW/2 - 4) = 904/2 - 4 = 448px wide, x=8
Gap: 8px removed
Right col: remaining 904 - 448 - 8 = 448px wide, x=464

dividerX_ = 464 (right.getX())
joystick pad: padSize = jmin(448, 300) = 300px, centered in 448px right col
  → padX = 464 + (448-300)/2 = 464 + 74 = 538, width=300
```

### New editor (1120px) — layout target

```
Window: 1120 × 810
Area after reduce(8): 1104 × 794, from x=8

Left col: FIXED at 448px (same as before), x=8
Gap: 8px
LFO X col: 130px, x = 8 + 448 + 8 = 464
Gap: 4px
LFO Y col: 130px, x = 464 + 130 + 4 = 598
Gap: 4px
Joystick + right col: 1104 - 448 - 8 - 130 - 4 - 130 - 4 = 380px, x=736

dividerX_ = 464 (left.getX() + width + gap = same as before — left column unchanged)
joystick pad: padSize = jmin(380, 300) = 300px
  → centered in 380px: padX = 736 + (380-300)/2 = 736 + 40 = 776
```

**resized() changes needed:**
1. `setSize(1120, 810)` in constructor
2. In `resized()`: replace `auto left = area.removeFromLeft(colW / 2 - 4)` with `auto left = area.removeFromLeft(448)`
3. Add `auto lfoXCol = area.removeFromLeft(130)` and gap/lfoYCol after the existing left/gap split
4. `auto right = area` then proceeds as before (joystick centering etc.)
5. `dividerX_ = lfoXCol.getX()` — same pixel position as before

---

## Processor Changes Required

Phase 13 is complete but did NOT add `beatOccurred_`, `modulatedJoyX_`, or `modulatedJoyY_`. These must be added in Phase 14:

### PluginProcessor.h additions

```cpp
// In public section (readable by UI thread):
std::atomic<bool>  beatOccurred_  { false };
std::atomic<float> modulatedJoyX_ { 0.0f  };
std::atomic<float> modulatedJoyY_ { -1.0f };
```

### PluginProcessor.cpp additions

After the LFO section in `processBlock` (after `chordP.joystickX/Y` are updated with LFO offset):

```cpp
// Expose modulated joystick position for JoystickPad visual tracking
modulatedJoyX_.store(chordP.joystickX, std::memory_order_relaxed);
modulatedJoyY_.store(chordP.joystickY, std::memory_order_relaxed);

// Beat clock detection
{
    static double prevBeatCount = -1.0;
    double currentBeatCount;
    if (isDawPlaying && ppqPos >= 0.0)
        currentBeatCount = ppqPos;  // DAW: use ppqPos directly
    else
    {
        const float bpm = effectiveBpm_.load(std::memory_order_relaxed);
        currentBeatCount = (bpm / 60.0) * sampleCount_ / sampleRate_;
    }
    if (prevBeatCount >= 0.0 && std::floor(currentBeatCount) > std::floor(prevBeatCount))
        beatOccurred_.store(true, std::memory_order_relaxed);
    prevBeatCount = currentBeatCount;
}
```

Note: `sampleCount_` is already maintained in LfoEngine per-instance. The processor needs its own sample counter, or derive from looper's. Simplest: add `int64_t sampleCounter_ = 0` to PluginProcessor private and increment by `blockSize` each processBlock.

---

## Open Questions

1. **sampleCount_ in PluginProcessor vs LfoEngine**
   - What we know: LfoEngine has its own `sampleCount_` (private). The processor does not have one.
   - What's unclear: Should the processor add its own, or query the LFO engine's? LfoEngine's counter is private.
   - Recommendation: Add `int64_t sampleCounter_ = 0` to PluginProcessor private section; increment by `blockSize` each processBlock. Reset on transport stop (same as lfoX_.reset() call).

2. **Enabled LED click target — custom hitTest or hidden ToggleButton**
   - What we know: CONTEXT.md says "click LED or header area toggles". LED is drawn in paint() at panel header.
   - What's unclear: The cleanest JUCE approach — override `mouseDown` on the panel component, or use a transparent 8px ToggleButton placed over the LED area.
   - Recommendation: Use a hidden `juce::ToggleButton` (setAlpha(0) or zero-size) with a ButtonAttachment to `lfoXEnabled`, then respond to `mouseDown` at the LED position in the editor or in a helper component. If LFO panels are plain-member (not a Component subclass), use `PluginEditor::mouseDown()` with bounds check on LED rect.

3. **Label placement in 130px panel — left label + slider width**
   - What we know: Label ~28px wide, slider fills remainder. At 130px total with 8px left/right padding: slider = 130 - 16 (padding) - 28 (label) - 4 (gap) = 82px wide.
   - What's unclear: Whether 82px is enough for a usable slider. At 18px height with 6px track height, this renders fine.
   - Recommendation: 82px is sufficient. The existing `drawLinearSlider` works at any width >= 20px.

---

## Sources

### Primary (HIGH confidence)
- `Source/PluginProcessor.cpp` lines 65–83 — exact APVTS parameter IDs, all 16 LFO params verified
- `Source/PluginProcessor.cpp` lines 248–281 — parameter ranges, subdivision choices, waveform choices
- `Source/PluginProcessor.h` — complete processor public API, confirmed no beatOccurred_/modulatedJoy atomics exist yet
- `Source/PluginEditor.h` — complete editor member declarations, confirmed no LFO UI members exist yet
- `Source/PluginEditor.cpp` lines 1407–1429 — resized() left/right split logic (verified math above)
- `Source/PluginEditor.cpp` lines 204–231 — drawLinearSlider (LinearHorizontal only supported)
- `Source/PluginEditor.cpp` lines 769–790 — styleLabel, styleCombo, styleButton helpers
- `Source/PluginEditor.cpp` lines 1798–1821 — drawSectionPanel lambda pattern
- `Source/LfoEngine.h` lines 16, 42–46 — Waveform enum order, ProcessParams fields

### Secondary (MEDIUM confidence)
- JUCE 8.0.4 `SliderParameterAttachment` — works with `AudioProcessorParameter&` directly (consistent with JUCE 7+ API; project uses it via `juce_audio_processors`)

---

## Metadata

**Confidence breakdown:**
- APVTS parameter IDs: HIGH — read directly from PluginProcessor.cpp source
- Layout math: HIGH — computed from actual resized() code and window dimensions
- Rate slider sync swap pattern: HIGH — JUCE SliderParameterAttachment API is standard
- Beat detection logic: MEDIUM — design correct, exact sampleCounter_ placement needs plan decision
- drawLinearSlider constraints: HIGH — source confirmed LinearHorizontal-only support
- Subdivision string table: HIGH — verified from PluginProcessor.cpp line 278

**Research date:** 2026-02-26
**Valid until:** 2026-03-28 (30 days — stable JUCE API, no fast-moving deps)
