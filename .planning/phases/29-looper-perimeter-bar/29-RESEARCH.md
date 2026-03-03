# Phase 29: Looper Perimeter Bar - Research

**Researched:** 2026-03-03
**Domain:** JUCE C++ UI rendering — custom progress visualization in paint()
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Bar appearance**
- 2px thick
- Color: `Clr::gateOn` (same green used for active gates / existing progress fill)

**Label protection**
- The "LOOPER" knockout label sits at the top-left corner of the panel border (the start/end point of the circuit)
- The bar skips over the label area — jumps past the text so it never paints over the characters at any point in the circuit

**Idle / stopped state**
- When the looper is stopped or no loop is loaded: render a dim ghost ring — a faint full-perimeter outline that hints the animation path
- When playing or recording: the traveling bar replaces the ghost ring

**Animation feel**
- Leading edge with a trailing tail/fade behind it
- Tail fades out (opacity gradient or shortened fill) — exact tail length/falloff is Claude's Discretion

### Claude's Discretion

- Exact tail length and fade curve
- Ghost ring color (likely `Clr::gateOff` or a dimmed `Clr::gateOn`)
- Whether to keep or remove the `looperPositionBarBounds_` strip (remove it — it is being replaced)
- How to expand the 30Hz repaint region from `looperPositionBarBounds_` to `looperPanelBounds_`

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| LOOP-01 | Remove existing linear looper progress bar (strip rendered below Rec gates) | Identified exact code block at PluginEditor.cpp line 3550-3574; `looperPositionBarBounds_` layout assignment at resized() line 2931 |
| LOOP-02 | Rectangular perimeter progress bar travels clockwise around Looper section box, one circuit per loop cycle, 30 Hz via existing timerCallback | Architecture Pattern 1 (perimeter math), Pattern 2 (tail fade); timerCallback repaint target update |
| LOOP-03 | Bar origin at top-left (LOOPER label corner), travels right → down → left → up | Perimeter coordinate math section; starting offset = label skip distance |
| LOOP-04 | "LOOPER" section label remains fully visible at all times | Pattern 3 (label exclusion clip using ScopedSaveState + excludeClipRegion); label bounds computed from drawLfoPanel constants |
</phase_requirements>

---

## Summary

Phase 29 is a pure UI rendering change inside `PluginEditor::paint()` and `PluginEditor::timerCallback()`. No audio-thread, APVTS, or processor changes are needed. The existing horizontal progress strip (`looperPositionBarBounds_`) is replaced by a clockwise-traveling bar that circuits the perimeter of `looperPanelBounds_`.

The core algorithm converts a fractional loop position [0, 1) into a distance along the four sides of the bounding rectangle, then draws a stroked path segment of fixed pixel length for the "head" and a fading tail behind it. The label protection is achieved by treating the label rectangle as a dead zone: the bar's logical distance calculation skips the label span, and a JUCE clip region excludes the label area so no pixels paint over it.

The repaint target in `timerCallback()` changes from `looperPositionBarBounds_` (4px strip) to `looperPanelBounds_` (the full LOOPER section rectangle). Because `looperPanelBounds_` is already valid after `resized()`, the isEmpty() guard pattern already in use works without modification.

**Primary recommendation:** Implement perimeter bar as a `juce::Path` with clip exclusion for the label zone; draw the full ghost ring first, then overdraw the animated segment for playing state.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE Graphics | 8.0.4 (already linked) | `g.strokePath()`, `juce::Path`, `juce::Graphics::ScopedSaveState`, `g.excludeClipRegion()` | Already used throughout PluginEditor.cpp for all rendering |
| JUCE Rectangle | 8.0.4 | `juce::Rectangle<float>` perimeter math, `.toFloat()`, `.reduced()` | Already used for all bounds calculations |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce::PathStrokeType` | 8.0.4 | Control stroke cap style (rounded ends look clean) | Already used for knob arc rendering — same pattern applies to bar segment |
| `juce::ColourGradient` | 8.0.4 | Tail fade from `Clr::gateOn` → transparent | Already used elsewhere in the editor; simpler than multi-segment alpha math |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `juce::Path` + `strokePath` | `fillRect` segments on each side | Path approach handles corners and clipping cleanly; fillRect requires 4 separate clipped draw calls |
| `excludeClipRegion` for label | Offset bar start past label | Clip approach works regardless of label width; offset math couples tightly to font metrics |

**Installation:** No new packages — uses JUCE APIs already linked.

---

## Architecture Patterns

### Recommended Project Structure

All changes are in two methods of `PluginEditor.cpp`:

```
PluginEditor.cpp
├── paint()                        # Replace looper bar block (~line 3550)
│   ├── ghost ring draw (stopped/no loop)
│   └── animated perimeter bar (playing/recording)
├── resized()                      # Remove looperPositionBarBounds_ layout
│   └── looperPositionBarBounds_ = {}  // or remove its use
└── timerCallback()                # Change repaint target
    └── repaint(looperPanelBounds_) instead of repaint(looperPositionBarBounds_)

