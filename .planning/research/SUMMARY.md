# Project Research Summary

**Project:** ChordJoystick
**Domain:** JUCE VST3 MIDI-only chord generator / performance controller plugin with SDL2 gamepad support (Windows)
**Researched:** 2026-02-22
**Confidence:** MEDIUM (direct codebase inspection + verified JUCE/SDL2/VST3 training knowledge; web search unavailable this session)

## Executive Summary

ChordJoystick is a MIDI-only VST3 plugin built on JUCE 8 that maps an XY joystick (mouse or gamepad) to harmonic space, allowing musicians to continuously navigate chord voicings rather than step-select them. The plugin generates 4-voice MIDI with per-voice trigger sources (TouchPlate, joystick motion, or random subdivisions), a DAW-synced beat looper, and native PS4/Xbox controller support via SDL2. The codebase already has substantial scaffolding in place: APVTS parameter layout, ChordEngine, ScaleQuantizer, TriggerSystem, LooperEngine, and GamepadInput are all present. Research confirms the architecture and stack choices are sound for a paid Windows VST3 release, but four critical bugs and several major distribution issues exist that must be resolved before any public release.

The recommended build path follows the component dependency graph: stabilize the pure-logic engine layer first (ScaleQuantizer, ChordEngine), then wire up MIDI output in a DAW-testable state, then harden the real-time threading (replace the LooperEngine mutex), then integrate SDL2 safely (SDL singleton, SDL_HINT_JOYSTICK_THREAD), then address Ableton routing, and finally prepare the distribution chain (static CRT, code signing, installer). No external dependencies need to change — the stack of JUCE 8, SDL2 2.30.9 static, MSVC 2022, and CMake 3.22+ is correct and proven. The single most urgent action is pinning JUCE to a specific release tag in CMakeLists.txt before any further commits.

The competitive landscape strongly supports ChordJoystick's differentiators: no competitor (Scaler 2, Chord Prism, Ripchord) offers continuous harmonic navigation via joystick, per-voice sample-and-hold gates, gamepad as a first-class controller, or a beat-locked joystick looper. These features are genuinely novel in the paid plugin market. The risk profile is dominated by real-time threading constraints (mutex in audio thread, unconditional CC flood) and Windows distribution mechanics (code signing, MSVC CRT, DAW routing compatibility) rather than any product-direction uncertainty.

## Key Findings

### Recommended Stack

The existing stack is correct. JUCE 8 via CMake FetchContent, SDL2 2.30.9 static, C++17, and MSVC 2022 on Windows 11 are the right choices for a paid VST3 plugin targeting Ableton and Reaper. The only immediate change required is replacing `GIT_TAG origin/master` with a pinned release tag (e.g., `8.0.4`) in the JUCE FetchContent block — the current unpinned state produces non-reproducible builds. SDL2 is already correctly pinned and statically linked. All subsystems not needed for gamepad operation (SDL_AUDIO, SDL_VIDEO, SDL_HAPTIC) are already disabled, which is the correct approach for a plugin embedded in a DAW.

**Core technologies:**
- **JUCE 8.x (pin to 8.0.4):** Plugin framework, APVTS, MIDI, GUI, Timer — industry standard; provides everything needed for VST3 DAW integration
- **SDL2 2.30.9 (static):** Gamepad/controller HID — SDL2 (not SDL3) is the stable choice; static linking avoids DLL dependency in the plugin bundle
- **CMake 3.22+:** Build system — required minimum for JUCE 8 CMake API; `juce_add_plugin` is the correct entry point
- **MSVC 2022 / C++17:** Compiler — VST3 on Windows requires MSVC for reliable DAW compatibility; MinGW causes exception-handling issues in some hosts
- **Catch2 3.x:** Unit testing for pure-logic classes (ChordEngine, ScaleQuantizer) — testable without a DAW host

### Expected Features

ChordJoystick's entire v1 feature set is already defined in requirements and is confirmed as the correct scope. No competitor offers this combination. All P1 features must ship together — the product is only coherent when the full interaction model (joystick + gates + gamepad + looper) is present.

**Must have (table stakes — users will refund without these):**
- Scale quantization with 20+ presets + custom 12-key piano entry
- Real-time MIDI output with note-off guarantee on transport stop and plugin close
- APVTS state persistence (saves with DAW session and presets)
- Visual XY position indicator and active note display
- Octave / transpose control and per-voice MIDI channel routing

