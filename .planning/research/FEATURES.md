# Feature Research

**Domain:** MIDI Chord Generator Plugins (Performance/Live Controllers)
**Researched:** 2026-02-21
**Confidence:** MEDIUM

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Scale quantization | All chord generators must constrain output to musical scales; otherwise sounds dissonant/wrong | MEDIUM | Must support common scales (major, minor, pentatonic, modes) |
| Chord trigger via single note/button | Core value prop: play chords with one finger/key | LOW | Basic MIDI note-in → chord-out mapping |
| Multiple note voicings (3-4 notes per chord) | Standard chord thickness expected | LOW | Most generators output triads or 4-note chords |
| MIDI output (VST3/AUv3) | Required for integration with DAWs and synths | MEDIUM | Platform-specific format requirements |
| Preset scale selection | Users need quick access to common scales | LOW | Dropdown or button matrix |
| Basic interval control | Users expect ability to customize chord tones (root, 3rd, 5th) | LOW | Knobs or dropdowns for interval selection |
| MIDI learn / parameter mapping | Essential for hardware controller integration | MEDIUM | Industry standard for performance plugins |
| DAW clock sync | Critical for timing accuracy in compositions | MEDIUM | MIDI clock or Ableton Link |
| Velocity sensitivity | Dynamic expression expected from any MIDI controller | LOW | Note-on velocity handling |
| Standalone operation | Must work without DAW for hardware setups | LOW | Desktop mode for live performance |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Joystick/XY control for pitch generation** | Tactile, real-time chord manipulation during live performance - THIS IS THE CORE DIFFERENTIATOR | HIGH | Joystick position maps to chord intervals/voicings |
| **Real-time looper with gesture recording** | Record and loop performance gestures synced to tempo; unique for chord generators | HIGH | Loop points, subdivision, overdub |
| **Touchplate/trigger input mapping** | Physical touchplates for finger drumming chord triggering; tactile performance | MEDIUM | Hardware integration or on-screen buttons |
| **Performance mode** | Transform static chords into rhythmic patterns; popularized by Scaler 2 | MEDIUM | Strumming, arpeggio patterns from chords |
| **Per-voice octave adjustment** | Individual control over each chord tone's octave for voicing | LOW | 4 knobs for 4 notes |
| **Randomized trigger with subdivision** | Generative-style chord changes on clock subdivisions | MEDIUM | Adds variation without manual input |
| **Per-voice MIDI channel routing** | Route each chord voice to different instruments/channels | LOW | Advanced but valuable for sound design |
| **Preset library with genre styles** | Ready-to-use chord progressions; saves setup time | MEDIUM | Import/export capability |
| **AI-assisted chord suggestions** | Intelligent suggestions based on context; emerging differentiator | HIGH | Used by newer tools like Melody Sauce |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Audio synthesis | Users want "all-in-one" tool | Adds massive complexity, dilutes core value | Keep MIDI-only, let users pair with their synths |
| Hardware joystick integration | "Make it work with my controller" | Requires specialized driver support, testing burden | Focus on mouse/keyboard first; add later if demand |
| Full preset save/load system | "I want to save my setups" | Complex UI, serialization logic for v1 | Basic preset storage, full system post-MVP |
| Real-time audio analysis (key detection) | "Analyze my audio track" | Requires audio input path; adds latency concerns | Input-only mode, analyze incoming MIDI instead |
| Cloud preset sharing | "Share with community" | Server infrastructure, moderation, version management | Local file export/import for v1 |
| Unlimited polyphony | "More notes is better" | CPU/memory, typically unnecessary for chords (4-8 notes max) | Cap at practical limit (8-16 notes) |

## Feature Dependencies

```
[Scale Quantization]
    └──requires──> [Preset Scale Selection]
                       └──requires──> [MIDI Output]

[Joystick Pitch Control]
    └──requires──> [Scale Quantization]
    └──requires──> [Interval Control]

[Looper with DAW Sync]
    └──requires──> [DAW Clock Sync]
    └──requires──> [Subdivision Selection]

[Per-voice Octave Adjustment]
    └──requires──> [Multiple Note Voicings]

[Touchplate Triggers]
    └──requires──> [Chord Trigger via Input]
```

### Dependency Notes

