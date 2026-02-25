---
phase: 10-trigger-quantization-infrastructure
plan: "03"
subsystem: looper
tags: [quantize, looper, midi, atomic, lock-free, shadow-copy, dedup]

# Dependency graph
requires:
  - phase: 10-01
    provides: snapToGrid() and quantizeSubdivToGridSize() declared in LooperEngine.h, implemented and tested
  - phase: 10-02
    provides: APVTS quantizeMode + quantizeSubdiv params registered; processor stub methods in place
provides:
  - Full LooperEngine quantize backend: shadow copy, live snap in recordGate(), post-record service in process(), collision dedup in applyQuantizeToStore()
  - PluginProcessor stubs wired to real LooperEngine API
  - processBlock() pushes APVTS quantize state to LooperEngine each block
affects: ["10-04-ui-wiring", "10-05-integration-test"]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Shadow copy pattern: save originalStore_[] before destructive quantize, revert via hasOriginals_ flag"
    - "Pending atomic flag pattern: message thread sets pendingQuantize_, audio thread exchanges(false) at top of process()"
    - "scratchDedup_ class member: avoids large stack allocation (~49KB) in applyQuantizeToStore() — matches existing scratchNew_/scratchMerged_ pattern"
    - "Duration-preserving note-off: rigid shift of gate-off using lastSnappedOnBeat_[voice] + original duration"

key-files:
  created: []
  modified:
    - Source/LooperEngine.h
    - Source/LooperEngine.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "scratchDedup_ is a class member (not local array) to avoid ~49KB stack allocation in applyQuantizeToStore() — matches existing scratchNew_/scratchMerged_ pattern"
  - "lastRawOnBeat_[4] and lastSnappedOnBeat_[4] are audio-thread-only double arrays (no atomic needed) — only written/read in recordGate()"
  - "applyQuantizeToStore() skips Joystick and Filter events entirely — only Gate-type events are touched"
  - "hasOriginals_ and quantizeActive_ also reset on deleteLoop() to prevent stale revert after delete"
  - "Post mode re-applies quantize after finaliseRecording() by setting pendingQuantize_ — new content is auto-quantized"

patterns-established:
  - "Quantize pending flag: message thread sets, audio thread exchanges(false) at top of process() before other service blocks"
  - "Shadow copy revert: originalStore_[] + originalCount_ restored atomically without mutex (audio-thread-only during playback)"

requirements-completed: [QUANT-01, QUANT-02, QUANT-03]

# Metrics
duration: 25min
completed: 2026-02-25
---

# Phase 10 Plan 03: LooperEngine Quantize Backend Summary

**Full lock-free quantize backend: shadow copy with revert, live gate snap in recordGate(), post-record applyQuantizeToStore() with Gate-only dedup, processor stubs wired to real LooperEngine API**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-02-25T00:00:00Z
- **Completed:** 2026-02-25T00:25:00Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added 10 new members to LooperEngine (originalStore_[], shadow copy atomics, per-voice beat tracking arrays, scratchDedup_) plus 5 public API methods (setQuantizeMode, setQuantizeSubdiv, isQuantizeActive, applyQuantize, revertQuantize)
- Implemented applyQuantizeToStore(): snaps only Gate events, preserves note duration on note-off, sorts by beat, deduplicates same-voice same-beat gate-ons using scratchDedup_ class member
- Added pendingQuantize_/pendingQuantizeRevert_ service block in process() with recording_ guard; added hasOriginals_ reset in finaliseRecording() with Post-mode auto-re-apply
- Implemented live quantize in recordGate(): saves raw position, snaps note-on, computes duration-preserved note-off from snapped-on + raw duration (with wrap handling and 1/64 minimum)
- Wired PluginProcessor stubs: looperApplyQuantize/looperRevertQuantize/looperQuantizeIsActive now call real LooperEngine methods; processBlock() pushes APVTS quantizeMode+quantizeSubdiv to looper_ each block

## Task Commits

Each task was committed atomically:

1. **Task 1: Add shadow copy members and quantize API to LooperEngine** - `2c2fb3c` (feat)
2. **Task 2: Live quantize in recordGate() and wire processor stubs** - `41e1118` (feat)

**Plan metadata:** (docs commit — see final commit hash)

## Files Created/Modified
- `Source/LooperEngine.h` - Added 10 private members (shadow copy, atomics, per-voice arrays, scratchDedup_), 5 public quantize API methods, applyQuantizeToStore() private declaration
- `Source/LooperEngine.cpp` - Implemented applyQuantizeToStore() (Gate-only, dedup, re-sort), pending flag service in process(), hasOriginals_ reset in finaliseRecording(), live quantize in recordGate()
- `Source/PluginProcessor.h` - Replaced 3 stub bodies with real looper_ calls; updated setQuantizeMode/setQuantizeSubdiv to also call looper_ directly
- `Source/PluginProcessor.cpp` - Added APVTS quantize state push to looper_ in processBlock()

## Decisions Made
- scratchDedup_ is a class member to avoid ~49KB stack allocation in applyQuantizeToStore() — matches existing scratchNew_/scratchMerged_ pattern
- lastRawOnBeat_[4] audio-thread-only (no atomic) — only written/read in recordGate(), no cross-thread access
- applyQuantizeToStore() only touches Gate-type events — joystick and filter events are intentionally skipped
- hasOriginals_ and quantizeActive_ reset on deleteLoop() to prevent stale revert state after loop deletion
- Post mode sets pendingQuantize_ after finaliseRecording() to auto-quantize new recordings

## Deviations from Plan

None — plan executed exactly as written.

The plan explicitly called for `scratchDedup_` as a class member (matching the existing scratch buffer pattern), and the implementation followed this exactly. No auto-fixes or architectural deviations were needed.

## Issues Encountered

None. Both builds succeeded cleanly. Test results unchanged at 22 pass / 5 fail, with the 5 failures being the pre-existing `hasContent()` issue (TC 4, 5, 6, 10, 11) documented as known failures in the plan deviation note.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness
- LooperEngine quantize backend is fully functional and ready for Plan 10-04 (UI wiring)
- All PluginProcessor passthrough stubs are wired — no stubs remain
- processBlock() pushes APVTS quantize state each block, so UI changes will be immediately reflected
- TC 12 (quantize math) still passes; no regressions introduced

---
*Phase: 10-trigger-quantization-infrastructure*
*Completed: 2026-02-25*
