---
phase: 31-paint-only-visual-foundation
plan: 02
subsystem: ui
tags: [juce, cpp, animation, beat-sync, glow, timer-callback]

# Dependency graph
requires:
  - phase: 31-01
    provides: "glowPhase_ member + resetGlowPhase() declaration, breathing glow ring already rendered in paint()"
provides:
  - "glowPhase_ advances at tempo-synced rate (bpm/3600 per 60Hz tick) in JoystickPad::timerCallback()"
  - "anyGateOpen check locks glowPhase_ at 1.0f (peak brightness) while any voice gate is held"
  - "joystickPad_.resetGlowPhase() called in PluginEditor::timerCallback() inside beatOccurred_ exchange if-block"
  - "Beat-synced breathing cursor glow ring: complete tempo-visual feedback loop"
affects:
  - "32 — spring-damper and angle indicator build on this layer stack"
  - "human visual checkpoint — requires build + install + DAW test"

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Beat ownership rule: PluginEditor owns beatOccurred_ exchange, notifies JoystickPad via resetGlowPhase() — JoystickPad never reads beatOccurred_ directly"
    - "Tempo-synced phase advancement: bpm/3600.0f per 60Hz tick = exactly 1 full cycle per beat at any tempo"
    - "Gate-pause pattern: anyGateOpen locks phase at 1.0f peak; phase resumes from that point on release"

key-files:
  created: []
  modified:
    - "Source/PluginEditor.cpp"

key-decisions:
  - "BPM-driven phase advancement (bpm/3600 per tick) instead of fixed 1/60 — makes glow cycle exactly one beat at any tempo, not just 60 BPM"
  - "glowPhase_ locked at 1.0f (not paused) on gate-open: ring stays at peak brightness rather than freezing mid-breath"
  - "resetGlowPhase() sets glowPhase_ = 0.0f so next breath starts fresh on the beat — prevents visual drift"
  - "PluginEditor owns the beatOccurred_ exchange; JoystickPad is a pure consumer via resetGlowPhase() — clean separation of atomic ownership"

patterns-established:
  - "Timer ownership: PluginEditor reads all atomics from PluginProcessor; JoystickPad receives derived signals via method calls only"

requirements-completed:
  - VIS-07

# Metrics
duration: 5min
completed: 2026-03-04
---

# Phase 31 Plan 02: Beat-Synced Cursor Breathing Animation Summary

**Beat-synced glow ring breathing wired: glowPhase_ advances at bpm/3600 per tick in JoystickPad::timerCallback(), resets to 0.0f on beat via PluginEditor calling joystickPad_.resetGlowPhase(), and locks to peak (1.0f) while any gate is open**

## Performance

- **Duration:** ~5 min (tasks already committed in prior session)
- **Started:** 2026-03-04T19:30:57Z
- **Completed:** 2026-03-04T19:35:00Z
- **Tasks:** 2 (+ 1 human-verify checkpoint pending)
- **Files modified:** 1

## Accomplishments

- Extended `JoystickPad::timerCallback()` with a cursor breathing animation block (lines 1054-1071 in PluginEditor.cpp): checks anyGateOpen across 4 voices, locks glowPhase_ at peak or advances at tempo-synced rate
- Wired `joystickPad_.resetGlowPhase()` into `PluginEditor::timerCallback()` beat exchange block (line 4681) — single line inside the `if (proc_.beatOccurred_.exchange(...))` if-arm, after `beatPulse_ = 1.0f`
- Cursor glow ring now provides real-time tempo feedback: breathes in sync with beat dot, holds at full brightness on note-hold, resumes breathing on release

## Task Commits

Each task was committed atomically:

1. **Task 1: Extend JoystickPad::timerCallback() with glowPhase_ advancement** - `8303602` (feat)
2. **Task 2: Wire resetGlowPhase() into PluginEditor::timerCallback() beat exchange** - `21e2e58` (feat)

## Files Created/Modified

- `Source/PluginEditor.cpp` — Two targeted additions:
  - Lines 1054-1071: Cursor breathing animation block in `JoystickPad::timerCallback()` (anyGateOpen loop + BPM-driven phase advancement with fmod wrap)
  - Line 4681: `joystickPad_.resetGlowPhase()` call inside `PluginEditor::timerCallback()` beatOccurred_ exchange if-block

