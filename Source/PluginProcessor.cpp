#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// ─── Parameter IDs ────────────────────────────────────────────────────────────
namespace ParamID
{
    // Chord
    static const juce::String globalTranspose  = "globalTranspose";
    static const juce::String thirdInterval    = "thirdInterval";
    static const juce::String fifthInterval    = "fifthInterval";
    static const juce::String tensionInterval  = "tensionInterval";
    static const juce::String rootOctave       = "rootOctave";
    static const juce::String thirdOctave      = "thirdOctave";
    static const juce::String fifthOctave      = "fifthOctave";
    static const juce::String tensionOctave    = "tensionOctave";
    static const juce::String joystickXAtten   = "joystickXAtten";
    static const juce::String joystickYAtten   = "joystickYAtten";

    // Scale
    static const juce::String scalePreset      = "scalePreset";
    static const juce::String useCustomScale   = "useCustomScale";
    // scaleNote0..scaleNote11 generated inline

    // Trigger threshold
    static const juce::String joystickThreshold = "joystickThreshold";

    // Trigger
    static const juce::String triggerSource0   = "triggerSource0";
    static const juce::String triggerSource1   = "triggerSource1";
    static const juce::String triggerSource2   = "triggerSource2";
    static const juce::String triggerSource3   = "triggerSource3";
    static const juce::String randomDensity    = "randomDensity";
    static const juce::String randomSubdiv0    = "randomSubdiv0";
    static const juce::String randomSubdiv1    = "randomSubdiv1";
    static const juce::String randomSubdiv2    = "randomSubdiv2";
    static const juce::String randomSubdiv3    = "randomSubdiv3";
    static const juce::String randomGateTime   = "randomGateTime";

    // Filter
    static const juce::String filterXAtten     = "filterXAtten";
    static const juce::String filterYAtten     = "filterYAtten";
    static const juce::String filterXOffset    = "filterXOffset";
    static const juce::String filterYOffset    = "filterYOffset";
    static const juce::String filterMidiCh     = "filterMidiCh";

    // MIDI channels
    static const juce::String voiceCh0         = "voiceCh0";
    static const juce::String voiceCh1         = "voiceCh1";
    static const juce::String voiceCh2         = "voiceCh2";
    static const juce::String voiceCh3         = "voiceCh3";

    // Looper
    static const juce::String looperSubdiv     = "looperSubdiv";
    static const juce::String looperLength     = "looperLength";

    // Arpeggiator
    static const juce::String arpEnabled       = "arpEnabled";
    static const juce::String arpSubdiv        = "arpSubdiv";
    static const juce::String arpOrder         = "arpOrder";

    // Quantize
    static const juce::String quantizeMode     = "quantizeMode";
    static const juce::String quantizeSubdiv   = "quantizeSubdiv";

    // LFO
    static const juce::String lfoXEnabled    = "lfoXEnabled";
    static const juce::String lfoYEnabled    = "lfoYEnabled";
    static const juce::String lfoXWaveform   = "lfoXWaveform";
    static const juce::String lfoYWaveform   = "lfoYWaveform";
    static const juce::String lfoXRate       = "lfoXRate";
    static const juce::String lfoYRate       = "lfoYRate";
    static const juce::String lfoXLevel      = "lfoXLevel";
    static const juce::String lfoYLevel      = "lfoYLevel";
    static const juce::String lfoXPhase      = "lfoXPhase";
    static const juce::String lfoYPhase      = "lfoYPhase";
    static const juce::String lfoXDistortion = "lfoXDistortion";
    static const juce::String lfoYDistortion = "lfoYDistortion";
    static const juce::String lfoXSync       = "lfoXSync";
    static const juce::String lfoYSync       = "lfoYSync";
    static const juce::String lfoXSubdiv     = "lfoXSubdiv";
    static const juce::String lfoYSubdiv     = "lfoYSubdiv";

}

