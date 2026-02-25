# Phase 11: UI Polish and Installer - Research

**Researched:** 2026-02-25
**Domain:** JUCE C++ UI painting, SDL2 controller name API, Inno Setup installer
**Confidence:** HIGH — all findings sourced from direct code inspection of the actual project files

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Section grouping — rounded rect panels**
- Each section (LOOPER, FILTER MOD, GAMEPAD) gets a rounded rectangle panel
- Panel fill: `Clr::panel.brighter(0.12f)` — same approach used elsewhere (pill toggle background)
- Panel border: `Clr::accent.brighter(0.5f)` — consistent with existing outer border style
- Corner radius: 6–8px recommended to match the pill/button aesthetic
- Section label: centered, inset on top edge of panel border (floating/knockout style)
- Label sits centered over the top border line, drawn on top of the border (bg color behind text to create the knockout)
- Font: `Clr::textDim`, 9–10px bold
- Text: "LOOPER", "FILTER MOD", "GAMEPAD"
- GAMEPAD and FILTER MOD controls are currently mixed in the same row — they must be moved into separate panels with distinct visual zones
- LOOPER section: left column — wraps PLAY/REC/RST/DEL buttons, mode buttons, subdiv/length controls, and the new position bar
- FILTER MOD section: right column bottom — wraps FILTER MOD ON/OFF, REC FILTER, filterXModeBox_, filterYModeBox_, and the filter attenuator/offset knobs
- GAMEPAD section: right column bottom — wraps GAMEPAD ON/OFF button, the PANIC! button, and the gamepadStatusLabel_
- Exact pixel layout of the reorganization is Claude's discretion; honor existing control sizes and spacing conventions

**Looper position bar**
- Placement: full-width strip inside the Looper panel, between the mode buttons row and the subdiv/length controls row
- Dimensions: full width of the Looper panel, 10px tall (±2px)
- Visual style: filled sweep `Clr::gateOn` (`0xFF00E676`) from left edge to current position fraction; background track dark fill (`Clr::gateOff` or `Clr::panel`)
- No text overlay
- Idle/stopped: empty (no fill), position at 0
- Playing: fill sweeps left-to-right proportional to `LooperEngine::getPlaybackPosition()` (fraction 0.0–1.0)
- Loop wrap: position resets smoothly to 0 (no backward jump)
- Recording: show position sweep same as playback
- Drawn in `paint()` using the existing 30 Hz editor timer for updates (no new timer)
- A dedicated member `juce::Rectangle<int> looperPositionBarBounds_` stored in `resized()` and read in `paint()`

**Gamepad controller name detection**
| State | Label text | Color |
|-------|-----------|-------|
| No controller connected | "No controller" | `Clr::textDim` |
| PS4 DualShock 4 | "PS4 Connected" | `Clr::gateOn` |
| PS5 DualSense | "PS5 Connected" | `Clr::gateOn` |
| Xbox (any variant) | "Xbox Connected" | `Clr::gateOn` |
| Any other controller | "Controller Connected" | `Clr::gateOn` |
- Color behavior: permanent state change — connected = `Clr::gateOn` (green), disconnected = `Clr::textDim` (grey). No flash animation.
- Detection: `SDL_GameControllerName()` — case-insensitive substring match via `juce::String::containsIgnoreCase()`
  - Contains "DualSense" → PS5
  - Contains "DualShock 4" or "PS4" → PS4
  - Contains "Xbox" or "xinput" → Xbox
  - Anything else connected → generic fallback
- Integration point: `GamepadInput::onConnectionChangeUI` callback — pass controller name string (or empty string on disconnect)

**Installer — version 1.3**
- AppVersion string: `1.3` (not 1.1)
- Install path: no change — same VST3 destination, same product name, same GUID
- Information page feature list:
  - Section visual grouping (LOOPER / FILTER MOD / GAMEPAD panels)
  - Gamepad controller name display (PS4/PS5/Xbox detection)
  - Trigger quantization (live and post-record snap-to-grid)
  - MIDI Panic (full 16-channel sweep)
- No other installer changes — no EULA, no wizard pages, no path changes
- File to update: `installer/DimaChordJoystick-Setup.iss` — bump `AppVersion`, update feature list

### Claude's Discretion
- Exact pixel layout of the FILTER MOD / GAMEPAD panel reorganization
- Corner radius (6–8px recommended)
- Precise label font size (9–10px bold recommended)
- Looper position bar height (10px ±2px)

