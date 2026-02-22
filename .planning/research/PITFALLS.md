# Pitfalls Research

**Domain:** JUCE VST3 MIDI-only plugin with SDL2 gamepad integration (Windows)
**Researched:** 2026-02-22
**Confidence:** MEDIUM (all external research tools unavailable; findings derived from direct source code analysis + training knowledge of JUCE/SDL2/VST3 ecosystem; critical claims marked with confidence level)

---

## Critical Pitfalls

### Pitfall 1: std::mutex on the Audio Thread Inside LooperEngine

**What goes wrong:**
`LooperEngine::process()` acquires `std::lock_guard<std::mutex> lk(eventsMutex_)` on every audio callback (line 143 of LooperEngine.cpp). The recording helpers `recordJoystick()` and `recordGate()` also acquire the same mutex. `processBlock()` in PluginProcessor.cpp calls all three of these from the audio thread. The mutex can block the audio thread if the UI or gamepad thread happens to call `reset()` or `deleteLoop()` simultaneously — both of which also acquire `eventsMutex_`. A blocked audio thread causes glitches (xruns) and, in some DAWs that hold their own locks during the plugin's processBlock, risks priority inversion or deadlock.

**Why it happens:**
Using `std::mutex` is the natural first approach when protecting a `std::vector`. The audio thread accesses it in `process()`, and recording writes happen from the same audio thread, so the lock is usually uncontested. But the destructive ops (`reset`, `deleteLoop`) are called from the UI/gamepad timer thread, which creates a cross-thread contention point.

**How to avoid:**
Replace the `eventsMutex_` / `std::vector` design with a lock-free ring buffer or a JUCE `AbstractFifo` + fixed-size array. Events are recorded to the ring buffer on the audio thread; playback reads from it on the audio thread. UI-side destructive operations post a command via an atomic flag (e.g., `pendingClear_`) and the audio thread drains it at the top of `process()`. Alternatively, use a JUCE `SpinLock` — it will not sleep the audio thread, though it still busy-waits under contention.

