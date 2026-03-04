---
phase: 31-paint-only-visual-foundation
plan: 01
subsystem: ui
tags: [juce, cpp, opengl, paint, juce-image, bitmap, starfield, milkyway, heatmap, semitone-grid]

# Dependency graph
requires:
  - phase: none
    provides: "v1.6 codebase with JoystickPad component"
provides:
  - "JoystickPad::resized() baking milkyWayCache_, heatmapCache_, starfield_ (300 stars)"
  - "JoystickPad::paint() with 8-layer space visual stack"
  - "resetGlowPhase() public method ready for Plan 02 beat wiring"
  - "glowPhase_ member initialized to 0.0f, glow ring breathing math in paint()"
affects:
  - "31-02 — cursor breathing (wires glowPhase_ in timerCallback)"
  - "32 — spring-damper and angle indicator composited into this layer stack"

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "BitmapData pixel loop for image baking in resized() — zero per-frame allocation"
    - "ScopedSaveState + setOpacity() for per-layer opacity without global state bleed"
    - "Path circle clip via g.reduceClipRegion(juce::Path) — not the Rectangle overload"
    - "12-bit bitmask (uint16_t scaleMask) for O(1) per-line scale membership test"
    - "Fixed-seed juce::Random(0xDEADBEEF12345678LL) for deterministic starfield"

key-files:
  created: []
  modified:
    - "Source/PluginEditor.h"
    - "Source/PluginEditor.cpp"

key-decisions:
  - "Three-layer Gaussian milky way: outer (sigma=55%h), mid (18%h), core (4%h) — realistic cool-white/blue-grey tones"
  - "Star sort by radius descending: larger stars appear first as Population knob increases"
  - "Semitone grid uses joystickXAtten/joystickYAtten as exact semitone counts (not area counts as v1.6 did)"
  - "Horizontal grid lines at higher alpha (0.38f in-scale) than vertical (0.28f in-scale) per VIS-12"
  - "glowPhase_ breathing math already in paint() at Plan 01; timerCallback wiring deferred to Plan 02"
  - "Corner-dimming gamepad block removed — near-black background makes it unnecessary"

patterns-established:
  - "Layer ordering: background -> milkyway -> stars -> heatmap (circle-clipped) -> grid -> particles -> cursor -> border"
  - "All image baking in resized(), zero allocation in paint()"
  - "ScopedSaveState wraps every drawImage with setOpacity to prevent opacity bleed"

requirements-completed:
  - VIS-03
  - VIS-07
  - VIS-12

# Metrics
duration: 25min
completed: 2026-03-04
---

# Phase 31 Plan 01: Paint-Only Visual Foundation — Space Stack Summary

**JoystickPad::resized() bakes milky way band + 300-star starfield + blue-magenta heatmap; paint() replaced with 8-layer space visual stack including semitone-driven grid with in/out-of-scale differentiation and beat-ready breathing glow ring**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-03-04T00:00:00Z
- **Completed:** 2026-03-04T00:25:00Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments

- Added `resized()` override that bakes 3 images (milky way diagonal Gaussian band, blue-magenta radial heatmap) and generates a 300-star deterministic starfield sorted by radius descending
- Rewrote `paint()` with all 8 layers: near-black background, density-driven milky way at scaled opacity, population-driven starfield, circle-clipped heatmap, semitone-count grid with dashed horizontal / solid vertical lines and in/out-of-scale alpha differentiation, particles (unchanged), dark halo + breathing glow ring (glowPhase_ ready for Plan 02), cursor dot + ticks, border at 0.30f alpha
- Build: 0 errors in Debug configuration — VST3 built and auto-installed

## Task Commits

Each task was committed atomically:

1. **Task 1: Add JoystickPad members to PluginEditor.h** - `feb78f0` (feat)
2. **Task 2: Implement JoystickPad::resized() + resetGlowPhase()** - `ed2eb14` (feat)
3. **Task 3: Rewrite JoystickPad::paint() with space stack** - `9ab6fc2` (feat)

## Files Created/Modified

- `Source/PluginEditor.h` — Added `resized()` and `resetGlowPhase()` declarations, `StarDot` struct, `milkyWayCache_`, `heatmapCache_`, `starfield_`, `glowPhase_` private members (lines 117-151)
- `Source/PluginEditor.cpp` — Added `JoystickPad::resized()` at line 827 (bakes 3 images + starfield); added `resetGlowPhase()` at line 930; replaced `paint()` body at line 1111 with 8-layer space stack (lines 1111-1317)

## Key Implementation Details

**resized() layout (PluginEditor.cpp lines 827-930):**
- Guard: `if (w < 2 || h < 2) return`
- milkyWayCache_ baked via BitmapData pixel loop, 3-layer Gaussian (outer/mid/core), diagonal band from (0, h*0.35) to (w, h*0.65)
- starfield_ generated with `juce::Random(0xDEADBEEF12345678LL)`, 300 stars, sorted by `r` descending
- heatmapCache_ baked via angle-based `atan2(-dy, dx)` → blue-to-magenta, alpha ramps with radius

**paint() key patterns (PluginEditor.cpp lines 1111-1317):**
- Layer 2 (milky way): `ScopedSaveState` + `g.setOpacity(bandAlpha)` where `bandAlpha = jmap(density, 1.0f, 8.0f, 0.15f, 1.0f)`
- Layer 3 (stars): `visCount = floor(300 * (pop-1)/63)`, draw first N from sorted starfield_
- Layer 4 (heatmap): `juce::Path circle; circle.addEllipse(circleRect); g.reduceClipRegion(circle)`
- Layer 5 (grid): `uint16_t scaleMask` bitmask; vertical lines solid; horizontal lines `g.drawDashedLine({3.0f, 3.0f})`; center ramp `1 - (1 - |t*2-1|) * 0.6`
- Layer 7b (glow): `breath = 0.5 + 0.5*sin(glowPhase_ * twoPi)`; glowAlpha 0.15..0.45; glowR dotR+4..dotR+12

## Decisions Made

- Milky way band goes from (0, h*0.35) to (w, h*0.65) giving ~-30 degree angle (top-left to bottom-right) as specified in CONTEXT.md
- Horizontal grid lines are brighter than vertical (in-scale H: 0.38f, in-scale V: 0.28f) per VIS-12
- getStep() caps grid density at 48 semitones (step=1), 96 (step=2), >96 (step=4) to prevent visual noise
- `glowPhase_` initialized to 0.0f; breathing glow ring math already written in paint() — Plan 02 only needs to advance glowPhase_ in timerCallback

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Build succeeded with 0 errors on first attempt.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- Plan 31-01 complete — Plan 31-02 can proceed to wire `glowPhase_` advancement in `JoystickPad::timerCallback()` and `joystickPad_.resetGlowPhase()` in `PluginEditor::timerCallback()`
- The breathing glow ring is already rendering (with glowPhase_=0.0f static); Plan 02 makes it animate

## Self-Check

- [x] `Source/PluginEditor.h` modified — `resized()`, `resetGlowPhase()`, `StarDot`, `milkyWayCache_`, `heatmapCache_`, `starfield_`, `glowPhase_` present
- [x] `Source/PluginEditor.cpp` modified — `JoystickPad::resized()` at line 827, `resetGlowPhase()` at line 930, new `paint()` at line 1111
- [x] Build: 0 errors Debug
- [x] git commits: feb78f0, ed2eb14, 9ab6fc2

---
*Phase: 31-paint-only-visual-foundation*
*Completed: 2026-03-04*
