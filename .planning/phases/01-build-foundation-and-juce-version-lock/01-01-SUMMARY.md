---
phase: 01-build-foundation-and-juce-version-lock
plan: 01
subsystem: infra
tags: [juce, cmake, vst3, msvc, static-crt, ableton, fetchcontent]

# Dependency graph
requires: []
provides:
  - Reproducible JUCE 8.0.4 build via CMake FetchContent with pinned tag
  - Static CRT (MultiThreaded) configured globally before FetchContent
  - Ableton-compatible BusesProperties constructor with disabled stereo dummy bus
  - isBusesLayoutSupported accepting 0 or 2 output channels
  - ChordJoystick.vst3 bundle installed to C:\Program Files\Common Files\VST3\
affects: [02-engine-validation, 03-core-midi-output, all downstream phases]

# Tech tracking
tech-stack:
  added: [JUCE 8.0.4 (pinned tag 8.0.4 / commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a), SDL2 2.30.9 static, Visual Studio 18 2026 Community (MSVC 14.50)]
  patterns: [Static CRT via CMAKE_MSVC_RUNTIME_LIBRARY before FetchContent, Runtime host detection via juce::PluginHostType()]

key-files:
  created: []
  modified:
    - CMakeLists.txt
    - Source/PluginProcessor.cpp

key-decisions:
  - "Used Visual Studio 18 2026 generator — VS 17 2022 and VS 16 2019 generators listed by CMake but no matching installation found on this machine"
  - "JUCE 8.0.4 BusesLayout API uses getMainInputChannels()/getMainOutputChannels() not getMainInputChannelCount()/getMainOutputChannelCount() — plan had the pre-8.0.4 names"
  - "GIT_SHALLOW TRUE is safe with named tag 8.0.4 and avoids full clone download"
  - "Ableton dummy bus marked false (disabled-by-default) so it carries no audio"

patterns-established:
  - "CRT setting: CMAKE_MSVC_RUNTIME_LIBRARY must precede all FetchContent calls to ensure JUCE internals compile with /MT"
  - "Host detection: juce::PluginHostType() runtime check for Ableton-specific workarounds, keeps other hosts unaffected"

requirements-completed: []

# Metrics
duration: 25min
completed: 2026-02-22
---

# Phase 01 Plan 01: Build Foundation and JUCE Version Lock Summary

