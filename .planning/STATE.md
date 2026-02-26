# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-26)

**Core value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper with trigger quantization, gamepad control — no competitor provides this as a unified instrument.
**Current focus:** v1.4 LFO + Clock — Phase 14 Plan 02 complete

## Current Position

Phase: 14 of 15 (LFO UI + Beat Clock)
Plan: 2 of 3 (Plan 02 complete)
Status: In progress
Last activity: 2026-02-26 — Phase 14 Plan 02 complete (1120px editor, LFO X/Y panels with full APVTS attachments, sync-toggle Rate swap, beat pulse dot)

```
v1.0 MVP    [██████████] SHIPPED 2026-02-23
v1.3 Polish [██████████] SHIPPED 2026-02-25
  Phase 09  [██████████]   MIDI Panic            Complete (2/2 plans)
  Phase 10  [██████████]   Trigger Quantization  Complete (5/5 plans)
  Phase 11  [██████████]   UI Polish + Installer Complete (4/4 plans)
v1.4 LFO    [████████░░] ~60% — Phase 14 in progress
  Phase 12  [██████████]   LFO Engine Core       Complete (2/2 plans done)
  Phase 13  [██████████]   processBlock + APVTS  Complete (1/1 plans done)
  Phase 14  [██████░░░░]   LFO UI + Beat Clock   In progress (2/3 plans done)
  Phase 15  [░░░░░░░░░░]   Distribution          Not started
  Phase 16  [░░░░░░░░░░]   Gamepad Preset Ctrl   Not started
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

### Pending Todos

None.

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-02-26
Stopped at: Completed 14-02-PLAN.md (1120px editor, LFO X/Y panels, sync toggle, beat pulse dot — commits c33fc7a + 6968771)
Next step: Phase 14 Plan 03 (JoystickPad LFO tracking dot via modulatedJoyX_/Y_, human verify)
