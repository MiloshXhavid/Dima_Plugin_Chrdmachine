# Phase 04: Per-Voice Trigger Sources and Random Gate - Context

**Gathered:** 2026-02-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Add joystick-motion and random subdivision trigger sources to TriggerSystem, selectable per voice via a 3-way toggle UI control. The existing TouchPlate trigger (from Phase 03) remains unchanged and is the default. Per-voice MIDI channel routing, retrigger rules, and note-off guarantees from Phase 03 all apply to the new trigger sources.

LooperEngine integration is Phase 05. Velocity sensitivity is deferred. SDL2 gamepad triggers are Phase 06.

</domain>

<decisions>
## Implementation Decisions

### Trigger source selector UI
- **Control:** Small 3-way toggle per voice using short codes: **PAD / JOY / RND**
- **Position:** Below each TouchPlate pad (4 toggles total, one per voice)
- **Inactive pad behavior:** When a voice is in JOY or RND mode, its TouchPlate pad dims visually and becomes non-functional (no manual trigger from pad in those modes)
- The toggle is a per-voice APVTS parameter (triggerSource0..3 already defined)

### Joystick motion threshold
- **Scope:** One global threshold — all joystick-mode voices share the same sensitivity setting
- **Control:** A slider labeled THRESHOLD (or SENS) — not a knob
- **Gate model:** Continuous gate — note-on fires when motion exceeds threshold, note-off fires when motion falls back below threshold (joystick stops moving)
- **Axis:** Both axes contribute — any movement in any direction (X or Y) triggers; combined magnitude used for threshold comparison
- **Minimum gate duration:** ~50ms minimum to prevent sub-millisecond click artifacts from tiny joystick jitter

### Random gate behavior
- **Density model:** Average hits per bar (range 1–8) — not raw probability percentage
- **Pitch at trigger time:** Current joystick position at the moment the random trigger fires (live pitch, not sample-and-hold)
- **Subdivision:** Per-voice — each voice in RND mode has its own subdivision selector (1/4, 1/8, 1/16, 1/32); this uses the existing randomSubdiv APVTS parameter
- **Transport-stopped behavior:** Random gate fires based on wall-clock time even when DAW transport is stopped (always-on generative behavior)
- **When transport IS playing:** Use ppqPosition for subdivision alignment to stay in sync with the DAW

### Note duration for non-pad triggers
- **Random notes:** Configurable gate-time knob — range from very short to full subdivision length; determines how long each random-triggered note sustains
- **Joystick motion notes:** Duration = however long motion stays above threshold; minimum 50ms (see threshold section)
- **Retrigger rule:** Same as Phase 03 — always close-then-reopen, one note at a time per voice (no polyphonic overlap within a voice)
- **Random retrigger:** If a note is still ringing when the next random trigger fires, cut the previous note immediately and start the new one

### Claude's Discretion
- Exact slider/knob placement in the editor layout (wherever it fits cleanly near the existing controls)
- Implementation of wall-clock timing when transport is stopped (e.g., juce::Time::getMillisecondCounter() or a sample-counting fallback)
- How combined XY magnitude is computed for threshold comparison (e.g., sqrt(dx²+dy²) or max(|dx|, |dy|))
- APVTS parameter name for the gate-time knob (e.g., `randomGateTime` or `randomGate`)
- The gate-time knob range and default value

</decisions>

<specifics>
## Specific Ideas

- The 3-way toggle (PAD / JOY / RND) should be compact and legible — fits below the existing TouchPlate pads without expanding the editor height significantly
- Random gate as "always on" generative tool (even without transport) gives it a standalone compositional use — useful when sketching without a running session

</specifics>

<deferred>
## Deferred Ideas

- Per-voice joystick threshold (each voice has its own sensitivity) — future phase
- Variable velocity on random/joystick triggers — velocity implementation deferred globally
- SDL2 gamepad button triggers for joystick and random mode selection — Phase 06
- CC-controllable density parameter (external modulation of random density) — backlog

</deferred>

---

*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Context gathered: 2026-02-22*
