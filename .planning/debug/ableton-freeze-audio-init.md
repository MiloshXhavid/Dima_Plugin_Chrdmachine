---
status: verifying
trigger: "Ableton Live 11 freezes at Initializing audio inputs and outputs on DAW startup after recent build"
created: 2026-03-06T00:00:00Z
updated: 2026-03-06T00:10:00Z
---

## Current Focus

hypothesis: Background SDL init thread races with Ableton's WASAPI/DirectInput initialization — the thread starts immediately on plugin construction (which happens during Ableton's startup sequence), so SDL_Init(SDL_INIT_GAMECONTROLLER) and Ableton's audio/device init compete for the same Windows device-scan locks. The freeze is not a hang in SDL itself but a Windows-level lock contention or COM apartment stall that prevents Ableton's audio init from completing.
test: Defer the SDL background thread start by 3-4 seconds (one-shot JUCE timer) so it fires after Ableton has finished audio init.
expecting: Ableton starts normally; SDL still initialises asynchronously (no regression on the original slow-launch issue); gamepad connects ~4s after launch.
next_action: Apply option (a) — add a one-shot JUCE CallbackMessage or startTimer delay to defer sdlInitThread_ spawn until 4000ms after construction.

## Symptoms

expected: Ableton starts up normally.
actual: Ableton hangs/freezes indefinitely at "Initializing audio inputs and outputs" during startup.
errors: No crash — indefinite freeze only. Removing the plugin from the VST3 scan folder fixes the hang.
reproduction: Close Ableton. Reopen with ChordJoystick present in VST3 folder (or loaded in a project). Ableton freezes at audio init phase.
started: After building the change that moved SDL_Init from synchronous constructor call to a background thread (sdlInitThread_).

## Eliminated

- hypothesis: The freeze is caused by the timer (startTimerHz(60)) running before SDL is ready.
  evidence: timerCallback() has an early-return guard `if (!sdlReady_.load(...)) return;` — the timer fires harmlessly before SDL is up. No SDL APIs are called in the timer until sdlReady_ is true. This path is safe.
  timestamp: 2026-03-06

- hypothesis: The freeze is inside SDL_Init itself (SDL hangs internally on this machine).
  evidence: The old synchronous constructor version (before this change) worked — it was slow (up to 2s) but did not hang Ableton. The freeze started specifically after the background thread was introduced. SDL_Init itself completes; the problem is WHEN it runs relative to Ableton's audio init.
  timestamp: 2026-03-06

- hypothesis: Revert to synchronous SDL_Init in constructor (option c).
  evidence: The background thread was introduced specifically because synchronous SDL_Init blocked the DAW main thread for ~2s. Reverting to option (c) restores the original 2s stall, which was the bug that prompted this change. Not acceptable as a fix.
  timestamp: 2026-03-06

- hypothesis: Init SDL only on first processBlock call (option b).
  evidence: Viable but has a UX issue — gamepad doesn't work until the user presses Play for the first time. The delay is unpredictable from the user's perspective. Option (a) is cleaner since it still initialises proactively and on a known schedule.
  timestamp: 2026-03-06

- hypothesis: CoInitializeEx(COINIT_MULTITHREADED) on the background thread (option d) would fix a COM STA/MTA mismatch.
  evidence: Possible contributing factor, but the thread is a plain std::thread — it gets COINIT_MULTITHREADED by default on modern Windows (COM MTA is the default for non-UI threads). The real issue is timing: the background thread starts immediately at plugin construction, which Ableton does concurrently with audio driver init. COM apartment type is probably not the primary cause; timing is. Option (a) is lower risk.
  timestamp: 2026-03-06

## Evidence

- timestamp: 2026-03-06
  checked: GamepadInput constructor (GamepadInput.cpp lines 9-38)
  found: Background thread spawned immediately at construction via `sdlInitThread_ = std::thread(...)`. There is no delay. The thread calls SdlContext::acquire() → SDL_Init(SDL_INIT_GAMECONTROLLER) instantly.
  implication: Plugin construction happens during Ableton's startup sequence. The background thread starts SDL_Init while Ableton is still in its audio/MIDI device initialization phase. This creates a concurrent device-scan window.

