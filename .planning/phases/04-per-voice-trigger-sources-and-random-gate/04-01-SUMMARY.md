---
phase: 04-per-voice-trigger-sources-and-random-gate
plan: 01
subsystem: trigger
tags: [juce, midi, trigger, joystick, continuous-gate, legato, vst3, cpp]

# Dependency graph
requires:
  - phase: 03-core-midi-output-and-note-off-guarantee
    provides: TriggerSystem with resetAllGates(), processBlockBypassed(), note-off guarantee

provides:
  - joystickThreshold APVTS param (0.001..0.1, default 0.015) controlling gate sensitivity
  - Continuous joystick gate model in TriggerSystem (Chebyshev magnitude)
  - Legato MIDI note transitions for JOY gate: note-on(newPitch) before note-off(oldPitch)
  - joyActivePitch_[4] tracks currently sounding MIDI pitch per JOY voice
  - joystickStillSamples_[4] debounce counter: fires note-off after 50ms of stillness below threshold
  - Horizontal THRESHOLD slider in PluginEditor right column with SliderParameterAttachment
  - TouchPlate dimming and mouseDown/mouseUp no-op guard in JOY and RND modes

affects:
  - 04-02 (random gate — shares TriggerSystem per-voice infrastructure)
  - 06-sdl2-gamepad (notifyJoystickMoved stub may be revisited)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Chebyshev distance (max|dx|, |dy|) for joystick motion magnitude — avoids diagonal bias
    - Legato MIDI ordering: note-on(newPitch) inserted before note-off(oldPitch) in MidiBuffer at same sampleOffset
    - getRawParameterValue() load() from paint() message thread — safe for atomic<float>*

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "Chebyshev distance (max|dx|, |dy|) for threshold comparison — avoids diagonal asymmetry vs Manhattan"
  - "Legato MIDI ordering: note-on new pitch first, note-off old pitch second at same sampleOffset — JUCE MidiBuffer plays same-timestamp events in insertion order"
  - "Gate closes after 50ms debounce below threshold — joystickStillSamples_[v] accumulates blockSize each block; fires note-off and resets when >= sampleRate*0.05"
  - "joyActivePitch_[v] tracks sounding pitch separately from activePitch_[v] to support legato note-off targeting"
  - "Removed pitch bend approach entirely — synths use different bend-range defaults, making semi-accurate bends unreliable"
  - "joystickTrig_ atomic kept (not removed) since resetAllGates() clears it; continuous gate logic bypasses it"
  - "THRESH label painted via PluginEditor::paint() direct g.drawText — no extra juce::Label member needed"
  - "TouchPlate dimming reads APVTS from paint() via getRawParameterValue()->load() — atomic, safe on message thread"

patterns-established:
  - "Legato MIDI pattern: noteOn(newPitch) + noteOff(oldPitch) at identical sampleOffset; JUCE MidiBuffer insertion order determines playback order"
  - "Mode-aware TouchPlate: check triggerSource param in paint()/mouseDown()/mouseUp() before acting"

requirements-completed: [TRIG-01, TRIG-02]

# Metrics
duration: 45min
completed: 2026-02-22
---

# Phase 4 Plan 1: Joystick Continuous Gate + Legato Note Tracking Summary

**Chebyshev-magnitude continuous JOY gate model with legato MIDI note transitions (note-on new pitch before note-off old pitch), 50ms stillness debounce note-off, APVTS joystickThreshold param, THRESHOLD slider in UI, TouchPlate dimming in JOY/RND modes, and no pitch bend**

## Performance

- **Duration:** ~45 min (across original plan + three iterative fixes)
- **Completed:** 2026-02-22
- **Tasks:** All tasks and fixes complete
- **Files modified:** 5 (TriggerSystem.h, TriggerSystem.cpp, PluginProcessor.cpp, PluginEditor.h, PluginEditor.cpp)

## Accomplishments