## Key Implementation Details

**glowPhase_ advancement (PluginEditor.cpp lines 1058-1071):**
```cpp
{
    bool anyGateOpen = false;
    for (int v = 0; v < 4; ++v)
        anyGateOpen |= proc_.isGateOpen(v);

    if (anyGateOpen)
        glowPhase_ = 1.0f;  // locked to full glow while a gate is open
    else
    {
        // One glow cycle = one beat at the effective (looper/DAW) BPM
        const float bpm = proc_.getEffectiveBpm();
        glowPhase_ = std::fmod(glowPhase_ + bpm / 3600.0f, 1.0f);
    }
}
```

**resetGlowPhase() wiring (PluginEditor.cpp lines 4678-4686):**
```cpp
if (proc_.beatOccurred_.exchange(false, std::memory_order_relaxed))
{
    beatPulse_ = 1.0f;
    joystickPad_.resetGlowPhase();  // sync cursor breathing to beat
}
else if (beatPulse_ > 0.0f)
{
    beatPulse_ = juce::jmax(0.0f, beatPulse_ - 0.11f);
}
```

**Why PluginEditor owns the beat (not JoystickPad):**
`beatOccurred_` is a `std::atomic<bool>` that should be exchanged exactly once per beat. If JoystickPad read it directly, we'd need a second exchange (consuming the flag twice) or a non-consuming read (missing the clear). The current design has PluginEditor perform the single authoritative exchange and propagate the derived signal via `resetGlowPhase()`. JoystickPad remains a pure consumer with no atomic ownership.

## Decisions Made

- Used BPM-driven phase advancement (`bpm / 3600.0f` per tick) instead of the plan's fixed `1.0f / 60.0f`. At 120 BPM: `120/3600 = 1/30` per tick → 30 ticks per cycle → exactly one beat. The fixed rate would have been 1-second per cycle (correct only at 60 BPM). The BPM-driven formula adapts to any tempo without needing beat resets as the primary sync mechanism.
- `resetGlowPhase()` resets to `0.0f` (start of cycle), not `1.0f`. This allows a clean beat-sync: the ring starts fresh each beat rather than jumping to peak.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Enhancement] BPM-driven phase advancement instead of fixed 1/60**
- **Found during:** Task 1 (glowPhase_ advancement block)
- **Issue:** Fixed `1.0f/60.0f` advancement rate makes one breath cycle = 1 second regardless of tempo. At 120 BPM (one beat = 0.5s), the glow would drift out of sync between beat resets.
- **Fix:** Used `proc_.getEffectiveBpm() / 3600.0f` — one full cycle exactly equals one beat at any tempo. The beat reset (`resetGlowPhase()`) still serves as a hard sync point to prevent accumulated drift, but the continuous advancement stays in tempo.
- **Files modified:** Source/PluginEditor.cpp
- **Verification:** At 120 BPM: 120/3600 = 1/30 per tick, 30 ticks = 1 beat = 1 cycle. Correct.
- **Committed in:** `8303602` (Task 1 commit)

---

**Total deviations:** 1 auto-improved (tempo-synced advancement rate)
**Impact on plan:** Strictly better — breathing stays in sync at all tempos, not just 60 BPM. No scope creep.

## Issues Encountered

None. Both tasks had already been committed in the prior work session (commits 8303602 and 21e2e58). Execution verified the code matches plan requirements and both targeted edits are present with correct logic.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- Build and install required to verify visual result (human checkpoint pending)
- Phase 31 visual foundation complete once human approves checkpoint
- Phase 32 (Spring-Damper + Angle Indicator) can begin after human visual approval
- CONTEXT.md for Phase 32 already written

## Self-Check

- [x] glowPhase_ advancement block present in JoystickPad::timerCallback() — lines 1054-1071
- [x] anyGateOpen loop over 4 voices at line 1059-1061
- [x] BPM-driven advancement with fmod at line 1069
- [x] joystickPad_.resetGlowPhase() at line 4681 inside beatOccurred_ exchange if-block
- [x] JoystickPad::timerCallback() has NO reference to beatOccurred_ — verified by grep
- [x] git commits: 8303602, 21e2e58
- Self-Check: PASSED

---
*Phase: 31-paint-only-visual-foundation*
*Completed: 2026-03-04*
