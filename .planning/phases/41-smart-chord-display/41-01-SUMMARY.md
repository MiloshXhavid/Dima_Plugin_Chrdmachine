---
phase: 41-smart-chord-display
plan: "01"
subsystem: chord-display
tags: [tdd, chord-names, unicode, inference, scale-aware]
dependency_graph:
  requires: []
  provides: [ChordNameContext, computeChordNameSmart, displayVoiceMask_, getCurrentVoiceMask]
  affects: [ChordNameHelper.h, PluginProcessor.h, PluginProcessor.cpp, Tests/ChordNameTests.cpp]
tech_stack:
  added: []
  patterns: [TDD red-green, inline header-only function, u8 unicode literals C++17]
key_files:
  created: []
  modified:
    - Source/ChordNameHelper.h
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Tests/ChordNameTests.cpp
decisions:
  - "uint8_t voiceMask in ChordNameContext — bit-per-voice, default 0xF, same best-effort non-atomic contract as displayPitches_"
  - "computeChordNameSmart delegates to computeChordNameStr after substituting inferred effective pitches — no duplication of template table"
  - "u8 string prefix for Unicode nm entries — compatible with const char* in C++17, avoids /utf-8 compile flag requirement"
  - "pendingVoiceMask_ accumulates across voices in same processBlock; last displayVoiceMask_ write wins with correct accumulated mask"
  - "Site 3 (arp) adds NEW displayPitches_ snapshot per CONTEXT decision D — display now updates rhythmically on each arp step"
metrics:
  duration: "~45 minutes"
  completed: "2026-03-09"
  tasks: 3
  files_modified: 4
---

# Phase 41 Plan 01: Smart Inference Data Layer + TDD Summary

**One-liner:** Scale-aware chord name inference via ChordNameContext struct + computeChordNameSmart, with 9 Unicode jazz symbols and per-voice mask tracking in PluginProcessor.

## What Was Built

Plan 01 implements the pure-logic layer for smart chord display — no JUCE, no audio thread complexity, entirely testable:

**ChordNameHelper.h extensions:**
- `ChordNameContext` struct: `scalePattern` (semitone offset array), `scaleSize`, `voiceMask` (4-bit, one per voice)
- `computeChordNameSmart`: substitutes inferred pitch classes for absent voices (bit not set in mask) before passing to existing `computeChordNameStr` template matching
  - Voice 1 (third): checks scale for minor third (rootPc+3) vs major third (rootPc+4); exclusive presence → infer; ambiguous or absent → leave real pitch
  - Voice 3 (tension): checks scale for maj7 (rootPc+11) vs dom7 (rootPc+10); exclusive presence → infer
- 9 Unicode nm substitutions in kT[] using u8"..." prefix: △ (maj7, mmaj7 variants, extensions), ø (m7b5), °7 (dim7)

**PluginProcessor.h additions:**
- `getCurrentVoiceMask()` getter alongside `getCurrentPitches()`
- `displayVoiceMask_` field (default 0xF)
- `pendingVoiceMask_` accumulator (reset each processBlock)

**PluginProcessor.cpp — 3 snapshot sites:**
- Site 1 (param-hash): `displayVoiceMask_ = 0xF`
- Site 2 (deliberate trigger): `pendingVoiceMask_ |= (1 << voice)` then `displayVoiceMask_ = pendingVoiceMask_`
- Site 3 (arp step): `displayPitches_ = heldPitch_; displayVoiceMask_ = 0xF` — NEW snapshot, enables rhythmic arp display updates

**TDD cycle:**
- RED: 7 new TEST_CASEs referencing ChordNameContext/computeChordNameSmart → compile failure confirmed
- GREEN: All 46 [chordname] test cases pass (64 assertions) after implementation
- Existing tests updated to expect Unicode strings ("Cmaj7" → u8"C\u25b3", etc.)

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | RED: smart inference tests | 3515a70 | Tests/ChordNameTests.cpp |
| 2 | GREEN: ChordNameSmart + Unicode | 929dcbe | Source/ChordNameHelper.h, Tests/ChordNameTests.cpp |
| 3 | displayVoiceMask_ + 3 snapshot sites | 3778c20 | Source/PluginProcessor.h, Source/PluginProcessor.cpp |

## Verification

```
[chordname] tag: All tests passed (64 assertions in 46 test cases)
VST3 target: Build succeeded, 0 errors (auto-installed to Common Files VST3)
```

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing] Updated existing test expectations for Unicode rename**
- **Found during:** Task 2 GREEN phase
- **Issue:** Existing tests compared against old string names ("Cmaj7", "Cdim7", "Cm7b5", "Cmaj9", "Cmaj7#11", "Cmmaj7") which no longer matched after kT[] Unicode substitutions
- **Fix:** Updated 6 existing CHECK() assertions to use u8"\uXXXX" escape sequences
- **Files modified:** Tests/ChordNameTests.cpp
- **Commit:** 929dcbe

**2. [Rule 2 - Missing] Added #include <cstdint> to ChordNameHelper.h**
- **Found during:** Task 2 implementation
- **Issue:** uint8_t used in ChordNameContext struct without stdint include (standalone test file has no JUCE headers)
- **Fix:** Added `#include <cstdint>` at top of ChordNameHelper.h
- **Files modified:** Source/ChordNameHelper.h
- **Commit:** 929dcbe

## Self-Check

**Files exist:**
- Source/ChordNameHelper.h — contains ChordNameContext ✓
- Source/PluginProcessor.h — contains displayVoiceMask_ ✓
- Source/PluginProcessor.cpp — contains displayVoiceMask_ at 3 sites ✓
- Tests/ChordNameTests.cpp — contains computeChordNameSmart tests ✓

**Commits exist:**
- 3515a70 (RED tests) ✓
- 929dcbe (GREEN impl + Unicode) ✓
- 3778c20 (PluginProcessor mask) ✓

## Self-Check: PASSED