- Added `joystickThreshold` APVTS parameter (0.001..0.1, default 0.015) and horizontal THRESHOLD slider in right column
- Replaced one-shot delta-based joystick trigger with sustained continuous gate model (open on threshold-crossing)
- Implemented legato MIDI note transitions for JOY gate: when quantizer zone changes while gate is open, `noteOn(newPitch)` is inserted into MidiBuffer before `noteOff(oldPitch)` at the same sample offset — supporting synths receive the new note before the old is released, enabling legato (no envelope restart)
- Gate closes automatically after 50ms of stillness below threshold: `joystickStillSamples_[v]` accumulates `blockSize` each block; when `>= sampleRate * 0.05`, fires `noteOff` and resets state — counter resets to 0 on any above-threshold motion
- `joyActivePitch_[v]` tracks the currently sounding pitch per JOY voice for correct note-off targeting
- TouchPlate pads dim visually and become non-functional in JOY and RND trigger modes
- Removed all pitch bend code: `BendCallback`, `onBend`, `sendBendRangeRPN()`, `sendPitchBend()`, `joyLastBendValue_[4]`, RPN CC (101, 100, 6, 38), `pitchWheel` calls
- Re-added `joystickStillSamples_[4]` (was removed in pitch-bend rewrite) as a 50ms debounce counter for gate-close on stillness
- Build clean, VST3 installed to `C:\Program Files\Common Files\VST3\ChordJoystick.vst3`

## Commit History

All work committed atomically:

| Commit | Type | Description |
|--------|------|-------------|
| `afc4ba0` | feat | joystickThreshold APVTS param + continuous gate model |
| `0a2e023` | feat | Threshold slider + TouchPlate dimming in PluginEditor |
| `34fd29d` | fix | 500ms stillness retrigger (pitch-change detection on settle) |
| `58f8158` | fix | Real-time retrigger during movement (superseded approach) |
| `4806173` | fix | Pitch bend approach (superseded — removed) |
| `a2cea2e` | fix | JOY legato note tracking — note-on new before note-off old, no pitch bend |
| `3ccd1a2` | fix | Close JOY gate after 50ms of stillness below threshold |

## Files Created/Modified

- `Source/TriggerSystem.h` — Removed `BendCallback`, `onBend` from `ProcessParams`, removed `joyLastBendValue_[4]`; kept `joyActivePitch_[4]`; removed `sendBendRangeRPN()` and `sendPitchBend()` declarations; added `joystickThreshold` to `ProcessParams`; added `joystickStillSamples_[4]` for 50ms debounce
- `Source/TriggerSystem.cpp` — Implemented legato JOY gate: gate opens on threshold-crossing with `fireNoteOn`; while open and above threshold, if pitch zone changes, inserts `noteOn(newPitch)` then `noteOff(oldPitch)` directly; below threshold accumulates `joystickStillSamples_[v] += blockSize` and fires note-off after 50ms; counter resets on motion; `resetAllGates()` clears counter
- `Source/PluginProcessor.cpp` — Added `joystickThreshold` APVTS param; removed `tp.onBend` lambda and all RPN CC events; removed `#include <limits>` (no longer needed for `INT_MIN` sentinel)
- `Source/PluginEditor.h` — Added `thresholdSlider_` and `thresholdSliderAtt_` members
- `Source/PluginEditor.cpp` — Constructor: slider init + APVTS attachment; `resized()`: threshold row below filter atten; `paint()`: THRESH label; TouchPlate: mode-aware `paint()`/`mouseDown()`/`mouseUp()`

## Decisions Made

