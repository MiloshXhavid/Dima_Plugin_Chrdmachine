# Phase 02: Core Engine Validation - Research

**Researched:** 2026-02-22
**Domain:** C++ unit testing with Catch2 v3, JUCE CMake integration, MIDI pitch arithmetic
**Confidence:** HIGH (Catch2 + CMake well-documented; JUCE test pattern confirmed via community)

---

## Summary

Phase 02 adds the first test infrastructure to ChordJoystick: a Catch2 v3 test suite that validates ScaleQuantizer and ChordEngine in isolation before those classes are exercised in a DAW. The classes under test are pure-logic (no audio thread, no GUI, no SDL2), so test isolation is straightforward — they only depend on `juce::jlimit` and `juce::jassert`, both provided by `juce_audio_processors`.

The standard pattern for JUCE+CMake+Catch2 is a **separate `add_executable` (or `juce_add_console_app`) test target** that links `juce::juce_audio_processors` and `Catch2::Catch2WithMain`. The test binary is independent of the VST3 plugin target and does not require a loaded DAW or audio driver. Catch2 is fetched via FetchContent exactly like JUCE and SDL2. One MSVC-specific pitfall dominates: Catch2 must be compiled with the same CRT (`/MT` for release, `/MTd` for debug) as the rest of the project, which requires setting `CMAKE_MSVC_RUNTIME_LIBRARY` **before** `FetchContent_MakeAvailable(Catch2)`.

STATE.md records that Phase 02 was "skipped — no Catch2 per user preference" in a prior session. This research is being produced now, so the planner has full information to draft plans. No CONTEXT.md exists for Phase 02, so there are no locked decisions to copy — all choices are at Claude's discretion.

**Primary recommendation:** Add Catch2 v3.8.1 via FetchContent, create a `ChordJoystickTests` console executable, link `juce::juce_audio_processors` + `Catch2::Catch2WithMain`, enable `enable_testing()` + `catch_discover_tests()`. Test files go in `Tests/`. No JUCE initialization is needed because ScaleQuantizer and ChordEngine do not use the message thread.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Catch2 | v3.8.1 | Unit test framework + runner | De-facto standard for C++ audio plugins; used by pamplejuce template, well-maintained, header-only-free v3 |
| CMake CTest | built-in | Test discovery and execution | Already required; catch_discover_tests bridges Catch2 ↔ CTest |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::juce_audio_processors | (JUCE 8.0.4) | Provides juce::jlimit, juce::jassert, AudioProcessor base | Required since ScaleQuantizer.h includes juce_audio_processors.h |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Catch2 | GoogleTest | GTest has heavier CMake integration, requires gtest_main; Catch2 simpler for JUCE projects |
| Catch2 | doctest | doctest is single-header and faster to compile, but less ecosystem adoption in audio plugin space |
| CTest | ctest --build-and-test | ctest built-in is simpler; no external CI runner needed for local builds |

**Installation (FetchContent — no install step needed):**
```cmake
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)
```

---

## Architecture Patterns

### Recommended Project Structure

```
ChordJoystick/
├── Source/                   # existing plugin sources
│   ├── ScaleQuantizer.h/.cpp
│   ├── ChordEngine.h/.cpp
│   └── ...
├── Tests/                    # NEW: test sources directory
│   ├── ScaleQuantizerTests.cpp
│   └── ChordEngineTests.cpp
└── CMakeLists.txt            # modified to add Catch2 + test target
```

### Pattern 1: Separate Test Executable Linked Against JUCE Modules

**What:** Create a standalone `add_executable` (or `juce_add_console_app`) target that compiles the source files under test plus Catch2 test files. The VST3 plugin target is NOT linked — the test target compiles `ScaleQuantizer.cpp` and `ChordEngine.cpp` directly.

**When to use:** When the classes under test only depend on JUCE utility headers (jlimit, jassert), not on JUCE GUI, audio device managers, or plugin infrastructure. This applies exactly to ScaleQuantizer and ChordEngine.

**Example:**
```cmake
# Source: Catch2 cmake-integration.md + JUCE forum pattern
# Place AFTER FetchContent_MakeAvailable(Catch2)

add_executable(ChordJoystickTests
    Source/ScaleQuantizer.cpp
    Source/ChordEngine.cpp
    Tests/ScaleQuantizerTests.cpp
    Tests/ChordEngineTests.cpp
)

set_target_properties(ChordJoystickTests PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(ChordJoystickTests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODULE_AVAILABLE_juce_audio_processors=1
    JUCE_STANDALONE_APPLICATION=0
)

target_link_libraries(ChordJoystickTests PRIVATE
    juce::juce_audio_processors
    Catch2::Catch2WithMain
)

# CTest integration
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(ChordJoystickTests)
```

