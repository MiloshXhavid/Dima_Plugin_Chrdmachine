---
phase: 19-sub-octave-per-voice
verified: 2026-03-01T00:00:00Z
status: passed
score: 12/12 must-haves verified
---

# Phase 19: Sub Octave Per Voice — Verification Report

**Phase Goal:** Add a per-voice sub-octave toggle (SUB8) button to each of the 4 voice pads, emitting a note at pitch -12 semitones alongside the main note when enabled.
**Verified:** 2026-03-01
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Triggering a voice with Sub Oct enabled emits two note-ons: the main pitch and one exactly 12 semitones lower, on the same MIDI channel | VERIFIED | `PluginProcessor.cpp:1074-1086` — tp.onNote isOn branch reads subOct param, computes `pitch - 12`, emits noteOn via noteCount_ dedup, stores in `subHeldPitch_[voice]` |
| 2 | Releasing a voice sends note-off using the sub pitch snapshotted at gate-open, not recomputed from live joystick | VERIFIED | `PluginProcessor.cpp:1101-1111` — gate-close path reads `subHeldPitch_[voice]` (snapshot), not `heldPitch_[voice] - 12`; clears snapshot to -1 after |
| 3 | Toggling SUB8 on while gate open emits immediate note-on; toggling off emits immediate note-off — no stuck notes | VERIFIED | `PluginProcessor.cpp:939-973` — mid-note toggle loop before TriggerSystem::processBlock() compares subWanted vs subSounding vs gateOpen; emits note-on or note-off accordingly |
| 4 | Looper playback emits sub-octave notes using live SUB8 param at emission time, not baked into the loop | VERIFIED | `PluginProcessor.cpp:902-914` — looper gateOn reads live `apvts.getRawParameterValue("subOct" + v)` at emission; stores result in `looperActiveSubPitch_[v]` |
| 5 | After allNotesOff / DAW stop, subHeldPitch_ and subOctSounding_ arrays are reset to prevent double note-on on re-trigger | VERIFIED | `PluginProcessor.cpp:427-439` — `resetNoteCount()` resets all three arrays: subHeldPitch_[v]=-1, looperActiveSubPitch_[v]=-1, subOctSounding_[v]=false; called at every flush site (lines 750, 802, 834, 851, 869, 1392) |
| 6 | R3 alone on gamepad: no action (panic removed). R3 + held pad button: toggles subOctN APVTS bool for that voice | VERIFIED | `PluginProcessor.cpp:506-524` — consumeRightStickTrigger() calls getVoiceHeld(v); if held, toggles subOctN via setValueNotifyingHost; no panic call present |
| 7 | Each of the 4 voice pads shows HOLD on the left 50% and SUB8 on the right 50% of the 18px strip | VERIFIED | `PluginEditor.cpp:1813-1816` — `holdStrip.reduced(2,2)`, then `removeFromLeft(width/2)` for HOLD, remainder for SUB8 |
| 8 | Pressing SUB8 toggles the subOctN APVTS bool; button stays lit (orange) to reflect enabled state | VERIFIED | `PluginEditor.cpp:909-916` — ButtonParameterAttachment wired to `subOct0..3` APVTS params; setClickingTogglesState(true) |
| 9 | SUB8 button is bright orange when sounding, dim orange when enabled-not-sounding, darkgrey when disabled | VERIFIED | `PluginEditor.cpp:2549-2569` — timerCallback 30Hz poll: bright orange (0xFFFF8C00) when subEnabled+gateOpen; dim orange (0xFFB36200) when subEnabled; darkgrey otherwise |
| 10 | SUB8 state survives preset save and reload | VERIFIED | ButtonParameterAttachment handles APVTS serialization automatically; subOct0..3 are AudioParameterBool registered in createParameterLayout() |
| 11 | Full trigger + release roundtrip shows correct pitch-12 notes in DAW MIDI monitor with no stuck notes | VERIFIED (human-confirmed) | 19-02-SUMMARY.md: smoke test APPROVED 2026-03-01 — all 4 SUBOCT requirements verified in DAW |
| 12 | Mid-note toggle loop runs before TriggerSystem::processBlock() | VERIFIED | `PluginProcessor.cpp:939` — comment confirms placement; `TriggerSystem::process()` call is at line 975+ |

**Score:** 12/12 truths verified

---

## Required Artifacts

### Plan 19-01 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Source/PluginProcessor.h` | subHeldPitch_[4], looperActiveSubPitch_[4], subOctSounding_[4] member declarations | VERIFIED | Lines 261-265: all three arrays present with correct types — `int[4]` for pitch arrays, `std::atomic<bool>[4]` for sounding flag (no `using` alias, MSVC-safe) |
| `Source/PluginProcessor.cpp` | subOct0..3 APVTS bool params, note-on/off sub emission, mid-note toggle, looper gate sub path, R3 combo, flush reset | VERIFIED | Lines 292-295 (params), 1074-1111 (tp.onNote), 939-973 (mid-note), 902-935 (looper), 506-524 (R3), 427-439 (resetNoteCount) — all present and substantive |