**Should have (core differentiators — the reason to buy this over free alternatives):**
- XY joystick mapped to harmonic axes (Y = Root+Third, X = Fifth+Tension) — the central innovation
- Sample-and-hold pitch gate model ("aim then shoot" — no competitor has this)
- Per-voice trigger source: TouchPlate / Joystick Motion / Random subdivisions
- PS4/Xbox gamepad as first-class controller with hot-plug via SDL2
- DAW-synced beat looper recording joystick position and gate events
- Filter CC (CC71/CC74) from gamepad left stick — two-handed instrument experience

**Defer to v2+:**
- macOS / AU support — requires Apple developer account, notarization, Mac build environment
- Standalone app mode — JUCE makes this low-effort but adds ASIO/WASAPI complexity
- Preset browser UI — DAW presets cover v1 needs adequately
- MPE output — DAW ecosystem niche; conflicts with 4-voice channel model
- MIDI CC input learn — adds significant state machine complexity

### Architecture Approach

The architecture follows the standard JUCE plugin threading model: UI and gamepad threads communicate with the audio thread exclusively through `std::atomic<T>` members and edge-flag consume patterns (`atomic::exchange(false)`). APVTS parameter reads on the audio thread use `getRawParameterValue()` which returns a stable `std::atomic<float>*`. The component hierarchy is clean and flat: ScaleQuantizer and ChordEngine are stateless pure functions, TriggerSystem and LooperEngine are stateful with atomic boundaries, and GamepadInput runs SDL2 poll at 60Hz on JUCE's message thread. The data flow (mouse/gamepad → atomics → processBlock → ChordEngine → ScaleQuantizer → MidiBuffer → DAW) is architecturally sound. One critical violation exists: LooperEngine acquires a `std::mutex` inside `processBlock()`, which must be replaced before any production release.

**Major components:**
1. **PluginProcessor** — Orchestration hub; owns APVTS, calls all subsystems in processBlock, bridges all thread boundaries
2. **ChordEngine + ScaleQuantizer** — Pure stateless pitch computation: joystick XY + intervals + transpose → quantized MIDI note; testable without DAW
3. **TriggerSystem** — 4-voice gate state machine; handles TouchPlate / Joystick / Random trigger sources; fires note-on/off callbacks at sample offsets
4. **LooperEngine** — Beat-timestamped event recorder and playback engine; DAW-synced via AudioPlayHead ppqPosition; currently has mutex violation
5. **GamepadInput** — SDL2 60Hz Timer poll on message thread; normalizes axes; edge-detects buttons; writes to atomics consumed in processBlock
6. **PluginEditor** — All JUCE Component rendering; APVTS attachments; 30Hz Timer repaint for gate LEDs and looper beat display

### Critical Pitfalls

1. **std::mutex in processBlock via LooperEngine** — Blocks the audio thread when UI calls reset/deleteLoop simultaneously; causes xruns. Fix: replace event storage with lock-free AbstractFifo + fixed-size array; use atomic flags for destructive operations. Address in the looper implementation phase before any DAW testing.

2. **SDL_Init / SDL_Quit per plugin instance** — Multiple plugin instances in the same DAW process race on SDL's global state during initialization; one instance's destructor calls SDL_Quit and silently breaks other instances. Fix: process-level reference-counted singleton (10 lines). Address in the initial SDL2 integration phase.

3. **Filter CC flooding when no gamepad is connected** — CC74 and CC71 are emitted every processBlock unconditionally, flooding the downstream synth at the audio callback rate (~175 msgs/sec). Fix: gate on `gamepad_.isConnected()` and emit only on value change. Address at first smoke test — this makes the plugin unusable without a gamepad.

4. **Note-off not sent on releaseResources()** — `releaseResources()` body is empty; held voices stick on the downstream synth when the DAW stops or the plugin is bypassed. Fix: iterate open gates and emit noteOff + allNotesOff in releaseResources(). Address in the core MIDI output phase.

5. **JUCE fetched from origin/master** — Non-reproducible builds; JUCE master can break plugin API mid-development. Fix: pin to a specific release tag (`GIT_TAG 8.0.4`) in CMakeLists.txt. Address immediately — one-line change.

## Implications for Roadmap

Based on combined research, the component dependency graph, and the pitfall-to-phase mapping, seven phases are recommended. The ordering follows strict dependency requirements: pure logic before audio wiring, real-time safety before DAW testing, single-host before multi-host, and functionality before distribution.

