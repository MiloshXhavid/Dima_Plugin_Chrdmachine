# Project State

## Project Reference

**ChordJoystick** — XY joystick → 4-voice chord MIDI VST3, gamepad + looper, paid Windows release.

Core value: Continuous harmonic navigation via joystick with per-voice sample-and-hold gates — no competitor provides this.

## Current Position

- **Phase:** 03 of 7 — Core MIDI Output and Note-Off Guarantee
- **Plan:** 03-01 (complete)
- **Status:** In progress (1/2 plans done)

## Progress

```
Phase 01 [████░░░░░░]   Build Foundation    (partial — plugin crashes in Ableton)
Phase 02 [██████████]   Engine Validation   (COMPLETE — ScaleQuantizer+ChordEngine 15 tests green, checkpoint approved)
Phase 03 [█████░░░░░]   Core MIDI Output    (in progress — 1/2 plans done, note-off guarantee complete)
Phase 04 [░░░░░░░░░░]   Trigger Sources
Phase 05 [░░░░░░░░░░]   Looper Hardening
Phase 06 [░░░░░░░░░░]   SDL2 Gamepad
Phase 07 [░░░░░░░░░░]   Distribution

Overall: [███░░░░░░░] ~30% (Phase 01 partial, Phase 02 complete 2/2, Phase 03 in progress 1/2)
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
Stopped at: 03-01-PLAN.md fully complete. Note-off guarantee + gate LED + channel conflict warning done. Next: Phase 03 Plan 02 (DAW verification in Reaper).
Resume file: .planning/phases/03-core-midi-output-and-note-off-guarantee/03-02-PLAN.md
