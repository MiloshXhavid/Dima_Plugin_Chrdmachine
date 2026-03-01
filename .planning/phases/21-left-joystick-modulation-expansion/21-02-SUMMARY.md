---
phase: 21-left-joystick-modulation-expansion
plan: 02
subsystem: ui
tags: [juce, combobox, timer, cpp17, layout]

# Dependency graph
requires:
  - phase: 21-01
    provides: 6-item filterXMode/filterYMode APVTS choice params registered in PluginProcessor
provides:
  - filterXModeBox_ with 6 items: Cutoff (CC74), VCF LFO (CC12), LFO-X Freq, LFO-X Phase, LFO-X Level, Gate Length
  - filterYModeBox_ with 6 items: Resonance (CC71), LFO Rate (CC76), LFO-Y Freq, LFO-Y Phase, LFO-Y Level, Gate Length
  - resized() layout: filterXModeBox_ bounds set above filterYModeBox_ (X-above-Y per LJOY-03)
  - timerCallback Atten label relabeling: modes 0-1 show "Atten"/" %", modes 2-5 show Hz/Phase/Level/Gate unit labels with no "%" suffix
affects:
  - 21-03 (smoke test / validation; this UI wiring is what the user verifies)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "timerCallback label update with change-guard: if (label.getText() != newText) styleLabel(label, newText) — avoids 30Hz unnecessary repaint churn"
    - "ComboBox 6-item registration matching APVTS 6-item StringArray: item IDs 1-6 (1-based) map to APVTS indices 0-5"

key-files:
  created: []
  modified:
    - Source/PluginEditor.cpp

key-decisions:
  - "drawAbove() labels (LEFT X / LEFT Y) auto-follow component position — no change needed when layout order swaps"
  - "filterXModeBox_ item 2 string updated from 'VCF LFO   (CC12)' to 'VCF LFO (CC12)' (spacing cleanup) — functionally identical"

patterns-established:
  - "timerCallback mode-indexed label: const juce::String label = (mode == N) ? 'text' : ... — readable ternary chain gated by change-check"

requirements-completed: [LJOY-01, LJOY-02, LJOY-03, LJOY-04]

# Metrics
duration: 15min
completed: 2026-03-01
---

# Phase 21 Plan 02: Left Joystick UI Wiring Summary

**6-item ComboBoxes for filterX/Y mode selectors with X-above-Y layout swap and Atten label/suffix relabeling in timerCallback — matching APVTS 6-item choice params from plan 21-01**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-03-01T05:14:05Z
- **Completed:** 2026-03-01T05:30:00Z
- **Tasks:** 1 (+ checkpoint:human-verify pending)
- **Files modified:** 1

## Accomplishments

- Replaced 3-item filterXModeBox_ (Cutoff/VCF LFO/Mod Wheel) with 6-item list matching APVTS: Cutoff (CC74), VCF LFO (CC12), LFO-X Freq, LFO-X Phase, LFO-X Level, Gate Length
- Replaced 2-item filterYModeBox_ (Resonance/LFO Rate) with 6-item list: Resonance (CC71), LFO Rate (CC76), LFO-Y Freq, LFO-Y Phase, LFO-Y Level, Gate Length
- Swapped resized() layout so filterXModeBox_ is positioned above filterYModeBox_ (LJOY-03)
- Added timerCallback block: relabels filterXAttenLabel_/filterYAttenLabel_ to mode-appropriate unit (Hz/Phase/Level/Gate/Atten) and removes " %" suffix for LFO/Gate modes — guarded by change-checks to avoid 30Hz churn
- Plugin compiles and links cleanly: VST3 DLL at `build/ChordJoystick_artefacts/Release/VST3/DIMEA CHORD JOYSTICK MK2.vst3/Contents/x86_64-win/` stamped Mar 1 06:14

## Task Commits

Each task was committed atomically:

1. **Task 1: Replace ComboBox items, swap resized() layout, add timerCallback label relabeling** - `6c21b78` (feat)

## Files Created/Modified

- `Source/PluginEditor.cpp` - 6-item filterXModeBox_ and filterYModeBox_ items; resized() X-above-Y swap; timerCallback Atten label/suffix update block

## Decisions Made

- **drawAbove() auto-follows component:** The `drawAbove(filterXModeBox_, "LEFT X")` and `drawAbove(filterYModeBox_, "LEFT Y")` calls in paint() use the component's `getX()/getY()` directly — no change needed when the layout order swaps, each label correctly appears above its own box.
- **Spacing cleanup on item 2 strings:** Previous items used padded strings ("Cutoff    (CC74)") for alignment in the old narrow 3-item list. The new 6-item strings use natural spacing — visually cleaner in the wider dropdown.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Build post-step "Permission denied" / MSB3073 when copying VST3 to `C:\Program Files\Common Files\VST3` — pre-existing UAC issue documented in MEMORY.md (fix-install.ps1). Compilation and linking succeeded cleanly. VST3 DLL freshly built at build/ChordJoystick_artefacts.

## User Setup Required

To deploy for DAW verification: run `fix-install.ps1` as Administrator (or manually copy the VST3 from `build/ChordJoystick_artefacts/Release/VST3/` to `C:\Program Files\Common Files\VST3\`). Then rescan plugins in DAW.

## Next Phase Readiness

- UI wiring complete — checkpoint:human-verify needed to confirm visual and functional behavior in DAW
- All 6-item ComboBox items match the APVTS StringArray registered in plan 21-01
- No blockers

---
*Phase: 21-left-joystick-modulation-expansion*
*Completed: 2026-03-01*

## Self-Check: PASSED

- FOUND: `Source/PluginEditor.cpp` (6-item filterXModeBox_ + filterYModeBox_ + resized() swap + timerCallback label update)
- FOUND: commit `6c21b78` (Task 1 — UI wiring)
- FOUND: VST3 DLL timestamped Mar 1 06:14 (compile + link succeeded)
