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
    static const juce::String randomSubdiv0    = "randomSubdiv0";
    static const juce::String randomSubdiv1    = "randomSubdiv1";
    static const juce::String randomSubdiv2    = "randomSubdiv2";
    static const juce::String randomSubdiv3    = "randomSubdiv3";
    static const juce::String randomPopulation  = "randomPopulation";
    static const juce::String randomProbability = "randomProbability";
    static const juce::String gateLength        = "gateLength";

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

    // Single-Channel Routing
    static constexpr const char* singleChanMode   = "singleChanMode";
    static constexpr const char* singleChanTarget = "singleChanTarget";

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
    juce::StringArray trigSrcNames { "TouchPlate", "Joystick", "Random Free", "Random Hold" };
    addChoice(ParamID::triggerSource0, "Trigger Source Root",    trigSrcNames, 0);
    addChoice(ParamID::triggerSource1, "Trigger Source Third",   trigSrcNames, 0);
    addChoice(ParamID::triggerSource2, "Trigger Source Fifth",   trigSrcNames, 0);
    addChoice(ParamID::triggerSource3, "Trigger Source Tension", trigSrcNames, 0);
    addFloat("randomPopulation",  "Random Population",  1.0f, 64.0f, 8.0f);
    addFloat("randomProbability", "Random Probability", 0.0f, 1.0f,  1.0f);
    addFloat("gateLength",        "Gate Length",        0.0f, 1.0f,  0.5f);
    {
        const juce::StringArray subdivChoices { "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv0", "Random Subdiv Root",    subdivChoices, 5));  // default = 1/8
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv1", "Random Subdiv Third",   subdivChoices, 5));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv2", "Random Subdiv Fifth",   subdivChoices, 5));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv3", "Random Subdiv Tension", subdivChoices, 5));
    }

    // Random clock mode
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "randomClockSync", "Random Clock Sync", false));

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
        ParamID::filterXOffset, "MOD FIX X",
        juce::NormalisableRange<float>(-50.0f, 50.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::filterYOffset, "MOD FIX Y",
        juce::NormalisableRange<float>(-50.0f, 50.0f, 1.0f), 0.0f));
    addInt  (ParamID::filterMidiCh, "Filter MIDI Channel",  1, 16, 1);
    {
        juce::StringArray xModes { "Cutoff (CC74)", "VCF LFO (CC12)", "LFO-X Freq",
                                    "LFO-X Phase",  "LFO-X Level",    "Gate Length" };
        juce::StringArray yModes { "Resonance (CC71)", "LFO Rate (CC76)", "LFO-Y Freq",
                                    "LFO-Y Phase",     "LFO-Y Level",     "Gate Length" };
        addChoice("filterXMode", "Left Stick X Mode", xModes, 0);  // 0=CC74, 1=CC12, 2=LFO-X Freq, 3=Phase, 4=Level, 5=Gate
        addChoice("filterYMode", "Left Stick Y Mode", yModes, 0);  // 0=CC71, 1=CC76, 2=LFO-Y Freq, 3=Phase, 4=Level, 5=Gate
    }

    // ── MIDI routing ──────────────────────────────────────────────────────────
    addInt(ParamID::voiceCh0, "Root MIDI Channel",    1, 16, 1);
    addInt(ParamID::voiceCh1, "Third MIDI Channel",   1, 16, 2);
    addInt(ParamID::voiceCh2, "Fifth MIDI Channel",   1, 16, 3);
    addInt(ParamID::voiceCh3, "Tension MIDI Channel", 1, 16, 4);
    {
        juce::StringArray routingModes { "Multi-Channel", "Single Channel" };
        addChoice("singleChanMode",   "Routing Mode",          routingModes, 1);
        addInt   ("singleChanTarget", "Single Channel Target",  1, 16, 1);
    }

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
    // arpGateTime removed in Phase 20; unified gateLength param (0.0–1.0) registered in Trigger section above

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
    addFloat(ParamID::lfoXLevel,      "LFO X Level",      0.0f, 1.0f, 0.0f);
    addFloat(ParamID::lfoYLevel,      "LFO Y Level",      0.0f, 1.0f, 0.0f);
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
    {
        const juce::StringArray ccDests { "Off","CC1 \xe2\x80\x93 Mod Wheel","CC2 \xe2\x80\x93 Breath",
            "CC5 \xe2\x80\x93 Portamento","CC7 \xe2\x80\x93 Volume","CC10 \xe2\x80\x93 Pan",
            "CC11 \xe2\x80\x93 Expression","CC12 \xe2\x80\x93 VCF LFO",
            "CC16 \xe2\x80\x93 GenPurp 1","CC17 \xe2\x80\x93 GenPurp 2",
            "CC71 \xe2\x80\x93 Resonance","CC72 \xe2\x80\x93 Release",
            "CC73 \xe2\x80\x93 Attack","CC74 \xe2\x80\x93 Filter Cut","CC75 \xe2\x80\x93 Decay",
            "CC76 \xe2\x80\x93 Vibrato Rate","CC77 \xe2\x80\x93 Vibrato Depth",
            "CC91 \xe2\x80\x93 Reverb","CC93 \xe2\x80\x93 Chorus" };
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoXCcDest","LFO X CC Dest",ccDests,0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoYCcDest","LFO Y CC Dest",ccDests,0));
    }

    // ── Sub Octave (Phase 19) ─────────────────────────────────────────────────
    addBool("subOct0", "Sub Oct Root",    false);
    addBool("subOct1", "Sub Oct Third",   false);
    addBool("subOct2", "Sub Oct Fifth",   false);
    addBool("subOct3", "Sub Oct Tension", false);

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
    smoothedGateLength_.reset(sampleRate_, 0.015);  // 15ms — removes zipper noise without sluggish feel
    smoothedGateLength_.setCurrentAndTargetValue(
        apvts.getRawParameterValue("gateLength")->load());
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

    for (int v = 0; v < 4; ++v)
    {
        // Use sentChannel_ snapshot so note-off matches the channel used at note-on.
        // Fall back to effectiveChannel logic if sentChannel_ was never set (default 1-4).
        const int ch = sentChannel_[v];

        const int pitch = trigger_.getActivePitch(v);
        if (pitch >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), 0);
    }
    trigger_.resetAllGates();
    resetNoteCount();
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

