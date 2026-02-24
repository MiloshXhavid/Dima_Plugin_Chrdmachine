# Requirements: ChordJoystick

**Defined:** 2026-02-24
**Core Value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper, gamepad control — no competitor provides this as a unified instrument.

## v1.1 Requirements

### PATCH — Post-v1.0 Implementations (Formalized)

- [ ] **PATCH-01**: Plugin sends CC64=127 (sustain on) before each note-on so sustain-pedal-aware synths don't cut notes
- [ ] **PATCH-02**: Per-voice hold mode — pad press sends note-off (mute), pad release sends note-on with current pitch (inverse gate)
- [ ] **PATCH-03**: Filter Mod button gates all left-stick CC output; Left X/Y mode dropdowns and atten knobs visually disabled when Filter Mod is off
- [ ] **PATCH-04**: MIDI Panic button is a one-shot action — press sends allNotesOff and button immediately returns to pressable state (no persistent mute or output-blocking toggle)
- [ ] **PATCH-05**: Filter looper recording always active (recFilter_=true) — left-stick gestures are recorded to loop when looper is in record mode
- [ ] **PATCH-06**: Joystick threshold applied to filter CC dedup — changing atten knobs does not retrigger filter CC when gamepad axis has natural noise

### PANIC — Improved MIDI Panic

- [ ] **PANIC-01**: Panic sends full MIDI reset sequence: CC64=0, CC120 (All Sound Off), CC123 (All Notes Off) — no CC121 (corrupts downstream VST3 instrument parameters)
- [ ] **PANIC-02**: Panic sweeps all 16 MIDI channels (not just the 4 configured voice channels) to clear stuck notes regardless of routing
- [ ] **PANIC-03**: Panic button shows brief flash feedback on press (using existing flashPanic_ counter pattern) — no persistent mute state

### QUANT — Trigger Quantization

- [ ] **QUANT-01**: Quantize toggle in looper section — when ON, recorded gate events are snapped to the selected subdivision grid in real-time during recording
- [ ] **QUANT-02**: Post-record QUANTIZE action button — re-snaps all gate events in the current loop to the selected subdivision without re-recording
- [ ] **QUANT-03**: Quantize subdivision selector (1/4, 1/8, 1/16, 1/32 notes) — independent control from the random gate subdivision per voice

### UI — Visual & Release

- [ ] **UI-01**: Section visual grouping — Filter Mod section, looper section, and gamepad section each have clear visual separators or headers in the plugin UI
- [ ] **UI-02**: Gamepad status shows connected controller name or type (PS4 / Xbox / Generic), not just ON/OFF
- [ ] **UI-03**: Looper playback position bar — visual progress indicator showing current beat position within the loop length
- [ ] **UI-04**: Installer rebuilt for v1.1 with updated version string and new feature list in installer description page

## v1.2 Requirements

(None defined yet)

## Out of Scope

| Feature | Reason |
|---------|--------|
| Audio synthesis | Plugin sends MIDI only — no sound engine |
| Built-in preset browser | DAW preset management sufficient for v1 |
| MIDI input processing | Plugin generates MIDI, does not process incoming |
| Standalone app mode | VST3 host required |
| macOS / Linux support | Windows first, cross-platform deferred to v2 |
| MPE / polyphonic expression | Standard MIDI channels only |
| Code signing | SmartScreen acceptable for v1; deferred to v2 |
| Looper joystick quantization | Only gate event quantization in scope; joystick is continuous |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PATCH-01 | Phase 08 | Pending |
| PATCH-02 | Phase 08 | Pending |
| PATCH-03 | Phase 08 | Pending |
| PATCH-04 | Phase 08 | Pending |
| PATCH-05 | Phase 08 | Pending |
| PATCH-06 | Phase 08 | Pending |
| PANIC-01 | Phase 09 | Pending |
| PANIC-02 | Phase 09 | Pending |
| PANIC-03 | Phase 09 | Pending |
| QUANT-01 | Phase 10 | Pending |
| QUANT-02 | Phase 10 | Pending |
| QUANT-03 | Phase 10 | Pending |
| UI-01 | Phase 11 | Pending |
| UI-02 | Phase 11 | Pending |
| UI-03 | Phase 11 | Pending |
| UI-04 | Phase 11 | Pending |

**Coverage:**
- v1.1 requirements: 16 total
- Mapped to phases: 16
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-24*
*Last updated: 2026-02-24 — traceability populated by roadmapper*
