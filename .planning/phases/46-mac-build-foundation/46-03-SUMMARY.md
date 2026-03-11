---
phase: 46-mac-build-foundation
plan: 03
subsystem: infra
tags: [cmake, xcodebuild, universal-binary, lipo, juce, sdl2, arm64, x86_64]

# Dependency graph
requires:
  - phase: 46-01
    provides: Xcode project generated via cmake -G Xcode
  - phase: 46-02
    provides: ParameterID version hints on all AudioParameter constructors
provides:
  - Universal Release build of VST3 and AU plugins (arm64 + x86_64 fat binary)
  - Plugins installed to ~/Library/Audio/Plug-Ins/ by POST_BUILD cmake commands
affects:
  - 46-04 (auval validation — requires installed AU universal binary)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "cmake must be configured with -DCMAKE_OSX_ARCHITECTURES=arm64;x86_64 on command line (not just CMakeLists.txt) to prevent SDL2 from injecting x86-only compiler flags (-mmmx/-msse/-msse2/-msse3) that break arm64 cross-compilation"
    - "Plugin product name must not contain shell-special characters (parentheses, brackets, etc.) — cmake Xcode generator escapes spaces with backslash but does NOT escape parentheses in generated post-build shell scripts"
    - "getRawParameterValue()->load() pattern required in ternary expressions mixing float with std::atomic<float> — clang is strict about this for cross-architecture builds"

key-files:
  created: []
  modified:
    - CMakeLists.txt
    - Source/PluginProcessor.cpp

key-decisions:
  - "Renamed PLUGIN_PRODUCT_NAME from 'Arcade Chord Control (BETA-Test)' to 'Arcade Chord Control Beta-Test' — cmake Xcode generator does not escape parentheses in post-build sh scripts, causing sh syntax error"
  - "cmake must be re-run with -DCMAKE_OSX_ARCHITECTURES=arm64;x86_64 on command line; the CMakeLists.txt CACHE STRING approach is not sufficient because Xcode generator overrides it with NATIVE_ARCH_ACTUAL"
  - "SDL2 x86-only SIMD flags are added at cmake configure time based on host CPU — only suppressed when cmake configure itself runs with CMAKE_OSX_ARCHITECTURES set, which triggers SDL2 to skip x86 SIMD for cross-arch builds"
  - "ONLY_ACTIVE_ARCH=NO alone is insufficient for universal builds — ARCHS must also be set to arm64 x86_64 in the xcodeproj (done by cmake when CMAKE_OSX_ARCHITECTURES is set on command line)"
  - "Xcode 26.3 (Build 17C529) confirmed: universal binary builds work correctly when cmake is configured with CMAKE_OSX_ARCHITECTURES on the command line — the earlier research note about Xcode 16.2 AU regression does NOT apply to this Xcode version"

patterns-established:
  - "Universal binary cmake configure command: cmake -G Xcode -B build-mac -S . -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
  - "getRawParameterValue()->load() is the safe pattern for mixing APVTS values with plain float in ternary expressions"

requirements-completed: [MAC-01]

# Metrics
duration: 48min
completed: 2026-03-11
---

# Phase 46 Plan 03: Release Build + Universal Binary Verification Summary

**Universal Release build (arm64 + x86_64 fat binary) confirmed for both VST3 and AU plugins; three build blockers auto-fixed before success**

## Performance

- **Duration:** ~48 min (including 3 failed build attempts and cmake reconfigure)
- **Started:** 2026-03-11T13:16:43Z
- **Completed:** 2026-03-11T14:05:15Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- VST3 universal binary: `lipo -info` reports `x86_64 arm64`
- AU component universal binary: `lipo -info` reports `x86_64 arm64`
- Both plugin bundles installed to `~/Library/Audio/Plug-Ins/` by POST_BUILD cmake commands
- xcodebuild exits 0 with `** BUILD SUCCEEDED **`
- Three build blockers resolved: ternary ambiguity in PluginProcessor.cpp, parentheses in plugin name, SDL2 x86 SIMD flags breaking arm64 cross-compilation

## Task Commits

Each task was committed atomically:

1. **Task 1: Build Release with xcodebuild ONLY_ACTIVE_ARCH=NO** - `1aa4b70` (fix)
2. **Task 2: Verify universal binaries with lipo -info** - verification only, no files changed

**Plan metadata:** (docs commit — see below)

## Files Created/Modified
- `/Users/student/Documents/Dimea_Arcade_Chord_Control/CMakeLists.txt` — renamed PLUGIN_PRODUCT_NAME to remove parentheses from shell scripts
- `/Users/student/Documents/Dimea_Arcade_Chord_Control/Source/PluginProcessor.cpp` — fixed 4 ambiguous ternary expressions using `->load()` on atomic<float>

