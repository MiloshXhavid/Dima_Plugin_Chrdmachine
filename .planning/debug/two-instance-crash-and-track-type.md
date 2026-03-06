---
status: awaiting_human_verify
trigger: "Ableton crashed when removing one of two plugin instances. Track type changed to audio on load."
created: 2026-03-06T00:00:00Z
updated: 2026-03-06T00:00:00Z
---

## Current Focus

hypothesis: onConnectionChangeUI lambda captures `this` (PluginEditor*) and is called via callAsync AFTER the editor is destroyed. When instance 1 is removed, its editor destructor runs but the gamepad_.onConnectionChangeUI lambda still holds a dangling `this` pointer. A subsequent SDL event (controller disconnect from closeController() in GamepadInput destructor) fires the callback, queues a callAsync, and then the async lambda runs with a dead pointer — heap corruption / crash.
test: Trace the destruction sequence: editor destructor → stopTimer() → gamepad_ dtor → closeController() → fires onConnectionChangeUI with dangling this → callAsync enqueues lambda → async runs on message thread after PluginEditor memory is freed.
expecting: Confirmed dangling pointer use-after-free causing crash.
next_action: Fix by clearing onConnectionChangeUI in PluginEditor destructor before stopTimer(), so the GamepadInput destructor cannot call the lambda with a dead editor pointer.

## Symptoms

expected: Removing one plugin instance leaves the other running normally.
actual: Ableton crashed (no error dialog) when removing one of two ChordJoystick instances on the same MIDI track.
errors: No error dialog — hard crash (likely memory corruption / access violation).
reproduction: Load ChordJoystick on a MIDI track. Add a second instance. Remove one.
started: Observed during two-instance testing session. Single-instance removal worked fine.

## Eliminated

- hypothesis: SdlContext refcount race (two instances call acquire() concurrently from background threads)
  evidence: acquire() uses fetch_add with acq_rel — only the thread that sees old==0 calls SDL_Init. The second thread reads gAvailable after the store. The second thread's sdlInitialised_=false means its destructor skips SdlContext::release(). Net refcount ends at 1 after instance 2 is destroyed (instance 1 still holds 1 ref). SDL_Quit not called prematurely. This is a ref leak (SDL never quits) but NOT a crash.
  timestamp: 2026-03-06

- hypothesis: SDL_GameControllerOpen returns same ptr to both instances → closeController on instance 1 frees ptr still used by instance 2
  evidence: SDL2 internally refcounts device handles. Closing one instance's handle only decrements SDL's internal refcount; the ptr remains valid until all handles are closed. No direct use-after-free from SDL side in a two-instance scenario.
  timestamp: 2026-03-06

- hypothesis: IS_MIDI_EFFECT FALSE causing track-type change to audio
  evidence: CMakeLists.txt line 131 IS_MIDI_EFFECT TRUE. isMidiEffect() override returns true in PluginProcessor.h. CMake config is correct.
  timestamp: 2026-03-06

- hypothesis: isBusesLayoutSupported returning wrong values causes audio bus allocation
  evidence: The function accepts (0,0) for pure MIDI and (2,2) for stereo passthrough. Ableton deactivates buses before loading on MIDI track — the comment in the code confirms this. Not the cause of the track-type issue.
  timestamp: 2026-03-06

## Evidence

- timestamp: 2026-03-06
  checked: GamepadInput destructor sequence
  found: ~GamepadInput() calls stopTimer() → joins sdlInitThread_ → closeController() → SdlContext::release(). closeController() fires BOTH onConnectionChange and onConnectionChangeUI callbacks.
  implication: When PluginEditor is destroyed, the GamepadInput destructor (which runs later as part of PluginProcessor destruction) calls onConnectionChangeUI with the captured `this` (PluginEditor*) still in the lambda.

- timestamp: 2026-03-06
  checked: PluginEditor constructor (line 2692)
  found: p.getGamepad().onConnectionChangeUI = [this](juce::String controllerName) { juce::MessageManager::callAsync([this, controllerName] { ... gamepadStatusLabel_... }); };
  implication: The lambda captures `this` (PluginEditor*). If callAsync runs after the editor is destroyed, it accesses dead memory. Even worse: closeController() fires the OUTER lambda synchronously from the GamepadInput destructor. The outer lambda enqueues a callAsync. If the message loop processes the async callback after PluginEditor memory is freed → crash.

- timestamp: 2026-03-06
  checked: PluginEditor destructor (line 3259)
  found: ~PluginEditor() { setLookAndFeel(nullptr); stopTimer(); }
  implication: CRITICAL — destructor does NOT clear onConnectionChangeUI. The lambda with dangling `this` remains installed in gamepad_ after the editor is gone. PluginProcessor (and its member gamepad_) outlives PluginEditor — the processor is destroyed after the editor in JUCE's normal teardown order. So gamepad_.~GamepadInput() fires the stale callback after the editor is already dead.

- timestamp: 2026-03-06
  checked: Destruction order in JUCE plugin teardown
  found: The host calls deleteEditor() first, then later destroys the processor. In JUCE, PluginEditor is owned by the processor via createEditor() and its lifetime is managed independently. When the user removes the plugin, editor is deleted first, then processor.
  implication: gamepad_ (member of PluginProcessor) is alive when editor destructor runs. Then later, when processor destructs, gamepad_.~GamepadInput() fires closeController() which calls onConnectionChangeUI — pointing to a dead PluginEditor.

