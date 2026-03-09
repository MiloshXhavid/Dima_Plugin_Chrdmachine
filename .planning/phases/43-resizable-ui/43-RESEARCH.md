# Phase 43: Resizable UI — Research

**Researched:** 2026-03-09
**Domain:** JUCE AudioProcessorEditor resize, ComponentBoundsConstrainer, scale-factor layout
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

1. **Resize interaction:** OS corner drag only — host exposes the resize grip. No custom resize handle drawn in the UI. No step presets or right-click menu.
2. **Resize style:** Free continuous drag — no snapping to discrete steps.
3. **Scale range and aspect ratio:**
   - Minimum 0.75x → 840×630
   - Maximum 2.0x → 2240×1680
   - Default/base 1x → 1120×840
   - Aspect ratio locked at 1120:840 (4:3) via `ComponentBoundsConstrainer`
4. **Persistence:** Scale factor saved per instance inside `getStateInformation` / `setStateInformation`. Stored as a ValueTree property on the APVTS state (not an APVTS parameter — no automation). Editor restores to saved scale before first paint.
5. **No MIDI side effects:** Resize must not trigger MIDI output or APVTS parameter changes. Scale factor is editor-only state; processor knows nothing about it.

### Claude's Discretion

None specified — all meaningful decisions are locked.

### Deferred Ideas (OUT OF SCOPE)

- **Mini mode (Phase 44):** Single-click toggle collapses UI to joystick-only widget (~280×280). Out of scope for Phase 43.
</user_constraints>

---

## Summary

Phase 43 replaces the fixed `setSize(1120, 840)` call with a proportional scale-factor system. The plugin window will resize continuously from 840×630 (0.75x) to 2240×1680 (2.0x), locking the 4:3 aspect ratio via `ComponentBoundsConstrainer::setFixedAspectRatio`.

The implementation has two distinct layers: (1) the JUCE resize infrastructure (`setResizable`, `setResizeLimits`, `getConstrainer()->setFixedAspectRatio`), which is straightforward and well-supported in JUCE 8; and (2) the layout adaptation, which is the substantive work. The current `resized()` uses all hardcoded integer pixel constants. These must all be replaced with scale-factor-multiplied values computed from `scaleFactor_ = (float)getWidth() / 1120.0f` at the top of `resized()`. The `paint()` function also uses hardcoded pixel constants for row heights, font sizes, and the header bar height that must be scaled.

Persistence is straightforward: add a `"uiScaleFactor"` double attribute to the APVTS ValueTree root element in `getStateInformation`, read it back in `setStateInformation` into a processor-side member, and have the editor constructor read it. Since the editor can be recreated without a new `setStateInformation` call, the cleanest approach is to store the scale factor on the processor (thread-safe, available at editor construction time) but keep it editor-only state (no automation, no MIDI, no processBlock involvement).

**Primary recommendation:** Derive `scaleFactor_` from `getWidth()` in `resized()` and apply it uniformly to every integer pixel constant. Store the last-known scale as a non-APVTS double on `PluginProcessor`.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `ComponentBoundsConstrainer` | JUCE 8.0.4 (project current) | Enforce aspect ratio + min/max size during host-driven resize | Built into JUCE, no alternatives needed |
| JUCE `AudioProcessorEditor::setResizable` | JUCE 8.0.4 | Enable host resize + optional in-window corner resizer | The only correct way to expose resize to the DAW |
| JUCE `ValueTree` attribute | JUCE 8.0.4 | Persist scale factor in state XML | Existing serialization mechanism in this codebase |

### No Additional Libraries Required
Everything needed is already in JUCE. No new CMake dependencies.

**No installation step required.**

---

## Architecture Patterns

### Pattern 1: JUCE Resizable Editor Setup

**What:** Call `setResizable`, `setResizeLimits`, and `getConstrainer()->setFixedAspectRatio` in the editor constructor after `setSize`.

**When to use:** All JUCE plugin editors needing locked-ratio resize.

```cpp
// Source: JUCE AudioProcessorEditor docs + forum verified pattern
// In PluginEditor constructor, AFTER setSize(1120, 840):
setResizeLimits(840, 630, 2240, 1680);   // min 0.75x, max 2.0x
getConstrainer()->setFixedAspectRatio(1120.0 / 840.0);  // 4:3
setResizable(true, false);  // host can resize; no in-window corner handle (OS provides it)
```

**Order matters:** `setResizeLimits` must be called before `setFixedAspectRatio`. `setResizable` should be called last.

### Pattern 2: Scale-Factor Derivation in resized()

