# Phase 1: Project Setup - Research

**Researched:** 2026-02-21
**Domain:** JUCE CMake build pipeline and APVTS parameter system
**Confidence:** HIGH

## Summary

This phase establishes the foundational build infrastructure for a JUCE audio plugin using CMake (the modern standard, not Projucer). Key findings: JUCE 8.x uses CMake 3.22+ with `juce_add_plugin()` for multi-format builds (VST3, AUv3, Standalone). APVTS (AudioProcessorValueTreeState) provides the parameter system with group-based organization. GitHub Actions with matrix strategy handles cross-platform CI. Universal binaries require `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

**Primary recommendation:** Use JUCE as a CMake subdirectory with FetchContent, configure all three plugin formats from day one, and organize APVTS parameters into groups matching the 6 functional areas defined in CONTEXT.md.

---

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Folder organization:** By Type (standard JUCE folders) — Source/, Components/, DSP/, Models/
- **Component folders:** Header files in same folder as source
- **Tests:** Root tests/ folder
- **Plugin formats:** VST3 + AUv3 + Standalone from day one
- **CI/CD:** GitHub Actions with Windows + macOS
- **Architectures:** Universal (x64 + ARM)
- **Build configurations:** Both Debug and Release

### Parameter Layout Decisions
- **Organization:** By Function (grouped by feature)
- **Parameter groups (6 total):** Quantizer, Intervals, Triggers, Joystick, Switches, Looper
- **Value ranges:** Normalized (0.0-1.0), convert in code
- **Automation:** Full automation support from day one

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| (Foundational) | Establish JUCE build pipeline with CMake | CMake 3.22+ with juce_add_plugin(), VST3/AUv3/Standalone formats |
| (Foundational) | Define all APVTS parameters | ParameterLayout with AudioProcessorParameterGroups |
| MIDI-01 | Plugin outputs MIDI via VST3 | Built into juce_add_plugin() |
| MIDI-02 | Plugin outputs MIDI via AUv3 | Built into juce_add_plugin() |
| MIDI-03 | Standalone mode | FORMATS includes Standalone |
</phase_requirements>

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.x (latest) | Audio plugin framework | Industry standard for VST3/AUv3 |
| CMake | 3.22+ | Build system | Required for modern JUCE, replaces Projucer |
| C++ | 17 or 20 | Language | JUCE minimum requirement |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CPM.cmake | Latest | Package management | FetchContent alternative for dependencies |
| Catch2 | 3.x | Unit testing | Recommended by JUCE community templates |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CMake | Projucer | Projucer is deprecated for new projects; CMake is cross-platform friendly |
| JUCE FetchContent | Manual JUCE clone | FetchContent auto-downloads, cleaner repo |
| AAX | Skip AAX | Requires PACE SDK ($500+/year); not in scope |

---

## Architecture Patterns

### Recommended Project Structure
```
ChordJoystick/
├── CMakeLists.txt           # Root build config
├── JUCE/                   # JUCE framework (submodule or FetchContent)
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   └── PluginEditor.cpp
├── Components/             # UI components
├── DSP/                   # Audio processing
├── Models/                # Data models
├── Parameters/            # APVTS parameter definitions
│   └── ParameterHelper.h
└── tests/                 # Unit tests
```

### Pattern 1: CMake with JUCE
**What:** Using CMake to build JUCE plugins instead of Projucer
**When to use:** All new JUCE projects (as of 2024+)
**Example:**
```cmake
cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(ChordJoystick
    FORMATS VST3 AU Standalone
    PLUGIN_MANUFACTURER_CODE ChJo
    PLUGIN_CODE ChJo
    PRODUCT_NAME "ChordJoystick"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(ChordJoystick PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
```
**Source:** [JUCE examples/CMake/AudioPlugin](https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt)

### Pattern 2: APVTS Parameter Groups
**What:** Organizing parameters into functional groups using AudioProcessorParameterGroups
**When to Use:** Any plugin with multiple parameter categories
**Example:**
```cpp
AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    
    auto layout = std::make_unique<ParameterLayout>();
    
    // Quantizer group - 12 scale buttons
    auto quantizerGroup = std::make_unique<AudioProcessorParameterGroup>("quantizer", "Quantizer", "|");
    for (int i = 0; i < 12; ++i)
    {
        quantizerGroup->addParameter(std::make_unique<AudioParameterBool>("scale_" + String(i), "Scale " + String(i), false));
    }
    layout->add(std::move(quantizerGroup));
    
    // Intervals group
    auto intervalsGroup = std::make_unique<AudioProcessorParameterGroup>("intervals", "Intervals", "|");
    intervalsGroup->addParameter(std::make_unique<AudioParameterFloat>("root", "Root", NormalisableRange(0.0f, 12.0f), 0.0f));
    intervalsGroup->addParameter(std::make_unique<AudioParameterFloat>("third", "Third", NormalisableRange(0.0f, 12.0f), 4.0f));
    intervalsGroup->addParameter(std::make_unique<AudioParameterFloat>("fifth", "Fifth", NormalisableRange(0.0f, 12.0f), 7.0f));
    intervalsGroup->addParameter(std::make_unique<AudioParameterFloat>("tension", "Tension", NormalisableRange(0.0f, 12.0f), 11.0f));
    layout->add(std::move(intervalsGroup));
    
    return *layout;
}
```

### Pattern 3: GitHub Actions CI
**What:** Cross-platform CI with matrix strategy
**When to use:** Any production plugin needing automated builds
**Example:**
```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        include:
          - os: macos-latest
            cmake_args: -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
          - os: windows-latest
          - os: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      - name: Configure
        run: cmake -B build ${{ matrix.cmake_args }}
      - name: Build
        run: cmake --build build --config Release
```
**Source:** [Pamplejuce workflow](https://github.com/sudara/pamplejuce/blob/main/.github/workflows/build_and_test.yml)

### Anti-Patterns to Avoid
- **Projucer for new projects:** JUCE is deprecating Projucer in favor of CMake; avoid for new projects
- **Single format builds:** Build all formats (VST3, AU, Standalone) from day one to catch format-specific issues
- **Hardcoding parameter ranges:** Always use NormalisableRange for APVTS parameters to support automation

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plugin format builds | Custom makefiles | juce_add_plugin() | Handles SDK paths, bundle signing, vst3/AU specifics |
| Parameter serialization | Custom XML/JSON | APVTS ValueTree | Handles state save/restore, undo/redo, automation |
| Cross-platform builds | Platform-specific scripts | CMake + GitHub Actions | Works on all platforms, matrix strategy |
| Unit testing framework | Custom test runner | Catch2 | JUCE community standard, header-only |
| Dependency management | Manual downloads | CPM.cmake or FetchContent | Reproducible builds |

---

## Common Pitfalls

### Pitfall 1: JUCE Submodule Not Initialized
**What goes wrong:** CMake fails to find JUCE modules
**Why it happens:** Forgetting `git submodule update --init` or `submodules: recursive` in CI
**How to avoid:** Use FetchContent instead of submodules, or ensure CI checks out submodules
**Warning signs:** "JUCE_audio_processors" module not found errors

### Pitfall 2: Plugin ID Conflicts
**What goes wrong:** Plugin fails to load in DAW due to duplicate plugin ID
**Why it happens:** Using default or example PLUGIN_CODE
**How to avoid:** Set unique 4-character PLUGIN_CODE and PLUGIN_MANUFACTURER_CODE in CMake
**Warning signs:** Plugin appears twice in DAW, crashes on load

### Pitfall 3: macOS Code Signing
**What goes wrong:** AU/VST3 fails to load on macOS due to signing issues
**Why it happens:** Building without Developer ID certificate or notarization
**How to avoid:** For development, use ad-hoc signing; for distribution, set up proper signing in CI
**Warning signs:** "Plug-in not valid" error in Logic/Ableton

### Pitfall 4: Parameter Automation Not Working
**What goes wrong:** Parameters don't respond to DAW automation
**Why it happens:** Not using APVTS, or not implementing getParameter/getStateInformation
**How to avoid:** Use APVTS which handles automation automatically; only override if needed
**Warning signs:** Parameters work in UI but not in DAW automation

### Pitfall 5: Universal Binary Not Building
**What goes wrong:** Only one architecture builds on macOS
**Why it happens:** Missing `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` flag
**How to avoid:** Always specify both architectures in CMake configuration
**Warning signs:** Plugin only works on Intel or Apple Silicon, not both

---

## Code Examples

### CMake Plugin Configuration (Complete)
```cmake
cmake_minimum_required(VERSION 3.22)
project(ChordJoystick VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch JUCE automatically
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
)
set(JUCE_BUILD_EXAMPLES OFF)
set(JUCE_BUILD_EXTRAS OFF)
FetchContent_MakeAvailable(JUCE)

# Add plugin
juce_add_plugin(ChordJoystick
    FORMATS VST3 AU AUv3 Standalone
    PLUGIN_MANUFACTURER_CODE ChJo
    PLUGIN_CODE ChJo
    PRODUCT_NAME "ChordJoystick"
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Instrument
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
)

target_sources(ChordJoystick PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(ChordJoystick PRIVATE
    juce::juce_audio_processors
    juce::juce_gui_basics
)
```

### APVTS with Normalized Parameters
```cpp
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor()
        : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo()))
        , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
    {}

    juce::AudioProcessorValueTreeState parameters;
    
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        
        // Use normalized ranges (0.0-1.0) - convert to actual values in code
        auto params = ParameterLayout{};
        
        // Global transpose - normalized 0-12 semitones
        params.add(std::make_unique<AudioParameterFloat>(
            "global_transpose", "Global Transpose",
            NormalisableRange(0.0f, 12.0f, 1.0f), 0.0f
        ));
        
        // XY attenuator
        params.add(std::make_unique<AudioParameterFloat>(
            "xy_attenuator", "XY Attenuator",
            NormalisableRange(0.0f, 24.0f, 1.0f), 12.0f
        ));
        
        return params;
    }
};
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Projucer | CMake | JUCE 7+ (2023) | Cross-platform IDEs without Xcode/VS dependency |
| VST2 + VST3 | VST3 only | Modern DAWs | VST2 is deprecated |
| Manual parameter management | APVTS | Many years | Built-in state, automation, undo |
| Single-platform CI | Matrix CI (GitHub Actions) | Modern CI tools | Test all platforms automatically |
| Intel-only macOS | Universal binaries | Apple Silicon (2020) | Support both architectures |

