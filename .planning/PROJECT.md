# ChordJoystick

## What This Is

ChordJoystick is a paid JUCE VST3 MIDI generator plugin for Windows that sends 4-voice chord MIDI data to an external synthesizer. Musicians control chord voicings in real time by moving an XY joystick (Y axis = Root + Third, X axis = Fifth + Tension), quantizing to a selected scale. The plugin can be driven by mouse/keyboard or optionally by a PS4/Xbox gamepad, and includes a DAW-synced looper for recording and replaying joystick gestures and trigger events.

## Core Value

An XY joystick mapped to harmonic space — combined with per-note trigger gates, scale quantization, a gesture looper, and gamepad control — that no existing MIDI tool provides as a unified instrument.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Plugin compiles to VST3 and loads in a DAW (Ableton, Reaper) without crashes
- [ ] Plugin generates MIDI note data (no audio output, pure MIDI generator)
- [ ] XY joystick (mouse-driven): Y axis controls Root + Third pitch, X axis controls Fifth + Tension pitch
- [ ] 4 individual interval knobs (Root always 0, Third/Fifth/Tension relative to root, 0–12 semitones)
- [ ] Global transpose knob (−24 to +24 semitones, shifts all voices)
- [ ] Per-voice octave offset knobs (−3 to +3 octaves precision adder)
- [ ] Joystick X and Y attenuator knobs (0–127 range, scales joystick output to MIDI range)
- [ ] 12 piano-style scale buttons for custom scale entry (chromatic order)
- [ ] Scale preset selector (20+ presets: Major, Minor, Harmonic Minor, Pentatonic, Blues, modes, etc.)
- [ ] Quantization: nearest note in scale, ties go DOWN, search over 2 octaves
- [ ] 4 touchplates (Root / Third / Fifth / Tension): press = sample-and-hold pitch + MIDI note-on, release = note-off
- [ ] Per-voice trigger source: TouchPlate, Joystick movement, or Random
- [ ] Random gate: synced subdivisions (1/4, 1/8, 1/16, 1/32) with density knob
- [ ] Looper: record/play/stop/reset/delete, DAW-synced, time signatures (3/4 4/4 5/4 7/8 9/8 11/8), 1–16 bars length
- [ ] Looper records both joystick position AND gate events timestamped to beat position
- [ ] PS4 / Xbox controller support via SDL2 (optional — plugin fully usable with mouse only):
  - Right stick → pitch joystick XY
  - R1 = Root, R2 = Third, L1 = Fifth, L2 = Tension triggers
  - Left stick X → CC74 (filter cutoff), Y → CC71 (filter resonance)
  - L3 (left stick click) → trigger all 4 notes simultaneously
  - Cross = looper start/stop, Square = reset, Triangle = record, Circle = delete
- [ ] Filter CC attenuators: separate X/Y knobs scaling gamepad left-stick output (0–127)
- [ ] Per-voice MIDI channel routing (voices 1–4 to any MIDI channel 1–16)
- [ ] Filter CC MIDI channel selector
- [ ] State persistence (all APVTS parameters saved/recalled per DAW session/preset)

### Out of Scope

- Audio synthesis — plugin sends MIDI only, no sound engine
- Built-in preset browser — v1 relies on DAW preset management
- MIDI input processing — plugin generates MIDI, does not process incoming MIDI
- Standalone app mode — VST3 host required
- macOS / Linux support — Windows first, cross-platform is v2
- MPE / polyphonic expression — standard MIDI channels only
- Physical hardware touchplates/joystick — UI controls only (gamepad is the hardware bridge)

## Context

- **Codebase status**: Full implementation already written in this session — CMakeLists.txt + 14 source files in Source/. Not yet compiled or tested.
- **Build system**: JUCE 8 via CMake FetchContent + SDL2 static (release-2.30.9). VST3 target only.
- **Language**: C++17, JUCE 8, SDL2 for gamepad
- **Development OS**: Windows 11 Pro
- **Target DAWs**: Ableton Live, Reaper (primary test targets)
- **Distribution plan**: Paid release via Gumroad or own website
- **Audience**: Releasing to other musicians — UI polish, stability, and usability without a gamepad all matter

## Constraints

- **Tech Stack**: JUCE 8 + CMake — locked, all MIDI plugin infrastructure follows JUCE patterns
- **Gamepad**: SDL2 static lib — must initialize with `SDL_INIT_GAMECONTROLLER` only (no video subsystem)
- **MIDI**: 4 voices across configurable channels + filter CCs (CC74 cutoff, CC71 resonance)
- **Platform**: Windows 11 first; macOS/Linux deferred
- **No audio buses**: Plugin is pure MIDI effect — `isBusesLayoutSupported` must reject all audio buses

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| JUCE 8 + CMake FetchContent | Modern JUCE build, no Projucer dependency | — Pending |
| SDL2 for gamepad (static) | Cross-platform controller support, normalizes PS4+Xbox | — Pending |
| VST3 only (no AU/Standalone) | Windows target, reduce complexity for v1 | — Pending |
| CC74 cutoff + CC71 resonance | MIDI standard filter CCs, maximum synth compatibility | — Pending |
| Sample-and-hold pitch | Pitch only updates when pad is triggered, not on joystick move | — Pending |
| Quantize ties → down | Deterministic behavior at scale note equidistance | — Pending |
| 4 voices on channels 1–4 default | Simple routing default, user-configurable per voice | — Pending |

---
*Last updated: 2026-02-22 after initialization*
