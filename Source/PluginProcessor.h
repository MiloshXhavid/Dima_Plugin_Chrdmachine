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
    bool acceptsMidi()   const override { return false; }
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
    // Default: X = center (0.0), Y = bottom (-1.0 = lowest pitch)
    std::atomic<float> joystickX {0.0f};
    std::atomic<float> joystickY {-1.0f};

    // Touch-plate state (UI → processor)
    void setPadState(int voice, bool pressed) { trigger_.setPadState(voice, pressed); }
    bool isGateOpen(int voice) const          { return trigger_.isGateOpen(voice); }

    // Looper transport (UI / gamepad)
    void looperStartStop() { looper_.startStop(); }
    void looperRecord()    { looper_.record();    }
    void looperReset()     { looper_.reset();     }
    void looperDelete()    { looper_.deleteLoop(); }
    bool looperIsPlaying()   const { return looper_.isPlaying(); }
    bool looperIsRecording() const { return looper_.isRecording(); }

    // Gamepad
    GamepadInput& getGamepad() { return gamepad_; }

    // ── APVTS ─────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    ChordEngine   chord_;
    TriggerSystem trigger_;
    LooperEngine  looper_;
    GamepadInput  gamepad_;

    double sampleRate_ = 44100.0;

    // Previous joystick position — used to compute per-block deltas for JOY gate detection.
    // Initialised to match joystickX/joystickY defaults so the first delta is 0.
    float prevJoyX_ = 0.0f;
    float prevJoyY_ = -1.0f;

    // Held (sample-and-hold) pitches for each voice
    std::array<int, 4> heldPitch_ {60, 64, 67, 70};

    // Build ChordEngine::Params from current APVTS + joystick state
    ChordEngine::Params buildChordParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
