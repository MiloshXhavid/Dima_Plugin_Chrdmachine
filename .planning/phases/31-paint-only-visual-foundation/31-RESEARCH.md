# Phase 31: Paint-Only Visual Foundation — Research

**Researched:** 2026-03-04
**Domain:** JUCE 8 custom component painting — procedural image baking, starfield rendering, semitone grid, beat-synced animation
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**1. Background: Milky Way Band**
- Procedurally generated `juce::Image` baked once in `JoystickPad::resized()`, never rebuilt in `paint()`.
- Band angle: diagonal top-left to bottom-right (~-30° from horizontal).
- Color tone: neutral white/grey realistic — cool white core with slight blue-grey fringe. Not stylized.
- Band construction: multi-layer Gaussian-like simulation via perpendicular distance from band center line:
  - Wide soft glow (large sigma, very low alpha) as outer nebula haze
  - Medium layer for the main band body
  - Narrow bright core (high brightness, tight width)
- Band prominence: moderate — clearly visible but heatmap gradient dominates overall color.
- Band brightness at runtime scales with `randomDensity` parameter (1–8):
  - Density=1 → ~15% alpha; Density=8 → ~100% alpha
  - Implementation: `g.setOpacity(bandAlpha)` before `g.drawImage(milkyWayCache_)`

**2. Starfield**
- 300 stars pre-generated in `resized()` with deterministic fixed seed.
- Star properties baked: position (normalized 0..1), radius (0.4–2.5px), color (mostly white, ~15% blue-tinted, ~8% warm yellow), brightness.
- Visible count scales linearly with `randomPopulation` parameter (1–64):
  - Population=1 → 0 stars; Population=32 → ~150 stars; Population=64 → all 300
  - Implementation: draw only `floor(300 * (pop-1)/63.0f)` stars from pre-sorted vector
- Stars sorted by brightness descending at generate time (brightest appear first as Population increases).

**3. Heatmap Gradient**
- Keep blue→magenta radial gradient from original Phase 31, baked as `juce::Image` in `resized()`.
- Drawn inside `ScopedSaveState` with inscribed circle clip.
- Drawn AFTER milky way band (heatmap tints and partially obscures space background).
- Semi-transparent so milky way remains visible through heatmap.

**4. Semitone Grid**
- X axis (horizontal lines): `joystickYAtten` parameter (0–127) = semitone range of Y axis.
- Y axis (vertical lines): `joystickXAtten` parameter (0–127) = semitone range of X axis.
- Display cap: >48 semitones → show every 2nd; >96 → show every 4th.
- In-scale semitones: alpha 0.28f, stroke 1.2px. Out-of-scale: alpha 0.10f, stroke 0.8px.
- Scale read from APVTS `scaleNote0..11` + `scalePreset` + `globalTranspose` — visual-only.
- Horizontal lines: dashed `{3.0f, 3.0f}` via `g.drawDashedLine()`.
- Vertical lines: solid via `g.drawLine()`.
- Center-weighted brightness ramp: lines fade toward center, peak at edges.

**5. Cursor Visuals**
- Dark halo: `0xff05050f` at 0.65f alpha, radius = dotR + 8px. Unchanged.
- Glow ring: beat-synced breathing animation:
  - Alpha oscillates 0.15f–0.45f, radius dotR+4 to dotR+12
  - One full inhale/exhale per quarter note using `beatPulse_` and separate `glowPhase_` float
  - Gate-open behavior: breathing pauses, glow stays at full brightness (alpha 0.45f, outer radius)
  - `glowPhase_` (0..1) advances per timer tick, resets to 0 on beat; paused when any gate open

**6. Original Phase 31 Elements — Status**
| Element | Status |
|---------|--------|
| Near-black `0xff05050f` background | Keep |
| 70-star deterministic starfield (fixed seed) | Removed — replaced by 300-star system |
| Blue→magenta radial heatmap | Keep |
| Axis-differentiated grid (H dashed, V solid) | Keep, upgraded to semitone-driven |
| Dark halo behind cursor | Keep |
| Static cyan glow ring | Replaced by beat-synced breathing glow |
| Border alpha 0.30f | Keep |

**New Private Members Required (in JoystickPad — PluginEditor.h):**
```cpp
juce::Image milkyWayCache_;           // baked in resized()
struct StarDot { float x, y, r; juce::Colour c; };
std::vector<StarDot> starfield_;      // 300 pre-generated, brightness-sorted
float glowPhase_ = 0.0f;             // 0..1, advances with beat
```

**APVTS Parameters Read (visual-only):**
| Parameter ID | Used For | Range |
|---|---|---|
| `randomDensity` | Milky way alpha | 1–8 |
| `randomPopulation` | Star count | 1–64 |
| `joystickXAtten` | Vertical grid line count | 0–127 |
| `joystickYAtten` | Horizontal grid line count | 0–127 |
| `scaleNote0..11` | In-scale grid highlight | bool |
| `scalePreset` | In-scale grid highlight | 0–19 |
| `globalTranspose` | Grid scale offset | 0–11 |

