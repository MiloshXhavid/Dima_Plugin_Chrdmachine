---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Routing + Expression
status: in_progress
last_updated: "2026-03-01T12:00:00.000Z"
progress:
  total_phases: 17
  completed_phases: 16
  total_plans: 44
  completed_plans: 44
---

---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Routing + Expression
status: unknown
last_updated: "2026-03-01T07:09:26.634Z"
progress:
  total_phases: 16
  completed_phases: 14
  total_plans: 41
  completed_plans: 38
---

---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Routing + Expression
status: unknown
last_updated: "2026-03-01T05:32:34.162Z"
progress:
  total_phases: 14
  completed_phases: 13
  total_plans: 36
  completed_plans: 35
---

---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Routing + Expression
status: unknown
last_updated: "2026-03-01T00:56:49.784Z"
progress:
  total_phases: 12
  completed_phases: 11
  total_plans: 31
  completed_plans: 30
---

---
gsd_state_version: 1.0
milestone: v1.5
milestone_name: Routing + Expression
status: in_progress
last_updated: "2026-03-01T00:00:00Z"
progress:
  total_phases: 10
  completed_phases: 9
  total_plans: 26
  completed_plans: 26
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-28)

**Core value:** XY joystick mapped to harmonic space — per-note trigger gates, scale quantization, gesture looper with trigger quantization, gamepad control — no competitor provides this as a unified instrument.
**Current focus:** v1.5 — Phase 24: Gamepad Option Mode 1 COMPLETE (all 3 plans done, all 7 OPT1 requirements verified live in DAW)

## Current Position

Phase: 24.1 (LFO Joystick Visual Tracking and Center Offset) — In Progress (1/2 plans)
Plan: 24.1-01 complete — PluginProcessor backend refactor: override floats, display atomics, SmoothedValue, base+offset dispatch
Status: 24.1-02 pending — PluginEditor timerCallback poll + build + deploy + human verify
Last activity: 2026-03-01 — Phase 24.1 plan 01 complete (build verified, no compiler errors)

```
v1.0 MVP    [██████████] SHIPPED 2026-02-23
v1.3 Polish [██████████] SHIPPED 2026-02-25
v1.4 LFO    [██████████] SHIPPED 2026-02-26
v1.5 Routing+Expression  [█████     ] In progress
  Phase 17  [██████████]   Bug Fixes              COMPLETE 2026-02-28
  Phase 18  [██████████]   Single-Channel Routing COMPLETE 2026-02-28
  Phase 19  [██████████]   Sub Octave Per Voice   COMPLETE 2026-03-01
  Phase 20  [██████████]   RND Trigger Extensions COMPLETE 2026-03-01
  Phase 21  [██████████]   Left Joystick Targets  COMPLETE 2026-03-01
  Phase 22  [██████████]   LFO Recording          COMPLETE 2026-03-01
  Phase 23  [██████████]   Arpeggiator            COMPLETE 2026-03-01
  Phase 24  [██████████]   Gamepad Option Mode 1  COMPLETE 2026-03-01
  Phase 25  [          ]   Distribution           Not started
```

## Performance Metrics

**Velocity:**
- Total plans completed: 50 (v1.0: 17, v1.3: 11, v1.4: 9, v1.5: 19 [Phase 17 complete + 18-01 + 18-02 + 18-03 + 19-01 + 19-02 + 20-01 + 20-02 + 20-03 + 21-01 + 21-02 + 22-01 + 22-02 + 22-03 + 23-01 + 23-02 + 24-01 + 24-02 + 24-03])
- Average duration: not tracked per plan
- Total execution time: not tracked

## Accumulated Context

### Roadmap Evolution

- Phase 24.1 inserted after Phase 24: LFO joystick visual tracking and center offset (URGENT — pre-distribution fix)

### Decisions

See .planning/PROJECT.md Key Decisions table — full log with outcomes.

