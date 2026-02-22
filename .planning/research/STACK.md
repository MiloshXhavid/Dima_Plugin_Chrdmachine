# Stack Research

**Domain:** JUCE VST3 MIDI Generator Plugin (Windows) with SDL2 gamepad
**Researched:** 2026-02-22
**Confidence:** MEDIUM (WebSearch and WebFetch unavailable; based on codebase inspection + verified knowledge of JUCE/SDL2/VST3 ecosystem as of training cutoff August 2025; flag items marked LOW for external validation)

---

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| JUCE | 8.x — pin to a tagged release (e.g., `8.0.4`) | Plugin framework, APVTS, MIDI, GUI, Timer | Industry standard for VST3/AU/AAX on all platforms; provides `juce_add_plugin`, APVTS, GUI components, and DAW integration that would take months to replicate |
| CMake | 3.22+ | Build system | JUCE's own CMake API requires 3.22 minimum; FetchContent support, generator-expression support, and MSVC toolchain integration are mature at this version |
| SDL2 | 2.30.9 (static) | Gamepad/controller input | SDL2 is the proven cross-platform HID library for game controllers; SDL3 is NOT a drop-in replacement and has a different API; 2.30.9 is the latest stable SDL2 branch release |
| C++17 | — | Language standard | JUCE 8 requires C++17 minimum; C++17 is well-supported by MSVC 19.x, Clang-cl, and Clang; `std::atomic`, structured bindings, and `if constexpr` are all used in the codebase |
| MSVC 2022 (or Clang-cl 17+) | VS 2022 / LLVM 17+ | Compiler | VST3 SDK on Windows is tested against MSVC; JUCE CI uses MSVC for Windows builds; Clang-cl is a valid alternative with better diagnostics but may require extra CMake toolchain config |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Catch2 | 3.x (3.7+) | Unit testing for non-audio logic | Use for ChordEngine, ScaleQuantizer, and LooperEngine unit tests — these are pure logic classes with no DAW dependency |
| pluginval | Latest (v1.x) | Automated VST3 validation in DAW host simulator | Run before each release to catch host compatibility bugs (null playheads, channel layout rejections, state restore failures) |
| Inno Setup | 6.x | Windows installer for paid distribution | Free, scriptable, widely used by indie plugin developers on Windows; produces a proper .exe installer that places the VST3 in the correct system folder |
| PACE Eden / Gumroad | — | License enforcement / storefront | Gumroad handles payment + simple serial delivery for solo developer releases; defer DRM until sales volume warrants it |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Visual Studio 2022 | IDE + MSVC compiler | Required for Windows VST3; CMake generates a `.sln` that VS opens natively; use `cmake -G "Visual Studio 17 2022" -A x64` |
| CMake 3.22+ | Build orchestration | Use `cmake --preset` pattern if adding CI; `cmake -B build -G "Visual Studio 17 2022"` for local dev |
| pluginval | Host-agnostic VST3 validation | Download from Tracktion GitHub; run `pluginval.exe --validate ChordJoystick.vst3 --strictness-level 5` |
| Process Monitor / DebugView | DAW load debugging | When a plugin silently fails to load in Ableton/Reaper, ProcMon shows missing DLL chains; DebugView captures `DBG()` output |
| Dependency Walker / pedeps | DLL audit | Verify the VST3 bundle has zero external DLL dependencies before shipping (SDL2 must be statically linked) |

---

## Installation