**What:** Compute `scaleFactor_` as the first line of `resized()` from the live width. All integer pixel constants are multiplied by `scaleFactor_`. Helper lambda `sc(int n)` reduces verbosity.

**When to use:** Any layout that was authored at a fixed base size and needs proportional scaling.

```cpp
// In PluginEditor::resized()
// Source: derived from JUCE component layout patterns
scaleFactor_ = (float)getWidth() / 1120.0f;

auto sc = [this](int px) -> int {
    return juce::roundToInt((float)px * scaleFactor_);
};

auto area = getLocalBounds().reduced(sc(8));
area.removeFromTop(sc(28));    // header bar
area.removeFromBottom(sc(60)); // footer

constexpr int kLeftColW = 448;
auto left = area.removeFromLeft(sc(kLeftColW));
// ... all other constants likewise wrapped in sc()
```

**Font scaling:**
```cpp
// In paint()
g.setFont(pixelFont_.withHeight(sc(10)));  // or scaled directly
```

Since `sc()` is only available inside `resized()`, font sizes used in `paint()` must be derived from `scaleFactor_` stored as a member, not from `sc()`.

```cpp
// In paint() — use scaleFactor_ member directly
g.setFont(pixelFont_.withHeight(10.0f * scaleFactor_));
g.setFont(juce::Font(9.5f * scaleFactor_, juce::Font::bold));  // section titles
```

### Pattern 3: Scale Factor Persistence

**What:** Store scale as a double attribute on the APVTS ValueTree root element, not as an APVTS parameter.

**When to use:** Editor-only state that must survive save/load but must not be automatable.

```cpp
// In PluginProcessor — add member:
double savedUiScale_ { 1.0 };  // default 1x; editor reads this at construction

// In getStateInformation():
auto state = apvts.copyState();
std::unique_ptr<juce::XmlElement> xml(state.createXml());
xml->setAttribute("uiScaleFactor", savedUiScale_);   // attach to root element
copyXmlToBinary(*xml, dest);

// In setStateInformation():
// After xml validation, before apvts.replaceState():
savedUiScale_ = juce::jlimit(0.75, 2.0, xml->getDoubleAttribute("uiScaleFactor", 1.0));
apvts.replaceState(juce::ValueTree::fromXml(*xml));
// Note: attribute on root element is not loaded into the ValueTree — it's consumed here directly.

// In PluginEditor constructor:
scaleFactor_ = (float)proc_.savedUiScale_;
setSize(juce::roundToInt(1120.0f * scaleFactor_),
        juce::roundToInt(840.0f  * scaleFactor_));
```

**Thread safety note:** `savedUiScale_` is written by `setStateInformation` (audio thread) and read by the editor constructor (message thread). These two calls never overlap in practice (DAW guarantees `setStateInformation` completes before the editor is opened), but marking it `std::atomic<double>` is cleaner.

### Recommended Project Structure (changes only)

```
Source/
├── PluginEditor.h       — add: scaleFactor_ member (float, default 1.0f)
├── PluginEditor.cpp     — modify: constructor, resized(), paint(), paintOverChildren()
└── PluginProcessor.h    — add: savedUiScale_ member (std::atomic<double> or double)
    PluginProcessor.cpp  — modify: getStateInformation, setStateInformation
```

### Anti-Patterns to Avoid

- **Calling `setFixedAspectRatio` before `setResizeLimits`:** `getConstrainer()` may return a default constrainer that gets replaced by `setResizeLimits`. Call `setResizeLimits` first.
- **Using `setResizable(true, true)` with a custom constrainer:** The `true` for `useBottomRightCornerResizer` adds a JUCE-drawn corner widget that conflicts with the host's native resize chrome. Use `setResizable(true, false)` — the context says OS corner drag is the interaction model.
- **Storing scale factor as an APVTS parameter:** Parameters are automatable and appear in the DAW automation lane. Scale factor must be a raw ValueTree attribute on the root element (not a `<PARAM>` child).
- **Calling `getConstrainer()` and getting nullptr:** `AudioProcessorEditor` always has a default constrainer. `getConstrainer()` is safe to call without null-check after `setResizeLimits`.
- **Integer truncation in sc() for small values:** At 0.75x, `sc(8)` = `roundToInt(6.0)` = 6. For very small values like `sc(1)` (pixel-width lines), use `juce::jmax(1, sc(n))` to prevent zero-pixel draws.
- **Forgetting to scale `dividerX_` and `dividerX2_`:** These are stored in `resized()` and used in `paint()`. They will automatically reflect the scaled column widths because they derive from column positions computed with `sc()`.
- **Forgetting to scale `kLfoVisBufSize`-related draw math:** The oscilloscope draw in `paint()` uses the vis bounds rectangles set in `resized()` — those will scale correctly. Just ensure any per-pixel loop bounds use `rect.getWidth()` (derived, not hardcoded).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Aspect ratio enforcement | Custom resize-event handler that clamps width/height | `ComponentBoundsConstrainer::setFixedAspectRatio` | Handles all host interaction paths — resize via API, via OS drag, via host automation |
| State persistence | Custom binary format or new XML file | APVTS ValueTree root attribute via existing `getStateInformation` | Reuses existing XML round-trip; attribute on root survives `replaceState` without being loaded as APVTS state |
| Scale factor math | `AffineTransform` applied to Graphics context | `scaleFactor_` multiplied into layout constants in `resized()` | Transform approach breaks hit-testing, child component bounds, and tooltip geometry. Layout recalculation is the correct JUCE idiom |