All reads: `apvts.getRawParameterValue(...)->load()` (no listeners).

**Paint Layer Order (bottom to top):**
1. Near-black fill (`0xff05050f`)
2. Milky way band image (alpha = f(randomDensity))
3. Starfield dots (count = f(randomPopulation))
4. Heatmap radial gradient image (circle-clipped)
5. Grid lines (semitone-driven, in/out-of-scale differentiated)
6. Particle system (existing — unchanged)
7. Cursor: dark halo → glow ring (breathing/paused) → dot fill → crosshair ticks
8. Border stroke (alpha 0.30f)

### Claude's Discretion

None specified in CONTEXT.md.

### Deferred Ideas (OUT OF SCOPE)

- Original 70-star deterministic starfield with fixed seed — deferred to later phase
- Any changes to audio/MIDI behavior
- Any new APVTS parameters
- Spring-damper animation (Phase 32)
- Angle indicator (Phase 32)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| VIS-03 | Joystick pad background has radial gradient cached in resized() | heatmapCache_ bake pattern documented in Code Examples; BitmapData pixel loop pattern confirmed |
| VIS-07 | Joystick pad has space/arcade visual theme — space background, glow, visual character | Milky way + starfield + breathing glow cursor patterns all documented |
| VIS-12 | Horizontal grid lines brighter than vertical (X-axis dominant feel) | Semitone grid pattern with in/out-of-scale alpha differentiation documented |
</phase_requirements>

---

## Summary

Phase 31 re-implements the joystick pad's visual foundation from scratch (the codebase was reset to v1.6 after the original Phase 31 was executed and then rolled back with Phases 32–34). The current `JoystickPad` in the v1.6 codebase has NO `resized()` override, no `heatmapCache_`, no `starfield_`, and no `milkyWayCache_` — everything must be added fresh.

The phase is entirely paint-only: all five new visual features (milky way, starfield, heatmap, semitone grid, breathing cursor) are implemented as pure rendering operations with zero audio/MIDI changes. The `JoystickPad` class already has a 60 Hz `timerCallback()` (it manages the particle system), which also provides the heartbeat for the `glowPhase_` advancement. The `beatPulse_` float lives in `PluginEditor` (not `JoystickPad`), so the breathing cursor must use a self-contained `glowPhase_` that advances in `JoystickPad::timerCallback()` and resets to 0 when `PluginEditor::beatOccurred_` fires — but `JoystickPad` does not have access to `beatOccurred_` directly. The simplest solution is to advance `glowPhase_` freely in `JoystickPad::timerCallback()` at 1/60 per tick (60 Hz), and optionally accept a `resetGlowPhase()` call from `PluginEditor::timerCallback()` when the beat fires. Alternatively, `glowPhase_` can track `beatPulse_` as its sync reference by reading `proc_.beatOccurred_` (already a public atomic in PluginProcessor).

The critical JUCE API to understand is `juce::Image::BitmapData` for per-pixel baking in `resized()`, `g.setOpacity()` for the density-driven milky way alpha, `g.drawDashedLine()` for horizontal grid lines, and `juce::Graphics::ScopedSaveState` + `g.reduceClipRegion()` for the circle-clipped heatmap. All of these are stable JUCE 8 APIs (HIGH confidence from direct codebase inspection of existing similar code in PluginEditor.cpp).

**Primary recommendation:** Implement in two tasks — Task 01 adds `resized()` with all three baked images (milky way, starfield, heatmap) plus the upgraded semitone grid in `paint()`. Task 02 adds the breathing cursor glow using `glowPhase_` advanced in `timerCallback()`.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_gui_basics | 8.0.4 | All rendering APIs: Graphics, Image, Path, Colour | Already in use throughout PluginEditor.cpp |
| juce::Image | 8.0.4 | Off-screen pixel buffer for milky way and heatmap | BitmapData pixel access — confirmed in existing heatmap code pattern from VERIFICATION.md (line 836) |
| juce::Random | 8.0.4 | Deterministic starfield generation (fixed seed) | Already used in `spawnGoldParticles()` and `spawnBurst()` in JoystickPad |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| std::sin / std::cos / std::sqrt | C++17 stdlib | Perpendicular distance computation, Gaussian falloff | Milky way band angle and falloff calculation |
| juce::jlimit / juce::jmin / juce::jmax | 8.0.4 | Alpha clamping, count clamping | Throughout paint() to prevent out-of-range values |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| BitmapData pixel loop | juce::Image::setPixelAt | setPixelAt is slower (no batch lock); BitmapData is correct for bulk pixel writes |
| g.setOpacity() | g.setColour(c.withAlpha()) | setOpacity applies to all subsequent draws including drawImage; withAlpha only works for solid colour fills |

**Installation:** No new packages — all APIs already available via JUCE 8.0.4 in the existing CMakeLists.txt.

---

## Architecture Patterns