### Deferred Ideas (OUT OF SCOPE)
- MIDI Panic feature list entry in installer: included in v1.3 list even though it's Phase 09 (user confirmed it's a v1.1/v1.3 milestone feature)
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| UI-01 | Section visual grouping — Filter Mod section, looper section, and gamepad section each have clear visual separators or headers in the plugin UI | Rounded-rect panel pattern already exists (`drawFilterGroup`, `arpBlockBounds_`); new section panels follow exact same pattern. Looper bounds derived from existing `left` column layout; Filter Mod + Gamepad panels drawn from new stored bounds in right column. |
| UI-02 | Gamepad status shows connected controller name or type (PS4 / Xbox / Generic), not just ON/OFF | `onConnectionChangeUI` callback currently fires with `bool`. Must change signature or pass name separately. SDL2 `SDL_GameControllerName(controller_)` returns `const char*` available at connect time in `tryOpenController()`. Label text and colors fully locked. |
| UI-03 | Looper playback position bar — visual progress indicator showing current beat position within the loop length | `LooperEngine::getPlaybackBeat()` exists (returns `double`, audio-thread writes `playbackBeat_` atomic float). `getLoopLengthBeats()` exists for denominator. Bar is drawn in `paint()` from cached `looperPositionBarBounds_` member. Fraction = `getPlaybackBeat() / getLoopLengthBeats()`. |
| UI-04 | Installer rebuilt for v1.1 with updated version string and new feature list in installer description page | Actual installer file is `installer/DimaChordJoystick-Setup.iss` (not `installer.iss` as originally referenced). Current `AppVersion=1.2`. Must bump to `1.3`. No InfoBeforeFile or description section currently exists — must add one or update comment block. |
</phase_requirements>

---

## Summary

Phase 11 is a pure UI/installer polish pass on an already-working C++/JUCE plugin. All functional code is complete. The work is entirely paint(), resized(), one callback signature change, and an Inno Setup file edit.

The codebase already has the rounded-rect panel pattern (`drawFilterGroup()` for CUTOFF/RESONANCE, `arpBlockBounds_` for the ARP panel) so the new LOOPER/FILTER MOD/GAMEPAD panels follow the identical idiom. The looper position bar has all the data it needs: `getPlaybackBeat()` and `getLoopLengthBeats()` are already public on `LooperEngine` and exposed through `PluginProcessor`'s `looper_` member (or a new thin accessor). The gamepad name detection requires one targeted change to `GamepadInput`: capture the controller name string at connect time and pass it through the `onConnectionChangeUI` callback instead of a bare `bool`.

The only code change that touches existing behavior is the `onConnectionChangeUI` signature: it currently passes `bool`. Changing it to pass a `juce::String` (controller name, empty on disconnect) is a single-point change in `GamepadInput.h` and the two call sites in `GamepadInput.cpp`. The PluginEditor lambda that consumes the callback must also be updated accordingly.

**Primary recommendation:** Work top-down — 1) add `looperPositionBarBounds_` member to `PluginEditor.h`, 2) update `onConnectionChangeUI` signature in `GamepadInput.h`, 3) implement new resized() panel regions, 4) update paint() with rounded-rect panels + progress bar, 5) wire controller name in `tryOpenController()`, 6) bump installer version.

---

## Standard Stack

### Core (already in project — no new dependencies)
| Component | Version | Purpose | Role in Phase 11 |
|-----------|---------|---------|-----------------|
| JUCE juce_gui_basics | 8.0.4 | `juce::Graphics`, `juce::Rectangle`, `juce::Label` | All paint/layout work |
| SDL2 | release-2.30.9 | `SDL_GameControllerName()` | Controller name string |
| Inno Setup | 6.7.1 | `.iss` installer script | Version bump |
| C++17 | — | Language | All code |

### No new libraries required
Phase 11 does not require any new dependencies. All APIs needed exist in the project:
- `g.fillRoundedRectangle()` / `g.drawRoundedRectangle()` — already used in `drawFilterGroup()` and for ARP panel
- `LooperEngine::getPlaybackBeat()` / `getLoopLengthBeats()` — already public
- `SDL_GameControllerName()` — SDL2 already linked; call available at `tryOpenController()` time
- `juce::String::containsIgnoreCase()` — standard JUCE String method

---

## Architecture Patterns

### Pattern 1: Existing Rounded-Rect Panel (drawFilterGroup)

This is the exact pattern to replicate for LOOPER / FILTER MOD / GAMEPAD panels.

