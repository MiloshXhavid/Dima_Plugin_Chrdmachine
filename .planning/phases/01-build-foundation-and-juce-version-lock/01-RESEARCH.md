# Phase 01: Build Foundation and JUCE Version Lock - Research

**Researched:** 2026-02-22
**Domain:** CMake / JUCE 8 / MSVC / Ableton Live 11 VST3 MIDI routing
**Confidence:** HIGH (CMake/static CRT), HIGH (JUCE tag), MEDIUM (Ableton MIDI routing — DAW behavior is partly empirical)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **DAW smoke test target:** Ableton Live 11 only — no Reaper available. Test from Phase 01 onward.
  Pass criteria: plugin loads without crash + all APVTS params save/restore + at least one audible note via a routed soft synth.
- **JUCE version:** Pin to 8.0.4 — do not use origin/master, do not check for newer tags.
- **COPY_PLUGIN_AFTER_BUILD:** Enabled — auto-copy VST3 bundle to `%COMMONPROGRAMFILES%\VST3\` after each build.
  Dev machine has full admin rights — no elevation issues.
- **Unit testing (Catch2):** Do not add Catch2 in Phase 01 or Phase 02.
  User prefers manual verification via Ableton.

### Claude's Discretion

- Specific CMakeLists.txt structure and ordering.
- Static CRT configuration (`set_property(TARGET ... PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")`).
- CMake generator to use (Visual Studio 17 2022 vs Ninja).
- How to handle the existing `GIT_SHALLOW TRUE` flag for JUCE FetchContent.

### Deferred Ideas (OUT OF SCOPE)

- Catch2 unit tests for ChordEngine / ScaleQuantizer — deferred entirely; user prefers manual testing.
- Phase 02 (Core Engine Validation) scope adjustment — not this phase.
- Reaper testing — user doesn't have Reaper, skip entirely.
- Ableton Live 12 upgrade consideration — no action for v1.
</user_constraints>

---

## Summary

Phase 01 is a build-only phase: fix the CMakeLists.txt so the project compiles with a pinned JUCE version, validate that the VST3 loads in Ableton Live 11, confirm APVTS round-trip, and hear one note. There are no new features to write.

Three issues are known going in: (1) `GIT_TAG origin/master` must be replaced with the exact `8.0.4` tag (commit `51d11a2be6d5c97ccf12b4e5e827006e19f0555a`); (2) static CRT must be configured explicitly for distribution builds; (3) Ableton Live 11 refuses to load MIDI-only VST3 plugins that declare zero audio buses — a dummy stereo output bus is required, added conditionally via `PluginHostType().isAbletonLive()` in the constructor's `BusesProperties`. The APVTS save/restore pattern already implemented in `PluginProcessor.cpp` is correct and requires no changes.

**Primary recommendation:** Fix `GIT_TAG`, set `CMAKE_MSVC_RUNTIME_LIBRARY` globally before `FetchContent_MakeAvailable`, and add the Ableton-conditional dummy audio output bus in the `PluginProcessor` constructor. These three changes are the entire functional scope of 01-01.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.4 (tag `8.0.4`, commit `51d11a2be6d5c97ccf12b4e5e827006e19f0555a`) | Plugin framework | Locked decision |
| SDL2 | release-2.30.9 | Gamepad | Already correctly pinned in CMakeLists.txt — no change needed |
| CMake | 3.22+ (project already requires 3.22) | Build system | JUCE 8 minimum |
| MSVC | Visual Studio 2019+ | Compiler | JUCE 8 minimum on Windows |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce::PluginHostType` | (part of JUCE) | Runtime host detection | Used in constructor to add dummy Ableton audio bus |

---

## Architecture Patterns

### Pattern 1: Pinning JUCE with FetchContent

**What:** Replace `GIT_TAG origin/master` with the exact release tag. Use `GIT_SHALLOW TRUE` — this is safe with a named tag.

**When to use:** Always for reproducible builds.

**Example:**
```cmake
# Source: https://github.com/juce-framework/JUCE/releases/tag/8.0.4
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4        # commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a
    GIT_SHALLOW    TRUE
)
```

Note on `GIT_SHALLOW TRUE` with a tag: shallow clones work correctly with named tags (`8.0.4`). The limitation is that `GIT_SHALLOW TRUE` cannot be used with a bare commit hash (SHA) — the tag name must be used. Since `8.0.4` is an immutable annotated tag on the JUCE repository this is safe.

If absolute reproducibility is required (guards against tag being force-pushed, which JUCE has never done), use the full commit hash without `GIT_SHALLOW`:
```cmake
GIT_TAG  51d11a2be6d5c97ccf12b4e5e827006e19f0555a  # 8.0.4
# (omit GIT_SHALLOW when using a hash)
```

**Recommendation (Claude's Discretion):** Use `GIT_TAG 8.0.4` with `GIT_SHALLOW TRUE`. The JUCE project has never moved a release tag. This gives a faster clone than omitting shallow, and the tag is unambiguous. Document the commit hash in a comment for reference.

---

### Pattern 2: Static MSVC CRT — Global Setting

**What:** Set `CMAKE_MSVC_RUNTIME_LIBRARY` as early as possible in `CMakeLists.txt`, before any `FetchContent_MakeAvailable` call. This propagates to JUCE's own internal targets, preventing linker mismatch errors.

**Why global (not per-target):** If you set only the plugin target's property, JUCE's intermediate static library targets will still be compiled with the default dynamic CRT (`/MD`), causing LNK2038 "mismatch detected for RuntimeLibrary" link errors.

**Example:**
```cmake
# Add this immediately after project() declaration, before any FetchContent calls
cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