### Recommended Structure for JoystickPad

The class needs to grow from its current state (no `resized()`) to having both `resized()` and a larger `paint()`:

```
JoystickPad (PluginEditor.h)
├── Private members to ADD:
│   ├── juce::Image milkyWayCache_          — baked in resized()
│   ├── juce::Image heatmapCache_           — baked in resized()
│   ├── struct StarDot { float x, y, r; juce::Colour c; }
│   ├── std::vector<StarDot> starfield_     — 300 stars, brightness-sorted
│   └── float glowPhase_ = 0.0f            — 0..1, advances in timerCallback()
├── resized() override — NEW
│   ├── bake milkyWayCache_ (BitmapData pixel loop)
│   ├── generate starfield_ (juce::Random fixed seed)
│   └── bake heatmapCache_ (BitmapData pixel loop)
├── timerCallback() — EXTENDED
│   └── advance glowPhase_ by 1.0f/60.0f; pause if any gate open; reset on beat
└── paint() — EXTENDED
    ├── Layer 1: near-black fill
    ├── Layer 2: milkyWayCache_ with setOpacity(bandAlpha)
    ├── Layer 3: starfield dots (count = f(randomPopulation))
    ├── Layer 4: heatmapCache_ circle-clipped
    ├── Layer 5: semitone grid (dashed H + solid V, in/out-of-scale)
    ├── Layer 6: particles (existing, unchanged)
    ├── Layer 7: cursor (dark halo + breathing glow ring + dot + ticks)
    └── Layer 8: border stroke
```

### Pattern 1: BitmapData Pixel Loop for Image Baking

**What:** Lock a `juce::Image` for direct pixel access, write all pixels in a nested loop, unlock.
**When to use:** Any procedurally generated image that must be computed once in `resized()` and drawn many times in `paint()`.

```cpp
// Source: VERIFICATION.md line 836 + codebase pattern confirmed in JoystickPad paint() context
void JoystickPad::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    if (w < 2 || h < 2) return;

    // ── Milky way cache ──────────────────────────────────────────────────────
    milkyWayCache_ = juce::Image(juce::Image::ARGB, w, h, true);
    {
        juce::Image::BitmapData bmp(milkyWayCache_, juce::Image::BitmapData::writeOnly);

        // Band center line: from (0, h*0.35) to (w, h*0.65) gives ~-30 deg diagonal
        const float bx0 = 0.0f,    by0 = (float)h * 0.35f;
        const float bx1 = (float)w, by1 = (float)h * 0.65f;
        const float lineLen = std::sqrt((bx1-bx0)*(bx1-bx0) + (by1-by0)*(by1-by0));
        const float nx = -(by1 - by0) / lineLen;  // perpendicular normal
        const float ny =  (bx1 - bx0) / lineLen;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                // Perpendicular distance from band center line
                const float dx = (float)x - bx0;
                const float dy = (float)y - by0;
                const float dist = std::abs(dx * nx + dy * ny);

                // Three-layer Gaussian falloff
                const float outerSigma  = (float)h * 0.55f;
                const float midSigma    = (float)h * 0.18f;
                const float coreSigma   = (float)h * 0.04f;

                auto gauss = [](float d, float sigma) -> float {
                    return std::exp(-(d * d) / (2.0f * sigma * sigma));
                };

                const float outer = gauss(dist, outerSigma) * 0.12f;   // haze
                const float mid   = gauss(dist, midSigma)   * 0.55f;   // body
                const float core  = gauss(dist, coreSigma)  * 1.00f;   // bright core

                float brightness = juce::jmin(1.0f, outer + mid + core);

                // Color: cool white core fades to blue-grey fringe
                const float r = brightness * 0.88f;
                const float g = brightness * 0.90f;
                const float b = brightness;  // slight blue bias

                const uint8_t R = (uint8_t)(r * 255.0f);
                const uint8_t G = (uint8_t)(g * 255.0f);
                const uint8_t B = (uint8_t)(b * 255.0f);
                const uint8_t A = (uint8_t)(juce::jmin(1.0f, brightness * 1.6f) * 255.0f);

                bmp.setPixelColour(x, y, juce::Colour(R, G, B, A));
            }
        }
    }

    // ── Starfield ────────────────────────────────────────────────────────────
    starfield_.clear();
    starfield_.reserve(300);
    juce::Random rng(0xDEADBEEF12345678LL);  // fixed deterministic seed

    for (int i = 0; i < 300; ++i)
    {
        StarDot s;
        s.x = rng.nextFloat();  // normalized 0..1
        s.y = rng.nextFloat();
        s.r = 0.4f + rng.nextFloat() * 2.1f;   // 0.4–2.5px

        const float roll = rng.nextFloat();
        if (roll < 0.77f)
            s.c = juce::Colour(0xFFFFFFFF);                       // white
        else if (roll < 0.92f)
            s.c = juce::Colour(0xFFAABBFF);                       // blue-tinted
        else
            s.c = juce::Colour(0xFFFFEEAA);                       // warm yellow

        starfield_.push_back(s);
    }

    // Sort by radius descending (larger/brighter first → appear first as Population grows)
    std::sort(starfield_.begin(), starfield_.end(),
        [](const StarDot& a, const StarDot& b) { return a.r > b.r; });

    // ── Heatmap cache ────────────────────────────────────────────────────────
    heatmapCache_ = juce::Image(juce::Image::ARGB, w, h, true);
    {
        juce::Image::BitmapData bmp(heatmapCache_, juce::Image::BitmapData::writeOnly);
        const float cx = w * 0.5f, cy = h * 0.5f;
        const float maxR = std::sqrt(cx * cx + cy * cy);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const float dx = (float)x - cx;
                const float dy = (float)y - cy;
                const float r  = std::sqrt(dx * dx + dy * dy) / maxR;
                // angle 0=right, pi/2=up → blend blue(top) to magenta(bottom-right)
                const float angle = std::atan2(-dy, dx);  // -dy = screen Y flipped
                const float t = (angle + juce::MathConstants<float>::pi)
                              / juce::MathConstants<float>::twoPi;

                // Blue→magenta radial: center transparent, edges colored
                const uint8_t R = (uint8_t)(t * 255.0f);
                const uint8_t G = 0;
                const uint8_t B = (uint8_t)((1.0f - t) * 255.0f);
                const uint8_t A = (uint8_t)(juce::jmin(1.0f, r * 1.3f) * 180.0f);

                bmp.setPixelColour(x, y, juce::Colour(R, G, B, A));
            }
        }
    }
}
```