**Key insight:** Using `AffineTransform` on the Graphics context in `paint()` would render visuals at the correct scale but leave all child component bounds (buttons, sliders, combos) at their original unscaled positions. Every child component receives its own `setBounds()` call in `resized()`. The only correct approach for interactive components is to scale the `setBounds` arguments.

---

## Common Pitfalls

### Pitfall 1: Graphics Context vs. Component Bounds Mismatch
**What goes wrong:** Developer scales only `paint()` draws but not `resized()` `setBounds` calls. Mouse events hit the wrong controls.
**Why it happens:** Confusing render-side transforms with layout-side bounds.
**How to avoid:** Scale ALL `setBounds` calls in `resized()` first. Then scale `paint()` draws that use hardcoded pixel offsets.
**Warning signs:** Clicking what appears to be a button activates a different control.

### Pitfall 2: Font Size Not Scaled
**What goes wrong:** At 2x, pixel font is 10px on a 2240-wide window — microscopic and unreadable.
**Why it happens:** Font heights are set at construction time with literal floats, not in `resized()`.
**How to avoid:** In `paint()` and `paintOverChildren()`, derive all `g.setFont(...)` sizes from `scaleFactor_`. In labels/buttons set at construction, override font in `resized()` after computing `scaleFactor_`. For `PixelLookAndFeel`, note it uses `pixelFont_.withHeight(h)` patterns — the base height is from the JUCE Font default; scale the height passed to `withHeight()`.
**Warning signs:** Text is tiny at large sizes or overflows at small sizes.

### Pitfall 3: Hardcoded Constants in paint() Bypassing Layout
**What goes wrong:** `paint()` uses its own pixel constants (e.g. `removeFromTop(28)` for header bar) independently from `resized()`. At non-1x scale, divider lines, panel borders, and section labels appear at the wrong position.
**Why it happens:** `paint()` and `resized()` both encode layout knowledge, creating duplication.
**How to avoid:** In `resized()`, store all absolute positions in member variables (like the existing `quantizerBoxBounds_`, `lfoXPanelBounds_`, etc.) and have `paint()` derive everything from those stored bounds rather than re-computing from pixel constants.
**Warning signs:** Section title labels appear above/below the boxes they label at non-1x scale.

### Pitfall 4: `scaleFactor_` Read Before `resized()` Fires
**What goes wrong:** `paint()` is called immediately after `setSize` in the constructor, before `resized()` has a chance to run. If `scaleFactor_` is initialized to 1.0f (the default), the first paint will use 1x scale regardless of the loaded scale. This is fine for paint-only elements but any stored bounds rectangles would be zero/empty.
**Why it happens:** JUCE calls `resized()` during `setSize` (synchronously), so in practice this is safe — `resized()` runs before `paint()` in normal JUCE flow.
**How to avoid:** Initialize `scaleFactor_` to `(float)proc_.savedUiScale_` BEFORE calling `setSize` in the constructor. `resized()` will then derive the same value from `getWidth()` when it fires. Consistency guaranteed.
**Warning signs:** First-frame flicker or layout jump on open.

### Pitfall 5: JoystickPad's Internal Pixel State
**What goes wrong:** `JoystickPad::resized()` already uses `getWidth()`/`getHeight()` for internal calculations (star positions, milky way cache bake, etc.) and derives from component bounds. These will naturally scale when the component gets a new `setBounds`. No explicit scaling needed inside `JoystickPad`.
**Why it happens:** Not a pitfall per se — it's correct behavior — but worth verifying that `JoystickPad::resized()` calls `spaceBgBaked_` re-bake (image cache is sized from `getWidth()`/`getHeight()`). It does.
**How to avoid:** No action needed. Verify that `starfield_` respawn uses normalized coordinates (it does — `StarDot` stores normalized positions).