PluginEditor.h
└── looperPositionBarBounds_       # Keep declaration; set to {} in resized
```

No new member variables are required. No new files are needed.

### Pattern 1: Perimeter Distance Math

**What:** Convert rectangle perimeter into a 1D [0, 1) coordinate system, then map a fraction to (x, y) on that perimeter.
**When to use:** Whenever animating along a rectangular path.

The perimeter is divided into four segments: top, right, bottom, left (clockwise from top-left). A `totalPerim` = 2*(W+H). A fractional position `f` maps to a pixel distance `d = f * totalPerim`, then into which segment it falls.

```cpp
// Source: Project-specific — JUCE Rectangle math, no external source needed
// looperPanelBounds_ is juce::Rectangle<int>; convert to float
const auto b = looperPanelBounds_.toFloat().reduced(1.0f); // inset so 2px stroke stays inside border
const float W = b.getWidth();
const float H = b.getHeight();
const float totalPerim = 2.0f * (W + H);

// fraction in [0,1) from getLooperPlaybackBeat() / getLooperLengthBeats()
// distance along perimeter (starts at top-left, goes right)
float d = fraction * totalPerim;

// Map d to (x, y) on the perimeter
auto perimPoint = [&](float dist) -> juce::Point<float>
{
    dist = std::fmod(dist, totalPerim);
    if (dist < 0) dist += totalPerim;
    if (dist < W)                          // top edge: left → right
        return { b.getX() + dist,           b.getY() };
    dist -= W;
    if (dist < H)                          // right edge: top → bottom
        return { b.getRight(),              b.getY() + dist };
    dist -= H;
    if (dist < W)                          // bottom edge: right → left
        return { b.getRight() - dist,       b.getBottom() };
    dist -= W;
    return { b.getX(),                     b.getBottom() - dist }; // left edge: bottom → top
};
```

### Pattern 2: Traveling Bar Segment with Tail

**What:** Build a `juce::Path` from `tailStart` to `leadEnd`, where `leadEnd` is the current perimeter position and `tailStart` is `kTailLength` pixels behind it.
**When to use:** The animated playing/recording state.

Recommended tail length: 40px (approximately 1/8th of a typical perimeter, visually similar to a clock sweep hand). Claude's discretion per CONTEXT.md.

```cpp
// Source: Project-specific; uses juce::Path + juce::PathStrokeType already in codebase
constexpr float kTailPx = 40.0f;

// Build path from tail to head along perimeter
juce::Path barPath;
const int kSegments = 20; // subdivide for smooth corners
barPath.startNewSubPath(perimPoint(d - kTailPx));
for (int i = 1; i <= kSegments; ++i)
{
    const float t = (d - kTailPx) + (kTailPx * i / kSegments);
    barPath.lineTo(perimPoint(t));
}

// Draw tail → head with uniform color (alpha gradient via gradient fill is optional enhancement)
const juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved,
                                   juce::PathStrokeType::rounded);
