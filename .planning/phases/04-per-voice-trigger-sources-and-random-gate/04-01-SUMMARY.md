---
phase: 04-per-voice-trigger-sources-and-random-gate
plan: 01
subsystem: trigger
tags: [juce, midi, trigger, joystick, continuous-gate, retrigger, vst3, cpp]

# Dependency graph
requires:
  - phase: 03-core-midi-output-and-note-off-guarantee
    provides: TriggerSystem with resetAllGates(), processBlockBypassed(), note-off guarantee

provides:
  - joystickThreshold APVTS param (0.001..0.1, default 0.015) controlling gate sensitivity
  - Continuous joystick gate model in TriggerSystem (Chebyshev magnitude)
  - JOY retrigger model: noteOff(oldPitch) then noteOn(newPitch) when scale degree changes
  - joyActivePitch_[4]: pitch currently sounding per voice, -1 = no active gate
  - joystickStillSamples_[4] debounce counter: fires note-off after 50ms of stillness below threshold
  - Horizontal THRESHOLD slider in PluginEditor right column with SliderParameterAttachment
  - TouchPlate dimming and mouseDown/mouseUp no-op guard in JOY and RND modes

affects:
  - 04-02 (random gate -- shares TriggerSystem per-voice infrastructure)
  - 06-sdl2-gamepad (notifyJoystickMoved stub may be revisited)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Chebyshev distance (max|dx|, |dy|) for joystick motion magnitude -- avoids diagonal bias
    - JOY retrigger: fireNoteOff(oldPitch) then fireNoteOn(newPitch) when heldPitches[v] changes
    - 50ms debounce: joystickStillSamples_[v] accumulates blockSize per block below threshold
    - getRawParameterValue() load() from paint() message thread -- safe for atomic<float>*

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Simple retrigger over pitch bend (final): pitchBend required RPN setup and synth configuration; retrigger is universal — works with any synth. noteOff(oldPitch) then noteOn(newPitch) when joystick crosses a quantizer zone boundary."
  - "joyActivePitch_[v] tracks the pitch currently sounding; -1 = gate closed. Set on noteOn, cleared on noteOff."
  - "Gate closes after 50ms debounce below threshold — joystickStillSamples_[v] accumulates blockSize each block"
  - "Chebyshev distance: max(|dx|, |dy|) over Manhattan -- avoids diagonal-motion double-triggering"
  - "fireNoteOff already sets gateOpen_[v] false and activePitch_[v] = -1; no redundant stores needed in JOY branch"

patterns-established:
  - "Retrigger-on-pitch-change: compare newPitch against joyActivePitch_[v] each block; send noteOff/noteOn pair only when they differ"
  - "JOY gate state: gateOpen_ (atomic), activePitch_ (fireNoteOff/On tracking), joyActivePitch_ (retrigger comparison)"

requirements-completed: [TRIG-01, TRIG-02]

# Metrics
duration: ~90min
completed: 2026-02-22
---

# Phase 4 Plan 1: Joystick Continuous Gate + Retrigger on Pitch Change Summary

**Chebyshev-magnitude continuous JOY gate model with simple retrigger: noteOff(oldPitch) then noteOn(newPitch) when the joystick crosses a scale-degree boundary. Works with any synth. No pitch bend, no RPN. 50ms stillness debounce for gate close. APVTS joystickThreshold param, THRESHOLD slider in UI, TouchPlate dimming in JOY/RND modes.**

## Performance

- **Duration:** ~90 min (original plan + four iterative fixes + pitch-bend-to-retrigger migration)
- **Completed:** 2026-02-22
- **Tasks:** All tasks and fixes complete
- **Files modified:** 5 (TriggerSystem.h, TriggerSystem.cpp, PluginProcessor.cpp, PluginEditor.h, PluginEditor.cpp)

## Accomplishments

- Added `joystickThreshold` APVTS parameter (0.001..0.1, default 0.015) and horizontal THRESHOLD slider in right column
- Replaced one-shot delta-based joystick trigger with sustained continuous gate model (gate opens on threshold-crossing)
- Implemented JOY retrigger model: gate opens with `fireNoteOn(heldPitches[v])`; while gate is open and joystick is moving, if `heldPitches[v]` has changed (quantizer crossed a new scale degree), fires `fireNoteOff` on the old pitch then `fireNoteOn` on the new pitch. `joyActivePitch_[v]` tracks the currently sounding pitch for comparison.
- Gate closes automatically after 50ms of stillness below threshold: `joystickStillSamples_[v]` accumulates `blockSize` each block below threshold; when `>= sampleRate * 0.05`, fires `fireNoteOff` then clears `joyActivePitch_[v]`
- Removed all pitch bend / RPN infrastructure: `joyBasePitch_[4]`, `joyLastBendValue_[4]`, `sendBendRangeRPN()`, `MidiCallback onMidi` in `ProcessParams`, `tp.onMidi` lambda in PluginProcessor, `pitchWheel` calls in `processBlockBypassed`
- `joyActivePitch_[v]` replaces `joyBasePitch_[v]`: tracks the pitch currently sounding (for retrigger comparison and gate-close note-off); initialized to `-1` (gate closed)
- `resetAllGates()` updated: clears `joyActivePitch_[v]` instead of `joyBasePitch_/joyLastBendValue_`
- Build clean: all three files compile; VST3 artifact at `build/ChordJoystick_artefacts/Debug/VST3/ChordJoystick.vst3`
- TouchPlate pads dim visually and become non-functional in JOY and RND trigger modes