// ─── Parameter layout ─────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addInt   = [&](const juce::String& id, const juce::String& name,
                        int min, int max, int def)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(id, name, min, max, def));
    };
    auto addFloat = [&](const juce::String& id, const juce::String& name,
                        float min, float max, float def)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            id, name, juce::NormalisableRange<float>(min, max, 0.01f), def));
    };
    auto addBool  = [&](const juce::String& id, const juce::String& name, bool def)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(id, name, def));
    };
    auto addChoice = [&](const juce::String& id, const juce::String& name,
                         juce::StringArray choices, int def)
    {
        layout.add(std::make_unique<juce::AudioParameterChoice>(id, name, choices, def));
    };

    // ── Chord ─────────────────────────────────────────────────────────────────
    // Transpose: 12 discrete semitone values displayed as note names (C..B)
    {
        static const char* kNoteNames[12] = {
            "C","C#/Db","D","D#/Eb","E","F","F#/Gb","G","G#/Ab","A","A#/Bb","B"
        };
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { ParamID::globalTranspose, 1 },
            "Global Transpose", 0, 11, 0,
            juce::AudioParameterIntAttributes{}
                .withStringFromValueFunction([](int v, int) -> juce::String {
                    return juce::String(kNoteNames[juce::jlimit(0, 11, v)]);
                })
                .withValueFromStringFunction([](const juce::String& s) -> int {
                    for (int i = 0; i < 12; ++i)
                        if (s.equalsIgnoreCase(kNoteNames[i])) return i;
                    return s.getIntValue();
                })
        ));
    }
    addInt  (ParamID::thirdInterval,   "Third Interval",    0, 12,  4);
    addInt  (ParamID::fifthInterval,   "Fifth Interval",    0, 12,  7);
    addInt  (ParamID::tensionInterval, "Tension Interval",  0, 12, 11);
    addInt  (ParamID::rootOctave,      "Root Octave",       0, 12,  2);
    addInt  (ParamID::thirdOctave,     "Third Octave",      0, 12,  4);
    addInt  (ParamID::fifthOctave,     "Fifth Octave",      0, 12,  3);
    addInt  (ParamID::tensionOctave,   "Tension Octave",    0, 12,  3);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickXAtten, "Joy X Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickYAtten, "Joy Y Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 24.0f));
    addFloat(ParamID::joystickThreshold,  "Joystick Threshold", 0.001f, 0.1f, 0.015f);
    addFloat("joystickGateTime", "Joystick Gate Time", 0.05f, 5.0f, 1.0f);

    // ── Scale ─────────────────────────────────────────────────────────────────
    {
        juce::StringArray scaleNames;
        for (int i = 0; i < (int)ScalePreset::COUNT; ++i)
            scaleNames.add(ScaleQuantizer::getScaleName(static_cast<ScalePreset>(i)));

        addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 0);
    }
    addBool(ParamID::useCustomScale, "Use Custom Scale", false);
    for (int i = 0; i < 12; ++i)
    {
        static const bool kMajorDefault[12] = {1,0,1,0,1,1,0,1,0,1,0,1};
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "scaleNote" + juce::String(i),
            "Scale Note " + juce::String(i),
            kMajorDefault[i]));
    }

    // ── Trigger ───────────────────────────────────────────────────────────────
    juce::StringArray trigSrcNames { "TouchPlate", "Joystick", "Random" };
    addChoice(ParamID::triggerSource0, "Trigger Source Root",    trigSrcNames, 0);
    addChoice(ParamID::triggerSource1, "Trigger Source Third",   trigSrcNames, 0);
    addChoice(ParamID::triggerSource2, "Trigger Source Fifth",   trigSrcNames, 0);
    addChoice(ParamID::triggerSource3, "Trigger Source Tension", trigSrcNames, 0);
    addFloat (ParamID::randomDensity,  "Random Density",   1.0f, 8.0f, 4.0f);
    {
        const juce::StringArray subdivChoices { "1/4", "1/8", "1/16", "1/32" };
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv0", "Random Subdiv Root",    subdivChoices, 1));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv1", "Random Subdiv Third",   subdivChoices, 1));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv2", "Random Subdiv Fifth",   subdivChoices, 1));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv3", "Random Subdiv Tension", subdivChoices, 1));
    }
    addFloat("randomGateTime", "Random Gate Time", 0.0f, 1.0f, 0.5f);

    // Random clock mode
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "randomClockSync", "Random Clock Sync", true));

    // Free-running tempo for random triggers
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "randomFreeTempo", "Random Free Tempo",
        juce::NormalisableRange<float>(30.0f, 240.0f, 0.1f), 120.0f));

    // ── Filter ────────────────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::filterXAtten, "Filter Cutoff Attenuator",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 99.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::filterYAtten, "Filter Resonance Attenuator",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 99.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::filterXOffset, "Filter Cutoff Base",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 64.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::filterYOffset, "Filter Resonance Base",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 64.0f));
    addInt  (ParamID::filterMidiCh, "Filter MIDI Channel",  1, 16, 1);
    {
        juce::StringArray yModes { "Resonance", "LFO Rate" };
        juce::StringArray xModes { "Cutoff",    "VCF LFO",  "Mod Wheel" };
        addChoice("filterYMode", "Left Stick Y Mode", yModes, 0);  // 0=CC71 Res, 1=CC76 LFO Rate, 2=CC64 Sustain
        addChoice("filterXMode", "Left Stick X Mode", xModes, 0);  // 0=CC74 Cut, 1=CC12 VCF LFO, 2=CC1 Mod
    }

    // ── MIDI routing ──────────────────────────────────────────────────────────
    addInt(ParamID::voiceCh0, "Root MIDI Channel",    1, 16, 1);
    addInt(ParamID::voiceCh1, "Third MIDI Channel",   1, 16, 2);
    addInt(ParamID::voiceCh2, "Fifth MIDI Channel",   1, 16, 3);
    addInt(ParamID::voiceCh3, "Tension MIDI Channel", 1, 16, 4);

    // ── Looper ────────────────────────────────────────────────────────────────
    addChoice(ParamID::looperSubdiv, "Loop Time Signature",
              { "3/4", "4/4", "5/4", "7/8", "9/8", "11/8" }, 1);
    addInt   (ParamID::looperLength, "Loop Length (bars)", 1, 16, 2);

    // ── Arpeggiator ───────────────────────────────────────────────────────────
    addBool(ParamID::arpEnabled, "Arp Enabled", false);
    addChoice(ParamID::arpSubdiv, "Arp Subdivision",
              { "1/4", "1/8T", "1/8", "1/16T", "1/16", "1/32" }, 2);  // default: 1/8
    addChoice(ParamID::arpOrder, "Arp Order",
              { "Up", "Down", "Up+Down", "Down+Up", "Outer-In", "Inner-Out", "Random" }, 0);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "arpGateTime", "Arp Gate Time",
        juce::NormalisableRange<float>(5.0f, 100.0f, 1.0f), 75.0f));  // percentage 5-100%

    // ── Filter CC live display (read-only from DAW perspective) ──────────────
    // Updated every timer tick from audio-thread atomics so DAW can see stick movement.
    addFloat("filterCutLive", "Filter Cut CC", 0.0f, 127.0f, 0.0f);
    addFloat("filterResLive", "Filter Res CC", 0.0f, 127.0f, 0.0f);

    // ── Quantize ──────────────────────────────────────────────────────────────
    addInt(ParamID::quantizeMode,   "Quantize Mode",   0, 2, 0);  // 0=Off, 1=Live, 2=Post (default Off)
    addInt(ParamID::quantizeSubdiv, "Quantize Subdiv", 0, 3, 1);  // 0=1/4, 1=1/8, 2=1/16, 3=1/32 (default 1/8)

    // ── LFO ───────────────────────────────────────────────────────────────────
    addBool(ParamID::lfoXEnabled, "LFO X Enabled", false);
    addBool(ParamID::lfoYEnabled, "LFO Y Enabled", false);
    {
        juce::StringArray waveforms { "Sine", "Triangle", "Saw Up", "Saw Down",
                                      "Square", "S&H", "Random" };
        addChoice(ParamID::lfoXWaveform, "LFO X Waveform", waveforms, 0);
        addChoice(ParamID::lfoYWaveform, "LFO Y Waveform", waveforms, 0);
    }
    // Rate: log-scale NormalisableRange so slider midpoint ≈ 1 Hz.
    // Skew factor computed as: log(0.5) / log((midpoint - start) / (end - start))
    //   = log(0.5) / log((1.0 - 0.01) / (20.0 - 0.01)) ≈ 0.2306
    // This means at normalized position 0.5 the displayed value is ~1 Hz.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoXRate, "LFO X Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.2306f),
        1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoYRate, "LFO Y Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.2306f),
        1.0f));
    addFloat(ParamID::lfoXLevel,      "LFO X Level",      0.0f, 2.0f, 0.0f);
    addFloat(ParamID::lfoYLevel,      "LFO Y Level",      0.0f, 2.0f, 0.0f);
    addFloat(ParamID::lfoXPhase,      "LFO X Phase",      0.0f, 360.0f, 0.0f);
    addFloat(ParamID::lfoYPhase,      "LFO Y Phase",      0.0f, 360.0f, 0.0f);
    addFloat(ParamID::lfoXDistortion, "LFO X Distortion", 0.0f, 1.0f, 0.0f);
    addFloat(ParamID::lfoYDistortion, "LFO Y Distortion", 0.0f, 1.0f, 0.0f);
    addBool (ParamID::lfoXSync,       "LFO X Sync",       false);
    addBool (ParamID::lfoYSync,       "LFO Y Sync",       false);
    {
        juce::StringArray subdivs { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
        addChoice(ParamID::lfoXSubdiv, "LFO X Subdivision", subdivs, 2);  // default: 1/4
        addChoice(ParamID::lfoYSubdiv, "LFO Y Subdivision", subdivs, 2);  // default: 1/4
    }

    return layout;
}

// ─── Constructor ──────────────────────────────────────────────────────────────

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), false)
          .withOutput("Output", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "ChordJoystick", createParameterLayout())
{
    gamepad_.onConnectionChange = [this](bool connected)
    {
        if (!connected)
        {
            pendingAllNotesOff_.store(true, std::memory_order_release);
            pendingCcReset_.store(true,     std::memory_order_release);
        }
        // CC dedup state reset so next connect gets a fresh emission.
        // prevCcCut_/prevCcRes_ are atomic<int> — safe to write from message thread.
        if (!connected)
        {
            prevCcCut_.store(-1, std::memory_order_relaxed);
            prevCcRes_.store(-1, std::memory_order_relaxed);
        }
    };
}

