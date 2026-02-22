---
phase: 03-core-midi-output-and-note-off-guarantee
plan: 01
subsystem: midi
tags: [juce, vst3, midi, note-off, gate, bypass, trigger-system, plugin-processor, plugin-editor]

# Dependency graph
requires:
  - phase: 02-core-engine-validation
    provides: TriggerSystem, PluginProcessor, PluginEditor already wired — gap-fill only
provides:
  - TriggerSystem::resetAllGates() — atomic clear of all 4 voices, allTrigger_, joystickTrig_
  - PluginProcessor::processBlockBypassed() — flushes note-offs for all open voices into MidiBuffer
  - PluginProcessor::releaseResources() — calls resetAllGates() to prevent zombie notes on shutdown
  - noteOff in processBlock uses explicit (uint8_t)0 velocity (3-arg form)
  - TouchPlate gate LEDs use Clr::gateOn (green) when gate is open
  - channelConflictLabel_ in PluginEditor with 30 Hz conflict detection, no per-tick repaint storms
affects:
  - 03-02-DAW-verification
  - Phase 04 TriggerSystem expansion
  - Phase 05 LooperEngine hardening

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "processBlockBypassed() for bypass-safe note-off flushing"
    - "resetAllGates() as shared cleanup primitive called from multiple exit paths"
    - "channelConflictShown_ cache prevents per-tick setVisible() calls"

key-files:
  created: []
  modified:
    - Source/TriggerSystem.h
    - Source/TriggerSystem.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "resetAllGates() is the single cleanup primitive for all exit paths (bypass + shutdown) — avoids duplicated atomic clearing logic"
  - "processBlockBypassed() writes noteOff before calling resetAllGates() so the host sees the messages"
  - "channelConflictShown_ cache prevents calling setVisible() every 30 Hz tick when state is stable"
  - "noteOff uses explicit (uint8_t)0 velocity to avoid MIDI interpretation ambiguity on some hosts"
  - "Gate LED uses Clr::gateOn (0xFF4CAF50 green) not Clr::highlight (red/pink) — corrects semantic mismatch"

patterns-established:
  - "Bypass-safe note-off: processBlockBypassed() flushes MidiBuffer then resets atomics"
  - "Shutdown-safe state: releaseResources() resets TriggerSystem atomics — next prepareToPlay starts clean"
  - "UI conflict detection: compare raw parameter values at timer rate, cache state, toggle visibility only on change"

requirements-completed: [MIDI-01, MIDI-02, MIDI-03]

# Metrics
duration: 15min
completed: 2026-02-22
---

# Phase 03 Plan 01: Note-Off Guarantee and Gate LED Fix Summary

**Zombie-note prevention via resetAllGates() + processBlockBypassed(), green gate LEDs on TouchPlate pads, and MIDI channel conflict warning in the editor**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-02-22T00:00:00Z
- **Completed:** 2026-02-22T00:15:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- `TriggerSystem::resetAllGates()` clears all 4 voice atomics (gateOpen_, activePitch_, padPressed_, padJustFired_) plus allTrigger_ and joystickTrig_ — single cleanup primitive
- `PluginProcessor::processBlockBypassed()` flushes note-off for all active voices into MidiBuffer then calls resetAllGates() — no zombie notes on plugin bypass
- `PluginProcessor::releaseResources()` no longer empty — calls resetAllGates() so next prepareToPlay starts clean
- noteOff in processBlock uses 3-arg form `noteOff(ch, pitch, (uint8_t)0)` — explicit velocity 0
- TouchPlate gate LEDs glow green (`Clr::gateOn` = `0xFF4CAF50`) when gate is open, not red/pink
- Channel conflict label hidden by default, shown when any two voiceCh values match, toggled without per-tick repaint storms
- 15/15 Catch2 tests still pass after all changes

## Task Commits

Each task was committed atomically:

1. **Task 1: Add resetAllGates(), processBlockBypassed(), fix releaseResources() + noteOff velocity** - `437f0e3` (feat)
2. **Task 2: Fix gate LED color and add MIDI channel conflict warning** - `6f7e1d5` (feat)

## Files Created/Modified

- `Source/TriggerSystem.h` - Added `resetAllGates()` public method declaration
- `Source/TriggerSystem.cpp` - Implemented `resetAllGates()` clearing all 4 voice atomics + allTrigger_ + joystickTrig_
- `Source/PluginProcessor.h` - Added `processBlockBypassed()` override declaration
- `Source/PluginProcessor.cpp` - releaseResources() calls resetAllGates(); processBlockBypassed() implemented; noteOff uses explicit (uint8_t)0
- `Source/PluginEditor.h` - Added `channelConflictLabel_` and `channelConflictShown_` to private section
- `Source/PluginEditor.cpp` - TouchPlate uses Clr::gateOn; channelConflictLabel_ initialized, positioned, and toggled in timerCallback

## Decisions Made

- `resetAllGates()` is the single cleanup primitive for all exit paths rather than inline clearing at each call site — clearer intent and easier to maintain.
- `processBlockBypassed()` writes noteOff messages into MidiBuffer before calling resetAllGates() — order matters so the host sees the messages.
- `channelConflictShown_` cache prevents `setVisible()` being called every 30 Hz tick when state is stable — avoids unnecessary repaint storms.
- noteOff uses explicit `(uint8_t)0` velocity — some hosts interpret 2-arg noteOff differently, 3-arg form is unambiguous.
- Gate LED corrected to `Clr::gateOn` (green) instead of `Clr::highlight` (red/pink) — the existing color was a semantic mismatch; green is universally understood as "open/active" for gate indicators.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Build exit code 1 is the known shell issue (`/c/Users/Milosh: Is a directory`) documented in MEMORY.md. The build and VST3 installation both succeeded — confirmed by presence of `ChordJoystick_VST3.vcxproj -> ...ChordJoystick.vst3` and successful install to `C:\Program Files\Common Files/VST3/ChordJoystick.vst3` in the output.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 03 Plan 02 (DAW verification): VST3 is installed and ready for Reaper testing. Load plugin, press ROOT pad, verify green glow, verify note-off on release, verify bypass kills all notes.
- All 5 known note-off gaps from the research phase are now closed: releaseResources empty (fixed), processBlockBypassed missing (added), noteOff velocity ambiguous (fixed), gate LED color wrong (fixed), channel conflict undetected (fixed).
- Remaining blocker from STATE.md: Plugin crashes on Ableton Live 11 instantiation — not addressed in this plan (Ableton-specific COM threading issue).

---
*Phase: 03-core-midi-output-and-note-off-guarantee*
*Completed: 2026-02-22*
