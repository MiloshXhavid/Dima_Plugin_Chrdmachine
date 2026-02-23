# Project State

## Project Reference

**ChordJoystick** — XY joystick → 4-voice chord MIDI VST3, gamepad + looper, paid Windows release.

Core value: Continuous harmonic navigation via joystick with per-voice sample-and-hold gates — no competitor provides this.

## Current Position

- **Phase:** 07 of 7 — Distribution and Release — **COMPLETE**
- **Plan:** 07-02 COMPLETE (2/2 plans done) — Phase 07 fully done
- **Status:** Milestone complete

## Progress

```
Phase 01 [██████████]   Build Foundation    (COMPLETE — Ableton crash fixed 05-03, all criteria retroactively met)
Phase 02 [██████████]   Engine Validation   (COMPLETE — ScaleQuantizer+ChordEngine 15 tests green, checkpoint approved)
Phase 03 [██████████]   Core MIDI Output    (COMPLETE — 2/2 plans done, all 6 DAW tests passed in Reaper)
Phase 04 [████████░░]   Trigger Sources     (IN PROGRESS — 04-01+04-02 COMPLETE, 04-03 pending if planned)
Phase 05 [██████████]   Looper Hardening    (COMPLETE — 3/3 plans done, Reaper+Ableton verified)
Phase 06 [██████████]   SDL2 Gamepad        (COMPLETE — 4/4 plans done, all 8 DAW tests approved)
Phase 07 [██████████]   Distribution   (COMPLETE — 07-01 pluginval + 07-02 Inno Setup installer, clean-machine approved)

Overall: [██████████] 100% (All 7 phases complete — v1.0 release candidate ready)
```

## What's Been Built

- All 14 source files written (full implementation):
  - ScaleQuantizer, ChordEngine, TriggerSystem, LooperEngine, GamepadInput
  - PluginProcessor, PluginEditor
