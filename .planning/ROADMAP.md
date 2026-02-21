# Roadmap: ChordJoystick

## Overview

A JUCE-based MIDI plugin that generates 4-note chords controlled by dual-axis joystick, with touchplate triggers and looper for live performance. Journey: Build foundation → Core MIDI engine → UI controls → Advanced triggers → Looper.

## Phases

- [ ] **Phase 1: Project Setup** - Build pipeline, CMake, parameter infrastructure
- [ ] **Phase 2: Core MIDI Engine** - MIDI output, scale system, interval/octave controls, clock sync
- [ ] **Phase 3: Input Controls** - Joystick XY control, touchplate triggers, GUI components
- [ ] **Phase 4: Trigger System** - Source selection, joystick triggers, random subdivision triggers
- [ ] **Phase 5: Looper** - Transport controls, loop recording, sync to clock

## Phase Details

### Phase 1: Project Setup
**Goal**: Establish JUCE build pipeline with CMake and define all APVTS parameters
**Depends on**: Nothing (first phase)
**Requirements**: None (foundational)
**Success Criteria** (what must be TRUE):
  1. Project builds via CMake with VST3, AUv3, and Standalone targets
  2. Plugin loads in test DAW without crashes
  3. All planned parameters exist in APVTS (velocity, channel routing, scales, intervals, octaves, transpose, attenuator, trigger source, clock, looper)
  4. Basic PluginProcessor/PluginEditor shell compiles and runs
**Plans:** 2 plans

Plans:
- [x] 01-01-PLAN.md — CMake build pipeline + basic PluginProcessor/Editor shell
- [ ] 01-02-PLAN.md — Full APVTS parameters + build verification


### Phase 2: Core MIDI Engine
**Goal**: Transform parameter inputs into quantized MIDI note output
**Depends on**: Phase 1
**Requirements**: MIDI-01, MIDI-02, MIDI-03, MIDI-04, SCAL-01, SCAL-02, SCAL-03, SCAL-04, INTV-01, INTV-02, INTV-03, INTV-04, OCTV-01, OCTV-02, OCTV-03, OCTV-04, CLOK-01, CLOK-02, CLOK-03
**Success Criteria** (what must be TRUE):
  1. Plugin outputs MIDI notes in VST3 format that appear in DAW
  2. Plugin outputs MIDI notes in AUv3 format on macOS
  3. Plugin runs in standalone mode without DAW
  4. Notes are quantized to selected scale (Major, Minor, Pentatonic, etc.)
  5. Global transpose shifts all output notes by specified semitones
  6. Per-note octave adjustment changes individual voice octaves
  7. MIDI velocity responds to touchplate trigger intensity
  8. DAW clock sync receives MIDI clock and aligns output
  9. Standalone clock runs at adjustable BPM
**Plans**: TBD

### Phase 3: Input Controls
**Goal**: Interactive joystick and touchplate UI for real-time control
**Depends on**: Phase 2
**Requirements**: JOYS-01, JOYS-02, JOYS-03, JOYS-04, TOUC-01, TOUC-02, TOUC-03, TOUC-04
**Success Criteria** (what must be TRUE):
  1. Joystick X-axis visually controls Fifth and Tension interval selection
  2. Joystick Y-axis visually controls Root and Third interval selection
  3. XY attenuator knob scales the output range (e.g., 1 octave = 12)
  4. Pitch updates in real-time as joystick moves
  5. Four touchplates render in UI and respond to clicks
  6. Each touchplate triggers its assigned note (note 1-4)
  7. Touchplates latch: note sustains after finger leaves
  8. Re-triggering touchplate updates pitch from current joystick position
**Plans**: TBD

### Phase 4: Trigger System
**Goal**: Flexible trigger sources including joystick movement and random subdivision
**Depends on**: Phase 3
**Requirements**: TRIG-01, TRIG-02, TRIG-03, TRIG-04, MIDI-05
**Success Criteria** (what must be TRUE):
  1. UI selector allows choosing trigger source (touchplate / joystick movement)
  2. Joystick movement triggers configured notes automatically
  3. Random trigger option fires notes at subdivisions (1/4, 1/8, 1/16, etc.)
  4. Subdivision clock selector configures random trigger timing
  5. Per-note MIDI channel routing allows routing each voice to different channel
**Plans**: TBD

### Phase 5: Looper
**Goal**: Record and loop gesture sequences synced to clock
**Depends on**: Phase 4
**Requirements**: LOOP-01, LOOP-02, LOOP-03, LOOP-04, LOOP-05, LOOP-06
**Success Criteria** (what must be TRUE):
  1. Start/Stop button controls looper playback
  2. Record button captures gesture sequences
  3. Reset button clears recorded loop
  4. Loop subdivision selector works (3/4, 4/4, 5/4, 7/8, 9/8, 11/8)
  5. Loop length selector sets 1-16 bars
  6. Looper stays in sync with selected clock (DAW or standalone)
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Project Setup | 1/2 | In progress | 01-01 |
| 2. Core MIDI Engine | 0/N | Not started | - |
| 3. Input Controls | 0/N | Not started | - |
| 4. Trigger System | 0/N | Not started | - |
| 5. Looper | 0/N | Not started | - |

---

*Roadmap created: 2026-02-21*
*Based on v1 requirements (42 total)*