**Deprecated/outdated:**
- Projucer: Still works but deprecated; CMake is the future
- VST2: Still supported but no longer needed; VST3 is universal
- AAX without PACE SDK: Can't build without paid license

---

## Open Questions

1. **JUCE 8 vs 7**
   - What we know: JUCE 8 is latest with improved CMake support
   - What's unclear: Breaking changes between 7 and 8
   - Recommendation: Use latest stable (8.x), but test thoroughly

2. **Code Signing for Development**
   - What we know: Ad-hoc signing works for local dev
   - What's unclear: Whether to set up notarization now or later
   - Recommendation: Skip for now, add when preparing for distribution

---

## Sources

### Primary (HIGH confidence)
- [JUCE CMake AudioPlugin Example](https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt) - Official CMake configuration
- [JUCE APVTS Documentation](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html) - Parameter system
- [Pamplejuce Template](https://github.com/sudara/pamplejuce) - Industry-standard template with CI

### Secondary (MEDIUM confidence)
- [WolfSound Audio Plugin Template](https://github.com/JanWilczek/audio-plugin-template) - Comprehensive CMake setup
- [Cross-Platform Builds Guide](https://claude-plugins.dev/skills/@yebot/rad-cc-plugins/cross-platform-builds) - Claude Code skill
- [ Reilly Spitzfaden CI Tutorial](https://reillyspitzfaden.com/posts/2025/08/plugins-for-everyone-crossplatform-juce-with-cmake-github-actions) - GitHub Actions setup

### Tertiary (LOW confidence)
- [Forum discussions on AUv3 CMake](https://forum.juce.com/t/building-an-auv3-plugin-with-cmake/53765) - Community verification

---

## Metadata

**Confidence breakdown:**
- Standard Stack: HIGH - Official JUCE docs and community consensus
- Architecture: HIGH - Multiple verified templates follow same patterns
- Pitfalls: HIGH - Well-documented in forums and tutorials

**Research date:** 2026-02-21
**Valid until:** 30 days (JUCE releases are ~6 months apart, CMake API is stable)
