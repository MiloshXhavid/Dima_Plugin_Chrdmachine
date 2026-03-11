---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Cross-Platform Launch
status: in_progress
last_updated: "2026-03-11T14:05:15Z"
last_activity: "2026-03-11 — Completed 46-03: universal Release build confirmed (x86_64 arm64) for VST3 and AU plugins"
progress:
  total_phases: 21
  completed_phases: 18
  total_plans: 44
  completed_plans: 44
---

## Current Position

Phase: 46 — Mac Build Foundation
Plan: 03 complete (next: 04)
Status: in_progress
Last activity: 2026-03-11 — Completed 46-03: universal Release build confirmed (x86_64 arm64) for VST3 and AU plugins

```
v2.0 Progress: [░░░░░░░░░░░░░░░░░░░░] 0% (0/4 phases)
Phase 46: [ ] Mac Build Foundation
Phase 47: [ ] License Key System
Phase 48: [ ] Code Signing and Notarization
Phase 49: [ ] macOS Distribution and GitHub Release
```

## Accumulated Context

### Key Decisions
- Mac support promoted from Out of Scope to v2.0 primary goal
- License key system: LemonSqueezy API (activate/validate/deactivate), in-plugin HTTPS validation via juce::URL (no new libraries — juce_network already linked)
- AU format added alongside VST3 for Logic/GarageBand compatibility
- Universal binary: arm64 + x86_64 via CMAKE_OSX_ARCHITECTURES — MUST be set before all FetchContent calls
- Code signing: Developer ID Application (plugin bundles) + Developer ID Installer (PKG) — both require Apple Developer Program ($99/year)
- Windows signing: Azure Trusted Signing preferred (verify eligibility before Phase 48) or OV cert fallback
- License persistence: juce::PropertiesFile (decide Keychain vs PropertiesFile before Phase 47 implementation begins)
- LicenseManager: process-level singleton, background juce::Thread for network, atomic flag for processBlock reads
- LicensePanel: non-modal JUCE Component overlay, shown via callAfterDelay(200ms) — NOT a DialogWindow
- Notarization: xcrun notarytool (altool retired Nov 2023 — do not use)
- macOS PKG installer: pkgbuild + productbuild (not DMG drag-to-folder — fails silently on Sonoma for system paths)
- Xcode version: prefer Xcode 15.x — Xcode 16.2 has confirmed AU universal binary regression; NOTE: Xcode 26.3 (Build 17C529) works correctly when cmake is configured with CMAKE_OSX_ARCHITECTURES on command line
- juce::ParameterID { id, 1 } required on all AudioParameter constructors — bare string IDs produce version hint 0 which causes auval warnings; applied to all 28+ parameters in createParameterLayout()
- cmake universal binary configure command: cmake -G Xcode -B build-mac -S . -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" — must be on command line, not just CMakeLists.txt CACHE STRING
- Plugin product name must not contain shell-special characters — cmake Xcode generator escapes spaces but NOT parentheses in generated post-build sh scripts; product name is now "Arcade Chord Control Beta-Test"
- SDL2 bakes x86-only SIMD flags (-mmmx/-msse/-msse2/-msse3) at configure time based on host CPU — only suppressed when cmake configure runs with CMAKE_OSX_ARCHITECTURES set; must wipe build-mac/ and reconfigure if architecture was previously missing

### Pre-Phase Research Flags
- Phase 47 (before implementation): Decide Keychain vs PropertiesFile for license key storage; run manual LemonSqueezy API test for deactivation endpoint behavior (LOW confidence)
- Phase 48 (before starting): Verify Azure Trusted Signing eligibility at azure.microsoft.com/products/artifact-signing — if ineligible, plan OV cert + USB token (1–3 week procurement lead time)
- Phase 48: Confirm `lipo -info` on `.component` reports `x86_64 arm64` — RESOLVED in 46-03: both VST3 and AU report x86_64 arm64 on Xcode 26.3

### Critical Pitfalls (do not repeat)
- CMAKE_OSX_ARCHITECTURES MUST be set on cmake command line (not just CMakeLists.txt CACHE STRING) — Xcode generator overrides with NATIVE_ARCH_ACTUAL if not on command line
- CMAKE_OSX_ARCHITECTURES must be set BEFORE FetchContent runs — setting it after produces single-arch JUCE/SDL2 static libs and a linker failure
- SDL2 detects host CPU SIMD capabilities at configure time and adds x86-only flags (-mmmx/-msse) to ALL SDL2 source files — these break arm64 cross-compile; only suppressed when CMAKE_OSX_ARCHITECTURES is set on cmake command line at configure time
- codesign --deep invalidates nested bundle signatures — sign leaf-to-root without --deep
- HARDENED_RUNTIME_OPTIONS must include network.client or all LemonSqueezy calls silently fail in notarized builds
- HID entitlements (device.usb + device.bluetooth) must be in entitlements plist or gamepad detection silently fails in notarized builds
- processBlock must NEVER execute network I/O — reads only atomic bool from LicenseManager; all juce::URL calls run on juce::Thread

### Blockers
- None