g.setColour(Clr::gateOn);
g.strokePath(barPath, stroke);
```

For the tail fade, use a `juce::ColourGradient` along the path direction. Because JUCE's ColourGradient is axis-aligned, the simplest fade is a fixed-alpha approach: draw the tail at `Clr::gateOn.withAlpha(0.25f)` and the last few px at full alpha. Alternatively, skip the gradient and use a shortened tail (20px) at full alpha — simpler, still effective.

### Pattern 3: Label Exclusion Clip

**What:** Exclude the "LOOPER" label rectangle from the clip region before stroking the bar, so the bar cannot paint over the label characters.
**When to use:** Anywhere the bar must avoid a fixed text region.

The label bounds are computable from the `drawLfoPanel` constants already in `paint()`:
- `textX` = `(int)fb.getX() + 6`
- `textY` = `(int)fb.getY() - textH / 2` (where `textH = 14`)
- `textW` = `g.getCurrentFont().getStringWidth("LOOPER") + 10`
- `textH` = 14

```cpp
// Source: Project-specific — uses juce::Graphics::ScopedSaveState already in PluginEditor.cpp
{
    juce::Graphics::ScopedSaveState ss(g);

    // Compute label bounds (mirrors drawLfoPanel logic)
    const auto fb = looperPanelBounds_.toFloat();
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
    const int labelW = g.getCurrentFont().getStringWidth("LOOPER") + 10;
    const int labelH = 14;
    const int labelX = (int)fb.getX() + 6;
    const int labelY = (int)fb.getY() - labelH / 2;

    // Exclude label area — bar will never render over it
    g.excludeClipRegion(juce::Rectangle<int>(labelX, labelY, labelW, labelH));

    // Now stroke the bar path — pixels in the excluded region are untouched
    g.setColour(Clr::gateOn);
    g.strokePath(barPath, stroke);
}
```

### Pattern 4: Ghost Ring (Stopped State)

**What:** Full-perimeter stroked path at low alpha when looper is stopped or no loop is loaded.
**When to use:** `!proc_.looperIsPlaying() && !proc_.looperIsRecording()` OR `getLooperLengthBeats() <= 0.0`.

```cpp
// Source: Project-specific
// Ghost ring = same perimeter path, full circuit, faint color
juce::Path ghostPath;
const int kGhostSegs = 40;
ghostPath.startNewSubPath(perimPoint(0.0f));
for (int i = 1; i <= kGhostSegs; ++i)
    ghostPath.lineTo(perimPoint(totalPerim * i / kGhostSegs));
ghostPath.closeSubPath();