PluginProcessor::~PluginProcessor() {}

// ─── Prepare ─────────────────────────────────────────────────────────────────

void PluginProcessor::prepareToPlay(double sr, int /*blockSize*/)
{
    sampleRate_ = sr;
    lfoX_.reset();
    lfoY_.reset();
    sampleCounter_ = 0;
    prevBeatCount_ = -1.0;
}

void PluginProcessor::releaseResources()
{
    // Cannot write MIDI here — audio thread already stopped.
    // Reset all gate state so next prepareToPlay starts clean.
    trigger_.resetAllGates();
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& audio,
                                            juce::MidiBuffer& midi)
{
    audio.clear();
    midi.clear();

    static const juce::String chIDs[4] = {
        "voiceCh0", "voiceCh1", "voiceCh2", "voiceCh3" };

    for (int v = 0; v < 4; ++v)
    {
        const int ch = (int)apvts.getRawParameterValue(chIDs[v])->load();  // 1-based

        const int pitch = trigger_.getActivePitch(v);
        if (pitch >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), 0);
    }
    trigger_.resetAllGates();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // MIDI effect: buses must be disabled (0 ch) or stereo passthrough (2 ch).
    // Ableton deactivates audio buses before loading on a MIDI track;
    // accepting both layouts prevents kResultFalse from activateBus().
    const int numIn  = layouts.getMainInputChannels();
    const int numOut = layouts.getMainOutputChannels();
    return (numIn == 0 && numOut == 0) || (numIn == 2 && numOut == 2);
}

// ─── ChordParams builder ──────────────────────────────────────────────────────

ChordEngine::Params PluginProcessor::buildChordParams() const
{
    ChordEngine::Params p;

    // Apply gamepad right-stick on top of UI joystick
    const float gpX = gamepad_.getPitchX();
    const float gpY = gamepad_.getPitchY();
    const float jx  = joystickX.load();
    const float jy  = joystickY.load();

    // Per-axis center snap: values within ±5% snap to exactly 0 (matches UI snap in updateFromMouse).
    // This prevents hardware noise near stick-center from jittering the pitch.
    constexpr float kCenterSnap = 0.05f;
    const float gpXs = std::abs(gpX) < kCenterSnap ? 0.0f : gpX;
    const float gpYs = std::abs(gpY) < kCenterSnap ? 0.0f : gpY;

    // Prefer gamepad if any axis is active after snap.
    // Allow during playback when: actively recording, or no joystick content recorded yet.
    const bool gpActive = (!looper_.isPlaying() || looper_.isRecording() || !looper_.hasJoystickContent())
                          && (std::abs(gpXs) + std::abs(gpYs)) > 0.0f;
    p.joystickX = gpActive ? gpXs : jx;
    p.joystickY = gpActive ? gpYs : jy;

    p.xAtten = apvts.getRawParameterValue(ParamID::joystickXAtten)->load();
    p.yAtten = apvts.getRawParameterValue(ParamID::joystickYAtten)->load();

    p.globalTranspose = (int)apvts.getRawParameterValue(ParamID::globalTranspose)->load();
    p.intervals[0] = 0;
    p.intervals[1] = (int)apvts.getRawParameterValue(ParamID::thirdInterval)->load();
    p.intervals[2] = (int)apvts.getRawParameterValue(ParamID::fifthInterval)->load();
    p.intervals[3] = (int)apvts.getRawParameterValue(ParamID::tensionInterval)->load();

    p.octaves[0] = (int)apvts.getRawParameterValue(ParamID::rootOctave)->load();
    p.octaves[1] = (int)apvts.getRawParameterValue(ParamID::thirdOctave)->load();
    p.octaves[2] = (int)apvts.getRawParameterValue(ParamID::fifthOctave)->load();
    p.octaves[3] = (int)apvts.getRawParameterValue(ParamID::tensionOctave)->load();

    p.useCustomScale = apvts.getRawParameterValue(ParamID::useCustomScale)->load() > 0.5f;
    p.scalePreset    = static_cast<ScalePreset>(
        (int)apvts.getRawParameterValue(ParamID::scalePreset)->load());

    for (int i = 0; i < 12; ++i)
        p.customNotes[i] =
            apvts.getRawParameterValue("scaleNote" + juce::String(i))->load() > 0.5f;

    return p;
}

// ─── processBlock ─────────────────────────────────────────────────────────────

