# Feature Research

**Domain:** MIDI Chord Generator / Performance Controller VST Plugin
**Researched:** 2026-02-22
**Confidence:** MEDIUM (training data through Aug 2025; web search unavailable this session — verify competitor details before finalizing requirements)

---

> **Note on sources:** WebSearch and WebFetch were unavailable during this research session.
> All competitor analysis is drawn from training data (knowledge cutoff Aug 2025).
> Confidence ratings reflect this limitation. Recommend manual spot-check against
> current KVR Audio listings before locking requirements.

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete. In the paid plugin category, these are pre-purchase expectations; their absence triggers refund requests and negative reviews.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Scale quantization with presets | Every chord plugin since 2018 has this; users search "scale lock" | MEDIUM | ChordJoystick already has 20+ presets + custom piano buttons — this is satisfied |
| Real-time MIDI output to DAW | Core promise of any MIDI generator | LOW | JUCE handles transport; must work on Ableton and Reaper without extra routing steps |
| DAW preset save/recall | Paid plugins without preset persistence feel broken; users expect state to survive DAW session close | LOW | APVTS covers this — already in requirements |
| Stable operation (no crashes, no note sticking) | Musicians use plugins in live performance; hung notes or crashes are show-stoppers | HIGH | Note-off reliability is the hardest guarantee; test all gate/trigger paths exhaustively |
| Note-off guarantee on plugin close/bypass | Standard expectation — all notes must silence on bypass or DAW stop | MEDIUM | JUCE processBlock() must send note-off on transport stop and plugin close |
| Octave / transpose control | Every keyboard/MIDI tool has global transpose; users reach for this first | LOW | Already in requirements (global transpose −24 to +24, per-voice octave offset) |
| Configurable MIDI channel output | Routing to specific instruments via channel is workflow-critical for multi-timbral setups | LOW | Already in requirements (per-voice channel 1–16) |
| Visual feedback of current chord/pitch state | Users need to see what the plugin is doing — joystick position, active notes, current root | MEDIUM | XY display with position indicator is the primary UI; must show active pitches |
| Basic latency transparency | Plugin must not add perceptible latency to MIDI output | MEDIUM | JUCE MIDI output in processBlock() is sample-accurate; do not buffer unnecessarily |
| Windows DAW compatibility | Target DAWs (Ableton, Reaper) must load and pass MIDI without extra config | MEDIUM | JUCE VST3 is well-tested on both; SDL2 must not conflict with DAW audio threads |

### Differentiators (Competitive Advantage)

