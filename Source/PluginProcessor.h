#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ChordEngine.h"
#include "TriggerSystem.h"
#include "LooperEngine.h"
#include "GamepadInput.h"
#include "LfoEngine.h"
#include <atomic>
#include <array>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    // ── AudioProcessor overrides ──────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ChordJoystick"; }
    bool acceptsMidi()   const override { return true;  }
    bool producesMidi()  const override { return true;  }
    bool isMidiEffect()  const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms() override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ── Public state (read by UI, written by UI / gamepad) ────────────────────
    // Live joystick position from UI drag (-1..+1)
    // Default: center (0, 0)
    std::atomic<float> joystickX {0.0f};
    std::atomic<float> joystickY {0.0f};
    // Set by JoystickPad mouseDown/Drag; gives mouse priority over a drifting physical stick.
    // Cleared by buildChordParams() when the physical stick crosses the deliberate-intent threshold.
    mutable std::atomic<bool>  mouseJoyActive_ { false };

    // Touch-plate state (UI → processor)
    void setPadState(int voice, bool pressed) { trigger_.setPadState(voice, pressed); }
    bool isGateOpen(int voice) const          { return trigger_.isGateOpen(voice); }

    // Per-voice hold: when true, mouseUp on the pad does not send note-off
    std::atomic<bool> padHold_[4] {};

    // Looper transport (UI / gamepad)
    void looperStartStop() { looper_.startStop(); }
    void looperRecord()    { looper_.record();    }
    void looperReset()     { looper_.reset();     }
    void looperDelete()    { looper_.deleteLoop(); }
    bool looperIsPlaying()     const { return looper_.isPlaying();     }
    bool looperIsRecording()   const { return looper_.isRecording();   }
    bool looperIsRecordArmed() const { return looper_.isRecordArmed(); }

    // New looper API (Phase 05)
    void looperSetRecJoy(bool b)     { looper_.setRecJoy(b);     }
    void looperSetRecGates(bool b)   { looper_.setRecGates(b);   }
    void looperSetRecFilter(bool b)  { looper_.setRecFilter(b);  }
    void looperSetSyncToDaw(bool b)  { looper_.setSyncToDaw(b);  }
    bool looperIsCapReached()  const { return looper_.isCapReached(); }

    // Looper playback position for UI progress bar (read by message thread — lock-free float atomic)
    double getLooperPlaybackBeat()  const { return looper_.getPlaybackBeat();    }
    double getLooperLengthBeats()   const { return looper_.getLoopLengthBeats(); }
    bool looperIsSyncToDaw()   const { return looper_.isSyncToDaw();  }
    bool looperIsRecJoy()      const { return looper_.isRecJoy();     }
    bool looperIsRecGates()    const { return looper_.isRecGates();   }
    bool looperIsRecFilter()   const { return looper_.isRecFilter();  }

    // DAW playing state — written by audio thread each processBlock, read by UI
    bool isDawPlaying() const { return dawPlaying_.load(std::memory_order_relaxed); }

    // Joystick additive offset for looper playback (sticky mouse offset + non-sticky stick offset)
    // Written by JoystickPad (message thread) on mouseDown/Drag; read by processBlock.
    std::atomic<float> looperJoyOffsetX_ { 0.0f };
    std::atomic<float> looperJoyOffsetY_ { 0.0f };

    // Looper content query passthroughs (read by UI timerCallback and onClick handlers)
    bool looperHasJoystickContent() const { return looper_.hasJoystickContent(); }
    bool looperHasGateContent()     const { return looper_.hasGateContent();     }
    bool looperHasFilterContent()   const { return looper_.hasFilterContent();   }

    // Lane-clear API (called from UI onClick handlers)
    void looperClearGateLane()   { looper_.clearGateLane();   }
    void looperClearJoyLane()    { looper_.clearJoyLane();    }
    void looperClearFilterLane() { looper_.clearFilterLane(); }

    void looperSetRecWaitForTrigger(bool b) { looper_.setRecWaitForTrigger(b); }
    bool looperIsRecWaitForTrigger()  const { return looper_.isRecWaitForTrigger(); }
    bool looperIsRecWaitArmed()         const { return looper_.isRecWaitArmed(); }
    bool looperIsRecPendingNextCycle()  const { return looper_.isRecPendingNextCycle(); }
    // True when armed but not yet recording (blink state for the REC button).
    bool looperIsRecPending()           const { return looper_.isRecordPending() || looper_.isRecPendingNextCycle(); }
    void looperArmWait()                      { looper_.armWait(); }

    // ── LFO Recording (Phase 22) ──────────────────────────────────────────────
    // Message-thread callers: PluginEditor ARM / CLR button onClick handlers.
    void armLfoX()             { lfoX_.arm();            }
    void armLfoY()             { lfoY_.arm();            }
    void clearLfoXRecording()  { lfoX_.clearRecording(); }
    void clearLfoYRecording()  { lfoY_.clearRecording(); }
    void resetLfoXPhase()      { lfoX_.requestPhaseReset(); }
    void resetLfoYPhase()      { lfoY_.requestPhaseReset(); }
    LfoRecState getLfoXRecState() const { return lfoX_.getRecState(); }
    LfoRecState getLfoYRecState() const { return lfoY_.getRecState(); }
    // Instant LFO capture: start Armed LFOs recording immediately (looper keeps playing).
    void triggerLfoInstantCapture() { lfoInstantCapture_.store(true, std::memory_order_relaxed); }

    // Quantize mode (0=Off 1=Live 2=Post) + subdivision (0=1/4 1=1/8 2=1/16 3=1/32)
    // Called from PluginEditor onClick handlers and timerCallback.
    void setQuantizeMode(int mode)
    {
        if (auto* p = apvts.getParameter("quantizeMode"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(mode)));
        looper_.setQuantizeMode(mode);
    }
    void setQuantizeSubdiv(int subdiv)
    {
        if (auto* p = apvts.getParameter("quantizeSubdiv"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(subdiv)));
        looper_.setQuantizeSubdiv(subdiv);
    }
    int getQuantizeMode() const
    {
        auto* v = apvts.getRawParameterValue("quantizeMode");
        return v ? static_cast<int>(*v) : 0;
    }
    int getQuantizeSubdiv() const
    {
        auto* v = apvts.getRawParameterValue("quantizeSubdiv");
        return v ? static_cast<int>(*v) : 1;
    }
    // Post-record quantize trigger / revert — wired to LooperEngine
    void looperApplyQuantize()  { looper_.applyQuantize();  }
    void looperRevertQuantize() { looper_.revertQuantize(); }
    bool looperQuantizeIsActive() const { return looper_.isQuantizeActive(); }

    // Arp armed-but-waiting state — read by UI for blink
    bool isArpWaitingForPlay() const { return arpWaitingForPlay_; }

    // Effective BPM (DAW BPM when synced, free tempo otherwise) — read by UI
    float getEffectiveBpm() const { return effectiveBpm_.load(std::memory_order_relaxed); }

    // Gamepad
    GamepadInput& getGamepad() { return gamepad_; }

    // Gamepad per-instance toggle (PluginEditor button)
    void setGamepadActive(bool b) { gamepadActive_.store(b, std::memory_order_relaxed); }
    bool isGamepadActive()  const { return gamepadActive_.load(std::memory_order_relaxed); }

    // Left-joystick filter mod on/off (PluginEditor button, default OFF)
    void setFilterModActive(bool b) { filterModActive_.store(b, std::memory_order_relaxed); }
    bool isFilterModActive()  const { return filterModActive_.load(std::memory_order_relaxed); }

    // MIDI mute — when true, all MIDI output is blocked after allNotesOff fires on activation
    void setMidiMuted(bool b) { midiMuted_.store(b, std::memory_order_relaxed); }
    bool isMidiMuted()  const { return midiMuted_.load(std::memory_order_relaxed); }

    // Pitches for chord-name display: snapshotted on TouchPlate/Random triggers only.
    std::array<int,4> getCurrentPitches() const { return displayPitches_; }
    uint8_t getCurrentVoiceMask() const { return displayVoiceMask_; }

    // Arp current voice for pad highlight (audio thread writes, UI reads)
    // -1 = no arp note sounding; 0-3 = voice currently being arped
    std::atomic<int> arpCurrentVoice_ { -1 };
    int getArpCurrentVoice() const { return arpCurrentVoice_.load(std::memory_order_relaxed); }
    std::atomic<int> arpCurrentStep_  { 0 };   // audio thread writes; timer reads for grid highlight

    // Looper voice active: true while the looper has a gate-on note sounding for this voice.
    // Read by UI for pad highlight (best-effort, display-only).
    bool isLooperVoiceActive(int v) const noexcept { return looperActivePitch_[v] >= 0; }

    // D-pad BPM delta: set from processBlock (audio thread), consumed on message thread
    std::atomic<int> pendingBpmDelta_ { 0 };

    // Gamepad button flash signals: incremented on audio thread, consumed by UI timer
    std::atomic<int> flashLoopReset_  { 0 };
    std::atomic<int> flashLoopDelete_ { 0 };
    std::atomic<int> flashPanic_      { 0 };

    // MIDI panic — sends all-notes-off on all voice channels (UI button)
    void triggerPanic()
    {
        midiMuted_.store(false, std::memory_order_relaxed);
        pendingPanic_.store(true, std::memory_order_relaxed);
    }

    // Live filter CC values — written by audio thread, read by message thread (timerCallback)
    // to call setValueNotifyingHost on "filterCutLive" / "filterResLive" APVTS params.
    std::atomic<float> filterCutDisplay_ { 0.f };
    std::atomic<float> filterResDisplay_ { 0.f };

    // Beat-clock cross-thread signals — audio thread writes, UI timer reads
    std::atomic<bool>  beatOccurred_  { false };
    std::atomic<float> modulatedJoyX_ { 0.0f  };
    std::atomic<float> modulatedJoyY_ { 0.0f  };

    // Subdivision multiplier for LFO sync mode when stick targets LFO Freq (mode index 2).
    // Written in filter dispatch block; read in LFO ProcessParams block.
    // 1.0f = no modification (default). Range: 0.25–4.0.
    std::atomic<float> lfoXSubdivMult_ { 1.0f };
    std::atomic<float> lfoYSubdivMult_ { 1.0f };

    // LFO/Gate joystick display values — written by audio thread (dispatch block),
    // read by message thread (timerCallback) to update slider visuals via setValue(dontSendNotification).
    // These hold base + joystick offset. APVTS is never written by the dispatch block.
    // One-block latency (~6ms at 256/44.1k) — inaudible.
    std::atomic<float> lfoXRateDisplay_  { 0.01f };
    std::atomic<float> lfoXPhaseDisplay_ { 0.0f  };
    std::atomic<float> lfoXLevelDisplay_ { 0.0f  };
    std::atomic<float> lfoYRateDisplay_  { 0.01f };
    std::atomic<float> lfoYPhaseDisplay_ { 0.0f  };
    std::atomic<float> lfoYLevelDisplay_ { 0.0f  };
    std::atomic<float> lfoXOutputDisplay_{ 0.0f  };  // LFO X last output; > 0 = positive half-cycle
    std::atomic<float> lfoYOutputDisplay_{ 0.0f  };  // LFO Y last output; > 0 = positive half-cycle
    std::atomic<float> gateLengthDisplay_{ 0.5f  };

    // MOD FIX knob display — uses live stick (not S&H) for stability.
    // Holds xOffset + liveX * 63.5 * atten, clamped to -50..+50.
    std::atomic<float> filterXOffsetDisplay_ { 0.0f };
    std::atomic<float> filterYOffsetDisplay_ { 0.0f };

    // Raw live left-stick values for UI colour feedback (LFO fader red/blue tint).
    std::atomic<float> leftStickXDisplay_    { 0.0f };
    std::atomic<float> leftStickYDisplay_    { 0.0f };

    // Note-on flash per voice — incremented on note-on, consumed by JoystickPad timerCallback.
    std::atomic<int>   voiceTriggerFlash_[4] {};

    // Live pitch for crosshair visualization — written by audio thread after LFO application,
    // read by JoystickPad::timerCallback() at 30 Hz. One-block latency (~6ms) — inaudible for display.
    std::atomic<int> livePitch0_ { 60 };
    std::atomic<int> livePitch1_ { 64 };
    std::atomic<int> livePitch2_ { 67 };
    std::atomic<int> livePitch3_ { 70 };

    // ── APVTS ─────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    ChordEngine   chord_;
    TriggerSystem trigger_;
    LooperEngine  looper_;
    GamepadInput  gamepad_;
    LfoEngine     lfoX_;          // X-axis LFO (modulates fifth/tension voices)
    LfoEngine     lfoY_;          // Y-axis LFO (modulates root/third voices)

    // LFO disable ramp state (audio-thread only)
    float lfoXRampOut_ = 0.0f;   // current ramped LFO X contribution
    float lfoYRampOut_ = 0.0f;   // current ramped LFO Y contribution
    std::atomic<bool> lfoInstantCapture_ { false }; // set by UI, consumed in processBlock
    int lfoInstantCaptureSamplesLeft_ = 0;          // audio-thread countdown for Bug-2 one-cycle stop
    bool prevDawPlaying_ = false;                   // audio-thread edge detect for transport stop (Bug-3)
    std::atomic<bool> dawPlaying_ { false };         // audio thread writes, UI reads

    // LFO direct CC dedup — audio-thread only. -1 = force-send on next block.
    int prevLfoCcX_ = -1;
    int prevLfoCcY_ = -1;

    double sampleRate_ = 44100.0;
    std::atomic<float> effectiveBpm_ { 120.0f };

    // Previous joystick position — used to compute per-block deltas for JOY gate detection.
    // Initialised to match joystickX/joystickY defaults so the first delta is 0.
    float prevJoyX_ = 0.0f;
    float prevJoyY_ = -1.0f;

    // Free-tempo sample counter: incremented every processBlock, reset on transport events.
    // Used for beat detection when DAW is not playing.
    int64_t sampleCounter_ = 0;

    // Beat detection state — previous beat count for floor-crossing detection.
    // Promoted from static local so it can be reset on transport stop/start events.
    double prevBeatCount_ = -1.0;

    // ── Gamepad per-instance active flag ─────────────────────────────────────
    // Written by PluginEditor toggle button (message thread).
    // Read in processBlock (audio thread). No mutex — atomic only.
    std::atomic<bool> gamepadActive_    { true  };
    std::atomic<bool> filterModActive_  { true };   // left-joystick filter mod on/off (default ON)
    std::atomic<bool> midiMuted_        { false };  // true = block all MIDI output
    std::atomic<bool> stickSwap_        { false };  // swap left ↔ right stick routing
    std::atomic<bool> stickInvert_      { false };  // invert both stick axes

    bool gamepadVoiceWasHeld_[4] = {};  // audio thread only — tracks previous gamepad held state
    bool allNotesWasHeld_ = false;       // audio thread only — tracks previous L3 held state
    bool prevIsDawPlaying_ = false;      // audio thread only — for DAW stop detection
    bool prevLooperWasPlaying_ = false;  // audio thread only — for looper stop detection
    bool prevLooperRecording_  = false;  // audio thread only — LFO rec edge detection
    bool lfoXMidCycleRec_ = false;       // audio thread only — LFO X started mid-cycle (looper REC press), skip first looperCycled stop
    bool lfoYMidCycleRec_ = false;       // audio thread only — LFO Y started mid-cycle (looper REC press), skip first looperCycled stop
    int   prevXMode_    = -1;            // audio thread only — dedup reset on X mode change
    int   prevYMode_    = -1;            // audio thread only — dedup reset on Y mode change
    float prevFilterX_  = -99.0f;       // audio thread only — raw joystick X, -99 = uninitialised
    float prevFilterY_  = -99.0f;       // audio thread only — raw joystick Y, -99 = uninitialised
    float prevBaseX_    = -1.0f;        // last xOffset used in a CC send; -1 = first run
    float prevBaseY_    = -1.0f;
    float prevAttenX_   = -1.0f;        // last xAtten used — change triggers CC resend
    float prevAttenY_   = -1.0f;

    // LFO override floats — audio-thread only (no synchronization needed).
    // Set in the dispatch block each block when joystick mode matches.
    // Reset to -1.0f unconditionally at top of dispatch block.
    // Consumed by LFO params block (next call) using >= 0.0f sentinel.
    // -1.0f = inactive: LFO params block reads APVTS directly.
    float lfoXRateOverride_  = -1.0f;
    float lfoXPhaseOverride_ = -1.0f;
    float lfoXLevelOverride_ = -1.0f;
    float lfoYRateOverride_  = -1.0f;
    float lfoYPhaseOverride_ = -1.0f;
    float lfoYLevelOverride_ = -1.0f;

    // Gate length smoother — eliminates zipper noise when joystick modulates gate %.
    // Prepared in prepareToPlay(); target set each processBlock from dispatch atomic or APVTS.
    // juce_dsp excluded per requirements; SmoothedValue is in juce_core.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGateLength_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCcCut_;  // Bug 5: 30ms ramp on filter CC X
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCcRes_;  // Bug 5: 30ms ramp on filter CC Y

    // ── CC dedup: last emitted integer values for CC74 and CC71 ──────────────
    // -2 = on-load sentinel: first filter activation initialises tracking silently
    //      (base knob not yet touched — don't override the synth's own value).
    // -1 = force-send: used after panic / mode-switch / gamepad reconnect.
    // ≥0 = normal dedup (last sent value).
    // Declared atomic<int> to avoid data race: reset from message thread
    // (onConnectionChange lambda), read/written on audio thread (processBlock).
    std::atomic<int> prevCcCut_ { -2 };
    std::atomic<int> prevCcRes_ { -2 };

    // ── Disconnect pending flags ──────────────────────────────────────────────
    // Set from message thread (onConnectionChange lambda).
    // Consumed (exchange false) at top of processBlock filter CC section.
    // Use atomic so audio thread and message thread share correctly without mutex.
    std::atomic<bool> pendingAllNotesOff_ { false };
    std::atomic<bool> pendingCcReset_     { false };
    std::atomic<bool> pendingPanic_       { false };

    // Held (sample-and-hold) pitches for each voice — updated every processBlock.
    std::array<int, 4> heldPitch_ {60, 64, 67, 70};

    // Display pitches — snapshotted on trigger (pad/random/gamepad button) OR when
    // any chord-shaping parameter changes (transpose, intervals, octaves, scale).
    // Read by the UI timer for the chord-name label.
    std::array<int, 4> displayPitches_ {60, 64, 67, 70};

    // Voice mask snapshotted alongside displayPitches_.
    // Bit v = 1 means voice v produced a real note-on in the trigger event.
    // Default 0xF = all voices real (used for param-change and arp snapshots).
    uint8_t displayVoiceMask_ {0xF};

    // Accumulates voice note-ons within a processBlock; written to displayVoiceMask_ at
    // the trigger snapshot site (Site 2). Reset to 0 at top of processBlock.
    uint8_t pendingVoiceMask_ = 0;

    // Hash of chord-shaping APVTS params — changes here update the display even
    // without a trigger.  Joystick & LFO are intentionally excluded.
    int64_t lastChordParamHash_ = -1;

    // Pitch that was actually sent in the last looper gate-on per voice.
    // gateOff MUST use this pitch (not heldPitch_, which may have changed) to
    // send the matching noteOff and avoid stuck notes.
    std::array<int, 4> looperActivePitch_ {-1, -1, -1, -1};

    // ── Sub Octave (Phase 19) ─────────────────────────────────────────────────
    // Sub pitch snapshotted at live gate-open (tp.onNote isOn branch).
    int subHeldPitch_[4]         = { -1, -1, -1, -1 };
    // Sub pitch snapshotted at looper gate-on (live SUB8 param at emission time).
    int looperActiveSubPitch_[4] = { -1, -1, -1, -1 };
    // True while sub note is actively on — used for mid-note toggle detection.
    std::atomic<bool> subOctSounding_[4] = {};

    // ── Single-Channel Routing state (audio-thread-only) ─────────────────────
    // Reference counter: noteCount_[ch-1][pitch] = number of voices holding
    // a note-on on that (channel, pitch) pair. noteOn emits only when count
    // goes from 0→1; noteOff emits only when count goes to 0.
    int noteCount_[16][128] = {};

    // Channel used at note-on time per voice — used for matching note-off even
    // if the channel setting changes mid-hold.
    int sentChannel_[4]     = {1, 2, 3, 4};

    // Channel used at looper gate-on time per voice — used for looper gate-off.
    int looperActiveCh_[4]  = {1, 2, 3, 4};

    // Channel used at arp note-on time — used for all arp note-offs.
    int arpActiveCh_        = 1;

    // Previous-block values for mode/target change detection (flush trigger).
    int prevSingleChanMode_   = 0;
    int prevSingleChanTarget_ = 1;

    // ── Arpeggiator state (audio-thread-only) ─────────────────────────────────
    double    arpPhase_           = 0.0;      // accumulated beats within subdivision
    long long arpPrevSubdivIdx_   = -1;       // last seen subdiv index; -1 fires step immediately on first block
    double   arpNoteOffRemaining_ = 0.0;      // beats until gate-time note-off fires
    int      arpStep_             = 0;        // current position in step sequence
    int      arpActivePitch_      = -1;       // pitch of currently sounding arp note
    int      arpActiveVoice_      = -1;       // voice of currently sounding arp note
    uint32_t arpRandSeed_         = 1;        // LCG seed for random order
    int      arpRandomOrder_[4]   = {0,1,2,3};// cached random sequence, shuffled each cycle
    bool     arpWaitingForPlay_   = false;    // armed but waiting for DAW play press to launch
    bool     prevArpOn_           = false;    // previous block's arp-enabled state

    // ── Option-mode D-pad parameter control ───────────────────────────────────
    // Mode 1 (octaves):            up=rootOct  down=3rdOct  left=tenOct  right=5thOct
    // Mode 2 (transpose+intervals): up=transpose down=3rdInt left=tenInt  right=5thInt
    // Step an int APVTS param by delta, wrapping within [minVal..maxVal].
    static void stepWrappingParam(juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& id, int minVal, int maxVal, int delta)
    {
        auto* p = apvts.getParameter(id);
        auto* v = apvts.getRawParameterValue(id);
        if (!p || !v) return;
        const int range = maxVal - minVal + 1;
        const int cur   = static_cast<int>(v->load()) - minVal;  // 0-based
        const int next  = ((cur + delta) % range + range) % range;
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(next + minVal)));
    }
    // Step an int APVTS param by delta, clamping at [minVal..maxVal] (no wrap).
    static void stepClampingParam(juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& id, int minVal, int maxVal, int delta)
    {
        auto* p = apvts.getParameter(id);
        auto* v = apvts.getRawParameterValue(id);
        if (!p || !v) return;
        const int cur  = static_cast<int>(v->load());
        const int next = juce::jlimit(minVal, maxVal, cur + delta);
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(next)));
    }

    // Reset all noteCount_ entries to zero — call at every allNotesOff flush site.
    void resetNoteCount() noexcept;

    // Build ChordEngine::Params from current APVTS + joystick state
    ChordEngine::Params buildChordParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
