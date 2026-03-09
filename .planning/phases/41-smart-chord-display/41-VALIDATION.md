---
phase: 41
slug: smart-chord-display
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-09
---

# Phase 41 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GoogleTest (existing ChordEngineTests.cpp / ChordNameTests.cpp in Tests/) |
| **Config file** | Tests/ChordNameTests.cpp |
| **Quick run command** | `cmake --build "C:/Users/Milosh Xhavid/get-shit-done/build" --config Release --target ChordJoystickTests && "C:/Users/Milosh Xhavid/get-shit-done/build/Release/ChordJoystickTests.exe"` |
| **Full suite command** | same |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run quick run command
- **After every plan wave:** Run full suite command
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 41-01-01 | 01 | 1 | computeChordNameSmart | unit | Tests/ChordNameTests.cpp | ❌ W0 | ⬜ pending |
| 41-01-02 | 01 | 1 | displayVoiceMask_ writes | build | cmake build target | ✅ | ⬜ pending |
| 41-01-03 | 01 | 1 | UI integration | build+manual | cmake build + visual check | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `Tests/ChordNameTests.cpp` — unit tests for `computeChordNameSmart` covering minor, major, dom7, maj7, m7, 6th, sus2/sus4, tension extensions, absent voice inference

*If existing infrastructure covers: wave 0 adds test stubs before or alongside implementation.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Chord name retained during silence | update-on-trigger | No DAW silence simulation in unit tests | Play chord, stop triggering, verify name stays |
| Correct name shown in Reaper for real scale | scale inference | Live audio context required | Load Minor scale, trigger partial voices, verify quality |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