**VERIFICATION.md line 836 confirms:** `heatmapCache_ = juce::Image(juce::Image::ARGB, w, h, true)` was the exact pattern used in the original Phase 31.

### Pattern 2: Density/Population Driven Rendering in paint()

**What:** Read an APVTS parameter each frame and use it to gate rendering quantity.
**When to use:** Visual parameters that must reflect live APVTS values without caching.

```cpp
// Source: pattern from existing grid code (PluginEditor.cpp:1024-1046)
void JoystickPad::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const int w = getWidth(), h = getHeight();

    // Layer 1: Background
    g.setColour(juce::Colour(0xff05050f));
    g.fillRect(b);

    // Layer 2: Milky Way (alpha driven by randomDensity 1–8)
    if (milkyWayCache_.isValid())
    {
        const float density = proc_.apvts.getRawParameterValue("randomDensity")->load();
        const float bandAlpha = juce::jmap(density, 1.0f, 8.0f, 0.15f, 1.0f);
        juce::Graphics::ScopedSaveState ss(g);
        g.setOpacity(bandAlpha);
        g.drawImage(milkyWayCache_, b);
    }

    // Layer 3: Starfield (count driven by randomPopulation 1–64)
    if (!starfield_.empty())
    {
        const float pop = proc_.apvts.getRawParameterValue("randomPopulation")->load();
        const int visibleCount = (int)std::floor(300.0f * (pop - 1.0f) / 63.0f);
        const int count = juce::jmin(visibleCount, (int)starfield_.size());
        for (int i = 0; i < count; ++i)
        {
            const auto& s = starfield_[i];
            g.setColour(s.c);
            g.fillEllipse(s.x * b.getWidth()  - s.r,
                          s.y * b.getHeight() - s.r,
                          s.r * 2.0f, s.r * 2.0f);
        }
    }

    // Layer 4: Heatmap (circle-clipped)
    if (heatmapCache_.isValid())
    {
        juce::Graphics::ScopedSaveState ss(g);
        const float circleR = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
        const juce::Rectangle<float> circleRect(
            b.getCentreX() - circleR, b.getCentreY() - circleR,
            circleR * 2.0f, circleR * 2.0f);
        juce::Path circle;
        circle.addEllipse(circleRect);
        g.reduceClipRegion(circle);
        g.drawImage(heatmapCache_, b);
    }

    // ... layers 5–8 below
}
```

### Pattern 3: Semitone Grid with In/Out-Of-Scale Differentiation

**What:** Map semitone count from APVTS to grid line positions; differentiate in-scale vs out-of-scale.
**When to use:** Whenever grid density drives from a parameter and needs musical scale awareness.

