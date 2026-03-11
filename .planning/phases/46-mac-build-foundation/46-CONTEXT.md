# Phase 46: Mac Build Foundation - Context

**Gathered:** 2026-03-11
**Status:** Ready for planning

<domain>
## Phase Boundary

Get the plugin compiling as a universal binary (arm64+x86_64) producing VST3 and AU formats on macOS, passing `auval` validation with zero errors, and confirmed working in Logic Pro (AU), Reaper (VST3), and Ableton (VST3). No new features — pure build system fixes and validation work.

</domain>

<decisions>
## Implementation Decisions

### Build directory
- Wipe `build-mac/` entirely and start fresh — stale CMakeCache from the failed attempt must not carry forward
- Reconfigure from scratch with `cmake -G Xcode`

### Root cause of the build error (confirmed)
- `configure_file` for the Inno Setup `.iss.in` template (CMakeLists.txt line 5-9) is NOT wrapped in `if(WIN32)` — CMake tries to read the Windows installer template on Mac and fails
- Fix: wrap that block in `if(WIN32) ... endif()` before any other changes
- This is MAC-05

### Universal binary
- `CMAKE_OSX_ARCHITECTURES "arm64;x86_64"` MUST be set before the FetchContent calls (before line 19 in CMakeLists.txt)
- Setting it after FetchContent produces single-arch JUCE/SDL2 static libs and a linker failure — do not move it

### Xcode version strategy
- Use Xcode 26.3 as-is (already installed)
- After build, run `lipo -info` on both the `.vst3` binary and `.component` binary
- If both report `x86_64 arm64` → success
- If either is single-arch → Xcode regression confirmed; handle in a follow-up (install Xcode 15.x or investigate workaround)
- Do NOT pre-optimize for this — verify first

### auval preparation
- All APVTS parameters need version hints set (`setVersionHint(1)` or equivalent) before AU can pass auval with zero warnings
- Manufacturer code `MxCJ`, plugin code `DCJM` → `auval -v aumu DCJM MxCJ`
- AU main type is already `kAudioUnitType_MusicDevice` in CMakeLists.txt

### DAW smoke test scope
- All 3 target DAWs are available on this Mac: Logic Pro (AU), Reaper (VST3), Ableton Live (VST3)
- Each must detect the plugin in an instrument slot and show MIDI output in the DAW MIDI monitor
- Logic is the AU smoke test; Reaper and Ableton are VST3

### Gamepad test
- PS4/Xbox controller connected via USB must be detected: `SDL_NumJoysticks() > 0` and controller type label visible in plugin UI

### Claude's Discretion
- How to set APVTS parameter version hints (exact method/location in PluginProcessor.cpp)
- Whether DEPLOYMENT_TARGET needs to be set (e.g. macOS 11.0 minimum)
- SDL2 compile flags on Apple Silicon (should work via FetchContent with OSX_ARCHITECTURES set)

</decisions>

<code_context>
## Existing Code Insights

### Confirmed build error location
- `CMakeLists.txt` line 5-9: unconditional `configure_file` for Inno Setup `.iss.in` — fails on Mac
- Fix: wrap in `if(WIN32) ... endif()`

### CMAKE_OSX_ARCHITECTURES placement
- Must be inserted before line 19 (FetchContent_Declare juce) in CMakeLists.txt
- The MSVC static CRT block (lines 12-14) is already inside `if(MSVC)` — safe model to follow

### Already correct in CMakeLists.txt
- `FORMATS VST3 AU` — AU already declared
- `HARDENED_RUNTIME_ENABLED TRUE` — already set
- `PLUGIN_MANUFACTURER_CODE "MxCJ"`, `PLUGIN_CODE "DCJM"` — matches auval invocation
- `AU_MAIN_TYPE "kAudioUnitType_MusicDevice"` — correct for MIDI instrument
- `elseif(APPLE)` block already has copy-after-build commands for VST3 → `~/Library/Audio/Plug-Ins/VST3/` and AU → `~/Library/Audio/Plug-Ins/Components/`
- Windows-specific Reaper cache wipe and VST3 copy are inside `if(WIN32)` — safe on Mac

### APVTS parameters
- ~50+ parameter IDs in `PluginProcessor.cpp` — all need version hints for auval
- No `setVersionHint` calls found in current codebase

### stale artifacts
- `build-mac/` exists with failed CMakeCache.txt and partial Xcode project — must delete before reconfigure

</code_context>

<specifics>
## Specific Ideas

- The build error is well-understood: the Inno Setup `configure_file` is not guarded. Fix that first, then set `CMAKE_OSX_ARCHITECTURES`, then clean configure.
- `lipo -info` verification is the gate before moving to auval — if single-arch, stop and investigate Xcode before continuing.

</specifics>

<deferred>
## Deferred Ideas

- None — discussion stayed within phase scope

</deferred>

---

*Phase: 46-mac-build-foundation*
*Context gathered: 2026-03-11*