## Commit History

All work committed atomically:

| Commit | Type | Description |
|--------|------|-------------|
| `afc4ba0` | feat | joystickThreshold APVTS param + continuous gate model |
| `0a2e023` | feat | Threshold slider + TouchPlate dimming in PluginEditor |
| `a2cea2e` | fix | JOY legato note tracking (superseded) |
| `3ccd1a2` | fix | Close JOY gate after 50ms of stillness below threshold |
| `f6e6d5f` | fix | JOY pitch tracking via quantized pitch bend (superseded) |
| `418c2a0` | fix | JOY retrigger on pitch change — noteOff old then noteOn new (final) |

## Files Created/Modified

- `Source/TriggerSystem.h` — Removed `MidiCallback` type alias and `onMidi` field from `ProcessParams`; removed `joyBasePitch_[4]`, `joyLastBendValue_[4]`, `sendBendRangeRPN()` private helper; added `joyActivePitch_[4] {-1,-1,-1,-1}`
- `Source/TriggerSystem.cpp` — Removed `sendBendRangeRPN()` implementation; constructor fills `joyActivePitch_` with -1; gate-open: `fireNoteOn(pitch)` + `joyActivePitch_[v] = pitch`; gate-open moving: compare `newPitch != joyActivePitch_[v]`, fire `noteOff`/`noteOn` pair; gate-close (50ms): `fireNoteOff`, `joyActivePitch_[v] = -1`; `resetAllGates()`: clears `joyActivePitch_[v]`
- `Source/PluginProcessor.cpp` — Removed `tp.onMidi` lambda; removed `pitchWheel(ch, 0)` from `processBlockBypassed()`

## Decisions Made

- **Simple retrigger as final model (over pitch bend):** Pitch bend required per-channel RPN setup (6 CC messages) and synth configuration to handle bend range; not all synths respond correctly. Retrigger (noteOff + noteOn) is universally understood by every hardware and software synthesizer. Since `heldPitches` is already scale-quantized, pitch changes only occur at discrete in-scale degree boundaries — the behavior is identical to pressing a new key on a keyboard.
- **joyActivePitch_ replaces joyBasePitch_:** The old `joyBasePitch_` was fixed at gate-open and never updated (bend moved the pitch instead). The new `joyActivePitch_` is updated on every retrigger to track the currently sounding note — this is required for correct note-off when gate closes and for the retrigger comparison.
- **No pitchWheel in processBlockBypassed:** The old implementation reset pitch bend to 0 before note-off. With the retrigger model there is no pitch bend state to clean up; `processBlockBypassed` now only sends `noteOff` for any active pitch from `getActivePitch()`.
- **Gate closes after 50ms debounce below threshold:** Joystick jitter at centre would cause spurious gate closes without debounce. `joystickStillSamples_[v]` accumulates `blockSize` per block; threshold is `sampleRate * 0.05` samples.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Pitch bend model replaced with retrigger**
- **Found during:** Post-plan refinement (explicit instruction to simplify)
- **Issue:** Pitch bend model (commit `f6e6d5f`) required RPN bend-range setup and synth configuration; not reliably supported by all synths. Added complexity without musical benefit over retrigger for a discrete-scale-degree quantizer.
- **Fix:** Removed `joyBasePitch_[4]`, `joyLastBendValue_[4]`, `MidiCallback onMidi`, `sendBendRangeRPN()`, all `pitchWheel` calls. Added `joyActivePitch_[4]` tracking current pitch; when joystick moves and `newPitch != joyActivePitch_[v]`, fires `noteOff(old)` then `noteOn(new)`.
- **Files modified:** Source/TriggerSystem.h, Source/TriggerSystem.cpp, Source/PluginProcessor.cpp
- **Commit:** 418c2a0

## Self-Check: PASSED

- `Source/TriggerSystem.h` — present, contains `joyActivePitch_`, no `joyBasePitch_`, no `MidiCallback`, no `sendBendRangeRPN`
- `Source/TriggerSystem.cpp` — present, contains retrigger logic, no `pitchWheel`, no RPN sequence
- `Source/PluginProcessor.cpp` — present, no `tp.onMidi` lambda, no `pitchWheel` in `processBlockBypassed`
- Commit `418c2a0` — verified in git log

## Verification

Build output confirms clean compile:
- `TriggerSystem.cpp` compiled
- `PluginProcessor.cpp` compiled
- `ChordJoystick_SharedCode.lib` rebuilt
- `ChordJoystick.vst3` artifact built at `build/ChordJoystick_artefacts/Debug/VST3/ChordJoystick.vst3`
- Install step failed with `Permission denied` (pre-existing: no admin rights to write `C:\Program Files\Common Files\VST3\`)

## Next Steps for DAW Testing

Load `build/ChordJoystick_artefacts/Debug/VST3/ChordJoystick.vst3` in Reaper with a MIDI monitor:

1. Set ROOT voice to JOY trigger source
2. Move joystick above threshold — observe `noteOn(pitch)` in MIDI monitor
3. Move joystick to different quantizer zone — observe `noteOff(oldPitch)` then `noteOn(newPitch)` — NO pitch bend messages
4. Verify pitch changes correspond to in-scale degree boundaries only
5. Release joystick to center — after ~50ms, observe `noteOff(pitch)` in MIDI monitor

---
*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Plan: 01*
*Completed: 2026-02-22*
