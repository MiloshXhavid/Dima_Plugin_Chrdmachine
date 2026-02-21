# Stack Research

**Domain:** JUCE MIDI Plugin Development (AUv3/VST3)
**Researched:** 2026-02-21
**Confidence:** HIGH

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|------------------|
| JUCE | 8.0.x (latest 8.0.12) | Cross-platform C++ framework for audio plugins | Industry standard for AUv3/VST3 development. Current version with MIDI 2.0 preview, WebView UI support, and improved Unicode. |
| C++ | C++20 (minimum) | Programming language | JUCE 8 requires C++20 for modern features like `char8_t` overloads. C++20 is stable and well-supported across compilers. |
| CMake | 3.21+ | Build system | Official JUCE CMake API is now the recommended approach (replacing Projucer for cross-platform). Enables CI/CD and editor-agnostic workflows. |

### Plugin Targets

| Technology | Purpose | Why Recommended |
|------------|---------|------------------|
| VST3 | Windows/Linux plugin format | Steinberg's modern standard, required for many DAWs |
| AUv3 | macOS plugin format | Apple's current audio plugin standard, required for Logic Pro, GarageBand |
| Standalone | Desktop application | Test and use plugin as standalone app |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Visual Studio 2022 (Windows) | IDE/compiler | Default for Windows. JUCE 8.0.12 now defaults to VS 2026. |
| Xcode (macOS) | IDE/compiler | Required for building AUv3. Use latest stable. |
| VS Code + CMake Tools | Editor | Popular alternative to full IDEs, works cross-platform |
| Git + GitHub | Version control + CI | GitHub Actions is the standard for JUCE CI |

### Testing & Validation

| Technology | Purpose | When to Use |
|------------|---------|-------------|
| GoogleTest | Unit testing | Core logic, scale calculations, MIDI message handling |
| Pluginval | Plugin validation | CI testing, cross-platform validation. Official JUCE tool. |
| Valgrind (Linux) | Memory debugging | Linux-specific memory leak detection |

### CI/CD

| Technology | Purpose | Notes |
|------------|---------|-------|
| GitHub Actions | Automated builds | Industry standard. Use `juce-actions` for JUCE-specific workflows. |
| Cross-platform matrix | Windows/macOS/Linux | Essential for plugin distribution |

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|--------------|-------------|------------------------|
| CMake | Projucer | Legacy projects, beginners wanting visual project setup |
| JUCE 8 | JUCE 7 | Need maximum stability, avoiding breaking changes |
| C++20 | C++17 | Compiler constraints, legacy code requirements |
| GitHub Actions | GitLab CI/Jenkins | Team already invested in alternative CI |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| VST2 (legacy) | Deprecated, no new features, licensing concerns | VST3 |
| JUCE 6.x | No longer supported, missing modern features | JUCE 8.x |
| Projucer-only workflow | Platform-locked, no CI support, deprecated by community | CMake-based workflow |
| C++14 or older | Missing modern features, harder to maintain | C++20 |

## Stack Patterns by Variant

**If targeting all major platforms (Windows/macOS/Linux):**
- Use CMake workflow with GitHub Actions
- Build matrix: Windows (VS), macOS (Xcode), Linux (GCC/Clang)
- Because: Single codebase, automated cross-platform validation

**If macOS-only focus:**
- Can use Projucer + Xcode workflow
- Consider adding CLAP support (JUCE 9 will include it)
- Because: Simpler setup when not shipping cross-platform

**If needing WebView UI:**
- Use JUCE 8 with WebView module
- Frontend can use React/Vue/vanilla JS
- Because: Modern UI development workflow

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| JUCE 8.0.x | C++20, CMake 3.21+ | Requires C++20 for full Unicode support |
| Visual Studio 2022/2026 | JUCE 8.x | VS 2026 is new default in 8.0.12 |
| Xcode 15+ | JUCE 8.x | Required for modern C++ support |
| JUCE 8.x | JUCE 7.x modules | Generally backward compatible |

## Installation

```bash
# Clone JUCE
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git

# Common CMake approach (using cmake-includes)
# See: https://github.com/sudara/cmake-includes

# Dependencies
- CMake 3.21+
- C++20 compatible compiler
- Platform-specific: Visual Studio 2022+, Xcode 15+, or GCC/Clang
```

## Sources

- [JUCE 8.0.12 Release](https://github.com/juce-framework/JUCE/releases) — HIGH confidence
- [JUCE Roadmap Q3 2025](https://juce.com/blog/juce-roadmap-update-q3-2025/) — HIGH confidence
- [What's New In JUCE 8](https://juce.com/releases/whats-new/) — HIGH confidence
- [Cross-Platform JUCE with CMake & GitHub Actions](https://reillyspitzfaden.com/posts/2025/08/plugins-for-everyone-crossplatform-juce-with-cmake-github-actions) — MEDIUM confidence
- [JUCE CMake Template](https://scruffycatstudios.com/posts/juce-cmake-template/) — MEDIUM confidence
- [Continuous Integration for Audio Plugins](https://moonbase.sh/articles/continuous-integration-for-audio-plugins-tips-tricks-gotchas/) — MEDIUM confidence
- [Audio Plugin Template with GoogleTest](https://github.com/JanWilczek/audio-plugin-template) — MEDIUM confidence

---

*Stack research for: ChordJoystick - JUCE MIDI Plugin*
*Researched: 2026-02-21*