### Phase 1: Build Foundation and JUCE Version Lock
**Rationale:** The CMakeLists.txt has a critical `GIT_TAG origin/master` issue that produces non-reproducible builds. This must be fixed before any further development. This phase also establishes the VST3 plugin loading baseline and validates the APVTS parameter round-trip — nothing else can be verified until the plugin loads cleanly in a DAW.
**Delivers:** Reproducible build with pinned JUCE tag; plugin loads in Reaper without crashes; all 40+ APVTS parameters save/restore correctly; static CRT configured for distribution readiness.
**Addresses:** Scale quantization APVTS wiring, global transpose, per-voice octave offsets, MIDI channel routing (all low-complexity P1 features that require no additional logic — just wiring)
**Avoids:** JUCE unpinned version (Pitfall 5), MSVC CRT dependency (Pitfall 11)
**Research flag:** Standard patterns — skip research-phase. CMakeLists changes are mechanical; JUCE CMake API is well-documented.

### Phase 2: Core Engine Validation (ChordEngine + ScaleQuantizer)
**Rationale:** ChordEngine and ScaleQuantizer are pure functions with no DAW dependency — they can be verified with Catch2 unit tests before any audio plumbing. Building confidence in the pitch computation layer before wiring it into processBlock prevents subtle pitch bugs from being obscured by threading issues later.
**Delivers:** Catch2 test suite covering scale quantization edge cases (single note, all 12 notes, tie-breaking), ChordEngine pitch computation across all 4 voices, and custom 12-key scale entry logic. All tests passing.
**Addresses:** Scale quantization (20+ presets + custom entry), XY joystick pitch math, interval/octave offset correctness
**Avoids:** Discovering pitch bugs late when they're entangled with MIDI output and threading
**Research flag:** Standard patterns — skip research-phase. Pure C++ logic with known algorithms; well-covered by unit tests.

### Phase 3: Core MIDI Output and Note-Off Guarantee
**Rationale:** TriggerSystem's gate state machine is the heart of the performance interaction. Wiring it to produce actual MIDI note-on/off events in processBlock, and guaranteeing note-off on all exit paths (releaseResources, bypass, DAW stop), is the minimum viable product state for any meaningful DAW testing. The note-off guarantee is a table-stakes feature whose absence produces the most viscerally broken user experience.
**Delivers:** All 4 voices producing sample-accurate MIDI note-on/off via TouchPlate triggers; note-off flushed on transport stop and plugin bypass; MIDI channel routing verified per voice; visual gate LED feedback in UI.
**Addresses:** TouchPlate sample-and-hold gate model, note-off guarantee (all paths), per-voice MIDI channel routing, visual XY position indicator
**Avoids:** Note-off leak (Pitfall 4), MIDI channel off-by-one (Architecture anti-pattern 4)
**Research flag:** Standard patterns — skip research-phase. JUCE processBlock and MidiBuffer patterns are well-established.

### Phase 4: Per-Voice Trigger Sources and Random Gate
**Rationale:** Once basic note-on/off works via TouchPlate, adding the two additional trigger sources (joystick motion threshold and random subdivisions) completes the core interaction model. Random gate with DAW-synced subdivisions requires `AudioPlayHead` beat position — establishing this integration here also lays groundwork for the looper.
**Delivers:** Per-voice trigger source selector (TouchPlate / Joystick Motion / Random) fully functional; random gate with 1/4–1/32 subdivision and density control, DAW-synced; joystick motion trigger with configurable threshold.
**Addresses:** Per-voice trigger source routing, random gate (subdivisions + density), joystick motion trigger
**Avoids:** Trigger source conflict when multiple sources enabled for same voice (Architecture — per-voice selector is the resolution)
**Research flag:** Standard patterns — skip research-phase. Subdivision-synced timing is a known pattern using ppqPosition modulo.

### Phase 5: LooperEngine Hardening and DAW Sync
**Rationale:** The LooperEngine as coded has two critical issues that must be resolved before use: the mutex in the audio thread (Pitfall 1) and the ppqPosition anchor drift (Pitfall 12), plus the vector reallocation risk (Pitfall 7). This phase refactors LooperEngine to be real-time safe, then validates beat-locked recording and playback against Ableton and Reaper. This is the highest-complexity phase.
**Delivers:** Lock-free LooperEngine using AbstractFifo + fixed-size event array; atomic flags for UI-side destructive operations (reset, delete); ppqPosition anchor recorded at loop start; hard capacity cap preventing audio-thread allocation; verified 1–16 bar loop record/play/stop in both Ableton and Reaper.
**Addresses:** DAW-synced looper (record/play/stop/reset, 1–16 bars), looper transport controls
**Avoids:** Mutex on audio thread (Pitfall 1), vector reallocation on audio thread (Pitfall 7), ppqPosition anchor drift (Pitfall 12)
**Research flag:** Needs research-phase during planning. Lock-free event buffer design has multiple valid approaches (AbstractFifo, double-buffer swap, SPSC queue); phase research should evaluate which fits the record-then-playback pattern best.