Features that set ChordJoystick apart from Scaler 2, Chord Prism, Ripchord, Chordjam, and Captain Chords. These are the reasons a musician picks this over free alternatives.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| XY joystick mapped to harmonic axes | No competitor maps X and Y independently to different chord voices; this is genuinely novel — X = Fifth+Tension, Y = Root+Third | HIGH | The core innovation; must feel fluid and expressive, not jerky or quantized-feeling in motion |
| Sample-and-hold pitch gate model | Joystick sets pitch continuously; pad press stamps the current pitch — pitch freezes on trigger. Most tools trigger immediately from quantized steps | HIGH | This allows "aim then shoot" musical gestures — a workflow no competitor offers |
| Per-voice trigger source selection | Root, Third, Fifth, Tension each independently triggerable by pad, joystick motion, or random | HIGH | Competitors either trigger all voices together or have fixed arpeggio patterns; this is additive not prescriptive |
| PS4/Xbox gamepad as first-class controller | No paid MIDI chord plugin has native gamepad support; controllers are $30–60 vs $200+ MIDI controllers | HIGH | Significant hardware cost reduction for target audience; must handle controller hot-plug gracefully |
| DAW-synced gesture looper | Records joystick position AND gate events as a beat-locked loop — playback is tempo-relative, not absolute time | HIGH | Competitors (Chord Prism, Scaler 2) have chord sequence patterns but not joystick gesture capture |
| Gamepad left-stick as filter CC | Sending CC71/CC74 from left stick while right stick controls harmony turns the controller into a two-handed expressive instrument | MEDIUM | No competitor does this — it's a composited instrument experience rather than a utility plugin |
| Random gate with subdivisions + density | Per-voice randomized triggering at 1/4–1/32 with density control; not just a global random mode | MEDIUM | Most tools with random mode apply it globally; per-voice random enables polyrhythmic generative textures |
| 12-key custom scale entry via piano UI | Users can draw any scale they want, not limited to presets | LOW | Standard in theory but often omitted or hidden in competitors; make it prominent |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem attractive but create scope, maintenance, or conceptual debt in v1.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Built-in audio synthesis / sound engine | "Play it without a synth" requests; feels more complete | Doubles scope; audio engine = separate domain (voice allocation, polyphony, CPU); changes plugin category from utility to instrument; breaks the MIDI-only value proposition | Document clearly that ChordJoystick drives external synths; ship with a short demo video showing Ableton + synth track setup |
| MIDI input processing (incoming chord detection) | Users want to play a chord in and have it quantize | Inverse of the product — ChordJoystick generates, not analyzes; adding MIDI input requires note detection, voice attribution, chord analysis engine | Explicitly out of scope v1; if requested, log as future feature |
| Standalone app mode | "Use without a DAW" | Standalone requires audio subsystem, UI windowing lifecycle, different distribution; adds weeks of work; Windows standalone has ASIO/WASAPI complexity | JUCE Standalone wrapper can be added later with minimal code; defer to v1.5 |
| Built-in arpeggiator patterns | Users expect arpeggios in any chord tool | ChordJoystick's per-voice trigger model IS a more flexible arpeggiator; a separate arpeggiator conflicts with the trigger source model conceptually and creates UI bloat | Educate in docs: random + joystick motion trigger = generative arpeggio; TouchPlate sequencing = manual arpeggio |
| macOS / AU support in v1 | Mac users will request it | Requires Mac build machine, AU wrapper, notarization, code signing with Apple developer account ($99/yr); SDL2 gamepad tested less thoroughly on macOS | Windows ships first; add macOS in v2 with a Mac dev environment ready |
| Full preset browser UI | Professional plugins have preset browsers with categories, tags, search | A proper preset browser is a significant UI subsystem (list box, filtering, file I/O, import/export); APVTS + DAW preset management covers 80% of the need | Use DAW preset management for v1; a custom browser is a v2 feature |
| MPE (MIDI Polyphonic Expression) | Expressive MIDI users ask for MPE support | MPE requires per-note pitch bend and channel management that conflicts with the fixed 4-voice channel model; MIDI 2.0/MPE is still a niche DAW feature | Standard MIDI channels are universal; MPE is v2+ when it has broader DAW support |
| MIDI input learn / CC mapping | Power users want to remap controls | CC learn adds a state machine (listening mode, conflict detection, persistent mapping store) that significantly complicates the parameter model | APVTS host automation covers most DAW CC-to-parameter needs; defer to v2 |
| Chord library / chord name display | Display the chord name (Cmaj7, Dm9) currently being played | Chord identification from 4 arbitrary pitches requires a chord analysis engine; edge cases multiply rapidly (polychords, inversions, non-standard voicings) | Show the raw pitch values (MIDI note numbers or note names); label the voices (Root / Third / Fifth / Tension) |

## Feature Dependencies

```
Scale Quantization
    └──required by──> XY Joystick Pitch Mapping
                          └──required by──> Sample-and-Hold Gate Model
                                                └──required by──> Looper (records quantized positions)

Per-Voice Trigger Sources
    └──required by──> TouchPlate UI
    └──required by──> Joystick Motion Trigger
    └──required by──> Random Gate (subdivisions + density)

Gamepad Support (SDL2)
    └──enables──> Gamepad as XY Controller
    └──enables──> Gamepad Trigger Buttons (R1/R2/L1/L2)
    └──enables──> Filter CC Left Stick
    └──required by──> Gamepad Looper Controls (Cross/Square/Triangle/Circle)

Looper
    └──requires──> DAW Transport Sync (JUCE AudioPlayHead)
    └──requires──> Per-Voice Trigger Sources (to record gate events)
    └──requires──> XY Joystick Pitch Mapping (to record positions)

MIDI Channel Routing
    └──required by──> Per-Voice MIDI Output
    └──required by──> Filter CC Channel Output

APVTS Parameter State
    └──required by──> All knobs, buttons, selectors (persistence)
    └──required by──> DAW Preset Save/Recall
```

### Dependency Notes

- **Scale Quantization requires XY Joystick:** The joystick output is raw; quantization is the transform that makes pitches musical. Without scale quantization the joystick is just a pitch bend controller, not a chord generator.
- **Sample-and-Hold requires Quantization:** The held pitch must be a quantized note; holding a raw joystick float would produce non-musical results.
- **Looper requires DAW Transport Sync:** Beat-locked recording requires `AudioPlayHead::getCurrentPosition()` to stamp events with beat position, not wall-clock time.
- **Gamepad conflicts with Mouse-only joystick:** Both drive the same XY parameter; must use last-input-wins priority — gamepad right stick overrides mouse drag when gamepad is connected, mouse resumes when gamepad disconnects. This is a subtle state management problem.
- **Random Gate conflicts with Joystick Motion Trigger (per voice):** For a given voice, only one trigger source is active at a time; the per-voice selector is the resolution mechanism — it's a configuration choice, not a runtime conflict.

## MVP Definition

### Launch With (v1)

