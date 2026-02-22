# Project State

## Project Reference

**ChordJoystick** — XY joystick → 4-voice chord MIDI VST3, gamepad + looper, paid Windows release.

Core value: Continuous harmonic navigation via joystick with per-voice sample-and-hold gates — no competitor provides this.

## Current Position

- **Phase:** 02 of 7 — Core Engine Validation
- **Plan:** 02-02 (complete — checkpoint approved by user)
- **Status:** Milestone complete

## Progress

```
Phase 01 [████░░░░░░]   Build Foundation    (partial — plugin crashes in Ableton)
Phase 02 [██████████]   Engine Validation   (COMPLETE — ScaleQuantizer+ChordEngine 15 tests green, checkpoint approved)
Phase 03 [░░░░░░░░░░]   Core MIDI Output
Phase 04 [░░░░░░░░░░]   Trigger Sources
Phase 05 [░░░░░░░░░░]   Looper Hardening
Phase 06 [░░░░░░░░░░]   SDL2 Gamepad
Phase 07 [░░░░░░░░░░]   Distribution

Overall: [██░░░░░░░░] ~25% (Phase 01 partial, Phase 02 complete 2/2 plans)
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

## Known Issues (Must Fix Before Shipping)

1. ~~**JUCE pinned to `origin/master`**~~ — FIXED in 01-01 (now 8.0.4).
2. **[BLOCKER] Plugin crashes on load in Ableton Live 11** — appears in browser, crashes on instantiation. SDL_HINT_JOYSTICK_THREAD fix applied, root cause unresolved. Must fix before Phase 03 DAW testing.
3. **std::mutex in LooperEngine processBlock** — Blocks audio thread. Fix in Phase 05.
4. **Filter CC (CC71/CC74) emitted unconditionally** — Floods synth at ~175 msgs/sec when no gamepad. Fix in Phase 06.
5. **releaseResources() is empty** — Stuck notes on transport stop. Fix in Phase 03.
6. **SDL_Init/SDL_Quit per instance** — Multi-instance race condition. Fix in Phase 06.
7. **COPY_PLUGIN_AFTER_BUILD requires elevated process** — manual copy needed each rebuild.

## Pending Todos

(none)

## Blockers / Concerns

- **[ACTIVE BLOCKER]** Plugin crashes on Ableton instantiation — PluginEditor or COM threading likely cause
- Ableton MIDI routing: empirical testing required (no definitive documentation)

## Session Continuity

Last session: 2026-02-22
Stopped at: 02-02-PLAN.md fully complete — checkpoint approved by user. Phase 02 done. Next: Phase 03 (Core MIDI Output).
Resume file: .planning/phases/03-core-midi-output/ (Phase 03, Plan 03-01)