# Static CRT for all targets (must come before FetchContent)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
...
```

`MultiThreaded$<$<CONFIG:Debug>:Debug>` expands to:
- Release: `/MT` (MultiThreaded static)
- Debug: `/MTd` (MultiThreadedDebug static)

**Anti-pattern:** Do NOT mix this with manually appended `/MT` flags in `CMAKE_CXX_FLAGS`. Use only the property. Visual Studio and VS Code CMake integration may inject `/MD` into cache — always wipe the build directory after changing this.

---

### Pattern 3: Ableton Live 11 Dummy Audio Bus

**What:** Ableton Live 11 refuses to load VST3 plugins that declare no audio buses, even if the plugin is a pure MIDI generator. The plugin must expose at least a stereo output bus to appear in Ableton's device browser and load without crashing.

**Why:** This is a long-standing Ableton limitation documented across multiple JUCE forum threads (2020–2024). Ableton does not implement the VST3 "MIDI Effect" category in the same way as Reaper or Bitwig. A plugin with zero audio buses is silently rejected or crashes.

**Correct pattern (runtime host detection):**

In `PluginProcessor.cpp`, change the `AudioProcessor` base constructor call:

```cpp
// Current (broken for Ableton):
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()),
      apvts(...)

// Fixed:
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          juce::PluginHostType().isAbletonLive()
              ? BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), false)
              : BusesProperties()),
      apvts(...)
```

The `false` argument to `withOutput` marks the bus as disabled by default — it carries no audio, it exists only to satisfy Ableton's bus requirement.

Also update `isBusesLayoutSupported` to accept the optional stereo output:

```cpp
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // No audio input ever
    if (layouts.getMainInputChannelCount() != 0) return false;
    // Allow zero audio output (Reaper/Bitwig) or stereo output (Ableton)
    const int outCh = layouts.getMainOutputChannelCount();
    if (outCh != 0 && outCh != 2) return false;
    return true;
}
```

**IS_SYNTH and IS_MIDI_EFFECT in CMakeLists.txt:** The existing values (`IS_SYNTH FALSE`, `IS_MIDI_EFFECT FALSE`) are reasonable. `IS_MIDI_EFFECT TRUE` actively breaks Ableton Live 11 — do not set it. `IS_SYNTH TRUE` is not required when using the dummy bus approach, but may help in some hosts. Leave existing values unless the dummy bus approach does not work, then try `IS_SYNTH TRUE` as a follow-up.

**VST3_CATEGORIES:** Current value `"Instrument"` is the closest correct category for a MIDI generator that appears as an instrument in Ableton. Do not change it unless empirical testing reveals a problem.

**Ableton routing procedure (for smoke test):**
1. Create a MIDI track. Add ChordJoystick as a device.
2. Create a second MIDI track with an instrument (e.g., Wavetable).
3. On the instrument track: set "MIDI From" to the ChordJoystick track, set the channel to "ChordJoystick" (the plugin output), set Monitor to "In".
4. Press a touch-plate button in ChordJoystick — the instrument should sound.

**Important limitation:** Ableton merges all MIDI channels internally when routing track-to-track. The 4-voice multi-channel routing (voices on MIDI ch 1–4) will appear merged on a single channel in this routing setup. Per-channel routing requires external MIDI routing or a separate MIDI output device — this is an Ableton Live 11 architectural constraint, not a plugin bug. For the Phase 01 smoke test, a single note on any channel is sufficient.

---

### Pattern 4: APVTS Save/Restore (Verification Only)

The existing implementation in `PluginProcessor.cpp` is already correct:

```cpp
// getStateInformation — already implemented correctly
void PluginProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