```cmake
# CMakeLists.txt — correct setup (annotated with issues found in current codebase)

cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

include(FetchContent)

# JUCE: pin to a specific tag, NOT origin/master
# origin/master changes without notice and has broken plugins mid-development before.
# Check https://github.com/juce-framework/JUCE/releases for latest 8.x tag.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4          # <-- use a specific release tag
    GIT_SHALLOW    TRUE           # faster clone
)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(juce)

# SDL2: 2.30.9 is correct — this is the latest SDL2 (not SDL3)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.9
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED          OFF CACHE BOOL "" FORCE)
set(SDL_STATIC          ON  CACHE BOOL "" FORCE)
set(SDL_JOYSTICK        ON  CACHE BOOL "" FORCE)
set(SDL_GAMECONTROLLER  ON  CACHE BOOL "" FORCE)
# All other subsystems disabled — critical for plugin safety
set(SDL_HAPTIC          OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO           OFF CACHE BOOL "" FORCE)
set(SDL_VIDEO           OFF CACHE BOOL "" FORCE)
set(SDL_RENDER          OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR          OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)
```

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| FetchContent (pinned tag) | Git submodule | Submodule is fine and avoids re-cloning on clean builds; FetchContent with `GIT_SHALLOW TRUE` achieves similar speed; submodule gives more direct version control — choose submodule if the team is comfortable with git submodule workflow |
| FetchContent (pinned tag) | System-installed JUCE | Use installed JUCE (`find_package(JUCE)`) only when targeting Linux package managers or Nix; on Windows it adds setup friction for contributors |
| SDL2 static | SDL3 static | SDL3 has an entirely different API (no `SDL_GameControllerOpen`, uses `SDL_OpenGamepad`); migration would require rewriting `GamepadInput.cpp`; SDL2 is still maintained and appropriate for this project |
| MSVC 2022 | MinGW-w64 | MinGW produces `.dll` VST3 bundles that many DAWs reject (Ableton historically has MinGW compatibility issues with exception handling); MSVC is the safe choice for paid distribution |
| Catch2 v3 | Google Test | Both work for JUCE plugin testing; Catch2 has better REQUIRE/CHECK macros for musical assertions and integrates with CMake CTest cleanly via `catch_discover_tests` |
| Inno Setup | NSIS | Both produce Windows installers; Inno Setup has cleaner scripting syntax and is more actively maintained in 2025 |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `GIT_TAG origin/master` for JUCE | JUCE master can break plugin compilation between commits; "it worked yesterday" failures are common; a commit from master that changed AudioProcessor API broke hundreds of plugins in 2023 | Pin to a specific release tag: `8.0.4` or latest `8.x.y` |
| SDL3 | SDL3's `SDL_Gamepad` API is entirely different from SDL2's `SDL_GameController` API; all `SDL_GameControllerOpen`, `SDL_GameControllerGetAxis`, `SDL_CONTROLLER_AXIS_*` calls in `GamepadInput.cpp` would need rewriting; SDL3 also has a different initialization and event model | SDL2 release-2.30.9 (already in use) |
| SDL2 dynamic linking (DLL) in a plugin | A VST3 plugin is loaded into a host process; shipping SDL2.dll alongside the plugin requires the host's working directory to have that DLL, which is unreliable; static linking bundles SDL2 into the VST3 binary cleanly | `SDL_STATIC ON` (already configured correctly) |
| SDL_INIT_VIDEO in a plugin | SDL's video subsystem tries to create a window event loop and may interfere with the host's message pump; it also links additional Windows DLLs (opengl32, etc.) that are unnecessary | `SDL_INIT_GAMECONTROLLER` only (already correct in the codebase) |
| Projucer (`.jucer` files) | JUCE 8 CMake API is the supported path; Projucer and CMake cannot easily coexist; all JUCE documentation and CI examples now use CMake | CMake with `juce_add_plugin` |
| ASIO SDK for audio | This is a MIDI-only plugin with no audio buses; including ASIO adds licensing complexity with no benefit | No audio SDK needed; `isBusesLayoutSupported` correctly rejects all audio layouts |
| `juce_audio_utils` (if unused) | This module includes deprecated code and increases compile time; audit whether it is actually needed | Include only modules in use: `juce_audio_processors`, `juce_gui_basics` are the minimum for this plugin |
| Windows XP / Win32 only target | JUCE 8 requires Windows 10+; older targets reduce the available Windows API surface | Target Windows 10+ minimum |

---

## Stack Patterns by Variant