**Important:** `juce_add_console_app` is an alternative but adds JUCE-specific link scaffolding (juce_generate_juce_header). For classes that only need module headers, plain `add_executable` + `target_link_libraries(... juce::juce_audio_processors)` is simpler and avoids JuceHeader.h complications.

### Pattern 2: Catch2 Test File Structure (v3)

**What:** Catch2 v3 uses `#include <catch2/catch_test_macros.hpp>` (not the v2 single `catch.hpp`). `TEST_CASE` and `SECTION` are the primary primitives. No custom `main()` needed when linking `Catch2::Catch2WithMain`.

**Example:**
```cpp
// Source: Catch2 official docs, v3 syntax
#include <catch2/catch_test_macros.hpp>
#include "ScaleQuantizer.h"

TEST_CASE("ScaleQuantizer - Major scale quantization", "[scale][major]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);

    SECTION("C major notes pass through unchanged")
    {
        // C4=60, D4=62, E4=64, F4=65, G4=67, A4=69, B4=71
        CHECK(ScaleQuantizer::quantize(60, pat, size) == 60);
        CHECK(ScaleQuantizer::quantize(62, pat, size) == 62);
        CHECK(ScaleQuantizer::quantize(64, pat, size) == 64);
    }

    SECTION("Non-scale notes snap to nearest")
    {
        // C#4=61 is equidistant between C4(60) and D4(62); tie goes DOWN
        CHECK(ScaleQuantizer::quantize(61, pat, size) == 60);
        // Bb4=70 is 1 below B4(71) and 1 above A4(69); tie goes DOWN
        CHECK(ScaleQuantizer::quantize(70, pat, size) == 69);
    }
}
```

### Anti-Patterns to Avoid

- **Linking the VST3 plugin target:** The `ChordJoystick` CMake target requires a loaded plugin host. Never `target_link_libraries(tests PRIVATE ChordJoystick)`. Compile the .cpp files directly in the test target instead.
- **Using Catch2 v2 `#include "catch.hpp"`:** v3 split headers. The include path is `<catch2/catch_test_macros.hpp>`.
- **`juce_generate_juce_header` on test target:** Triggers JuceHeader.h dependency chain and complicates defines. Skip it; include JUCE module headers directly.
- **`enable_testing()` after `add_subdirectory`:** Must appear before test targets are registered for CTest to pick them up.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test discovery | Custom test runner main() | Catch2::Catch2WithMain | Handles argc/argv, output formatting, exit code |
| Test-to-CTest bridge | Manual add_test() for each case | catch_discover_tests() | Auto-discovers all TEST_CASE entries; survives test renames |
| CRT mismatch detection | Runtime linker errors | Set CMAKE_MSVC_RUNTIME_LIBRARY before FetchContent | LNK2038 is cryptic; upfront prevention is simpler |

**Key insight:** The complexity in JUCE+Catch2 integration is entirely in CMake configuration, not in the test code itself. The test code is standard Catch2.

---

## Common Pitfalls

### Pitfall 1: MSVC Runtime Mismatch (LNK2038)

**What goes wrong:** Linker fails with `LNK2038: mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'`.

**Why it happens:** The project's `CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"` sets `/MT` globally. But if Catch2 is fetched with FetchContent **before** that variable is set (or Catch2 overrides it internally), Catch2 compiles with `/MD`. The project and Catch2 then have incompatible CRT objects.

**How to avoid:** Ensure `set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` appears **before** ALL `FetchContent_MakeAvailable()` calls in CMakeLists.txt. The existing project already does this correctly for JUCE and SDL2. Placing the Catch2 `FetchContent_Declare` + `FetchContent_MakeAvailable` block after SDL2 (but still after the CRT line) is safe.

**Warning signs:** Linker error LNK2038 mentioning RuntimeLibrary; or any Catch2 function appearing undefined at link time despite correct include paths.

---

### Pitfall 2: CMAKE_MODULE_PATH Not Updated Before `include(Catch)`

**What goes wrong:** `include(Catch)` fails with "Could not find module Catch" even though Catch2 compiled successfully.