// setStateInformation — already implemented correctly
void PluginProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
```

`copyState()` is thread-safe (takes a copy before serialisation). `replaceState()` triggers listeners correctly. The tag name check prevents loading state from a different plugin. No changes required.

**APVTS round-trip test in Ableton (no pluginval):**
1. Load plugin. Set several parameters to non-default values (e.g., globalTranspose = 5, thirdInterval = 3, scalePreset = Dorian).
2. Save the Ableton project.
3. Close Ableton. Reopen the project.
4. Verify the parameters are restored to the values set in step 1.
5. Also test: right-click the plugin → "Save Preset" → reload preset.

---

### Pattern 5: COPY_PLUGIN_AFTER_BUILD

Already set to `TRUE` in the existing CMakeLists.txt. No change needed. JUCE copies the `.vst3` bundle to `%COMMONPROGRAMFILES%\VST3\` automatically on build success. This requires an elevated build or that the directory is writable. The user confirmed admin rights are available.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| APVTS state serialisation | Custom XML serialiser | `apvts.copyState() + createXml()` (already done) | JUCE handles thread-safety and type coercion |
| Runtime host detection | `#ifdef` preprocessor flags | `juce::PluginHostType().isAbletonLive()` | Runtime detection works regardless of build target |
| Static CRT selection | Manual `/MT` flag in `CMAKE_CXX_FLAGS` | `CMAKE_MSVC_RUNTIME_LIBRARY` property | Flag approach conflicts with CMake's own management and breaks with Clang-cl |

**Key insight:** The entire scope of 01-01 is three targeted edits to existing files — no new infrastructure needed.

---

## Common Pitfalls

### Pitfall 1: Stale Build Directory After Changing Runtime Library

**What goes wrong:** CMake caches `CMAKE_MSVC_RUNTIME_LIBRARY` and `CMAKE_CXX_FLAGS` from the first configure run. If you add the global `CMAKE_MSVC_RUNTIME_LIBRARY` setting after a build directory already exists, the old cached values may override it, and you get LNK2038 mismatch errors.

**Why it happens:** CMake's cache (`CMakeCache.txt`) persists between reconfigures unless explicitly cleared.

**How to avoid:** After adding the static CRT setting, delete the entire build directory and reconfigure from scratch: `rm -rf build && cmake -B build -S .`

**Warning signs:** LNK2038 "mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'"

---

### Pitfall 2: GIT_SHALLOW TRUE with a Commit Hash

**What goes wrong:** `GIT_SHALLOW TRUE` with `GIT_TAG <full-sha>` causes a CMake error or silently falls back to a full clone.

**Why it happens:** Shallow clones require a named ref (branch or tag) to know where to stop the history walk.

**How to avoid:** Use the tag name `8.0.4`, not the SHA, when `GIT_SHALLOW TRUE` is present. If using the SHA for absolute reproducibility, remove `GIT_SHALLOW TRUE`.

---

### Pitfall 3: Plugin Invisible in Ableton Without Dummy Audio Bus

**What goes wrong:** Plugin does not appear in Ableton's VST3 browser or loads but produces no MIDI output.

**Why it happens:** Ableton Live 11 requires at least one audio bus on VST3 plugins. Plugins with `BusesProperties()` (zero buses) are silently rejected or categorised incorrectly.

**How to avoid:** Add the Ableton-conditional dummy stereo output bus in the constructor (see Pattern 3 above). This must also be reflected in `isBusesLayoutSupported` to allow the 0-channel or 2-channel layout.

**Warning signs:** Plugin shows in browser but disappears after scan, or Ableton shows "Plugin could not be loaded" without further details.

---

### Pitfall 4: JUCE FetchContent Re-Download on Every Configure

**What goes wrong:** CMake re-fetches JUCE on every configure run because `origin/master` always resolves to HEAD, causing slow builds and non-reproducible behaviour.