### Pitfall 6: Spring-Damper Display Positions After Resize
**What goes wrong:** `displayCx_`, `displayCy_` in `JoystickPad` are in pixel coordinates relative to the pad. After a resize, the pad is a different size but the spring target is `proc_.joystickX/Y` → `getWidth()/2` + offset. If the spring velocity `springVelX_/Y_` is non-zero at the moment of resize, a brief ghost motion may occur.
**Why it happens:** Pixel-space velocity becomes incorrect at the new scale.
**How to avoid:** In `JoystickPad::resized()`, reset `springVelX_ = springVelY_ = 0.0f` and re-snap `displayCx_/Cy_` to the current joystick position in new pixel space (the existing code already snaps to `getWidth()*0.5f` — this is correct for center, but should snap to the actual current joystick position). Low priority since it resolves within one spring settle cycle (~150ms).

---

## Code Examples

### Minimal Constructor Setup
```cpp
// Source: JUCE AudioProcessorEditor docs + forum verified pattern
// PluginEditor constructor — after all component setup, before return:
scaleFactor_ = (float)proc_.savedUiScale_;
setSize(juce::roundToInt(1120.0f * scaleFactor_),
        juce::roundToInt(840.0f  * scaleFactor_));
setResizeLimits(840, 630, 2240, 1680);
getConstrainer()->setFixedAspectRatio(1120.0 / 840.0);
setResizable(true, false);
```

### Scale Helper Lambda in resized()
```cpp
// PluginEditor::resized() — first thing:
scaleFactor_ = (float)getWidth() / 1120.0f;
auto sc = [this](int px) -> int {
    return juce::jmax(1, juce::roundToInt((float)px * scaleFactor_));
};
// Usage: area.removeFromTop(sc(28))  instead of  area.removeFromTop(28)
```

### Persistence in getStateInformation
```cpp
// PluginProcessor::getStateInformation — add after existing xml creation:
auto state = apvts.copyState();
std::unique_ptr<juce::XmlElement> xml(state.createXml());
xml->setAttribute("uiScaleFactor", (double)savedUiScale_.load());
copyXmlToBinary(*xml, dest);
```

### Persistence in setStateInformation
```cpp
// PluginProcessor::setStateInformation — add inside the xml validity check:
savedUiScale_.store(juce::jlimit(0.75, 2.0,
    xml->getDoubleAttribute("uiScaleFactor", 1.0)));
// Existing apvts.replaceState(...) follows unchanged
```

