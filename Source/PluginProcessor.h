#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ChordEngine.h"
#include "TriggerSystem.h"
#include "LooperEngine.h"
#include "GamepadInput.h"
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
    bool looperIsSyncToDaw()   const { return looper_.isSyncToDaw();  }
    bool looperIsRecJoy()      const { return looper_.isRecJoy();     }
    bool looperIsRecGates()    const { return looper_.isRecGates();   }
    bool looperIsRecFilter()   const { return looper_.isRecFilter();  }

    void looperSetRecWaitForTrigger(bool b) { looper_.setRecWaitForTrigger(b); }
    bool looperIsRecWaitForTrigger()  const { return looper_.isRecWaitForTrigger(); }
    bool looperIsRecWaitArmed()       const { return looper_.isRecWaitArmed(); }

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

    // D-pad BPM delta: set from processBlock (audio thread), consumed on message thread
    std::atomic<int> pendingBpmDelta_ { 0 };

    // Gamepad button flash signals: incremented on audio thread, consumed by UI timer
    std::atomic<int> flashLoopReset_  { 0 };
    std::atomic<int> flashLoopDelete_ { 0 };
    std::atomic<int> flashPanic_      { 0 };

    // MIDI panic — sends all-notes-off on all voice channels (UI button or R3 gamepad)
    // Always clears midiMuted_ so the UI panic button can recover from an R3-triggered mute.
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
    std::atomic<bool> filterModActive_  { false };  // left-joystick filter mod on/off
    std::atomic<bool> midiMuted_        { false };  // true = block all MIDI output

    bool gamepadVoiceWasHeld_[4] = {};  // audio thread only — tracks previous gamepad held state
    bool allNotesWasHeld_ = false;       // audio thread only — tracks previous L3 held state
    bool prevIsDawPlaying_ = false;      // audio thread only — for DAW stop detection
    int   prevXMode_    = -1;            // audio thread only — dedup reset on X mode change
    int   prevYMode_    = -1;            // audio thread only — dedup reset on Y mode change
    float prevFilterX_  = -99.0f;       // audio thread only — raw joystick X, -99 = uninitialised
    float prevFilterY_  = -99.0f;       // audio thread only — raw joystick Y, -99 = uninitialised

    // ── CC dedup: last emitted integer values for CC74 and CC71 ──────────────
    // -1 = never sent; forces emission on first connect.
    // Declared atomic<int> to avoid data race: reset from message thread
    // (onConnectionChange lambda), read/written on audio thread (processBlock).
    std::atomic<int> prevCcCut_ { -1 };
    std::atomic<int> prevCcRes_ { -1 };

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

    // Build ChordEngine::Params from current APVTS + joystick state
    ChordEngine::Params buildChordParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