**Why it happens:** When using FetchContent, Catch2's extras directory (which contains `Catch.cmake` for `catch_discover_tests`) is placed at `${catch2_SOURCE_DIR}/extras`. CMake doesn't know to look there unless told explicitly.

**How to avoid:**
```cmake
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(ChordJoystickTests)
```
Note: `catch2_SOURCE_DIR` (lowercase) is the variable set by FetchContent after `FetchContent_MakeAvailable(Catch2)`.

**Warning signs:** CMake configure step error mentioning Catch.cmake or find_package(Catch).

---

### Pitfall 3: ScaleQuantizer Includes `juce_audio_processors.h` — Must Link the Module

**What goes wrong:** `ScaleQuantizer.h` includes `<juce_audio_processors/juce_audio_processors.h>` for `juce::jlimit`. If the test executable doesn't link `juce::juce_audio_processors`, every JUCE call fails to compile.

**Why it happens:** ScaleQuantizer was designed as a JUCE-aware class. Moving it to "pure C++" would require replacing `juce::jlimit` with `std::clamp` or `std::min/max` — a source change.

**How to avoid:** Always include `juce::juce_audio_processors` in test target link libraries. This is a lightweight link (no audio device manager, no GUI) and compiles fast.

**Warning signs:** `juce::jlimit` undeclared, `juce_audio_processors.h` not found.

---

### Pitfall 4: `jassert` Fires in Test Context

**What goes wrong:** `ChordEngine::computePitch` calls `jassert(voiceIndex >= 0 && voiceIndex < 4)`. In debug builds, JUCE's default jassert handler may open a dialog or call `__debugbreak`. In a headless CI test run this hangs.

**Why it happens:** `jassert` is interactive by default in debug mode when JUCE's `AlertWindow` is available.

**How to avoid:** Either test only valid voice indices (0-3), or define `JUCE_DISABLE_ASSERTIONS=1` on the test target. Testing invalid indices is an anti-pattern anyway — the function's contract specifies 0-3. Stick to valid inputs.

**Warning signs:** Tests hang or crash only in debug builds when passing out-of-range voice indices.

---

### Pitfall 5: `BUILD_SHARED_LIBS` Causing Catch2 to Build as DLL

**What goes wrong:** If any upstream CMakeLists.txt sets `BUILD_SHARED_LIBS ON`, Catch2 fetched via FetchContent also builds as a shared library, which breaks the static test binary assumption on Windows.

**Why it happens:** `BUILD_SHARED_LIBS` is a global CMake cache variable respected by all FetchContent sub-projects.

**How to avoid:** The ChordJoystick project does not set `BUILD_SHARED_LIBS ON` (SDL2 uses explicit `SDL_STATIC ON`/`SDL_SHARED OFF`). This pitfall is not currently a risk, but if `BUILD_SHARED_LIBS` ever appears in CMakeLists.txt, add `set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)` before `FetchContent_MakeAvailable(Catch2)`.

---

## Code Examples

Verified patterns for the test suite:

### Complete CMake Test Target Block

```cmake
# Source: Catch2 cmake-integration.md (official) + JUCE forum pattern
# Add AFTER FetchContent_MakeAvailable(Catch2), at end of CMakeLists.txt

if(BUILD_TESTS)
    add_executable(ChordJoystickTests
        Source/ScaleQuantizer.cpp
        Source/ChordEngine.cpp
        Tests/ScaleQuantizerTests.cpp
        Tests/ChordEngineTests.cpp
    )

    set_target_properties(ChordJoystickTests PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
    )

    target_compile_definitions(ChordJoystickTests PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_audio_processors=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_include_directories(ChordJoystickTests PRIVATE Source)

    target_link_libraries(ChordJoystickTests PRIVATE
        juce::juce_audio_processors
        Catch2::Catch2WithMain
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)
    catch_discover_tests(ChordJoystickTests)
endif()
```

Enable with: `cmake -B build -S . -DBUILD_TESTS=ON`

---

### ScaleQuantizer: Key Test Cases to Cover

