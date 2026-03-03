# Phase 29: Looper Perimeter Bar - Context

**Gathered:** 2026-03-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace the existing 4px horizontal progress strip (`looperPositionBarBounds_`) with a clockwise-traveling bar that circuits the outer edge of the LOOPER section rectangle (`looperPanelBounds_`). The bar completes one full lap per loop cycle. The "LOOPER" knockout label must remain fully legible at all times. No other looper behavior changes.

</domain>

<decisions>
## Implementation Decisions

### Bar appearance
- 2px thick
- Color: `Clr::gateOn` (same green used for active gates / existing progress fill)

### Label protection
- The "LOOPER" knockout label sits at the top-left corner of the panel border (the start/end point of the circuit)
- The bar skips over the label area — jumps past the text so it never paints over the characters at any point in the circuit

### Idle / stopped state
- When the looper is stopped or no loop is loaded: render a dim ghost ring — a faint full-perimeter outline that hints the animation path
- When playing or recording: the traveling bar replaces the ghost ring

### Animation feel
- Leading edge with a trailing tail/fade behind it
- Tail fades out (opacity gradient or shortened fill) — exact tail length/falloff is Claude's Discretion

### Claude's Discretion
- Exact tail length and fade curve
- Ghost ring color (likely `Clr::gateOff` or a dimmed `Clr::gateOn`)
- Whether to keep or remove the `looperPositionBarBounds_` strip (remove it — it is being replaced)
- How to expand the 30Hz repaint region from `looperPositionBarBounds_` to `looperPanelBounds_`

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `looperPanelBounds_` (`juce::Rectangle<int>`): the LOOPER section rectangle — this is the perimeter to animate around
- `looperPositionBarBounds_`: existing 4px horizontal strip — will be removed/replaced
- `proc_.looperIsPlaying()`, `proc_.looperIsRecording()`: state queries for when to show the traveling bar vs ghost ring
- `proc_.getLooperLengthBeats()`, `proc_.getLooperPlaybackBeat()`: fractional position [0,1] for bar placement
- `Clr::gateOn` / `Clr::gateOff`: established colors for active/inactive states
- `drawLfoPanel(looperPanelBounds_, "LOOPER")`: draws the panel fill + border + "LOOPER" label at top-left — bar must coexist with this

### Established Patterns
- 30Hz `timerCallback` in `PluginEditor.cpp` already calls `repaint(looperPositionBarBounds_)` — needs to be updated to repaint `looperPanelBounds_` instead
- Progress bar drawn inside `paint()` in `PluginEditor.cpp` after section panels are drawn — new perimeter bar goes in the same block

### Integration Points
- `PluginEditor::paint()` — replace the looper position bar paint block (~line 3550)
- `PluginEditor::resized()` — `looperPositionBarBounds_` layout can be removed or zeroed out
- `PluginEditor::timerCallback()` — repaint target changes from strip bounds to panel bounds

</code_context>

<specifics>
## Specific Ideas

- The roadmap specifies: bar starts and ends at top-left corner, travels right → down → left → up (clockwise)
- The circuit must be exactly one lap per loop cycle (fraction from `getLooperPlaybackBeat / getLooperLengthBeats` drives position)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 29-looper-perimeter-bar*
*Context gathered: 2026-03-03*
