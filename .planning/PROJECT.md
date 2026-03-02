# ChordJoystick

## What This Is

ChordJoystick is a paid JUCE VST3 MIDI generator plugin for Windows that sends 4-voice chord MIDI data to an external synthesizer. Musicians control chord voicings in real time by moving an XY joystick (Y axis = Root + Third, X axis = Fifth + Tension), quantizing to a selected scale. The plugin can be driven by mouse/keyboard or optionally by a PS4/PS5/Xbox gamepad, and includes a DAW-synced looper for recording and replaying joystick gestures and trigger events with live or post-record quantization.

## Core Value

An XY joystick mapped to harmonic space — combined with per-note trigger gates, scale quantization, a gesture looper with trigger quantization, and gamepad control — that no existing MIDI tool provides as a unified instrument.

## Current State (v1.5)

- **Shipped:** 2026-03-02
- **GitHub:** https://github.com/MiloshXhavid/Dima_Plugin_Chrdmachine/releases/tag/v1.5 (pre-release)
- **Codebase:** ~7,000+ C++ LOC, 20+ source files + Catch2 test suite
- **Build:** JUCE 8.0.4 + CMake FetchContent + SDL2 2.30.9 static, VS 18 2026, Release
- **Installer:** `installer/Output/DIMEA-ChordJoystickMK2-v1.5-Setup.exe` (Inno Setup 6, static CRT, no MSVC redist required)
- **Known limitation:** COPY_PLUGIN_AFTER_BUILD requires elevation — use `fix-install.ps1` manually after rebuild

## Requirements

### Validated — v1.0

- ✓ Plugin compiles to VST3 and loads in a DAW without crashes — v1.0
- ✓ Plugin generates MIDI note data (pure MIDI generator, no audio output) — v1.0
- ✓ XY joystick: Y axis = Root + Third, X axis = Fifth + Tension — v1.0
- ✓ 4 interval knobs (Third/Fifth/Tension relative to root, 0–12 semitones) — v1.0
- ✓ Global transpose knob (0–11 semitones, note-name display) — v1.0
- ✓ Per-voice octave offset knobs (−3 to +3 octaves) — v1.0
- ✓ Joystick X and Y attenuator knobs (0–127) — v1.0
- ✓ 12 piano-style scale buttons + auto-copy from preset — v1.0
- ✓ Scale preset selector (20+ presets) — v1.0
- ✓ Quantization: nearest note in scale, ties down, 2-octave search — v1.0
- ✓ 4 touchplates: press = note-on, release = note-off — v1.0
- ✓ Per-voice trigger source: TouchPlate, Joystick, or Random — v1.0
- ✓ Random gate: synced subdivisions with density knob — v1.0
- ✓ Looper: record/play/stop/reset/delete, DAW-synced, time signatures, 1–16 bars — v1.0
- ✓ Looper records joystick position + gate events timestamped to beat position — v1.0
- ✓ PS4/PS5/Xbox controller support via SDL2 — v1.0
- ✓ Filter CC attenuators (CC74 cutoff, CC71 resonance) — v1.0
- ✓ Per-voice MIDI channel routing (1–16) — v1.0
- ✓ Filter CC MIDI channel selector — v1.0
- ✓ State persistence (APVTS) — v1.0

### Validated — v1.3

- ✓ MIDI Panic: CC64=0 + CC120 + CC123 on all 16 channels (48 events, no CC121) — v1.3
- ✓ PANIC! button with flash feedback, one-shot (no persistent mute) — v1.3
- ✓ Trigger quantization: Off / Live (during record) / Post (re-snap existing loop) — v1.3
- ✓ Quantize subdivision selector: 1/4, 1/8, 1/16, 1/32 — v1.3
- ✓ Gamepad status label shows controller type: PS4 / PS5 / Xbox / Controller Connected / No controller — v1.3
- ✓ Looper playback position bar (10px strip, 30 Hz update) — v1.3
- ✓ LOOPER section panel with rounded border and floating knockout label — v1.3
- ✓ Installer v1.3 with feature list — v1.3

### Validated — v1.4

- ✓ Dual LFO engine (X + Y axis) — 7 waveforms, Freq/Phase/Distortion/Level, DAW-sync — v1.4
- ✓ LFO UI section left of joystick — shape dropdown, per-axis on/off, Sync toggle — v1.4
- ✓ Beat clock dot on Free BPM knob face — flashes on beat, follows DAW sync — v1.4
- ✓ Gamepad Option mode: D-pad Up/Down scrolls Program Changes, OPTION indicator — v1.4
- ✓ GitHub v1.4 release + desktop backup — v1.4

### Validated — v1.5

- ✓ Single Channel routing mode — all 4 voices → one MIDI channel, no note collisions — v1.5
- ✓ Per-voice Sub Octave — fires parallel note 1 oct lower; Hold/SubOct split button on pad UI — v1.5
- ✓ Left Joystick X/Y target expansion — LFO freq/phase/level + gate length as modulation targets — v1.5
- ✓ LFO recording — arm, record 1 cycle, playback; Distort stays live; Clear button — v1.5
- ✓ Arpeggiator — 6 modes, 4 rates, gate length, bar-boundary launch — v1.5
- ✓ Random trigger extensions — Free/Hold modes, Population + Probability knobs, 1/64 subdiv — v1.5
- ✓ Option Mode 1 — Circle=Arp, Triangle=Rate, Square=Order, X=RND Sync; R3+pad=Sub Oct — v1.5
- ✓ LFO joystick visual tracking — sliders track joystick at 30 Hz, base+offset model — v1.5
- ✓ Bug fix: Looper anchor drift after record cycle — v1.5
- ✓ Bug fix: PS4 BT reconnect crash — v1.5