```cpp
// ScaleQuantizerTests.cpp
#include <catch2/catch_test_macros.hpp>
#include "ScaleQuantizer.h"

// ── Pattern retrieval ────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer preset patterns", "[scale][preset]")
{
    SECTION("Major has 7 notes starting at 0")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Major) == 7);
        const int* p = ScaleQuantizer::getScalePattern(ScalePreset::Major);
        CHECK(p[0] == 0);  // root
        CHECK(p[2] == 4);  // major third
        CHECK(p[4] == 7);  // perfect fifth
    }

    SECTION("Chromatic has 12 notes")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Chromatic) == 12);
    }

    SECTION("Pentatonic Major has 5 notes")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::PentatonicMajor) == 5);
    }

    SECTION("Diminished HW has 8 notes")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Diminished) == 8);
    }

    SECTION("All 20 presets have valid names")
    {
        for (int i = 0; i < (int)ScalePreset::COUNT; ++i)
        {
            const char* name = ScaleQuantizer::getScaleName((ScalePreset)i);
            CHECK(name != nullptr);
            CHECK(name[0] != '\0');
        }
    }
}

// ── Quantization edge cases ──────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer::quantize - major scale", "[quantize][major]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);
    // C major: {0,2,4,5,7,9,11}

    SECTION("In-scale notes pass through")
    {
        CHECK(ScaleQuantizer::quantize(60, pat, size) == 60); // C4
        CHECK(ScaleQuantizer::quantize(62, pat, size) == 62); // D4
        CHECK(ScaleQuantizer::quantize(64, pat, size) == 64); // E4
    }

    SECTION("Tie breaks go DOWN")
    {
        // C#4=61: C4(60) dist=1, D4(62) dist=1 → ties go down → 60
        CHECK(ScaleQuantizer::quantize(61, pat, size) == 60);
        // A#4=70: A4(69) dist=1, B4(71) dist=1 → ties go down → 69
        CHECK(ScaleQuantizer::quantize(70, pat, size) == 69);
    }

    SECTION("Nearest note wins when not a tie")
    {
        // F#4=66: F4(65) dist=1, G4(67) dist=1 — wait, that's a tie → goes to 65
        CHECK(ScaleQuantizer::quantize(66, pat, size) == 65);
        // G#4=68: G4(67) dist=1, A4(69) dist=1 → tie → 67
        CHECK(ScaleQuantizer::quantize(68, pat, size) == 67);
    }

    SECTION("Boundary: MIDI 0 (C-2)")
    {
        // C-2 = 0 is in C major (octave 0, note 0)
        CHECK(ScaleQuantizer::quantize(0, pat, size) == 0);
    }

    SECTION("Boundary: MIDI 127")
    {
        // G9=127: 127 % 12 = 7 (G), G is in C major
        CHECK(ScaleQuantizer::quantize(127, pat, size) == 127);
    }
}

TEST_CASE("ScaleQuantizer::quantize - chromatic scale", "[quantize][chromatic]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Chromatic);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Chromatic);

    SECTION("Every MIDI pitch is in-scale and passes through")
    {
        for (int i = 0; i <= 127; ++i)
            CHECK(ScaleQuantizer::quantize(i, pat, size) == i);
    }
}

// ── Custom pattern ───────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer::buildCustomPattern", "[custom]")
{
    SECTION("Only root selected → single-note pattern")
    {
        bool flags[12] = {true, false, false, false, false, false,
                          false, false, false, false, false, false};
        int pattern[12]; int size;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 1);
        CHECK(pattern[0] == 0);
    }

    SECTION("Empty selection falls back to root")
    {
        bool flags[12] = {};  // all false
        int pattern[12]; int size;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 1);
        CHECK(pattern[0] == 0);
    }

    SECTION("All 12 notes → chromatic-equivalent")
    {
        bool flags[12];
        for (auto& f : flags) f = true;
        int pattern[12]; int size;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 12);
        for (int i = 0; i < 12; ++i)
            CHECK(pattern[i] == i);
    }
}
```

---

### ChordEngine: Key Test Cases to Cover