{
    juce::Graphics::ScopedSaveState ss(g);
    // Exclude label
    g.excludeClipRegion(juce::Rectangle<int>(labelX, labelY, labelW, labelH));
    g.setColour(Clr::gateOff.brighter(0.3f));  // dim ring — Claude's discretion color
    const juce::PathStrokeType ghostStroke(1.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded);
    g.strokePath(ghostPath, ghostStroke);
}
```

### Anti-Patterns to Avoid

- **Don't use `fillRect` per side for the animated bar:** Each fill call requires a separate clip setup; juce::Path handles the corners and single-call rendering cleanly.
- **Don't compute label width during paint() without setting font first:** `g.getCurrentFont()` returns whatever was last set. Always call `g.setFont(...)` before `getStringWidth()`.
- **Don't call `repaint(looperPanelBounds_)` inside paint():** Only call repaint from timerCallback or event handlers — never from within paint().
- **Don't derive label exclusion bounds from pixels empirically:** Compute them from the same constants as `drawLfoPanel` (textX = fb.getX()+6, textY = fb.getY()-7, textH = 14) to stay in sync if panel bounds ever change.
- **Don't forget to ScopedSaveState before excludeClipRegion:** Without the save/restore, the exclusion persists for the rest of paint() and clips other elements.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Perimeter point mapping | Custom sin/cos arc approximation | Direct linear segment math on Rectangle | Rectangle sides are straight; trig is unnecessary complexity and harder to debug |
| Stroke caps at endpoints | Manual circle caps | `juce::PathStrokeType::rounded` | Built into JUCE stroke type — one line |
| Clip exclusion for label | Manual bounds intersection test before drawing | `g.excludeClipRegion()` + `ScopedSaveState` | JUCE clips at pixel level — no math required, handles sub-pixel correctly |
| Tail color fade | Per-pixel alpha math | Single lower-alpha color for full tail OR ColourGradient | Sufficient visual result with far less code |

**Key insight:** JUCE's `Path` + `strokePath` + `excludeClipRegion` is purpose-built for this scenario. The perimeter math is ~20 lines and the entire paint block should be under 60 lines.

---

## Common Pitfalls

### Pitfall 1: looperPositionBarBounds_ still referenced in timerCallback

**What goes wrong:** After removing the paint block for the old strip, if timerCallback still checks `looperPositionBarBounds_.isEmpty()` and calls `repaint(looperPositionBarBounds_)`, the new perimeter bar never animates because `looperPanelBounds_` is never scheduled for repaint.
**Why it happens:** Two separate places reference the old bounds: `paint()` and `timerCallback()`. Easy to update one and forget the other.
**How to avoid:** Update timerCallback in the same commit as the paint block. Change the guard to `looperPanelBounds_` and repaint that.
**Warning signs:** Plugin builds but bar does not animate at all.

### Pitfall 2: perimPoint fmod wrapping at boundary

**What goes wrong:** When `d - kTailPx` goes negative (bar is near the start of the circuit), `fmod` of a negative float gives a negative result in C++. The tail point maps to an incorrect location.
**Why it happens:** C++ `std::fmod` follows the sign of the dividend — `-5 mod 100 = -5`, not `95`.
**How to avoid:** After `fmod`, add `totalPerim` and fmod again, or use the guard `if (dist < 0) dist += totalPerim` as shown in Pattern 1.
**Warning signs:** Tail visually "jumps" to wrong corner when bar crosses the start/top-left origin.

### Pitfall 3: Label exclusion bounds mismatch with drawLfoPanel

**What goes wrong:** The label exclusion rectangle doesn't match what `drawLfoPanel` actually painted, so either a small gap in the bar is visible when no label is there, or the bar still bleeds onto the label.
**Why it happens:** `drawLfoPanel` uses `fb.getY() - textH/2` for `textY`, which means the label is centered on the panel's top border. The bar exclusion must use identical math.
**How to avoid:** Extract label bound computation into a small helper lambda shared between `drawLfoPanel` and the bar painter, OR compute it inline using the same constants (textH=14, textX = fb.getX()+6, textY = fb.getY()-7).
**Warning signs:** Visible gap or bleed at label position. Test by pausing the animation (comment out the fraction multiply) at fraction=0.0 and checking that the exclusion zone aligns with the white text.

### Pitfall 4: Painting the perimeter bar BEFORE drawLfoPanel

**What goes wrong:** If the perimeter bar path is stroked before `drawLfoPanel` runs, the panel fill (`fillRoundedRectangle`) will overdraw the bar entirely.
**Why it happens:** paint() is sequential — later calls overdraw earlier calls.
**How to avoid:** Draw the perimeter bar AFTER `drawLfoPanel(looperPanelBounds_, "LOOPER")` in paint(). The existing looper bar block at line ~3550 is already after the drawLfoPanel calls at line 3397 — maintaining this order is correct.
**Warning signs:** Bar not visible at all; no error, just invisible.

### Pitfall 5: Ghost ring using getLooperLengthBeats() == 0 check only

**What goes wrong:** If the looper has a loop loaded but is stopped (paused mid-loop), `getLooperLengthBeats()` is non-zero but `looperIsPlaying()` is false. Showing the animated bar when stopped is wrong per spec.
**Why it happens:** Conflating "has a loop" with "is playing."
**How to avoid:** Condition on `proc_.looperIsPlaying() || proc_.looperIsRecording()` for the animated bar; show ghost ring otherwise (regardless of whether a loop is loaded).
**Warning signs:** Bar shows at a frozen position when looper is paused.

---

## Code Examples

Verified patterns from the existing codebase:

### Existing ScopedSaveState + excludeClipRegion usage

```cpp
// Source: PluginEditor.cpp line 197 — existing pattern in codebase
juce::Graphics::ScopedSaveState ss(g);
g.reduceClipRegion(activeHalf.toNearestInt());
g.setColour(Clr::highlight.withAlpha(0.70f));
g.fillRoundedRectangle(bounds, pr);
// ss destructor restores clip automatically
```

`excludeClipRegion` works identically to `reduceClipRegion` but subtracts instead of intersects — both respect `ScopedSaveState`.

### Existing strokePath usage (knob arc)

```cpp
// Source: PluginEditor.cpp line 64-71 — existing pattern
const juce::PathStrokeType thinStroke(1.5f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded);
juce::Path trackArc;
trackArc.addArc(cx - trackR, cy - trackR, trackR * 2.0f, trackR * 2.0f,
                rotaryStartAngle, rotaryEndAngle, true);
