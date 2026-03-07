---
phase: 37-looper-internalbeat-fix
verified: 2026-03-07T03:00:00Z
status: human_needed
score: 3/4 must-haves verified
human_verification:
  - test: "DAW smoke test — overdub no-duplicate"
    expected: "No extra note-ons appear at the loop start boundary after overdub recording auto-stops (SC1, SC2, SC3 from plan)"
    why_human: "Runtime MIDI behaviour in a DAW cannot be verified statically; SUMMARY states user approved but no programmatic evidence exists"
---

# Phase 37: Looper internalBeat_ Fix Verification Report

**Phase Goal:** Fix the LooperEngine double-scan bug where internalBeat_=0.0 at line 773 overwrites the correct fmod-absorbed overshoot value, causing beat-0 events to fire twice on the block after a recording ends.
**Verified:** 2026-03-07T03:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | After overdub auto-stop, internalBeat_ reflects fmod overshoot (non-zero) | VERIFIED | `internalBeat_ = 0.0` absent from the auto-stop block (lines 768-784); fmod at line 758 is the sole write path in free-running mode |
| 2 | Beat-0 events do not fire an extra time on the block after recording ends | VERIFIED | Follows directly from truth 1; TC 14 guards against regression with `beatAfterStop > 1e-4` assertion |
| 3 | Existing TC 01-13 continue to pass after the code change | VERIFIED | SUMMARY confirms all 14 Catch2 tests pass GREEN; pre-existing TC 4/5/6/10/11 failures were also fixed in this phase |
| 4 | DAW MIDI monitor shows no duplicate notes at loop start boundary after overdub | HUMAN NEEDED | SUMMARY states user approved smoke test; cannot verify programmatically |

**Score:** 3/4 truths verified automatically

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Source/LooperEngine.cpp` | Line 773 `internalBeat_ = 0.0` removed; comment updated to mention "Both modes absorb overshoot" | VERIFIED | Auto-stop block (lines 768-784) contains no `internalBeat_ = 0.0` assignment. Comment at lines 778-780 reads "Both modes now absorb overshoot correctly" with per-mode explanation. Two remaining occurrences of `internalBeat_ = 0.0` at lines 186 and 602 are legitimate (constructor reset and explicit looper-reset path — not the bug site). |
| `Tests/LooperEngineTests.cpp` | TC 14 with `[looper][bug-double-scan]` tag, containing "double-scan" | VERIFIED | TC 14 present at lines 504-572. Tag `[looper][bug-double-scan]` confirmed. Comment block contains "double-scan". Assertions `beatAfterStop > 1e-4` and `beatAfterStop < blockBeats` present. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `Source/LooperEngine.cpp` | `Tests/LooperEngineTests.cpp` | TC 14 tests the exact line removed | VERIFIED | TC 14 comment explicitly references "fmod at line 758" and "line 773 removed"; the test drives free-running blocks at 512 samples to guarantee non-integer overshoot and asserts the non-zero beat position that the fix enables |

### Requirements Coverage

No requirement IDs were declared for this phase.

### Anti-Patterns Found

None. No TODO/FIXME/placeholder comments in the modified blocks. No empty implementations. No stub returns.

### Human Verification Required

#### 1. DAW Smoke Test — Overdub No-Duplicate

**Test:** In a DAW, record a loop with gate events and let it auto-stop. Open a MIDI monitor on the looper's output channel and watch beat 0 as the loop plays back. Then record an overdub and observe 2+ playback cycles.
**Expected:**
- SC1: No extra note-ons appear at the loop start boundary (beat 0 fires exactly once per cycle)
- SC2: No "resets to beginning" artifact after the overdub completes
- SC3: Multiple consecutive overdubs produce no phantom duplicates
**Why human:** Runtime MIDI event timing in a live DAW session cannot be verified by static code analysis or Catch2 unit tests. The SUMMARY documents user approval of this smoke test (Task 3 checkpoint); the verifier cannot independently confirm runtime behaviour.

### Gaps Summary

No gaps. The code fix is correct and complete. The regression test is substantive and wired. The only open item is the DAW smoke test, which the SUMMARY states was user-approved — this is flagged human_needed rather than passed because the verifier cannot independently confirm a runtime MIDI result from static analysis.

If the DAW smoke test approval from the SUMMARY is accepted as sufficient, this phase can be treated as **passed**.

---

_Verified: 2026-03-07T03:00:00Z_
_Verifier: Claude (gsd-verifier)_
