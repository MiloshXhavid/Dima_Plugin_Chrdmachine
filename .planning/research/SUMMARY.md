# Project Research Summary

**Project:** ChordJoystick
**Domain:** MIDI Chord Generator Plugin (AUv3/VST3)
**Researched:** 2026-02-21
**Confidence:** HIGH

## Executive Summary

ChordJoystick is a MIDI chord generator plugin that uses joystick/XY input as its primary differentiator—users manipulate chord voicings in real-time by moving a virtual joystick, paired with touchplate triggers for performance. Built with JUCE 8.0.x (C++20, CMake), the plugin targets VST3 and AUv3 formats for cross-platform DAW integration.

**Recommended approach:** Start with the core plugin shell and parameter infrastructure, then build MIDI generation logic (scale quantization + chord synthesis) before tackling the GUI. This ensures the audio engine is solid before investing in UI complexity. The joystick control is the core differentiator—it maps XY position to chord intervals and voicings, constrained to musical scales.

**Key risks:**
1. **Real-time safety** — Memory allocation in `processBlock()` causes xruns; must pre-allocate all buffers
2. **Thread safety** — APVTS parameter access across UI/audio threads requires discipline
3. **Multi-instance** — Static memory shared between plugin instances causes state corruption

These are well-documented JUCE pitfalls with clear prevention strategies (see PITFALLS.md).

## Key Findings

### Recommended Stack

**Core technologies:**
- **JUCE 8.0.x** — Industry-standard C++ framework for AUv3/VST3; MIDI 2.0 preview, WebView UI support
- **C++20** — Required by JUCE 8 for modern Unicode and char8_t support
- **CMake 3.21+** — Official recommended approach, enables CI/CD, replaces legacy Projucer
- **VST3 + AUv3** — Target both Windows and macOS; add standalone for testing
- **GoogleTest** — Unit testing for core logic (scale calculations, MIDI message handling)
- **pluginval** — Official JUCE validation tool for CI; catch bugs before users do

**Why CMake over Projucer:** Cross-platform builds without platform-locked project files; GitHub Actions integration; community consensus moving away from Projucer.

### Expected Features

**Must have (table stakes):**
- Scale quantization (major, minor, pentatonic, modes) — users expect this or chords sound wrong
- 4-note chord generation (root, 3rd, 5th, tension)
- MIDI output (VST3/AUv3)
- DAW clock sync (slave mode) + standalone clock (master mode)
- Per-note octave adjustment

**Should have (competitive differentiators):**
- **Joystick/XY control** — Core differentiator; maps position to chord intervals in real-time
- **Touchplate triggers** — 4 buttons for finger performance
- Performance mode (rhythmic patterns from chords) — popularized by Scaler 2

**Defer (v2+):**
- Real-time looper with gesture recording — high value but complex; add post-MVP
- AI-assisted chord suggestions — emerging tech, not table stakes
- Hardware joystick integration — testing burden, focus on mouse/keyboard first
- Per-voice MIDI channel routing — advanced, not essential for launch

### Architecture Approach

The canonical JUCE pattern: **Processor-Editor separation with APVTS as the single source of truth.**

**Major components:**
1. **PluginProcessor** — Audio/MIDI processing, owns APVTS, lives on audio thread
2. **PluginEditor** — GUI rendering, subscribes to APVTS parameter changes via listeners
3. **ScaleEngine** — Quantizes notes to musical scales; independent of JUCE for testability
4. **MidiGenerator** — Creates 4-note chords from joystick position; reads from ScaleEngine
5. **TimingEngine** — DAW sync vs standalone clock, subdivision handling
6. **LooperEngine** — Record/playback gesture sequences (v2)

**Critical pattern:** All parameter state flows through APVTS. No direct processor→editor method calls. This ensures thread-safe automation and proper DAW integration.

### Critical Pitfalls

1. **Memory allocation in processBlock** — Pre-allocate buffers in `prepareToPlay()`. Never `new`, `malloc`, or grow `std::vector` in the audio callback. Causes xruns, clicks, crashes.

2. **Thread-unsafe parameter access** — `parameterChanged()` can fire from ANY thread. Use `getRawParameterValue()` for lock-free reads; only set trivial values in callbacks.

