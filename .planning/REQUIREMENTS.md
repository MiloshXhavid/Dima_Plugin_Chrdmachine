# Requirements: ChordJoystick v2.0

**Defined:** 2026-03-10
**Core Value:** XY joystick mapped to harmonic space — paid, cross-platform (Windows + macOS), with license key protection

## v2.0 Requirements

### Mac Build (MAC)

- [ ] **MAC-01**: Plugin builds as universal binary (arm64 + x86_64) via `cmake -G Xcode` with both VST3 and AU targets
- [ ] **MAC-02**: AU plugin passes `auval` validation with zero errors (NEEDS_MIDI_OUTPUT TRUE, uppercase manufacturer code, APVTS version hints on all params)
- [ ] **MAC-03**: Plugin loads and produces correct MIDI in Logic Pro (AU), Reaper (VST3), and Ableton (VST3) on macOS
- [ ] **MAC-04**: SDL2 gamepad (PS4/PS5/Xbox) works on macOS — controller detected, all buttons and axes functional
- [ ] **MAC-05**: Inno Setup configure_file in CMakeLists.txt wrapped in `if(WIN32)` — Mac `cmake` configure runs clean with no errors

### License Keys (LIC)

- [ ] **LIC-01**: First-launch overlay dialog appears in PluginEditor with license key entry field and Activate button (deferred 200ms, non-modal overlay — not a blocking modal window)
- [ ] **LIC-02**: Activation calls LemonSqueezy `/activate` on a background juce::Thread (never on audio thread); `instance_id` stored in `juce::PropertiesFile` on success; dialog dismissed
- [ ] **LIC-03**: Plugin calls LemonSqueezy `/validate` on launch with stored `instance_id`; re-validates at most once per 7 days; result cached in PropertiesFile
- [ ] **LIC-04**: Offline grace period of 7 days — if no internet connection and last validation was within 7 days, plugin operates normally; beyond 7 days enters restricted mode
- [ ] **LIC-05**: Unlicensed / expired mode mutes MIDI output after 10 minutes of use; activated and validated plugin has no output restriction
- [ ] **LIC-06**: User can deactivate (free machine slot) via a button in plugin settings/UI; calls LemonSqueezy `/deactivate` endpoint and clears stored instance_id

### Code Signing (SIGN)

- [ ] **SIGN-01**: macOS VST3 + AU bundles signed with Developer ID Application certificate + hardened runtime enabled + entitlements plist (`com.apple.security.network.client`, `com.apple.security.device.usb`, `com.apple.security.device.bluetooth`)
- [ ] **SIGN-02**: macOS plugin bundles notarized via `xcrun notarytool` and stapled — Gatekeeper passes on a clean Mac without any user override
- [ ] **SIGN-03**: Windows installer `.exe` signed with OV/EV certificate or Azure Trusted Signing — SmartScreen warning eliminated or substantially reduced

### Distribution (DIST)

- [ ] **DIST-01**: macOS PKG installer installs VST3 to `~/Library/Audio/Plug-Ins/VST3/` and AU to `~/Library/Audio/Plug-Ins/Components/` via `pkgbuild` + `productbuild`
- [ ] **DIST-02**: macOS PKG signed with Developer ID Installer certificate and notarized
- [ ] **DIST-03**: GitHub release v2.0 has both Windows `.exe` installer and macOS `.pkg` as downloadable assets
- [ ] **DIST-04**: Release notes document macOS 11+ requirement, arm64/Intel support, and LemonSqueezy license activation steps

## Future Requirements

### Enhanced Licensing
- **LIC-F01**: Trial mode with full functionality for 14 days before key required
- **LIC-F02**: In-app "Buy Now" link to LemonSqueezy product page
- **LIC-F03**: License transfer between machines via deactivate-then-activate flow from UI

### Pro Tools / AAX
- **AAX-01**: AAX format for Pro Tools (requires Avid AAX SDK, separate signing)

### Linux
- **LINUX-01**: Linux VST3 build via CMake (deferred — no SDL2 gamepad standardization)

## Out of Scope

| Feature | Reason |
|---------|--------|
| Mac App Store distribution | Sandboxing incompatible with system plugin paths and Keychain patterns; direct distribution only |
| Pro Tools (AAX) | Requires separate Avid AAX SDK + signing; deferred to future milestone |
| Linux build | SDL2 gamepad behavior inconsistent across distros; deferred |
| GitHub Actions CI/CD for Mac build | CI macOS minutes cost; build on local Mac machine for v2.0 |
| Machine fingerprinting / hardware lock | LemonSqueezy activation_limit (2–3 machines) is sufficient; no custom fingerprinting |
| Subscription / recurring billing | One-time purchase via LemonSqueezy for v2.0 |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| MAC-01 | Phase 46 | Pending |
| MAC-02 | Phase 46 | Pending |
| MAC-03 | Phase 46 | Pending |
| MAC-04 | Phase 46 | Pending |
| MAC-05 | Phase 46 | Pending |
| LIC-01 | Phase 47 | Pending |
| LIC-02 | Phase 47 | Pending |
| LIC-03 | Phase 47 | Pending |
| LIC-04 | Phase 47 | Pending |
| LIC-05 | Phase 47 | Pending |
| LIC-06 | Phase 47 | Pending |
| SIGN-01 | Phase 48 | Pending |
| SIGN-02 | Phase 48 | Pending |
| SIGN-03 | Phase 48 | Pending |
| DIST-01 | Phase 49 | Pending |
| DIST-02 | Phase 49 | Pending |
| DIST-03 | Phase 49 | Pending |
| DIST-04 | Phase 49 | Pending |

**Coverage:**
- v2.0 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-10*
*Last updated: 2026-03-10 after initial definition*