```cpp
// Source: PluginEditor.cpp lines 1705–1718
auto drawFilterGroup = [&](juce::Rectangle<int> bounds, const juce::String& title)
{
    if (bounds.isEmpty()) return;
    const auto fb = bounds.toFloat().expanded(3.0f, 4.0f);
    g.setColour(Clr::panel.brighter(0.08f));
    g.fillRoundedRectangle(fb, 5.0f);
    g.setColour(Clr::accent.brighter(0.25f));
    g.drawRoundedRectangle(fb, 5.0f, 1.0f);
    // Group header above the panel
    g.setColour(Clr::textDim.brighter(0.3f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
    g.drawText(title, (int)fb.getX(), (int)fb.getY() - 12, (int)fb.getWidth(), 12,
               juce::Justification::centred);
};
```

**For Phase 11**, the CONTEXT.md specifies a floating/knockout label ON the border (not above it). The implementation difference is:
1. Draw the rounded rect border first
2. Draw a small filled rect in `Clr::bg` (or panel fill) at the top center to mask the border line
3. Draw the label text on top

This is described in the CONTEXT.md `<specifics>` block. The planner should implement the knockout variant, not the "above" variant used for CUTOFF/RESONANCE/ARP.

### Pattern 2: Cached Bounds Member (arpBlockBounds_)

```cpp
// Source: PluginEditor.h line 254
juce::Rectangle<int> arpBlockBounds_;  // for drawing the panel in paint()

// Source: PluginEditor.cpp line 1581
arpBlockBounds_ = left.removeFromBottom(kArpH);
```

Phase 11 adds three new members following this pattern:
- `juce::Rectangle<int> looperPanelBounds_` — full looper section bounds (left column)
- `juce::Rectangle<int> filterModPanelBounds_` — FILTER MOD section bounds (right column)
- `juce::Rectangle<int> gamepadPanelBounds_` — GAMEPAD section bounds (right column)
- `juce::Rectangle<int> looperPositionBarBounds_` — the progress bar strip (inside looper panel)

### Pattern 3: Looper Progress Bar Calculation

The fraction for the progress bar:

```cpp
// Source: LooperEngine.h lines 145–146
double getPlaybackBeat() const { return static_cast<double>(playbackBeat_.load()); }
double getLoopLengthBeats() const;
```

PluginProcessor exposes `looper_` as private — needs a thin accessor or direct access via the existing `looperIsPlaying()` pattern. The planner must add a `getLoopPosition()` accessor to PluginProcessor:

```cpp
// To add in PluginProcessor.h (public section)
float getLooperPlaybackFraction() const
{
    const double len = looper_.getLoopLengthBeats();
    if (len <= 0.0) return 0.0f;
    return static_cast<float>(looper_.getPlaybackBeat() / len);
}
```

Then in `paint()`:
```cpp
// In PluginEditor::paint(), after other panel drawing
if (!looperPositionBarBounds_.isEmpty())
{
    const auto barB = looperPositionBarBounds_.toFloat();
    // Background track
    g.setColour(Clr::gateOff);
    g.fillRect(barB);
    // Filled sweep
    if (proc_.looperIsPlaying() || proc_.looperIsRecording())
    {
        const float fraction = juce::jlimit(0.0f, 1.0f, proc_.getLooperPlaybackFraction());
        g.setColour(Clr::gateOn);
        g.fillRect(barB.withWidth(barB.getWidth() * fraction));
    }
}
```

The `repaint()` is already called 30 Hz from `timerCallback()` via the pad/joystick repaints — the editor itself calls `repaint()` indirectly. However, for the progress bar, `repaint()` must be called on the editor itself (not just child components). Check whether the existing timer already calls `repaint()` on the editor. If not, add `repaint()` to `timerCallback()`.

### Pattern 4: Controller Name in onConnectionChangeUI

**Current signature (GamepadInput.h line 79):**
```cpp
std::function<void(bool)> onConnectionChangeUI;
```

**Required change:** Pass controller name string so PluginEditor can determine PS4/PS5/Xbox/generic.

**Option A — Change to string signature (cleanest):**
```cpp
// GamepadInput.h
std::function<void(juce::String)> onConnectionChangeUI;
// Empty string = disconnected, non-empty = connected with name

// GamepadInput.cpp in tryOpenController():
if (onConnectionChangeUI)
{
    const char* name = SDL_GameControllerName(controller_);
    onConnectionChangeUI(name ? juce::String(name) : juce::String("Unknown Controller"));
}

// GamepadInput.cpp in closeController():
if (onConnectionChangeUI) onConnectionChangeUI(juce::String());  // empty = disconnected
```

**Option B — Keep bool, add separate getName() method (no signature change):**
```cpp
// Add to GamepadInput.h:
juce::String getControllerName() const;

// GamepadInput.cpp:
juce::String GamepadInput::getControllerName() const
{
    if (!controller_) return {};
    const char* name = SDL_GameControllerName(controller_);
    return name ? juce::String(name) : juce::String("Unknown Controller");
}
```