**Why it happens:** Branch-name tags like `origin/master` are moving refs. CMake detects the ref has moved and re-downloads.

**How to avoid:** Pin to `8.0.4` (this is the fix). Once pinned, FetchContent will only download once and cache in `_deps/`.

---

### Pitfall 5: IS_MIDI_EFFECT TRUE Breaks Ableton

**What goes wrong:** Plugin fails to load or doesn't appear in Ableton's instrument/effect racks at all.

**Why it happens:** `IS_MIDI_EFFECT TRUE` sets the VST3 subcategory to `MIDIEffect`. Ableton Live 11 does not support this VST3 subcategory and filters these plugins out.

**How to avoid:** Keep `IS_MIDI_EFFECT FALSE` (current value). Use `IS_SYNTH FALSE` + `NEEDS_MIDI_OUTPUT TRUE` + dummy audio bus instead.

---

### Pitfall 6: LooperEngine std::mutex in processBlock

**What goes wrong:** DAW may hang or stutter because `std::mutex::lock()` on the audio thread is a priority-inversion hazard.

**Why it matters for Phase 01:** Even though the LooperEngine mutex fix is Phase 05 scope, the mutex will be called during any processBlock invocation. If Ableton's audio engine happens to hold a thread under load, the plugin can stall.

**Mitigation for Phase 01:** The smoke test only needs to confirm basic loading and one note. Keep DAW CPU load low during Phase 01 testing (no other CPU-heavy plugins). The mutex will not deadlock under light load in a controlled test. Document as a known risk; do not attempt to fix in this phase.

---

## Code Examples

### Complete CMakeLists.txt Changes for 01-01

```cmake
cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

# Static CRT — must be set before FetchContent populates JUCE targets
# Ensures /MT (Release) and /MTd (Debug) across all targets including JUCE internals
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# ─── JUCE ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4        # commit 51d11a2be6d5c97ccf12b4e5e827006e19f0555a — Nov 18 2024
    GIT_SHALLOW    TRUE
)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(juce)

# ... (SDL2 section unchanged) ...

juce_add_plugin(ChordJoystick
    PRODUCT_NAME            "ChordJoystick"
    FORMATS                 VST3
    PLUGIN_MANUFACTURER_CODE "MxCJ"
    PLUGIN_CODE             "CJoy"
    VST3_CATEGORIES         "Instrument"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

set_target_properties(ChordJoystick PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED ON
)

# ... (target_sources, target_compile_definitions, target_link_libraries unchanged) ...
```

### PluginProcessor.cpp Constructor Change for Ableton Bus

```cpp
// Source: https://forum.juce.com/t/vst3-midi-plugins-wont-load-in-ableton-live/36323
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          juce::PluginHostType().isAbletonLive()
              ? BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), false)
              : BusesProperties()),
      apvts(*this, nullptr, "ChordJoystick", createParameterLayout())
{
}
```

### isBusesLayoutSupported Update

```cpp
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // No audio input ever
    if (layouts.getMainInputChannelCount() != 0) return false;
    // Allow zero output (Reaper/Bitwig/standalone) or stereo dummy output (Ableton)
    const int outCh = layouts.getMainOutputChannelCount();
    if (outCh != 0 && outCh != 2) return false;
    return true;
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `GIT_TAG origin/master` | `GIT_TAG 8.0.4 GIT_SHALLOW TRUE` | This phase | Reproducible, fast clone |
| Dynamic CRT (`/MD`) | `CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded..."` | This phase | No MSVC redistributable dependency |
| `BusesProperties()` zero buses | Ableton-conditional stereo dummy bus | This phase | Plugin loads in Ableton Live 11 |
| N/A | APVTS pattern already correct | Prior session | No change needed |

**Deprecated/outdated in this project:**
- `GIT_TAG origin/master`: Non-reproducible, re-downloads every configure. Replaced by `8.0.4`.
- Manual `/MT` flag injection: Conflicts with CMake CRT management. Replaced by `CMAKE_MSVC_RUNTIME_LIBRARY`.

---

## Open Questions

1. **Will `IS_SYNTH FALSE` + dummy bus be sufficient for Ableton, or is `IS_SYNTH TRUE` required?**
   - What we know: Multiple JUCE forum threads confirm the dummy bus approach works. `IS_SYNTH TRUE` is an additional option but not universally required.
   - What's unclear: Ableton Live 11 (vs Live 12) specific behaviour — most forum data is from 2021–2023.
   - Recommendation: Try `IS_SYNTH FALSE` + dummy bus first. If the plugin doesn't appear in Ableton's browser, try `IS_SYNTH TRUE` as step two. Document empirical result in the phase summary.