**JUCE 8.0.4 pinned via FetchContent with static CRT, Ableton dummy-bus fix applied, ChordJoystick.vst3 compiled and installed with 0 errors using Visual Studio 18 2026**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-02-22T00:00:00Z
- **Completed:** 2026-02-22
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- CMakeLists.txt fixed: JUCE pinned to immutable tag 8.0.4 with GIT_SHALLOW TRUE, static CRT set globally before FetchContent (prevents LNK2038 mismatch)
- PluginProcessor.cpp fixed: constructor conditionally adds disabled stereo output bus when running in Ableton Live; isBusesLayoutSupported updated to accept 0 or 2 output channels
- Full build succeeded with 0 errors using MSVC 14.50 / Visual Studio 18 2026; ChordJoystick.vst3 installed to `C:\Program Files\Common Files\VST3\` via COPY_PLUGIN_AFTER_BUILD

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix CMakeLists.txt — pin JUCE tag and set global static CRT** - `7ec16ea` (chore)
2. **Task 2: Fix PluginProcessor.cpp — Ableton dummy bus + bus layout validator** - `fc60872` (fix)

## Files Created/Modified

- `CMakeLists.txt` - Pinned JUCE to 8.0.4, added GIT_SHALLOW TRUE, added static CRT setting before FetchContent
- `Source/PluginProcessor.cpp` - Added Ableton conditional BusesProperties, updated isBusesLayoutSupported to accept 0 or 2 output channels

## Decisions Made

- **Visual Studio 18 2026 generator:** CMake listed VS 17 2022 and VS 16 2019 as available generators but neither was installed. Only VS 18 2026 (Community) was present (installed 2026-02-21). Used "Visual Studio 18 2026" with `-A x64`.
- **Exact cmake command used:** `cmake -B "C:/Users/Milosh Xhavid/get-shit-done/build" -S "C:/Users/Milosh Xhavid/get-shit-done" -G "Visual Studio 18 2026" -A x64`
- **JUCE confirmed fetched:** Tag 8.0.4 / commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a (Nov 18 2024), confirmed from build/_deps/juce-src
- **GIT_SHALLOW TRUE:** Works correctly with named annotated tag 8.0.4

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed BusesLayout API method names for JUCE 8.0.4**
- **Found during:** Task 2 (build step — compile errors)
- **Issue:** Plan specified `getMainInputChannelCount()` and `getMainOutputChannelCount()` in `isBusesLayoutSupported`. These method names do not exist in JUCE 8.0.4's `BusesLayout` struct. Compiler errors: `C2039: "getMainInputChannelCount" ist kein Member von "juce::AudioProcessor::BusesLayout"`. The correct JUCE 8.0.4 API uses `getMainInputChannels()` and `getMainOutputChannels()` (verified in `juce_AudioProcessor.h` lines 363/366).
- **Fix:** Replaced `getMainInputChannelCount()` with `getMainInputChannels()` and `getMainOutputChannelCount()` with `getMainOutputChannels()` in `isBusesLayoutSupported`.
- **Files modified:** `Source/PluginProcessor.cpp`
- **Verification:** Build succeeded with 0 errors after fix; logic is equivalent — both return channel count as int.
- **Committed in:** `fc60872` (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 — API bug)
**Impact on plan:** Auto-fix was necessary for compilation. Plan was written against a JUCE version whose API used the longer method names; JUCE 8.0.4 uses the shorter names. No behavioral change.

## Issues Encountered

- **Stale CMakeCache.txt:** A partial prior configure had created a CMakeCache.txt with "Visual Studio 17 2022". This caused a generator mismatch error when re-running configure. Resolved by deleting the build directory before re-running configure.
- **CMake configure download time:** FetchContent downloaded JUCE 8.0.4 and SDL2 2.30.9 on first configure (~98 seconds). Expected per plan.

## User Setup Required

None — no external service configuration required.

## Build Results

- **Configure:** Success — Visual Studio 18 2026, MSVC 14.50.35717, Windows SDK 10.0.26100.0
- **JUCE tag fetched:** 8.0.4 (commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a, Nov 18 2024)
- **SDL2 version:** release-2.30.9-0-gc98c4fbff (static library)
- **Build:** Success — 0 errors, 1 warning (JUCE_DISPLAY_SPLASH_SCREEN ignored — not used in JUCE 8)
- **VST3 bundle:** `C:\Program Files\Common Files\VST3\ChordJoystick.vst3` — present and installed by COPY_PLUGIN_AFTER_BUILD
- **LNK2038 issues:** None — static CRT setting placed before FetchContent as required

## Next Phase Readiness

- Build is verified and reproducible. Ready to execute 01-02 (engine validation / first DAW test).
- Known issue remains: LooperEngine uses std::mutex on audio thread — deferred to Phase 05.
- Filter CC emitted unconditionally — deferred to Phase 06.
- SDL2 gamepad testing under DAW conditions still required (Phase 06).

---
*Phase: 01-build-foundation-and-juce-version-lock*
*Completed: 2026-02-22*

## Self-Check: PASSED

- FOUND: CMakeLists.txt
- FOUND: Source/PluginProcessor.cpp
- FOUND: .planning/phases/01-build-foundation-and-juce-version-lock/01-01-SUMMARY.md
- FOUND: ChordJoystick.vst3 in C:\Program Files\Common Files\VST3\
- FOUND: commit 7ec16ea (chore(01-01): pin JUCE to 8.0.4 and set global static CRT)
- FOUND: commit fc60872 (fix(01-01): add Ableton dummy bus and fix bus layout validator)
