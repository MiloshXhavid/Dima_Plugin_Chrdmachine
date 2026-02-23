---
phase: 01-build-foundation-and-juce-version-lock
plan: 02
subsystem: infra
tags: [ableton, vst3, smoke-test, sdl2, partial]

key-files:
  created: []
  modified: []

key-decisions:
  - "Plugin appears in Ableton Live 11 VST3 browser — dummy bus approach works for discovery"
  - "Plugin crashes on instantiation (drag to MIDI track) — Ableton shows 'plugin cannot be opened'"
  - "SDL_HINT_JOYSTICK_THREAD fix applied and deployed — did not resolve the crash"
  - "Proceeding to Phase 03 by user decision — Ableton load issue tracked as blocker"
  - "Phase 02 (Catch2 unit tests) skipped per user preference for manual testing"

requirements-completed: []

duration: session
completed: 2026-02-22
---

# Phase 01 Plan 02: Smoke Test — PARTIAL (DAW load blocked)

**Plugin visible in Ableton VST3 browser but crashes on instantiation. Proceeding to Phase 03 by user decision.**

## Smoke Test Result

- **Step 1 (VST3 browser):** PASS — ChordJoystick appears in Ableton Live 11 VST3 browser after rescan
- **Step 2 (load on MIDI track):** FAIL — "plugin lässt sich nicht öffnen" (plugin cannot be opened)
- **Step 3 (trigger note):** NOT REACHED
- **Step 4 (APVTS round-trip):** NOT REACHED

User response: `"failed: plugin couldn't open in midi track of ableton"`

## IS_SYNTH Finding (Open Question 1)

Not resolved — plugin crashes before load completes. `IS_SYNTH FALSE` + dummy bus is sufficient for browser visibility. Whether `IS_SYNTH TRUE` would help load is unknown.

## processBlock behaviour in Ableton (Open Question 2)

Not observed — plugin did not reach processBlock.

## MIDI Channel Merging (Open Question 3)

Not observed — plugin did not load.

## Diagnostics Attempted

1. **SDL_HINT_JOYSTICK_THREAD "1"** added before `SDL_Init` in `GamepadInput.cpp` — deployed to installed VST3, crash persists
2. **Crash timing:** Occurs when dragging to MIDI track (plugin instantiation), not at browser scan
3. **Likely root causes not yet ruled out:**
   - PluginEditor constructor crash (UI component failure on Ableton's message thread)
   - COM threading conflict (`SDL_Init` calls `CoInitialize` — may conflict with Ableton's COM apartment)
   - PluginProcessor member init order issue (chord_, trigger_, looper_, gamepad_ all default-constructed)

## Known Risks Carried Forward

- **[BLOCKER]** Plugin does not load in Ableton Live 11 — must be resolved before Phase 03 DAW testing
- LooperEngine std::mutex on audio thread — Phase 05
- Filter CC flood when no gamepad — Phase 06
- releaseResources() empty — Phase 03

## Build Notes

- JUCE 8.0.4 / Visual Studio 18 2026 Community / MSVC 14.50
- `COPY_PLUGIN_AFTER_BUILD` fails without elevated process — manual copy required each rebuild
- SDL hint fix committed: `2ee7bb2`

---
*Phase: 01-build-foundation-and-juce-version-lock*
*Completed: 2026-02-22 (partial — DAW smoke test blocked)*

## Self-Check: PARTIAL

- FOUND: Plugin visible in Ableton VST3 browser
- BLOCKED: Plugin instantiation crash — root cause unresolved
- PROCEEDING: User decision to advance to Phase 03

---

## Retroactive Resolution (2026-02-23)

The Ableton instantiation crash was diagnosed and fixed in **Phase 05-03** (`isMidiEffect()=true`, empty `BusesProperties()`, `isBusesLayoutSupported` accepting 0-channel only). Root cause: plugin was presenting as an instrument rather than a MIDI effect — Ableton refused to load it on a MIDI track.

All original 01-02 smoke test criteria are now satisfied:

| Step | Original result | Final result |
|------|----------------|--------------|
| 1 — Plugin in Ableton VST3 browser | ✓ PASS | ✓ PASS (appears in MIDI Effects browser as Fx/MIDI) |
| 2 — Loads on MIDI track without crash | ✗ FAIL | ✓ PASS (fixed in 05-03, verified in 06-03) |
| 3 — Touch-plate triggers audible note | NOT REACHED | ✓ PASS (verified in 06-03 gamepad test #3) |
| 4 — APVTS parameters survive save/reopen | NOT REACHED | ✓ PASS (verified across multiple phases) |

IS_SYNTH finding (Open Question 1): `IS_SYNTH FALSE` + `isMidiEffect()=true` + empty `BusesProperties()` is the correct combination. Ableton requires the MIDI effect category — not `IS_SYNTH TRUE`.

Phase 01 goal fully achieved. Marking complete retroactively.
