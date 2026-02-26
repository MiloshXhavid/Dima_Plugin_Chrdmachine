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
    bool isMidiEffect()  const override { return true;  }
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
    // Default: X = center (0.0), Y = bottom (-1.0 = lowest pitch)
    std::atomic<float> joystickX {0.0f};
    std::atomic<float> joystickY {-1.0f};

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

    void looperSetRecWaitForTrigger(bool b) { looper_.setRecWaitForTrigger(b); }
    bool looperIsRecWaitForTrigger()  const { return looper_.isRecWaitForTrigger(); }
    bool looperIsRecWaitArmed()         const { return looper_.isRecWaitArmed(); }
    bool looperIsRecPendingNextCycle()  const { return looper_.isRecPendingNextCycle(); }
    // True when armed but not yet recording (blink state for the REC button).
    bool looperIsRecPending()           const { return looper_.isRecordPending() || looper_.isRecPendingNextCycle(); }
    void looperArmWait()                      { looper_.armWait(); }

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

    // Arp current voice for pad highlight (audio thread writes, UI reads)
    // -1 = no arp note sounding; 0-3 = voice currently being arped
    std::atomic<int> arpCurrentVoice_ { -1 };
    int getArpCurrentVoice() const { return arpCurrentVoice_.load(std::memory_order_relaxed); }

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

    double sampleRate_ = 44100.0;
    std::atomic<float> effectiveBpm_ { 120.0f };

    // Previous joystick position — used to compute per-block deltas for JOY gate detection.
    // Initialised to match joystickX/joystickY defaults so the first delta is 0.
    float prevJoyX_ = 0.0f;
    float prevJoyY_ = -1.0f;

    // ── Gamepad per-instance active flag ─────────────────────────────────────
    // Written by PluginEditor toggle button (message thread).
    // Read in processBlock (audio thread). No mutex — atomic only.
    std::atomic<bool> gamepadActive_    { true  };
    std::atomic<bool> filterModActive_  { true };   // left-joystick filter mod on/off (default ON)
    std::atomic<bool> midiMuted_        { false };  // true = block all MIDI output

    bool gamepadVoiceWasHeld_[4] = {};  // audio thread only — tracks previous gamepad held state
    bool allNotesWasHeld_ = false;       // audio thread only — tracks previous L3 held state
    bool prevIsDawPlaying_ = false;      // audio thread only — for DAW stop detection
    bool prevLooperWasPlaying_ = false;  // audio thread only — for looper stop detection
    int   prevXMode_    = -1;            // audio thread only — dedup reset on X mode change
    int   prevYMode_    = -1;            // audio thread only — dedup reset on Y mode change
    float prevFilterX_  = -99.0f;       // audio thread only — raw joystick X, -99 = uninitialised
    float prevFilterY_  = -99.0f;       // audio thread only — raw joystick Y, -99 = uninitialised
    float prevBaseX_    = -1.0f;        // last xOffset used in a CC send; -1 = first run
    float prevBaseY_    = -1.0f;

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

    // Held (sample-and-hold) pitches for each voice
    std::array<int, 4> heldPitch_ {60, 64, 67, 70};

    // Pitch that was actually sent in the last looper gate-on per voice.
    // gateOff MUST use this pitch (not heldPitch_, which may have changed) to
    // send the matching noteOff and avoid stuck notes.
    std::array<int, 4> looperActivePitch_ {-1, -1, -1, -1};

    // ── Arpeggiator state (audio-thread-only) ─────────────────────────────────
    double   arpPhase_            = 0.0;      // accumulated beats within subdivision
    double   arpNoteOffRemaining_ = 0.0;      // beats until gate-time note-off fires
    int      arpStep_             = 0;        // current position in step sequence
    int      arpActivePitch_      = -1;       // pitch of currently sounding arp note
    int      arpActiveVoice_      = -1;       // voice of currently sounding arp note
    uint32_t arpRandSeed_         = 1;        // LCG seed for random order
    int      arpRandomOrder_[4]   = {0,1,2,3};// cached random sequence, shuffled each cycle
    bool     arpWaitingForPlay_   = false;    // armed but waiting for DAW play press to launch
    bool     prevArpOn_           = false;    // previous block's arp-enabled state

    // Build ChordEngine::Params from current APVTS + joystick state
    ChordEngine::Params buildChordParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
