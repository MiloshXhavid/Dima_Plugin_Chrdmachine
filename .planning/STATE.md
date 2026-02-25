# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-24)

**Core value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper, gamepad control — no competitor provides this as a unified instrument.
**Current focus:** v1.1 — Phase 08: Post-v1.0 Patch Verification

## Current Position

Phase: 08 of 11 (Post-v1.0 Patch Verification)
Plan: 1 of TBD
Status: In progress
Last activity: 2026-02-24 — Plan 08-01 complete (PATCH-01 + PATCH-04 source fixes)

```
v1.0 MVP    [██████████] SHIPPED 2026-02-23
v1.1 Polish [█░░░░░░░░░] 5% (1/TBD plans)
  Phase 08  [█░░░░░░░░░]   Patch Verification    In progress (1 plan done)
  Phase 09  [░░░░░░░░░░]   MIDI Panic + Mute     Not started
  Phase 10  [░░░░░░░░░░]   Quantize Infra        Not started
  Phase 11  [░░░░░░░░░░]   UI Polish + Installer Not started
```

## Performance Metrics

**v1.0 Velocity:**
- Total plans completed: 17
- Total phases: 7
- Avg plans/phase: 2.4

**v1.1 Velocity:**
- Plans completed: 0
- Phases started: 0

*Updated after each plan completion*

## Accumulated Context

### Decisions

See .planning/PROJECT.md Key Decisions table — full log with outcomes.

Recent decisions affecting v1.1:
- **NEVER send CC121 in panic sweep** — downstream VST3 instruments (Kontakt, Waves CLA-76) map CC121 to plugin parameters via JUCE VST3 wrapper; use CC64=0 + CC120 + CC123 only
- **Panic is one-shot, not a mute toggle** — press sends 48 CC events (16ch × CC64=0+CC120+CC123), button immediately returns to pressable; no persistent output-blocking state
- **Post-record quantize uses pendingQuantize_ flag** — playbackStore_[] has no mutex; audio thread reads it every block; flag set on message thread, serviced in LooperEngine::process() on audio thread
- **std::fmod(quantized, loopLen) required** — rounding near loop end produces beatPosition == loopLen; fmod prevents silent miss or double-trigger at wrap
- **Single 30 Hz timer only** — no second timer for animations; panic flash and progress bar driven from existing startTimerHz(30) in PluginEditor via phase counters
- **CC64=127 on voice channel only (08-01)** — Each injection uses ch0+1 (voice's own MIDI channel), not filterMidiCh; sustain must be per-voice for downstream synths on channels 1-4
- **timerCallback R3 sync untouched (08-01)** — Lines 1484-1502 sync panicBtn_ to proc_.isMidiMuted() for R3 gamepad; pre-existing functionality deferred to Phase 09; PATCH-04 only fixes UI button click wiring

### Pending Todos

None.

### Blockers/Concerns

- Phase 10 quantize implementation requires Catch2 tests for snapToGrid wrap edge case BEFORE integrating into LooperEngine — do not skip
- Phase 10 QUANTIZE button disabled only while **recording** (enabled during playback — user can preview toggle mid-loop; SC3 updated from discussion)
- Phase 11 progress bar must NOT use juce::ProgressBar — runs its own internal timer; use custom component repainted from editor timerCallback

## Session Continuity

Last session: 2026-02-25
Stopped at: Phase 10 context gathered (10-CONTEXT.md written) — ready for /gsd:plan-phase 10 once Phase 09 completes
Resume file: None