g.setColour(juce::Colour(0xFF2A3050));
g.strokePath(trackArc, thinStroke);
```

For the perimeter bar: same `PathStrokeType::rounded` cap, stroke width = 2.0f (per locked decision).

### Existing looper state query pattern

```cpp
// Source: PluginEditor.cpp line 3560-3573 — existing pattern
if (proc_.looperIsPlaying() || proc_.looperIsRecording())
{
    const double len  = proc_.getLooperLengthBeats();
    const double beat = proc_.getLooperPlaybackBeat();
    if (len > 0.0)
    {
        const float fraction = juce::jlimit(0.0f, 1.0f,
                                static_cast<float>(beat / len));
        // use fraction here
    }
}
```

Exactly the same guard logic applies to the new perimeter bar.

### Existing timer repaint pattern

```cpp
// Source: PluginEditor.cpp line 3714-3717 — existing pattern
// Change from:
if (!looperPositionBarBounds_.isEmpty())
    repaint(looperPositionBarBounds_);
// Change to:
if (!looperPanelBounds_.isEmpty())
    repaint(looperPanelBounds_);
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Linear 4px horizontal strip (`looperPositionBarBounds_`) | Clockwise rectangular perimeter bar around `looperPanelBounds_` | Phase 29 | More immediate "clock-face" readability; label always visible |

**What is removed:**
- `looperPositionBarBounds_` layout in `resized()` (line 2931 — the `removeFromTop(4)` call and the assignment)
- The paint block at line 3550-3574 (fill+background for the strip)
- The `repaint(looperPositionBarBounds_)` call in timerCallback

**What is added:**
- Ghost ring render (stopped state, ~15 lines)
- Animated perimeter bar render with tail (playing/recording state, ~35 lines)
- Label exclusion clip block

**`looperPositionBarBounds_` member variable:** Keep the declaration in PluginEditor.h but zero it out in resized() with `looperPositionBarBounds_ = {};`. This avoids changing the header declaration which is low value — or remove the declaration entirely. Either approach works.

---

## Open Questions

1. **Tail fade implementation detail**
   - What we know: CONTEXT.md specifies "tail fades out (opacity gradient or shortened fill)" and marks exact tail length/falloff as Claude's Discretion
   - What's unclear: Whether a single lower-alpha flat color for the tail or a proper ColourGradient is preferred visually
   - Recommendation: Implement as a two-segment path: first draw the tail half at `Clr::gateOn.withAlpha(0.3f)`, then overdraw the leading quarter at `Clr::gateOn` (full alpha). This is 6 extra lines and avoids ColourGradient complexity. Review visually and adjust alpha.

2. **looperPositionBarBounds_ member variable lifecycle**
   - What we know: It is declared in PluginEditor.h and assigned in resized()
   - What's unclear: Whether to remove the declaration from the .h file entirely or just zero it in resized()
   - Recommendation: Zero it in resized() (`looperPositionBarBounds_ = {};`). Removing from .h is a minor cleanup that risks compiler errors if any other site still references it — do a search first.

---

## Sources

### Primary (HIGH confidence)

- PluginEditor.cpp — direct code read: existing looper bar block (line 3550-3574), drawLfoPanel (line 3366-3390), timerCallback looper repaint (line 3714-3717), resized() looper section (line 2895-2955), existing ScopedSaveState pattern (line 197), existing strokePath pattern (line 64-71)
- PluginEditor.h — direct code read: looperPanelBounds_ and looperPositionBarBounds_ declarations (line 451-456)
- PluginProcessor.h — direct code read: looperIsPlaying(), looperIsRecording(), getLooperPlaybackBeat(), getLooperLengthBeats() signatures (line 60-73)
- JUCE 8.0.4 (linked project dependency) — Graphics::excludeClipRegion, Path, PathStrokeType::rounded verified by reading existing usage in PluginEditor.cpp

### Secondary (MEDIUM confidence)

- JUCE official documentation (https://docs.juce.com/master/classGraphics.html) — `excludeClipRegion` and `ScopedSaveState` documented; behavior confirmed by existing usage in codebase

### Tertiary (LOW confidence)

- None

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — uses only JUCE APIs already used in the file; no new dependencies
- Architecture: HIGH — complete code examples derived from existing patterns in the codebase
- Pitfalls: HIGH — derived from direct reading of the existing code and known JUCE behaviors

**Research date:** 2026-03-03
**Valid until:** 2026-04-03 (JUCE APIs stable; no time-sensitive ecosystem)
