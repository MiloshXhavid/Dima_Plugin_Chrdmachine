# Phase 06: SDL2 Gamepad Integration - Context

**Gathered:** 2026-02-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Fix SDL lifecycle bugs and wire a physical gamepad (PS4/Xbox) into the running plugin:
- Right stick → joystick XY (same as mouse/touch joystick pad)
- Buttons → per-voice TouchPlate triggers
- Left stick → CC74 (cutoff) + CC71 (resonance)
- Hot-plug detection confirmed working
- Process-level SDL singleton (reference-counted) to support multiple plugin instances
- No CC floods when controller disconnected

Creating or modifying the gamepad button layout beyond what's already assigned (L1/R1/L2/R2 = voices, etc.) is out of scope.

</domain>

<decisions>
## Implementation Decisions

### Axis Dead Zones

- **Reuse existing `joystickThreshold` APVTS param** for the right stick dead zone — no new param or UI knob needed
- **Shape:** Square/per-axis (each axis independently ignored below threshold) — consistent with the existing Chebyshev distance approach already in the codebase
- **Center return behavior:** Hold last pitch (sample-and-hold) — when stick returns within dead zone, pitch stays at last reported position; consistent with the existing joystick mouse model
- **Button debounce:** 20ms debounce on gamepad buttons to prevent double-triggers on noisy hardware

### UI Connection Indicator

- **Small text label at the bottom of PluginEditor** (status-bar style, bottom edge)
- Label text: `"GAMEPAD: connected"` when active, `"GAMEPAD: none"` when absent
- **Hot-plug:** Silent — just start working; label updates, no other UI change or notification
- **Disconnect:** Label updates to `"GAMEPAD: none"` — no other notification

### Multi-Controller Behavior

- **Controller selection:** First controller found at SDL_Init is used — stable, no UI selector needed
- **On disconnect:** Silently go to "no gamepad" (label updates to `"GAMEPAD: none"`)
- **Stuck notes on disconnect:** Send all-notes-off on disconnect to prevent stuck MIDI notes
- **Multi-instance support:** SDL singleton shared across instances (reference-counted SDL_Init/SDL_Quit as already scoped in ROADMAP)
- **Per-instance GAMEPAD ACTIVE toggle:** Add a toggle button (e.g. `[GAMEPAD ON]`/`[GAMEPAD OFF]`) in PluginEditor so users can deactivate gamepad input on a specific instance when multiple ChordJoystick instances are open — only the instance with GAMEPAD ON active reads the controller

### CC Value-Change Threshold

- **Gate strategy:** Both dead zone AND value-change dedup — dead zone suppresses center wobble; dedup prevents redundant CC when stick moves slowly (only emit when mapped 0–127 integer value changes by ≥1)
- **Dead zone for CC axis:** Reuse same `joystickThreshold` param — one threshold param controls both right and left stick
- **CC output range:** Attenuated by existing `filterXAtten`/`filterYAtten` APVTS params (already wired for this purpose)
- **On disconnect:** Send CC reset to value 0 on both CC74 and CC71 — synth filter returns to closed/minimum position cleanly

### Claude's Discretion

- Exact SDL event polling vs callback design for hot-plug detection
- Internal SDL singleton implementation details (static instance, atomic ref-count, etc.)
- GAMEPAD ACTIVE toggle placement within the PluginEditor bottom section (near the status label)

</decisions>

<specifics>
## Specific Ideas

- The per-instance `[GAMEPAD ON/OFF]` toggle is specifically for the multi-instance scenario: two ChordJoystick tracks in a DAW, user wants only one responding to the physical controller
- CC reset-to-0 on disconnect should be sent on the configured `filterMidiCh` channel (not all channels)
- The 20ms button debounce should be implemented in `GamepadInput` — not in `TriggerSystem` (keep each subsystem clean)

</specifics>

<deferred>
## Deferred Ideas

- None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-sdl2-gamepad-integration*
*Context gathered: 2026-02-23*
