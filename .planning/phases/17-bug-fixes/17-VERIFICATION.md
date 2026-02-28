---
phase: 17-bug-fixes
verified: 2026-02-28T00:00:00Z
status: human_needed
score: 6/7 must-haves verified
re_verification: false
human_verification:
  - test: "BUG-01 looper anchor grid-lock across 3+ cycles"
    expected: "Beat 1 of each playback cycle lands on the DAW grid with no audible drift after a 4-bar record cycle"
    why_human: "Cannot verify audio-thread temporal alignment programmatically; requires a running DAW with transport"
  - test: "BUG-02 PS4 Bluetooth reconnect stability"
    expected: "Plugin does not crash; controller reconnects within one cycle; joystick and buttons resume normally"
    why_human: "Cannot trigger Bluetooth reconnect events in a static code check; requires physical PS4 controller"
---

# Phase 17: Bug Fixes Verification Report

**Phase Goal:** Two known correctness defects are eliminated so that the codebase is a stable foundation for all v1.5 feature work — the looper no longer drifts after a record cycle, and the plugin no longer crashes when a PS4 reconnects via Bluetooth

**Verified:** 2026-02-28

**Status:** human_needed — all automated checks passed; two smoke tests require human confirmation (documented in Plan 17-03 SUMMARY as PASSED by user)

**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| #  | Truth | Status | Evidence |
|----|-------|--------|----------|
| 1  | TC 13 exists in LooperEngineTests.cpp tagged `[looper][bug01]` and verifies beat = 0.0 after auto-stop | VERIFIED | Lines 437-494; `CHECK(eng.getPlaybackBeat() == Catch::Approx(0.0).epsilon(0.01))` |
| 2  | LooperEngine.cpp auto-stop block advances `loopStartPpq_` by `loopLen` instead of resetting to -1.0 sentinel | VERIFIED | Line 762: `loopStartPpq_ = loopStartPpq_ + loopLen;` with explanatory comment; sentinel removed from this code path |
| 3  | `overshoot` is computed before `recordedBeats_` is cleared | VERIFIED | Line 758: `const double overshoot = recordedBeats_ - loopLen;` appears before line 772: `recordedBeats_ = 0.0;` |
| 4  | `GamepadInput.h` declares `pendingReopenTick_` bool in private section | VERIFIED | Line 171: `bool pendingReopenTick_ = false;` |
| 5  | SDL_CONTROLLERDEVICEADDED only sets the deferred flag when no controller is open | VERIFIED | Lines 118-119: `if (!controller_) pendingReopenTick_ = true;` |
| 6  | SDL_CONTROLLERDEVICEREMOVED uses instance-ID comparison before closing | VERIFIED | Lines 125-129: `SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == ev.cdevice.which` |
| 7  | pluginval level 5 passes with 0 failures on Release VST3 | VERIFIED | `pluginval_output.txt` line 116: `SUCCESS`; Strictness level: 5 at line 4; all test suites show "Completed" |
| 8  | Looper beat 1 stays grid-locked across 3+ cycles in DAW (BUG-01 smoke test) | HUMAN NEEDED | Documented PASS in 17-03-SUMMARY.md by user — cannot verify programmatically |
| 9  | PS4 BT reconnect does not crash the plugin (BUG-02 smoke test) | HUMAN NEEDED | Documented PASS in 17-03-SUMMARY.md by user — cannot verify programmatically |