These are the features that make ChordJoystick a complete, shippable, paid plugin.

- [x] XY joystick with mouse control (Y = Root+Third pitch, X = Fifth+Tension pitch) — the core innovation
- [x] Scale quantization: 20+ presets + 12-key custom entry, nearest-note algorithm, ties go down — without this the joystick is noise
- [x] 4-voice TouchPlate triggers (sample-and-hold, note-on/note-off) — the performance gesture system
- [x] Per-voice trigger source: TouchPlate / Joystick Motion / Random — makes the system compositionally flexible
- [x] Random gate with subdivisions (1/4–1/32) and density knob — adds generative value without MIDI input
- [x] Global transpose + per-voice octave offset + interval knobs — registers and voicing control
- [x] Per-voice MIDI channel routing (1–16) — multi-timbral setups are common; without this, routing to multiple instruments requires workarounds
- [x] DAW-synced looper (record/play/stop/reset, 1–16 bars, standard time signatures) — the feature that elevates this from performance tool to composition tool
- [x] PS4/Xbox gamepad support via SDL2 with hot-plug — the hardware differentiator
- [x] Filter CC output (CC74/CC71) from gamepad left stick — completes the two-handed instrument concept
- [x] APVTS state persistence (saves with DAW session and plugin presets) — non-negotiable for paid plugin quality
- [x] Note-off guarantee on transport stop, plugin bypass, and DAW close

### Add After Validation (v1.x)

Add these once v1 ships and user feedback confirms demand:

- [ ] Standalone app mode — when enough users ask for "use without Ableton"; JUCE makes this low-effort once VST3 is stable
- [ ] macOS build + AU format — when a Mac build environment is set up
- [ ] MIDI CC input learn — when power users request custom controller mapping
- [ ] Velocity control per voice or per trigger — when users want dynamics beyond note-on velocity 127

### Future Consideration (v2+)

Defer until product-market fit is established and revenue justifies the scope:

- [ ] Built-in preset browser UI — needs significant UI subsystem work; DAW presets cover v1
- [ ] MPE output — needs DAW ecosystem to mature further; niche market now
- [ ] MIDI chord analysis / input processing — inverse product direction; validate separately
- [ ] Additional DAW sync modes (MIDI clock slave, Ableton Link) — for users who don't use a DAW host
- [ ] Pattern sequencer for joystick positions — automating the XY path over time

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| XY joystick pitch mapping | HIGH | HIGH | P1 |
| Scale quantization (20+ presets + custom) | HIGH | MEDIUM | P1 |
| TouchPlate sample-and-hold gate | HIGH | MEDIUM | P1 |
| Per-voice trigger source | HIGH | MEDIUM | P1 |
| APVTS state persistence | HIGH | LOW | P1 |
| Note-off guarantee (all paths) | HIGH | MEDIUM | P1 |
| Global transpose + octave offsets | HIGH | LOW | P1 |
| DAW-synced looper | HIGH | HIGH | P1 |
| Per-voice MIDI channel routing | MEDIUM | LOW | P1 |
| PS4/Xbox gamepad support | HIGH | HIGH | P1 |
| Random gate (subdivisions + density) | MEDIUM | MEDIUM | P1 |
| Filter CC (CC71/CC74) from gamepad | MEDIUM | LOW | P1 |
| Visual XY position indicator | HIGH | LOW | P1 |
| Active note display (4 voice pitches) | MEDIUM | LOW | P2 |
| Standalone app mode | MEDIUM | MEDIUM | P3 |
| Built-in preset browser | LOW | HIGH | P3 |
| MIDI CC input learn | MEDIUM | HIGH | P3 |
| Chord name display | LOW | HIGH | P3 |
| macOS / AU support | HIGH | HIGH | P3 |

**Priority key:**
- P1: Must have for launch (all already in v1 requirements)
- P2: Should have, add when possible
- P3: Nice to have, future consideration

## Competitor Feature Analysis

> Confidence: LOW-MEDIUM — Training data through Aug 2025. Verify against current product pages before finalizing.