**Warning signs:**
- Occasional audio dropout / xrun reported in DAW when pressing looper Reset/Delete during playback
- JUCE assertion fired: "This method should not be called from the audio thread" (if JUCE detects you're doing allocations — `events_.push_back` can reallocate, triggering an assert in debug builds)
- Ableton Live's audio thread watchdog killing the plugin

**Phase to address:** Looper implementation phase — before any DAW testing. The `.reserve(4096)` in the constructor reduces reallocation risk but does not prevent it if events exceed 4096.

**Severity: CRITICAL**

---

### Pitfall 2: SDL_Init / SDL_Quit Inside a Shared-Library (VST3 DLL) Lifetime

**What goes wrong:**
`SDL_Init(SDL_INIT_GAMECONTROLLER)` is called in `GamepadInput::GamepadInput()` and `SDL_Quit()` is called in `~GamepadInput()`. The VST3 is a DLL. In DAWs that create and destroy plugin instances during scanning, project loading, or when the user removes/re-adds the plugin, this means `SDL_Init` and `SDL_Quit` can be called multiple times within the same process. SDL2 uses a global internal subsystem reference count, so this is nominally safe for single-threaded use. However, some DAWs (Ableton Live 11+, some hosts using Plugin Alliance's scanning infrastructure) instantiate plugins from background threads during scanning. If two plugin instances are created concurrently, two `SDL_Init` calls race on SDL's global state, which is not thread-safe during initialization. Additionally, `SDL_Quit()` tears down all of SDL including subsystems initialized by other instances, which means a second live instance loses its gamepad connection silently.

**Why it happens:**
SDL2's global state model was designed for applications with single-point entry/exit. Embedding it in a DLL that can have multiple instances in the same process breaks this assumption. The code correctly uses `SDL_INIT_GAMECONTROLLER` only (no video subsystem), which avoids the worst issues (no window handle conflicts), but SDL's event pump and joystick driver are still global.

**How to avoid:**
- Implement a process-level SDL lifecycle singleton using a reference-counted static: first `GamepadInput` constructor calls `SDL_Init`, last destructor calls `SDL_Quit`. Protect the reference count with an `std::atomic<int>` or a `std::call_once` init guard.
- Alternatively, use `SDL_WasInit()` to check before calling `SDL_Init`, and only call `SDL_Quit()` when the ref count reaches zero.
- For scanning safety, wrap the init in a try-catch with a known-safe fallback (disable gamepad silently if SDL fails).

```cpp
// Example: process-level SDL singleton
namespace SdlLifetime {
    static std::atomic<int> refCount {0};
    static std::mutex        initMutex;

    void acquire() {
        std::lock_guard<std::mutex> lk(initMutex);
        if (refCount.fetch_add(1) == 0)
            SDL_Init(SDL_INIT_GAMECONTROLLER);
    }
    void release() {
        std::lock_guard<std::mutex> lk(initMutex);
        if (refCount.fetch_sub(1) == 1)
            SDL_Quit();
    }
}
```

Note: this init-guard mutex only runs at plugin instantiation/destruction, never on the audio thread — safe.

**Warning signs:**
- Second plugin instance has no gamepad connection even with controller plugged in
- DAW crashes during plugin scanning (especially Ableton Live's plugin scan subprocess)
- SDL error "already initialized" or silent failure reported via `DBG("SDL_Init failed")`
- In debug builds, SDL_assert fires during concurrent init

**Phase to address:** SDL2 integration / initial build phase.

**Severity: CRITICAL**

---

### Pitfall 3: Continuous Filter CC Output When No Gamepad Is Connected

**What goes wrong:**
In `processBlock()`, the filter CC section always fires (lines 343–344 of PluginProcessor.cpp):
```cpp
midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCut), 0);
midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, ccRes), 0);
```
When no gamepad is connected, `getFilterX()` / `getFilterY()` return 0.0f (the atomic default). The math maps 0.0f to `atten * 0.5f` — so with the default attenuator of 64.0, the plugin outputs CC74=32 and CC71=32 every single audio block, forever, whether or not a gamepad is present. This saturates the DAW's MIDI output with junk CC messages at the audio callback rate (typically 44–96 times per second), which: (a) confuses any downstream synthesizer whose filter CC74/71 is mapped; (b) interferes with DAW automation recording; (c) generates thousands of CC events per second in the DAW's MIDI monitor.

**Why it happens:**
The CC output logic does not gate on gamepad connection state. The dead-zone inside `normaliseAxis()` returns 0.0f only for axis raw values (it checks the SDL axis), but the processor-level filter code runs unconditionally.

**How to avoid:**
Gate the filter CC output on `gamepad_.isConnected()`:
```cpp
if (gamepad_.isConnected()) {
    midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCut), 0);
    midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, ccRes), 0);
}
```
Additionally consider tracking the previous CC value and only emitting on change, to avoid redundant events even when connected.

**Warning signs:**
- DAW MIDI Monitor shows thousands of CC74/CC71 messages flooding every second
- Synthesizer connected to plugin output has its filter constantly overriding any automation
- Ableton MIDI clips show dense CC automation recorded from the plugin even when no gamepad is plugged in

**Phase to address:** First compile / first DAW test.

**Severity: CRITICAL** — will make the plugin unusable for any user without a gamepad.

---

### Pitfall 4: Note-Off Leak on Plugin Destruction / Bypass

**What goes wrong:**
When the DAW stops or the plugin is removed/bypassed while a voice gate is open, the plugin never fires the note-off MIDI events. The downstream synthesizer's voices remain held at their last pitch indefinitely — the "stuck note" problem. The `TriggerSystem` holds gate state in `gateOpen_[v]` atomics and `activePitch_[v]` integers, but nothing flushes these as note-offs on `releaseResources()` or `PluginProcessor` destruction.

**Why it happens:**
In audio plugins, the `releaseResources()` callback is the correct place to emit cleanup MIDI. JUCE MIDI-only plugins must explicitly output note-offs there. The current implementation's `releaseResources()` body is empty.

**How to avoid:**
In `PluginProcessor::releaseResources()`, iterate over all 4 voices and if `trigger_.isGateOpen(v)`, write `noteOff(voiceCh, activePitch, 0)` into a temporary MidiBuffer, then flush it via `getPlayHead()` if possible — or store a "pending all-notes-off" flag that fires at the top of the next `processBlock()`. Also emit `allNotesOff()` / `allSoundOff()` CC messages (CC123 / CC120) on the relevant channels.

Additionally, when the DAW sends a `reset()` or when the looper fires simultaneous `gateOff` overrides at loop boundary, the pitch tracking must be current — verify that `activePitch_[v]` always reflects the last note-on pitch before emitting note-off.

**Warning signs:**
- Pressing Stop in DAW leaves synthesizer notes ringing
- Removing/re-inserting plugin in DAW signal chain leaves voices stuck on the synth
- Voice gate indicators in UI show "open" even after DAW stop

**Phase to address:** Core MIDI output phase; verified in first DAW integration test.

**Severity: CRITICAL**

---

### Pitfall 5: FetchContent JUCE from `origin/master` (Unversioned)

**What goes wrong:**
The CMakeLists.txt uses `GIT_TAG origin/master` for JUCE:
```cmake
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master)
```
This fetches the tip of JUCE master at configure time. JUCE master can introduce breaking API changes between point releases. When a team member or CI builds the project weeks later, they get a different JUCE than the initial developer, causing non-reproducible builds. JUCE 8's master also occasionally breaks compatibility with specific DAW plugin validators (especially the VST3 validator Steinberg publishes).

**Why it happens:**
`origin/master` is a common shortcut during early development to get the latest version. Version pinning is deferred as a "later" task and then forgotten.

**How to avoid:**
Pin JUCE to a specific release tag immediately:
```cmake
GIT_TAG 8.0.4   # or whichever is current stable
```
Check https://github.com/juce-framework/JUCE/releases for the current stable tag. Use `GIT_SHALLOW TRUE` to reduce clone time.

SDL2 is already correctly pinned to `release-2.30.9` — apply the same discipline to JUCE.

**Warning signs:**
- Build breaks after `cmake --fresh` on a different machine
- JUCE API call that worked last week fails to compile
- VST3 validator reports different results on different developer machines

**Phase to address:** Initial CMake / build setup — fix before first commit.

**Severity: MAJOR** (will cause non-reproducible builds; acceptable temporarily but must be fixed before any collaboration or CI)

---

### Pitfall 6: VST3 Category "Instrument" Blocks MIDI-Only Use in Some DAWs

**What goes wrong:**
The CMakeLists.txt declares:
```cmake
VST3_CATEGORIES "Instrument"
IS_SYNTH FALSE
NEEDS_MIDI_INPUT FALSE
NEEDS_MIDI_OUTPUT TRUE
IS_MIDI_EFFECT FALSE
```
Using `VST3_CATEGORIES "Instrument"` while `IS_SYNTH FALSE` and having no audio output buses creates an unusual plugin type. In Ableton Live, VST3 plugins categorized as "Instrument" are placed on instrument tracks that expect audio output. Since this plugin produces no audio, Live may warn or refuse to load it as an instrument, or load it but show a broken audio routing indicator. Reaper is more permissive. The correct VST3 category for a MIDI generator that is not a synthesizer is `"Generator"` or `"Tools"` combined with `IS_MIDI_EFFECT TRUE` (Fx track in Ableton) or using the MIDI effect routing approach.

**Why it happens:**
JUCE's plugin categories and the DAW's mental model of "instrument vs MIDI effect" do not map cleanly. A MIDI-only generator falls between categories.

**How to avoid:**
Test both approaches against Ableton 11/12 and Reaper 7 before finalizing:
- Option A: `IS_MIDI_EFFECT TRUE`, `VST3_CATEGORIES "Tools"` — loads on MIDI effect track in Ableton (requires routing MIDI to a synth instrument track)
- Option B: `IS_SYNTH TRUE` (even with no audio output), `VST3_CATEGORIES "Instrument"` — loads as instrument but outputs no audio (Reaper tolerates this; Ableton may show routing warnings)

The safest approach for Ableton MIDI output is `IS_SYNTH TRUE` with audio output buses explicitly returning empty buffers — Ableton routes MIDI from instrument tracks more reliably than from MIDI effect slots. Test with the Steinberg VST3 Plugin Validator before release.

**Warning signs:**
- Plugin fails Steinberg's VST3 validator
- Ableton refuses to route MIDI out of the plugin to an external instrument
- Plugin appears in wrong category in DAW browser

**Phase to address:** First DAW compatibility test phase.

**Severity: MAJOR**

---

### Pitfall 7: `std::vector::push_back` Heap Allocation on Audio Thread During Recording

**What goes wrong:**
`LooperEngine::recordJoystick()` and `recordGate()` call `events_.push_back()` from the audio thread (LooperEngine.cpp lines 89–92, 102–103). `push_back` can trigger `std::vector` reallocation when capacity is exceeded. Heap allocation on the real-time audio thread causes unbounded latency spikes — the OS memory allocator uses locks and can take milliseconds to satisfy an allocation request, causing audio dropouts. The `events_.reserve(4096)` in the constructor mitigates this until 4096 events are stored, but at 60 Hz joystick updates (two events per poll: X and Y) for a 16-bar loop at 120bpm, capacity can be exhausted in approximately 4 minutes of continuous recording.

**Why it happens:**
`std::vector` is the natural container. The `reserve()` call is a good attempt but insufficient for long sessions. Recording is an inherently unbounded operation if left uncapped.

**How to avoid:**
- Cap recording: when `events_.size() >= events_.capacity()`, stop recording (set `recording_` to false) and notify the UI. Alternatively, wrap around and overwrite old events (ring buffer semantics).
- Pre-allocate with a safe upper bound. At 120 BPM, 16 bars = 32 beats. Joystick at 60 Hz = 120 events/second. Gate events are sparse. A 16-bar loop at 60 Hz for ~30 seconds = ~3600 event pairs = ~7200 events. Reserve 8192 to cover worst case.
- Alternatively, calculate max events at `prepareToPlay` time using `sampleRate` and `bpm` to set a loop-length-appropriate reserve.

**Warning signs:**
- Audio dropouts appearing after minutes of recording but not at start
- JUCE's JUCE_AUDIO_PROCESSOR_NOT_ON_AUDIO_THREAD assertion fires in debug builds (if `reserve` triggers reallocation through a custom allocator)
- Profiler shows occasional multi-millisecond spikes in processBlock during recording

**Phase to address:** Looper implementation phase.

**Severity: MAJOR**

---

### Pitfall 8: SDL2 SDL_PollEvent on Main Thread Conflicts with DAW Window Message Pump

**What goes wrong:**
`GamepadInput::timerCallback()` calls `SDL_PollEvent()` at 60 Hz on JUCE's main message thread. On Windows, SDL's event pump internally calls `PeekMessage()` / `DispatchMessage()` for certain events — specifically device-connection notifications (WM_DEVICECHANGE), which is how SDL2 detects controller hotplug. When running inside a DAW, the DAW owns the Windows message pump on the main thread. Two systems calling `PeekMessage()` on the same thread is not itself a problem (they share the queue), but SDL may dispatch messages intended for the DAW's window handles, or SDL's window registration (SDL creates a hidden HWnd even in headless mode for the event pump) can conflict with Ableton Live's MIDI device management, which also uses WM_DEVICECHANGE. This has been reported to cause controller detection failures or rare DAW message corruption in some hosts.

**Why it happens:**
SDL2 was designed for standalone applications where it owns the message loop. Inside a DLL hosted by a DAW, ownership of the message loop is ambiguous.

**How to avoid:**
- Explicitly set `SDL_HINT_NO_SIGNAL_HANDLERS` and `SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4` before `SDL_Init` (irrelevant hints but the underlying hint that matters is `SDL_HINT_JOYSTICK_THREAD` — set to "1" to enable SDL's background joystick polling thread on Windows, which bypasses the message pump entirely for gamepad state).
- Use `SDL_HINT_JOYSTICK_THREAD "1"` before `SDL_Init`. This makes SDL poll joystick state on its own thread rather than via the message pump, eliminating the WM conflict. However, it means `SDL_PollEvent` is not needed — use `SDL_GameControllerGetAxis/Button` directly with this hint. Hotplug detection still works via the background thread.
- If `SDL_HINT_JOYSTICK_THREAD` is set, the `SDL_PollEvent` loop in `timerCallback()` can be removed; just poll axis/button values directly.

**Warning signs:**
- DAW loses MIDI device connections after plugin is instantiated
- Gamepad never detected even when plugged in (SDL's WM_DEVICECHANGE message eaten by DAW)
- Ableton Live MIDI preferences showing device disconnections when plugin is active

**Phase to address:** SDL2 integration phase, specifically tested in Ableton.

**Severity: MAJOR** (LOW confidence on exact failure mode — specific DAW interaction is training-knowledge, needs empirical validation in Ableton)

---

### Pitfall 9: Ableton Live MIDI Output Routing for Non-Instrument VST3 Plugins

**What goes wrong:**
Ableton Live's MIDI routing treats VST3 plugins differently based on their type declaration. A plugin on an Instrument track can output MIDI to an external instrument (via MIDI To routing). A plugin on a MIDI Effect track (IS_MIDI_EFFECT TRUE) can output MIDI but only if placed in the MIDI effect chain before an instrument. A pure MIDI generator (no audio output, IS_MIDI_EFFECT FALSE, IS_SYNTH FALSE) may not have its MIDI output routable to external hardware in Ableton Live without workarounds. Reaper handles all of these cases correctly and transparently.

**Why it happens:**
Ableton Live's routing model predates the VST3 MIDI generator category. It was designed around instruments (generate audio) and MIDI effects (transform MIDI in real time) — not MIDI-only generators that are also not effects.

**How to avoid:**
- Test both `IS_MIDI_EFFECT TRUE` and `IS_SYNTH TRUE` configurations in Ableton Live before settling on the final plugin type declaration.
- If targeting Ableton as a primary DAW, consider using `IS_SYNTH TRUE` with a dummy stereo audio output bus (silence) to satisfy Ableton's instrument routing model. This is the approach used by many MIDI generator plugins (e.g., Max for Live devices exported as instruments).
- Document clearly in plugin README which DAW routing setup to use.

**Warning signs:**
- In Ableton, the plugin's MIDI output does not appear in the "MIDI To" dropdown of other tracks
- External synthesizer receives no MIDI from the plugin despite the plugin appearing to function correctly
- Ableton MIDI routing panel shows no MIDI output from the plugin track

**Phase to address:** DAW compatibility phase, required before any user testing.

**Severity: MAJOR**

---

### Pitfall 10: Windows Code Signing and Installer Requirements for Paid Distribution

**What goes wrong:**
Distributing an unsigned VST3 DLL on Windows 10/11 triggers Windows SmartScreen warnings ("Windows protected your PC"). Most musicians will not bypass this — they will assume the plugin is malware and request a refund. Additionally, some DAWs (Studio One, Logic Pro via Bootcamp, certain security-hardened setups) refuse to load unsigned DLLs or show aggressive warnings. A paid plugin distributed via Gumroad without code signing will have high refund/support rates from this alone.

**Why it happens:**
Code signing is a separate infrastructure step (obtaining an EV or OV certificate, signing the DLL and installer). Developers commonly defer it as a "business concern" and are surprised by SmartScreen blocking.

**How to avoid:**
- Obtain an OV (Organization Validation) or EV (Extended Validation) code signing certificate. EV certificates are trusted immediately by SmartScreen; OV certificates build trust over time (reputation score). EV is recommended for commercial plugins.
- Use `signtool.exe` (included in Windows SDK) to sign the VST3 DLL and any installer executable.
- Budget: EV certificates cost ~$400–600/year. OV certificates cost ~$200–400/year. Providers: DigiCert, Sectigo, GlobalSign.
- NSIS or Inno Setup for the installer; Inno Setup is simpler for single-DLL plugins.
- Alternative (LOW cost): Use a self-signed certificate with documented instructions for users to trust it — not recommended for paid plugins.

**Warning signs:**
- SmartScreen blocks DLL during first install
- DAW plugin scanner skips or quarantines the DLL
- Support tickets saying "plugin won't install" from Windows 11 users

**Phase to address:** Distribution/release preparation phase.

**Severity: MAJOR** for paid distribution; MINOR for personal/beta testing use.

---

### Pitfall 11: MSVC Runtime DLL Dependency in VST3

**What goes wrong:**
If the VST3 DLL is built with MSVC (Visual Studio), it links against the MSVC C++ runtime (MSVCP140.dll, VCRUNTIME140.dll). These DLLs must be present on the end user's system. The MSVC redistributable package installs them, but it is not pre-installed on all Windows systems. Without it, the VST3 DLL fails to load with a cryptic "DLL not found" error. The user's DAW typically just reports the plugin as "failed to load" with no useful error message.

**Why it happens:**
CMake with MSVC defaults to dynamic CRT linking. The developer's machine has the runtime installed via Visual Studio. Test machines often have it too. Only clean-install user machines reveal the problem.

**How to avoid:**
- In CMakeLists.txt, set `CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"` to force static CRT linkage. This embeds the runtime in the DLL — no redistributable required. The tradeoff is a larger DLL (~200KB additional).
- Alternatively, ship an installer that includes the MSVC redistributable installer (vc_redist.x64.exe) and runs it silently.
- SDL2 static build already avoids SDL DLL dependency — apply same principle to CRT.

```cmake
# Add to CMakeLists.txt for static CRT
set_property(TARGET ChordJoystick PROPERTY MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>")
```

**Warning signs:**
- "Failed to load plugin" errors on fresh Windows installs during beta testing
- User reports that Reaper/Ableton shows plugin in red or missing in scanner
- Dependency Walker / Dependency Viewer shows unresolved MSVCP140.dll

**Phase to address:** Build configuration phase.

**Severity: MAJOR** for distribution, MINOR during development.

---

### Pitfall 12: Looper + DAW Sync Race: ppqPosition Jumps at Transport Start

**What goes wrong:**
In `LooperEngine::process()`, when `isDawPlaying` is true, the loop position is derived from `std::fmod(p.ppqPosition, loopLen)`. When the DAW transport starts, `ppqPosition` jumps from its stopped value to the play cursor position. If the looper recorded events relative to a different starting ppqPosition, the first playback block after transport restart will scan for events in the new window, which may not match where events were recorded. This causes the looper to fire events at wrong positions or miss the loop start entirely for one cycle.

**Why it happens:**
DAW ppqPosition reflects the global song position, not a loop-relative position. The looper uses `fmod` to reduce it to loop-relative, but the reference point can shift between recording and playback sessions if the user repositions the DAW cursor.

**How to avoid:**
- Record the `ppqPosition` at the moment the looper starts recording (call it `recordStartPpq_`). All recorded event positions should be stored relative to this anchor: `pos = fmod(ppqPosition - recordStartPpq_, loopLen)`.
- On playback, use the same anchor to compute the current loop-relative position: `beatAtBlockStart = fmod(ppqPosition - playbackStartPpq_, loopLen)`.
- Reset the anchor whenever the user presses Record from a stopped state.
- When no DAW sync is available, the internal clock is correct since it starts from 0.

**Warning signs:**
- Looper events fire slightly late or early after stopping and restarting the DAW
- Events cluster at the wrong position when DAW cursor is moved before play
- Gate notes stuck on because note-off fires before note-on in the next cycle

**Phase to address:** Looper implementation phase.

**Severity: MAJOR**

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| `std::mutex` in audio thread | Easy synchronization, correct behavior in simple cases | Audio dropouts under contention; hard to debug timing-related glitches | Never for production audio thread paths |
| `GIT_TAG origin/master` for JUCE | Always get latest features | Non-reproducible builds; breaking API changes | Never — pin immediately |
| Filter CC always emitting | Simple code | Floods MIDI output when no gamepad; confuses synths and DAWs | Never for shipping code |
| No note-off on releaseResources | Less code | Stuck notes on DAW stop/bypass | Never for a MIDI-output plugin |
| `vector::push_back` on audio thread | Simple recording | Heap allocation glitches after reserve exhausted | Only with hard capacity cap enforced |
| Unsigned DLL distribution | Faster to release | SmartScreen blocks install for paid users | Beta testing only |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| SDL2 in DLL | Calling SDL_Init/SDL_Quit per plugin instance | Reference-counted process-level singleton |
| SDL2 event pump in DAW | SDL_PollEvent on main thread causes WM_DEVICECHANGE conflicts | Set `SDL_HINT_JOYSTICK_THREAD "1"` before SDL_Init; poll state directly |
| JUCE APVTS + audio thread | Calling `getParameter()` (returns AudioParameter*) on audio thread — valid, but `getRawParameterValue()` is faster and lock-free | Use `getRawParameterValue()` exclusively on audio thread; current code is correct |
| VST3 MIDI output in Ableton | IS_MIDI_EFFECT or IS_SYNTH confusion leaves MIDI unroutable | Test both modes; IS_SYNTH TRUE with empty audio output is most compatible |
| MSVC CRT | Dynamic CRT means redistributable required | Static CRT (`/MT`) bundles runtime in DLL |
| FetchContent + CI | Network fetch on every CI run is slow and fragile | Use `GIT_SHALLOW TRUE` and pin exact tag; consider vendoring |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| `std::mutex` block on audio thread | Intermittent audio dropouts; harder to reproduce at lower buffer sizes | Lock-free design for event buffer | Any buffer size < 512 samples under DAW load |
| `vector::push_back` reallocation on audio thread | Sporadic millisecond spike in processBlock profiler | Hard capacity cap + reserve at construction | After 4096 events recorded (~3-5 min of dense recording) |
| 12 `getRawParameterValue().load()` calls per block for scale notes | Minimal in practice — atomics are fast, but ~30 total loads per block | Cache scale params in a struct at top of processBlock | Not a real bottleneck at current scale; revisit if CPU profiling shows it |
| Filter CC every block | MIDI buffer bloat; downstream synth busy processing redundant CCs | Gate on isConnected() + emit only on value change | Immediately — every block at the audio callback rate |
| SDL timer at 60 Hz on message thread | Main thread load; JUCE timer jitter under DAW load | Background thread via SDL_HINT_JOYSTICK_THREAD | Jitter becomes audible when DAW GUI is heavily loaded |

---

## "Looks Done But Isn't" Checklist

- [ ] **Note cleanup:** Does the plugin send note-off for all open gates when `releaseResources()` is called? Verify by: start a note, stop DAW, check synth has no held voices.
- [ ] **Gamepad-absent users:** Does the plugin work correctly (no MIDI spam, no crash, no missing UI controls) with zero gamepads attached? Verify by: run plugin with no controller, check MIDI monitor for spurious CC messages.
- [ ] **MIDI channel routing:** Do all 4 voices actually transmit on their configured channels (not just channel 1)? Verify by: set voice 2 to channel 5, trigger it, confirm MIDI monitor shows ch5.
- [ ] **Scale quantization edge cases:** Does quantize handle a scale with only one note active? With all 12 active? Verify boundary conditions in ScaleQuantizer.
- [ ] **Looper boundary events:** Are gate note-offs always paired with their note-ons across loop boundaries? Verify that a gate that starts near the end of the loop and wraps does not produce orphaned note-ons.
- [ ] **State persistence round-trip:** Save preset, close plugin, reopen — do all 40+ parameters restore correctly including the 12 custom scale notes? Test in both Ableton and Reaper.
- [ ] **VST3 validator passes:** Run Steinberg's VST3 validator (vst3-validator or pluginval) against the built DLL with zero failures.
- [ ] **SDL_Init failure is silent:** If SDL_Init fails (no gamepad driver installed), does the plugin still load and function as a mouse-driven instrument? Verify by blocking SDL_Init and confirming the plugin degrades gracefully.

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Mutex on audio thread | MEDIUM | Redesign LooperEngine event storage to use JUCE AbstractFifo + fixed array; replace all mutex guards with atomic flag for destructive ops |
| SDL_Init/SDL_Quit per instance | LOW | Wrap in 10-line reference-counted singleton; can be done without touching callers |
| Filter CC always emitting | LOW | Add one `if (gamepad_.isConnected())` guard around the CC block |
| Note-off leak | LOW | Add flush loop to releaseResources(); ~10 lines |
| JUCE unpinned version | LOW | Change one line in CMakeLists.txt; rebuild |
| VST3 category wrong | MEDIUM | Change CMakeLists, rebuild, retest all DAW routing |
| MSVC CRT | LOW | One CMakeLists.txt property; rebuild |
| Vector reallocation on audio thread | MEDIUM | Refactor event storage to AbstractFifo with fixed capacity; requires LooperEngine redesign |
| ppqPosition anchor drift | MEDIUM | Add recordStartPpq_ anchor field; adjust recording and playback position calculations |
| Code signing | HIGH (time/cost) | Obtain EV certificate ($400-600); integrate signtool into build pipeline |

---

## Pitfall-to-Phase Mapping

| Pitfall | Severity | Prevention Phase | Verification |
|---------|----------|------------------|--------------|
| std::mutex on audio thread | CRITICAL | Looper implementation | No xruns under DAW stress test with looper active |
| SDL_Init/SDL_Quit per instance | CRITICAL | SDL2 integration (initial build) | Two plugin instances in one DAW process, both detect gamepad |
| Filter CC always emitting | CRITICAL | First compile / smoke test | MIDI monitor shows no CC when gamepad disconnected |
| Note-off leak | CRITICAL | Core MIDI output | Stop DAW with gates open; synth voices all clear |
| FetchContent JUCE unpinned | MAJOR | Build setup (first task) | Second developer machine builds identical binary |
| VST3 category/routing | MAJOR | DAW compatibility phase | MIDI output routes to external synth in Ableton AND Reaper |
| Vector reallocation on audio thread | MAJOR | Looper implementation | Profiler shows no spikes after 5+ min of dense recording |
| SDL event pump conflict | MAJOR | SDL2 integration | Ableton MIDI devices stable after plugin instantiation |
| Ableton MIDI routing | MAJOR | DAW compatibility phase | External instrument receives MIDI in Ableton Live 11/12 |
| Code signing | MAJOR | Release preparation | SmartScreen does not block install on clean Windows 11 VM |
| MSVC CRT | MAJOR | Build configuration | Plugin loads on clean Windows 11 VM with no VS installed |
| ppqPosition anchor drift | MAJOR | Looper implementation | Events fire at correct beat after DAW cursor repositioned |

---

## Sources

- Direct source code analysis: `Source/GamepadInput.cpp`, `Source/LooperEngine.cpp`, `Source/LooperEngine.h`, `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/TriggerSystem.cpp`, `Source/PluginEditor.cpp`, `CMakeLists.txt` — HIGH confidence (first-hand evidence)
- SDL2 subsystem reference counting behavior: `SDL_Init` / `SDL_Quit` documentation at https://wiki.libsdl.org/SDL2/SDL_Init — training knowledge, MEDIUM confidence; `SDL_HINT_JOYSTICK_THREAD` documentation at https://wiki.libsdl.org/SDL2/SDL_HINTS — training knowledge, MEDIUM confidence
- JUCE audio thread constraints (no heap allocation, no mutex blocking): JUCE documentation "The Audio Callback" and JUCE forum consensus — MEDIUM confidence (external sources unavailable for verification this session)
- VST3 MIDI generator category behavior in Ableton Live: training knowledge of Ableton Live 11/12 VST3 hosting behavior — LOW confidence, requires empirical testing
- Windows code signing / SmartScreen: Microsoft documentation — MEDIUM confidence
- MSVC static CRT: CMake MSVC_RUNTIME_LIBRARY documentation — HIGH confidence (well-documented CMake feature)

---

*Pitfalls research for: JUCE VST3 MIDI plugin with SDL2 gamepad integration (ChordJoystick)*
*Researched: 2026-02-22*
*External research tools unavailable this session — all findings from direct code inspection + training knowledge. Mark LOW/MEDIUM confidence items for empirical validation during development.*