## Decisions Made
- Renamed plugin from `"Arcade Chord Control (BETA-Test)"` to `"Arcade Chord Control Beta-Test"` to avoid cmake Xcode generator parentheses-escaping bug in generated post-build shell scripts
- cmake must be invoked with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` on the command line (CMakeLists.txt CACHE STRING not sufficient — Xcode generator overrides with NATIVE_ARCH_ACTUAL when not set on command line at configure time)
- build-mac/ must be wiped and reconfigured when CMAKE_OSX_ARCHITECTURES was previously empty in cache — SDL2 bakes x86 SIMD flags at configure time

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed 4 ambiguous ternary expressions in PluginProcessor.cpp**
- **Found during:** Task 1 (Build Release with xcodebuild)
- **Issue:** Lines 1094, 1108, 1155, 1169: ternary operator had `float` (lfoX/YRateOverride_, lfoX/YPhaseOverride_) on one branch and `std::atomic<float>` (from `*apvts.getRawParameterValue(...)` dereference) on other branch. Clang for x86_64 cross-compilation rejected this as ambiguous — error: "conditional expression is ambiguous; 'float' can be converted to 'std::atomic<float>' and vice versa"
- **Fix:** Changed `*apvts.getRawParameterValue(ParamID::X)` to `apvts.getRawParameterValue(ParamID::X)->load()` in all 4 ternary expressions
- **Files modified:** Source/PluginProcessor.cpp
- **Verification:** Build proceeded past PluginProcessor.cpp compile step
- **Committed in:** 1aa4b70 (Task 1 commit)

**2. [Rule 3 - Blocking] Renamed plugin product name to remove parentheses**
- **Found during:** Task 1, second build attempt (post-build script failure)
- **Issue:** cmake Xcode generator generates post-build shell scripts using backslash to escape spaces but does NOT escape parentheses. The generated script line `cmake -E remove -f /path/Arcade\ Chord\ Control\ (BETA-Test).vst3/...` caused sh syntax error: "syntax error near unexpected token `('" because `(BETA-Test)` is interpreted as a subshell
- **Fix:** Changed `PLUGIN_PRODUCT_NAME` in CMakeLists.txt from `"Arcade Chord Control (BETA-Test)"` to `"Arcade Chord Control Beta-Test"`. Re-ran cmake to regenerate xcodeproj
- **Files modified:** CMakeLists.txt
- **Verification:** Post-build script ran successfully, no syntax error
- **Committed in:** 1aa4b70 (Task 1 commit)

**3. [Rule 3 - Blocking] Fresh cmake configure with CMAKE_OSX_ARCHITECTURES on command line**
- **Found during:** Task 1, third build attempt (SDL2 arm64 compile failures)
- **Issue:** When cmake was previously configured without explicit `CMAKE_OSX_ARCHITECTURES` (or with empty cache value), SDL2's cmake detected the x86_64 host CPU and added `-mmmx -msse -msse2 -msse3` flags per-file. These flags are baked into the Xcode project's compile response files and cannot be overridden at xcodebuild invocation time. When building arm64 targets, clang rejected these x86-only flags with: "unsupported option '-mmmx' for target 'arm64-apple-macos26.2'"
- **Fix:** Wiped `build-mac/` directory entirely. Re-ran cmake with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` on the command line. cmake now sets `ARCHS = (arm64,x86_64)` and `ONLY_ACTIVE_ARCH = NO` directly in xcodeproj. SDL2 does not add x86-only SIMD flags when configured for multi-arch
- **Files modified:** CMakeLists.txt (already had `if(APPLE)` block from Plan 01, cmake cache regenerated)
- **Verification:** `grep "mmmx\|msse" build-mac/ChordJoystick.xcodeproj/project.pbxproj` returns empty; build succeeded for both architectures
- **Committed in:** 1aa4b70 (part of same fix commit)

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking)
**Impact on plan:** All auto-fixes necessary for the build to complete. The root cause of all three issues was the same: the project had not previously been built as a universal binary on this Xcode version, so these build system incompatibilities had not been encountered before. No scope creep.

## Issues Encountered
- Xcode 26.3 generates `-target x86_64-apple-macos26.2` for x86_64 objects (macOS 26 Sequoia SDK). This is new compared to earlier research references to macOS 11/12. No incompatibilities found beyond the SDL2 SIMD flag issue.
- The background task `run_in_background=true` for xcodebuild worked correctly for subsequent builds. The first background run had a shell parsing issue with multi-line commands using `tee` + `echo "exit: $?"` combined — simplified to single-line command for subsequent runs.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- AU universal binary is installed at `~/Library/Audio/Plug-Ins/Components/Arcade Chord Control Beta-Test.component`
- Plan 04 (auval validation) can proceed — AU universal binary is present
- NOTE: Plugin product name is now `Arcade Chord Control Beta-Test` (no parentheses). Any DAW preset or save file that referenced the old name `Arcade Chord Control (BETA-Test)` will need the name updated
- cmake configure command for future builds: `cmake -G Xcode -B build-mac -S . -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`
- xcodebuild command for future builds: `xcodebuild -project build-mac/ChordJoystick.xcodeproj -scheme ChordJoystick_All -configuration Release ONLY_ACTIVE_ARCH=NO`

## Self-Check: PASSED

- CMakeLists.txt: FOUND
- Source/PluginProcessor.cpp: FOUND
- 46-03-SUMMARY.md: FOUND
- VST3 build dir: FOUND
- Commit 1aa4b70: FOUND
- VST3 binary lipo: x86_64 arm64 (UNIVERSAL)
- AU component lipo: x86_64 arm64 (UNIVERSAL)

---
*Phase: 46-mac-build-foundation*
*Completed: 2026-03-11*