```cpp
// Source: derived from existing grid pattern (PluginEditor.cpp:1023-1046)
// Layer 5: Semitone Grid
{
    const int xSemitones = (int)proc_.apvts.getRawParameterValue("joystickXAtten")->load();
    const int ySemitones = (int)proc_.apvts.getRawParameterValue("joystickYAtten")->load();

    // Build scale bitmask from APVTS (bit N = pitch class N in scale, pre-transposed)
    const int transpose = (int)proc_.apvts.getRawParameterValue("globalTranspose")->load();
    uint16_t scaleMask = 0;
    for (int n = 0; n < 12; ++n)
    {
        if (proc_.apvts.getRawParameterValue("scaleNote" + juce::String(n))->load() > 0.5f)
            scaleMask |= (1 << n);
    }

    // Compute display step (cap at 48/96 to prevent visual noise)
    auto getStep = [](int semis) -> int {
        if (semis <= 48) return 1;
        if (semis <= 96) return 2;
        return 4;
    };

    // Vertical lines (X axis semitones from joystickXAtten)
    const int xStep = getStep(xSemitones);
    for (int i = 0; i < xSemitones; i += xStep)
    {
        const float t = (float)i / (float)xSemitones;
        const float px = b.getX() + t * b.getWidth();
        // Center ramp: fade lines toward center
        const float ramp = 1.0f - (1.0f - std::abs(t * 2.0f - 1.0f)) * 0.6f;
        // In-scale check: semitone i relative to root
        const int pitchClass = ((i % 12) + transpose) % 12;
        const bool inScale = ((scaleMask >> pitchClass) & 1) != 0;
        const float alpha = inScale ? juce::jmin(0.28f, 0.28f * ramp)
                                    : juce::jmin(0.10f, 0.10f * ramp);
        const float thickness = inScale ? 1.2f : 0.8f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawLine(px, b.getY(), px, b.getBottom(), thickness);
    }

    // Horizontal lines (Y axis semitones from joystickYAtten) — dashed
    const int yStep = getStep(ySemitones);
    for (int i = 0; i < ySemitones; i += yStep)
    {
        const float t = (float)i / (float)ySemitones;
        const float py = b.getY() + t * b.getHeight();
        const float ramp = 1.0f - (1.0f - std::abs(t * 2.0f - 1.0f)) * 0.6f;
        const int pitchClass = ((i % 12) + transpose) % 12;
        const bool inScale = ((scaleMask >> pitchClass) & 1) != 0;
        const float alpha = inScale ? juce::jmin(0.28f, 0.28f * ramp)
                                    : juce::jmin(0.10f, 0.10f * ramp);
        const float thickness = inScale ? 1.2f : 0.8f;
        const float dashLengths[] = { 3.0f, 3.0f };
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawDashedLine(juce::Line<float>(b.getX(), py, b.getRight(), py),
                         dashLengths, 2, thickness);
    }
}
```

### Pattern 4: Beat-Synced Breathing Cursor Glow

**What:** Advance `glowPhase_` in `timerCallback()`, pause when any gate is open, derive alpha/radius from a smooth oscillation.
**When to use:** Any animation that must sync to beat and pause on a condition.

```cpp
// In JoystickPad::timerCallback() — add AFTER existing particle/gate code:
{
    bool anyGateOpen = false;
    for (int v = 0; v < 4; ++v)
        anyGateOpen |= proc_.isGateOpen(v);

    if (anyGateOpen)
    {
        // Gate open: hold at peak glow, no advance
        glowPhase_ = 1.0f;  // locked to full brightness
    }
    else
    {
        // Advance phase — 60 Hz timer, one full cycle = 60 frames = 1 second
        // Beat sync: reset to 0 when beat fires
        if (proc_.beatOccurred_.load(std::memory_order_relaxed))
            glowPhase_ = 0.0f;
        else
            glowPhase_ = std::fmod(glowPhase_ + 1.0f / 60.0f, 1.0f);
    }
}

// In JoystickPad::paint() — cursor section replaces static glow ring:
{
    constexpr float dotR = 7.0f;
    const float haloR = dotR + 8.0f;

    // Dark halo
    g.setColour(juce::Colour(0xff05050f).withAlpha(0.65f));
    g.fillEllipse(cx - haloR, cy - haloR, haloR * 2.0f, haloR * 2.0f);

    // Breathing glow ring: alpha 0.15..0.45, radius dotR+4..dotR+12
    const float phase = glowPhase_;
    const float breath = 0.5f + 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi);
    const float glowAlpha = 0.15f + breath * 0.30f;       // 0.15..0.45
    const float glowR     = dotR + 4.0f + breath * 8.0f;  // dotR+4..dotR+12
    g.setColour(Clr::highlight.withAlpha(glowAlpha));
    g.fillEllipse(cx - glowR, cy - glowR, glowR * 2.0f, glowR * 2.0f);

    // Dot fill + outline + ticks (existing code, unchanged)
    // ...
}
```

**Important sync note:** `proc_.beatOccurred_` is a public `std::atomic<bool>` in `PluginProcessor`. `JoystickPad::timerCallback()` already reads `proc_` for gate state and LFO data — reading `beatOccurred_` there is consistent with existing patterns. However, `PluginEditor::timerCallback()` already calls `proc_.beatOccurred_.exchange(false, ...)` to clear the flag. Since `JoystickPad::timerCallback()` runs at 60 Hz and `PluginEditor::timerCallback()` runs at 30 Hz, there is a risk of `JoystickPad` reading the beat flag before `PluginEditor` clears it (double-read), or missing it (PluginEditor clears first). **Recommended approach:** Do NOT read `beatOccurred_` from `JoystickPad`. Instead, add a `void resetGlowPhase()` method to `JoystickPad` and call it from `PluginEditor::timerCallback()` immediately after detecting the beat. This keeps the beat flag ownership in `PluginEditor` only.