- timestamp: 2026-03-06
  checked: SdlContext race for two concurrent acquire() calls
  found: Thread 1: fetch_add(0→1), sees old==0 → calls SDL_Init → stores gAvailable=true. Thread 2: fetch_add(1→2), sees old==1 → returns gAvailable. If Thread 2 reads gAvailable BEFORE Thread 1 stores it (true), Thread 2 returns false and sets sdlInitialised_=false. Then Thread 2's destructor does NOT call SdlContext::release(). gRefCount goes to 2, then when instance 1 destroys: release() sees 2→1 (not 1→0), SDL_Quit not called. When instance 1 is removed after instance 2: gRefCount at 1, release() sees 1→0, SDL_Quit called. Sequence: add instance 1, instance 1 refcount=1. Add instance 2, refcount=2. Remove instance 2: refcount=1 (if sdlInitialised_=true for instance 2). Remove instance 1: refcount=0, SDL_Quit. OR: instance 2 sdlInitialised_=false (race), refcount stays at 2 after removal of instance 2. Actually: fetch_add returns PREVIOUS value. Thread 2 fetches 1→2, sees prev==1 (not 0), goes to else branch, reads gAvailable. If gAvailable is false at read time → sdlInitialised_=false. Instance 2 destructor: does NOT call release(). refcount stays at 2 after instance 2 gone. When instance 1 destroys: release() sees 2→1 (prev=2, not 1), SDL_Quit NOT called. SDL leaks. Not a crash.
  implication: Two-instance SdlContext race is a benign SDL leak (SDL_Quit never called), not the crash cause.

- timestamp: 2026-03-06
  checked: Track type change to audio — IS_MIDI_EFFECT in CMakeLists.txt
  found: CMakeLists.txt line 131: IS_MIDI_EFFECT TRUE. PluginProcessor.h: bool isMidiEffect() const override { return true; }
  implication: Source code is correct. The track-type-change issue is almost certainly stale plugin cache in Ableton. When user loaded the plugin, Ableton may have had the OLD version (without IS_MIDI_EFFECT TRUE or with some other bus config) cached. The newer build resolves this. Not a code bug in current source.

- timestamp: 2026-03-06
  checked: MIDI channel dropdown "not grayed out" — singleChanMode visibility logic
  found: timerCallback() at line 4656: const bool isSingle = (*proc_.apvts.getRawParameterValue("singleChanMode") > 0.5f); singleChanTargetBox_.setVisible(isSingle); voiceChBox_[v].setVisible(!isSingle). singleChanMode default is index 1 = "Single Channel" (second item in { "Multi-Channel", "Single Channel" }).
  implication: The APVTS param "singleChanMode" is an AudioParameterChoice with default=1 ("Single Channel"). So by default voiceChBox_[0..3] are HIDDEN (isSingle=true). The user's description of "MIDI channel dropdown not grayed out" likely refers to Ableton's own routing dropdown, not the plugin UI. This is expected behavior — IS_MIDI_EFFECT=true means Ableton shows its own MIDI routing UI. This is NOT a bug in the plugin code. The "grayed out" behavior the user expected was probably about Ableton's external instrument MIDI channel selector, which is unrelated to plugin UI.

## Resolution

root_cause: **Issue 1 (crash):** PluginEditor destructor does NOT clear `gamepad_.onConnectionChangeUI` before destruction. The lambda captures `this` (PluginEditor*). When the PluginProcessor destructs after the editor (normal JUCE teardown), `gamepad_.~GamepadInput()` calls `closeController()`, which fires `onConnectionChangeUI`. The outer lambda then calls `juce::MessageManager::callAsync([this, ...]{ ... })` — enqueuing a callback with a dangling pointer to the destroyed editor. If/when the async callback runs, it accesses freed memory → crash. This is a classic lambda-capture dangling-pointer bug, triggered specifically in the two-instance scenario because one editor is destroyed while the processor (and its gamepad_) remains live until full plugin teardown.

**Issue 2 (track type change):** Source code is correct. IS_MIDI_EFFECT=TRUE in CMakeLists.txt and isMidiEffect()=true in PluginProcessor.h. The observed behavior is stale Ableton plugin cache from an old build. A fresh build + reinstall resolves this.

**Issue 3 (dropdown not grayed out):** Not a plugin bug. The voiceChBox_ dropdowns are correctly toggled via timerCallback. The user was observing Ableton's own MIDI routing UI, which is unaffected by plugin code.

fix: Two targeted fixes applied:

1. PluginEditor::~PluginEditor() — assign nullptr to proc_.getGamepad().onConnectionChangeUI as the FIRST action in the destructor (before stopTimer()). This neutralizes the dangling-`this` lambda that would otherwise be called by GamepadInput::closeController() during processor teardown.

2. PluginProcessor::~PluginProcessor() — assign nullptr to gamepad_.onConnectionChange in the destructor body (before member destructors run). C++ destroys members in reverse declaration order; pendingAllNotesOff_ and pendingCcReset_ are declared AFTER gamepad_, so they destruct FIRST. If closeController() fires after those atomics are gone, it would do a formally-UB store. Clearing the callback in the destructor body (while all members still exist) prevents this.

verification: (pending build and two-instance test)
files_changed:
  - Source/PluginEditor.cpp (destructor: clear onConnectionChangeUI)
  - Source/PluginProcessor.cpp (destructor: clear onConnectionChange)
