# ChordJoystick - MIDI Chord Generator Plugin

## What This Is

A JUCE-based MIDI plugin that generates 4 simultaneous pitch signals and gate triggers controlled by a dual-axis joystick. Users select a scale and chord structure via joystick position, then trigger notes via touchplates. Includes a looper for recording and looping gesture sequences synchronized to DAW tempo.

## Core Value

Live performance chord controller — play expressive chords with one hand while the other triggers notes, all quantized to a selected scale.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Joystick-controlled pitch generation (4 notes)
- [ ] Scale-quantized output via chromatic buttons
- [ ] Preset scale selector (Maj, Min, Pentatonic, etc.)
- [ ] Interval control knobs (root, 3rd, 5th, tension)
- [ ] Global transpose knob
- [ ] XY axis attenuator (0-127 range)
- [ ] Per-note octave adjustment (4 knobs)
- [ ] 4 touchplates for triggering notes
- [ ] Latching behavior (pitch updates on trigger)
- [ ] Trigger source selection (touchplate / joystick movement)
- [ ] Synced random trigger option with subdivision clock
- [ ] Looper with transport controls (start/stop/record/reset)
- [ ] Loop subdivision selector (3/4, 4/4, 5/4, 7/8, 9/8, 11/8)
- [ ] Loop length selector (1-16 bars)
- [ ] DAW clock sync (slave mode)
- [ ] Standalone clock (master mode)

### Out of Scope

- Audio synthesis (MIDI output only)
- Hardware joystick integration (mouse/keyboard control initially)
- Preset saving/loading for joystick positions

## Context

- User has built similar functionality in PureData (without looper)
- Targeting JUCE framework for professional AUv3/VST3 plugin
- Works as virtual plugin and potentially with hardware controllers
- Both performance (live playing) and composition (idea sketching) use cases

## Constraints

- **Platform**: JUCE C++ (cross-platform)
- **Formats**: AUv3 (macOS), VST3 (Windows/Linux)
- **Output**: MIDI only (no audio synthesis)
- **Sync**: Must support both DAW-synced and standalone clock modes

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| JUCE framework | Professional plugin ecosystem, cross-platform AUv3/VST3 | — Pending |
| MIDI output only | User already has software synths to control | — Pending |
| Latching touchplates | Allows hand to leave touchplate while note sustains | — Pending |

---
*Last updated: 2026-02-21 after questioning*