**Recommendation:** Option A (string signature) is cleaner — name is only meaningful at connect/disconnect time and passing it in the callback avoids a race where the editor reads the name after the controller has already been closed. The CONTEXT.md says "pass controller name string (or empty string on disconnect)."

**PluginEditor lambda (current, PluginEditor.cpp lines 1191–1200):**
```cpp
// Current:
p.getGamepad().onConnectionChangeUI = [this](bool connected)
{
    juce::MessageManager::callAsync([this, connected]
    {
        gamepadStatusLabel_.setText(
            connected ? "GAMEPAD: connected" : "GAMEPAD: none",
            juce::dontSendNotification);
        ...
    });
};

// Updated version:
p.getGamepad().onConnectionChangeUI = [this](juce::String controllerName)
{
    juce::MessageManager::callAsync([this, controllerName]
    {
        if (controllerName.isEmpty())
        {
            gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
            gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
        }
        else
        {
            juce::String labelText;
            if (controllerName.containsIgnoreCase("DualSense"))
                labelText = "PS5 Connected";
            else if (controllerName.containsIgnoreCase("DualShock 4") ||
                     controllerName.containsIgnoreCase("PS4"))
                labelText = "PS4 Connected";
            else if (controllerName.containsIgnoreCase("Xbox") ||
                     controllerName.containsIgnoreCase("xinput"))
                labelText = "Xbox Connected";
            else
                labelText = "Controller Connected";

            gamepadStatusLabel_.setText(labelText, juce::dontSendNotification);
            gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::gateOn);
        }
        // Also update gamepadActiveBtn_ visual (existing logic)
        gamepadActiveBtn_.setToggleState(proc_.isGamepadActive() && !controllerName.isEmpty(),
                                         juce::dontSendNotification);
    });
};
```

### Pattern 5: Quantize Section Label and quantizeSubdivBox_ Width

**Current quantize row layout (resized(), lines 1626–1636):**
```cpp
auto qRow = section.removeFromTop(20);
constexpr int qBtnW = 32;
constexpr int qDropW = 48;  // <-- TOO NARROW for "1/32"
constexpr int qGap = 2;

quantizeOffBtn_ .setBounds(qRow.removeFromLeft(qBtnW)); qRow.removeFromLeft(qGap);
quantizeLiveBtn_.setBounds(qRow.removeFromLeft(qBtnW)); qRow.removeFromLeft(qGap);
quantizePostBtn_.setBounds(qRow.removeFromLeft(qBtnW)); qRow.removeFromLeft(qGap + 4);
quantizeSubdivBox_.setBounds(qRow.removeFromLeft(qDropW));  // only 48px, shows "..."
```

**Fix:** Increase `qDropW` from 48 to at least 58px. The loopSubdivBox_ uses `const int subdivW = 58` (line 1612) to show "11/8". "1/32" is similar character length — 58px is sufficient.

**Quantize Trigger section label:** Currently no label above the Off/Live/Post buttons. Per the additional context in the objective, a label "QUANTIZE TRIGGER" (or "QUANTIZE") needs to be added above that row. This can be done via `drawAbove()` or a new `drawSectionTitle()` call in `paint()`, OR by adding a row of 14px above the quantize button row in `resized()` for a `juce::Label`. Using the existing `drawAbove()` pattern on one of the quantize buttons is simplest, but `drawAbove()` is centered on a single component — for a label spanning all three buttons + dropdown, either:
- Compute a combined rect spanning `quantizeOffBtn_` left edge to `quantizeSubdivBox_` right edge and draw text above it in `paint()`
- Or add a dedicated `juce::Label` widget

**Recommendation:** Draw in `paint()` using the same approach as `drawSectionTitle` — a 12px text drawn at `quantizeOffBtn_.getY() - 14` spanning from button left to subdiv box right.

