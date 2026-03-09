---
phase: 41-smart-chord-display
plan: "02"
subsystem: chord-display
tags: [editor-wiring, scale-aware, uat, approved]
dependency_graph:
  requires: [41-01]
  provides: [smart-chord-display-live-ui]
  affects: [Source/PluginEditor.cpp, Source/ChordNameHelper.h, Tests/ChordNameTests.cpp]
tech_stack:
  added: []
  patterns: [APVTS getRawParameterValue, ScaleQuantizer static methods, ChordNameContext inline construction]
key_files:
  created: []
  modified:
    - Source/PluginEditor.cpp
    - Source/ChordNameHelper.h
    - Tests/ChordNameTests.cpp
decisions:
  - "computeChordName wrapper updated to accept ChordNameContext — forward to computeChordNameSmart"
  - "customPatBuf declared before if/else block to keep ctx.scalePattern pointer valid across branches"
  - "scalePreset int cast from float APVTS value; useCustomScale compared with > 0.5f threshold"
  - "Unicode symbols (△, ø, °7) reverted to full-word notation (maj7, m7b5, dim7) per UAT feedback — symbols caused readability issues in plugin UI font"
metrics:
  duration: "~30 minutes"
  completed: "2026-03-09"
  tasks: 3
  files_modified: 3
---

# Phase 41 Plan 02: Editor Wiring + UAT Summary

**One-liner:** PluginEditor.cpp wired to call computeChordNameSmart with live APVTS scale context and voice mask; UAT approved with Unicode symbols reverted to full-word notation per user feedback.

## What Was Built

**PluginEditor.cpp changes:**
- `computeChordName` wrapper signature updated from `(const int pitches[4])` to `(const int pitches[4], const ChordNameContext& ctx)` — delegates to `computeChordNameSmart`
- timerCallback chord block builds `ChordNameContext` per tick:
  - Reads `scalePreset` and `useCustomScale` from APVTS
  - Custom scale path: reads `scaleNote0..11` bools, calls `ScaleQuantizer::buildCustomPattern`
  - Preset path: calls `ScaleQuantizer::getScalePattern` + `getScaleSize`
  - Sets `ctx.voiceMask = proc_.getCurrentVoiceMask()`
  - Calls `computeChordName(pitches.data(), ctx)` → `computeChordNameSmart`

**UAT result — all 5 criteria passed:**
1. Chord names display correctly (maj7, m7b5, dim7 in full-word notation)
2. Scale inference — third: Natural Minor → minor quality; Major → major quality (Voice 1 absent)
3. Scale inference — seventh: Major → maj7; Mixolydian → 7 (Voice 3 absent)
4. Silence retention confirmed — label holds last chord name
5. Arp display updates rhythmically per step

**Post-UAT revert (user decision):** Unicode symbols reverted to full-word notation. The △/ø/°7 symbols caused readability issues with the plugin's pixel font at the chord label size. All 9 kT[] entries back to original strings; test expectations updated accordingly.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Wire computeChordName + timerCallback | 8e24f4e | Source/PluginEditor.cpp |
| 2 | Build + install VST3 | 8e24f4e | (auto-installed) |
| 3 | UAT checkpoint — approved | — | Human verified |
| — | Unicode revert per UAT feedback | 29d61df | Source/ChordNameHelper.h, Tests/ChordNameTests.cpp |

## Additional fixes applied during this session (out-of-scope, committed separately)

| Fix | Commit | Files |
|-----|--------|-------|
| Gamepad stick drift hysteresis (mouseJoyActive_ 15% threshold) | 9d3ed76 | PluginProcessor.h/.cpp, PluginEditor.cpp |
| Deferred SDL init 4 s — prevents Ableton startup deadlock | 6c9ebb9 | GamepadInput.cpp/.h |
| LFO phase reset on ON button click (phaseResetPending_) | bf15d57 | LfoEngine.cpp/.h |

## Deviations from Plan

**1. [User Decision] Unicode symbols reverted to full-word notation**
- **Found during:** Task 3 (UAT)
- **Issue:** △/ø/°7 symbols caused readability issues in plugin UI at chord label font/size
- **Fix:** Reverted all 9 kT[] Unicode nm entries to original full-word strings; test expectations updated
- **Files modified:** Source/ChordNameHelper.h, Tests/ChordNameTests.cpp
- **Commit:** 29d61df

## Self-Check

**Files exist:**
- Source/PluginEditor.cpp — contains computeChordNameSmart call ✓
- Source/ChordNameHelper.h — uses full-word notation ✓

**Commits exist:**
- 8e24f4e (editor wiring) ✓
- 29d61df (Unicode revert) ✓

## Self-Check: PASSED