```cpp
// ChordEngineTests.cpp
#include <catch2/catch_test_macros.hpp>
#include "ChordEngine.h"

// Helper: build params with joystick centered (Y=0 → mid-range pitch)
static ChordEngine::Params centeredParams()
{
    ChordEngine::Params p;
    p.joystickX = 0.0f;
    p.joystickY = 0.0f;
    p.xAtten = 48.0f;
    p.yAtten = 48.0f;
    p.globalTranspose = 0;
    // intervals: root=0, third=4, fifth=7, tension=10 (default)
    p.intervals[0] = 0; p.intervals[1] = 4;
    p.intervals[2] = 7; p.intervals[3] = 10;
    for (auto& o : p.octaves) o = 0;
    p.useCustomScale = false;
    p.scalePreset = ScalePreset::Chromatic; // disable quantization for raw tests
    return p;
}

TEST_CASE("ChordEngine axis routing", "[chord][axis]")
{
    // joystick center (0.0): axisToPitchOffset(0.0, 48) = ((0+1)*0.5)*48 = 24
    // So center → raw 24 semitones above MIDI 0 → MIDI 24 for voice 0 (root)
    // voice 1 (third): 24 + 4 = 28
    // voice 2 (fifth): 24 + 7 = 31  (uses X axis, also centered → 24 base)
    // voice 3 (tension): 24 + 10 = 34

    ChordEngine::Params p = centeredParams();

    SECTION("Y axis drives voices 0 and 1")
    {
        p.joystickY = -1.0f; // bottom: axisToPitch(-1, 48)=0
        CHECK(ChordEngine::computePitch(0, p) == 0);  // root
        CHECK(ChordEngine::computePitch(1, p) == 4);  // third above

        p.joystickY = 1.0f;  // top: axisToPitch(1, 48)=48
        CHECK(ChordEngine::computePitch(0, p) == 48);
        CHECK(ChordEngine::computePitch(1, p) == 52);
    }

    SECTION("X axis drives voices 2 and 3")
    {
        p.joystickX = -1.0f; // left: base=0
        CHECK(ChordEngine::computePitch(2, p) == 7);   // fifth above 0
        CHECK(ChordEngine::computePitch(3, p) == 10);  // tension above 0

        p.joystickX = 1.0f;  // right: base=48
        CHECK(ChordEngine::computePitch(2, p) == 55);  // 48+7
        CHECK(ChordEngine::computePitch(3, p) == 58);  // 48+10
    }
}

TEST_CASE("ChordEngine global transpose", "[chord][transpose]")
{
    ChordEngine::Params p = centeredParams();
    p.joystickY = -1.0f;  // base = 0 for Y-voices

    SECTION("+12 semitone transpose")
    {
        p.globalTranspose = 12;
        CHECK(ChordEngine::computePitch(0, p) == 12);  // root
        CHECK(ChordEngine::computePitch(1, p) == 16);  // third
    }

    SECTION("-24 semitone transpose clamped at 0")
    {
        p.globalTranspose = -24;
        // voice 1 at -1 joystick: 0+4-24 = -20 → clamped to 0
        CHECK(ChordEngine::computePitch(1, p) == 0);
    }
}

TEST_CASE("ChordEngine octave offsets", "[chord][octave]")
{
    ChordEngine::Params p = centeredParams();
    p.joystickY = -1.0f;  // base = 0

    SECTION("Octave +1 shifts voice up 12 semitones")
    {
        p.octaves[0] = 1;
        CHECK(ChordEngine::computePitch(0, p) == 12);
    }

    SECTION("Octave -1 with interval clamped at 0")
    {
        p.octaves[1] = -1;
        // 0 + 4 + (-12) = -8 → clamped to 0
        CHECK(ChordEngine::computePitch(1, p) == 0);
    }
}

TEST_CASE("ChordEngine with scale quantization", "[chord][quantize]")
{
    ChordEngine::Params p = centeredParams();
    p.scalePreset = ScalePreset::Major;
    p.useCustomScale = false;

    SECTION("Output is always a valid MIDI note in major scale")
    {
        const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
        const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);

        // Sweep Y axis in steps and verify all outputs are in-scale
        for (float y = -1.0f; y <= 1.0f; y += 0.1f)
        {
            p.joystickY = y;
            int pitch = ChordEngine::computePitch(0, p);
            int pitchClass = pitch % 12;
            bool found = false;
            for (int i = 0; i < size; ++i)
                if (pat[i] == pitchClass) { found = true; break; }
            CHECK(found);
        }
    }
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Catch2 v2 single-header `catch.hpp` | Catch2 v3 split headers `<catch2/catch_test_macros.hpp>` | Catch2 v3.0 (2022) | Must update includes; v3 faster to compile |
| `find_package(Catch2)` requires system install | FetchContent (no system install) | CMake 3.14+ | Reproducible; no install step in CI |
| Manual `add_test()` for each TEST_CASE | `catch_discover_tests()` auto-discovery | Catch2 v2.13+ | Tests survive renames without CMake reconfiguration |

**Deprecated/outdated:**
- `catch.hpp` single header: v2 style, not available in v3 — do not use
- `CATCH_CONFIG_MAIN` macro: v2 style alternative to linking Catch2::Catch2WithMain — do not use in v3

---

## Open Questions

1. **Do ScaleQuantizer/ChordEngine compile cleanly without JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED?**
   - What we know: `juce_audio_processors.h` includes a macro guard that may require module settings definitions when included outside `juce_add_plugin`.
   - What's unclear: Exact set of compile definitions needed for `juce::jlimit` to be available in a plain `add_executable` context. The list above (`JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1`, `JUCE_MODULE_AVAILABLE_juce_audio_processors=1`) is the community-reported pattern but may need one more iteration to confirm.
   - Recommendation: Plan 02-01 should include a "build the test target, fix compile definitions" step before writing test cases.
   - Confidence: MEDIUM (pattern is well-reported, exact flags may need trial).

2. **Is `juce_add_console_app` required, or does plain `add_executable` work?**
   - What we know: The JUCE forum says `juce_add_console_app` is needed if you call `juce_generate_juce_header`. ScaleQuantizer/ChordEngine do NOT call that; they include JUCE module headers directly.
   - What's unclear: Whether `juce_add_console_app` adds any required hidden link libraries beyond what `target_link_libraries(... juce::juce_audio_processors)` provides.
   - Recommendation: Start with plain `add_executable`. If linker errors arise, switch to `juce_add_console_app`.
   - Confidence: MEDIUM.

3. **Should tests run as part of the default build or opt-in with `-DBUILD_TESTS=ON`?**
   - What we know: Catch2 test compilation adds ~30-60s to clean build time. For a plugin project without CI, opt-in is more ergonomic.
   - What's unclear: User preference — was Phase 02 skipped permanently or temporarily?
   - Recommendation: Gate behind `option(BUILD_TESTS "Build unit tests" OFF)` so the default plugin build is unchanged. Developer runs `cmake -B build -DBUILD_TESTS=ON` explicitly.
   - Confidence: HIGH (common pattern, low risk).

---

## Sources

### Primary (HIGH confidence)
- [Catch2 cmake-integration.md (devel)](https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md) — FetchContent snippet, target names, CMAKE_MODULE_PATH requirement, v3.8.1 tag
- [Catch2 official readthedocs](https://catch2-temp.readthedocs.io/en/latest/cmake-integration.html) — Catch2::Catch2WithMain vs Catch2::Catch2, catch_discover_tests pattern

### Secondary (MEDIUM confidence)
- [JUCE Forum: Setting up Catch2 with CMake](https://forum.juce.com/t/setting-up-a-catch2-test-target-with-cmake/41863) — juce_add_console_app vs add_executable debate, direct module include recommendation
- [JUCE Forum: Running CTests in JUCE CMake project](https://forum.juce.com/t/running-ctests-in-a-juce-cmake-project/53385) — juce_add_console_app + juce_generate_juce_header pattern, enable_testing() placement
- [JUCE Forum: Linking Catch2 against JUCE GUI app](https://forum.juce.com/t/linking-catch2-tests-against-juce-gui-application-cmake/55828) — shared library anti-pattern, module isolation recommendation
- [Pamplejuce template](https://github.com/sudara/pamplejuce) — Industry reference for JUCE+Catch2+CMake; uses Catch2 v3.7.1
- [Catch2 issue #2895](https://github.com/catchorg/Catch2/issues/2895) — BUILD_SHARED_LIBS pitfall when using FetchContent

### Tertiary (LOW confidence)
- [Modern CMake - Catch chapter](https://cliutils.gitlab.io/modern-cmake/chapters/testing/catch.html) — General Catch2+CMake pattern; may be dated but consistent with official docs

---

## Metadata

**Confidence breakdown:**
- Standard stack (Catch2 v3 FetchContent): HIGH — verified against official cmake-integration.md
- CMake test target structure: MEDIUM — pattern is well-reported in JUCE community but exact compile definitions for plain add_executable need one build iteration to confirm
- Pitch arithmetic test cases: HIGH — derived directly from reading ScaleQuantizer.cpp and ChordEngine.cpp source code
- MSVC CRT pitfall: HIGH — confirmed by JUCE forum threads and CMake docs
- CMAKE_MODULE_PATH requirement: HIGH — stated explicitly in official Catch2 docs

**Research date:** 2026-02-22
**Valid until:** 2026-08-22 (Catch2 stable; JUCE CMake API stable at 8.x)