### Anti-Patterns to Avoid

- **Allocating `juce::Image` in `paint()`:** paint() is called at 60 Hz. Any `juce::Image` construction in paint() causes per-frame heap allocation → jitter. All images must be created in `resized()`.
- **Using `setPixelAt()` in a nested loop:** `juce::Image::setPixelAt()` acquires a lock on each call. Use `juce::Image::BitmapData` with `writeOnly` mode for bulk pixel writes — single lock, direct pointer access.
- **Reading `scaleNote0..11` with a string loop inside paint():** Each `getRawParameterValue("scaleNote" + String(n))` call does a HashMap lookup. Either read all 12 in a batch at the top of paint() or cache the bitmask. Since the scale doesn't change every frame, the batch-read-at-top approach is acceptable.
- **Calling `resized()` logic inside `timerCallback()`:** The image bake is expensive (~few ms). `resized()` is called only on layout changes. Never trigger it from the timer.
- **Using `g.reduceClipRegion(juce::Path)` with a non-path overload:** The path overload is required for circular clip. The rectangle overload does NOT clip to a circle.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Perpendicular distance from a line | Custom line equation math | `dx*nx + dy*ny` where (nx,ny) is the normalized line normal | Already a standard formula; no library needed but the formula is 2 lines |
| Per-pixel image baking | `setPixelAt()` loop | `juce::Image::BitmapData` with `writeOnly` | BitmapData gives a raw pointer; setPixelAt has per-call overhead |
| Smooth oscillation curve | Custom lookup table | `std::sin(phase * twoPi)` | Sin is hardware-accelerated, lookup tables add complexity for no gain |
| Scale-aware grid | Custom MIDI theory class | Read `scaleNote0..11` bits from existing APVTS params | The data is already there; no new infrastructure needed |

**Key insight:** All rendering in this phase uses JUCE primitives already present in PluginEditor.cpp. There are no new library dependencies.

---

## Common Pitfalls

### Pitfall 1: `JoystickPad` Has No `resized()` in Current v1.6 Codebase
**What goes wrong:** Developer assumes `resized()` exists and tries to add code to it; gets a compile error or edits the wrong class.
**Why it happens:** The codebase was reset to v1.6 which pre-dates the original Phase 31. The VERIFICATION.md and CONTEXT.md describe the target state, not the current state.
**How to avoid:** Confirm: `JoystickPad` in the current `PluginEditor.h` has NO `resized()` declaration. The `paint()` method starts at line 1006. Task 01 must both declare `resized()` in the header and implement it in the .cpp.
**Warning signs:** Linker error "unresolved external symbol JoystickPad::resized" if declaration is added to .h without implementation.

### Pitfall 2: `beatOccurred_` Ownership Conflict
**What goes wrong:** `JoystickPad::timerCallback()` reads and clears `proc_.beatOccurred_` causing `PluginEditor::timerCallback()` to miss the beat flag (for the beat dot display), or vice versa.
**Why it happens:** `beatOccurred_` is a single atomic bool; both timers run concurrently (60 Hz vs 30 Hz). The existing code in `PluginEditor::timerCallback()` calls `.exchange(false)` to both read and clear.
**How to avoid:** `JoystickPad` must NOT call `.exchange()` on `beatOccurred_`. Use `resetGlowPhase()` pattern: `PluginEditor::timerCallback()` calls `joystickPad_.resetGlowPhase()` immediately after the beat exchange. `JoystickPad` never touches `beatOccurred_` directly.
**Warning signs:** Beat dot in PluginEditor stops pulsing when both the beat dot and breathing glow are active.

### Pitfall 3: `g.setOpacity()` Affects ALL Subsequent Draw Calls
**What goes wrong:** Setting `g.setOpacity(bandAlpha)` before `g.drawImage(milkyWayCache_)` causes all subsequent layers (starfield, heatmap) to also be drawn at reduced alpha.
**Why it happens:** `g.setOpacity()` is global state on the `juce::Graphics` object, not scoped.
**How to avoid:** Wrap the milky way draw in `juce::Graphics::ScopedSaveState ss(g);` — the scope restores opacity to 1.0f when `ss` goes out of scope. Confirmed pattern: PluginEditor.cpp already uses ScopedSaveState for the particle clip region (line 1092) and heatmap circle clip.
**Warning signs:** Starfield and heatmap appear washed out at low density values.

### Pitfall 4: `BitmapData` Out-of-Bounds Access
**What goes wrong:** Pixel loop iterates `x` from 0 to `w-1` and `y` from 0 to `h-1`, but the image was created with dimensions that changed between `resized()` calls. Accessing beyond image dimensions is undefined behavior.
**Why it happens:** Image width/height is set at construction; if `getWidth()/getHeight()` changes before the pixel loop completes, the loop uses stale bounds.
**How to avoid:** Capture `const int w = getWidth(), h = getHeight()` at the top of `resized()` and use those captured values for both image construction and the pixel loop.
**Warning signs:** Rare crash or visual corruption on window resize.

