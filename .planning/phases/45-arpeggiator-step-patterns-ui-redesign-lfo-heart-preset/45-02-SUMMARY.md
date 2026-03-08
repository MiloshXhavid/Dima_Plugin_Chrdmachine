---
phase: 45-arpeggiator-step-patterns-ui-redesign-lfo-heart-preset
plan: 02
subsystem: ui
tags: [juce, arpeggiator, step-pattern, apvts, combobox, mousedown, timerCallback]

# Dependency graph
requires:
  - phase: 45-01
    provides: 10 new APVTS params (arpStepState0..7, arpLength, randomSyncMode) and arpCurrentStep_ atomic for step highlight

provides:
  - 8-cell step grid (2 rows x 4 cols) rendered inside 84px arp box in paint()
  - mouseDown() hit-test cycles arpStepState{i} ON/TIE/OFF on cell click
  - arpLengthBox_ ComboBox attached to arpLength APVTS param (LEN combo in controls row)
  - RND SYNC button rewired to manual 3-state cycling (FREE/INT/DAW) via onClick
  - updateRndSyncButtonAppearance() syncs button text and color to randomSyncMode
  - resized() arp block: 4-column controls row (Rate | Order | LEN | Gate), no label row
  - timerCallback: repaint arpBlockBounds_ on current step change + RND SYNC appearance sync

affects: [PluginEditor, arp step playback visual feedback, RND SYNC preset load behavior]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "mouseDown() at editor level for step-grid click routing — hit-tests member rects, dispatches to APVTS AudioParameterChoice via dynamic_cast + setValueNotifyingHost"
    - "3-state button cycling pattern: no ButtonParameterAttachment, onClick reads getIndex(), computes (idx+1)%3, calls convertTo0to1 — identical to Phase 45-01 gamepad rndSyncToggle"
    - "updateRndSyncButtonAppearance() helper called from constructor, onClick, and timerCallback — single source of truth for button text + color"
    - "Step grid dimming: alpha=1.0 for active cells (i < arpLen), alpha=0.4 for inactive — reads arpLength+1 with jlimit(1,8)"
    - "Full type name for unique_ptr member declared before using alias — avoids ComboAtt forward reference error in header"

key-files:
  created: []
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp

key-decisions:
  - "arpLengthAtt_ declared as full type std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> in header (before using ComboAtt alias is visible) — avoids C2065 undeclared identifier build error"
  - "mouseDown() at PluginEditor level (not a nested child component) — arpStepRowABounds_/arpStepRowBBounds_ are member rects, not components"
  - "arpSubdivLabel_, arpOrderLabel_, arpGateTimeLabel_ kept declared and styled but NOT added to component tree — invisible in new layout but still valid for mode1Labels[] in timerCallback"
  - "Separator in paint() is a single vertical line at mid-width of the step rows (visual grouping only), not a row separator"
  - "timerCallback uses static int lastStep to detect step changes — avoids full-editor repaint every 30Hz tick"

patterns-established:
  - "Phase 45 step grid pattern: arpStepRowABounds_/B_ set in resized(), drawn in paint() with cellRect lambda, hit-tested in mouseDown()"

requirements-completed: [ARP-01, ARP-02, ARP-03]

# Metrics
duration: 45min
completed: 2026-03-08
---

# Phase 45 Plan 02: Arp Step Patterns UI Summary

**8-cell step grid (2 rows x 4 cols) with ON/TIE/OFF click cycling, LEN combo, and 3-state RND SYNC button (FREE/INT/DAW) inside the unchanged 84px arp box — UAT approved; two post-UAT correctness fixes for arpLen wrap and TIE noteOff suppression**

## Performance

- **Duration:** ~45 min (including post-UAT fixes)
- **Started:** 2026-03-08T21:00Z
- **Completed:** 2026-03-08T21:46Z
- **Tasks:** 2 code tasks + UAT checkpoint (approved) + 2 post-UAT bug fix commits
- **Files modified:** 2

## Accomplishments

- 8-cell arp step grid rendered in paint() with ON (bright fill), TIE (dim fill + horizontal bar), and OFF (empty bordered rect) states; cells beyond arpLength show at 40% alpha
- mouseDown() hit-tests arpStepRowABounds_ and arpStepRowBBounds_, cycles arpStepState{i} AudioParameterChoice ON→TIE→OFF→ON
- arpLengthBox_ ComboBox (1–8) added and attached to arpLength APVTS param; displayed as 3rd of 4 equal columns in controls row
- RND SYNC button rewired to manual onClick cycling; updateRndSyncButtonAppearance() drives FREE(gray)/INT(green)/DAW(blue) text and color from randomSyncMode
- resized() arp block redesigned: 20px ARP ON button, 14px row A (cells 0–3), 14px row B (cells 4–7), 20px 4-column controls row — fits within kArpH=84px
- timerCallback: repaint arpBlockBounds_ only on step change (static lastStep guard); updateRndSyncButtonAppearance() called every tick for preset-load sync

