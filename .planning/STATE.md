# Project State

## Project Reference

**ChordJoystick** — XY joystick → 4-voice chord MIDI VST3, gamepad + looper, paid Windows release.

Core value: Continuous harmonic navigation via joystick with per-voice sample-and-hold gates — no competitor provides this.

## Current Position

- **Phase:** 03 of 7 — Core MIDI Output and Note-Off Guarantee
- **Plan:** 03-01 (not started)
- **Status:** pending

## Progress

```
Phase 01 [████░░░░░░]   Build Foundation    (partial — plugin crashes in Ableton)
Phase 02 [──────────]   Engine Validation   (skipped — no Catch2 per user preference)
Phase 03 [░░░░░░░░░░]   Core MIDI Output
Phase 04 [░░░░░░░░░░]   Trigger Sources
Phase 05 [░░░░░░░░░░]   Looper Hardening
Phase 06 [░░░░░░░░░░]   SDL2 Gamepad
Phase 07 [░░░░░░░░░░]   Distribution

Overall: [░░░░░░░░░░] ~10% (Phase 01 partial, Phase 02 skipped)
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
Stopped at: Phase 01 partial — advancing to Phase 03 by user decision. Phase 02 skipped (no Catch2).
Resume file: .planning/phases/03-core-midi-output/