3. **MIDI buffer timestamp mishandling** — JUCE's `MidiBuffer` stores per-sample positions. Iterate with `for (auto metadata : buffer)` and use `metadata.samplePosition`, not just the message.

4. **Static memory/shared state** — Never use `static` or singletons for plugin state. DAWs load multiple instances; they'll share static memory and corrupt each other.

5. **Plugin state not restoring** — Implement `getStateInformation()` / `setStateInformation()` properly; use APVTS for automatic state management; test full DAW save/reload cycles.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Core Plugin Shell + Parameter Infrastructure
**Rationale:** Establish the build pipeline and JUCE foundation before writing any music logic. Must verify plugin loads in DAW before adding features.

**Delivers:**
- CMake-based project with VST3/AUv3/Standalone targets
- Minimal PluginProcessor + PluginEditor shell
- APVTS parameter system with all planned parameters defined
- Basic project structure (Plugin/, Editor/, Core/, Parameters/)

**Avoids:** Platform-specific format issues caught early; parameter system designed upfront rather than retrofitted

### Phase 2: MIDI Generation Engine
**Rationale:** The core value proposition—transforming joystick input into musical output. Must work before GUI development so the UI has something to control.

**Delivers:**
- ScaleEngine with lookup tables (major, minor, pentatonic, Dorian)
- MidiGenerator creating 4-note chords from interval inputs
- processBlock() wiring MIDI input → quantization → output
- DAW clock sync (slave) and standalone clock (master)

**Uses:** JUCE MidiBuffer, APVTS parameter reads in processBlock

### Phase 3: GUI Components
**Rationale:** Users need visual feedback and tactile control. Building on working MIDI engine ensures UI connects to real functionality.

**Delivers:**
- JoystickComponent (XY pad) with drag handling
- TouchplateComponent (4 triggers)
- Scale selector, octave controls, transpose controls
- Parameter bindings via SliderAttachment/ComboBoxAttachment

**Implements:** Editor hierarchy, custom LookAndFeel

### Phase 4: Looper + Polish (v1.x)
**Rationale:** High-differentiation feature but complex timing logic. Ship core first, add looper once chord generation is validated.

**Delivers:**
- LooperEngine with record/playback
- Loop subdivision selector
- Basic preset save/load

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 4 (Looper):** Complex timing logic, ring buffer design, subdivision handling—may need `/gsd-research-phase` for detailed implementation
- **Phase 2 (MIDI Engine):** MIDI clock state machine details—research host transport sync edge cases

Phases with standard patterns (skip research-phase):
- **Phase 1:** JUCE shell is well-documented, use JUCE tutorial code
- **Phase 3:** Standard JUCE UI components, no novel patterns needed

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | JUCE 8.0.12 official release, C++20 stable, CMake well-documented |
| Features | MEDIUM | Competitor analysis done, no direct user validation; differentiation space is defensible |
| Architecture | HIGH | Canonical JUCE patterns from official tutorials and melatonin.dev |
| Pitfalls | HIGH | Well-documented JUCE forum issues, clear prevention strategies |

**Overall confidence:** HIGH

### Gaps to Address

- **User validation:** Features are based on competitor analysis, not direct user interviews. Validate joystick + touchplate workflow before heavy UI investment.
- **Performance testing:**尚未 tested with high voice counts; may need optimization research if CPU usage is high
- **Cross-DAW compatibility:** VST3/AUv3 quirks differ per DAW—plan validation matrix early

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 Release — https://github.com/juce-framework/JUCE/releases
- JUCE Tutorial: Create a basic Audio/MIDI plugin — https://juce.com/tutorials/tutorial_code_basic_plugin/
- JUCE Tutorial: Adding plug-in parameters — https://docs.juce.com/master/tutorial_audio_parameter.html
- Melatonin: Big list of JUCE tips and tricks — https://melatonin.dev/blog/big-list-of-juce-tips-and-tricks/

### Secondary (MEDIUM confidence)
- Cross-Platform JUCE with CMake & GitHub Actions — https://reillyspitzfaden.com/posts/2025/08/plugins-for-everyone-crossplatform-juce-with-cmake-github-actions
- Plugin Boutique / Retailer product comparisons — competitor feature analysis

### Tertiary (LOW confidence)
- Community forum posts — edge cases, individual DAW quirks (need validation during testing)

---
*Research completed: 2026-02-21*
*Ready for roadmap: yes*