### Phase 6: SDL2 Gamepad Integration (Safe)
**Rationale:** SDL2 integration is deferred until the plugin's core behavior is stable. The SDL2 pitfalls (singleton lifecycle, message pump conflict) are self-contained and can be addressed in a focused phase. Setting `SDL_HINT_JOYSTICK_THREAD "1"` before SDL_Init eliminates the WM_DEVICECHANGE message pump conflict with DAWs. The process-level singleton prevents multi-instance SDL_Init/SDL_Quit races.
**Delivers:** SDL2 singleton lifecycle with reference counting; `SDL_HINT_JOYSTICK_THREAD "1"` configured before init; gamepad right stick drives joystick XY via existing atomics; gamepad buttons drive TouchPlate triggers; hot-plug detection confirmed; filter CC (CC71/CC74) from left stick gated on isConnected(); no MIDI spam when no gamepad attached.
**Addresses:** PS4/Xbox gamepad support with hot-plug, filter CC output (CC71/CC74), gamepad looper transport controls
**Avoids:** SDL_Init/SDL_Quit per instance (Pitfall 2), filter CC flooding (Pitfall 3), SDL event pump conflict (Pitfall 8)
**Research flag:** Needs research-phase for SDL_HINT_JOYSTICK_THREAD behavior with specific SDL2 2.30.9 version — verify the hint is honored in this version and understand implications for SDL_PollEvent usage pattern.

### Phase 7: DAW Compatibility, Distribution, and Release
**Rationale:** Final phase resolves all distribution blockers: Ableton MIDI routing (the most DAW-specific concern), code signing, and installer creation. These are best addressed together as they share the goal of producing a shippable, user-installable plugin.
**Delivers:** Ableton Live 11/12 and Reaper 7 confirmed MIDI routing (VST3 category/IS_SYNTH setting finalized); pluginval passing at strictness level 5; Inno Setup installer placing bundle in correct VST3 system folder; EV or OV code signing certificate applied to DLL and installer; SmartScreen not blocking on clean Windows 11 VM.
**Addresses:** DAW compatibility (Windows), Windows distribution, code signing
**Avoids:** VST3 category/routing confusion (Pitfall 6), Ableton MIDI routing issues (Pitfall 9), code signing/SmartScreen blocks (Pitfall 10), MSVC CRT missing on end-user machines (Pitfall 11)
**Research flag:** Needs research-phase to determine the correct IS_SYNTH / IS_MIDI_EFFECT / VST3_CATEGORIES combination for Ableton Live MIDI output routing — this is empirically validated, not documented definitively.

### Phase Ordering Rationale

- **Phases 1–2 before Phase 3:** Pure logic and build correctness must be verified before real-time threading is exercised. A pitch bug found in unit tests is a 5-minute fix; the same bug found after looper integration is a multi-hour debugging session.
- **Phase 3 before Phase 4:** The note-off guarantee must be in place before trigger source complexity is added. Adding random gates on top of an existing note-off bug creates unpredictable stuck-note scenarios that are hard to attribute.
- **Phase 5 before Phase 6:** The LooperEngine mutex must be resolved before gamepad integration because both the looper transport controls (SDL button events) and the existing mutex contention paths interact. Fixing the mutex with gamepad already integrated adds a second variable.
- **Phase 6 before Phase 7:** SDL2 integration issues (especially the message pump conflict in Ableton) affect DAW compatibility testing. DAW compatibility testing should run against the full stack.
- **Phases 1–4 are sequential with no unusual risk.** Phases 5 and 6 are the technically risky phases and are appropriately flagged for research.

### Research Flags

**Phases needing deeper research during planning:**
- **Phase 5 (LooperEngine Hardening):** Lock-free single-producer single-consumer event buffer design has multiple valid approaches; phase research should evaluate AbstractFifo vs double-buffer swap vs a purpose-built ring buffer against the specific read/write pattern (audio thread writes during record, audio thread reads during playback, UI thread triggers destructive ops via atomic flags).
- **Phase 6 (SDL2 Gamepad):** Verify `SDL_HINT_JOYSTICK_THREAD "1"` behavior in SDL2 2.30.9 specifically — this hint changes the polling model and invalidates the current `SDL_PollEvent` timer pattern; confirm the correct replacement API call sequence.
- **Phase 7 (Distribution):** Ableton Live MIDI routing for MIDI-only VST3 generators is not definitively documented; empirical testing with IS_SYNTH TRUE vs IS_MIDI_EFFECT TRUE vs combined approaches is required.

