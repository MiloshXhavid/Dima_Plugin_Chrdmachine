# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-24)

**Core value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper, gamepad control — no competitor provides this as a unified instrument.
**Current focus:** v1.1 — Phase 11: UI Polish + Installer

## Current Position

Phase: 11 of 11 (UI Polish + Installer)
Plan: 1 of TBD — Plan 11-01 complete
Status: Phase 11 in progress — section panels (LOOPER/FILTER MOD/GAMEPAD) added, QUANTIZE TRIGGER label added, qDropW widened to 58px
Last activity: 2026-02-25 — Plan 11-01 complete (section panels with knockout floating labels, paint/resized changes, build clean)

```
v1.0 MVP    [██████████] SHIPPED 2026-02-23
v1.1 Polish [█████░░░░░] 36% (7/TBD plans)
  Phase 08  [██████████]   Patch Verification    Complete (1 plan done)
  Phase 09  [██████████]   MIDI Panic + Mute     Complete (2 plans done)
  Phase 10  [██████████]   Quantize Infra        Complete (5/5 plans done)
  Phase 11  [█░░░░░░░░░]   UI Polish + Installer In progress (1 plan done)
```

## Performance Metrics

**v1.0 Velocity:**
- Total plans completed: 17
- Total phases: 7
- Avg plans/phase: 2.4

**v1.1 Velocity:**
- Plans completed: 7
- Phases started: 3
- Phase 09 duration: ~23 min total (2 plans: 09-01 impl ~15 min + 09-02 verify ~8 min)
- Phase 10 duration: ~5 plans completed across multiple sessions

*Updated after each plan completion*

## Accumulated Context

### Decisions

See .planning/PROJECT.md Key Decisions table — full log with outcomes.

Recent decisions affecting v1.1:
- **NEVER send CC121 in panic sweep** — downstream VST3 instruments (Kontakt, Waves CLA-76) map CC121 to plugin parameters via JUCE VST3 wrapper; use CC64=0 + CC120 + CC123 only
- **Panic is one-shot, not a mute toggle** — press sends 48 CC events (16ch x CC64=0+CC120+CC123), button immediately returns to pressable; no persistent output-blocking state
- **Post-record quantize uses pendingQuantize_ flag** — playbackStore_[] has no mutex; audio thread reads it every block; flag set on message thread, serviced in LooperEngine::process() on audio thread
- **std::fmod(quantized, loopLen) required** — rounding near loop end produces beatPosition == loopLen; fmod prevents silent miss or double-trigger at wrap
- **Single 30 Hz timer only** — no second timer for animations; panic flash and progress bar driven from existing startTimerHz(30) in PluginEditor via phase counters
- **CC64=127 on voice channel only (08-01)** — Each injection uses ch0+1 (voice's own MIDI channel), not filterMidiCh; sustain must be per-voice for downstream synths on channels 1-4
- **Panic sweep is flat for(ch=1..16) loop (09-01)** — no channel deduplication, always hits all 16 channels; old killCh/sent[]/fCh approach removed
- **R3 gamepad calls triggerPanic() directly in processBlock (09-01)** — audio-thread safe, only writes atomics; replaces silent consume
- **snapToGrid declared non-static in LooperEngine.h (10-01)** — required for Catch2 linkage without test accessor or friend pattern; single source of truth, no duplication
- **Tie-breaking uses (beatPos <= midpoint) ? lower : upper (10-01)** — ties snap to EARLIER grid point; RESEARCH.md used strict less-than (wrong: ties→upper); CONTEXT.md locked decision overrides research example
- **Pre-existing TC 4/5/6/10/11 failures deferred (10-01)** — hasContent() returns false in 5 tests; confirmed pre-existing before 10-01 changes; investigate before 10-03
- **scratchDedup_ is a class member not a local array (10-03)** — applyQuantizeToStore() would allocate ~49KB on stack; matches existing scratchNew_/scratchMerged_ pattern; avoids stack overflow on MSVC debug builds
- **applyQuantizeToStore() skips non-Gate events (10-03)** — Joystick and Filter events are never snapped; only Gate-type events modified to preserve joystick playback continuity
- **hasOriginals_ and quantizeActive_ reset on deleteLoop() (10-03)** — prevents stale revert state after loop deletion; quantize state always consistent with playbackStore_ content
- **Post mode auto-re-applies after finaliseRecording() (10-03)** — pendingQuantize_ set to true in finaliseRecording() when quantizeMode_ == 2; new overdubs auto-quantized on next process() call
- **Quantize UI radio group ID=1 (10-04)** — no other radio groups existed in PluginEditor; ID=1 used for Off/Live/Post TextButtons; timerCallback() syncs toggle states + disables all four controls during recording
- **Phase 10 complete — all quantize features verified in DAW (10-05)** — build clean, TC 12 233/233 assertions pass, human DAW checklist fully approved
- **drawSectionPanel as paint() lambda (11-01)** — matches existing drawFilterGroup pattern; knockout floating label uses fillRect behind text to erase border line
- **filterModPanelBounds_ = union of 6 controls (11-01)** — covers filterModBtn_, filterRecBtn_, filterYModeBox_, filterXModeBox_, gateTimeSlider_, thresholdSlider_ + expanded(4,6)
- **qDropW widened 48→58px (11-01)** — "1/32" text was truncated at 48px; 58px confirmed sufficient

### Pending Todos

None.

### Blockers/Concerns

- (RESOLVED by 10-01) Phase 10 quantize implementation required Catch2 tests for snapToGrid wrap edge case BEFORE integrating into LooperEngine — TC 12 now passes
- Phase 10 QUANTIZE button disabled only while **recording** (enabled during playback — user can preview toggle mid-loop; SC3 updated from discussion)
- Phase 11 progress bar must NOT use juce::ProgressBar — runs its own internal timer; use custom component repainted from editor timerCallback

## Session Continuity

Last session: 2026-02-25 (11-01 complete)
Stopped at: Completed 11-01-PLAN.md — section panels + QUANTIZE TRIGGER label
Resume file: None
