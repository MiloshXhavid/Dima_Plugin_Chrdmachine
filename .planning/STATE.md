# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-26)

**Core value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper with trigger quantization, gamepad control — no competitor provides this as a unified instrument.
**Current focus:** v1.4 Distribution — Phase 16 in progress (installer built, awaiting smoke test)

## Current Position

Phase: 16 of 16 (Distribution)
Plan: 1 of 2 (Plan 01 in progress — Task 1 complete, Task 2 checkpoint awaiting human smoke test)
Status: In Progress
Last activity: 2026-02-26 — Phase 16 Plan 01 Task 1 complete (.iss updated to v1.4, fresh installer compiled)

```
v1.0 MVP    [██████████] SHIPPED 2026-02-23
v1.3 Polish [██████████] SHIPPED 2026-02-25
  Phase 09  [██████████]   MIDI Panic            Complete (2/2 plans)
  Phase 10  [██████████]   Trigger Quantization  Complete (5/5 plans)
  Phase 11  [██████████]   UI Polish + Installer Complete (4/4 plans)
v1.4 LFO    [██████████] Complete — Phase 14 done
  Phase 12  [██████████]   LFO Engine Core       Complete (2/2 plans done)
  Phase 13  [██████████]   processBlock + APVTS  Complete (1/1 plans done)
  Phase 14  [██████████]   LFO UI + Beat Clock   Complete (3/3 plans done)
v1.5 Preset [██████████] Complete — Phase 15 done
  Phase 15  [██████████]   Gamepad Preset Ctrl   Complete (2/2 plans done)
v1.4 Dist   [█░░░░░░░░░] In Progress
  Phase 16  [█░░░░░░░░░]   Distribution          In Progress (1/2 plans — checkpoint)
```

## Accumulated Context

### Decisions

See .planning/PROJECT.md Key Decisions table — full log with outcomes.

Recent decisions affecting v1.4:
- LFO output applied as additive offset inside buildChordParams() local scope — never written to joystickX/Y atomics (prevents looper conflict)
- S&H/Random waveforms use project LCG (`seed * 1664525u + 1013904223u`), not std::rand() — audio-thread safe on MSVC
- Sync mode derives phase from ppqPos when DAW is playing; uses sample counter only when DAW is stopped — prevents drift
- No second timer for LFO UI — all display reads in existing 30 Hz timerCallback
- LfoEngine is fully standalone (no JUCE/APVTS/LooperEngine headers) — tested in isolation before wiring into processBlock
- nextLcg() maps via static_cast<int32_t>(rng_) / 0x7FFFFFFF for bipolar [-1,+1] output (differs from TriggerSystem::nextRandom() which uses rng_>>1 for unipolar [0,1))
- evaluateWaveform() declared non-const (not const + const_cast) — Random waveform calls nextLcg() which mutates rng_
- Triangle test values match actual implementation (single-peak: trough at phi=0, peak at phi=0.5) — plan spec described a double-frequency shape that does not match LfoEngine.cpp formula
- LfoEngine.cpp requires #include <algorithm> for std::min; JUCE headers masked this in plugin build, standalone test build exposed it (auto-fixed)
- Phase 13: chordP declared non-const so LFO can write additive offset to joystickX/Y before pitch compute
- Phase 13: Skew factor 0.2306f hard-coded for log-scale rate range [0.01,20] Hz midpoint=1Hz — JUCE 8.0.4 lacks getSkewFactorFromMidPoint()
- Phase 13: Joystick-source voice retriggering with LFO active is intentional — post-LFO chordP drives TriggerSystem deltaX/deltaY
- Phase 14-01: beatOccurred_ is a sticky bool (audio writes true, UI timer reads + clears) — simpler than a counter for 30 Hz polling
- Phase 14-01: prevBeatCount_ promoted from static local to private member — static locals cannot be reset on transport events
- Phase 14-01: effectiveBpm_ reused for free-tempo beat detection — avoids duplicate APVTS read
- Phase 14-02: Left column fixed at kLeftColW=448 — previous colW/2-4 would have over-widened at 1120px
- Phase 14-02: SliderParameterAttachment swap on Sync toggle (reset + recreate) — preserves APVTS value traceability vs setRange() alone
- Phase 14-02: Hidden ToggleButton pattern for LED — zero-alpha button carries ButtonAttachment; mouseDown routes LED hit-area clicks to it
- Phase 14-02: beatPulse_ drawn adjacent to bpmDisplayLabel_.getRight() in paint() — avoids modifying the label widget
- Phase 14-03: Beat dot moved to center overlay on randomFreeTempoKnob_ face — more intuitive than text-adjacent dot
- Phase 14-03: LFO panels widened 130→170px, slider label offset 30→34px — ensures label legibility
- Phase 14-03: Footer redesigned as full-width 54px strip — eliminates column height mismatch overlap
- Phase 14-03: FREE BPM knob relocated to looper section; arp block relocated to right column below pads

Recent decisions affecting v1.4 distribution:
- Phase 16-01: .iss bundle name updated from ChordJoystick MK2.vst3 to DIMEA CHORD JOYSTICK MK2.vst3 — the build output name from CMakeLists PRODUCT_NAME
- Phase 16-01: All four .iss locations updated (Source, DestDir, UninstallDelete, DefaultDirName) to prevent stale-bundle packaging
- Phase 16-01: AppVersion bumped to 1.4; AppComments updated with LFO/beat-clock/preset-control feature summary

Recent decisions affecting v1.5:
- Phase 15-01: SDL_CONTROLLER_BUTTON_GUIDE maps to Option/PS/Guide on all controller types (DualSense, DS4, Xbox) — correct SDL2 constant for the intended button
- Phase 15-01: D-pad Up/Down PC delta fires on falling edge (button-up) — rising-edge would fire immediately on press before user has released, causing doubled PCs
- Phase 15-01: One-frame lockout (optionFrameFired_) prevents D-pad Up/Down from registering BPM delta in the same 16ms frame Option fires
- Phase 15-01: D-pad Left/Right always rising-edge regardless of preset-scroll mode — looper toggle semantics are mode-independent
- Phase 15-02: programCounter_ is audio-thread-only int (no atomic) — never read from message thread
- Phase 15-02: juce::MidiMessage::programChange takes 1-based channel + 0-based program; filterMidiCh stores 1-based directly, no conversion needed
- Phase 15-02: Boundary clamp uses jlimit + equality check — if newProgram == programCounter_ after clamp, skip send (pressing at 0 or 127 does nothing)
- Phase 15-02: OPTION indicator uses text suffix pattern (base + " | OPTION") — no blink/animation, satisfies subtle secondary-weight constraint

### Pending Todos

None.

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-02-26
Stopped at: Checkpoint — 16-01 Task 2 (smoke test) — installer/Output/DimaChordJoystickMK2-Setup.exe ready at 14:11; needs DAW smoke test
Next step: Install fresh binary, verify LFO + OPTION indicator in DAW, then type "approved" to resume 16-01 Task 2 checkpoint