- **Legato over pitch bend:** Pitch bend requires each receiving synth to be pre-configured with a matching bend range; the RPN setup sequence is not reliably honoured by all instruments. Legato note messages (note-on new + note-off old) are a universally understood protocol — every polyphonic synth handles them correctly.
- **Legato insertion order:** `noteOn(newPitch)` is inserted into `MidiBuffer` before `noteOff(oldPitch)` at identical `sampleOffset`. JUCE `MidiBuffer` processes same-timestamp events in insertion order, so the synth sees the new note held before the old one releases — the definition of MIDI legato.
- **50ms debounce gate-close on stillness:** After the pitch-bend rewrite removed stillness tracking, the gate had no auto-close mechanism — notes would sustain indefinitely after joystick release. A 50ms debounce (accumulated as `blockSize` samples per block, compared against `sampleRate * 0.05`) was re-added. The debounce is long enough to avoid false closures during slow joystick movement and short enough to feel responsive on release.
- **`joyActivePitch_[v]` separate from `activePitch_[v]`:** The `activePitch_` array is managed by `fireNoteOn`/`fireNoteOff`. During a legato transition the new pitch is written to `activePitch_` by `fireNoteOn`, but we still need the old pitch for the manual `noteOff`. `joyActivePitch_[v]` holds the old value until after both MIDI messages are queued.
- **Chebyshev distance:** `max(|dx|, |dy|)` over Manhattan distance avoids diagonal-motion double-triggering while remaining simple and allocation-free.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Pitch bend approach removed — replaced with legato note tracking**
- **Found during:** Fix 3 / this continuation
- **Issue:** Pitch bend requires per-synth RPN bend-range configuration; the sentinel `INT_MIN` approach was fragile and non-standard. Gate pitch was locked to the original note-on pitch regardless of joystick movement.
- **Fix:** Replaced with legato MIDI: `noteOn(newPitch)` then `noteOff(oldPitch)` at same sample offset. Removed `BendCallback`, all RPN CC, `pitchWheel` calls, `joyLastBendValue_`, `joystickStillSamples_`, and 500ms stillness retrigger from `TriggerSystem` and `PluginProcessor`.
- **Files modified:** Source/TriggerSystem.h, Source/TriggerSystem.cpp, Source/PluginProcessor.cpp
- **Commit:** a2cea2e

**2. [Rule 1 - Bug] 500ms stillness retrigger logic removed (prior iteration)**
- **Found during:** Fix 1 (was itself a deviation fix; now removed as part of legato rewrite)
- **Issue:** The 500ms settle timer was a workaround for the pitch-bend approach's inability to update pitch in real time. With legato note tracking, pitch updates continuously — no settle timer needed.
- **Fix:** Removed `joystickStillSamples_[4]` and all stillness counter logic from `processBlock` and `resetAllGates`.
- **Commit:** a2cea2e

**3. [Rule 2 - Missing functionality] JOY gate had no auto-close mechanism after pitch-bend rewrite**
- **Found during:** Post-completion review
- **Issue:** The legato rewrite (commit a2cea2e) removed `joystickStillSamples_[4]` entirely. This left JOY-source gates with no note-off path on joystick release — notes sustained indefinitely until `resetAllGates()` was called. Held notes blocking subsequent triggers.
- **Fix:** Re-added `joystickStillSamples_[4]` as a 50ms debounce counter. Counter accumulates `blockSize` each block while below threshold; fires `noteOff(joyActivePitch_[v])` and resets all JOY gate state when `>= sampleRate * 0.05`. Counter resets to 0 whenever motion is above threshold. Added `.fill(0)` in constructor and reset in `resetAllGates()`.
- **Files modified:** Source/TriggerSystem.h, Source/TriggerSystem.cpp
- **Commit:** 3ccd1a2

## Verification

Build output confirms clean compile and VST3 install:
- `ChordJoystick_SharedCode.lib` rebuilt
- `ChordJoystick.vst3` rebuilt and installed to `C:\Program Files\Common Files\VST3\`

## Next Steps for DAW Testing

Load `ChordJoystick.vst3` in Reaper with a MIDI monitor and a legato-capable synth:

1. Set ROOT voice to JOY trigger source
2. Move joystick above threshold — observe single `noteOn` in MIDI monitor
3. Move joystick to different quantizer zone while holding — observe `noteOn(newPitch)` followed immediately by `noteOff(oldPitch)` (same timestamp, back-to-back) — NO second attack on a legato synth
4. Set a non-legato synth — should hear pitch change without gap (both events same timestamp)
5. Release joystick to center — after ~50ms, observe `noteOff` in MIDI monitor and note stops sounding

---
*Phase: 04-per-voice-trigger-sources-and-random-gate*
*Plan: 01*
*Completed: 2026-02-22*