- APVTS parameter layout (all 40+ params) committed
- Scale quantization implementation committed
- PROJECT.md with validated requirements
- Full research completed (.planning/research/)
- **[NEW] CMakeLists.txt fixed: JUCE 8.0.4 pinned, static CRT set (01-01)**
- **[NEW] PluginProcessor.cpp fixed: Ableton dummy bus, isBusesLayoutSupported updated (01-01)**
- **[NEW] Build verified: ChordJoystick.vst3 compiled and installed (Visual Studio 18 2026)**
- **[NEW] Catch2 v3.8.1 FetchContent added, ChordJoystickTests target, ScaleQuantizer 218 assertions green (02-01)**
- **[NEW] ChordEngine 9-case test suite added; combined suite 15 tests, 0 failures; axis routing, transpose, octave offsets, MIDI clamping, scale quantization verified (02-02)**
- **[NEW] TriggerSystem::resetAllGates() added; processBlockBypassed() flushes note-offs on bypass; releaseResources() calls resetAllGates(); noteOff uses explicit (uint8_t)0; green gate LEDs; channel conflict warning (03-01)**
- **[NEW] DAW verification complete: all 6 Reaper tests passed — 4-voice note-on/off, retrigger safety, bypass flush, transport sustain, green LEDs, channel conflict warning (03-02)**
- **[NEW] Continuous joystick gate model: Chebyshev magnitude threshold, joystickThreshold APVTS param (04-01)**
- **[NEW] THRESHOLD horizontal slider in PluginEditor right column; TouchPlate pads dim and disable in JOY/RND mode (04-01)**
- **[NEW] JOY retrigger model (final): gate opens with noteOn(pitch); when joystick crosses scale-degree boundary, fires noteOff(oldPitch) then noteOn(newPitch); closes after 50ms stillness. No pitch bend / RPN. Works with any synth. (04-01 Fix 5 / 418c2a0)**
- **[NEW] Per-voice ppqPosition-synced random clock: prevSubdivIndex_[4] integer comparison fires on boundary; sample-count fallback when transport stopped (04-02 / 2b256a9)**
- **[NEW] randomDensity 1..8 hits-per-bar model with hitsPerBarToProbability(); randomGateTime knob (fraction of subdivision, 10ms floor) for note duration (04-02 / 2b256a9)**
- **[NEW] randomSubdiv0..3 per-voice APVTS params; 4 per-voice combo boxes in PluginEditor column-aligned under trigger source selectors; randomGateTimeKnob_ rotary (04-02 / 448556c)**
- **[NEW] randomClockSync APVTS bool (default true): sync mode = DAW-only triggers, free mode = internal BPM accumulator; randomFreeTempo APVTS float (30-240 BPM, default 120) for free-running clock (04-02 / 30f52d4)**
- **[NEW] 4-column DENS/GATE/FREE BPM/SYNC random controls row; randomDensityKnob_ changed to NoTextBox rotary for visual consistency; labels visible above knob row (04-02 / 43f0724, 65f069c)**
- **[NEW] 04-02 human checkpoint APPROVED in Reaper — all 5 random gate tests passed (per-voice independence, DAW sync, density, gate time, mode switch clean)**
- **[NEW] LooperEngine fully rewritten lock-free: AbstractFifo SPSC double-buffer (eventBuf_+fifo_ record side, playbackStore_ playback side), punch-in merge (loopLen/64 touch radius), deleteRequest_/resetRequest_ atomic flags serviced at top of process(), ASSERT_AUDIO_THREAD() macro (05-01 / 72a9eeb, 3de3cec)**
- **[NEW] 11 new Catch2 LooperEngine tests: FIFO stress no-deadlock, DAW sync anchor, punch-in correctness, loop-wrap boundary sweep (96 combos: 6 subdivs x 16 bar lengths); all 26 tests pass. ENABLE_TSAN CMake option with MSVC guard. ScopedRead-before-reset ordering bug fixed. (05-01 / f27440b)**
- **[NEW] PluginProcessor: 7 new looper forwarding methods (looperSetRecJoy/SetRecGates/SetSyncToDaw/IsCapReached/IsSyncToDaw/IsRecJoy/IsRecGates); JUCE 8 AudioPlayHead::getPosition() Optional API replacing deprecated getCurrentPosition; looper gate playback bypasses TriggerSystem (direct MIDI emission) (05-02 / 290849c)**
- **[NEW] PluginEditor: [REC JOY], [REC GATES], [SYNC] TextButtons in second looper button row; onClick lambdas wire to forwarding methods; timerCallback 30 Hz state mirror; ~5 Hz capReached flash indicator (05-02 / fb1a71c)**
- **[NEW] Ableton MIDI effect fix: isMidiEffect()=true, acceptsMidi()=true, empty BusesProperties(), 0-channel isBusesLayoutSupported — plugin now loads in Ableton on MIDI tracks (05-03 / 9c207ce)**
- **[NEW] Per-voice slew: CC5 (portamento time) + CC65 (portamento on) emitted before every note-on; slewVoice0..3 APVTS params; 4 slew knobs in PluginEditor (05-03 / 4bafca2)**
- **[NEW] recordPending_ atomic: REC press defers recording start to next valid clock pulse (05-03 / 4bafca2)**
- **[NEW] DAW stop → looper all-notes-off in SYNC mode via prevDawPlaying_ edge detection (05-03 / 4bafca2)**
- **[NEW] 05-03 Reaper verification COMPLETE: all 4 tests passed (loads, 4-bar loop, DAW SYNC alignment, punch-in preservation); Ableton loads as MIDI effect**
- **[NEW] SdlContext.h/cpp: process-level SDL2 singleton with atomic ref-count (SDL_Init once per process, SDL_Quit on last release) — eliminates multi-instance DAW crash (06-01 / 0779877)**
- **[NEW] GamepadInput refactored: SdlContext::acquire/release, deadZone_ atomic + setDeadZone(), sample-and-hold on both sticks, ButtonState 20ms debounce on all 8 buttons, dual onConnectionChange/onConnectionChangeUI slots (06-01 / cb35c78)**
- **[NEW] PluginProcessor.h: gamepadActive_ atomic bool (per-instance, default true), setGamepadActive/isGamepadActive API, prevCcCut_/prevCcRes_ atomic<int> CC dedup state, pendingAllNotesOff_/pendingCcReset_ atomic disconnect flags (06-02 / 9feb28b)**
- **[NEW] PluginProcessor.cpp: CC74/CC71 flood eliminated — gated on isConnected() AND gamepadActive_; atomic dedup prevents redundant CC on slow drift; disconnect sends all-notes-off (4 voices) + CC74=0/CC71=0 via pending flags; setDeadZone forwarded from joystickThreshold APVTS each block (06-02 / 2973135)**
- **[NEW] PluginEditor: [GAMEPAD ON/OFF] TextButton toggle; onConnectionChangeUI lambda; status label corrected to "GAMEPAD: connected"/"GAMEPAD: none"; initial isConnected() check on editor open (06-03 / beefa49)**
- **[NEW] CMakeLists.txt: VST3_CATEGORIES "Fx|MIDI", IS_MIDI_EFFECT=TRUE, NEEDS_MIDI_INPUT=TRUE — plugin loads as MIDI effect in Ableton 11 browser; moduleinfo.json confirms Sub Categories ["Fx","Fx","MIDI"] (06-03 / c1a1f19)**
- **[NEW] Always-editable scale keyboard: auto-copies preset into scaleNote0..11 + flips useCustomScale=true on first click; customScaleToggle_ kept invisible so APVTS param persists (06-03 / c1a1f19)**
- **[NEW] Looper auto-stop: recordedBeats_ accumulator fires finaliseRecording() when loop length reached; [REC TOUCH] button defers recording start until next note-on (recWaitForTrigger_/recWaitArmed_ atomics) (06-03 / c1a1f19)**
- **[NEW] 06-03 DAW verification COMPLETE: all 7 gamepad tests passed (connection indicator, right stick, voice buttons, no CC flood, CC reset, toggle, hot-plug)**
- **[NEW] 06-04 Task 1 COMPLETE (e6dd2a6): looper auto-stop (recordedBeats_ accumulator), REC TOUCH (recWaitForTrigger_/recWaitArmed_), always-editable scale keyboard, CMakeLists Ableton fix — build Release green, VST3 installed**
- **[NEW] 06-04 fix (f6e80b1): globalTranspose 0..11 (key selector); JOY gate model changed to position-based close (stillnessCounter_ ≥ sampleRate ≈1s near center) — eliminates retrigger-on-hold bug**
- **[NEW] 06-04 fix (33ad08a): L3 (left stick click) = ALL touchplate via setPadState() held model for PAD-mode voices; globalTranspose text box shows note names (C/C#·Db/D/.../B) via AudioParameterIntAttributes**
- **[NEW] 06-04 DAW CHECKPOINT APPROVED: all 8 tests passed — UI layout, scale keyboard red, REC TOUCH, looper auto-stop, gamepad remap (L1=Root/L2=Third/R1=Fifth/R2=Tension), filter CC DAW visibility, JOY gate hold, JOY pitch CV (pitch bend ±24st)**
- **[NEW] Phase 06 COMPLETE: SDL-01..SDL-05 all satisfied; process-level SDL singleton, CC gating, disconnect all-notes-off, GAMEPAD toggle, full gamepad DAW verification****
- **[NEW] 07-01 COMPLETE (ae0becc, 889356e): pluginval strictness level 5 PASSED — all 19 test suites green; BusesProperties disabled stereo buses fix committed; pre-flight audit confirmed processBlock zero-sample safe, ScaleKeyboard listener balance 15/15**
- **[NEW] 07-02 COMPLETE (257f1f8): Inno Setup 6.7.1 script authored; DimaChordJoystick-Setup.exe (3.5 MB) compiled; dev-machine smoke test PASSED (installs to C:\Program Files\Common Files\VST3\, Ableton MIDI Effects confirmed, uninstall/reinstall clean); clean-machine test APPROVED — artifact ready for Gumroad upload**
- **[NEW] Phase 07 COMPLETE: DIST-01..DIST-04 all satisfied; pluginval level-5 pass + Inno Setup installer + clean-machine validation — v1.0 release candidate ready**

## Key Decisions

| Decision | Outcome |
|----------|---------|
| JUCE 8.0.4 + CMake FetchContent | LOCKED — pinned to tag 8.0.4 (commit 51d11a2), GIT_SHALLOW TRUE |
| Generator: Visual Studio 18 2026 | VS 17/16 not installed on this machine; VS 18 2026 Community used |
| SDL2 2.30.9 static | Locked — already correctly configured |
| VST3 only (Windows) | Locked — v1 scope |
| 4 voices on MIDI channels 1-4 | Default routing, user-configurable per voice |
| Sample-and-hold pitch model | Pitch locked at trigger moment, not continuous |
| Quantize ties → down | Deterministic behavior at equidistance |
| Lock-free LooperEngine | Required before DAW testing — current mutex impl is unsafe |
| JUCE 8.0.4 BusesLayout API | getMainInputChannels()/getMainOutputChannels() — NOT the Count() variants |
| Catch2 placed after SDL2 MakeAvailable | Ensures static CRT setting already applied; prevents LNK2038 |
| Test target compiles .cpp directly | Not linking ChordJoystick plugin target — avoids GUI/DAW deps in headless tests |
| ASCII hyphens in Catch2 test names | Windows ctest garbles Unicode em-dash in filter arguments |
| ScalePreset::Chromatic in baseParams() | Chromatic is 12-note pass-through — lets axis/transpose/octave tests get integer-exact values without quantization snap masking arithmetic bugs |
| resetAllGates() as single cleanup primitive | Shared between releaseResources() and processBlockBypassed() — avoids duplicated atomic clearing logic |
| noteOff explicit (uint8_t)0 velocity | 3-arg form avoids host interpretation ambiguity vs 2-arg form |
| Gate LED uses Clr::gateOn (green) | Corrects semantic mismatch — green universally understood as open/active |
| channelConflictShown_ cache | Prevents setVisible() being called every 30 Hz tick when state is stable |
| Reaper as DAW verification target | Ableton crashes on instantiation (deferred); Reaper provides full MIDI monitoring without the crash |
| 6-test structured DAW protocol | Discrete test cases: basic output, LED color, retrigger, bypass flush, transport, conflict UI — all passed 03-02 |
| Chebyshev distance for joystick magnitude | max(abs(dx), abs(dy)) avoids diagonal bias vs Manhattan distance |
| JOY gate holds indefinitely | No auto-close on stillness; closes only via resetAllGates() or voice mode switch |
| JOY retrigger as final model | noteOff(old)+noteOn(new) on scale-degree change; universal synth compatibility; no RPN/pitchBend infrastructure required |
| joyActivePitch_[v] tracks sounding pitch | -1=gate closed; updated on every retrigger; used for comparison and gate-close note-off |
| Pitch bend model superseded | joyBasePitch_/joyLastBendValue_/sendBendRangeRPN/MidiCallback all removed; simpler and more reliable |
| TouchPlate dimming reads APVTS in paint() | getRawParameterValue()->load() is safe from message thread (atomic<float>*) |
| PPQ subdivision index (int64_t) for DAW sync | Integer comparison avoids float equality issues; fires exactly once per boundary crossing |
| randomDensity range 1..8 hits-per-bar | More intuitive than 0..1 probability; hitsPerBarToProbability() converts to per-event probability |
| randomGateTime × samplesPerSubdiv with 10ms floor | Prevents inaudible staccato at extreme settings; block-granular countdown is sufficient for musical timing |
| wasPlaying_ edge detection for transport restart | prevSubdivIndex_[4] reset to -1 on play start; prevents spurious fire on the subdivision already seen before stop |
| randomClockSync sync/free mode | sync=true fires only when isDawPlaying (DAW stop = silence from RND voices); sync=false uses internal BPM at randomFreeTempo — wall-clock fallback from plan spec replaced with explicit user-controlled toggle |
| randomFreeTempo param (30-240 BPM, default 120) | BPM for free-running random clock; only active when randomClockSync=false |
| 4-column DENS/GATE/FREE BPM/SYNC row | randomDensityKnob_ changed to NoTextBox rotary to match Gate/FreeTempo knobs — visual consistency in random controls section |
| AbstractFifo double-buffer over snapshot-scan-reinsert | eventBuf_+fifo_ for write side during recording; playbackStore_ for stable read side during playback — no lock needed |
| ScopedRead must destruct before fifo_.reset() | JUCE AbstractFifo: reset() sets validEnd=0 then validStart=0; ScopedRead dtor calls finishedRead advancing validStart; if reset() runs first dtor leaves validStart=numRead with validEnd=0, getNumReady()=bufferSize-numRead=2047 |
| Scratch buffers as class members | scratchNew_/scratchMerged_ (each ~49KB) moved from local arrays to class members — prevents MSVC debug-mode stack overflow in finaliseRecording() |
| Punch-in touch radius = loopLen/64 | Beat window for replacing old events; wrap-around distance via min(dist, loopLen-dist); events outside all windows are preserved |
| JUCE 8 getPosition() Optional API | getPosition() returns Optional<PositionInfo>; each field also Optional — use .hasValue() before dereferencing ppqPosition/bpm; getIsPlaying() is bool (not Optional) |
| Looper gate playback bypasses TriggerSystem | loopOut.gateOn/gateOff emit midi.addEvent directly using heldPitch_[v]; TriggerSystem not informed of looper events so live pad input coexists independently |
| New looper buttons use direct atomic setters | loopRecJoyBtn_/loopRecGatesBtn_/loopSyncBtn_ write to LooperEngine atomics directly (not APVTS); state mirrored in 30 Hz timerCallback |
| isMidiEffect()=true for Ableton | Plugin categorised as MIDI effect; goes on MIDI track before instrument; empty BusesProperties(); isBusesLayoutSupported accepts 0-channel only |
| Per-voice slew via CC5+CC65 | Portamento on (CC65=127) + portamento time (CC5=slewValue) sent on each voice channel before every note-on; synth-universal |
| recordPending_ atomic for deferred record start | REC press arms pending flag; process() starts recording on next valid clock pulse — immediate if free-running, waits for DAW if SYNC mode |
| prevDawPlaying_ edge detection for DAW stop | When DAW stops in SYNC mode, dawStopped flag triggers all-notes-off and looper stop |
| SdlContext as file-scope static atomics | gRefCount + gAvailable as file-scope statics in SdlContext.cpp — no Meyers singleton, no JUCE header dependency in SDL-only translation unit |
| setDeadZone() uses memory_order_relaxed | Dead zone read once per 60Hz timer tick; full sequential consistency unnecessary overhead for this use case |
| Dual onConnectionChange / onConnectionChangeUI | Two independent callback slots prevent PluginEditor assignment from silently stomping PluginProcessor disconnect handler |
| DBG() not available in SdlContext.cpp | SdlContext.cpp is SDL-only (no JUCE headers); SDL_Init failure logged via (void)SDL_GetError() — readable in debugger without JUCE macro dependency |
| pendingAllNotesOff_/pendingCcReset_ use release/acq_rel ordering | store() on message thread with memory_order_release; exchange() on audio thread with memory_order_acq_rel — establishes happens-before for MIDI data |
| gamepadActive_ defaults to true | Existing instances behave as before (gamepad active) until PluginEditor toggle is added in 06-03 |
| prevCcCut_/prevCcRes_ reset to -1 on disconnect | Forces fresh CC emission on next connect instead of suppressing a valid zero value |
| setDeadZone called unconditionally each block | getRawParameterValue atomic load is cheap; avoids parameter listener complexity |
| JOY gate: position-based close (stillnessCounter_) | Gate holds while abs(axis) > threshold; closes only after ≥1s near center — eliminates retrigger when holding joystick steady at a pitch |
| globalTranspose 0..11 (key selector) | One-octave range is correct for key selection; per-voice octave offsets handle pitch range; narrowed from ±24 |
| Note-name display on transpose via AudioParameterIntAttributes | valueToText lambda with kNoteNames[] — JUCE 8.0.4 compatible, no deprecation warnings |
| L3 = setPadState() held model | Matches on-screen ALL touchplate; triggerAllNotes() momentary approach removed |
| filterCutLive_/filterResLive_ APVTS params at 30 Hz | setValueNotifyingHost from timerCallback bridges CC values to DAW automation lane without audio-thread APVTS write |
| RPN 0 on gate-open, bend = (pitch − gatePitch) × 2730 | JOY pitch CV via standard pitch bend; 0x2000 reset on gate-close; no retrigger |
| pluginval against build artifact, not system VST3 | System VST3 requires admin elevation; build artifact at build/ChordJoystick_artefacts/Release/VST3/ is fully equivalent for validation |
| BusesProperties with disabled stereo buses | withInput/withOutput(false) satisfies pluginval bus enumeration tests; isBusesLayoutSupported accepts (0,0) and (2,2); isMidiEffect()=true keeps Ableton MIDI-track placement |
| Installer EXE not in git | Binary artifact at installer/Output/DimaChordJoystick-Setup.exe distributed via Gumroad; .iss script committed so EXE is reproducible from source |
| Code signing deferred to v2 | SmartScreen "More info / Run anyway" acceptable for v1; unsigned EXE validated on clean machine; placeholder comment in .iss for future cert |
| DisableDirPage=yes in installer | VST3 path standardised at {commoncf64}\VST3\; no user path browsing prevents incorrect placement |
| Static CRT confirmed via clean-machine test | CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" validated — plugin installs and loads without Visual C++ Redistributable on clean Windows machine |

## Known Issues (Must Fix Before Shipping)

1. ~~**JUCE pinned to `origin/master`**~~ — FIXED in 01-01 (now 8.0.4).
2. ~~**[BLOCKER] Plugin crashes on load in Ableton Live 11**~~ — FIXED in 05-03 (isMidiEffect()=true, empty BusesProperties). Plugin loads as MIDI effect on MIDI tracks.
3. ~~**std::mutex in LooperEngine processBlock**~~ — FIXED in 05-01 (now fully lock-free AbstractFifo design).
4. ~~**Filter CC (CC71/CC74) emitted unconditionally**~~ — FIXED in 06-02 (gated on isConnected() && gamepadActive_, atomic dedup, disconnect reset).
5. ~~**releaseResources() is empty**~~ — FIXED in 03-01 (now calls resetAllGates()).
6. ~~**SDL_Init/SDL_Quit per instance**~~ — FIXED in 06-01 (SdlContext singleton, atomic ref-count, SDL_Init once per process).
7. ~~**Ableton MIDI effect category wrong**~~ — FIXED in 06-03 (VST3_CATEGORIES "Fx|MIDI", IS_MIDI_EFFECT=TRUE; loads correctly in MIDI Effects browser).
8. **COPY_PLUGIN_AFTER_BUILD requires elevated process** — manual copy needed each rebuild.

## Pending Todos

(none)

## Blockers / Concerns

- Ableton placement: must go on MIDI track *before* an instrument (not as instrument itself) — MIDI effect category

## Session Continuity

Last session: 2026-02-23
Stopped at: 07-02-PLAN.md complete — Phase 07 fully done. v1.0 release candidate ready for Gumroad upload.
Resume file: none — all 7 phases complete
