# Requirements: ChordJoystick MK2

**Defined:** 2026-02-26
**Core Value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper with trigger quantization, and gamepad control — no competitor provides this as a unified instrument.

## v1.4 Requirements

### LFO System

- [x] **LFO-01**: User can enable/disable X-axis LFO independently (On/Off toggle)
- [x] **LFO-02**: User can enable/disable Y-axis LFO independently (On/Off toggle)
- [x] **LFO-03**: User can select waveform shape per LFO via dropdown (Sine / Triangle / Saw Up / Saw Down / Square / S&H / Random)
- [x] **LFO-04**: When Sync OFF → frequency slider (free-running rate in Hz); when Sync ON → same control becomes subdivision selector (1/1, 1/2, 1/4, 1/8, 1/16, 1/32)
- [x] **LFO-05**: User can set LFO phase shift via slider (0–360°)
- [x] **LFO-06**: User can set LFO distortion (additive jitter/noise amount) via slider
- [x] **LFO-07**: User can set LFO level (modulation depth / amplitude) via slider
- [x] **LFO-08**: User can sync LFO to tempo via a single Sync button — syncs to Free BPM; when DAW sync is also active, follows DAW BPM
- [x] **LFO-09**: LFO X output modulates joystick X axis as additive offset (output clamped to -1..1)
- [x] **LFO-10**: LFO Y output modulates joystick Y axis as additive offset (output clamped to -1..1)
- [x] **LFO-11**: LFO section is positioned to the left of the joystick pad; joystick shifted right to accommodate

### Beat Clock

- [x] **CLK-01**: A flashing dot next to the Free BPM knob pulses once per beat (internal free tempo)
- [x] **CLK-02**: Beat dot follows DAW clock (pulses on DAW beat) when DAW sync is active

### Performance & Stability

- [x] **PERF-01**: LFO S&H / Random waveforms use the project's existing LCG pattern (not std::rand) — audio-thread safe under MSVC
- [x] **PERF-02**: LFO output is applied as an additive local float offset inside buildChordParams() — never writes to shared joystick atomics
- [x] **PERF-03**: Synced LFO phase derived from ppqPos (DAW mode) or sample counter (free mode) — no free-accumulation drift across transport stops

### Gamepad Preset Control

- [ ] **CTRL-01**: Option button on gamepad toggles "preset scroll" mode; plugin UI shows a visible indicator when mode is active
- [ ] **CTRL-02**: In preset scroll mode, the BPM up/down gamepad controls instead send MIDI Program Change +1 / −1 on a configurable MIDI channel
- [ ] **CTRL-03**: Plugin UI displays the current MIDI program number while preset scroll mode is active

### Distribution

- [ ] **DIST-01**: GitHub v1.4 release created with built installer binary and release notes
- [ ] **DIST-02**: Full plugin copy backed up to Desktop/Dima_plug-in

## Future Requirements (v2+)

### LFO Extensions

- **LFO-EXT-01**: LFO modulation of filter CC (CC74/CC71) as a second injection point
- **LFO-EXT-02**: Triplet and dotted note sync subdivisions (1/4T, 1/8T, etc.)
- **LFO-EXT-03**: LFO-to-LFO modulation (X LFO modulates Y LFO rate)

### Beat Clock

- **CLK-EXT-01**: Audible metronome click (MIDI note on dedicated channel) — requires routing decision

## Out of Scope

| Feature | Reason |
|---------|--------|
| juce_dsp module | isMidiEffect()=true — SIMD audio buffer abstractions meaningless |
| LFO modulation written back to APVTS | Illegal from audio thread; would impose 20ms smoother |
| Continuous pitch vibrato from LFO | Existing pitch is sample-and-hold at trigger time — LFO varies pitch at next trigger |
| Soft sync (LFO gradually drifts to phase) | Hard reset on transport start is industry standard; soft sync adds complexity with no UX benefit |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| LFO-01 | Phase 13 | Complete |
| LFO-02 | Phase 13 | Complete |
| LFO-03 | Phase 12 | Complete |
| LFO-04 | Phase 12 | Complete |
| LFO-05 | Phase 12 | Complete |
| LFO-06 | Phase 12 | Complete |
| LFO-07 | Phase 12 | Complete |
| LFO-08 | Phase 13 | Complete |
| LFO-09 | Phase 13 | Complete |
| LFO-10 | Phase 13 | Complete |
| LFO-11 | Phase 14 | Complete |
| CLK-01 | Phase 14 | Complete |
| CLK-02 | Phase 14 | Complete |
| PERF-01 | Phase 12 | Complete |
| PERF-02 | Phase 12 | Complete |
| PERF-03 | Phase 12 | Complete |
| CTRL-01 | Phase 16 | Pending |
| CTRL-02 | Phase 16 | Pending |
| CTRL-03 | Phase 16 | Pending |
| DIST-01 | Phase 15 | Pending |
| DIST-02 | Phase 15 | Pending |

**Coverage:**
- v1.4 requirements: 21 total
- Mapped to phases: 21 (Phase 12: 8, Phase 13: 5, Phase 14: 3, Phase 15: 2, Phase 16: 3)
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-26*
*Last updated: 2026-02-26 — traceability populated by roadmapper*
