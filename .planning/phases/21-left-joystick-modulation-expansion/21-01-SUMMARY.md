---
phase: 21-left-joystick-modulation-expansion
plan: 01
subsystem: audio-processor
tags: [juce, apvts, lfo, midi, atomic, cpp17]

# Dependency graph
requires:
  - phase: 20-rnd-trigger-extensions
    provides: unified gateLength param (0.0–1.0) registered in APVTS
  - phase: 14-lfo-ui-and-beat-clock
    provides: LFO engine (lfoXRate/lfoYRate/lfoXSync/lfoYSync/lfoXPhase/lfoYPhase/lfoXLevel/lfoYLevel params)
provides:
  - filterXMode extended to 6-item choice (0=Cutoff CC74, 1=VCF LFO CC12, 2=LFO-X Freq, 3=LFO-X Phase, 4=LFO-X Level, 5=Gate Length)
  - filterYMode extended to 6-item choice (0=Resonance CC71, 1=LFO Rate CC76, 2=LFO-Y Freq, 3=LFO-Y Phase, 4=LFO-Y Level, 5=Gate Length)
  - lfoXSubdivMult_ and lfoYSubdivMult_ std::atomic<float> members (initialized 1.0f)
  - processBlock dispatch: left-stick modes 2-5 write directly to APVTS raw params
  - Sync LFO Freq mode: std::pow(4.0f, stick*atten) multiplier applied to subdivBeats
affects:
  - 21-02 (UI wiring for 6-item filterXMode/filterYMode combo boxes)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "APVTS raw param write from audio thread: if (auto* raw = apvts.getRawParameterValue(id)) raw->store(val, std::memory_order_relaxed)"
    - "Subdivision multiplier via atomic float: lfoXSubdivMult_.store(std::pow(4.0f, stick*atten)); xp.subdivBeats *= lfoXSubdivMult_.load()"
    - "CC emit guard: if (xMode <= 1) / if (yMode <= 1) prevents CC output for non-CC modes"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "filterXMode index 2 repurposed from Mod Wheel (CC1) to LFO-X Freq — saved presets at index 2 load as LFO-X Freq (accepted behavior)"
  - "Sync LFO Freq mode uses exponential mapping: std::pow(4.0f, stick*atten) — stick left = 0.25x, center = 1.0x, stick right = 4.0x subdivision"
  - "LFO/Gate dispatch gated on stickUpdated (not baseChanged) — avoids continuous APVTS churn when stick is idle"
  - "lfoXSubdivMult_ reset to 1.0f each block when xMode != 2 — ensures clean state when switching away from LFO Freq target"

patterns-established:
  - "Audio-thread APVTS raw write pattern: getRawParameterValue(id)->store(val) — no notifications, safe from audio thread"
  - "Mode-indexed dispatch: CC targets (0–1) vs APVTS targets (2–5) split at xMode <= 1 guard"

requirements-completed: [LJOY-01, LJOY-02, LJOY-04]

# Metrics
duration: 25min
completed: 2026-03-01
---

# Phase 21 Plan 01: Left Joystick Modulation Expansion — Backend Summary

**6-item filterXMode/filterYMode APVTS choice params with processBlock dispatch routing left-stick to LFO Freq/Phase/Level and Gate Length via direct atomic raw-param writes**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-03-01T00:00:00Z
- **Completed:** 2026-03-01T00:25:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Extended filterXMode from 3 choices to 6 (Cutoff CC74, VCF LFO CC12, LFO-X Freq, LFO-X Phase, LFO-X Level, Gate Length)
- Extended filterYMode from 2 choices to 6 (Resonance CC71, LFO Rate CC76, LFO-Y Freq, LFO-Y Phase, LFO-Y Level, Gate Length)
- Added lfoXSubdivMult_ / lfoYSubdivMult_ atomic float members for cross-block subdivision scaling
- Wired processBlock filter dispatch so modes 2-5 write directly to APVTS raw params (gated by stickUpdated)
- CC emit blocks guarded to modes 0-1 only — no MIDI CC fired for LFO/Gate targets
- Sync-mode LFO Freq target applies exponential subdivision multiplier (0.25x–4x) to xp.subdivBeats / yp.subdivBeats
- Plugin compiles and links with 0 compiler errors, 0 new warnings

## Task Commits

Each task was committed atomically:

1. **Task 1: Extend APVTS choice params and add atomic members to header** - `c5812b4` (feat)
2. **Task 2: Extend processBlock filter dispatch and wire subdivision multiplier into LFO block** - `5c80585` (feat)

## Files Created/Modified

- `Source/PluginProcessor.h` - Added lfoXSubdivMult_ and lfoYSubdivMult_ std::atomic<float> members (initialized 1.0f) after modulatedJoyY_
- `Source/PluginProcessor.cpp` - Extended filterXMode/filterYMode StringArrays to 6 items; updated filter dispatch (ccXnum simplification, CC guards, LFO/Gate dispatch block, subdivBeats multiplier wiring)

## Decisions Made

- **Index 2 repurposed (X axis):** The old "Mod Wheel (CC1)" at index 2 for filterXMode is replaced by "LFO-X Freq". Any DAW sessions with X at index 2 will load as LFO-X Freq. Accepted behavior per plan note.
- **Exponential subdivision scale:** Sync LFO Freq uses `std::pow(4.0f, stick * atten)` — gives musically useful range (0.25x to 4.0x) with center = unity.
- **stickUpdated gate for APVTS writes:** LFO/Gate dispatch is inside `if (stickUpdated)` not `if (stickUpdated || baseChanged)` — base knob turning while in LFO mode has no effect on LFO params (intentional; base knob is a CC offset, irrelevant for LFO targets).
- **subdivMult reset when mode != 2:** Each block where xMode != 2, lfoXSubdivMult_ is reset to 1.0f — ensures clean handoff when user switches away from LFO Freq target without re-entering that branch.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Build post-step "Permission denied" when copying VST3 to `C:\Program Files\Common Files\VST3` — pre-existing UAC issue documented in project memory (fix-install.ps1). Not related to this plan's changes. Compilation and linking both succeeded cleanly.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- APVTS now has 6-item filterXMode and filterYMode — plan 21-02 (UI wiring) can proceed immediately
- The combo box UI in PluginEditor needs to update its item list to match the 6-item layout
- No blockers

---
*Phase: 21-left-joystick-modulation-expansion*
*Completed: 2026-03-01*

## Self-Check: PASSED

- FOUND: `.planning/phases/21-left-joystick-modulation-expansion/21-01-SUMMARY.md`
- FOUND: `Source/PluginProcessor.h` (lfoXSubdivMult_ + lfoYSubdivMult_ members)
- FOUND: `Source/PluginProcessor.cpp` (6-item modes, dispatch, subdivMult wiring)
- FOUND: commit `c5812b4` (Task 1 — APVTS extension + atomics)
- FOUND: commit `5c80585` (Task 2 — processBlock dispatch + subdivMult wiring)
