# Project State

## Project Reference

**ChordJoystick** — XY joystick → 4-voice chord MIDI VST3, gamepad + looper, paid Windows release.

Core value: Continuous harmonic navigation via joystick with per-voice sample-and-hold gates — no competitor provides this.

## Current Position

- **Phase:** 04 of 7 — Per-Voice Trigger Sources and Random Gate
- **Plan:** 04-02 (Tasks 1+2 complete — per-voice random clock + UI; awaiting DAW verification of Task 3)
- **Status:** Awaiting Reaper verification of random gate behavior (04-02 Task 3 checkpoint)

## Progress

```
Phase 01 [████░░░░░░]   Build Foundation    (partial — plugin crashes in Ableton)
Phase 02 [██████████]   Engine Validation   (COMPLETE — ScaleQuantizer+ChordEngine 15 tests green, checkpoint approved)
Phase 03 [██████████]   Core MIDI Output    (COMPLETE — 2/2 plans done, all 6 DAW tests passed in Reaper)
Phase 04 [████░░░░░░]   Trigger Sources     (IN PROGRESS — 04-02 Tasks 1+2 done, checkpoint pending DAW verify)
Phase 05 [░░░░░░░░░░]   Looper Hardening
Phase 06 [░░░░░░░░░░]   SDL2 Gamepad
Phase 07 [░░░░░░░░░░]   Distribution

Overall: [████░░░░░░] ~40% (Phase 01 partial, Phase 02 complete 2/2, Phase 03 complete 2/2)
```

## What's Been Built

- All 14 source files written (full implementation):
  - ScaleQuantizer, ChordEngine, TriggerSystem, LooperEngine, GamepadInput
  - PluginProcessor, PluginEditor
- APVTS parameter layout (all 40+ params) committed
- Scale quantization implementation committed
- PROJECT.md with validated requirements
- Full research completed (.planning/research/)
- **[NEW] CMakeLists.txt fixed: JUCE 8.0.4 pinned, static CRT set (01-01)**
- **[NEW] PluginProcessor.cpp fixed: Ableton dummy bus, isBusesLayoutSupported updated (01-01)**
- **[NEW] Build verified: ChordJoystick.vst3 compiled and installed (Visual Studio 18 2026)**
- **[NEW] Catch2 v3.8.1 FetchContent added, ChordJoystickTests target, ScaleQuantizer 218 assertions green (02-01)**
- **[NEW] ChordEngine 9-case test suite added; combined suite 15 tests, 0 failures; axis routing, transpose, octave offsets, MIDI clamping, scale quantization verified (02-02)**
- **[NEW] TriggerSystem::resetAllGates() added; processBlockBypassed() flushes note-offs on bypass; releaseResources() calls resetAllGates(); noteOff uses explicit (uint8_t)0; green gate LEDs; channel conflict warning (03-01)**
- **[NEW] DAW verification complete: all 6 Reaper tests passed — 4-voice note-on/off, retrigger safety, bypass flush, transport sustain, green LEDs, channel conflict warning (03-02)**
- **[NEW] Continuous joystick gate model: Chebyshev magnitude threshold, joystickThreshold APVTS param (04-01)**
- **[NEW] THRESHOLD horizontal slider in PluginEditor right column; TouchPlate pads dim and disable in JOY/RND mode (04-01)**
- **[NEW] JOY retrigger model (final): gate opens with noteOn(pitch); when joystick crosses scale-degree boundary, fires noteOff(oldPitch) then noteOn(newPitch); closes after 50ms stillness. No pitch bend / RPN. Works with any synth. (04-01 Fix 5 / 418c2a0)**
- **[NEW] Per-voice ppqPosition-synced random clock: prevSubdivIndex_[4] integer comparison fires on boundary; sample-count fallback when transport stopped (04-02 / 2b256a9)**
- **[NEW] randomDensity 1..8 hits-per-bar model with hitsPerBarToProbability(); randomGateTime knob (fraction of subdivision, 10ms floor) for note duration (04-02 / 2b256a9)**
- **[NEW] randomSubdiv0..3 per-voice APVTS params; 4 per-voice combo boxes in PluginEditor column-aligned under trigger source selectors; randomGateTimeKnob_ rotary (04-02 / 448556c)**

## Key Decisions