### Anti-Patterns to Avoid
- **Second timer for progress bar:** Explicitly forbidden in STATE.md. The existing 30 Hz timer in PluginEditor drives all UI updates including the progress bar.
- **juce::ProgressBar component:** Forbidden (it runs its own internal timer). Use custom `paint()` as shown in Pattern 3.
- **Reading controller name on disconnect:** Controller pointer is null after `closeController()`. Must capture name before closing or pass it in the callback.
- **Overwriting `onConnectionChange` in PluginEditor:** This slot is owned by PluginProcessor for stuck-note protection. Only touch `onConnectionChangeUI`.
- **Using `repaint()` only on child components:** The progress bar is drawn in `PluginEditor::paint()`, not a child component. The editor's `repaint()` must be called from `timerCallback()` to refresh it.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Progress bar widget | Custom Timer subclass | Existing 30 Hz `timerCallback()` → `repaint()` | STATE.md explicitly locks this; juce::ProgressBar has internal timer |
| Controller name classification | Complex regex or platform API | `SDL_GameControllerName()` + `juce::String::containsIgnoreCase()` | SDL2 already linked; simple substring match is sufficient |
| Rounded panel with knockout label | Custom Component subclass | Inline `paint()` drawing with `g.fillRect()` to knock out border | All existing panels are drawn inline in paint(); consistent with codebase |
| Installer GUI page | Wizard page or EULA | Comment block update in the `.iss` file | No InfoBeforeFile currently used; scope says no wizard pages |

---

## Common Pitfalls

### Pitfall 1: Progress bar drawn on editor but repaint() not called on editor
**What goes wrong:** Pads and joystick call `repaint()` on their own child components in `timerCallback()`, but the progress bar is drawn in `PluginEditor::paint()` itself. If `repaint()` is never called on the editor, the bar never updates.
**Why it happens:** The existing timer calls `padRoot_.repaint()` etc. — not `this->repaint()`. The editor's paint() is only invalidated when child components request it or the editor is resized.
**How to avoid:** Add `repaint()` call to `timerCallback()` for the looper bar region: `repaint(looperPositionBarBounds_)` (partial repaint is efficient) or `repaint()` (full). Partial repaint is preferred.
**Warning signs:** Progress bar stays empty even when looper is playing.

### Pitfall 2: getLoopLengthBeats() returns 0 on first call
**What goes wrong:** Division by zero in fraction calculation if `getLoopLengthBeats()` returns 0.
**Why it happens:** Loop may not be configured yet (no `setLoopLengthBars()` called), or may legitimately be 0.
**How to avoid:** Guard `if (len <= 0.0) return 0.0f` in the fraction accessor.
**Warning signs:** Crash or NaN in fill width calculation.

### Pitfall 3: onConnectionChangeUI signature change breaks PluginProcessor.cpp
**What goes wrong:** `PluginProcessor.cpp` may also reference `onConnectionChangeUI` in its constructor if the startup init code wires both slots. Changing the type from `std::function<void(bool)>` to `std::function<void(juce::String)>` breaks any existing assignment.
**Why it happens:** Code inspection shows `onConnectionChange` (bool) is owned by PluginProcessor; `onConnectionChangeUI` (bool) is assigned only in PluginEditor. But search must confirm no other assignment exists.
**How to avoid:** Grep for `onConnectionChangeUI` across all `.cpp` files before changing the signature. Only PluginEditor.cpp assigns it (confirmed by grep).
**Warning signs:** Compile error on `onConnectionChangeUI = [this](bool connected)` after type change.

### Pitfall 4: Installer file name mismatch
**What goes wrong:** The phase objective listed the installer as `installer/installer.iss` but the actual file is `installer/DimaChordJoystick-Setup.iss`.
**Why it happens:** The file was renamed from the template name when the project was set up.
**How to avoid:** Use `installer/DimaChordJoystick-Setup.iss` in all plan steps.
**Warning signs:** File not found error.

### Pitfall 5: quantizeSubdivBox_ width increased without accounting for available space
**What goes wrong:** Increasing `qDropW` from 48 to 58 works only if the qRow has enough remaining space after the three 32px buttons plus gaps.
**Why it happens:** The row is `section.removeFromTop(20)` — width equals the full left column width minus any padding. With three 32px buttons + 3 gaps of 2px + 1 gap of 6px = 102px used before the dropdown. Left column is ~colW/2 = ~240px wide. 240 - 102 = ~138px available, plenty for 58px.
**How to avoid:** Simple arithmetic confirms 58px fits. No issue.

### Pitfall 6: Looper panel bounds encompass the wrong controls
**What goes wrong:** The LOOPER panel rect in paint() may not correctly enclose all looper controls if the bounds are computed from a stale snapshot of the `left` area.
**Why it happens:** `resized()` uses progressive `.removeFromTop()` calls on a local `section` variable; the final rect must be captured before the area is consumed.
**How to avoid:** Capture `looperPanelBounds_` as the full available `left` area BEFORE any `removeFromTop()` in the looper section — i.e., at the start of the `// Looper` block — and compute the height as everything consumed up to (but not including) `arpBlockBounds_`.