**Phases with standard patterns (skip research-phase):**
- **Phase 1:** CMakeLists pinning, APVTS wiring, MSVC_RUNTIME_LIBRARY — all mechanical, well-documented.
- **Phase 2:** Pure C++ unit testing with Catch2 — no novel patterns.
- **Phase 3:** JUCE processBlock MIDI output and releaseResources — standard patterns in JUCE documentation and forum consensus.
- **Phase 4:** ppqPosition-based subdivision timing — established pattern, no novel risk.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | MEDIUM | Derived from direct codebase inspection + training knowledge; JUCE 8.x CMake API is stable; SDL2 in plugin context is less documented; verify JUCE release tag at github.com/juce-framework/JUCE/releases |
| Features | MEDIUM | Competitor analysis from training data through Aug 2025; manual verification against current KVR Audio listings recommended before locking requirements; project-internal feature decisions are HIGH confidence |
| Architecture | HIGH | Based on direct source file analysis; threading model is verifiable from code; JUCE audio thread constraints are well-established domain knowledge |
| Pitfalls | MEDIUM | Critical pitfalls (mutex, SDL lifecycle, CC flood, note-off) confirmed from source code — HIGH confidence on those four; DAW-specific behaviors (Ableton routing, SDL message pump) are MEDIUM/LOW confidence and require empirical validation |

**Overall confidence:** MEDIUM

### Gaps to Address

- **Ableton MIDI routing:** The correct combination of IS_SYNTH, IS_MIDI_EFFECT, and VST3_CATEGORIES for reliable MIDI output routing in Ableton Live 11/12 is not definitively resolvable from training data alone. Must be empirically tested in Phase 7 — budget time for multiple build/test cycles with different configurations.
- **SDL_HINT_JOYSTICK_THREAD in SDL2 2.30.9:** Verify this hint exists and functions as documented in this exact SDL2 version. If unavailable or unreliable, the fallback is accepting the message pump co-existence (which may work in practice) and adding robust error handling around SDL_PollEvent.
- **PS4 controller support without DS4Windows:** SDL2's GameController API normalizes XInput and DirectInput but PS4 DualShock 4 support on Windows may require DS4Windows for full axis mapping. Flag for validation with a physical PS4 controller during Phase 6.
- **LooperEngine event buffer sizing:** The correct maximum event capacity depends on the maximum loop length, gamepad poll rate, and whether gate events grow independently of joystick events. Calculate a hard upper bound in prepareToPlay() using sampleRate and bpm rather than using a fixed compile-time constant.
- **Competitor feature verification:** KVR Audio listings for Scaler 2, Chord Prism, and Ripchord should be spot-checked against current product pages before marketing copy is finalized. Confidence is MEDIUM on specific competitor feature claims.

## Sources

### Primary (HIGH confidence)
- Direct source analysis: `Source/PluginProcessor.h/.cpp`, `Source/TriggerSystem.h/.cpp`, `Source/LooperEngine.h/.cpp`, `Source/GamepadInput.h/.cpp`, `Source/ChordEngine.h`, `Source/ScaleQuantizer.h`, `Source/PluginEditor.h`, `CMakeLists.txt`
- `.planning/PROJECT.md` — authoritative project specification

### Secondary (MEDIUM confidence)
- JUCE 8 CMake API and APVTS patterns — training knowledge, Aug 2025 cutoff
- SDL2 2.30.x lifecycle and threading patterns — training knowledge
- JUCE audio thread real-time constraints — established domain knowledge, consistent across JUCE documentation and community
- VST3 Windows installer and code signing practices — training knowledge
- KVR Audio plugin database coverage (competitor features) — training knowledge, Aug 2025 cutoff

### Tertiary (LOW confidence — requires empirical validation)
- Ableton Live 11/12 VST3 MIDI generator routing behavior — training knowledge only; must be tested
- SDL2 message pump behavior inside DAW host on Windows — training knowledge; specific DAW interactions unverified
- PS4 controller SDL2 support without DS4Windows — training knowledge; validate with physical hardware

---
*Research completed: 2026-02-22*
*Ready for roadmap: yes*