| Decision | Outcome |
|----------|---------|
| JUCE 8.0.4 + CMake FetchContent | LOCKED — pinned to tag 8.0.4 (commit 51d11a2), GIT_SHALLOW TRUE |
| Generator: Visual Studio 18 2026 | VS 17/16 not installed on this machine; VS 18 2026 Community used |
| SDL2 2.30.9 static | Locked — already correctly configured |
| VST3 only (Windows) | Locked — v1 scope |
| 4 voices on MIDI channels 1-4 | Default routing, user-configurable per voice |
| Sample-and-hold pitch model | Pitch locked at trigger moment, not continuous |
| Quantize ties → down | Deterministic behavior at equidistance |
| Lock-free LooperEngine | Required before DAW testing — current mutex impl is unsafe |
| JUCE 8.0.4 BusesLayout API | getMainInputChannels()/getMainOutputChannels() — NOT the Count() variants |
| Catch2 placed after SDL2 MakeAvailable | Ensures static CRT setting already applied; prevents LNK2038 |
| Test target compiles .cpp directly | Not linking ChordJoystick plugin target — avoids GUI/DAW deps in headless tests |
| ASCII hyphens in Catch2 test names | Windows ctest garbles Unicode em-dash in filter arguments |
| ScalePreset::Chromatic in baseParams() | Chromatic is 12-note pass-through — lets axis/transpose/octave tests get integer-exact values without quantization snap masking arithmetic bugs |
| resetAllGates() as single cleanup primitive | Shared between releaseResources() and processBlockBypassed() — avoids duplicated atomic clearing logic |
| noteOff explicit (uint8_t)0 velocity | 3-arg form avoids host interpretation ambiguity vs 2-arg form |
| Gate LED uses Clr::gateOn (green) | Corrects semantic mismatch — green universally understood as open/active |
| channelConflictShown_ cache | Prevents setVisible() being called every 30 Hz tick when state is stable |
| Reaper as DAW verification target | Ableton crashes on instantiation (deferred); Reaper provides full MIDI monitoring without the crash |
| 6-test structured DAW protocol | Discrete test cases: basic output, LED color, retrigger, bypass flush, transport, conflict UI — all passed 03-02 |
| Chebyshev distance for joystick magnitude | max(abs(dx), abs(dy)) avoids diagonal bias vs Manhattan distance |
| JOY gate holds indefinitely | No auto-close on stillness; closes only via resetAllGates() or voice mode switch |
| JOY retrigger as final model | noteOff(old)+noteOn(new) on scale-degree change; universal synth compatibility; no RPN/pitchBend infrastructure required |
| joyActivePitch_[v] tracks sounding pitch | -1=gate closed; updated on every retrigger; used for comparison and gate-close note-off |
| Pitch bend model superseded | joyBasePitch_/joyLastBendValue_/sendBendRangeRPN/MidiCallback all removed; simpler and more reliable |
| TouchPlate dimming reads APVTS in paint() | getRawParameterValue()->load() is safe from message thread (atomic<float>*) |
| PPQ subdivision index (int64_t) for DAW sync | Integer comparison avoids float equality issues; fires exactly once per boundary crossing |
| randomDensity range 1..8 hits-per-bar | More intuitive than 0..1 probability; hitsPerBarToProbability() converts to per-event probability |
| randomGateTime × samplesPerSubdiv with 10ms floor | Prevents inaudible staccato at extreme settings; block-granular countdown is sufficient for musical timing |
| wasPlaying_ edge detection for transport restart | prevSubdivIndex_[4] reset to -1 on play start; prevents spurious fire on the subdivision already seen before stop |

## Known Issues (Must Fix Before Shipping)

1. ~~**JUCE pinned to `origin/master`**~~ — FIXED in 01-01 (now 8.0.4).
2. **[BLOCKER] Plugin crashes on load in Ableton Live 11** — appears in browser, crashes on instantiation. SDL_HINT_JOYSTICK_THREAD fix applied, root cause unresolved. Must fix before Phase 03 DAW testing.
3. **std::mutex in LooperEngine processBlock** — Blocks audio thread. Fix in Phase 05.
4. **Filter CC (CC71/CC74) emitted unconditionally** — Floods synth at ~175 msgs/sec when no gamepad. Fix in Phase 06.
5. ~~**releaseResources() is empty**~~ — FIXED in 03-01 (now calls resetAllGates()).
6. **SDL_Init/SDL_Quit per instance** — Multi-instance race condition. Fix in Phase 06.
7. **COPY_PLUGIN_AFTER_BUILD requires elevated process** — manual copy needed each rebuild.

## Pending Todos

(none)

## Blockers / Concerns

- **[ACTIVE BLOCKER]** Plugin crashes on Ableton instantiation — PluginEditor or COM threading likely cause
- Ableton MIDI routing: empirical testing required (no definitive documentation)

## Session Continuity

Last session: 2026-02-22
Stopped at: 04-02 Tasks 1+2 complete — per-voice random clock + UI built (2b256a9, 448556c). Load ChordJoystick.vst3 in Reaper. Verify: (1) RND fires without transport at subdivision rate; (2) RND syncs to 1/8-note grid when playing; (3) density 1 = sparse, density 8 = dense; (4) gate time knob controls note duration; (5) ROOT 1/4 + THIRD 1/32 fire independently. Type "approved" to continue to 04-03.
Resume file: .planning/phases/04-per-voice-trigger-sources-and-random-gate/04-02-PLAN.md (Task 3 checkpoint)
