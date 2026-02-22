# Phase 03: Core MIDI Output and Note-Off Guarantee - Context

**Gathered:** 2026-02-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Wire TriggerSystem into processBlock to produce sample-accurate MIDI note-on/off via TouchPlate triggers. Guarantee note-off on all exit paths. Add gate LED feedback to TouchPlate buttons in PluginEditor at 30 Hz. Per-voice MIDI channel routing verified.

Creating new trigger sources (joystick motion, random) is Phase 04. Velocity sensitivity is deferred.

</domain>

<decisions>
## Implementation Decisions

### Velocity model
- Velocity is fixed — just enough to trigger voice envelopes of external software synths
- Fixed value (Claude's discretion: 100 or 127 — pick what reads as "standard trigger")
- Joystick position does NOT affect velocity; joystick is pitch only
- No new APVTS velocity parameter this phase — Claude checks existing APVTS and decides if one is needed
- Note-off velocity: Claude's discretion (velocity 0 is standard MIDI practice)
- Full velocity implementation (variable, per-voice) is deferred to a later phase

### Retrigger behavior
- Retrigger model: **close-then-reopen** — if a voice is already playing and its TouchPlate fires again, send note-off immediately then note-on with the new pitch
- If the same voice retriggered with a different pitch: always send note-off for the OLD pitch before note-on for the NEW pitch — no stuck notes
- Retrigger setting is global (one behavior for all 4 voices — no per-voice config)
- If two different voices are assigned to the same MIDI channel: detect the conflict and show a visual warning in the UI

### Gate LED appearance
- Simple on/off: button glows when gate is open, dark when closed
- No pulsing or fading — clean binary state
- One color for all 4 gate LEDs (consistent visual language — no per-voice colors)
- LED is embedded in each TouchPlate button (the pad itself glows)
- Reflects **pad input state**, not MIDI state — simpler, no feedback loop required
- Updates at 30 Hz (per roadmap)

### Note-off edge cases
- **Transport stop:** Do nothing — let held pads ring until the user releases the TouchPlate. Notes sustain naturally.
- **Bypass:** Send note-off immediately for all active voices when plugin is bypassed. No zombie notes.
- **releaseResources() / shutdown:** Best-effort note-off before deactivation. Try to flush active notes before the audio thread stops.
- **CC 123 (All Notes Off):** Not handled — this plugin is a MIDI generator only, it does not process incoming MIDI.

### Claude's Discretion
- Exact fixed velocity value (100 or 127)
- Note-off velocity value (likely 0 per MIDI spec)
- Whether a global velocity APVTS parameter is needed (check existing code)
- Exact color of the gate LED glow (warm white, green, or blue — something that reads clearly on the existing UI)
- How the MIDI channel conflict warning is displayed (tooltip, red border, label — keep it subtle)

</decisions>

<specifics>
## Specific Ideas

- "The trigger triggers voice envelopes of external software synths" — velocity doesn't matter much for this use case; consistent triggering is the goal
- Sample-accurate note placement is important (processBlock precision)

</specifics>

<deferred>
## Deferred Ideas

- Variable velocity (per TouchPlate pressure or joystick position) — future phase
- Per-voice retrigger mode (retrigger vs legato toggle per voice) — future phase
- CC 123 panic button handling — not applicable (generator-only plugin)

</deferred>

---

*Phase: 03-core-midi-output-and-note-off-guarantee*
*Context gathered: 2026-02-22*