### Active — v1.6

- [ ] **DEF-01**: Third octave knob displays "4" by default
- [ ] **DEF-02**: Fifth octave knob displays "4" by default
- [ ] **DEF-03**: Tension octave knob displays "3" by default
- [ ] **DEF-04**: Scale preset defaults to Natural Minor on fresh load
- [ ] **BUG-03**: Remove `else noteCount_ = 0` clamp from all 5 note-off paths — eliminates stuck notes when any two voices produce the same pitch in Single Channel mode
- [ ] **TRIP-01**: Random trigger subdivision selector adds 1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T
- [ ] **TRIP-02**: Quantize trigger subdivision selector adds the same 6 triplet options
- [ ] **RND-08**: Random Free + RND SYNC OFF → gates fire at truly random intervals, no tempo grid
- [ ] **RND-09**: Random Free + RND SYNC ON + DAW Sync OFF → gates subdivided to internal free-tempo clock
- [ ] **RND-10**: Random Free + DAW Sync ON → gates subdivided to DAW beat grid
- [ ] **LOOP-01**: Remove existing linear looper progress bar (strip below Rec gates)
- [ ] **LOOP-02**: Rectangular perimeter bar travels clockwise around Looper box — one circuit = one loop cycle, 30 Hz
- [ ] **LOOP-03**: Bar starts/ends at the Looper label position, travels right → bottom-right → bottom-left → top-left
- [ ] **LOOP-04**: Looper label remains fully visible at all times

### Out of Scope

| Feature | Reason |
|---------|--------|
| Audio synthesis | Plugin sends MIDI only |
| Built-in preset browser | DAW preset management sufficient |
| MIDI input processing | Plugin generates, does not process |
| Standalone app mode | VST3 host required |
| macOS / Linux support | Windows first, cross-platform deferred to v2 |
| MPE / polyphonic expression | Standard MIDI channels only |
| Code signing | SmartScreen acceptable for v1; deferred to v2 |
| Looper joystick quantization | Only gate event quantization in scope |

## Constraints

- **Tech Stack**: JUCE 8 + CMake — locked, all MIDI plugin infrastructure follows JUCE patterns
- **Gamepad**: SDL2 static lib — must initialize with `SDL_INIT_GAMECONTROLLER` only
- **MIDI**: 4 voices across configurable channels + filter CCs (CC74 cutoff, CC71 resonance)
- **Platform**: Windows 11 first; macOS/Linux deferred
- **No audio buses**: Pure MIDI effect — `isBusesLayoutSupported` rejects all audio buses

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| JUCE 8 + CMake FetchContent | Modern JUCE build, no Projucer dependency | ✓ Good |
| SDL2 for gamepad (static) | Cross-platform controller support, normalizes PS4+Xbox | ✓ Good |
| VST3 only (no AU/Standalone) | Windows target, reduce complexity for v1 | ✓ Good |
| CC74 cutoff + CC71 resonance | MIDI standard filter CCs | ✓ Good |
| Sample-and-hold pitch | Pitch only updates at trigger, not on joystick move | ✓ Good |
| Quantize ties → down (≤ midpoint) | Deterministic at scale note equidistance | ✓ Good |
| 4 voices on channels 1–4 default | Simple routing, user-configurable | ✓ Good |
| isMidiEffect()=true + disabled stereo buses | Ableton placement + pluginval both satisfied | ✓ Good |
| Lock-free LooperEngine (AbstractFifo) | Audio-thread safety, no std::mutex in processBlock | ✓ Good |
| SdlContext process-level singleton | One SDL_Init/SDL_Quit per process | ✓ Good |
| Static CRT (MultiThreaded) | No MSVC redistributable on clean machine | ✓ Good |
| Code signing deferred to v2 | SmartScreen acceptable for v1 | ✓ Acceptable |
| NEVER send CC121 in panic | Downstream VST3 instruments map CC121 to parameters | ✓ Good |
| Panic is one-shot not a mute toggle | 48 CCs, returns to pressable immediately | ✓ Good |
| snapToGrid ties → earlier grid point | CONTEXT.md decision; deterministic behavior | ✓ Good |
| pendingQuantize_ flag for post-record | Audio-thread safe, no mutex needed | ✓ Good |
| scratchDedup_ as class member | Avoids ~49KB stack allocation in applyQuantizeToStore() | ✓ Good |
| Single 30 Hz timer (no second timer) | Position bar + animations driven from existing timerCallback | ✓ Good |
| onConnectionChangeUI passes juce::String | Richer UI (controller type) without extra API call | ✓ Good |
| Border-only section panels | Fills covered drawAbove labels at same Y coordinate | ✓ Good |
| Editor height 790 → 810 | Quantize row (20px) was clipped at 790 | ✓ Good |

## Current Milestone: v1.6 Triplets & Fixes

**Goal:** Fix the noteCount stuck-notes bug, add triplet subdivisions, change default octave/scale values, fix Random Free to be truly tempo-free, and replace the looper bar with a rectangular perimeter visualization.

**Target features:**
- Default octave values: Third=4, Fifth=4, Tension=3 (UI display values); default scale = Natural Minor
- Bug fix: noteCount_ reference counting clamp removed — eliminates stuck notes in Single Channel mode
- Triplet subdivisions (1/1T–1/32T) added to random trigger and quantize selectors
- Random Free redesign: no RND SYNC = truly free timing; RND SYNC = internal clock; DAW Sync = DAW grid
- Looper visualization: rectangular perimeter fill bar replacing the linear strip

---
*Last updated: 2026-03-02 — v1.6 milestone started*