Key v1.5 design decisions (locked, do not re-open):
- LFO recording: pre-distortion samples stored; Distort applied live on playback
- Arp step counter resets to 0 on toggle-off
- Single Channel looper uses live channel setting (not record-time)
- Gate Length is unified across Arp + Random sources (one param, both systems)
- Random Hold mode: RND Sync applies (held pad + sync = gated synced bursts)
- Population range 1–64; Probability 0–100%; subdivision adds 1/64
- Phase 17 must precede Phase 22 (looper anchor bug corrupts LFO recording seams)
- Phase 18 must precede Phase 19 (sentChannel_ infrastructure shared)
- [Phase 17-bug-fixes]: pendingReopenTick_ is a plain bool (not atomic) — timerCallback() runs exclusively on JUCE message thread
- [Phase 17-bug-fixes]: BUG-02: deferred-open pattern + instance-ID guard eliminates PS4 BT double-open and wrong-handle-close crashes
- [Phase 17-01]: BUG-01 fix: loopStartPpq_ += loopLen (not p.ppqPosition - overshoot) — plan formula was wrong; advancing by loopLen is always correct and handles FP drift
- [Phase 17-01]: TC 13 uses ppq = 4.0 - 1e-6 to expose FP drift bug; exact ppq=4.0 would not demonstrate the regression
- [Phase 18-01]: allNotesOff flush paths (DAW stop, gamepad disconnect) now cover all 16 channels — not just voiceChs[v] — to ensure Single Channel mode correctness
- [Phase 18-01]: processBlockBypassed uses sentChannel_ snapshots and calls resetNoteCount() on bypass activation
- [Phase 18-02]: Used full juce::APVTS::ComboBoxAttachment type in header (not ComboAtt alias) — alias declared later in same class, causing MSVC C2923
- [Phase 18-02]: singleChanTargetBox_ and voiceChBox_ grid share same vertical band in resized(); timerCallback setVisible() toggles exclusivity
- [Phase 19-01]: std::atomic<bool> used directly for subOctSounding_ (not typedef/alias) — avoids MSVC C2923 (same lesson as Phase 18-02)
- [Phase 19-01]: resetNoteCount() extended to reset sub arrays — single insertion point covers all 7 flush call sites
- [Phase 19-01]: Looper stop/reset paths need sub note-offs emitted before resetNoteCount() call (not covered by plan; auto-fixed Rule 2)
- [Phase 19-01]: R3 alone = no-op; panic removed from gamepad; UI panicBtn_ handles panic going forward
- [Phase 19-01]: Looper sub-octave uses live SUB8 param at emission time — not baked into loop — consistent with single-channel routing pattern
- [Phase 19-02]: ButtonParameterAttachment used for SUB8 (not manual onClick) — handles preset save/load automatically; HOLD uses manual onClick because it drives proc_.padHold_ directly
- [Phase 19-02]: holdStrip.reduced(2,2) applied before 50/50 split so both HOLD and SUB8 share equal margins
- [Phase 19-02]: Occasional stuck note (intermittent, low severity) observed during smoke test — deferred to future milestone; does not block SUBOCT completion
- [Phase 20-01]: TriggerSource::Random renamed to RandomFree (value 2 preserved) — existing DAW sessions load correctly
- [Phase 20-01]: Double-roll model: two independent nextRandom() calls per tick; both popProb and userProb must pass; expected notes/bar = population * probability
- [Phase 20-01]: Manual gate sentinel: gateLength <= 0.0f sets randomGateRemaining_[v] = -1; countdown guard > 0 skips sentinel so note sustains until next trigger's note-on fires note-off
- [Phase 20-01]: RandomHold pad-release check fires immediate note-off (overrides any active gate timer); processed before tick+roll evaluation
- [Phase 20-02]: arpGateTime param (5–100%) removed; unified gateLength (0.0–1.0) serves both Arp and Random gate-time — no /100 conversion needed, gateRatio reads directly
- [Phase 20-02]: tp.joystickGateTime hardcoded to 1.0f constant — arpGateTime was its only source and is gone; dedicated param deferred to future milestone
- [Phase 20-02]: trigSrcNames index 2 label changed from 'Random' to 'Random Free'; 'Random Hold' appended at index 3 — integer values unchanged for DAW session compat
- [Phase 20-03]: randomClockSync defaults to false — free-tempo mode is more useful out-of-the-box; sync requires explicit opt-in
- [Phase 20-03]: Widget reuse pattern for Population knob — randomDensityKnob_ widget kept, only attachment renamed/retargeted to randomPopulation
- [Phase 21-01]: filterXMode index 2 repurposed from Mod Wheel (CC1) to LFO-X Freq — saved presets at index 2 load as LFO-X Freq (accepted; Mod Wheel target removed)
- [Phase 21-01]: Sync LFO Freq target uses std::pow(4.0f, stick*atten) — exponential 0.25x–4x subdivision multiplier stored in lfoXSubdivMult_/lfoYSubdivMult_ atomics
- [Phase 21-01]: LFO/Gate dispatch gated on stickUpdated only (not baseChanged) — base knob irrelevant for non-CC modes
- [Phase 21-01]: CC emit blocks guarded to xMode<=1 / yMode<=1 — modes 2-5 never emit MIDI CC
- [Phase 21-02]: drawAbove() labels auto-follow component position — no paint() changes needed when resized() layout order swaps
- [Phase 21-02]: timerCallback Atten label update guarded by change-check (getText() != newText) to avoid 30Hz redundant styleLabel() calls
- [Phase 22-01]: recBuf_ stores post-level, pre-distortion values — distortion is never recorded, always applied live on output (LFOREC-06)
- [Phase 22-01]: reset() intentionally does NOT clear recState_ — transport stop must not destroy captured LFO recording; clearRecording() is the explicit API
- [Phase 22-01]: std::atomic<int> used for recState_ (not std::atomic<LfoRecState>) — established MSVC C2338 workaround
- [Phase 22-01]: capturedCount_ capped to kRecBufSize via std::min in playback — handles partial and ring-wrapped captures correctly
- [Phase 22-02]: Edge detection block inserted after looper_.process() return, before chord params — state transitions fire before LFO is processed in same block
- [Phase 22-02]: playbackPhase computed inside each LFO enabled branch independently to avoid computation when LFO is disabled
- [Phase 22-02]: loopOut.looperReset has two independent consumers: note-off handler (existing) + new clearRecording dispatch — both fire from same flag without conflict
- [Phase 22-03]: ARM toggle-cancel: second press while Armed returns to Unarmed (calls clearRecording) — toggle model preferred over fire-and-forget
- [Phase 22-03]: Immediate capture guard: ARM pressed during active looper playback starts capture immediately, no edge-detect delay
- [Phase 22-03]: setAlpha() required alongside setEnabled(false) for JUCE TextButton visual grayout in project's custom LookAndFeel
- [Phase 22-03]: LFO disable clears ARM/recording state — prevents invisible stuck-armed state when Enabled toggle is turned off mid-session
- [Phase 22-03]: 8x sub-block interpolated LFO capture writes per processBlock call — prevents aliased/blocky playback shapes at typical buffer sizes
- [Phase 23-01]: ARP-05 bar-boundary release uses timeSigNumer from DAW (not looper subdivision) in DAW sync mode — DAW grid is authoritative
- [Phase 23-01]: arpSyncOn guard mandatory in bar-boundary block — looper-sync mode uses looperJustStarted; bar-boundary fires only on DAW sync path
- [Phase 23-01]: timeSigNumer defaults to 4 (4/4) — safe fallback when host does not report time signature
- [Phase 23-01]: long long cast used for bar index (not floor) — consistent with step-locking pattern already in processBlock
- [Phase 24-01]: Mode 1 face-button double-press uses kDpadDoubleClickMs (300ms) — same constant as D-pad double-click; separate ButtonState trackers per button (btnMode1Circle_ etc.) keep looper detection unchanged
- [Phase 24-02]: looper consume calls wrapped in `if (optMode != 1)` — Mode 0 and Mode 2 retain full looper face-button control; Mode 1 exclusively dispatches arp signals
- [Phase 24-02]: lastHighlightMode_ int in PluginEditor.h guards per-mode highlight block to avoid redundant 30Hz repaints; Mode 1 = green tint, Mode 2 = muted blue, Mode 0 = transparent
- [Phase 24-02]: Mode 1 label changed from "OCTAVE" to "ARP" — arp control is now primary Mode 1 identity; D-pad octave remains available but is secondary

### Pending Todos

None.

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-03-01
Stopped at: Completed 24.1-01-PLAN.md — override floats, display atomics, SmoothedValue added; base+offset dispatch replaces writeParam; build verified
Next step: Phase 24.1 Plan 02 — timerCallback display poll, build, deploy, human verify with PS controller