## Task Commits

1. **Task 1 + Task 2: Editor step grid, LEN combo, RND SYNC 3-state** - `e4d066b` (feat)
2. **Post-UAT fix: arpStep wraps at arpLen, LEN combo triggers instant repaint** - `f2b71d6` (fix)
3. **Post-UAT fix: TIE suppresses gate-time noteOff on upcoming TIE step** - `312ac04` (fix)

**Plan metadata:** `e889d2a` (docs: complete plan)

## Files Created/Modified

- `Source/PluginEditor.h` - Added arpLengthBox_, arpLengthAtt_, arpStepRowABounds_, arpStepRowBBounds_; removed randomSyncButtonAtt_; declared mouseDown() and updateRndSyncButtonAppearance()
- `Source/PluginEditor.cpp` - Constructor (label visibility, arpLengthBox_ init, randomSyncButton_ 3-state rewrite), resized() arp block, paint() step grid, mouseDown() handler, updateRndSyncButtonAppearance() implementation, timerCallback additions

## Decisions Made

- arpLengthAtt_ uses full type in header (not the ComboAtt alias) to avoid undeclared identifier error — the using alias is defined later in the same class body
- mouseDown() on PluginEditor directly (not a child component) since the step rows are member Rectangle<int> values, not sub-components
- The three arp labels (arpSubdivLabel_, arpOrderLabel_, arpGateTimeLabel_) are styled but not added to component tree — they remain valid for timerCallback mode1Labels[] gamepad highlight logic without appearing on screen

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] arpLengthAtt_ type reference to undeclared ComboAtt alias**
- **Found during:** Task 1 (PluginEditor.h member declaration)
- **Issue:** ComboAtt using alias is declared at line 526 of the private section; arpLengthAtt_ was inserted at line 461, before the alias is visible to the compiler
- **Fix:** Changed `std::unique_ptr<ComboAtt>` to `std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>` in the header declaration
- **Files modified:** Source/PluginEditor.h
- **Verification:** Build succeeded with 0 errors after fix
- **Committed in:** e4d066b (task commit)

**2. [Rule 1 - Bug] arpStep wrapped at seqLen instead of arpLen for pattern index**
- **Found during:** Post-UAT playback verification
- **Issue:** patStep = arpStep_ % seqLen used the voice-count denominator; steps beyond arpLen could be reached at engine level regardless of LEN setting
- **Fix:** Changed pattern index wrap to arpStep_ % arpLen; voice cycling remains % seqLen (separate semantics). LEN combo change now triggers instant arpBlockBounds_ repaint.
- **Files modified:** Source/PluginProcessor.cpp, Source/PluginEditor.cpp
- **Verification:** Build clean; LEN=4 correctly stops at step 4 at runtime
- **Committed in:** f2b71d6

**3. [Rule 1 - Bug] TIE suppressed noteOff unconditionally instead of checking upcoming step**
- **Found during:** Post-UAT playback verification
- **Issue:** isTie guard suppressed the step-boundary noteOff whenever the current step was TIE; correct behavior requires checking whether the *upcoming* step is TIE (sustain across boundary)
- **Fix:** Read next step state at (arpStep_+1)%arpLen; suppress noteOff only when nextState==TIE
- **Files modified:** Source/PluginProcessor.cpp
- **Verification:** Build clean; TIE steps sustain correctly; TIE->ON boundary fires a new note-on
- **Committed in:** 312ac04

---

**Total deviations:** 3 auto-fixed (3 Rule 1 bugs)
**Impact on plan:** All fixes were correctness requirements. No scope creep.

## Issues Encountered

One build error on first compile: C2065 "ComboAtt: undeclared identifier" for arpLengthAtt_ in PluginEditor.h. Root cause was that the using alias `ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment` appears at line 526 but the new member was inserted at line 461. Fixed by using the full type name in the header.

## User Setup Required

Install the built VST3 (`do-reinstall.ps1` in admin PowerShell or manual copy from `build/ChordJoystick_artefacts/Release/VST3/`) before UAT.

## Next Phase Readiness

- Phase 45 complete (Plans 01 + 02). All ARP-01, ARP-02, ARP-03 requirements satisfied.
- arpStepState0..7, arpLength, randomSyncMode wired end-to-end (processor + editor); UAT approved.
- arpCurrentStep_ atomic available for future step-highlight improvements.
- Preset save/load verified working for step states and LEN.

---
*Phase: 45-arpeggiator-step-patterns-ui-redesign-lfo-heart-preset*
*Completed: 2026-03-08*