| Feature | Scaler 2 (Plugin Boutique) | Chord Prism (Audiomodern) | Ripchord (Trackbout) | ChordJoystick Approach |
|---------|---------------------------|--------------------------|---------------------|------------------------|
| Scale quantization | YES — 40+ scales, key detection | YES — scale lock | YES — scale snap | YES — 20+ presets + custom 12-key |
| Chord voicing | YES — inversion, spread, voice leading | YES — arpeggio mode | YES — 4-voice spread | YES — XY joystick controls interval positions |
| Real-time performance UI | Partial — chord pads, not continuous | YES — XY pad | NO | YES — joystick is primary; continuous not step |
| Arpeggiator | YES — multiple patterns | YES | NO | Per-voice trigger model supersedes this |
| MIDI channel routing | Limited — single output channel | Limited | NO | YES — per-voice 1–16 |
| Looper / recording | NO — chord progression recorder only | NO | NO | YES — beat-locked joystick + gate looper |
| Gamepad support | NO | NO | NO | YES — PS4/Xbox via SDL2 |
| State persistence | YES | YES | YES | YES (APVTS) |
| Preset browser | YES — extensive | Partial | NO | NO (v1 relies on DAW presets) |
| Sample-and-hold gate | NO | NO | NO | YES — core innovation |
| Per-voice trigger source | NO | NO | NO | YES — TouchPlate / Motion / Random per voice |
| Filter CC output | NO | NO | NO | YES — CC71/CC74 from gamepad left stick |
| Pricing (approx.) | ~$49–59 | ~$30–40 | FREE | Paid (TBD) |
| Platform | Win + Mac + Linux | Win + Mac | Win + Mac | Windows (v1) |

### Key Competitive Observations

1. **Scaler 2** is the market leader in paid chord generators. Its strength is its chord library and music theory depth. Its weakness is that it's a step-sequencer-oriented tool — chords are selected, not continuously navigated. ChordJoystick is more like an instrument than a utility.

2. **Chord Prism** is the closest competitor in the "expressive XY" space. It has an XY pad and scale lock. Key differentiators vs ChordJoystick: Chord Prism applies chords as polyphonic MIDI chords to X/Y — it maps chord type/root across axes, not individual voice intervals. It has no gamepad support, no looper, and no sample-and-hold gate model.

3. **Ripchord** is free and simple — drag a MIDI note in to trigger a chord. No real-time joystick, no looper, no gamepad. Not a direct competitor but sets user expectations for "free alternative exists."

4. **Chordjam** (Audiomodern) focuses on generative chord progressions with randomized triggers. No XY joystick, no per-voice gate control.

5. **The gap ChordJoystick fills:** Continuous harmonic navigation (not step-selection) + per-voice sample-and-hold gates + gamepad as first-class controller + beat-locked joystick looper. No competitor offers this combination. The closest analog is a hardware instrument (Roli Seaboard, Haken Continuum) that maps physical XY to harmonic space, but those are $500–$4000+ hardware devices. ChordJoystick is a software equivalent at a fraction of the cost.

## Feature Complexity Reference (for Phase Planning)

| Feature | Complexity Driver | Build Phase Estimate |
|---------|------------------|---------------------|
| XY joystick UI + MIDI output | Smooth mouse tracking, JUCE Component custom drawing | Phase 2–3 |
| Scale quantization engine | Nearest-note search over 2 octaves, tie-breaking logic | Phase 2 |
| Sample-and-hold gate + note-on/off | State machine per voice (armed → triggered → releasing) | Phase 3 |
| Per-voice trigger source routing | State selector driving 3 trigger backends | Phase 3 |
| Random gate (synced) | JUCE Timer or processBlock() beat-sync subdivision counter | Phase 4 |
| Joystick motion trigger | Threshold delta detection on XY changes | Phase 3 |
| SDL2 gamepad integration | Hot-plug detection, axis normalization, button mapping | Phase 5 |
| DAW-synced looper | AudioPlayHead beat position, event queue, playback interpolation | Phase 6 |
| Per-voice MIDI channel routing | APVTS int parameter → juce::MidiMessage channel field | Phase 2 |
| UI layout (TouchPlates, scale buttons, knobs) | JUCE Component layout, custom LookAndFeel | Phase 2–3 |
| Note-off guarantee (all paths) | processBlock() state audit on transport stop + bypass | Phase 3 |
| APVTS persistence | JUCE AudioProcessorValueTreeState — covered by framework | Phase 2 |

## Sources

- Training data: KVR Audio plugin database coverage (through Aug 2025) — LOW-MEDIUM confidence
- Training data: Scaler 2 feature documentation (Plugin Boutique) — MEDIUM confidence
- Training data: Chord Prism feature documentation (Audiomodern) — MEDIUM confidence
- Training data: Ripchord product page (Trackbout) — MEDIUM confidence
- Training data: Chordjam feature documentation (Audiomodern) — MEDIUM confidence
- Training data: JUCE MIDI plugin patterns and APVTS documentation — HIGH confidence (framework behavior is stable)
- Project context: `.planning/PROJECT.md` — HIGH confidence (authoritative for ChordJoystick requirements)
- Verify against: https://www.kvraudio.com/plugins/chord-generators/ (current listings)
- Verify against: https://www.plugin-alliance.com / Plugin Boutique / current Scaler 2 feature page

---
*Feature research for: JUCE VST3 MIDI Chord Generator / Performance Controller*
*Researched: 2026-02-22*