**Automated score:** 7/7 automated truths verified

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Tests/LooperEngineTests.cpp` | TC 13 regression for BUG-01 anchor drift | VERIFIED | TC 13 at lines 437-494; tagged `[looper][bug01]`; uses FP-drift ppq = 4.0 - 1e-6 to expose the bug correctly |
| `Source/LooperEngine.cpp` | Auto-stop block with overshoot-corrected `loopStartPpq_` | VERIFIED | Lines 756-774; `overshoot` computed at line 758; `loopStartPpq_ += loopLen` at line 762; `-1.0` sentinel removed from auto-stop path |
| `Source/GamepadInput.h` | `pendingReopenTick_` bool in private section | VERIFIED | Line 171: `bool pendingReopenTick_ = false;` — plain bool, not atomic (correct: message-thread-only) |
| `Source/GamepadInput.cpp` | Guarded SDL event loop + deferred open | VERIFIED | Lines 108-143: guarded ADDED handler, instance-ID REMOVED handler, post-loop deferred open, fallback poll preserved |
| `pluginval_output.txt` | pluginval level 5 run log with 0 failures | VERIFIED | File present at project root; `Strictness level: 5`; all suites pass; `SUCCESS` exit |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `Source/LooperEngine.cpp` | `loopStartPpq_` | `loopStartPpq_ + loopLen` (anchor advance) | VERIFIED (deviation) | Plan specified pattern `"ppqPosition - overshoot"` — not present. Actual formula `loopStartPpq_ + loopLen` is mathematically superior and documented in 17-01-SUMMARY as a Rule-1 auto-fix. The `overshoot` variable IS present (line 758) satisfying the `contains: "overshoot"` artifact check. |
| `Tests/LooperEngineTests.cpp` | `LooperEngine::getPlaybackBeat()` | Catch2 CHECK with Approx epsilon | VERIFIED | Line 493: `CHECK(eng.getPlaybackBeat() == Catch::Approx(0.0).epsilon(0.01))` |
| `Source/GamepadInput.cpp` | `tryOpenController()` | `pendingReopenTick_` deferred-open after event loop | VERIFIED | Lines 134-137: `if (pendingReopenTick_ && !controller_) { pendingReopenTick_ = false; tryOpenController(); }` |
| `Source/GamepadInput.cpp` | `closeController()` | `SDL_JoystickInstanceID` instance-ID comparison | VERIFIED | Lines 125-129: instance-ID guard gates `closeController()` call |
| `pluginval run` | VST3 binary | `pluginval --strictness-level 5 --validate-in-process` | VERIFIED | `pluginval_output.txt` confirms level 5, plugin path contains `ChordJoystick_artefacts`, `SUCCESS` |

**Note on Plan 17-01 key_link deviation:** The plan specified `pattern: "ppqPosition - overshoot"` as the expected formula. The implementation uses `loopStartPpq_ + loopLen` instead. The summary explicitly documents this as a Rule 1 (Bug) deviation: the plan formula would set the anchor to the block-START ppqPosition (3.0) rather than the correct recording-end ppq (4.0), making it wrong. The implemented formula is always correct regardless of block size or FP drift. This deviation IMPROVES correctness and is not a gap.

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| BUG-01 | 17-01, 17-03 | Looper playback starts at the correct beat position after a record cycle completes (no anchor drift or offset) | SATISFIED | `loopStartPpq_ += loopLen` fix in LooperEngine.cpp line 762; TC 13 regression test guards it; pluginval passes; user smoke test confirmed |
| BUG-02 | 17-02, 17-03 | Plugin does not crash when PS4 controller reconnects via Bluetooth (SDL2 double-open guard) | SATISFIED | `pendingReopenTick_` deferred-open + instance-ID guard in GamepadInput.cpp; plugin builds clean; pluginval passes; user smoke test confirmed |

REQUIREMENTS.md traceability table: both BUG-01 and BUG-02 show Phase 17, status "Complete" (lines 170-171). Both requirements carry `[x]` checkboxes in the Bug Fixes section (lines 116-117). No orphaned requirements.

---

### Commit Verification

All commits referenced in SUMMARYs are present in git log and match their descriptions:

| Commit | Description | Plan |
|--------|-------------|------|
| `e34c48f` | test(17-01): add failing TC 13 regression test for BUG-01 anchor drift | 17-01 |
| `af1ddff` | fix(17-01): fix BUG-01 looper anchor drift after auto-stop in DAW sync mode | 17-01 |
| `3edf8be` | feat(17-02): add pendingReopenTick_ member to GamepadInput.h | 17-02 |
| `917565e` | fix(17-02): guard SDL device events to fix BT reconnect crash (BUG-02) | 17-02 |
| `461972b` | chore(17-03): run pluginval level 5 on Release VST3 — SUCCESS | 17-03 |

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| — | — | None found | — | — |

No TODOs, FIXMEs, placeholders, or empty implementations in the modified files.

**Note:** `(void)overshoot;` at LooperEngine.cpp line 771 is intentional — the variable is computed for documentation clarity and future use (DAW sync overshoot absorption), suppressing the unused-variable warning. Not a stub.

---

### Human Verification Required

#### 1. BUG-01 — Looper Anchor Grid-Lock

**Test:** In Ableton with DAW sync enabled, record a 4-bar loop and let it play 3+ complete cycles. Observe whether beat 1 of each cycle lands on the DAW grid beat 1 with no drift.

**Expected:** Beat 1 of every playback cycle is grid-locked. No audible late-start slip between cycles.

**Why human:** Requires a running DAW transport and audio monitoring. The fix is in real-time audio-thread timing that cannot be exercised by grep or static analysis. The Catch2 TC 13 verifies the underlying logic; the smoke test verifies end-to-end behavior.

**Status from 17-03-SUMMARY.md:** User confirmed PASS — "beat 1 of each cycle landed exactly on the DAW grid beat 1, no audible drift or late-start slip observed."

#### 2. BUG-02 — PS4 Bluetooth Reconnect Stability

**Test:** Connect PS4 via Bluetooth, confirm controller is detected. Turn off controller via PS button. Wait 5 seconds. Reconnect via PS button. Verify plugin does not crash and controller resumes.

**Expected:** Plugin remains stable; gamepad status label updates after reconnect; joystick and buttons work normally.

**Why human:** Requires physical PS4 hardware and Bluetooth stack. SDL event dispatch during BT re-enumeration cannot be simulated statically.

**Status from 17-03-SUMMARY.md:** User confirmed PASS — "Plugin did not crash or freeze. Gamepad status label updated after reconnect. Joystick and buttons operated normally."

---

### Gaps Summary

No automated gaps found. All code artifacts are substantive and wired. All commits are present. Requirements BUG-01 and BUG-02 are fully satisfied.

The two human verification items have both been reported as PASS in the 17-03-SUMMARY.md. The overall status is `human_needed` because those smoke tests cannot be re-verified programmatically — they depend on the user's confirmation which is already on record.

**One documented deviation from plan (not a gap):** Plan 17-01 specified `loopStartPpq_ = p.ppqPosition - overshoot` as the fix formula. The implementation uses `loopStartPpq_ = loopStartPpq_ + loopLen`. The implemented formula is mathematically correct; the plan formula was wrong (used block-start ppq, not recording-end ppq). This deviation was identified by the executor and explicitly documented as a Rule 1 auto-fix in 17-01-SUMMARY.md. TC 13 passes with the implemented formula.

---

_Verified: 2026-02-28T00:00:00Z_
_Verifier: Claude (gsd-verifier)_