### Pitfall 7: onConnectionChangeUI fires at startup (tryOpenController) before PluginEditor assigns the lambda
**What goes wrong:** `GamepadInput` is constructed inside `PluginProcessor`, which runs before `PluginEditor` is created. `tryOpenController()` fires at construction and calls `onConnectionChangeUI(name)` — but the lambda hasn't been assigned yet, so it's a no-op. This means the initial label state must still be set synchronously in the PluginEditor constructor using `isConnected()` + `getControllerName()`.
**Why it happens:** Existing code already handles this at line 1222: `if (p.getGamepad().isConnected()) gamepadStatusLabel_.setText("GAMEPAD: connected", ...)`. The Phase 11 update must replace that sync init with the same name-detection logic.
**How to avoid:** In the PluginEditor constructor, after wiring `onConnectionChangeUI`, add sync initialization:
```cpp
if (p.getGamepad().isConnected())
{
    // call the same name-detection logic as the callback
}
```

---

## Code Examples

### Example 1: Knockout-style section label on panel border

```cpp
// Knockout / floating label on a rounded-rect panel border
auto drawSectionPanel = [&](juce::Rectangle<int> bounds,
                             const juce::String& title,
                             float cornerRadius)
{
    if (bounds.isEmpty()) return;
    const auto fb = bounds.toFloat();

    // 1. Fill panel background
    g.setColour(Clr::panel.brighter(0.12f));
    g.fillRoundedRectangle(fb, cornerRadius);

    // 2. Draw border
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRoundedRectangle(fb.reduced(0.5f), cornerRadius, 1.5f);

    // 3. Measure title text
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
    const int textW = g.getCurrentFont().getStringWidth(title) + 8;
    const int textH = 12;
    const int textX = (int)(fb.getCentreX()) - textW / 2;
    const int textY = (int)fb.getY() - textH / 2;  // centered on top border

    // 4. Knockout: erase border behind label
    g.setColour(Clr::panel.brighter(0.12f));  // same as panel fill
    g.fillRect(textX, textY, textW, textH);

    // 5. Draw label text
    g.setColour(Clr::textDim);
    g.drawText(title, textX, textY, textW, textH, juce::Justification::centred);
};
```

### Example 2: Progress bar in paint()

```cpp
// In PluginEditor::paint(), after panel drawing
if (!looperPositionBarBounds_.isEmpty())
{
    const auto barB = looperPositionBarBounds_.toFloat();
    // Background track
    g.setColour(Clr::gateOff);
    g.fillRoundedRectangle(barB, 2.0f);  // slight rounding to match aesthetic

    // Filled sweep (only when playing or recording)
    if (proc_.looperIsPlaying() || proc_.looperIsRecording())
    {
        const double len = proc_.looper_.getLoopLengthBeats();  // via accessor
        const double beat = proc_.looper_.getPlaybackBeat();    // via accessor
        const float fraction = (len > 0.0)
            ? juce::jlimit(0.0f, 1.0f, (float)(beat / len))
            : 0.0f;
        if (fraction > 0.0f)
        {
            g.setColour(Clr::gateOn);
            g.fillRoundedRectangle(barB.withWidth(barB.getWidth() * fraction), 2.0f);
        }
    }
}
```

Note: `looper_` is private in PluginProcessor. Access via new thin public accessors:
```cpp
// In PluginProcessor.h (public):
double getLooperPlaybackBeat()   const { return looper_.getPlaybackBeat(); }
double getLooperLengthBeats()    const { return looper_.getLoopLengthBeats(); }
```

### Example 3: Quantize trigger section label in paint()

```cpp
// In PluginEditor::paint(), after existing drawAbove() calls
// Draw "QUANTIZE" label spanning the full quantize row
if (quantizeOffBtn_.isVisible())
{
    const int labelX = quantizeOffBtn_.getX();
    const int labelW = quantizeSubdivBox_.getRight() - labelX;
    g.setColour(Clr::textDim.brighter(0.2f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
    g.drawText("QUANTIZE TRIGGER", labelX, quantizeOffBtn_.getY() - 14, labelW, 12,
               juce::Justification::left);
}
```

### Example 4: Installer version bump

```iss
; installer/DimaChordJoystick-Setup.iss — change only AppVersion line:
AppVersion=1.3
```

No description/feature list section currently exists in the `.iss` file. The CONTEXT.md says "update the feature list in the info/description text" — this can be done as a comment block in the `.iss` header (since there is no `[Messages]` or `InfoBeforeFile` currently), or by adding an `InfoBeforeFile` entry pointing to a text file. Given the CONTEXT says "no EULA update, no 'What's New' wizard page" — a header comment is the safest interpretation.

---

## Current Layout Analysis (KEY findings for planner)

### Right Column — Current Control Positions