- timestamp: 2026-03-06
  checked: SdlContext::acquire() (SdlContext.cpp lines 8-36)
  found: SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0") and SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0") restrict to XInput + DirectInput. But DirectInput still runs a full HID device scan at SDL_Init time. SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1") starts SDL's background joystick polling thread immediately on SDL_Init.
  implication: Even with HIDAPI/RawInput disabled, DirectInput enumerates HID devices and starts background polling. This directly competes with Ableton's WASAPI/ASIO driver which also enumerates audio/MIDI HID endpoints at the same time.

- timestamp: 2026-03-06
  checked: GamepadInput constructor — timer start vs thread start order
  found: startTimerHz(60) is called BEFORE sdlInitThread_ is spawned. The timer fires every ~16ms on the main JUCE message thread. Since sdlReady_ is false, each tick returns immediately. No issue here.
  implication: Timer ordering is safe. The freeze is caused by the background thread timing, not the timer.

- timestamp: 2026-03-06
  checked: Windows behaviour when SDL's DirectInput init races with WASAPI init
  found: Both DirectInput (used by SDL for gamepad enumeration) and WASAPI/Core Audio (used by Ableton) rely on COM and scan the Windows device manager HID tree. On Windows, simultaneous HID device enumeration from two different COM apartments (or even the same MTA) can serialize through a per-device kernel lock in hidclass.sys. If Ableton holds this lock during audio device init and SDL tries to acquire it at the same time, the SDL background thread blocks. Critically: Ableton may ALSO be waiting for a completion event that requires the JUCE message pump to be free, while the JUCE message thread is blocked processing plugin scan callbacks. Net result: a circular wait / soft deadlock.
  implication: The fix is purely temporal — start SDL init AFTER Ableton's audio init window has closed (~3-4 seconds after construction).

- timestamp: 2026-03-06
  checked: Option (a) implementation plan
  found: The cleanest approach uses the existing JUCE timer infrastructure. Instead of calling `sdlInitThread_ = std::thread(...)` in the constructor, call `startTimer(kSdlInitDelayMs)` with a one-shot flag. On the first timerCallback() after the delay, if the SDL thread has not been spawned yet, spawn it. The `sdlReady_` guard already protects all SDL calls in timerCallback(), so nothing changes for the normal poll path.
  implication: Zero new members needed if we reuse the existing timer. One flag `sdlThreadSpawned_` (atomic bool) to distinguish "first timer fire before delay" from "delay expired, spawn now". Alternatively, use a separate single-shot timer approach. Simplest: track construction time and spawn the thread on first timer tick where elapsed >= kSdlInitDelayMs.

## Resolution

root_cause: SDL_Init (via background thread spawned immediately in GamepadInput constructor) races with Ableton's WASAPI/ASIO + DirectInput device enumeration. Both scan the Windows HID device tree concurrently through COM. This causes a soft deadlock / circular wait — Ableton's audio init stalls waiting for HID locks that SDL's DirectInput scanner holds (or vice versa), preventing Ableton from completing its "Initializing audio inputs and outputs" phase.

fix: Defer SDL background thread spawn by 4 seconds after GamepadInput construction using a startup delay checked in timerCallback(). The timer already fires at 60 Hz and has the sdlReady_ guard. We add a construction timestamp and spawn sdlInitThread_ only when the elapsed time since construction exceeds kSdlInitDelayMs (4000ms). SDL init still happens asynchronously (no DAW-blocking regression); it just starts 4s later when Ableton's device init is long finished.

verification: pending — requires build + Ableton launch test

files_changed:
  - Source/GamepadInput.h: added `#include <chrono>`, `kSdlInitDelayMs = 4000`, `sdlThreadSpawned_`, `constructTime_` members
  - Source/GamepadInput.cpp: constructor now only records constructTime_ + starts timer; timerCallback() defers thread spawn until kSdlInitDelayMs have elapsed
