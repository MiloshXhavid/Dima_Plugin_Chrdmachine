# Requirements: ChordJoystick

**Defined:** 2026-02-21
**Core Value:** Live performance chord controller — play expressive chords with one hand while the other triggers notes, all quantized to a selected scale.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### MIDI Output

- [ ] **MIDI-01**: Plugin outputs MIDI notes via VST3 format
- [ ] **MIDI-02**: Plugin outputs MIDI notes via AUv3 format
- [ ] **MIDI-03**: Plugin runs in standalone mode without DAW
- [ ] **MIDI-04**: MIDI velocity sensitivity on note triggers
- [ ] **MIDI-05**: Per-note MIDI channel routing (optional: route each voice to different channel)

### Scale System

- [ ] **SCAL-01**: 12 chromatic buttons for custom scale selection (piano roll layout)
- [ ] **SCAL-02**: Scale preset selector (Major, Minor, Min Pent, Maj Pent, Harmonic Minor, Melodic Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian)
- [ ] **SCAL-03**: Joystick output quantized to selected scale
- [ ] **SCAL-04**: Global transpose knob (0-12 semitones, shifts all notes)

### Joystick Control

- [ ] **JOYS-01**: X-axis controls Fifth and Tension intervals
- [ ] **JOYS-02**: Y-axis controls Root note and Third intervals
- [ ] **JOYS-03**: XY attenuator knob (scales output range 0-127, e.g., 1 octave = 12)
- [ ] **JOYS-04**: Real-time pitch update on joystick movement

### Interval Control

- [ ] **INTV-01**: Interval knob for Third (0-12 semitones relative to root)
- [ ] **INTV-02**: Interval knob for Fifth (0-12 semitones relative to root)
- [ ] **INTV-03**: Interval knob for Tension (0-12 semitones relative to root)
- [ ] **INTV-04**: Global transpose knob acts as master pitch shift

### Octave Control

- [ ] **OCTV-01**: Octave knob for note 1 (Root) (-2 to +2 octaves)
- [ ] **OCTV-02**: Octave knob for note 2 (Third) (-2 to +2 octaves)
- [ ] **OCTV-03**: Octave knob for note 3 (Fifth) (-2 to +2 octaves)
- [ ] **OCTV-04**: Octave knob for note 4 (Tension) (-2 to +2 octaves)

### Touchplate Triggers

- [ ] **TOUC-01**: 4 touchplates for triggering notes
- [ ] **TOUC-02**: Each touchplate triggers its assigned note (note 1-4)
- [ ] **TOUC-03**: Latching behavior: pitch updates on trigger, note sustains
- [ ] **TOUC-04**: Re-triggering touchplate updates pitch from current joystick position

### Trigger System

- [ ] **TRIG-01**: Trigger source selector (touchplate / joystick movement)
- [ ] **TRIG-02**: Joystick movement triggers selected notes (configurable which)
- [ ] **TRIG-03**: Synced random trigger option with subdivision clock
- [ ] **TRIG-04**: Subdivision clock selector for random triggers

### Clock Sync

- [ ] **CLOK-01**: DAW clock sync (slave mode) via MIDI clock
- [ ] **CLOK-02**: Standalone clock (master mode) with adjustable BPM
- [ ] **CLOK-03**: Clock source selector (DAW/Standalone)

### Looper

- [ ] **LOOP-01**: Start/Stop button
- [ ] **LOOP-02**: Record button
- [ ] **LOOP-03**: Reset button
- [ ] **LOOP-04**: Loop subdivision selector (3/4, 4/4, 5/4, 7/8, 9/8, 11/8)
- [ ] **LOOP-05**: Loop length selector (1-16 bars)
- [ ] **LOOP-06**: Looper synced to selected clock

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Advanced Features

- **PRES-01**: Basic preset save/load
- **PERF-01**: Performance mode (rhythmic patterns from chords)
- **HARD-01**: Hardware joystick support via USB HID

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Audio synthesis | Keep MIDI-only; let users use their own synths |
| Unlimited polyphony | Cap at 8 notes for CPU efficiency |
| Cloud preset sharing | Local file export for v1 |
| Real-time audio analysis | Requires audio input path; adds latency |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| MIDI-01 | Phase 2 | Pending |
| MIDI-02 | Phase 2 | Pending |
| MIDI-03 | Phase 2 | Pending |
| MIDI-04 | Phase 2 | Pending |
| MIDI-05 | Phase 4 | Pending |
| SCAL-01 | Phase 2 | Pending |
| SCAL-02 | Phase 2 | Pending |
| SCAL-03 | Phase 2 | Pending |
| SCAL-04 | Phase 2 | Pending |
| JOYS-01 | Phase 3 | Pending |
| JOYS-02 | Phase 3 | Pending |
| JOYS-03 | Phase 3 | Pending |
| JOYS-04 | Phase 3 | Pending |
| INTV-01 | Phase 2 | Pending |
| INTV-02 | Phase 2 | Pending |
| INTV-03 | Phase 2 | Pending |
| INTV-04 | Phase 2 | Pending |
| OCTV-01 | Phase 2 | Pending |
| OCTV-02 | Phase 2 | Pending |
| OCTV-03 | Phase 2 | Pending |
| OCTV-04 | Phase 2 | Pending |
| TOUC-01 | Phase 3 | Pending |
| TOUC-02 | Phase 3 | Pending |
| TOUC-03 | Phase 3 | Pending |
| TOUC-04 | Phase 3 | Pending |
| TRIG-01 | Phase 4 | Pending |
| TRIG-02 | Phase 4 | Pending |
| TRIG-03 | Phase 4 | Pending |
| TRIG-04 | Phase 4 | Pending |
| CLOK-01 | Phase 2 | Pending |
| CLOK-02 | Phase 2 | Pending |
| CLOK-03 | Phase 2 | Pending |
| LOOP-01 | Phase 4 | Pending |
| LOOP-02 | Phase 4 | Pending |
| LOOP-03 | Phase 4 | Pending |
| LOOP-04 | Phase 4 | Pending |
| LOOP-05 | Phase 4 | Pending |
| LOOP-06 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 42 total
- Mapped to phases: 42
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-21*
*Last updated: 2026-02-21 after initial definition*