- **Joystick control requires scale quantization:** Without quantization, XY movement produces random/dissonant notes
- **Looper requires clock sync:** Loop must align to musical time, either master (standalone) or slave (DAW-synced)
- **Per-voice controls require multiple voices:** Single-note plugins don't benefit from per-voice routing

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept.

- [x] Scale quantization (preset scales: Major, Minor, Pentatonic, Dorian)
- [x] 4-note chord generation (root, 3rd, 5th, optional tension)
- [x] Interval control (which intervals constitute the chord)
- [x] XY joystick control for chord manipulation
- [x] Touchplate/button triggers (4) for note triggering
- [x] Latching behavior (notes sustain after trigger release)
- [x] MIDI output (VST3/AUv3)
- [x] DAW clock sync (slave mode)
- [x] Standalone clock (master mode)
- [x] Per-note octave adjustment
- [x] Global transpose

### Add After Validation (v1.x)

Features to add once core is working.

- [ ] Looper with gesture recording (high value differentiator)
- [ ] Loop subdivision selector (3/4, 4/4, 5/4, etc.)
- [ ] Loop length selector (1-16 bars)
- [ ] Trigger source selection (touchplate vs joystick movement)
- [ ] Random trigger with subdivision clock
- [ ] Basic preset save/load

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] Performance mode (rhythmic patterns from chords)
- [ ] Per-voice MIDI channel routing
- [ ] Preset library with genre styles
- [ ] Hardware joystick support
- [ ] AI chord suggestions

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Scale quantization | HIGH | MEDIUM | P1 |
| 4-note chord output | HIGH | LOW | P1 |
| XY joystick control | HIGH | HIGH | P1 |
| Touchplate triggers | HIGH | MEDIUM | P1 |
| MIDI output (VST3/AUv3) | HIGH | MEDIUM | P1 |
| DAW/standalone clock sync | HIGH | MEDIUM | P1 |
| Per-note octave control | MEDIUM | LOW | P1 |
| Looper with sync | HIGH | HIGH | P2 |
| Interval control (tension notes) | MEDIUM | LOW | P1 |
| Global transpose | MEDIUM | LOW | P1 |
| Subdivision selection | MEDIUM | LOW | P2 |
| Loop length control | MEDIUM | MEDIUM | P2 |
| Random trigger | MEDIUM | MEDIUM | P2 |
| Basic preset system | MEDIUM | MEDIUM | P2 |
| Per-voice MIDI routing | LOW | LOW | P3 |
| Performance mode | MEDIUM | HIGH | P3 |

**Priority key:**
- P1: Must have for launch
- P2: Should have, add when possible
- P3: Nice to have, future consideration

## Competitor Feature Analysis

| Feature | Scaler 2 ($59) | Captain Chords ($99) | Cthulhu ($39) | Chordz (Free) | ChordJoystick (planned) |
|---------|----------------|----------------------|---------------|---------------|-------------------------|
| Scale quantization | YES | YES | YES | YES | YES |
| Chord triggering | Keyboard input | Keyboard input | Keyboard input | Keyboard input | **Joystick + touchplates** |
| Real-time looper | NO | NO | NO | NO | **YES** |
| Performance patterns | YES | YES | YES | NO | NO (future) |
| Preset library | Extensive | Extensive | Limited | Minimal | Minimal (v1) |
| MIDI routing | Basic | Basic | Advanced | Basic | **Per-voice** |
| Expression control | Velocity only | Velocity only | XY pad | None | **Full XY joystick** |

**Key insight:** No competitor combines real-time joystick control + looper + touchplate triggers. This is the differentiation space ChordJoystick occupies.

## Sources

- Plugin Beats: "MIDI Plugins Chord Generator 2025" (2025-11-06)
- Plugin Boutique: Scaler 2 product documentation
- Mixed In Key: Captain Chords product info
- Xfer Records: Cthulhu product info
- CodeFN42: Chordz (free VST)
- FeelYourSound: ChordPotion, ChordConverter
- AudioDeluxe: Chord Prism 2
- UndoLooper, MSuperLooper: Looper feature research
- Mario Nieto: Chord Generator VST (iOS/Android AUv3)
- Retailer/product feature comparisons

---
*Feature research for: MIDI Chord Generator Plugins*
*Researched: 2026-02-21*