// ─── resetNoteCount ───────────────────────────────────────────────────────────

void PluginProcessor::resetNoteCount() noexcept
{
    std::fill(&noteCount_[0][0], &noteCount_[0][0] + 16 * 128, 0);

    // Also reset sub-octave snapshot arrays so stale sub pitches don't cause
    // double note-on on re-trigger after allNotesOff / DAW stop / panic.
    for (int v = 0; v < 4; ++v)
    {
        subHeldPitch_[v]         = -1;
        looperActiveSubPitch_[v] = -1;
        subOctSounding_[v].store(false, std::memory_order_relaxed);
    }
}

// ─── processBlock helpers ─────────────────────────────────────────────────────

static int ccDestToNumber(int idx)
{
    static const int t[] = {-1,1,2,5,7,10,11,12,16,17,71,72,73,74,75,76,77,91,93};
    return (idx >= 0 && idx < (int)std::size(t)) ? t[idx] : -1;
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

        // D-pad: mode 0=BPM/looper, mode 1=octaves (green), mode 2=transpose+intervals (red)
        const int optMode = gamepad_.getOptionMode();

        // Gate looper face-button consume: in Mode 1, face buttons are arp dispatch (not looper)
        if (optMode != 1)
        {
            if (gamepad_.consumeLooperStartStop()) looper_.startStop();
            if (gamepad_.consumeLooperRecord())    looper_.record();
            if (gamepad_.consumeLooperReset())   { looper_.reset();       flashLoopReset_.fetch_add(1,  std::memory_order_relaxed); }
            if (gamepad_.consumeLooperDelete())  { looper_.deleteLoop();  flashLoopDelete_.fetch_add(1, std::memory_order_relaxed); }
        }
        if (gamepad_.consumeRightStickTrigger())
        {
            // R3 + held pad (L1/L2/R1/R2): toggle subOctN for that voice.
            // R3 alone: no action. Panic is UI-button only (Phase 19 Decision 3).
            bool anyHeld = false;
            for (int v = 0; v < 4; ++v)
            {
                if (gamepad_.getVoiceHeld(v))
                {
                    anyHeld = true;
                    const juce::String paramID = "subOct" + juce::String(v);
                    auto* param = dynamic_cast<juce::AudioParameterBool*>(
                        apvts.getParameter(paramID));
                    if (param != nullptr)
                        param->setValueNotifyingHost(param->get() ? 0.0f : 1.0f);
                }
            }
            (void)anyHeld;
        }

        if (optMode == 1)
        {
            // Mode 1 — octaves: up=rootOct  down=3rdOct  left=tenOct  right=5thOct
            // Clamp (not wrap) so hitting the limit is silent, not a jump.
            // Range matches the APVTS declaration (0..12 absolute MIDI octave).
            { const int d = gamepad_.consumeOptionDpadDelta(0); if (d) stepClampingParam(apvts, "rootOctave",    0, 12, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(1); if (d) stepClampingParam(apvts, "thirdOctave",   0, 12, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(2); if (d) stepClampingParam(apvts, "tensionOctave", 0, 12, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(3); if (d) stepClampingParam(apvts, "fifthOctave",   0, 12, d); }

            // Mode 1 face-button arp dispatch (OPT1-01 through OPT1-04)
            // Circle → toggle arpEnabled; Triangle → step arpSubdiv; Square → step arpOrder; Cross → toggle randomClockSync
            if (gamepad_.consumeArpCircle())
            {
                auto* arpParam = dynamic_cast<juce::AudioParameterBool*>(
                    apvts.getParameter("arpEnabled"));
                if (arpParam != nullptr)
                {
                    const bool wasOn = arpParam->get();
                    arpParam->setValueNotifyingHost(wasOn ? 0.0f : 1.0f);
                    if (!wasOn)  // just turned ON
                    {
                        // DAW sync active: existing arpWaitingForPlay_ arm logic fires automatically
                        // when arpOn reads true at the arp block — no extra action needed here.
                        // Looper stopped (non-DAW-sync): reset to beat 0 then start playback.
                        if (!looper_.isSyncToDaw() && !looper_.isPlaying())
                        {
                            looper_.reset();
                            looper_.startStop();
                            // Note: both calls happen BEFORE looper_.process() call later in this block.
                            // looper_.process() integrates both commands atomically.
                        }
                        // If looper already playing: no looper side-effects.
                    }
                }
            }
            { const int d = gamepad_.consumeArpRateDelta();  if (d) stepWrappingParam(apvts, "arpSubdiv", 0, 5, d); }
            { const int d = gamepad_.consumeArpOrderDelta(); if (d) stepWrappingParam(apvts, "arpOrder",  0, 6, d); }
            if (gamepad_.consumeRndSyncToggle())
            {
                auto* rndParam = dynamic_cast<juce::AudioParameterBool*>(
                    apvts.getParameter("randomClockSync"));
                if (rndParam != nullptr)
                    rndParam->setValueNotifyingHost(rndParam->get() ? 0.0f : 1.0f);
            }
        }
        else if (optMode == 2)
        {
            // Mode 2 — transpose+intervals: up=transpose  down=3rdInt  left=tenInt  right=5thInt
            { const int d = gamepad_.consumeOptionDpadDelta(0); if (d) stepWrappingParam(apvts, "globalTranspose",   0, 11, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(1); if (d) stepWrappingParam(apvts, "thirdInterval",     0, 12, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(2); if (d) stepWrappingParam(apvts, "tensionInterval",   0, 12, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(3); if (d) stepWrappingParam(apvts, "fifthInterval",     0, 12, d); }
        }
        else
        {
            // Mode 0 — normal: up/down=BPM, left/right=looper rec state
            if (gamepad_.consumeDpadUp())    pendingBpmDelta_.fetch_add(1,  std::memory_order_relaxed);
            if (gamepad_.consumeDpadDown())  pendingBpmDelta_.fetch_add(-1, std::memory_order_relaxed);
            if (gamepad_.consumeDpadLeft())  looperSetRecGates(!looperIsRecGates());
            if (gamepad_.consumeDpadRight()) looperSetRecJoy(!looperIsRecJoy());
        }
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
    int    timeSigNumer = 4;           // beats per bar — default 4/4 (ARP-05)
    if (auto* head = getPlayHead())
    {
        if (auto posOpt = head->getPosition(); posOpt.hasValue())
        {
            isDawPlaying = posOpt->getIsPlaying();
            if (auto ppq = posOpt->getPpqPosition(); ppq.hasValue())
                ppqPos = *ppq;
            if (auto bpmOpt = posOpt->getBpm(); bpmOpt.hasValue())
                dawBpm = *bpmOpt;
            if (auto sig = posOpt->getTimeSignature(); sig.hasValue())  // ARP-05
                timeSigNumer = sig->numerator;                           // ARP-05
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

    // ── LFO Recording state machine — edge detection (Phase 22) ──────────────
    {
        const bool looperNowRecording = looper_.isRecording();
        const bool looperNowPlaying   = looper_.isPlaying();

        // Rising edge: looper just started recording → start capture on armed LFOs
        if (!prevLooperRecording_ && looperNowRecording)
        {
            if (lfoX_.getRecState() == LfoRecState::Armed)
                lfoX_.startCapture();
            if (lfoY_.getRecState() == LfoRecState::Armed)
                lfoY_.startCapture();
        }
        // Falling edge: looper just stopped recording → push to Playback
        else if (prevLooperRecording_ && !looperNowRecording)
        {
            if (lfoX_.getRecState() == LfoRecState::Recording)
                lfoX_.stopCapture();
            if (lfoY_.getRecState() == LfoRecState::Recording)
                lfoY_.stopCapture();
        }

        // Immediate capture: if ARM is pressed while looper is already in playback
        // (not recording), start capture on the very next processBlock rather than
        // waiting for a new record cycle.
        if (looperNowPlaying && !looperNowRecording)
        {
            if (lfoX_.getRecState() == LfoRecState::Armed)
                lfoX_.startCapture();
            if (lfoY_.getRecState() == LfoRecState::Armed)
                lfoY_.startCapture();
        }

        prevLooperRecording_ = looperNowRecording;

        // Looper Clear (deleteLoop or reset) → return LFO to Unarmed live mode
        if (loopOut.looperReset)
        {
            lfoX_.clearRecording();
            lfoY_.clearRecording();
        }
    }

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
            xp.rateHz       = (lfoXRateOverride_  >= 0.0f) ? lfoXRateOverride_
                                                           : *apvts.getRawParameterValue(ParamID::lfoXRate);
            {
                const bool xSyncOn  = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
                const int  xCurMode = (int)apvts.getRawParameterValue("filterXMode")->load();
                xp.subdivBeats = kLfoSubdivBeats[
                    juce::jlimit(0, 5, (int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv))];
                if (xSyncOn && xCurMode == 2)
                    xp.subdivBeats *= lfoXSubdivMult_.load(std::memory_order_relaxed);
            }
            xp.maxCycleBeats = 16.0;
            xp.waveform     = static_cast<Waveform>(
                (int)*apvts.getRawParameterValue(ParamID::lfoXWaveform));
            xp.phaseShift   = ((lfoXPhaseOverride_ >= 0.0f) ? lfoXPhaseOverride_
                                                             : *apvts.getRawParameterValue(ParamID::lfoXPhase))
                              / 360.0f;
            xp.distortion   = *apvts.getRawParameterValue(ParamID::lfoXDistortion);
            xp.level        = (lfoXLevelOverride_ >= 0.0f) ? lfoXLevelOverride_
                                                            : xLevel;
            // Inject looper playback position for LFO recording playback sync.
            // Only meaningful when recState == Playback; ignored in all other states.
            {
                const double loopLenBeats = looper_.getLoopLengthBeats();
                xp.playbackPhase = (loopLenBeats > 0.0)
                    ? static_cast<float>(std::fmod(looper_.getPlaybackBeat() / loopLenBeats, 1.0))
                    : 0.0f;
            }
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
            yp.rateHz       = (lfoYRateOverride_  >= 0.0f) ? lfoYRateOverride_
                                                           : *apvts.getRawParameterValue(ParamID::lfoYRate);
            {
                const bool ySyncOn  = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
                const int  yCurMode = (int)apvts.getRawParameterValue("filterYMode")->load();
                yp.subdivBeats = kLfoSubdivBeats[
                    juce::jlimit(0, 5, (int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv))];
                if (ySyncOn && yCurMode == 2)
                    yp.subdivBeats *= lfoYSubdivMult_.load(std::memory_order_relaxed);
            }
            yp.maxCycleBeats = 16.0;
            yp.waveform     = static_cast<Waveform>(
                (int)*apvts.getRawParameterValue(ParamID::lfoYWaveform));
            yp.phaseShift   = ((lfoYPhaseOverride_ >= 0.0f) ? lfoYPhaseOverride_
                                                             : *apvts.getRawParameterValue(ParamID::lfoYPhase))
                              / 360.0f;
            yp.distortion   = *apvts.getRawParameterValue(ParamID::lfoYDistortion);
            yp.level        = (lfoYLevelOverride_ >= 0.0f) ? lfoYLevelOverride_
                                                            : yLevel;
            // Inject looper playback position for LFO recording playback sync.
            // Only meaningful when recState == Playback; ignored in all other states.
            {
                const double loopLenBeats = looper_.getLoopLengthBeats();
                yp.playbackPhase = (loopLenBeats > 0.0)
                    ? static_cast<float>(std::fmod(looper_.getPlaybackBeat() / loopLenBeats, 1.0))
                    : 0.0f;
            }
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

    const bool singleMode   = (*apvts.getRawParameterValue("singleChanMode") > 0.5f);
    const int  singleTarget = (int)*apvts.getRawParameterValue("singleChanTarget");

    auto effectiveChannel = [&](int v) -> int {
        return singleMode ? singleTarget : voiceChs[v];
    };
    auto effectiveFilterChannel = [&]() -> int {
        return singleMode ? singleTarget
                          : (int)apvts.getRawParameterValue("filterMidiCh")->load();
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
            // Explicit per-pitch note-offs for ALL tracked sources before the generic sweep.
            // resetNoteCount() zeroes counts, so these must fire BEFORE that call to ensure
            // the looper stop detection (which runs after us) still sees valid counts.
            for (int v = 0; v < 4; ++v)
            {
                const int pitch = trigger_.getActivePitch(v);
                if (pitch >= 0)
                    midi.addEvent(juce::MidiMessage::noteOff(sentChannel_[v], pitch, (uint8_t)0), 0);
                if (subHeldPitch_[v] >= 0)
                    midi.addEvent(juce::MidiMessage::noteOff(sentChannel_[v], subHeldPitch_[v], (uint8_t)0), 0);
                // Looper notes: send explicit note-off here so they're covered even if
                // the looper stop detection's noteCount guard fires after the reset.
                if (looperActivePitch_[v] >= 0)
                    midi.addEvent(juce::MidiMessage::noteOff(looperActiveCh_[v], looperActivePitch_[v], (uint8_t)0), 0);
                if (looperActiveSubPitch_[v] >= 0)
                    midi.addEvent(juce::MidiMessage::noteOff(looperActiveCh_[v], looperActiveSubPitch_[v], (uint8_t)0), 0);
            }
            if (arpActivePitch_ >= 0)
                midi.addEvent(juce::MidiMessage::noteOff(arpActiveCh_, arpActivePitch_, (uint8_t)0), 0);
            for (int ch = 1; ch <= 16; ++ch)
            {
                midi.addEvent(juce::MidiMessage::controllerEvent(ch, 120, 0), 0);  // all sound off
                midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
            }
            trigger_.resetAllGates();
            resetNoteCount();
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
                    const int ch = looperActiveCh_[v];
                    if (noteCount_[ch - 1][looperActivePitch_[v]] > 0 &&
                        --noteCount_[ch - 1][looperActivePitch_[v]] == 0)
                        midi.addEvent(juce::MidiMessage::noteOff(ch, looperActivePitch_[v], (uint8_t)0), 0);
                    else
                        noteCount_[ch - 1][looperActivePitch_[v]] = 0;
                    looperActivePitch_[v] = -1;
                }
                // Looper sub-octave note-off on looper stop
                const int subPitch = looperActiveSubPitch_[v];
                if (subPitch >= 0)
                {
                    const int ch = looperActiveCh_[v];
                    if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                        midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
                    else
                        noteCount_[ch - 1][subPitch] = 0;
                }
            }
            resetNoteCount();
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
                const int ch = looperActiveCh_[v];
                if (noteCount_[ch - 1][looperActivePitch_[v]] > 0 &&
                    --noteCount_[ch - 1][looperActivePitch_[v]] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, looperActivePitch_[v], (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][looperActivePitch_[v]] = 0;
                looperActivePitch_[v] = -1;
            }
            // Looper sub-octave note-off on looper reset
            const int subPitch = looperActiveSubPitch_[v];
            if (subPitch >= 0)
            {
                const int ch = looperActiveCh_[v];
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][subPitch] = 0;
            }
        }
        resetNoteCount();
    }

    // ── MIDI Panic / Mute ─────────────────────────────────────────────────────
    // pendingPanic_ fires unconditionally: stops looper, sends allNotesOff, resets gates.
    // This happens BEFORE the mute gate so allNotesOff always reaches the synth.
    if (pendingPanic_.exchange(false, std::memory_order_acq_rel))
    {
        if (looper_.isPlaying()) looper_.startStop();
        // Explicit per-pitch note-offs first — some synths ignore CC120/CC123 but
        // always respond to individual note-off messages. This prevents stuck notes
        // on synths that don't fully implement allNotesOff / allSoundsOff.
        for (int v = 0; v < 4; ++v)
        {
            const int pitch = trigger_.getActivePitch(v);
            if (pitch >= 0)
                midi.addEvent(juce::MidiMessage::noteOff(sentChannel_[v], pitch, (uint8_t)0), 0);
            if (subHeldPitch_[v] >= 0)
                midi.addEvent(juce::MidiMessage::noteOff(sentChannel_[v], subHeldPitch_[v], (uint8_t)0), 0);
            if (looperActivePitch_[v] >= 0)
                midi.addEvent(juce::MidiMessage::noteOff(looperActiveCh_[v], looperActivePitch_[v], (uint8_t)0), 0);
            if (looperActiveSubPitch_[v] >= 0)
                midi.addEvent(juce::MidiMessage::noteOff(looperActiveCh_[v], looperActiveSubPitch_[v], (uint8_t)0), 0);
        }
        if (arpActivePitch_ >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(arpActiveCh_, arpActivePitch_, (uint8_t)0), 0);
        for (int ch = 1; ch <= 16; ++ch)
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(ch, 64,  0), 0);  // sustain off
            midi.addEvent(juce::MidiMessage::controllerEvent(ch, 120, 0), 0);  // all sound off
            midi.addEvent(juce::MidiMessage::allNotesOff(ch),             0);  // CC123 all notes off
        }
        trigger_.resetAllGates();
        looperActivePitch_.fill(-1);
        std::fill(std::begin(looperActiveSubPitch_), std::end(looperActiveSubPitch_), -1);
        arpActivePitch_ = -1;
        arpActiveVoice_ = -1;
        arpNoteOffRemaining_ = 0.0;
        resetNoteCount();
        prevCcCut_.store(-1, std::memory_order_relaxed);
        prevCcRes_.store(-1, std::memory_order_relaxed);
        flashPanic_.fetch_add(1, std::memory_order_relaxed);
        // Return immediately — panic is a hard MIDI cutoff.
        // Nothing may fire notes in the remainder of this block.
        return;
    }

    // ── Single-Channel mode/target change detection ───────────────────────────
    // Flush runs BEFORE the mute guard so allNotesOff always reaches the synth.
    {
        const bool modeChanged   = (singleMode ? 1 : 0) != prevSingleChanMode_;
        const bool targetChanged = singleMode && (singleTarget != prevSingleChanTarget_);

        if (modeChanged)
        {
            for (int ch = 1; ch <= 16; ++ch)
                midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
            trigger_.resetAllGates();
            looperActivePitch_.fill(-1);
            resetNoteCount();
            prevSingleChanMode_   = singleMode ? 1 : 0;
            prevSingleChanTarget_ = singleTarget;
        }
        else if (targetChanged)
        {
            midi.addEvent(juce::MidiMessage::allNotesOff(prevSingleChanTarget_), 0);
            std::fill(std::begin(noteCount_[prevSingleChanTarget_ - 1]),
                      std::end  (noteCount_[prevSingleChanTarget_ - 1]), 0);
            prevSingleChanTarget_ = singleTarget;
        }
    }

    // When muted: allNotesOff is already in the buffer above; block all further MIDI output.
    if (midiMuted_.load(std::memory_order_relaxed))
        return;

    // Looper gate playback: emit MIDI directly, bypassing TriggerSystem.
    // This satisfies the locked decision: live pad input passes through independently
    // because TriggerSystem is not informed of looper events — it only sees live pad state.
    for (int v = 0; v < 4; ++v)
    {
        if (loopOut.gateOn[v])
        {
            // Snapshot the pitch and channel at gate-on so gateOff uses the same values
            // even if heldPitch_[v] or the channel setting changes before the gate closes.
            looperActivePitch_[v] = heldPitch_[v];
            const int ch = effectiveChannel(v);
            looperActiveCh_[v] = ch;
            sentChannel_[v]    = ch;  // also snapshot sentChannel_ for consistency
            if (noteCount_[ch - 1][looperActivePitch_[v]]++ == 0)
                midi.addEvent(juce::MidiMessage::noteOn(ch, looperActivePitch_[v], (uint8_t)100), 0);

            // Looper sub-octave: live SUB8 param at emission time (not baked into loop)
            const bool subEnabled = (*apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
            if (subEnabled)
            {
                const int subPitch = juce::jlimit(0, 127, looperActivePitch_[v] - 12);
                looperActiveSubPitch_[v] = subPitch;
                if (noteCount_[ch - 1][subPitch]++ == 0)
                    midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), 0);
            }
            else
            {
                looperActiveSubPitch_[v] = -1;
            }
        }
        if (loopOut.gateOff[v] && looperActivePitch_[v] >= 0)
        {
            const int ch = looperActiveCh_[v];
            if (noteCount_[ch - 1][looperActivePitch_[v]] > 0 &&
                --noteCount_[ch - 1][looperActivePitch_[v]] == 0)
                midi.addEvent(juce::MidiMessage::noteOff(ch, looperActivePitch_[v], (uint8_t)0), 0);
            else
                noteCount_[ch - 1][looperActivePitch_[v]] = 0;
            looperActivePitch_[v] = -1;

            // Looper sub-octave note-off using snapshot
            const int subPitch = looperActiveSubPitch_[v];
            if (subPitch >= 0)
            {
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][subPitch] = 0;
                looperActiveSubPitch_[v] = -1;
            }
        }
    }

    // Mid-note SUB8 toggle: enable/disable sub-octave while gate is already open.
    // Runs BEFORE TriggerSystem::processBlock() so the toggle is detected before
    // any new gate events fire in this block.
    for (int v = 0; v < 4; ++v)
    {
        const bool subWanted   = (*apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
        const bool subSounding = subOctSounding_[v].load(std::memory_order_relaxed);
        const bool gateOpen    = isGateOpen(v);

        if (gateOpen && subWanted && !subSounding)
        {
            // SUB8 toggled ON while gate open — emit immediate note-on
            const int ch       = sentChannel_[v];
            const int subPitch = juce::jlimit(0, 127, heldPitch_[v] - 12);
            subHeldPitch_[v]   = subPitch;
            if (noteCount_[ch - 1][subPitch]++ == 0)
                midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), 0);
            subOctSounding_[v].store(true, std::memory_order_relaxed);
        }
        else if (subSounding && (!subWanted || !gateOpen))
        {
            // SUB8 toggled OFF or gate closed externally — emit immediate note-off
            const int ch       = sentChannel_[v];
            const int subPitch = subHeldPitch_[v];
            if (subPitch >= 0)
            {
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][subPitch] = 0;
                subHeldPitch_[v] = -1;
            }
            subOctSounding_[v].store(false, std::memory_order_relaxed);
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

    // Refresh display whenever a chord-shaping parameter changes (transpose,
    // intervals, octaves, scale) — without needing a trigger event.
    // Joystick position and LFO are intentionally excluded from the hash so
    // moving the joystick or the LFO sweeping does not update the label.
    {
        const int gt = (int)apvts.getRawParameterValue(ParamID::globalTranspose)->load();
        const int i1 = (int)apvts.getRawParameterValue(ParamID::thirdInterval)->load();
        const int i2 = (int)apvts.getRawParameterValue(ParamID::fifthInterval)->load();
        const int i3 = (int)apvts.getRawParameterValue(ParamID::tensionInterval)->load();
        const int o0 = (int)apvts.getRawParameterValue(ParamID::rootOctave)->load();
        const int o1 = (int)apvts.getRawParameterValue(ParamID::thirdOctave)->load();
        const int o2 = (int)apvts.getRawParameterValue(ParamID::fifthOctave)->load();
        const int o3 = (int)apvts.getRawParameterValue(ParamID::tensionOctave)->load();
        const int sp = (int)apvts.getRawParameterValue(ParamID::scalePreset)->load();
        const int uc = (int)apvts.getRawParameterValue(ParamID::useCustomScale)->load();
        int64_t hash =   ((int64_t)gt)
                       | ((int64_t)i1 << 4)  | ((int64_t)i2 << 8)  | ((int64_t)i3 << 12)
                       | ((int64_t)o0 << 16) | ((int64_t)o1 << 20) | ((int64_t)o2 << 24) | ((int64_t)o3 << 28)
                       | ((int64_t)sp << 32) | ((int64_t)uc << 37);
        if (uc)
        {
            int noteBits = 0;
            for (int i = 0; i < 12; ++i)
                noteBits |= ((int)apvts.getRawParameterValue("scaleNote" + juce::String(i))->load() << i);
            hash |= ((int64_t)noteBits << 38);
        }
        if (hash != lastChordParamHash_)
        {
            lastChordParamHash_ = hash;
            displayPitches_ = heldPitch_;
        }
    }

    const bool arpOn = (*apvts.getRawParameterValue(ParamID::arpEnabled) > 0.5f);

    bool anyNoteOnThisBlock = false;

    TriggerSystem::ProcessParams tp;
    tp.onNote = [&](int voice, int pitch, bool isOn, int sampleOff)
    {
        if (isOn)
        {
            anyNoteOnThisBlock = true;
            voiceTriggerFlash_[voice].fetch_add(1, std::memory_order_relaxed);

            // Snapshot the chord for the display label on deliberate triggers
            // (touchplate, gamepad button, random gate) but NOT on continuous
            // joystick-position changes — those would cause the label to track
            // the joystick rest position rather than the last played chord.
            if (src[voice] != TriggerSource::Joystick)
                displayPitches_ = heldPitch_;

            // Activate "start rec by touch" before recordGate so the triggering
            // note-on is captured. activateRecordingNow() is a no-op unless armed.
            looper_.activateRecordingNow();

            // Pad priority: choke any active arp note on this voice first.
            // This lets the live press cut the arp immediately ("instant choke").
            if (arpOn && arpActiveVoice_ == voice && arpActivePitch_ >= 0)
            {
                const int chArp = arpActiveCh_;
                if (noteCount_[chArp - 1][arpActivePitch_] > 0 &&
                    --noteCount_[chArp - 1][arpActivePitch_] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(chArp, arpActivePitch_, (uint8_t)0), sampleOff);
                else
                    noteCount_[chArp - 1][arpActivePitch_] = 0;
                arpActivePitch_ = -1;
                arpActiveVoice_ = -1;
                arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                arpNoteOffRemaining_ = 0.0;
            }
        }

        if (isOn)
        {
            // Record gate event in looper (using JUCE 8 ppqPos variable)
            const double beatPos = (hasDaw && ppqPos >= 0.0)
                ? ppqPos
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, true);
            // Live pad notes go through effectiveChannel with noteCount dedup.
            const int ch = effectiveChannel(voice);
            sentChannel_[voice] = ch;
            if (noteCount_[ch - 1][pitch]++ == 0)
                midi.addEvent(juce::MidiMessage::noteOn(ch, pitch, (uint8_t)100), sampleOff);

            // Sub-octave emission: snapshot pitch at gate-open so gate-close uses the same value
            const bool subEnabled = (*apvts.getRawParameterValue("subOct" + juce::String(voice)) > 0.5f);
            if (subEnabled)
            {
                const int subPitch = juce::jlimit(0, 127, pitch - 12);
                subHeldPitch_[voice] = subPitch;
                if (noteCount_[ch - 1][subPitch]++ == 0)
                    midi.addEvent(juce::MidiMessage::noteOn(ch, subPitch, (uint8_t)100), sampleOff);
                subOctSounding_[voice].store(true, std::memory_order_relaxed);
            }
            else
            {
                subHeldPitch_[voice] = -1;
            }
        }
        else
        {
            const double beatPos = (hasDaw && ppqPos >= 0.0)
                ? ppqPos
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, false);
            // Note-off uses the channel snapshot captured at note-on time.
            const int ch = sentChannel_[voice];
            if (noteCount_[ch - 1][pitch] > 0 && --noteCount_[ch - 1][pitch] == 0)
                midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), sampleOff);
            else
                noteCount_[ch - 1][pitch] = 0;  // clamp (defensive)

            // Sub-octave note-off — always use snapshot, never heldPitch_[voice] - 12
            const int subPitch = subHeldPitch_[voice];
            if (subPitch >= 0)
            {
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), sampleOff);
                else
                    noteCount_[ch - 1][subPitch] = 0;
                subHeldPitch_[voice] = -1;
                subOctSounding_[voice].store(false, std::memory_order_relaxed);
            }
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
        tp.midiChannels[v] = effectiveChannel(v);
    }
    tp.randomPopulation  = apvts.getRawParameterValue("randomPopulation")->load();
    tp.randomProbability = apvts.getRawParameterValue("randomProbability")->load();
    // ── Gate Length smoothing ─────────────────────────────────────────────
    // When joystick is routed to gate length, gateLengthDisplay_ holds base+offset.
    // When not joystick-routed, read APVTS directly.
    // Either way, ramp through SmoothedValue to eliminate zipper noise.
    {
        const int xGateMode = static_cast<int>(apvts.getRawParameterValue("filterXMode")->load());
        const int yGateMode = static_cast<int>(apvts.getRawParameterValue("filterYMode")->load());
        const bool gateFromJoystick = (xGateMode == 5 || yGateMode == 5);
        const float targetGate = gateFromJoystick
            ? gateLengthDisplay_.load(std::memory_order_relaxed)
            : apvts.getRawParameterValue("gateLength")->load();
        smoothedGateLength_.setTargetValue(targetGate);
        smoothedGateLength_.skip(blockSize);
    }
    tp.gateLength        = smoothedGateLength_.getCurrentValue();
    // joystickGateTime: seconds of stillness before Joystick-mode gate closes.
    // arpGateTime was removed in Phase 20; use a fixed 1.0 s constant until a dedicated param is added.
    tp.joystickGateTime   = 1.0f;
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

        // ARP-05: bar-boundary release — fires when armed and the current audio block
        // crosses a bar line while the DAW is already playing.
        // Guard: arpSyncOn only (looper sync uses looperJustStarted above).
        if (arpWaitingForPlay_ && arpSyncOn && isDawPlaying && ppqPos >= 0.0)
        {
            const double beatsThisBlock = lp.bpm * static_cast<double>(blockSize)
                                          / (sampleRate_ * 60.0);
            const double beatsPerBar    = static_cast<double>(timeSigNumer);
            const long long barAtStart  = static_cast<long long>(ppqPos / beatsPerBar);
            const long long barAtEnd    = static_cast<long long>((ppqPos + beatsThisBlock)
                                                                 / beatsPerBar);
            if (barAtEnd > barAtStart)
                arpWaitingForPlay_ = false;   // bar boundary crossed — launch
        }
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
            const int ch = arpActiveCh_;
            if (noteCount_[ch - 1][arpActivePitch_] > 0 &&
                --noteCount_[ch - 1][arpActivePitch_] == 0)
                midi.addEvent(juce::MidiMessage::noteOff(ch, arpActivePitch_, (uint8_t)0), 0);
            else
                noteCount_[ch - 1][arpActivePitch_] = 0;
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

        const double gateRatio = smoothedGateLength_.getCurrentValue();
        // Manual mode (gateRatio == 0.0): gateBeats = 0.0, arpNoteOffRemaining_ stays 0
        // → no gate-time note-off; step boundary provides implicit note-off via the cut-at-next-step path.
        const double gateBeats = subdivBeats * gateRatio;

        const double beatsThisBlock = lp.bpm * static_cast<double>(blockSize)
                                      / (sampleRate_ * 60.0);

        // Gate-time note-off: fires mid-block when gate expires before next step.
        if (arpActivePitch_ >= 0 && arpNoteOffRemaining_ > 0.0)
        {
            arpNoteOffRemaining_ -= beatsThisBlock;
            if (arpNoteOffRemaining_ <= 0.0)
            {
                const int ch = arpActiveCh_;
                if (noteCount_[ch - 1][arpActivePitch_] > 0 &&
                    --noteCount_[ch - 1][arpActivePitch_] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, arpActivePitch_, (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][arpActivePitch_] = 0;
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
                const int ch = arpActiveCh_;
                if (noteCount_[ch - 1][arpActivePitch_] > 0 &&
                    --noteCount_[ch - 1][arpActivePitch_] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, arpActivePitch_, (uint8_t)0), 0);
                else
                    noteCount_[ch - 1][arpActivePitch_] = 0;
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
            const int ch = effectiveChannel(voice);
            arpActiveCh_ = ch;
            sentChannel_[voice] = ch;
            if (noteCount_[ch - 1][pitch]++ == 0)
                midi.addEvent(juce::MidiMessage::noteOn(ch, pitch, (uint8_t)100), 0);
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
        const int fCh = effectiveFilterChannel();

        // Handle disconnect events.
        if (pendingAllNotesOff_.exchange(false, std::memory_order_acq_rel))
        {
            for (int ch = 1; ch <= 16; ++ch)
                midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
            resetNoteCount();
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
            prevLfoCcX_ = prevLfoCcY_ = -1;
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
            const int ccXnum = (xMode == 1) ? 12 : 74;  // 0=CC74, 1=CC12; modes 2-5 skip CC emit
            const int ccYnum = (yMode == 1) ? 76 : 71;  // 0=CC71, 1=CC76; modes 2-5 skip CC emit

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

                // xOffset/yOffset are now -50..+50 centered at 0; add to MIDI centre 64.
                const int ccCut = juce::jlimit(0, 127, (int)std::roundf(64.0f + xOffset + prevFilterX_ * 63.5f * (xAtten / 100.0f)));
                const int ccRes = juce::jlimit(0, 127, (int)std::roundf(64.0f + yOffset + prevFilterY_ * 63.5f * (yAtten / 100.0f)));

                if (xMode <= 1)  // CC targets only
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
                if (yMode <= 1)  // CC targets only
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

            // ── MOD FIX + live-stick display atomics (UI colour feedback) ─────────
            // Uses live stick (not S&H) so knob returns to offset when stick centered.
            {
                const float lx = gamepad_.getLeftStickXLive();
                const float ly = gamepad_.getLeftStickYLive();
                leftStickXDisplay_.store(lx, std::memory_order_relaxed);
                leftStickYDisplay_.store(ly, std::memory_order_relaxed);
                filterXOffsetDisplay_.store(
                    juce::jlimit(-50.0f, 50.0f, xOffset + lx * 63.5f * (xAtten / 100.0f)),
                    std::memory_order_relaxed);
                filterYOffsetDisplay_.store(
                    juce::jlimit(-50.0f, 50.0f, yOffset + ly * 63.5f * (yAtten / 100.0f)),
                    std::memory_order_relaxed);
            }

            // ── LFO / Gate Length dispatch (indices 2–5) ──────────────────────────
            // Reset all override floats unconditionally — only the active mode sets them.
            // This ensures that when the dropdown switches away from an LFO target, the
            // LFO params block reverts to APVTS immediately (no stale override).
            lfoXRateOverride_ = lfoXPhaseOverride_ = lfoXLevelOverride_ = -1.0f;
            lfoYRateOverride_ = lfoYPhaseOverride_ = lfoYLevelOverride_ = -1.0f;

            // Use live stick position (not S&H) so the offset is sustained while the
            // stick is held and returns to the slider base the moment it is released.
            // Not gated by stickUpdated — runs every block while a gamepad is connected.
            if (liveGamepad)
            {
                constexpr float kDeadzone = 0.12f;
                const float liveX = gamepad_.getLeftStickXLive();
                const float liveY = gamepad_.getLeftStickYLive();

                if (xMode >= 2 && xMode <= 5)
                {
                    const float deadX = (std::abs(liveX) < kDeadzone) ? 0.0f : liveX;
                    const float stick = deadX * (xAtten / 100.0f);

                    switch (xMode)
                    {
                        case 2:  // LFO-X Freq
                        {
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
                            if (syncOn)
                            {
                                const float mult = std::pow(4.0f, stick);
                                lfoXSubdivMult_.store(mult, std::memory_order_relaxed);
                            }
                            else
                            {
                                const float base   = apvts.getRawParameterValue(ParamID::lfoXRate)->load();
                                const float actual = juce::jlimit(0.01f, 20.0f, base + stick * 9.995f);
                                lfoXRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoXRateOverride_ = actual;
                            }
                            break;
                        }
                        case 3:  // LFO-X Phase (0–360°)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + stick * 180.0f);
                            lfoXPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXPhaseOverride_ = actual;
                            break;
                        }
                        case 4:  // LFO-X Level (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            lfoXLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXLevelOverride_ = actual;
                            break;
                        }
                        case 5:  // Gate Length (0–1) — no LFO override; smoothedGateLength_ reads display
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::gateLength)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            gateLengthDisplay_.store(actual, std::memory_order_relaxed);
                            break;
                        }
                        default: break;
                    }
                }

                if (yMode >= 2 && yMode <= 5)
                {
                    const float deadY = (std::abs(liveY) < kDeadzone) ? 0.0f : liveY;
                    const float stick = deadY * (yAtten / 100.0f);

                    switch (yMode)
                    {
                        case 2:  // LFO-Y Freq
                        {
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
                            if (syncOn)
                            {
                                const float mult = std::pow(4.0f, stick);
                                lfoYSubdivMult_.store(mult, std::memory_order_relaxed);
                            }
                            else
                            {
                                const float base   = apvts.getRawParameterValue(ParamID::lfoYRate)->load();
                                const float actual = juce::jlimit(0.01f, 20.0f, base + stick * 9.995f);
                                lfoYRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoYRateOverride_ = actual;
                            }
                            break;
                        }
                        case 3:  // LFO-Y Phase (0–360°)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + stick * 180.0f);
                            lfoYPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYPhaseOverride_ = actual;
                            break;
                        }
                        case 4:  // LFO-Y Level (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            lfoYLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYLevelOverride_ = actual;
                            break;
                        }
                        case 5:  // Gate Length (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::gateLength)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            gateLengthDisplay_.store(actual, std::memory_order_relaxed);
                            break;
                        }
                        default: break;
                    }
                }

                // Reset subdivision multiplier when not in sync-mode LFO Freq target
                if (xMode != 2) lfoXSubdivMult_.store(1.0f, std::memory_order_relaxed);
                if (yMode != 2) lfoYSubdivMult_.store(1.0f, std::memory_order_relaxed);
            }
        }
    }

    // ── LFO direct CC output ──────────────────────────────────────────────────
    {
        const int fCh    = effectiveFilterChannel();
        const bool xOn   = *apvts.getRawParameterValue("lfoXEnabled") > 0.5f;
        const bool yOn   = *apvts.getRawParameterValue("lfoYEnabled") > 0.5f;
        const int xCcNum = ccDestToNumber((int)*apvts.getRawParameterValue("lfoXCcDest"));
        const int yCcNum = ccDestToNumber((int)*apvts.getRawParameterValue("lfoYCcDest"));
        if (xOn && xCcNum >= 0)
        {
            const int v = juce::jlimit(0, 127, (int)std::roundf(64.0f + lfoXRampOut_ * 63.5f));
            if (v != prevLfoCcX_) { midi.addEvent(juce::MidiMessage::controllerEvent(fCh, xCcNum, v), 0); prevLfoCcX_ = v; }
        }
        if (yOn && yCcNum >= 0)
        {
            const int v = juce::jlimit(0, 127, (int)std::roundf(64.0f + lfoYRampOut_ * 63.5f));
            if (v != prevLfoCcY_) { midi.addEvent(juce::MidiMessage::controllerEvent(fCh, yCcNum, v), 0); prevLfoCcY_ = v; }
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