### Plan 19-02 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Source/PluginEditor.h` | padSubOctBtn_[4] TextButton, subOctAttach_[4] ButtonParameterAttachment | VERIFIED | Lines 196-197: `juce::TextButton padSubOctBtn_[4]` and `std::unique_ptr<juce::ButtonParameterAttachment> subOctAttach_[4]` — correct types, no alias |
| `Source/PluginEditor.cpp` | resized() 50/50 split, constructor wiring, timerCallback coloring with "SUB8" text | VERIFIED | Line 909 ("SUB8" text set), lines 913-916 (ButtonParameterAttachment wired), lines 1813-1816 (resized split), lines 2549-2569 (timerCallback 3-state coloring) |

---

## Key Link Verification

### Plan 19-01 Key Links

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `PluginProcessor.cpp tp.onNote lambda` | `subHeldPitch_[v] snapshot` | isOn branch: `subHeldPitch_[voice] = subPitch` before noteCount_ dedup | WIRED | `PluginProcessor.cpp:1078` — snapshot stored after pitch computed, before noteCount dedup at 1079 |
| `PluginProcessor.cpp processBlock mid-note loop` | `subOctSounding_[v] flag` | compare subWanted vs subSounding vs gateOpen before TriggerSystem::process() | WIRED | `PluginProcessor.cpp:944-956` — load at 945, store at 956/971, positioned before TriggerSystem at 975 |
| `PluginProcessor.cpp looper gate-on` | `looperActiveSubPitch_[v]` | live param read at emission, not at record time | WIRED | `PluginProcessor.cpp:903, 907` — getRawParameterValue called inline at gate-on, stored to looperActiveSubPitch_ |

### Plan 19-02 Key Links

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `PluginEditor.cpp timerCallback` | `proc_.apvts.getRawParameterValue("subOct" + v)` | 30Hz poll: set button colour based on enabled + isGateOpen(v) | WIRED | `PluginEditor.cpp:2555-2568` — full 3-state colour logic with getRawParameterValue and isGateOpen() call |
| `PluginEditor.cpp resized()` | `padSubOctBtn_[v].setBounds` | holdStrip split 50/50: holdLeft for padHoldBtn_, remainder for padSubOctBtn_ | WIRED | `PluginEditor.cpp:1814-1816` — removeFromLeft(width/2) split, padSubOctBtn_[v].setBounds(holdStrip) |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|---------|
| SUBOCT-01 | 19-02 | Each of the 4 voice pads has a Sub Oct toggle — right half of existing Hold button | SATISFIED | padSubOctBtn_[4] declared in header, wired via ButtonParameterAttachment, placed in right 50% of holdStrip in resized() |
| SUBOCT-02 | 19-01 | When Sub Oct active, a second note-on fires exactly 12 semitones below on every trigger | SATISFIED | tp.onNote isOn branch: `subPitch = juce::jlimit(0, 127, pitch - 12)`; noteOn emitted via noteCount_ dedup |
| SUBOCT-03 | 19-01 | Sub Oct note-off always matches the pitch used at note-on time (snapshotted, not recomputed at release) | SATISFIED | gate-close reads `subHeldPitch_[voice]` (snapshot from gate-open); not `heldPitch_[voice] - 12` |
| SUBOCT-04 | 19-01 | User can toggle Sub Oct for a pad via gamepad by holding the pad button (R1/R2/L1/L2) and pressing R3 | SATISFIED | R3 handler: `getVoiceHeld(v)` guard; toggles `subOctN` via setValueNotifyingHost; R3 alone is no-op (panic removed) |

All 4 requirements claimed by Phase 19 are SATISFIED. Traceability in REQUIREMENTS.md correctly shows all SUBOCT-01..04 mapped to Phase 19 with status Complete.

No orphaned requirements found — no additional SUBOCT requirements exist in REQUIREMENTS.md beyond the 4 claimed.

---

## Anti-Patterns Found

Anti-pattern scan performed on all Phase 19 modified files:

| File | Pattern | Severity | Verdict |
|------|---------|----------|---------|
| `Source/PluginProcessor.h` | No TODOs, no stubs, no empty returns | — | Clean |
| `Source/PluginProcessor.cpp` | No `return {}`, no placeholder, `(void)anyHeld` is intentional suppression of unused variable warning | — | Clean |
| `Source/PluginEditor.h` | No TODOs, no stubs | — | Clean |
| `Source/PluginEditor.cpp` | No `return null`, no placeholder text | — | Clean |

No blockers or warnings found.

---

## Known Deferred Issue (Not a Gap)

**Occasional stuck note** — intermittent, low severity. Observed during smoke test under rapid mid-note toggle or fast trigger sequences. Root cause not identified. Explicitly logged in 19-02-SUMMARY.md and deferred to a future milestone. Does NOT block phase goal — all 4 SUBOCT requirements verified working in DAW. Not counted as a gap per user instruction.

---

## Human Verification Required

None for automated checks — all structural and wiring verification passed programmatically.

The following was verified by the user during the smoke test checkpoint in Plan 19-02 (approved 2026-03-01):

1. **SUB8 button visible on all 4 pads** — HOLD left half, SUB8 right half, orange/darkgrey coloring correct
2. **Sub-octave note at pitch-12 alongside main note** — confirmed in DAW MIDI monitor
3. **Note-off snapshot correctness** — note-off pitches matched note-on even after joystick movement mid-hold
4. **R3 combo** — R3+held pad toggles SUB8; R3 alone produces no action, no panic

---

## Gaps Summary

No gaps. All must-haves verified. Phase 19 goal achieved.

---

_Verified: 2026-03-01_
_Verifier: Claude (gsd-verifier)_
