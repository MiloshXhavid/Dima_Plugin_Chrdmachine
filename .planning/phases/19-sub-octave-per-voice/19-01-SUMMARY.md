---
phase: 19-sub-octave-per-voice
plan: 01
subsystem: midi
tags: [cpp, juce, vst3, midi, sub-octave, apvts, gamepad]

# Dependency graph
requires:
  - phase: 18-single-channel-routing
    provides: "sentChannel_/noteCount_ dedup infrastructure used by sub-octave emission"
provides:
  - "subOct0..3 APVTS bool parameters (DAW automation-ready)"
  - "subHeldPitch_[4] snapshot array — gate-time sub pitch for correct note-off matching"
  - "looperActiveSubPitch_[4] snapshot array — looper gate sub pitch"
  - "subOctSounding_[4] atomic array — mid-note toggle detection"
  - "Sub-octave note-on/off emission using noteCount_ dedup pattern"
  - "Mid-note SUB8 toggle loop in processBlock before TriggerSystem::processBlock"
  - "Looper gate-on/off sub-octave path with live param at emission time"
  - "resetNoteCount() extended to reset all sub-octave arrays"
  - "R3 gamepad: panic removed; R3+held pad toggles subOctN bool"
affects: [19-02-ui-buttons, future-phases-using-sub-octave]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Sub-pitch snapshot pattern: snapshot at gate-open (not recomputed from live joystick) matches heldPitch_/looperActivePitch_ conventions"
    - "Mid-note toggle: compare wanted vs sounding vs gateOpen before TriggerSystem::processBlock"
    - "Sub reset in resetNoteCount: all flush sites covered automatically"
    - "R3 gamepad combo: consumeRightStickTrigger + getVoiceHeld → setValueNotifyingHost on subOctN"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "Sub-pitch snapshot stored at gate-open (subHeldPitch_[v]) — note-off always uses snapshot, never heldPitch_[v]-12 live"
  - "resetNoteCount() extended to also reset all sub arrays — single call covers all flush sites (DAW stop, panic, mode change, disconnect)"
  - "R3 alone: no-op (panic removed per Phase 19 Decision 3); R3+held pad toggles subOctN APVTS bool via setValueNotifyingHost (matches existing octave-change gamepad pattern)"
  - "Looper sub-octave uses live SUB8 param at emission time — not baked into loop — consistent with single-channel routing pattern"

patterns-established:
  - "Looper stop/reset path must emit sub note-offs before calling resetNoteCount() — ensures no stuck sub notes"

requirements-completed:
  - SUBOCT-02
  - SUBOCT-03
  - SUBOCT-04

# Metrics
duration: 35min
completed: 2026-03-01
---

# Phase 19 Plan 01: Sub Octave Backend Summary

**PluginProcessor sub-octave backend: APVTS bool params, snapshot arrays, noteCount_-dedup note emission, mid-note toggle loop, looper gate path, and R3 gamepad combo — zero errors in Debug and Release builds**

## Performance

- **Duration:** ~35 min
- **Started:** 2026-03-01T00:00:00Z
- **Completed:** 2026-03-01T00:35:00Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments
- Four `subOct0..3` AudioParameterBool parameters registered in APVTS and visible in DAW automation lanes
- Sub-octave note-on/off emission with `noteCount_` dedup — no stuck notes, correct channel snapshot
- Mid-note SUB8 toggle detection loop: toggling while gate open emits immediate note-on/off
- Looper gate-on/off handles sub-octave using live param at emission time (not baked)
- `resetNoteCount()` extended to reset all sub arrays — covers every allNotesOff flush site automatically
- R3 gamepad panic removed; R3+held pad toggles `subOctN` bool for up to 4 simultaneous voices
- Debug and Release builds succeed with zero errors

## Task Commits

Each task was committed atomically:

1. **Task 1: APVTS bool params and member declarations** - `01d1320` (feat)
2. **Task 2+3: Sub emission, toggle, looper path, flush reset, R3 combo** - `10f1ff1` (feat)

Note: Task 3 R3 changes were included in Task 2's commit batch (edit applied during Task 2 workflow). Release build verified in Task 3 step.

## Files Created/Modified
- `Source/PluginProcessor.h` - Added `subHeldPitch_[4]`, `looperActiveSubPitch_[4]`, `std::atomic<bool> subOctSounding_[4]`
- `Source/PluginProcessor.cpp` - APVTS params, sub emission in tp.onNote, mid-note toggle loop, looper gate sub path, resetNoteCount extension, R3 combo

## Decisions Made
- `std::atomic<bool>` used for `subOctSounding_` (not a typedef/alias) — avoids MSVC C2923 (same as Phase 18-02 APVTS::ComboBoxAttachment lesson)
- `resetNoteCount()` was the cleanest single insertion point for sub-array reset — all 7 call sites covered with one change
- Looper stop/reset paths needed explicit sub note-off emission before `resetNoteCount()` call, since those paths manually emit main note-offs first

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added sub note-off emission in looper-stop and looper-reset paths**
- **Found during:** Task 2 (allNotesOff flush path)
- **Issue:** Plan said to add sub-array reset inside `resetNoteCount()`. But looper-just-stopped and looper-reset paths manually emit main note-offs before calling `resetNoteCount()`. Without adding sub note-off emission in those manual blocks, sub notes would be cut by allNotesOff (host CC123) but `noteCount_` would mismatch on next note-on, causing skipped note-on events.
- **Fix:** Added sub note-off emission inside both the looper-just-stopped loop and the looper-reset loop, before `resetNoteCount()` is called.
- **Files modified:** Source/PluginProcessor.cpp
- **Verification:** Build passes clean. Logic mirrors existing looperActivePitch_ pattern exactly.
- **Committed in:** 10f1ff1 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 2 — missing critical correctness code)
**Impact on plan:** Auto-fix necessary to prevent noteCount_ mismatch on looper restart after sub note was sounding. No scope creep.

## Issues Encountered
- None — build succeeded on first attempt for both Debug and Release.

## User Setup Required
None - no external service configuration required. Manual DAW smoke test deferred to Plan 19-02.

## Next Phase Readiness
- Sub-octave APVTS backend is complete. Plan 19-02 can wire SUB8 UI buttons directly to `subOct0..3` APVTS params using ButtonAttachment.
- `subOctSounding_[v]` + `isGateOpen(v)` available for the SUB8 button brightness logic in timerCallback.
- No blockers.

---
*Phase: 19-sub-octave-per-voice*
*Completed: 2026-03-01*