### Font Scaling in paint()
```cpp
// PluginEditor::paint() — replace literal font heights:
g.setFont(pixelFont_.withHeight(11.0f * scaleFactor_));   // header title
g.setFont(juce::Font(9.5f * scaleFactor_, juce::Font::bold));  // section labels
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `setSize(1120, 840)` fixed | `setSize(scaled)` + `setResizeLimits` + `setFixedAspectRatio` | Phase 43 | Host can resize; layout adapts |
| Scale factor absent | `scaleFactor_ = getWidth() / 1120.0f` in `resized()` | Phase 43 | All layout constants are proportional |
| State without UI scale | `uiScaleFactor` attribute on APVTS XML root | Phase 43 | Scale persists across save/load |

**Not deprecated but clarified:** `setResizable(bool, bool)` second argument (`useBottomRightCornerResizer`) is `false` for this phase because the OS provides the native resize chrome. This is the correct behavior for modern DAW plugins.

---

## Open Questions

1. **`PluginLookAndFeel` font heights set at construction time**
   - What we know: Some labels/buttons have `setFont(juce::Font(10.0f))` at construction time in the PluginEditor constructor. These will not automatically scale.
   - What's unclear: Whether to re-set fonts in `resized()` for every label/button, or update `PixelLookAndFeel::getLabelFont` to return a scaled size.
   - Recommendation: The cleanest path is to update `PixelLookAndFeel::getLabelFont` and `getComboBoxFont` / `getTextButtonFont` to return `pixelFont_.withHeight(storedScaledHeight)`, where `PluginEditor` calls `pixelLaf_.setScaleFactor(scaleFactor_)` from `resized()`. This avoids touching every label's `setFont` call individually.

2. **`GamepadDisplayComponent::setBodyOffset`**
   - What we know: `bodyOffset_` is set in `resized()` as an integer. It will need to be `sc(bodyOffset)` at the new scale.
   - What's unclear: Whether `GamepadDisplayComponent::paint()` uses any hardcoded pixel values internally (it uses `getWidth()`/`getHeight()` proportion-based math, so should be self-scaling).
   - Recommendation: Verify that all `GamepadDisplayComponent::paint()` math is proportional (reads `getWidth()`/`getHeight()`). If so, only `setBodyOffset(sc(n))` needs updating in PluginEditor's `resized()`.

3. **`VelocityKnob::kBasePx` drag sensitivity at different scales**
   - What we know: `kBasePx = 300.0f` is the pixel distance for a full range sweep. At 2x scale the knob is twice as large but the same 300px drag still moves it full range — meaning drag sensitivity per-pixel-of-knob-height stays constant.
   - What's unclear: Whether this feels wrong at extreme scales (at 0.75x, 300px drag fills tiny knob; at 2x, 300px drag fills large knob — same ratio, so actually correct).
   - Recommendation: No change needed. `kBasePx` is screen-pixel distance, not knob-proportional distance. Behavior is consistent.

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + Catch2 (via FetchContent, project-standard) |
| Config file | `build/CMakeCache.txt` (BUILD_TESTS=ON required) |
| Quick run command | `cmake --build "C:/Users/Milosh Xhavid/get-shit-done/build" --config Release --target ChordJoystickTests` then `ctest --test-dir "C:/Users/Milosh Xhavid/get-shit-done/build" -C Release -R ChordJoystickTests` |
| Full suite command | same (single test binary) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| (none specified) | Scale factor clamp 0.75..2.0 | unit | Add to existing ChordEngineTests or new ScaleFactorTests.cpp | ❌ Wave 0 |
| (none specified) | XML round-trip: uiScaleFactor persists | unit | Inline XML attribute test in ChordEngineTests.cpp or new file | ❌ Wave 0 |
| (none specified) | No MIDI output on resize | manual | Open plugin in DAW, resize window, verify MIDI monitor shows nothing | manual-only |
| (none specified) | Controls clickable at 0.75x | manual | Visual + interaction test in host | manual-only |
| (none specified) | No overflow at 2.0x | manual | Visual test in host at 2240×1680 | manual-only |

### Sampling Rate
- **Per task commit:** Build VST3, install, open in DAW, resize to 0.75x and 2.0x, verify layout
- **Per wave merge:** Full build + DAW open/resize/save/load cycle
- **Phase gate:** Full suite green + DAW UAT (0.75x readable, 2.0x no overflow, save/load preserves scale)

### Wave 0 Gaps
- [ ] Optional: `Tests/ScaleFactorTests.cpp` — unit tests for XML persistence round-trip and clamp logic
- [ ] (Framework already installed via FetchContent — no new install step)

*(The core correctness criteria are visual/interactive and require manual DAW testing. Unit tests are optional additions for the persistence logic.)*

---

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.4 `ComponentBoundsConstrainer` class (project-installed) — `setFixedAspectRatio`, `setSizeLimits` API
- JUCE 8.0.4 `AudioProcessorEditor` class (project-installed) — `setResizable`, `setResizeLimits`, `getConstrainer` API
- [JUCE ComponentBoundsConstrainer docs](https://docs.juce.com/master/classComponentBoundsConstrainer.html) — method signatures verified
- [JUCE AudioProcessorEditor docs](https://docs.juce.com/master/classjuce_1_1AudioProcessorEditor.html) — resize method signatures verified
- Source code read: `PluginEditor.cpp` `resized()` at line 3953, `setSize(1120, 840)` at line 2595
- Source code read: `PluginProcessor.cpp` `getStateInformation`/`setStateInformation` at lines 2727–2750

### Secondary (MEDIUM confidence)
- [JUCE Forum: Correct Way to Resize with Constrained Aspect Ratio](https://forum.juce.com/t/correct-way-to-resize-with-constrained-aspect-ratio/24974) — `setResizeLimits` before `setFixedAspectRatio` ordering requirement
- [JUCE Forum: AudioProcessorEditor::setResizable() broken with custom constrainer](https://forum.juce.com/t/audioprocessoreditor-setresizable-broken-with-custom-constrainer/30022) — `setResizable` call order pitfall

### Tertiary (LOW confidence)
- None

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs are JUCE builtins, project already uses JUCE 8.0.4
- Architecture: HIGH — `resized()` layout recalc pattern is the JUCE-idiomatic approach, verified against existing codebase structure
- Pitfalls: HIGH — derived from direct reading of the existing `resized()` / `paint()` implementation; Graphics transform vs. bounds distinction is well-documented JUCE behavior

**Research date:** 2026-03-09
**Valid until:** 2026-09-09 (stable JUCE APIs, 6-month window)