2. **Does the dummy audio bus affect the processBlock signature?**
   - What we know: `processBlock` already calls `audio.clear()` and `audio.getNumSamples()`. With a disabled-by-default stereo bus, Ableton may or may not pass a non-zero sample buffer.
   - What's unclear: Whether `audio.getNumSamples()` returns 0 or the block size when the bus is disabled.
   - Recommendation: The existing `audio.clear()` call is safe regardless. TriggerSystem uses `audio.getNumSamples()` for block-size timing — if this returns 0 in Ableton, the trigger timing logic will be no-op. Test empirically during 01-02.

3. **Does Ableton Live 11 merge MIDI channels on the smoke test routing?**
   - What we know: Ableton merges all MIDI channels to one when routing internally between tracks. The 4-voice multi-channel design (voices on ch 1–4) will collapse to ch 1 in this routing.
   - What's unclear: Whether this affects the "audible note" smoke test criterion.
   - Recommendation: For 01-02, route ChordJoystick output to a single instrument track. Any voice triggering a note is sufficient to pass. Per-channel routing validation is deferred to Phase 03.

---

## Sources

### Primary (HIGH confidence)
- [Release 8.0.4 · juce-framework/JUCE](https://github.com/juce-framework/JUCE/releases/tag/8.0.4) — exact tag name, commit hash, release date confirmed
- [CMake MSVC_RUNTIME_LIBRARY documentation](https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html) — generator expression syntax confirmed
- [CMake CMAKE_MSVC_RUNTIME_LIBRARY variable](https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html) — global variable approach confirmed
- [CMake FetchContent documentation](https://cmake.org/cmake/help/latest/module/FetchContent.html) — GIT_SHALLOW TRUE with tag names confirmed

### Secondary (MEDIUM confidence)
- [JUCE Forum: VST3 MIDI Plugins Won't Load in Ableton Live](https://forum.juce.com/t/vst3-midi-plugins-wont-load-in-ableton-live/36323) — dummy audio bus pattern, PluginHostType::isAbletonLive(), verified by multiple developers across multiple threads
- [JUCE Forum: MIDI Effect Plugin characteristic — best practice?](https://forum.juce.com/t/midi-effect-plugin-characteristic-best-practice/59826) — IS_MIDI_EFFECT breaks Ableton Live 11 confirmed
- [JUCE Forum: Ensure that CMake uses static MSVC runtime library](https://forum.juce.com/t/ensure-that-cmake-uses-static-msvc-runtime-library-windows/55932) — global CMAKE_MSVC_RUNTIME_LIBRARY as second line of CMakeLists.txt confirmed working
- [Ableton: Accessing the MIDI output of a VST plug-in](https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in) — routing procedure (MIDI From / Monitor In) confirmed
- [JUCE: Saving and loading your plug-in state](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — APVTS copyState/replaceState pattern confirmed

### Tertiary (LOW confidence)
- [CMake FetchContent GIT_SHALLOW issues](https://gitlab.kitware.com/cmake/cmake/-/issues/21457) — GIT_SHALLOW TRUE does not work with bare commit hashes, only with tags/branches (consistent with official docs)
- [JUCE Forum: How to enable MSVC static runtime linking](https://forum.juce.com/t/how-to-enable-msvc-static-runtime-linking-with-cmake/40917) — cache variable approach (`CACHE INTERNAL ""`) mentioned but global set is sufficient per other thread

---

## Metadata

**Confidence breakdown:**
- JUCE tag / FetchContent: HIGH — release exists, tag confirmed, GIT_SHALLOW behaviour documented
- Static CRT configuration: HIGH — CMake docs + multiple JUCE-specific forum confirmations
- Ableton dummy bus pattern: MEDIUM — multiple forum threads agree, but exact IS_SYNTH interaction with Live 11 requires empirical validation
- APVTS round-trip: HIGH — existing implementation matches official JUCE tutorial pattern exactly
- Ableton MIDI routing procedure: HIGH — from Ableton official documentation

**Research date:** 2026-02-22
**Valid until:** 2026-04-22 (stable APIs, 60-day window; Ableton Live 11 behaviour unlikely to change)