The right column currently has these items stacked top-to-bottom after the joystick pad:

1. **36px gap** after joystick pad (line 1385: `right.removeFromTop(36)`)
2. **90px** — Knob row: joyXAtten | joyYAtten | CUTOFF group (filterCutGroupBounds_) | RESONANCE group (filterResGroupBounds_) [lines 1390–1422]
3. **4px gap** (line 1424)
4. **70px** — Touchplates + HOLD buttons (lines 1426–1438)
5. **3px gap** (line 1440)
6. **36px** — Global ALL pad (lines 1443–1446)
7. **6px gap** (line 1448)
8. **20px** — Gamepad status row: `[GAMEPAD ON/OFF] [FILTER MOD ON/OFF] [REC FILTER] [PANIC!] [gamepadStatusLabel_]` (lines 1450–1462)
9. **12px gap** (line 1466)
10. **22px** — filterYModeBox_ with "LEFT Y" label above (lines 1467–1468)
11. **12px gap** (line 1469)
12. **22px** — filterXModeBox_ with "LEFT X" label above (line 1470)
13. **14px gap** + **18px** — gateTimeSlider_ (lines 1472–1473)
14. **10px gap** + **18px** — thresholdSlider_ (lines 1474–1475)

**For Phase 11 reorganization**, items 8–12 (the GAMEPAD + FILTER MOD controls) need to be separated into two distinct sections. The exact pixel layout is Claude's discretion.

### Left Column — Looper Section Current Bounds

The looper section occupies `left` after `arpBlockBounds_` is reserved from the bottom (line 1581):

```
left column height after sections above:
  136px Scale + 6px gap + 80px Chords + 6px gap + 80px Octaves + 6px gap
+ 36px TrigSrc + 14px SubdivHeader + 22px SubdivRow + 14px gap
+ 60px RndControls + 16px BPM + 6px gap
= consumed above looper
```

The looper `section = left` (line 1585) starts at whatever is left. Its controls:
- **36px** — PLAY/REC/RST/DEL (lines 1588–1593)
- **2px gap** (line 1595)
- **28px** — REC GATES/REC JOY/REC TOUCH/DAW SYNC (lines 1598–1605)
- **4px gap** (line 1607)
- **46px** — Subdiv/Length controls (lines 1610–1621)
- **4px gap** (line 1623)
- **20px** — Quantize row: Off/Live/Post/subdiv (lines 1625–1636)

**New looper position bar** goes between row2 (REC mode buttons) and the subdiv/length row, i.e., after the 4px gap at line 1607. The bar itself is `10px` tall with an additional gap before the existing subdiv row.

### quantizeSubdivBox_ — Current Width

```cpp
constexpr int qDropW = 48;  // line 1629 — too narrow for "1/32"
```

Available space after 3 × 32px buttons + (2+2+6)px gaps = 102px consumed. Left column is approximately (window_width / 2 - 12px edges) ≈ 228px. 228 - 102 = 126px remaining. Changing `qDropW` from 48 to 58 uses 10 more pixels; still leaves 116px unused remainder. Safe change, no ripple effects.

### gamepadStatusLabel_ — Current Position and Text

- **Position:** Remainder of the 20px row after `[GAMEPAD ON/OFF 90px] [4px] [FILTER MOD ON/OFF 90px] [4px] [REC FILTER 58px] [4px] [PANIC! 60px right-aligned] [4px right gap]`
- **Current text:** "GAMEPAD: none" (init) or "GAMEPAD: connected" (on connect)
- **Current color:** `Clr::textDim` always — no color change on connect
- **Phase 11 change:** Text becomes "No controller" / "PS4 Connected" / "PS5 Connected" / "Xbox Connected" / "Controller Connected"; color changes to `Clr::gateOn` on connect

### LooperEngine — getPlaybackPosition() API

The method name in `LooperEngine.h` is `getPlaybackBeat()` (not `getPlaybackPosition()`). It returns `double`:

```cpp
// LooperEngine.h line 145
double getPlaybackBeat() const { return static_cast<double>(playbackBeat_.load()); }
double getLoopLengthBeats() const;  // returns beats per bar * loop bars
```

`playbackBeat_` is an `std::atomic<float>` written by the audio thread in `process()`. It is safe to read from the message thread (lock-free float atomic on x86/ARM).

### onConnectionChangeUI — Current Signature

```cpp
// GamepadInput.h line 79
std::function<void(bool)> onConnectionChangeUI;
```

Currently passes `bool connected`. Must change to `std::function<void(juce::String)>` (empty string = disconnected, non-empty = controller name).

### Installer File