### Pitfall 5: Heatmap Circle Clip Must Use `g.reduceClipRegion(juce::Path)`
**What goes wrong:** Developer uses `g.reduceClipRegion(juce::Rectangle<int>)` which clips to a rectangle, not a circle; the heatmap appears in corners.
**Why it happens:** `reduceClipRegion` has multiple overloads. The rectangle overload is the most natural to reach for.
**How to avoid:** Use `juce::Path circle; circle.addEllipse(circleRect); g.reduceClipRegion(circle);` — confirmed pattern from VERIFICATION.md line 29 ("circle.addEllipse(circleRect)").
**Warning signs:** Heatmap gradient appears in the square corners of the pad.

### Pitfall 6: Starfield Normalized Coordinates vs Pixel Coordinates
**What goes wrong:** Stars are stored as normalized 0..1 positions in `StarDot` but drawn without scaling to pixel bounds, placing all stars in the top-left corner.
**Why it happens:** Normalized storage is correct for resize-independence, but paint() must multiply by `b.getWidth()` and `b.getHeight()`.
**How to avoid:** In paint(): `g.fillEllipse(s.x * b.getWidth() - s.r, s.y * b.getHeight() - s.r, ...)`.
**Warning signs:** All stars clustered at (0,0) or similar small region.

---

## Code Examples

### Current JoystickPad::paint() Structure (v1.6 — what we're starting from)

The current paint() (PluginEditor.cpp lines 1006–1129) does:
```cpp
// 1. Background: Clr::accent fill
// 2. Circle inscribed outline
// 3. Grid: uniform white 0.20f alpha lines driven by joystickXAtten/joystickYAtten
//    (areas mode: xN lines divide pad into xN areas)
// 4. Corner dimming when gamepad active
// 5. Circle outline
// 6. Particles (ScopedSaveState + reduceClipRegion)
// 7. Static centre reference dot
// 8. Cursor: inner glow (highlight 0.25f) → filled dot (highlight) → outline (text) → ticks
// 9. Border: highlight 0.5f alpha
```

Phase 31 replaces/upgrades every section except particles, mouse interaction, and cursor dot/ticks geometry.

### How `juce::Image::BitmapData` Works (verified from existing heatmap in VERIFICATION.md)

```cpp
// Source: VERIFICATION.md observable truth #4: "pixel loop 845-868"
juce::Image img(juce::Image::ARGB, width, height, true /* clear to zero */);
{
    juce::Image::BitmapData bmp(img, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            bmp.setPixelColour(x, y, juce::Colour(R, G, B, A));
}
// bmp destructor unlocks the image; img is now safe to draw
g.drawImage(img, destinationRect);
```

### How `g.drawDashedLine()` Works (verified from VERIFICATION.md line 31)

```cpp
// Source: VERIFICATION.md observable truth #7: dashLengths[] = {4.0f, 4.0f}
// Phase 31 CONTEXT.md specifies {3.0f, 3.0f} for the new implementation
const float dashLengths[] = { 3.0f, 3.0f };
g.drawDashedLine(juce::Line<float>(x0, y0, x1, y1),
                 dashLengths,
                 2,           // numDashLengths (array size)
                 1.2f);       // lineThickness
```

### How `beatPulse_` Is Currently Used in PluginEditor (reference for glowPhase_ design)

```cpp
// PluginEditor.cpp:4371-4374
if (proc_.beatOccurred_.exchange(false, std::memory_order_relaxed))
    beatPulse_ = 1.0f;
else if (beatPulse_ > 0.0f)
    beatPulse_ = juce::jmax(0.0f, beatPulse_ - 0.11f);

// Used at line 3757-3758:
const juce::Colour dotClr = offClr.interpolatedWith(onClr, beatPulse_)
                              .withAlpha(0.3f + 0.7f * beatPulse_);
```

`beatPulse_` is a **decay value** (1.0 → 0.0 over ~9 frames at 30 Hz), not a phase oscillator. For the breathing glow, we need a separate `glowPhase_` that oscillates continuously. These serve different purposes and must remain separate.

### `isGateOpen()` — Already Available in JoystickPad

```cpp
// PluginEditor.cpp:894 — existing use in JoystickPad::timerCallback()
if (!proc_.isGateOpen(v)) continue;
```

`proc_.isGateOpen(v)` is already called inside `JoystickPad::timerCallback()` for the ambient glow particle system. The breathing-glow gate-pause logic uses the same call — no new accessor needed.

---

## State of the Art