void PluginProcessor::processBlock(juce::AudioBuffer<float>& audio,
                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenum;
    audio.clear();
    midi.clear();

    // Feed current joystick threshold to GamepadInput dead zone each block.
    // GamepadInput::setDeadZone is thread-safe (atomic store, relaxed).
    {
        const float dz = apvts.getRawParameterValue(ParamID::joystickThreshold)->load();
        gamepad_.setDeadZone(dz);
    }

    const int blockSize = audio.getNumSamples();

    // ── Poll gamepad triggers (momentary held-state gates + D-pad) ───────────
    if (gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed))
    {
        // Voice gates — hold mode is inverted: press = note-off (mute), release = note-on (hold resumes).
        // Normal mode: press = note-on, release = note-off.
        for (int v = 0; v < 4; ++v)
        {
            const bool held    = gamepad_.getVoiceHeld(v);
            const bool rising  = held  && !gamepadVoiceWasHeld_[v];
            const bool falling = !held &&  gamepadVoiceWasHeld_[v];
            const bool holdOn  = padHold_[v].load();

            if (rising)
                trigger_.setPadState(v, !holdOn);   // hold: note-off; normal: note-on
            else if (falling)
                trigger_.setPadState(v, holdOn);    // hold: note-on (resume); normal: note-off

            gamepadVoiceWasHeld_[v] = held;
        }

        // L3 (left stick press): trigger all PAD-mode voices, held while pressed.
        // Mirrors the ALL touchplate — respect hold mode per voice (same inversion as individual pads).
        {
            const bool held = gamepad_.getAllNotesHeld();
            if (held != allNotesWasHeld_)
            {
                for (int v = 0; v < 4; ++v)
                {
                    const int src = (int)apvts.getRawParameterValue(
                        "triggerSource" + juce::String(v))->load();
                    if (src == 0)
                    {
                        const bool holdOn = padHold_[v].load();
                        // hold mode: press=note-off, release=note-on (mirrors individual TouchPlate)
                        trigger_.setPadState(v, holdOn ? !held : held);
                    }
                }
                allNotesWasHeld_ = held;
            }
            // Consume the rising-edge flag so it doesn't accumulate
            gamepad_.consumeAllNotesTrigger();
        }

        if (gamepad_.consumeLooperStartStop()) looper_.startStop();
        if (gamepad_.consumeLooperRecord())    looper_.record();
        if (gamepad_.consumeLooperReset())   { looper_.reset();       flashLoopReset_.fetch_add(1,  std::memory_order_relaxed); }
        if (gamepad_.consumeLooperDelete())  { looper_.deleteLoop();  flashLoopDelete_.fetch_add(1, std::memory_order_relaxed); }
        if (gamepad_.consumeRightStickTrigger())
            triggerPanic();   // R3 -> MIDI panic (same as UI panicBtn_)

        // D-pad Up/Down: BPM delta (normal) or PC message (preset-scroll mode)
        // In preset-scroll mode: consumePcDelta() returns the delta (GamepadInput fires on button-up).
        // In normal mode: consumeDpadUp/Down fire on button-down (unchanged).
        if (gamepad_.isPresetScrollActive())
        {
            const int pcDelta = gamepad_.consumePcDelta();
            if (pcDelta != 0)
            {
                const int newProgram = juce::jlimit(0, 127, programCounter_ + pcDelta);
                if (newProgram != programCounter_)  // boundary clamp: skip send if already at limit
                {
                    programCounter_ = newProgram;
                    const int fCh = static_cast<int>(
                        apvts.getRawParameterValue(ParamID::filterMidiCh)->load());  // 1-based
                    midi.addEvent(
                        juce::MidiMessage::programChange(fCh, programCounter_),
                        0);  // sample offset 0 within the block
                }
            }
        }
        else
        {
            if (gamepad_.consumeDpadUp())    pendingBpmDelta_.fetch_add(1,  std::memory_order_relaxed);
            if (gamepad_.consumeDpadDown())  pendingBpmDelta_.fetch_add(-1, std::memory_order_relaxed);
        }
        if (gamepad_.consumeDpadLeft())  looperSetRecGates(!looperIsRecGates());
        if (gamepad_.consumeDpadRight()) looperSetRecJoy(!looperIsRecJoy());
    }
    else
    {
        // Gamepad inactive or disconnected — release any previously held voices
        for (int v = 0; v < 4; ++v)
        {
            if (gamepadVoiceWasHeld_[v]) trigger_.setPadState(v, false);
            gamepadVoiceWasHeld_[v] = false;
        }
    }

    // ── Looper ────────────────────────────────────────────────────────────────
    // Extract DAW position using JUCE 8 API (Optional<PositionInfo>)
    double ppqPos       = -1.0;
    double dawBpm       = 120.0;
    bool   isDawPlaying = false;
    if (auto* head = getPlayHead())
    {
        if (auto posOpt = head->getPosition(); posOpt.hasValue())
        {
            isDawPlaying = posOpt->getIsPlaying();
            if (auto ppq = posOpt->getPpqPosition(); ppq.hasValue())
                ppqPos = *ppq;
            if (auto bpmOpt = posOpt->getBpm(); bpmOpt.hasValue())
                dawBpm = *bpmOpt;
        }
    }
    const bool hasDaw = isDawPlaying;

    // Use free tempo knob when not DAW-synced; use DAW BPM when synced and DAW is rolling.
    const double freeTempo = static_cast<double>(
        apvts.getRawParameterValue("randomFreeTempo")->load());
    LooperEngine::ProcessParams lp;
    lp.sampleRate   = sampleRate_;
    lp.bpm          = (looper_.isSyncToDaw() && isDawPlaying && dawBpm > 0.0) ? dawBpm : freeTempo;
    lp.ppqPosition  = ppqPos;
    lp.blockSize    = blockSize;
    lp.isDawPlaying = isDawPlaying;
    effectiveBpm_.store(static_cast<float>(lp.bpm), std::memory_order_relaxed);

    const auto loopOut = looper_.process(lp);

    // Override joystick from looper playback if applicable
    if (loopOut.hasJoystickX) joystickX.store(loopOut.joystickX);
    if (loopOut.hasJoystickY) joystickY.store(loopOut.joystickY);

    // ── Build chord params ────────────────────────────────────────────────────
    ChordEngine::Params chordP = buildChordParams();

    // ── LFO modulation ────────────────────────────────────────────────────────
    // Subdivision lookup: beats per cycle (quarter-note = 1.0 beat)
    // Maps lfoXSubdiv / lfoYSubdiv choice index to subdivBeats for LfoEngine
    // Indices:               1/1  1/2  1/4  1/8   1/16   1/32
    static constexpr double kLfoSubdivBeats[6] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };
    {
        const bool  xEnabled = *apvts.getRawParameterValue(ParamID::lfoXEnabled) > 0.5f;
        const bool  yEnabled = *apvts.getRawParameterValue(ParamID::lfoYEnabled) > 0.5f;
        const float xLevel   = *apvts.getRawParameterValue(ParamID::lfoXLevel);
        const float yLevel   = *apvts.getRawParameterValue(ParamID::lfoYLevel);

        // Compute ramp targets — process only if enabled and level > 0
        // Per CONTEXT.md: at level=0, phase does NOT advance (bypass path).
        float xTarget = 0.0f;
        if (xEnabled && xLevel > 0.0f)
        {
            ProcessParams xp;
            xp.sampleRate   = sampleRate_;
            xp.blockSize    = blockSize;
            xp.bpm          = lp.bpm;
            xp.syncMode     = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
            xp.ppqPosition  = ppqPos;
            xp.isDawPlaying = isDawPlaying;
            xp.rateHz       = *apvts.getRawParameterValue(ParamID::lfoXRate);
            xp.subdivBeats  = kLfoSubdivBeats[
                juce::jlimit(0, 5, (int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv))];
            xp.maxCycleBeats = 16.0;
            xp.waveform     = static_cast<Waveform>(
                (int)*apvts.getRawParameterValue(ParamID::lfoXWaveform));
            xp.phaseShift   = *apvts.getRawParameterValue(ParamID::lfoXPhase) / 360.0f;
            xp.distortion   = *apvts.getRawParameterValue(ParamID::lfoXDistortion);
            xp.level        = xLevel;
            xTarget = lfoX_.process(xp);
        }

        float yTarget = 0.0f;
        if (yEnabled && yLevel > 0.0f)
        {
            ProcessParams yp;
            yp.sampleRate   = sampleRate_;
            yp.blockSize    = blockSize;
            yp.bpm          = lp.bpm;
            yp.syncMode     = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
            yp.ppqPosition  = ppqPos;
            yp.isDawPlaying = isDawPlaying;
            yp.rateHz       = *apvts.getRawParameterValue(ParamID::lfoYRate);
            yp.subdivBeats  = kLfoSubdivBeats[
                juce::jlimit(0, 5, (int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv))];
            yp.maxCycleBeats = 16.0;
            yp.waveform     = static_cast<Waveform>(
                (int)*apvts.getRawParameterValue(ParamID::lfoYWaveform));
            yp.phaseShift   = *apvts.getRawParameterValue(ParamID::lfoYPhase) / 360.0f;
            yp.distortion   = *apvts.getRawParameterValue(ParamID::lfoYDistortion);
            yp.level        = yLevel;
            yTarget = lfoY_.process(yp);
        }

        // Ramp toward target — smooths the disable transition so LFO fades out rather
        // than snapping hard to 0.  Coefficient is clamped to 1.0 so the filter is
        // always stable: coeff > 1 causes the IIR to overshoot and oscillate every block,
        // which is exactly the "cursor jumping" artefact the user reported.
        // At coeff = 1.0 (large buffers) the ramp collapses to a direct assignment —
        // the cursor tracks the LFO output exactly, one block of latency.
        const float rampCoeff = juce::jmin(1.0f,
                                    static_cast<float>(blockSize) /
                                    std::max(1.0f, static_cast<float>(sampleRate_ * 0.01)));
        lfoXRampOut_ += (xTarget - lfoXRampOut_) * rampCoeff;
        lfoYRampOut_ += (yTarget - lfoYRampOut_) * rampCoeff;

        // Store the UNCLIPPED modulated position for cursor display so the dot can
        // show the full LFO swing even when the joystick is near ±1.
        // (JoystickPad::paint() applies its own dotR-aware bounds clamp.)
        modulatedJoyX_.store(chordP.joystickX + lfoXRampOut_, std::memory_order_relaxed);
        modulatedJoyY_.store(chordP.joystickY + lfoYRampOut_, std::memory_order_relaxed);

        chordP.joystickX = std::clamp(chordP.joystickX + lfoXRampOut_, -1.0f, 1.0f);
        chordP.joystickY = std::clamp(chordP.joystickY + lfoYRampOut_, -1.0f, 1.0f);
    }
    // ─────────────────────────────────── end LFO ──────────────────────────────

    // ── Beat clock detection — fires beatOccurred_ once per beat ──────────────
    {
        double currentBeatCount;
        if (isDawPlaying && ppqPos >= 0.0)
        {
            // DAW playing: use ppqPos which the host advances by BPM * blockSize / (60 * sr)
            currentBeatCount = ppqPos;
        }
        else
        {
            // Free tempo: derive beat count from sample counter
            const float bpm = effectiveBpm_.load(std::memory_order_relaxed);
            currentBeatCount = (static_cast<double>(bpm) / 60.0)
                             * static_cast<double>(sampleCounter_) / sampleRate_;
        }

        if (prevBeatCount_ >= 0.0
            && std::floor(currentBeatCount) > std::floor(prevBeatCount_))
        {
            beatOccurred_.store(true, std::memory_order_relaxed);
        }
        prevBeatCount_ = currentBeatCount;

        // Advance free-tempo sample counter (also consumed by free-tempo beat path above)
        sampleCounter_ += blockSize;
    }

    // ── Compute & sample-hold pitches ─────────────────────────────────────────
    // We only recompute on trigger events.  The trigger callback does this.
    // Pre-compute all 4 pitches once so the callback can use them.
    int freshPitches[4];
    for (int v = 0; v < 4; ++v)
        freshPitches[v] = ChordEngine::computePitch(v, chordP);

    // ── Trigger system ────────────────────────────────────────────────────────
    const int voiceChs[4] =
    {
        (int)apvts.getRawParameterValue(ParamID::voiceCh0)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh1)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh2)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh3)->load(),
    };

    // Detect DAW stop/start — covers both SYNC-on (loopOut.dawStopped) and SYNC-off cases.
    // Sends all-notes-off and resets TriggerSystem gate state so notes don't hang.
    // dawJustStarted is used later by the arp arm logic.
    bool dawJustStarted = false;
    {
        dawJustStarted = !prevIsDawPlaying_ && isDawPlaying;
        const bool justStopped = (prevIsDawPlaying_ && !isDawPlaying) || loopOut.dawStopped;
        prevIsDawPlaying_ = isDawPlaying;
        if (justStopped)
        {
            for (int v = 0; v < 4; ++v)
                midi.addEvent(juce::MidiMessage::allNotesOff(voiceChs[v]), 0);
            trigger_.resetAllGates();
            lfoX_.reset();
            lfoY_.reset();
            lfoXRampOut_ = 0.0f;
            lfoYRampOut_ = 0.0f;
            sampleCounter_ = 0;
            prevBeatCount_ = -1.0;
        }
    }

    if (dawJustStarted)
    {
        lfoX_.reset();  // align phase to beat 0 on transport restart
        lfoY_.reset();
        lfoXRampOut_ = 0.0f;
        lfoYRampOut_ = 0.0f;
        sampleCounter_ = 0;
        prevBeatCount_ = -1.0;
    }

    // Detect looper start/stop — send note-offs for any hanging looper notes on stop.
    // When startStop() is called, playing_ goes false before process() runs,
    // so no gateOff events are emitted for voices still active. Detect the
    // transition here and emit note-offs manually.
    bool looperJustStarted = false;
    {
        const bool looperNowPlaying = looper_.isPlaying();
        if (prevLooperWasPlaying_ && !looperNowPlaying)
        {
            for (int v = 0; v < 4; ++v)
            {
                if (looperActivePitch_[v] >= 0)
                {
                    midi.addEvent(juce::MidiMessage::noteOff(voiceChs[v], looperActivePitch_[v], (uint8_t)0), 0);
                    looperActivePitch_[v] = -1;
                }
            }
        }
        looperJustStarted = !prevLooperWasPlaying_ && looperNowPlaying;
        prevLooperWasPlaying_ = looperNowPlaying;
    }

    // Looper reset (seek to bar 0): cut any active looper notes, keep live gates intact.
    if (loopOut.looperReset)
    {
        for (int v = 0; v < 4; ++v)
        {
            if (looperActivePitch_[v] >= 0)
            {
                midi.addEvent(juce::MidiMessage::noteOff(voiceChs[v], looperActivePitch_[v], (uint8_t)0), 0);
                looperActivePitch_[v] = -1;
            }
        }
    }

    // ── MIDI Panic / Mute ─────────────────────────────────────────────────────
    // pendingPanic_ fires unconditionally: stops looper, sends allNotesOff, resets gates.
    // This happens BEFORE the mute gate so allNotesOff always reaches the synth.
    if (pendingPanic_.exchange(false, std::memory_order_acq_rel))
    {
        if (looper_.isPlaying()) looper_.startStop();
        for (int ch = 1; ch <= 16; ++ch)
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(ch, 64,  0), 0);  // sustain off
            midi.addEvent(juce::MidiMessage::controllerEvent(ch, 120, 0), 0);  // all sound off
            midi.addEvent(juce::MidiMessage::allNotesOff(ch),             0);  // CC123 all notes off
        }
        trigger_.resetAllGates();
        looperActivePitch_.fill(-1);
        prevCcCut_.store(-1, std::memory_order_relaxed);
        prevCcRes_.store(-1, std::memory_order_relaxed);
        flashPanic_.fetch_add(1, std::memory_order_relaxed);
    }

    // When muted: allNotesOff is already in the buffer above; block all further MIDI output.
    if (midiMuted_.load(std::memory_order_relaxed))
        return;

    // Looper gate playback: emit MIDI directly, bypassing TriggerSystem.
    // This satisfies the locked decision: live pad input passes through independently
    // because TriggerSystem is not informed of looper events — it only sees live pad state.
    for (int v = 0; v < 4; ++v)
    {
        const int ch0 = voiceChs[v] - 1;  // 0-based for MIDI message
        if (loopOut.gateOn[v])
        {
            // Snapshot the pitch at gate-on so gateOff uses the same note number
            // even if heldPitch_[v] changes before the gate closes.
            looperActivePitch_[v] = heldPitch_[v];
            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, looperActivePitch_[v], (uint8_t)100), 0);
        }
        if (loopOut.gateOff[v] && looperActivePitch_[v] >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, looperActivePitch_[v], (uint8_t)0), 0);
            looperActivePitch_[v] = -1;
        }
    }

    TriggerSource src[4];
    const juce::String srcIDs[4] = {
        ParamID::triggerSource0, ParamID::triggerSource1,
        ParamID::triggerSource2, ParamID::triggerSource3
    };
    for (int v = 0; v < 4; ++v)
        src[v] = static_cast<TriggerSource>(
            (int)apvts.getRawParameterValue(srcIDs[v])->load());

    // Update held pitches for voices about to be triggered
    for (int v = 0; v < 4; ++v)
        heldPitch_[v] = freshPitches[v];

    const bool arpOn = (*apvts.getRawParameterValue(ParamID::arpEnabled) > 0.5f);

    bool anyNoteOnThisBlock = false;

    TriggerSystem::ProcessParams tp;
    tp.onNote = [&](int voice, int pitch, bool isOn, int sampleOff)
    {
        if (isOn)
        {
            anyNoteOnThisBlock = true;
            // Activate "start rec by touch" before recordGate so the triggering
            // note-on is captured. activateRecordingNow() is a no-op unless armed.
            looper_.activateRecordingNow();

            // Pad priority: choke any active arp note on this voice first.
            // This lets the live press cut the arp immediately ("instant choke").
            if (arpOn && arpActiveVoice_ == voice && arpActivePitch_ >= 0)
            {
                const int ch0v = voiceChs[voice] - 1;
                midi.addEvent(juce::MidiMessage::noteOff(ch0v + 1, arpActivePitch_, (uint8_t)0), sampleOff);
                arpActivePitch_ = -1;
                arpActiveVoice_ = -1;
                arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                arpNoteOffRemaining_ = 0.0;
            }
        }

        const int ch0 = voiceChs[voice] - 1;  // 0-based
        if (isOn)
        {
            // Record gate event in looper (using JUCE 8 ppqPos variable)
            const double beatPos = (hasDaw && ppqPos >= 0.0)
                ? ppqPos
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, true);
            // Live pad notes always go through regardless of arp state.
            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), sampleOff);
        }
        else
        {
            const double beatPos = (hasDaw && ppqPos >= 0.0)
                ? ppqPos
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, false);
            midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, pitch, (uint8_t)0), sampleOff);
        }
    };
    tp.onPitchBend = [&](int voice, int bendVal14, int ch0, int sampleOff)
    {
        // bendVal14: bipolar -8192..+8191; JUCE pitchWheel expects 0..16383 (8192 = centre).
        // On gate-open TriggerSystem passes bendVal14 == 0, so we also send the RPN to
        // configure the synth pitch-bend range to ±24 semitones (matching kBendSemitones).
        const int juceWheel = juce::jlimit(0, 16383, bendVal14 + 8192);
        if (bendVal14 == 0)
        {
            // Set pitch-bend range via RPN 0: ±24 semitones, then null-select RPN.
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 101,   0), sampleOff);  // RPN MSB
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 100,   0), sampleOff);  // RPN LSB = 0 (pitch bend range)
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1,   6,  24), sampleOff);  // data MSB = 24 semitones
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1,  38,   0), sampleOff);  // data LSB = 0 cents
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 101, 127), sampleOff);  // null RPN
            midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 100, 127), sampleOff);  // null RPN
        }
        midi.addEvent(juce::MidiMessage::pitchWheel(ch0 + 1, juceWheel), sampleOff);
    };
    tp.blockSize         = blockSize;
    tp.sampleRate        = sampleRate_;
    tp.bpm               = lp.bpm;
    tp.joystickX         = chordP.joystickX;
    tp.joystickY         = chordP.joystickY;
    // Compute per-block deltas for JOY gate detection.
    // When the joystick is still these are 0 regardless of absolute position.
    tp.deltaX            = chordP.joystickX - prevJoyX_;
    tp.deltaY            = chordP.joystickY - prevJoyY_;
    prevJoyX_            = chordP.joystickX;
    prevJoyY_            = chordP.joystickY;
    tp.joystickThreshold = apvts.getRawParameterValue(ParamID::joystickThreshold)->load();
    for (int v = 0; v < 4; ++v)
    {
        tp.sources[v]      = src[v];
        tp.heldPitches[v]  = heldPitch_[v];
        tp.midiChannels[v] = voiceChs[v];
    }
    tp.randomDensity      = apvts.getRawParameterValue(ParamID::randomDensity)->load();
    tp.randomGateTime     = apvts.getRawParameterValue(ParamID::randomGateTime)->load();
    tp.joystickGateTime   = apvts.getRawParameterValue("joystickGateTime")->load();
    // Per-voice random subdivisions (RandomSubdiv is a file-scope enum class in TriggerSystem.h)
    for (int v = 0; v < 4; ++v)
    {
        const int subdivIdx = static_cast<int>(
            *apvts.getRawParameterValue("randomSubdiv" + juce::String(v)));
        tp.randomSubdiv[v] = static_cast<RandomSubdiv>(subdivIdx);
    }
    // ppqPosition and isDawPlaying (from the JUCE 8 AudioPlayHead query above)
    if (isDawPlaying && ppqPos >= 0.0)
    {
        tp.ppqPosition  = ppqPos;
        tp.isDawPlaying = true;
    }
    else
    {
        tp.ppqPosition  = -1.0;
        tp.isDawPlaying = false;
    }
    tp.randomClockSync = (*apvts.getRawParameterValue("randomClockSync") > 0.5f);
    tp.randomFreeTempo = *apvts.getRawParameterValue("randomFreeTempo");

    // When arp is on, suppress joystick/random auto-triggers so arp has full
    // sequencing control. Pads (TouchPlate) still fire live notes and choke arp.
    if (arpOn)
        for (int v = 0; v < 4; ++v)
            tp.sources[v] = TriggerSource::TouchPlate;

    trigger_.processBlock(tp);

    // ── Arpeggiator ───────────────────────────────────────────────────────────
    // Arm-and-wait logic:
    //   DAW sync ON:  ARP ON while DAW rolling → arm; DAW play pressed → launch
    //   DAW sync OFF: ARP ON while looper playing → arm; looper PLAY pressed → launch
    {
        const bool arpSyncOn     = looper_.isSyncToDaw();
        // DAW sync OFF: either the looper OR the DAW transport can drive the arp.
        // This preserves the original behaviour (arp fires when DAW plays) while
        // also supporting the new looper-sync flow.
        const bool clockRunning  = arpSyncOn ? isDawPlaying : (looper_.isPlaying() || isDawPlaying);
        const bool clockStarted  = arpSyncOn ? dawJustStarted : (looperJustStarted || dawJustStarted);
        if (arpOn && !prevArpOn_ && clockRunning)
            arpWaitingForPlay_ = true;   // just enabled while clock is rolling — arm
        if (!arpOn)
            arpWaitingForPlay_ = false;  // disabled — reset arm state
        if (clockStarted)
            arpWaitingForPlay_ = false;  // clock just started — launch
    }
    prevArpOn_ = arpOn;

    // Arp requires its clock source to be running AND must not be in the armed-waiting state.
    // DAW sync ON  → DAW transport is the only clock.
    // DAW sync OFF → looper OR DAW transport can run the arp (looper beat takes priority
    //                for phase locking when both are active; falls back to free-run otherwise).
    const bool arpSyncOn  = looper_.isSyncToDaw();
    const bool arpClockOn = arpSyncOn ? isDawPlaying : (looper_.isPlaying() || isDawPlaying);
    if (!arpOn || !arpClockOn || arpWaitingForPlay_)
    {
        // Kill any hanging arp note when ARP is off or DAW stops.
        if (arpActivePitch_ >= 0 && arpActiveVoice_ >= 0)
        {
            const int ch0 = voiceChs[arpActiveVoice_] - 1;
            midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, arpActivePitch_, (uint8_t)0), 0);
            arpActivePitch_ = -1;
            arpActiveVoice_ = -1;
        }
        arpCurrentVoice_.store(-1, std::memory_order_relaxed);
        arpNoteOffRemaining_ = 0.0;
        arpStep_  = 0;
        arpPhase_ = 0.0;
    }
    else
    {
        // Subdivision interval in beats (1 beat = 1 quarter note).
        // Indices: 0=1/4, 1=1/8T, 2=1/8, 3=1/16T, 4=1/16, 5=1/32
        static const double kSubdivBeats[] = { 1.0, 1.0/3.0, 0.5, 1.0/6.0, 0.25, 0.125 };
        const int subdivIdx = juce::jlimit(0, 5,
            static_cast<int>(*apvts.getRawParameterValue(ParamID::arpSubdiv)));
        const double subdivBeats = kSubdivBeats[subdivIdx];

        const int orderIdx = juce::jlimit(0, 6,
            static_cast<int>(*apvts.getRawParameterValue(ParamID::arpOrder)));

        const double gateRatio = juce::jlimit(0.05, 1.0,
            static_cast<double>(apvts.getRawParameterValue("arpGateTime")->load()) / 100.0);
        const double gateBeats = subdivBeats * gateRatio;

        const double beatsThisBlock = lp.bpm * static_cast<double>(blockSize)
                                      / (sampleRate_ * 60.0);

        // Gate-time note-off: fires mid-block when gate expires before next step.
        if (arpActivePitch_ >= 0 && arpNoteOffRemaining_ > 0.0)
        {
            arpNoteOffRemaining_ -= beatsThisBlock;
            if (arpNoteOffRemaining_ <= 0.0)
            {
                const int ch0 = voiceChs[arpActiveVoice_] - 1;
                midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, arpActivePitch_, (uint8_t)0), 0);
                arpActivePitch_ = -1;
                arpActiveVoice_ = -1;
                arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                arpNoteOffRemaining_ = 0.0;
            }
        }

        // Count how many subdivision steps fire in this block.
        // DAW sync ON:  derive from PPQ position so steps lock to the beat grid.
        // DAW sync OFF + looper playing: derive from looper's internal beat position.
        // Otherwise: free-running phase accumulator.
        int stepsToFire = 0;
        if (arpSyncOn && ppqPos >= 0.0)
        {
            // Lock arp steps to DAW beat grid.
            const auto stepsAtStart = static_cast<long long>(ppqPos / subdivBeats);
            const auto stepsAtEnd   = static_cast<long long>((ppqPos + beatsThisBlock) / subdivBeats);
            stepsToFire = std::max(0, static_cast<int>(stepsAtEnd - stepsAtStart));
            arpPhase_ = std::fmod(ppqPos, subdivBeats);  // keep coherent
        }
        else if (!arpSyncOn && looper_.isPlaying())
        {
            // Lock arp steps to looper's internal beat position.
            // Guard against loop-wrap causing a negative difference (fire 0 steps on wrap frame).
            const double looperBeat = looper_.getPlaybackBeat();
            const auto stepsAtStart = static_cast<long long>(looperBeat / subdivBeats);
            const auto stepsAtEnd   = static_cast<long long>((looperBeat + beatsThisBlock) / subdivBeats);
            stepsToFire = std::max(0, static_cast<int>(stepsAtEnd - stepsAtStart));
            arpPhase_ = std::fmod(looperBeat, subdivBeats);  // keep coherent
        }
        else
        {
            arpPhase_ += beatsThisBlock;
            while (arpPhase_ >= subdivBeats) { arpPhase_ -= subdivBeats; ++stepsToFire; }
        }

        for (int s = 0; s < stepsToFire; ++s)
        {
            // Cut any still-sounding note at step boundary.
            if (arpActivePitch_ >= 0 && arpActiveVoice_ >= 0)
            {
                const int ch0 = voiceChs[arpActiveVoice_] - 1;
                midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, arpActivePitch_, (uint8_t)0), 0);
                arpActivePitch_ = -1;
                arpActiveVoice_ = -1;
                arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                arpNoteOffRemaining_ = 0.0;
            }

            // Build fixed-voice step sequence (all 4 voices, stack-allocated max 8).
            int seq[8]; int seqLen = 0;

            // 0=Up, 1=Down, 2=Up+Down, 3=Down+Up, 4=Outer-In, 5=Inner-Out, 6=Random
            switch (orderIdx)
            {
                case 0: seq[0]=0; seq[1]=1; seq[2]=2; seq[3]=3; seqLen=4; break;
                case 1: seq[0]=3; seq[1]=2; seq[2]=1; seq[3]=0; seqLen=4; break;
                case 2: seq[0]=0; seq[1]=1; seq[2]=2; seq[3]=3; seq[4]=2; seq[5]=1; seqLen=6; break;
                case 3: seq[0]=3; seq[1]=2; seq[2]=1; seq[3]=0; seq[4]=1; seq[5]=2; seqLen=6; break;
                case 4: seq[0]=0; seq[1]=3; seq[2]=1; seq[3]=2; seqLen=4; break;
                case 5: seq[0]=1; seq[1]=2; seq[2]=0; seq[3]=3; seqLen=4; break;
                case 6:
                default:
                {
                    if (arpStep_ == 0)
                    {
                        for (int i = 3; i > 0; --i)
                        {
                            arpRandSeed_ = arpRandSeed_ * 1664525u + 1013904223u;
                            const int j = static_cast<int>((arpRandSeed_ >> 16) % (uint32_t)(i + 1));
                            std::swap(arpRandomOrder_[i], arpRandomOrder_[j]);
                        }
                    }
                    seq[0]=arpRandomOrder_[0]; seq[1]=arpRandomOrder_[1];
                    seq[2]=arpRandomOrder_[2]; seq[3]=arpRandomOrder_[3]; seqLen=4; break;
                }
            }

            if (seqLen == 0) continue;
            if (arpStep_ >= seqLen) arpStep_ = 0;

            const int voice = seq[arpStep_];
            arpStep_ = (arpStep_ + 1) % seqLen;

            // Skip voices where a pad is held (pad has priority).
            if (trigger_.isGateOpen(voice)) continue;

            const int pitch = heldPitch_[voice];
            const int ch0   = voiceChs[voice] - 1;
            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100), 0);
            arpActivePitch_  = pitch;
            arpActiveVoice_  = voice;
            arpCurrentVoice_.store(voice, std::memory_order_relaxed);
            arpNoteOffRemaining_ = gateBeats;
        }
    }

    // Joystick-move trigger for "start rec by touch".
    // Note-on path already calls activateRecordingNow() inside tp.onNote.
    if (std::abs(tp.deltaX) > tp.joystickThreshold || std::abs(tp.deltaY) > tp.joystickThreshold)
        looper_.activateRecordingNow();  // no-op if not wait-armed

    // ── Looper joystick + filter recording ───────────────────────────────────
    {
        const double beatPos = (isDawPlaying && ppqPos >= 0.0)
            ? ppqPos : looper_.getPlaybackBeat();
        looper_.recordJoystick(beatPos, chordP.joystickX, chordP.joystickY);

        // Record filter (left stick) into looper when REC FILTER is armed.
        if (gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed))
            looper_.recordFilter(beatPos, gamepad_.getFilterX(), gamepad_.getFilterY());
    }

    // ── Filter CC ─────────────────────────────────────────────────────────────
    // Sources (in priority order):
    //   1. Looper playback  (loopOut.hasFilterX/Y — works without gamepad)
    //   2. Live gamepad left stick
    // Atten knobs scale range but do NOT emit CC on their own (joyMoved guard).
    // CC reset on disconnect via pendingCcReset_ (set from message thread).
    {
        const int fCh = (int)apvts.getRawParameterValue(ParamID::filterMidiCh)->load();

        // Handle disconnect events.
        if (pendingAllNotesOff_.exchange(false, std::memory_order_acq_rel))
        {
            for (int v = 0; v < 4; ++v)
                midi.addEvent(juce::MidiMessage::allNotesOff(voiceChs[v]), 0);
        }

        if (pendingCcReset_.exchange(false, std::memory_order_acq_rel))
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 12, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh,  1, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 76, 0), 0);
            prevCcCut_.store(0, std::memory_order_relaxed);
            prevCcRes_.store(0, std::memory_order_relaxed);
        }

        // Determine filter source — all gated by the filterMod on/off toggle.
        const bool filterModOn   = filterModActive_.load(std::memory_order_relaxed);
        const bool filterRecOn      = looper_.isRecFilter();     // REC FILTER button state
        const bool filterHasContent = looper_.hasFilterContent(); // any FilterX/Y events recorded
        const bool looperDriving = filterModOn && filterRecOn && filterHasContent
                                   && (loopOut.hasFilterX || loopOut.hasFilterY);
        const bool liveGamepad   = filterModOn && gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed);

        // Base knobs are the primary modulator and send CC independently of the joystick.
        // Joystick/looper updates are still gated by liveGamepad||looperDriving, but
        // the outer block is gated only by filterModOn so base-knob turns always reach here.
        if (filterModOn)
        {
            const float xAtten = apvts.getRawParameterValue(ParamID::filterXAtten)->load();
            const float yAtten = apvts.getRawParameterValue(ParamID::filterYAtten)->load();

            const int xMode  = (int)apvts.getRawParameterValue("filterXMode")->load();
            const int yMode  = (int)apvts.getRawParameterValue("filterYMode")->load();
            const int ccXnum = (xMode == 1) ? 12 : (xMode == 2) ? 1 : 74;
            const int ccYnum = (yMode == 1) ? 76 : 71;

            if (xMode != prevXMode_) { prevCcCut_.store(-1, std::memory_order_relaxed); prevXMode_ = xMode; }
            if (yMode != prevYMode_) { prevCcRes_.store(-1, std::memory_order_relaxed); prevYMode_ = yMode; }

            // ── Stick / looper position update (gated as before) ─────────────
            // Priority rules:
            //   1. Looper event this block  → use it
            //   2. Live stick               → use it
            //   3. Hold prevFilterX_        → between looper events
            bool stickUpdated = false;
            if (looperDriving || liveGamepad)
            {
                const bool looperPlaying = looper_.isPlaying();
                const bool recActive     = looper_.isRecording();
                const float newGfx = (filterRecOn && !recActive && filterHasContent && loopOut.hasFilterX) ? loopOut.filterX
                                   : ((liveGamepad && (!looperPlaying || !filterRecOn || recActive || !filterHasContent)) ? gamepad_.getFilterX() : prevFilterX_);
                const float newGfy = (filterRecOn && !recActive && filterHasContent && loopOut.hasFilterY) ? loopOut.filterY
                                   : ((liveGamepad && (!looperPlaying || !filterRecOn || recActive || !filterHasContent)) ? gamepad_.getFilterY() : prevFilterY_);

                const bool looperUpdate = filterRecOn && !recActive && filterHasContent
                                          && (loopOut.hasFilterX || loopOut.hasFilterY);
                const float fThresh = apvts.getRawParameterValue("joystickThreshold")->load();
                stickUpdated = looperUpdate
                    || (std::abs(newGfx - prevFilterX_) > fThresh)
                    || (std::abs(newGfy - prevFilterY_) > fThresh);

                if (stickUpdated)
                {
                    prevFilterX_ = newGfx;
                    prevFilterY_ = newGfy;
                }
            }

            // ── Base knob change detection (primary modulator) ───────────────
            const float xOffset = apvts.getRawParameterValue(ParamID::filterXOffset)->load();
            const float yOffset = apvts.getRawParameterValue(ParamID::filterYOffset)->load();
            const bool baseChanged = (xOffset != prevBaseX_) || (yOffset != prevBaseY_);

            if (stickUpdated || baseChanged)
            {
                prevBaseX_ = xOffset;
                prevBaseY_ = yOffset;

                const int ccCut = juce::jlimit(0, 127, (int)std::roundf(xOffset + prevFilterX_ * 63.5f * (xAtten / 100.0f)));
                const int ccRes = juce::jlimit(0, 127, (int)std::roundf(yOffset + prevFilterY_ * 63.5f * (yAtten / 100.0f)));

                {
                    const int prev = prevCcCut_.load(std::memory_order_relaxed);
                    if (prev == -2)
                        prevCcCut_.store(ccCut, std::memory_order_relaxed);  // on-load: silent init
                    else if (prev == -1 || ccCut != prev)
                    {
                        midi.addEvent(juce::MidiMessage::controllerEvent(fCh, ccXnum, ccCut), 0);
                        prevCcCut_.store(ccCut, std::memory_order_relaxed);
                    }
                }
                {
                    const int prev = prevCcRes_.load(std::memory_order_relaxed);
                    if (prev == -2)
                        prevCcRes_.store(ccRes, std::memory_order_relaxed);  // on-load: silent init
                    else if (prev == -1 || ccRes != prev)
                    {
                        midi.addEvent(juce::MidiMessage::controllerEvent(fCh, ccYnum, ccRes), 0);
                        prevCcRes_.store(ccRes, std::memory_order_relaxed);
                    }
                }
                filterCutDisplay_.store(static_cast<float>(ccCut), std::memory_order_relaxed);
                filterResDisplay_.store(static_cast<float>(ccRes), std::memory_order_relaxed);
            }
        }
    }

    // ── Looper config sync ────────────────────────────────────────────────────
    looper_.setSubdiv(static_cast<LooperSubdiv>(
        (int)apvts.getRawParameterValue(ParamID::looperSubdiv)->load()));
    looper_.setLoopLengthBars(
        (int)apvts.getRawParameterValue(ParamID::looperLength)->load());

    // Push quantize config to LooperEngine each block (cheap atomic stores)
    {
        const int qMode   = static_cast<int>(*apvts.getRawParameterValue(ParamID::quantizeMode));
        const int qSubdiv = static_cast<int>(*apvts.getRawParameterValue(ParamID::quantizeSubdiv));
        looper_.setQuantizeMode(qMode);
        looper_.setQuantizeSubdiv(qSubdiv);
    }
}

// ─── State persistence ────────────────────────────────────────────────────────

void PluginProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ─── Factory ──────────────────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}