- **Actual file:** `installer/DimaChordJoystick-Setup.iss` (not `installer.iss`)
- **Current AppVersion:** `1.2`
- **Target AppVersion:** `1.3`
- **No existing** `InfoBeforeFile`, `[Messages]`, or description section
- The file only has `[Setup]`, `[Files]`, `[UninstallDelete]`, and a commented `[Code]` section

---

## State of the Art

| Old Approach | Current Approach | Phase 11 Change |
|--------------|------------------|-----------------|
| "GAMEPAD: connected" / "GAMEPAD: none" | Same (bool callback) | "PS4 Connected" / "Xbox Connected" / etc. via name string callback |
| No looper progress indicator | None | 10px bar: gateOn fill sweeping left-to-right |
| Mixed GAMEPAD + FILTER MOD in same row | Same row | Separate rounded-rect panels |
| AppVersion=1.2 | AppVersion=1.2 | AppVersion=1.3 |
| quantizeSubdivBox_ 48px wide | 48px | 58px (shows "1/32" without truncation) |
| No "QUANTIZE TRIGGER" label | No label | Label drawn in paint() above quantize buttons |

---

## Open Questions

1. **Does timerCallback() currently call repaint() on the editor itself?**
   - What we know: It calls `padRoot_.repaint()`, `joystickPad_.repaint()`, etc. (child components)
   - What's unclear: Whether the editor's own `paint()` is refreshed at 30 Hz without an explicit `repaint()` call
   - Recommendation: Add `repaint(looperPositionBarBounds_)` to `timerCallback()` unconditionally. This is safe and cheap (partial repaint).

2. **Should the FILTER MOD and GAMEPAD panels stack vertically or side-by-side in the right column?**
   - What we know: They are currently in the same 20px row. CONTEXT.md says "separate panels with distinct visual zones." Exact pixel layout is Claude's discretion.
   - What's unclear: Whether vertical stacking (GAMEPAD above FILTER MOD or vice versa) or horizontal side-by-side better fits the available right column space below the threshold slider.
   - Recommendation: Vertical stacking is safer — the right column has finite width (~228px) and each panel needs enough room for its controls. GAMEPAD panel needs: `[GAMEPAD ON/OFF]` + `[PANIC!]` + `[status label]`. FILTER MOD panel needs: `[FILTER MOD ON/OFF]` + `[REC FILTER]` + mode combos + knobs. The knobs + combos take 90+4+22 = multiple rows.

3. **What happens to the existing init sync of gamepadStatusLabel_ text?**
   - What we know: Line 1222-1224: `if (p.getGamepad().isConnected()) gamepadStatusLabel_.setText("GAMEPAD: connected", ...)` runs on PluginEditor construction.
   - Recommendation: Replace with name-detection logic using `p.getGamepad().getControllerName()` (new method to add). If no method added, inline `SDL_GameControllerName` call is not available without SDL headers in PluginEditor. Cleanest: add `getControllerName()` to GamepadInput.

---

## Sources

### Primary (HIGH confidence)
- `Source/PluginEditor.cpp` — resized() lines 1358–1658, paint() lines 1662–1825, timerCallback() lines 1829–2048, constructor wiring of onConnectionChangeUI lines 1179–1224
- `Source/PluginEditor.h` — all member declarations
- `Source/GamepadInput.h` — onConnectionChangeUI signature (line 79), class structure
- `Source/GamepadInput.cpp` — tryOpenController() (lines 36–53), closeController() (lines 55–65)
- `Source/LooperEngine.h` — getPlaybackBeat() (line 145), getLoopLengthBeats() (line 146)
- `Source/PluginProcessor.h` — looper_ private member, existing accessor patterns
- `installer/DimaChordJoystick-Setup.iss` — actual installer file, AppVersion=1.2

### Secondary (MEDIUM confidence)
- `.planning/STATE.md` — confirmed: "Phase 11 progress bar must NOT use juce::ProgressBar — runs its own internal timer; use custom paint() from editor timerCallback"; "Single 30 Hz timer only"
- `.planning/phases/11-ui-polish-and-installer/11-CONTEXT.md` — all locked decisions, label text strings, color values

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new dependencies; all APIs verified in source files
- Architecture patterns: HIGH — derived directly from existing codebase patterns (drawFilterGroup, arpBlockBounds_, timerCallback structure)
- API surface (getPlaybackBeat, onConnectionChangeUI): HIGH — inspected directly from header files
- Installer file location and content: HIGH — actual file read and verified
- Pitfalls: HIGH — derived from direct code analysis and STATE.md locked decisions

**Research date:** 2026-02-25
**Valid until:** Stable — no external dependencies changing; valid until codebase changes