| Old Approach (v1.6 current) | New Approach (Phase 31) | Impact |
|------------------------------|-------------------------|--------|
| `Clr::accent` solid fill background | Near-black + milky way + heatmap layered background | Visual depth and space identity |
| 70-star fixed starfield | 300-star population-driven starfield | Live visual feedback of `randomPopulation` parameter |
| Uniform white grid lines | Semitone-driven in/out-of-scale differentiated grid | Musical context visible at a glance |
| Static cyan inner glow ring | Beat-synced breathing glow ring | Visual connection to tempo |
| Grid uses "areas" model (xN divides into xN zones) | Grid uses "semitones" model (one line per semitone) | Musically meaningful grid spacing |

**Deprecated by this phase:**
- `Clr::accent` fill in `JoystickPad::paint()` — replaced by `0xff05050f` near-black
- Static border alpha `0.5f` — replaced by `0.30f`
- Current areas-based grid loop — replaced by semitone-count loop

---

## Open Questions

1. **glowPhase_ beat sync method**
   - What we know: `beatOccurred_` is already exchanged by `PluginEditor::timerCallback()`. `JoystickPad` runs at 60 Hz; `PluginEditor` at 30 Hz.
   - What's unclear: Whether calling `proc_.beatOccurred_.load()` (read-only, no exchange) in `JoystickPad::timerCallback()` is safe alongside `PluginEditor`'s `.exchange(false)` call.
   - Recommendation: Use the `resetGlowPhase()` method pattern. `PluginEditor::timerCallback()` calls `joystickPad_.resetGlowPhase()` after its beat exchange. This is zero-risk: no atomic contention on the flag, single owner clears it.

2. **Exact heatmap color formula**
   - What we know: VERIFICATION.md confirms the original used a pixel loop from line 845-868 producing blue→magenta. CONTEXT.md says "same as original." The original loop formula is not in the current codebase (reset to v1.6).
   - What's unclear: Exact angle-to-color mapping used in the first implementation.
   - Recommendation: Use the angle-based formula in Code Examples above. It maps atan2(dy, dx) → [0,1] and linearly blends blue (0xFF0000FF) to magenta (0xFFFF00FF). CONTEXT.md only specifies "blue→magenta radial gradient" with no exact formula — the planner can use the formula above.

3. **Scale bitmask computation in paint()**
   - What we know: `ScaleKeyboard::getActiveScaleMask()` computes the pre-transposed bitmask. JoystickPad does not hold a reference to `ScaleKeyboard`.
   - What's unclear: Whether `JoystickPad` should compute the bitmask itself from APVTS or if `PluginEditor` should pass the mask to it.
   - Recommendation: Compute the bitmask inline in `JoystickPad::paint()` by reading `scaleNote0..11` from APVTS. This is 12 float reads per frame (~negligible). Avoids coupling `JoystickPad` to `ScaleKeyboard`. Pattern consistent with the existing approach of reading APVTS params directly in paint() (lines 1024-1025).

---

## Sources

### Primary (HIGH confidence)
- `Source/PluginEditor.cpp` lines 820–1129 — Full JoystickPad implementation in current v1.6 codebase; examined directly
- `Source/PluginEditor.h` lines 101–143 — Current JoystickPad class declaration; all members enumerated
- `.planning/phases/31-paint-only-visual-foundation/31-VERIFICATION.md` — Line-level evidence of the original Phase 31 implementation before the v1.6 reset; all 11 observable truths with exact line numbers
- `.planning/phases/31-paint-only-visual-foundation/31-CONTEXT.md` — All user decisions verbatim

### Secondary (MEDIUM confidence)
- JUCE 8.0.4 `juce::Image::BitmapData` — Inferred from codebase pattern (`juce::Image(juce::Image::ARGB, w, h, true)` at VERIFICATION.md line 836); writeOnly mode for bulk pixel writes is standard JUCE idiom
- `g.drawDashedLine()` with float array — Verified from VERIFICATION.md observable truth #7 (confirmed pattern in the original implementation)
- `g.setOpacity()` / `ScopedSaveState` interaction — Inferred from JUCE documentation pattern; confirmed that ScopedSaveState restores all graphics state including opacity

### Tertiary (LOW confidence)
- Gaussian falloff formula constants (outerSigma = h*0.55, midSigma = h*0.18, coreSigma = h*0.04) — These are suggested values based on visual reasoning; exact values should be tuned visually during implementation
- Star color proportions (77% white, 15% blue, 8% yellow) — Derived from CONTEXT.md text ("mostly white, ~15% blue-tinted, ~8% warm yellow"); exact thresholds are implementation detail

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs are already in use in the existing codebase; no new dependencies
- Architecture: HIGH — current state confirmed by direct code inspection; VERIFICATION.md gives line-level evidence of the target state
- Pitfalls: HIGH — most pitfalls are confirmed by reading the current code and comparing to the target described in CONTEXT.md
- Gaussian constants/color ratios: LOW — reasonable estimates, need visual validation during implementation

**Research date:** 2026-03-04
**Valid until:** 2026-04-04 (JUCE 8.0.4 APIs are stable; no external dependencies)