**If building for Windows distribution only (v1):**
- Use MSVC 2022 x64 release build
- Static link SDL2 (already done)
- Ship as a single `.vst3` folder (bundle directory structure required by VST3 spec)
- Installer places bundle in `%COMMONPROGRAMFILES%\VST3\`

**If adding unit tests later:**
- Add a separate CMake executable target (not a plugin target) that links `juce_audio_processors` without DAW context and `Catch2`
- ChordEngine, ScaleQuantizer, and LooperEngine are pure logic classes well-suited to standalone testing
- GamepadInput cannot be unit tested without a physical controller; use mock/interface pattern

**If adding macOS support (v2):**
- JUCE CMake handles AU/VST3 for macOS via the same `juce_add_plugin` call with `FORMATS VST3 AU`
- SDL2 builds on macOS via the same FetchContent approach
- Requires Apple Developer ID and notarization for distribution (separate effort)

---

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| JUCE 8.0.x | SDL2 2.30.x | No direct interaction; both are statically compiled into the plugin binary; no known conflicts |
| JUCE 8.0.x | C++17 | JUCE 8 requires C++17; C++20 is experimental in JUCE as of 2025 but not required |
| SDL2 2.30.9 | MSVC 2022 | Fully supported; SDL2's CMake build produces `SDL2-static` target cleanly with MSVC |
| JUCE 8.0.x | CMake 3.22 | JUCE 8 CMake API minimum is 3.22; `cmake_minimum_required(VERSION 3.22)` is correct |
| JUCE 8.0.x | Visual Studio 17 2022 (MSVC 19.3x) | Fully supported; JUCE CI validates against VS2022 |
| JUCE 8.0.x | Windows 10 / Windows 11 | Supported; Windows 11 Pro is the development target |
| Catch2 3.x | JUCE 8 standalone tests | Catch2 3.x requires C++14+; works fine alongside JUCE modules in a separate test executable |

---

## SDL2 in a DAW Plugin — Special Considerations

This is the highest-risk integration in the stack. The following are confirmed patterns from the existing code, with confidence notes.

### What the codebase does correctly (MEDIUM confidence)

1. **SDL includes isolated to `.cpp`** — `GamepadInput.h` uses forward declarations (`struct _SDL_GameController`); SDL headers only appear in `GamepadInput.cpp`. This prevents SDL type leakage into JUCE headers and reduces include-order conflicts.

2. **Only `SDL_INIT_GAMECONTROLLER` initialized** — Avoids SDL's video/audio subsystems which conflict with host message pumps and audio drivers.

3. **Polling via `juce::Timer` at 60 Hz** — Instead of a blocking SDL event loop (`SDL_WaitEvent`), the code uses `SDL_PollEvent` inside a `juce::Timer::timerCallback()`. This runs on the JUCE message thread, which is the correct thread for UI callbacks. No separate thread is created for SDL.

4. **Atomic state handoff** — All gamepad state is written to `std::atomic<float/bool>` members. The audio thread reads these without locks. This is the correct pattern for JUCE plugin threading.

5. **Static linking** — `SDL2-static` is linked; no `SDL2.dll` dependency at runtime.

### Known risks (LOW confidence — requires runtime validation)

1. **SDL_Init in plugin constructor** — `SDL_Init` is called in `GamepadInput()` which is called from `PluginProcessor()`. Some DAWs scan plugins by loading and immediately unloading them. SDL_Init/SDL_Quit pairs called rapidly during scanning may cause issues on some systems. Consider deferring SDL_Init until the editor is opened, or making it a no-op if already initialized.

2. **SDL_Quit in plugin destructor** — If multiple instances of the plugin are loaded, the second instance's destructor will call `SDL_Quit` and break the first instance's event loop. SDL uses a reference-count internally for some subsystems but `SDL_Quit` may not be reference-counted across DLL instances. Mitigation: use a process-wide singleton for `GamepadInput` or track SDL init with a static counter.

3. **Message pump interference** — No reports of `SDL_PollEvent` conflicting with JUCE's message loop on Windows as of 2025, but this is not officially documented as safe. The 60 Hz polling approach avoids blocking, which is the key safety property. LOW confidence that this will never cause issues in all DAW hosts.

4. **XInput vs DirectInput on Windows** — SDL2 on Windows can use XInput (Xbox controller protocol) or DirectInput. PS4 controllers require DirectInput or DS4Windows. SDL2's `SDL_GAMECONTROLLER` subsystem normalizes both, but PS4 support without DS4Windows is implementation-dependent on the SDL2 version. Flag for validation with a physical PS4 controller.

---

## Build and CI Considerations (Windows)

### Release build configuration

```cmake
# Add to CMakeLists.txt for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(ChordJoystick PRIVATE /O2 /GL)
    target_link_options(ChordJoystick PRIVATE /LTCG)
endif()
```

### VST3 bundle structure

JUCE's CMake output already creates the correct VST3 bundle directory on Windows:
```
ChordJoystick.vst3/
  Contents/
    x86_64-win/
      ChordJoystick.vst3   (the actual DLL)
```

`COPY_PLUGIN_AFTER_BUILD TRUE` in the CMakeLists.txt copies this bundle to the system VST3 folder automatically during development builds. This must be disabled for release/installer builds.

### Code signing

- Windows VST3 plugins do not require code signing to load in DAWs (unlike macOS notarization)
- However, Windows Defender SmartScreen will flag an unsigned `.exe` installer
- For Gumroad distribution: sign the Inno Setup installer with a code signing certificate (EV or OV)
- Certificate options: DigiCert, Sectigo — EV certificates get immediate SmartScreen reputation; OV certificates require a reputation period
- Cost: ~$300-500/year for EV; ~$100-200/year for OV

### Installer (Inno Setup)

Inno Setup script should:
1. Copy `ChordJoystick.vst3/` bundle to `{commoncf64}\VST3\`
2. Include an uninstaller
3. Optionally check if the DAW is running and warn before install
4. No DLL redistribution needed (SDL2 is statically linked)

---

## Sources

- Codebase inspection: `CMakeLists.txt`, `Source/GamepadInput.h`, `Source/GamepadInput.cpp`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` — HIGH confidence (direct evidence)
- `.planning/PROJECT.md` — HIGH confidence (authoritative project spec)
- JUCE 8 CMake API knowledge (training data, August 2025 cutoff) — MEDIUM confidence; verify current JUCE release tag at https://github.com/juce-framework/JUCE/releases
- SDL2 2.30.x lifecycle knowledge (training data) — MEDIUM confidence; verify SDL2 vs SDL3 maintenance status at https://github.com/libsdl-org/SDL/releases
- SDL2 in DAW plugin threading patterns — MEDIUM confidence for general patterns; LOW confidence for host-specific compatibility claims
- VST3 Windows installer practices — MEDIUM confidence (community knowledge, training data)
- Windows code signing practices — MEDIUM confidence (training data, verify current certificate providers and pricing)

---

*Stack research for: ChordJoystick — JUCE 8 VST3 MIDI Generator Plugin*
*Researched: 2026-02-22*
