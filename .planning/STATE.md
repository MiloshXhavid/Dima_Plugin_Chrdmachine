---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Cross-Platform Launch
status: planning
last_updated: "2026-03-11T13:02:28.547Z"
last_activity: 2026-03-10 — Roadmap written, awaiting plan-phase 46
progress:
  total_phases: 21
  completed_phases: 18
  total_plans: 44
  completed_plans: 41
---

## Current Position

Phase: 46 — Mac Build Foundation
Plan: 02 complete (next: 03)
Status: in_progress
Last activity: 2026-03-11 — Completed 46-02: all AudioParameter constructors updated to use juce::ParameterID { id, 1 }

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
- Xcode version: prefer Xcode 15.x — Xcode 16.2 has confirmed AU universal binary regression
- juce::ParameterID { id, 1 } required on all AudioParameter constructors — bare string IDs produce version hint 0 which causes auval warnings; applied to all 28+ parameters in createParameterLayout()

### Pre-Phase Research Flags
- Phase 47 (before implementation): Decide Keychain vs PropertiesFile for license key storage; run manual LemonSqueezy API test for deactivation endpoint behavior (LOW confidence)
- Phase 48 (before starting): Verify Azure Trusted Signing eligibility at azure.microsoft.com/products/artifact-signing — if ineligible, plan OV cert + USB token (1–3 week procurement lead time)
- Phase 48: Confirm `lipo -info` on `.component` reports `x86_64 arm64` — if single-arch, Xcode 16.2 regression confirmed, switch to Xcode 15.x

### Critical Pitfalls (do not repeat)
- CMAKE_OSX_ARCHITECTURES MUST be set before FetchContent — setting it after produces single-arch JUCE/SDL2 static libs and a linker failure
- codesign --deep invalidates nested bundle signatures — sign leaf-to-root without --deep
- HARDENED_RUNTIME_OPTIONS must include network.client or all LemonSqueezy calls silently fail in notarized builds
- HID entitlements (device.usb + device.bluetooth) must be in entitlements plist or gamepad detection silently fails in notarized builds
- processBlock must NEVER execute network I/O — reads only atomic bool from LicenseManager; all juce::URL calls run on juce::Thread

### Blockers
- None
