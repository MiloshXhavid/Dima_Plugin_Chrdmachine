#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CustomCcRoutingHelpers.h"
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
    addInt  (ParamID::thirdInterval,   "Third Interval",    0, 11,  4);
    addInt  (ParamID::fifthInterval,   "Fifth Interval",    0, 11,  7);
    addInt  (ParamID::tensionInterval, "Tension Interval",  0, 11, 11);
    addInt  (ParamID::rootOctave,      "Root Octave",       0, 11,  2);
    addInt  (ParamID::thirdOctave,     "Third Octave",      0, 11,  4);
    addInt  (ParamID::fifthOctave,     "Fifth Octave",      0, 11,  4);
    addInt  (ParamID::tensionOctave,   "Tension Octave",    0, 11,  3);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickXAtten, "Joy X Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickYAtten, "Joy Y Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickThreshold, "Joystick Threshold",
        juce::NormalisableRange<float>(0.0f, 0.1f, 0.001f), 0.05f));

    // ── Scale ─────────────────────────────────────────────────────────────────
    {
        juce::StringArray scaleNames;
        for (int i = 0; i < (int)ScalePreset::COUNT; ++i)
            scaleNames.add(ScaleQuantizer::getScaleName(static_cast<ScalePreset>(i)));

        addChoice(ParamID::scalePreset, "Scale Preset", scaleNames, 1);
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
    addFloat("randomPopulation",  "Random Population",  1.0f, 64.0f, 32.0f);
    addFloat("randomProbability", "Random Probability", 0.0f, 1.0f,  1.0f);
    addFloat("gateLength",        "Gate Length",        0.0f, 1.0f,  0.5f);
    {
        const juce::StringArray subdivChoices {
            "4/1", "2/1",
            "1/1", "1/1T",
            "1/2", "1/2T",
            "1/4", "1/4T",
            "1/8", "1/8T",
            "1/16", "1/16T",
            "1/32", "1/32T",
            "1/64",
            "2/1T", "4/1T"   // indices 15-16, appended for backward compat
        };
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv0", "Random Subdiv Root",    subdivChoices, 8));  // default = 1/8 (index 8)
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv1", "Random Subdiv Third",   subdivChoices, 8));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv2", "Random Subdiv Fifth",   subdivChoices, 8));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "randomSubdiv3", "Random Subdiv Tension", subdivChoices, 8));
    }

    // Random sync mode — 3-state: FREE / INT / DAW
    addChoice("randomSyncMode", "Random Sync Mode",
              juce::StringArray { "FREE", "INT", "DAW" }, 0);

    // Arp step pattern (8 cells, ON=0 / TIE=1 / OFF=2)
    {
        const juce::StringArray kStepStates { "ON", "TIE", "OFF" };
        for (int i = 0; i < 8; ++i)
            addChoice("arpStepState" + juce::String(i),
                      "Arp Step " + juce::String(i + 1) + " State",
                      kStepStates, 0);
    }

    // Arp pattern length (1–8 steps, combo index 0=length1, index 7=length8)
    addChoice("arpLength", "Arp Length",
              juce::StringArray { "1","2","3","4","5","6","7","8" }, 7);

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
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lfoXCustomCc", "LFO X Custom CC",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 74.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lfoYCustomCc", "LFO Y Custom CC",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 71.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "filterXCustomCc", "Filter X Custom CC",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 74.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "filterYCustomCc", "Filter Y Custom CC",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 71.0f));
    addInt  (ParamID::filterMidiCh, "Filter MIDI Channel",  1, 16, 1);
    {
        // X: 0=CC74, 1=CC12, 2=CC71, 3=CC76, 4=LFO-X Freq, 5=LFO-X Phase, 6=LFO-X Level, 7=Gate
        //    8=LFO-Y Freq, 9=LFO-Y Phase, 10=LFO-Y Level  (cross-LFO targets)
        // X/Y: 0-17=CC (shared list), 18=direct LFO Freq, 19=Phase, 20=Level, 21=Gate,
        //      22=cross LFO Freq, 23=cross LFO Phase, 24=cross LFO Level
        static const juce::StringArray kFilterCcChoices {
            "CC1 - Mod Wheel",   "CC2 - Breath",       "CC5 - Portamento",
            "CC7 - Volume",      "CC10 - Pan",          "CC11 - Expression",
            "CC12 - VCF LFO",    "CC16 - GenPurp 1",   "CC17 - GenPurp 2",
            "CC71 - Resonance",  "CC72 - Release",      "CC73 - Attack",
            "CC74 - Filter Cut", "CC75 - Decay",        "CC76 - Vibrato Rate",
            "CC77 - Vibrato Depth","CC91 - Reverb",     "CC93 - Chorus" };
        // LFO targets first (indices 0-6), then 18 CCs (indices 7-24).
        // This ordering ensures backward compatibility: LFO modes stay at low indices.
        juce::StringArray xModes { "LFO-X Freq", "LFO-X Phase", "LFO-X Level", "Gate Length",
                                   "LFO-Y Freq", "LFO-Y Phase", "LFO-Y Level" };
        xModes.addArray(kFilterCcChoices);
        xModes.add("Custom CC...");   // index 25 = kCustomFilterModeIdx
        juce::StringArray yModes { "LFO-Y Freq", "LFO-Y Phase", "LFO-Y Level", "Gate Length",
                                   "LFO-X Freq", "LFO-X Phase", "LFO-X Level" };
        yModes.addArray(kFilterCcChoices);
        yModes.add("Custom CC...");   // index 25 = kCustomFilterModeIdx
        addChoice("filterXMode", "Left Stick X Mode", xModes, 16); // default: CC71 Resonance (index 16 = 7+9)
        addChoice("filterYMode", "Left Stick Y Mode", yModes, 19); // default: CC74 Filter Cut (index 19 = 7+12)
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
              { "4/1", "2/1",
                "1/1", "1/1T",
                "1/2", "1/2T",
                "1/4", "1/4T",    // index 6 = default
                "1/8", "1/8T",
                "1/16", "1/16T",
                "1/32", "1/32T",
                "1/64",
                "2/1T", "4/1T" }, 6);  // default: 1/4 (matches Random Trigger default)
    addChoice(ParamID::arpOrder, "Arp Order",
              { "Up", "Down", "Up+Down", "Down+Up", "Outer-In", "Inner-Out", "Random" }, 0);
    // arpGateTime removed in Phase 20; unified gateLength param (0.0–1.0) registered in Trigger section above

    // ── Filter CC live display (read-only from DAW perspective) ──────────────
    // Updated every timer tick from audio-thread atomics so DAW can see stick movement.
    addFloat("filterCutLive", "Filter Cut CC", 0.0f, 127.0f, 0.0f);
    addFloat("filterResLive", "Filter Res CC", 0.0f, 127.0f, 0.0f);

    // ── Quantize ──────────────────────────────────────────────────────────────
    addInt(ParamID::quantizeMode,   "Quantize Mode",   0, 2, 0);  // 0=Off, 1=Live, 2=Post (default Off)
    {
        const juce::StringArray qSubdivChoices {
            "1/1",  "1/1T",    // 0–1
            "1/2",  "1/2T",    // 2–3
            "1/4",  "1/4T",    // 4–5
            "1/8",  "1/8T",    // 6–7
            "1/16", "1/16T",   // 8–9
            "1/32", "1/32T"    // 10–11
        };
        addChoice(ParamID::quantizeSubdiv, "Quantize Subdiv", qSubdivChoices, 6);  // default 6 = 1/8
    }

    // ── LFO ───────────────────────────────────────────────────────────────────
    addBool(ParamID::lfoXEnabled, "LFO X Enabled", false);
    addBool(ParamID::lfoYEnabled, "LFO Y Enabled", false);
    addBool("lfoXCursorLink", "LFO X Cursor Link", true);
    addBool("lfoYCursorLink", "LFO Y Cursor Link", true);
    {
        juce::StringArray waveforms { "Sine", "Triangle", "Saw Up", "Saw Down",
                                      "Square", "S&H", "Random" };
        addChoice(ParamID::lfoXWaveform, "LFO X Waveform", waveforms, 0);
        addChoice(ParamID::lfoYWaveform, "LFO Y Waveform", waveforms, 0);
    }
    // Rate: log-scale NormalisableRange so slider midpoint ≈ 1 Hz.
    // Skew factor: log(0.5) / log((midHz - minHz) / (maxHz - minHz))
    // 0.35 puts 50% of slider ≈ 0.8 Hz — spreads the useful slow range (0.1–4 Hz)
    // evenly across most of the slider instead of cramming it into the top 50%.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoXRate, "LFO X Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f),
        1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::lfoYRate, "LFO Y Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f),
        1.0f));
    addFloat(ParamID::lfoXLevel,      "LFO X Level",      0.001f, 1.0f, 0.001f);
    addFloat(ParamID::lfoYLevel,      "LFO Y Level",      0.001f, 1.0f, 0.001f);
    addFloat(ParamID::lfoXPhase,      "LFO X Phase",      0.0f, 360.0f, 0.0f);
    addFloat(ParamID::lfoYPhase,      "LFO Y Phase",      0.0f, 360.0f, 0.0f);
    addFloat(ParamID::lfoXDistortion, "LFO X Distortion", 0.0f, 1.0f, 0.0f);
    addFloat(ParamID::lfoYDistortion, "LFO Y Distortion", 0.0f, 1.0f, 0.0f);
    addBool (ParamID::lfoXSync,       "LFO X Sync",       false);
    addBool (ParamID::lfoYSync,       "LFO Y Sync",       false);
    {
        // 18 items sorted slow→fast: slow left, fast right.
        // 18 subdivisions: 16/1..1/32T. 1/4 is at index 9 (middle, MOD FIX centre → index 8.5 rounds to 9).
        juce::StringArray subdivs { "16/1", "8/1", "4/1", "4/1T", "2/1", "2/1T",
                                    "1/1", "1/1T", "1/2", "1/4", "1/4T",
                                    "1/8",
                                    "1/16.", "1/8T", "1/16", "1/16T", "1/32", "1/32T" };
        addChoice(ParamID::lfoXSubdiv, "LFO X Subdivision", subdivs, 9);   // default: 1/4
        addChoice(ParamID::lfoYSubdiv, "LFO Y Subdivision", subdivs, 9);   // default: 1/4
    }
    {
        const juce::StringArray ccDests { "Off","CC1 - Mod Wheel","CC2 - Breath",
            "CC5 - Portamento","CC7 - Volume","CC10 - Pan",
            "CC11 - Expression","CC12 - VCF LFO",
            "CC16 - GenPurp 1","CC17 - GenPurp 2",
            "CC71 - Resonance","CC72 - Release",
            "CC73 - Attack","CC74 - Filter Cut","CC75 - Decay",
            "CC76 - Vibrato Rate","CC77 - Vibrato Depth",
            "CC91 - Reverb","CC93 - Chorus",
            "Custom CC..." };   // index 19 = kCustomCcDestIdx
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoXCcDest","LFO X CC Dest",ccDests,0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoYCcDest","LFO Y CC Dest",ccDests,0));
    }
    {
        juce::StringArray sisterDests { "None", "Rate", "Phase", "Level", "Dist" };
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoXSister","LFO X Sister Dest",sisterDests,0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("lfoYSister","LFO Y Sister Dest",sisterDests,0));
    }
    addFloat("lfoXSisterAtten", "LFO X Sister Atten", -1.0f, 1.0f, 1.0f);
    addFloat("lfoYSisterAtten", "LFO Y Sister Atten", -1.0f, 1.0f, 1.0f);

    // ── Sub Octave (Phase 19) ─────────────────────────────────────────────────
    addBool("subOct0", "Sub Oct Root",    false);
    addBool("subOct1", "Sub Oct Third",   false);
    addBool("subOct2", "Sub Oct Fifth",   false);
    addBool("subOct3", "Sub Oct Tension", false);

    // ── Stick routing overrides ───────────────────────────────────────────────
    addBool("stickSwap",   "Stick Swap",   false);
    addBool("stickInvert", "Stick Invert", false);

    // ── Crosshair visualization (Phase 40) ───────────────────────────────────
    addBool("crosshairVisible", "Crosshair Visible", true);

    return layout;
}

// ─── Constructor ──────────────────────────────────────────────────────────────

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
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

PluginProcessor::~PluginProcessor()
{
    // Clear both callback slots before gamepad_ destructs.
    //
    // onConnectionChange: gamepad_ is declared last in the private section, so it
    // destructs first (C++ reverse-declaration-order rule). By the time
    // GamepadInput::~GamepadInput runs closeController() and fires this callback,
    // pendingAllNotesOff_ / pendingCcReset_ would already be destructed — UB.
    // Nulling here makes the callback a no-op.
    //
    // onConnectionChangeUI: the editor destructor nulls this in normal flow, but
    // we add a defensive null here for the case where the editor was never created
    // or was already destroyed by another path before this destructor runs.
    gamepad_.onConnectionChange   = nullptr;
    gamepad_.onConnectionChangeUI = nullptr;
}

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
    smoothedCcCut_.reset(sampleRate_, 0.030);  // 30ms — eliminates deadzone-boundary jitter on filter CC
    smoothedCcCut_.setCurrentAndTargetValue(64.0f);
    smoothedCcRes_.reset(sampleRate_, 0.030);
    smoothedCcRes_.setCurrentAndTargetValue(64.0f);
}

void PluginProcessor::releaseResources()
{
    // The host calls this after the last processBlock(), before the plugin is
    // deactivated or unloaded. The audio thread is already stopped. We cannot
    // write MIDI here, but we must reset all audio-thread-only state so that the
    // next prepareToPlay() + processBlock() cycle starts from a known-clean state.
    //
    // Failure to reset these means stale noteCount_[] entries from a previous
    // session can suppress note-offs (the count is already 0 so the decrement
    // path that emits noteOff never fires) or cause phantom notes on first block.

    // ── Gate / trigger state ──────────────────────────────────────────────────
    trigger_.resetAllGates();

    // ── Note-count reference counters (single-channel routing) ───────────────
    resetNoteCount();   // also zeros subHeldPitch_ and subOctSounding_

    // ── Looper pitch snapshots ────────────────────────────────────────────────
    looperActivePitch_.fill(-1);
    std::fill(std::begin(looperActiveSubPitch_), std::end(looperActiveSubPitch_), -1);

    // ── Arpeggiator pitch snapshot ────────────────────────────────────────────
    arpActivePitch_      = -1;
    arpActiveVoice_      = -1;
    arpNoteOffRemaining_ = 0.0;
    arpCurrentVoice_.store(-1, std::memory_order_relaxed);

    // ── LFO ramp-out state ────────────────────────────────────────────────────
    lfoXRampOut_ = 0.0f;
    lfoYRampOut_ = 0.0f;

    // ── CC dedup sentinels — force re-emit on next prepare ───────────────────
    // -1 = force-send (not -2 which is the "never touched" on-load sentinel).
    prevCcCut_.store(-1, std::memory_order_relaxed);
    prevCcRes_.store(-1, std::memory_order_relaxed);
    prevLfoCcX_ = -1;
    prevLfoCcY_ = -1;

    // ── Filter dedup sentinels — force recalculation on first block ──────────
    prevFilterX_ = -99.0f;
    prevFilterY_ = -99.0f;
    prevBaseX_   = -1.0f;
    prevBaseY_   = -1.0f;
    prevAttenX_  = -1.0f;
    prevAttenY_  = -1.0f;

    // ── Transport edge-detect booleans ────────────────────────────────────────
    // Reset so that the first processBlock() after re-arm sees a clean edge state
    // and does not skip the DAW-stop allNotesOff sequence.
    prevIsDawPlaying_      = false;
    prevLooperWasPlaying_  = false;
    prevLooperRecording_   = false;
    prevDawPlaying_        = false;
    prevArpOn_             = false;

    // ── Free-tempo beat detection ─────────────────────────────────────────────
    sampleCounter_ = 0;
    prevBeatCount_ = -1.0;

    // ── LFO instant-capture countdown ────────────────────────────────────────
    lfoInstantCaptureSamplesLeft_ = 0;
    lfoInstantCapture_.store(false, std::memory_order_relaxed);

    // ── LFO mid-cycle recording flags ─────────────────────────────────────────
    lfoXMidCycleRec_ = false;
    lfoYMidCycleRec_ = false;
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
    // Instrument: no audio input, stereo output.
    // Accept numOut == 0 for DAWs that don't need audio from a MIDI generator.
    const int numIn  = layouts.getMainInputChannels();
    const int numOut = layouts.getMainOutputChannels();
    return numIn == 0 && (numOut == 0 || numOut == 2);
}

// ─── ChordParams builder ──────────────────────────────────────────────────────

ChordEngine::Params PluginProcessor::buildChordParams() const
{
    ChordEngine::Params p;

    // Apply gamepad stick to chord engine with swap/invert:
    // SWAP: left stick (filterX/Y) → pitch when active; INV: swap X↔Y axes
    const bool  doSwap = stickSwap_.load();
    const bool  doInv  = stickInvert_.load();
    const float rawX = doSwap ? gamepad_.getFilterX() : gamepad_.getPitchX();
    const float rawY = doSwap ? gamepad_.getFilterY() : gamepad_.getPitchY();
    const float gpX = doInv ? rawY : rawX;
    const float gpY = doInv ? rawX : rawY;
    const float jx  = joystickX.load();
    const float jy  = joystickY.load();

    // Per-axis center snap: values within ±1% snap to exactly 0 (matches UI snap in updateFromMouse).
    // Kept small because GamepadInput now applies deadzone rescaling to the pitch stick,
    // so values near zero are already suppressed by the hardware deadzone.
    constexpr float kCenterSnap = 0.01f;
    const float gpXs = std::abs(gpX) < kCenterSnap ? 0.0f : gpX;
    const float gpYs = std::abs(gpY) < kCenterSnap ? 0.0f : gpY;

    // Gamepad pitch stick: active when any axis is deflected past snap threshold.
    // When looper is playing with joystick content, stick acts as additive offset (applied below).
    // When looper is not playing, stick overrides live mouse joystick as before.
    const bool looperJoyActive = looper_.isPlaying() && looper_.hasJoystickContent();
    const bool gpActive = !looperJoyActive
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

    // Cache stick-routing params for this block
    stickSwap_.store(*apvts.getRawParameterValue("stickSwap") > 0.5f);
    stickInvert_.store(*apvts.getRawParameterValue("stickInvert") > 0.5f);
    const bool blkSwap = stickSwap_.load();
    const bool blkInv  = stickInvert_.load();
    // Helpers: return filter-CC axis after swap (SWAP=right stick) and INV (X↔Y swap)
    auto filtXRaw = [&]() -> float { return blkSwap ? gamepad_.getPitchX() : gamepad_.getFilterX(); };
    auto filtYRaw = [&]() -> float { return blkSwap ? gamepad_.getPitchY() : gamepad_.getFilterY(); };
    auto filtX = [&]() -> float { return blkInv ? filtYRaw() : filtXRaw(); };
    auto filtY = [&]() -> float { return blkInv ? filtXRaw() : filtYRaw(); };

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
            // Range matches the APVTS declaration (0..11 absolute MIDI octave).
            { const int d = gamepad_.consumeOptionDpadDelta(0); if (d) stepClampingParam(apvts, "rootOctave",    0, 11, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(1); if (d) stepClampingParam(apvts, "thirdOctave",   0, 11, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(2); if (d) stepClampingParam(apvts, "tensionOctave", 0, 11, d); }
            { const int d = gamepad_.consumeOptionDpadDelta(3); if (d) stepClampingParam(apvts, "fifthOctave",   0, 11, d); }

            // Mode 1 face-button arp dispatch (OPT1-01 through OPT1-04)
            // Circle → toggle arpEnabled; Triangle → step arpSubdiv; Square → step arpOrder; Cross → cycle randomSyncMode
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
            { const int d = gamepad_.consumeArpRateDelta();  if (d) stepWrappingParam(apvts, "arpSubdiv", 0, 16, d); }
            { const int d = gamepad_.consumeArpOrderDelta(); if (d) stepWrappingParam(apvts, "arpOrder",  0, 6, d); }
            if (gamepad_.consumeRndSyncToggle())
            {
                auto* rndParam = dynamic_cast<juce::AudioParameterChoice*>(
                    apvts.getParameter("randomSyncMode"));
                if (rndParam != nullptr)
                {
                    const int next = (rndParam->getIndex() + 1) % 3;
                    rndParam->setValueNotifyingHost(rndParam->convertTo0to1(static_cast<float>(next)));
                }
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
    dawPlaying_.store(isDawPlaying, std::memory_order_relaxed);

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

    // Additive offset model: looper is the base; mouse sticky offset + gamepad live offset added on top.
    // looperJoyOffsetX_/Y_ = sticky mouse offset (written by JoystickPad mouseDown/Drag).
    // Gamepad right stick provides non-sticky live offset when looper is driving joystick content.
    {
        const bool  ljaSwap = stickSwap_.load();
        const bool  ljaInv  = stickInvert_.load();
        const float ljaRawX = ljaSwap ? gamepad_.getFilterX() : gamepad_.getPitchX();
        const float ljaRawY = ljaSwap ? gamepad_.getFilterY() : gamepad_.getPitchY();
        const float ljaGpX  = ljaInv ? ljaRawY : ljaRawX;
        const float ljaGpY  = ljaInv ? ljaRawX : ljaRawY;
        constexpr float kSnap = 0.01f;
        const float ljaGpXs  = std::abs(ljaGpX) < kSnap ? 0.0f : ljaGpX;
        const float ljaGpYs  = std::abs(ljaGpY) < kSnap ? 0.0f : ljaGpY;
        const bool  looperJoyActive = looper_.isPlaying() && looper_.hasJoystickContent();

        if (loopOut.hasJoystickX)
        {
            const float mouseOff = looperJoyOffsetX_.load(std::memory_order_relaxed);
            const float stickOff = looperJoyActive ? ljaGpXs : 0.0f;
            joystickX.store(juce::jlimit(-1.0f, 1.0f, loopOut.joystickX + mouseOff + stickOff));
        }
        if (loopOut.hasJoystickY)
        {
            const float mouseOff = looperJoyOffsetY_.load(std::memory_order_relaxed);
            const float stickOff = looperJoyActive ? ljaGpYs : 0.0f;
            joystickY.store(juce::jlimit(-1.0f, 1.0f, loopOut.joystickY + mouseOff + stickOff));
        }
    }

    // ── LFO Recording state machine — edge detection (Phase 22) ──────────────
    {
        const bool looperNowRecording = looper_.isRecording();

        // Rising edge: looper just started recording → start capture on armed LFOs.
        // midCycleRec = false: the auto punch-out (falling edge) fires simultaneously
        // with looperCycled at the first wrap, stopping capture after exactly one loop.
        if (!prevLooperRecording_ && looperNowRecording)
        {
            if (lfoX_.getRecState() == LfoRecState::Armed)
            {
                lfoX_.startCapture();
                lfoXMidCycleRec_ = false;
            }
            if (lfoY_.getRecState() == LfoRecState::Armed)
            {
                lfoY_.startCapture();
                lfoYMidCycleRec_ = false;
            }
        }
        // Falling edge: looper just stopped recording (auto punch-out at cycle wrap).
        else if (prevLooperRecording_ && !looperNowRecording)
        {
            if (lfoX_.getRecState() == LfoRecState::Recording && !lfoXMidCycleRec_)
                lfoX_.stopCapture();
            if (lfoY_.getRecState() == LfoRecState::Recording && !lfoYMidCycleRec_)
                lfoY_.stopCapture();
        }

        prevLooperRecording_ = looperNowRecording;

        // Fix 4: transport stopped — force-stop looper when DAW Sync is ON.
        // Bug-3: transport stopped while LFO was recording → finalise capture.
        if (prevDawPlaying_ && !isDawPlaying)
        {
            // Fix 4: force-stop looper when DAW Sync is ON and transport stops
            if (looper_.isSyncToDaw() && looper_.isPlaying())
            {
                if (looper_.isRecording())
                    looper_.record();   // finalize recording first (disarms, calls finaliseRecording)
                looper_.startStop();    // stop playback; note-offs emitted via allNotesOff in next process()
            }

            // Existing LFO capture stop (Bug-3 from Phase 22) — unchanged
            if (lfoX_.getRecState() == LfoRecState::Recording) { lfoX_.stopCapture(); lfoXMidCycleRec_ = false; }
            if (lfoY_.getRecState() == LfoRecState::Recording) { lfoY_.stopCapture(); lfoYMidCycleRec_ = false; }
            lfoInstantCaptureSamplesLeft_ = 0;
        }
        prevDawPlaying_ = isDawPlaying;

        // Cycle wrap during playback: start capture on Armed LFOs, stop after one full cycle.
        // Mid-cycle flag: first looperCycled after a looper-REC-triggered start clears the flag
        // instead of stopping, so the LFO records for one full cycle from the REC press point.
        // Snapshot states before any transition so start and stop don't collide in the same block.
        if (loopOut.looperCycled)
        {
            const bool xWasArmed     = (lfoX_.getRecState() == LfoRecState::Armed);
            const bool yWasArmed     = (lfoY_.getRecState() == LfoRecState::Armed);
            const bool xWasRecording = (lfoX_.getRecState() == LfoRecState::Recording);
            const bool yWasRecording = (lfoY_.getRecState() == LfoRecState::Recording);
            // Stop recording: only if NOT mid-cycle. Mid-cycle gets one more pass.
            if (xWasRecording && !lfoXMidCycleRec_) lfoX_.stopCapture();
            if (yWasRecording && !lfoYMidCycleRec_) lfoY_.stopCapture();
            // Mid-cycle first wrap: clear the flag so next cycle wrap will stop.
            if (xWasRecording && lfoXMidCycleRec_) lfoXMidCycleRec_ = false;
            if (yWasRecording && lfoYMidCycleRec_) lfoYMidCycleRec_ = false;
            // Start capture for Armed LFOs on cycle wrap (wait-for-wrap behavior, no mid-cycle).
            if (xWasArmed) { lfoX_.startCapture(); lfoXMidCycleRec_ = false; }
            if (yWasArmed) { lfoY_.startCapture(); lfoYMidCycleRec_ = false; }
        }

        // Looper reset seeks position only — do NOT clear LFO recordings.
        // (LFO recordings are preserved across looper position resets.)

        // Instant capture: looper REC pressed while LFO Armed + looper playing.
        // Starts recording immediately and stops after exactly one loop cycle in samples.
        if (lfoInstantCapture_.exchange(false, std::memory_order_relaxed))
        {
            const bool xArmed = (lfoX_.getRecState() == LfoRecState::Armed);
            const bool yArmed = (lfoY_.getRecState() == LfoRecState::Armed);
            if (xArmed || yArmed)
            {
                const double bpm        = lp.bpm > 0.0 ? lp.bpm : 120.0;
                const double loopBeats  = looper_.getLoopLengthBeats();
                lfoInstantCaptureSamplesLeft_ = (loopBeats > 0.0)
                    ? juce::roundToInt(loopBeats * (sampleRate_ * 60.0 / bpm))
                    : 0;
                if (xArmed) { lfoX_.startCapture(); lfoXMidCycleRec_ = false; }
                if (yArmed) { lfoY_.startCapture(); lfoYMidCycleRec_ = false; }
            }
        }
        // Count down instant-capture one-cycle timer and stop when elapsed.
        if (lfoInstantCaptureSamplesLeft_ > 0)
        {
            lfoInstantCaptureSamplesLeft_ -= blockSize;
            if (lfoInstantCaptureSamplesLeft_ <= 0)
            {
                lfoInstantCaptureSamplesLeft_ = 0;
                if (lfoX_.getRecState() == LfoRecState::Recording) lfoX_.stopCapture();
                if (lfoY_.getRecState() == LfoRecState::Recording) lfoY_.stopCapture();
            }
        }
    }

    // ── Build chord params ────────────────────────────────────────────────────
    ChordEngine::Params chordP = buildChordParams();

    // ── LFO modulation ────────────────────────────────────────────────────────
    // Subdivision lookup: beats per cycle (quarter-note = 1.0 beat)
    // Maps lfoXSubdiv / lfoYSubdiv choice index to subdivBeats for LfoEngine
    // 18 items sorted slow→fast matching the subdivs list above.
    // 16/1  8/1   4/1    4/1T       2/1   2/1T        1/1   1/1T       1/2   1/4   1/4T       | 1/8 |  1/16.  1/8T     1/16   1/16T    1/32    1/32T
    static constexpr double kLfoSubdivBeats[18] = {
        64.0, 32.0, 16.0, 32.0/3.0,  8.0, 16.0/3.0,   4.0, 8.0/3.0,   2.0,  1.0, 2.0/3.0,
        0.5,
        0.375, 1.0/3.0, 0.25, 1.0/6.0, 0.125, 1.0/12.0
    };
    static constexpr int kMaxSubdivIdx = 17;
    // filterXMode / filterYMode: indices 0-6 are LFO/Gate targets, 7-24 are CC modes, 25=Custom CC.
    // kNumLfoTargetModes, kFilterCcNums, kNumFilterCcModes are in CustomCcRoutingHelpers.h.
    {
        const bool  xEnabled = *apvts.getRawParameterValue(ParamID::lfoXEnabled) > 0.5f;
        const bool  yEnabled = *apvts.getRawParameterValue(ParamID::lfoYEnabled) > 0.5f;
        const float xLevel   = *apvts.getRawParameterValue(ParamID::lfoXLevel);
        const float yLevel   = *apvts.getRawParameterValue(ParamID::lfoYLevel);

        // Consume any pending phase resets unconditionally so they take effect
        // at the moment LFO is turned OFF, not deferred to re-enable.
        lfoX_.applyPhaseResetIfPending();
        lfoY_.applyPhaseResetIfPending();

        // Sister modulation: capture last frame's ramped Y output before any mutations.
        // LFO Y→X uses this (one-frame delay, negligible). LFO X→Y uses same-frame xTarget.
        const float prevYRampOut = lfoYRampOut_;
        const int xSisterDest = (int)*apvts.getRawParameterValue("lfoXSister"); // X→Y target
        const int ySisterDest = (int)*apvts.getRawParameterValue("lfoYSister"); // Y→X target
        const float xSisterAtten = *apvts.getRawParameterValue("lfoXSisterAtten");
        const float ySisterAtten = *apvts.getRawParameterValue("lfoYSisterAtten");

        // Rate range used for free-mode normalized modulation (mirrors createParameterLayout).
        static const juce::NormalisableRange<float> kLfoRateRange(0.01f, 20.0f, 0.0f, 0.35f);

        // Applies sister LFO modulation to a ProcessParams before calling process().
        // modSignal is already scaled to [-srcLevel, +srcLevel] by the source LFO.
        auto applySisterMod = [&](ProcessParams& p, int dest, float modSignal)
        {
            if (dest == 0) return; // None
            switch (dest)
            {
                case 1: // Rate — normalized log space (free) or exp2 multiplier (sync)
                    if (p.syncMode)
                        p.subdivBeats *= std::exp2(modSignal * 3.0f); // ±3 octaves at level=1
                    else
                    {
                        float norm = kLfoRateRange.convertTo0to1(p.rateHz);
                        norm = juce::jlimit(0.0f, 1.0f, norm + modSignal);
                        p.rateHz = kLfoRateRange.convertFrom0to1(norm);
                    }
                    break;
                case 2: // Phase — additive offset (±180° at level=1), wraps 0..1
                    p.phaseShift = std::fmod(p.phaseShift + modSignal * 0.5f + 2.0f, 1.0f);
                    break;
                case 3: // Level — multiplicative AM, clamped 0..1
                    p.level = juce::jlimit(0.0f, 1.0f, p.level * (1.0f + modSignal));
                    break;
                case 4: // Dist — multiplicative, clamped 0..1
                    p.distortion = juce::jlimit(0.0f, 1.0f, p.distortion * (1.0f + modSignal));
                    break;
                default: break;
            }
        };

        // Compute ramp targets — process only if enabled and level > 0
        // Per CONTEXT.md: at level=0, phase does NOT advance (bypass path).
        float xTarget = 0.0f;
        if (xEnabled && xLevel > 0.0f)
        {
            ProcessParams xp;
            xp.sampleRate   = sampleRate_;
            xp.blockSize    = blockSize;
            xp.syncMode     = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
            // Sync ON  → phase-lock to looper beat; looper is the clock source.
            // Sync OFF → free Hz, completely independent of looper/DAW (analog feel).
            if (xp.syncMode)
            {
                const bool looperOn = looper_.isPlaying();
                xp.bpm          = effectiveBpm_.load(std::memory_order_relaxed);
                xp.ppqPosition  = looperOn ? looper_.getPlaybackBeat() : -1.0;
                xp.isDawPlaying = looperOn;
            }
            else
            {
                xp.bpm          = lp.bpm;
                xp.ppqPosition  = ppqPos;
                xp.isDawPlaying = isDawPlaying;
            }
            xp.rateHz       = (lfoXRateOverride_  >= 0.0f) ? lfoXRateOverride_
                                                           : *apvts.getRawParameterValue(ParamID::lfoXRate);
            {
                const int  xCurMode = (int)apvts.getRawParameterValue("filterXMode")->load();
                xp.subdivBeats = kLfoSubdivBeats[
                    juce::jlimit(0, kMaxSubdivIdx, (int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv))];
                if (xp.syncMode && xCurMode == 0)
                    xp.subdivBeats *= lfoXSubdivMult_.load(std::memory_order_relaxed);
            }
            xp.maxCycleBeats = xp.syncMode
                ? std::max(1.0, looper_.getLoopLengthBeats())
                : 16.0;
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
            // Sister mod: LFO Y modulates LFO X using last frame's ramped Y output.
            applySisterMod(xp, ySisterDest, prevYRampOut * ySisterAtten);
            xTarget = lfoX_.process(xp);
            lfoXOutputDisplay_.store(xTarget, std::memory_order_relaxed);
        }
        else
        {
            lfoXOutputDisplay_.store(0.0f, std::memory_order_relaxed);
        }

        float yTarget = 0.0f;
        if (yEnabled && yLevel > 0.0f)
        {
            ProcessParams yp;
            yp.sampleRate   = sampleRate_;
            yp.blockSize    = blockSize;
            yp.syncMode     = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
            // Sync ON  → phase-lock to looper beat; looper is the clock source.
            // Sync OFF → free Hz, completely independent of looper/DAW (analog feel).
            if (yp.syncMode)
            {
                const bool looperOn = looper_.isPlaying();
                yp.bpm          = effectiveBpm_.load(std::memory_order_relaxed);
                yp.ppqPosition  = looperOn ? looper_.getPlaybackBeat() : -1.0;
                yp.isDawPlaying = looperOn;
            }
            else
            {
                yp.bpm          = lp.bpm;
                yp.ppqPosition  = ppqPos;
                yp.isDawPlaying = isDawPlaying;
            }
            yp.rateHz       = (lfoYRateOverride_  >= 0.0f) ? lfoYRateOverride_
                                                           : *apvts.getRawParameterValue(ParamID::lfoYRate);
            {
                const int  yCurMode = (int)apvts.getRawParameterValue("filterYMode")->load();
                yp.subdivBeats = kLfoSubdivBeats[
                    juce::jlimit(0, kMaxSubdivIdx, (int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv))];
                if (yp.syncMode && yCurMode == 0)
                    yp.subdivBeats *= lfoYSubdivMult_.load(std::memory_order_relaxed);
            }
            yp.maxCycleBeats = yp.syncMode
                ? std::max(1.0, looper_.getLoopLengthBeats())
                : 16.0;
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
            // Sister mod: LFO X modulates LFO Y using this frame's xTarget (same-frame, clean).
            applySisterMod(yp, xSisterDest, xTarget * xSisterAtten);
            yTarget = lfoY_.process(yp);
            lfoYOutputDisplay_.store(yTarget, std::memory_order_relaxed);
        }
        else
        {
            lfoYOutputDisplay_.store(0.0f, std::memory_order_relaxed);
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
        // Cursor link: when OFF the LFO still runs and sends CC but does not move cursor/pitch.
        // (JoystickPad::paint() applies its own dotR-aware bounds clamp.)
        const bool xLinked = *apvts.getRawParameterValue("lfoXCursorLink") > 0.5f;
        const bool yLinked = *apvts.getRawParameterValue("lfoYCursorLink") > 0.5f;
        modulatedJoyX_.store(chordP.joystickX + (xLinked ? lfoXRampOut_ : 0.0f), std::memory_order_relaxed);
        modulatedJoyY_.store(chordP.joystickY + (yLinked ? lfoYRampOut_ : 0.0f), std::memory_order_relaxed);

        chordP.joystickX = std::clamp(chordP.joystickX + (xLinked ? lfoXRampOut_ : 0.0f), -1.0f, 1.0f);
        chordP.joystickY = std::clamp(chordP.joystickY + (yLinked ? lfoYRampOut_ : 0.0f), -1.0f, 1.0f);
    }
    // ─────────────────────────────────── end LFO ──────────────────────────────

    // ── Live pitch for crosshair display — uses fully-resolved chordP (post-LFO) ─
    {
        for (int v = 0; v < 4; ++v)
        {
            const int p = ChordEngine::computePitch(v, chordP);
            switch (v)
            {
                case 0: livePitch0_.store(p, std::memory_order_relaxed); break;
                case 1: livePitch1_.store(p, std::memory_order_relaxed); break;
                case 2: livePitch2_.store(p, std::memory_order_relaxed); break;
                case 3: livePitch3_.store(p, std::memory_order_relaxed); break;
            }
        }
    }

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
        // Only reset LFOs that are in sync mode — free-mode LFOs run like analog
        // hardware and must not snap to phase 0 on every DAW transport press.
        const bool xSyncOn = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
        const bool ySyncOn = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
        if (xSyncOn) { lfoX_.reset(); lfoXRampOut_ = 0.0f; }
        if (ySyncOn) { lfoY_.reset(); lfoYRampOut_ = 0.0f; }
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
                    looperActivePitch_[v] = -1;
                }
                // Looper sub-octave note-off on looper stop
                const int subPitch = looperActiveSubPitch_[v];
                if (subPitch >= 0)
                {
                    const int ch = looperActiveCh_[v];
                    if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                        midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
                }
            }
            // Do NOT call resetNoteCount() here — pad-held notes still have live counts.
            // Individual looper note-offs above already decremented their entries correctly.
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
                looperActivePitch_[v] = -1;
            }
            // Looper sub-octave note-off on looper reset
            const int subPitch = looperActiveSubPitch_[v];
            if (subPitch >= 0)
            {
                const int ch = looperActiveCh_[v];
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
            }
        }
        // Do NOT call resetNoteCount() — pad-held note counts must stay intact across looper reset.
        trigger_.syncRandomFreePhases();  // re-align all random-free voices to bar 0
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
            voiceTriggerFlash_[v].fetch_add(1, std::memory_order_relaxed);
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
            looperActivePitch_[v] = -1;

            // Looper sub-octave note-off using snapshot
            const int subPitch = looperActiveSubPitch_[v];
            if (subPitch >= 0)
            {
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), 0);
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

            // Sub-octave note-off — always use snapshot, never heldPitch_[voice] - 12
            const int subPitch = subHeldPitch_[voice];
            if (subPitch >= 0)
            {
                if (noteCount_[ch - 1][subPitch] > 0 && --noteCount_[ch - 1][subPitch] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, subPitch, (uint8_t)0), sampleOff);
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
        const bool gateFromJoystick = (xGateMode == 3 || yGateMode == 3);
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
    tp.randomSyncMode = static_cast<int>(*apvts.getRawParameterValue("randomSyncMode"));
    tp.randomFreeTempo = *apvts.getRawParameterValue("randomFreeTempo");

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
        if (arpOn && !prevArpOn_)
            arpWaitingForPlay_ = true;   // always arm when ARP is enabled — wait for looper/DAW play
        if (!arpOn)
            arpWaitingForPlay_ = false;  // disabled — clear arm
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
    const bool arpClockOn = arpSyncOn ? isDawPlaying : looper_.isPlaying();
    if (!arpOn || !arpClockOn || arpWaitingForPlay_)
    {
        // Kill any hanging arp note when ARP is off or clock stops.
        // Always send note-off unconditionally — don't gate on noteCount, as another
        // path may have already decremented it, leaving the note orphaned.
        if (arpActivePitch_ >= 0 && arpActiveCh_ > 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(arpActiveCh_, arpActivePitch_, (uint8_t)0), 0);
            if (noteCount_[arpActiveCh_ - 1][arpActivePitch_] > 0)
                --noteCount_[arpActiveCh_ - 1][arpActivePitch_];
            arpActivePitch_ = -1;
            arpActiveVoice_ = -1;
        }
        arpCurrentVoice_.store(-1, std::memory_order_relaxed);
        arpNoteOffRemaining_ = 0.0;
        arpStep_           = 0;
        arpPhase_          = 0.0;
        arpPrevSubdivIdx_  = -1;
    }
    else
    {
        // Subdivision interval in beats (1 beat = 1 quarter note).
        // Indices match RandomSubdiv enum + randomSubdivN APVTS ordering (Phase 35)
        static const double kSubdivBeats[17] = {
            16.0,        // 0: 4/1
            8.0,         // 1: 2/1
            4.0,         // 2: 1/1
            8.0/3.0,     // 3: 1/1T
            2.0,         // 4: 1/2
            4.0/3.0,     // 5: 1/2T
            1.0,         // 6: 1/4  (default)
            2.0/3.0,     // 7: 1/4T
            0.5,         // 8: 1/8
            1.0/3.0,     // 9: 1/8T
            0.25,        // 10: 1/16
            1.0/6.0,     // 11: 1/16T
            0.125,       // 12: 1/32
            1.0/12.0,    // 13: 1/32T
            0.0625,      // 14: 1/64
            16.0/3.0,    // 15: 2/1T
            32.0/3.0     // 16: 4/1T
        };
        const int subdivIdx = juce::jlimit(0, 16,
            static_cast<int>(*apvts.getRawParameterValue(ParamID::arpSubdiv)));
        const double subdivBeats = kSubdivBeats[subdivIdx];

        const int orderIdx = juce::jlimit(0, 6,
            static_cast<int>(*apvts.getRawParameterValue(ParamID::arpOrder)));

        const double gateRatio = smoothedGateLength_.getCurrentValue();
        // Minimum 2% of subdivision (~5ms at 120BPM/8th) so slider fully closed = very short notes.
        const double gateBeats = subdivBeats * std::max(gateRatio, 0.02);

        const double beatsThisBlock = lp.bpm * static_cast<double>(blockSize)
                                      / (sampleRate_ * 60.0);

        // Gate-time note-off: fires mid-block when gate expires before next step.
        if (arpActivePitch_ >= 0 && arpNoteOffRemaining_ > 0.0)
        {
            arpNoteOffRemaining_ -= beatsThisBlock;
            if (arpNoteOffRemaining_ <= 0.0)
            {
                // Peek at the upcoming step state — if TIE, suppress noteOff and keep
                // arpActivePitch_ alive so the step loop can recognise isTie correctly.
                const int peekLen = juce::jlimit(1, 8,
                    static_cast<int>(*apvts.getRawParameterValue("arpLength")) + 1);
                const int peekState = static_cast<int>(
                    *apvts.getRawParameterValue("arpStepState" + juce::String(arpStep_ % peekLen)));

                if (peekState != 1)  // not TIE — fire noteOff normally
                {
                    const int ch = arpActiveCh_;
                    if (noteCount_[ch - 1][arpActivePitch_] > 0 &&
                        --noteCount_[ch - 1][arpActivePitch_] == 0)
                        midi.addEvent(juce::MidiMessage::noteOff(ch, arpActivePitch_, (uint8_t)0), 0);
                    arpActivePitch_ = -1;
                    arpActiveVoice_ = -1;
                    arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                }
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
            // Lock arp steps to DAW beat grid using start-of-block position.
            // Using ppqPos directly (not ppqPos + beatsThisBlock) avoids estimation
            // drift: end-of-block lookahead can pre-consume a boundary then produce
            // stepsToFire=0 on the block where ppqPos actually crosses it.
            // Backward-jump guard handles DAW loop/rewind without going silent.
            const auto curIdx = static_cast<long long>(ppqPos / subdivBeats);
            if (arpPrevSubdivIdx_ < 0 || curIdx < arpPrevSubdivIdx_)
                stepsToFire = 1;   // just launched OR backward jump (loop/rewind)
            else
                stepsToFire = static_cast<int>(curIdx - arpPrevSubdivIdx_);
            arpPrevSubdivIdx_ = curIdx;
            arpPhase_ = std::fmod(ppqPos, subdivBeats);
        }
        else if (!arpSyncOn && looper_.isPlaying())
        {
            // Lock arp steps to looper's internal beat position.
            // Same start-of-block + backward-jump logic (looper wraps to 0 on repeat).
            const double looperBeat = looper_.getPlaybackBeat();
            const auto curIdx = static_cast<long long>(looperBeat / subdivBeats);
            if (arpPrevSubdivIdx_ < 0 || curIdx < arpPrevSubdivIdx_)
                stepsToFire = 1;   // just launched OR loop wrapped
            else
                stepsToFire = static_cast<int>(curIdx - arpPrevSubdivIdx_);
            arpPrevSubdivIdx_ = curIdx;
            arpPhase_ = std::fmod(looperBeat, subdivBeats);
        }
        else
        {
            if (arpPrevSubdivIdx_ < 0)
            {
                stepsToFire = 1;
                arpPhase_ = 0.0;
                arpPrevSubdivIdx_ = 0;
            }
            else
            {
                arpPhase_ += beatsThisBlock;
                while (arpPhase_ >= subdivBeats) { arpPhase_ -= subdivBeats; ++stepsToFire; }
            }
        }

        for (int s = 0; s < stepsToFire; ++s)
        {
            // ── Read step-pattern state BEFORE step-boundary noteOff ──────────────
            const int arpLen  = juce::jlimit(1, 8,
                static_cast<int>(*apvts.getRawParameterValue("arpLength")) + 1);
            const int patStep = arpStep_ % arpLen;
            const int stepState = static_cast<int>(
                *apvts.getRawParameterValue("arpStepState" + juce::String(patStep)));
            // stepState: 0=ON, 1=TIE, 2=OFF

            // TIE with an active note: suppress step-boundary noteOff (sustain through this step)
            const bool isTie = (stepState == 1) && (arpActivePitch_ >= 0);

            // Cut any still-sounding note at step boundary — skipped for TIE.
            if (!isTie && arpActivePitch_ >= 0 && arpActiveVoice_ >= 0)
            {
                const int ch = arpActiveCh_;
                if (noteCount_[ch - 1][arpActivePitch_] > 0 &&
                    --noteCount_[ch - 1][arpActivePitch_] == 0)
                    midi.addEvent(juce::MidiMessage::noteOff(ch, arpActivePitch_, (uint8_t)0), 0);
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
            if (arpStep_ >= arpLen) arpStep_ = 0;

            const int voice = seq[arpStep_ % seqLen];
            arpStep_ = (arpStep_ + 1) % arpLen;

            // Store current step for UI highlighting
            arpCurrentStep_.store(patStep, std::memory_order_relaxed);

            // Skip voices where a pad is held (pad has priority).
            if (trigger_.isGateOpen(voice)) continue;

            // ── Apply step state ──────────────────────────────────────────────────
            if (stepState == 2)  // OFF: immediate note-off + skip note-on
            {
                if (arpActivePitch_ >= 0 && arpActiveCh_ > 0)
                {
                    if (noteCount_[arpActiveCh_ - 1][arpActivePitch_] > 0 &&
                        --noteCount_[arpActiveCh_ - 1][arpActivePitch_] == 0)
                        midi.addEvent(juce::MidiMessage::noteOff(arpActiveCh_, arpActivePitch_, (uint8_t)0), 0);
                    arpActivePitch_ = -1;
                    arpActiveVoice_ = -1;
                    arpCurrentVoice_.store(-1, std::memory_order_relaxed);
                    arpNoteOffRemaining_ = 0.0;
                }
                continue;
            }

            if (isTie)  // TIE: extend gate, no re-trigger
            {
                arpNoteOffRemaining_ = gateBeats;
                continue;
            }

            // stepState == ON (or TIE with no previous note → treat as ON) → existing note-on path
            const int pitch = heldPitch_[voice];
            const int ch = effectiveChannel(voice);
            arpActiveCh_ = ch;
            sentChannel_[voice] = ch;
            if (noteCount_[ch - 1][pitch]++ == 0)
                midi.addEvent(juce::MidiMessage::noteOn(ch, pitch, (uint8_t)100), 0);
            voiceTriggerFlash_[voice].fetch_add(1, std::memory_order_relaxed);
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
            looper_.recordFilter(beatPos, filtX(), filtY());
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
            // Reset CCs to their base-knob resting positions, NOT to 0.
            // Sending CC74=0 on disconnect would close the synth filter permanently
            // (cutoff at minimum), making the synth appear silent until the user
            // manually opens the filter again. Use 64+offset so the synth returns
            // to whatever the MOD FIX base knob is set to.
            const float xOff = apvts.getRawParameterValue(ParamID::filterXOffset)->load();
            const float yOff = apvts.getRawParameterValue(ParamID::filterYOffset)->load();
            const int ccCutRest = juce::jlimit(0, 127, (int)(64.0f + xOff));
            const int ccResRest = juce::jlimit(0, 127, (int)(64.0f + yOff));
            // Send to all modes' CC numbers (X and Y may map to any of these):
            // mode 0: CC74 (cut) / CC71 (res); mode 1: CC12/CC76; etc.
            // For CC74 and CC71, use the base-knob rest value to avoid closing the filter.
            // For CC12, CC1, CC76 (LFO aux), send 0 (no analog base knob).
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCutRest), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, ccResRest), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 12, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh,  1, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 76, 0), 0);
            prevCcCut_.store(ccCutRest, std::memory_order_relaxed);
            prevCcRes_.store(ccResRest, std::memory_order_relaxed);
            prevLfoCcX_ = prevLfoCcY_ = -1;
        }

        // Determine filter source — all gated by the filterMod on/off toggle.
        const bool filterModOn   = filterModActive_.load(std::memory_order_relaxed);
        const bool filterRecOn      = looper_.isRecFilter();     // REC FILTER button state
        const bool filterHasContent = looper_.hasFilterContent(); // any FilterX/Y events recorded
        const bool looperDriving = filterModOn && filterRecOn && filterHasContent
                                   && (loopOut.hasFilterX || loopOut.hasFilterY);
        // liveGamepad is true when either a physical controller is active OR the user is
        // mouse-dragging the left stick in the controller illustration (no controller needed).
        const bool liveGamepad   = filterModOn && ((gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed))
                                                   || gamepad_.isMouseFilterActive());

        // Base knobs are the primary modulator and send CC independently of the joystick.
        // Joystick/looper updates are still gated by liveGamepad||looperDriving, but
        // the outer block is gated only by filterModOn so base-knob turns always reach here.
        if (filterModOn)
        {
            const float xAtten = apvts.getRawParameterValue(ParamID::filterXAtten)->load();
            const float yAtten = apvts.getRawParameterValue(ParamID::filterYAtten)->load();

            const int xMode  = (int)apvts.getRawParameterValue("filterXMode")->load();
            const int yMode  = (int)apvts.getRawParameterValue("filterYMode")->load();
            // X/Y: indices 0-6 are LFO/Gate targets (-1 = no CC); 7-24 are named CC modes; 25=Custom CC.
            const int filterXCustom = (int)apvts.getRawParameterValue("filterXCustomCc")->load();
            const int filterYCustom = (int)apvts.getRawParameterValue("filterYCustomCc")->load();
            const int ccXnum = resolveFilterCcMode(xMode, filterXCustom);
            const int ccYnum = resolveFilterCcMode(yMode, filterYCustom);

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
                                   : ((liveGamepad && (!looperPlaying || !filterRecOn || recActive || !filterHasContent)) ? filtX() : prevFilterX_);
                const float newGfy = (filterRecOn && !recActive && filterHasContent && loopOut.hasFilterY) ? loopOut.filterY
                                   : ((liveGamepad && (!looperPlaying || !filterRecOn || recActive || !filterHasContent)) ? filtY() : prevFilterY_);

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
            const bool baseChanged = (xOffset != prevBaseX_) || (yOffset != prevBaseY_)
                                   || (xAtten != prevAttenX_) || (yAtten != prevAttenY_);

            // INV swaps which physical axis drives which CC channel.
            // Swap offset+atten so the knob that is labeled for axis A actually scales axis A.
            const float effXOffset = blkInv ? yOffset : xOffset;
            const float effYOffset = blkInv ? xOffset : yOffset;
            const float effXAtten  = blkInv ? yAtten  : xAtten;
            const float effYAtten  = blkInv ? xAtten  : yAtten;

            if (stickUpdated || baseChanged)
            {
                prevBaseX_  = xOffset;
                prevBaseY_  = yOffset;
                prevAttenX_ = xAtten;
                prevAttenY_ = yAtten;

                // xOffset/yOffset are now -50..+50 centred at 0; add to MIDI centre 64.
                const float ccCutF = juce::jlimit(0.0f, 127.0f, 64.0f + effXOffset + prevFilterX_ * 63.5f * (effXAtten / 100.0f));
                const float ccResF = juce::jlimit(0.0f, 127.0f, 64.0f + effYOffset + prevFilterY_ * 63.5f * (effYAtten / 100.0f));

                // On-load silent init: snap smoother to first value so no ramp from 64 on first use.
                if (prevCcCut_.load(std::memory_order_relaxed) == -2)
                {
                    smoothedCcCut_.setCurrentAndTargetValue(ccCutF);
                    prevCcCut_.store((int)std::roundf(ccCutF), std::memory_order_relaxed);
                }
                else
                    smoothedCcCut_.setTargetValue(ccCutF);

                if (prevCcRes_.load(std::memory_order_relaxed) == -2)
                {
                    smoothedCcRes_.setCurrentAndTargetValue(ccResF);
                    prevCcRes_.store((int)std::roundf(ccResF), std::memory_order_relaxed);
                }
                else
                    smoothedCcRes_.setTargetValue(ccResF);
            }
            // Advance smoothers every block so CC ramps continue between stick updates.
            smoothedCcCut_.skip(blockSize);
            smoothedCcRes_.skip(blockSize);
            if (stickUpdated || baseChanged || smoothedCcCut_.isSmoothing() || smoothedCcRes_.isSmoothing())
            {
                const int ccCut = (int)std::roundf(smoothedCcCut_.getCurrentValue());
                const int ccRes = (int)std::roundf(smoothedCcRes_.getCurrentValue());
                if (ccXnum >= 0)
                {
                    const int prev = prevCcCut_.load(std::memory_order_relaxed);
                    if (prev == -1 || ccCut != prev)
                    {
                        midi.addEvent(juce::MidiMessage::controllerEvent(fCh, ccXnum, ccCut), 0);
                        prevCcCut_.store(ccCut, std::memory_order_relaxed);
                    }
                }
                if (ccYnum >= 0)
                {
                    const int prev = prevCcRes_.load(std::memory_order_relaxed);
                    if (prev == -1 || ccRes != prev)
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
            // SWAP: right stick (getPitchX/Y) drives filter in swap mode.
            // INV:  X↔Y axes swapped.
            {
                const float rawLx = blkSwap ? gamepad_.getPitchX() : gamepad_.getLeftStickXDisplay();
                const float rawLy = blkSwap ? gamepad_.getPitchY() : gamepad_.getLeftStickYDisplay();
                const float lx = blkInv ? rawLy : rawLx;
                const float ly = blkInv ? rawLx : rawLy;
                leftStickXDisplay_.store(lx, std::memory_order_relaxed);
                leftStickYDisplay_.store(ly, std::memory_order_relaxed);
                filterXOffsetDisplay_.store(
                    juce::jlimit(-50.0f, 50.0f, effXOffset + lx * 63.5f * (effXAtten / 100.0f)),
                    std::memory_order_relaxed);
                filterYOffsetDisplay_.store(
                    juce::jlimit(-50.0f, 50.0f, effYOffset + ly * 63.5f * (effYAtten / 100.0f)),
                    std::memory_order_relaxed);
            }

            // ── LFO / Gate Length dispatch (indices 2–5) ──────────────────────────
            // Reset all override floats unconditionally — only the active mode sets them.
            // This ensures that when the dropdown switches away from an LFO target, the
            // LFO params block reverts to APVTS immediately (no stale override).
            lfoXRateOverride_ = lfoXPhaseOverride_ = lfoXLevelOverride_ = -1.0f;
            lfoYRateOverride_ = lfoYPhaseOverride_ = lfoYLevelOverride_ = -1.0f;

            // Use display stick position so that mouse-drag overrides (from the controller
            // illustration) are included alongside physical hardware values.
            // getLeftStickXDisplay/YDisplay() returns the mouse override when the physical
            // stick is still (below bypass threshold), otherwise returns the hardware value.
            if (liveGamepad)
            {
                constexpr float kDeadzone = 0.12f;
                const float rawLiveX = blkSwap ? gamepad_.getPitchX() : gamepad_.getLeftStickXDisplay();
                const float rawLiveY = blkSwap ? gamepad_.getPitchY() : gamepad_.getLeftStickYDisplay();
                const float liveX = blkInv ? rawLiveY : rawLiveX;
                const float liveY = blkInv ? rawLiveX : rawLiveY;

                if (xMode < kNumLfoTargetModes)
                {
                    const float deadX = (std::abs(liveX) < kDeadzone) ? 0.0f : liveX;
                    const float stick = deadX * (xAtten / 100.0f);

                    switch (xMode)
                    {
                        case 0:  // LFO-X Freq
                        {
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
                            if (syncOn)
                            {
                                // MOD FIX is the full base: -50..+50 → index 0..17.
                                // Stick shifts ±6 steps around that base.
                                const float baseIdxF = (xOffset + 50.0f) / 100.0f * 17.0f;
                                const int effectiveIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)std::roundf(baseIdxF + stick * 6.0f));
                                const int curIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv));
                                lfoXSubdivMult_.store(
                                    (float)(kLfoSubdivBeats[effectiveIdx] / kLfoSubdivBeats[curIdx]),
                                    std::memory_order_relaxed);
                                lfoXRateDisplay_.store((float)effectiveIdx, std::memory_order_relaxed);
                            }
                            else
                            {
                                // Additive: MOD FIX offsets around current Rate knob position.
                                // At noon (offset=0) → no jump. ±50 → ±0.5 normalized shift.
                                static const juce::NormalisableRange<float> kLfoRange(0.01f, 20.0f, 0.0f, 0.35f);
                                const float base = apvts.getRawParameterValue(ParamID::lfoXRate)->load();
                                const float norm = kLfoRange.convertTo0to1(base);
                                const float actual = kLfoRange.convertFrom0to1(
                                    juce::jlimit(0.0f, 1.0f, norm + (xOffset / 50.0f) * 0.5f + stick * 0.5f));
                                lfoXRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoXRateOverride_ = actual;
                            }
                            break;
                        }
                        case 1:  // LFO-X Phase (0–360°)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + (xOffset / 50.0f) * 180.0f + stick * 180.0f);
                            lfoXPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXPhaseOverride_ = actual;
                            break;
                        }
                        case 2:  // LFO-X Level (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + (xOffset / 50.0f) * 0.5f + stick * 0.5f);
                            lfoXLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXLevelOverride_ = actual;
                            break;
                        }
                        case 3:  // Gate Length (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::gateLength)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            gateLengthDisplay_.store(actual, std::memory_order_relaxed);
                            break;
                        }
                        case 4:  // LFO-Y Freq (X stick -> opposite LFO)
                        {
                            if (lfoY_.getRecState() == LfoRecState::Playback) break;
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
                            if (syncOn)
                            {
                                const int curIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv));
                                const int effectiveIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)std::roundf((float)curIdx + stick * 6.0f));
                                lfoYSubdivMult_.store(
                                    (float)(kLfoSubdivBeats[effectiveIdx] / kLfoSubdivBeats[curIdx]),
                                    std::memory_order_relaxed);
                                lfoYRateDisplay_.store((float)effectiveIdx, std::memory_order_relaxed);
                            }
                            else
                            {
                                static const juce::NormalisableRange<float> kLfoRange(0.01f, 20.0f, 0.0f, 0.35f);
                                const float base = apvts.getRawParameterValue(ParamID::lfoYRate)->load();
                                const float norm = kLfoRange.convertTo0to1(base);
                                const float actual = kLfoRange.convertFrom0to1(
                                    juce::jlimit(0.0f, 1.0f, norm + (xOffset / 50.0f) * 0.5f + stick * 0.5f));
                                lfoYRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoYRateOverride_ = actual;
                            }
                            break;
                        }
                        case 5:  // LFO-Y Phase (X stick -> opposite LFO)
                        {
                            if (lfoY_.getRecState() == LfoRecState::Playback) break;
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + (xOffset / 50.0f) * 180.0f + stick * 180.0f);
                            lfoYPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYPhaseOverride_ = actual;
                            break;
                        }
                        case 6: // LFO-Y Level (X stick -> opposite LFO)
                        {
                            if (lfoY_.getRecState() == LfoRecState::Playback) break;
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + (xOffset / 50.0f) * 0.5f + stick * 0.5f);
                            lfoYLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYLevelOverride_ = actual;
                            break;
                        }
                        default: break;
                    }
                }

                if (yMode < kNumLfoTargetModes)
                {
                    const float deadY = (std::abs(liveY) < kDeadzone) ? 0.0f : liveY;
                    const float stick = deadY * (yAtten / 100.0f);

                    switch (yMode)
                    {
                        case 0:  // LFO-Y Freq
                        {
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f;
                            if (syncOn)
                            {
                                const float baseIdxF = (yOffset + 50.0f) / 100.0f * 17.0f;
                                const int effectiveIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)std::roundf(baseIdxF + stick * 6.0f));
                                const int curIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv));
                                lfoYSubdivMult_.store(
                                    (float)(kLfoSubdivBeats[effectiveIdx] / kLfoSubdivBeats[curIdx]),
                                    std::memory_order_relaxed);
                                lfoYRateDisplay_.store((float)effectiveIdx, std::memory_order_relaxed);
                            }
                            else
                            {
                                // Additive: MOD FIX offsets around current Rate knob position.
                                static const juce::NormalisableRange<float> kLfoRange(0.01f, 20.0f, 0.0f, 0.35f);
                                const float base = apvts.getRawParameterValue(ParamID::lfoYRate)->load();
                                const float norm = kLfoRange.convertTo0to1(base);
                                const float actual = kLfoRange.convertFrom0to1(
                                    juce::jlimit(0.0f, 1.0f, norm + (yOffset / 50.0f) * 0.5f + stick * 0.5f));
                                lfoYRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoYRateOverride_ = actual;
                            }
                            break;
                        }
                        case 1:  // LFO-Y Phase (0–360°)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + (yOffset / 50.0f) * 180.0f + stick * 180.0f);
                            lfoYPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYPhaseOverride_ = actual;
                            break;
                        }
                        case 2:  // LFO-Y Level (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::lfoYLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + (yOffset / 50.0f) * 0.5f + stick * 0.5f);
                            lfoYLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYLevelOverride_ = actual;
                            break;
                        }
                        case 3:  // Gate Length (0–1)
                        {
                            const float base   = apvts.getRawParameterValue(ParamID::gateLength)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + stick * 0.5f);
                            gateLengthDisplay_.store(actual, std::memory_order_relaxed);
                            break;
                        }
                        case 4:  // LFO-X Freq (Y stick -> opposite LFO)
                        {
                            if (lfoX_.getRecState() == LfoRecState::Playback) break;
                            const bool syncOn = *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f;
                            if (syncOn)
                            {
                                const int curIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv));
                                const int effectiveIdx = juce::jlimit(0, kMaxSubdivIdx,
                                    (int)std::roundf((float)curIdx + stick * 6.0f));
                                lfoXSubdivMult_.store(
                                    (float)(kLfoSubdivBeats[effectiveIdx] / kLfoSubdivBeats[curIdx]),
                                    std::memory_order_relaxed);
                                lfoXRateDisplay_.store((float)effectiveIdx, std::memory_order_relaxed);
                            }
                            else
                            {
                                static const juce::NormalisableRange<float> kLfoRange(0.01f, 20.0f, 0.0f, 0.35f);
                                const float base = apvts.getRawParameterValue(ParamID::lfoXRate)->load();
                                const float norm = kLfoRange.convertTo0to1(base);
                                const float actual = kLfoRange.convertFrom0to1(
                                    juce::jlimit(0.0f, 1.0f, norm + (yOffset / 50.0f) * 0.5f + stick * 0.5f));
                                lfoXRateDisplay_.store(actual, std::memory_order_relaxed);
                                lfoXRateOverride_ = actual;
                            }
                            break;
                        }
                        case 5:  // LFO-X Phase (Y stick -> opposite LFO)
                        {
                            if (lfoX_.getRecState() == LfoRecState::Playback) break;
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXPhase)->load();
                            const float actual = juce::jlimit(0.0f, 360.0f, base + (yOffset / 50.0f) * 180.0f + stick * 180.0f);
                            lfoXPhaseDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXPhaseOverride_ = actual;
                            break;
                        }
                        case 6: // LFO-X Level (Y stick -> opposite LFO)
                        {
                            if (lfoX_.getRecState() == LfoRecState::Playback) break;
                            const float base   = apvts.getRawParameterValue(ParamID::lfoXLevel)->load();
                            const float actual = juce::jlimit(0.0f, 1.0f, base + (yOffset / 50.0f) * 0.5f + stick * 0.5f);
                            lfoXLevelDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXLevelOverride_ = actual;
                            break;
                        }
                        default: break;
                    }
                }

                // Reset subdivision multiplier when not in sync-mode LFO Freq target
                // lfoXSubdivMult_: driven by xMode==0 (X->LFO-X sync) OR yMode==4 (Y->LFO-X sync)
                if (xMode != 0 && yMode != 4) lfoXSubdivMult_.store(1.0f, std::memory_order_relaxed);
                // lfoYSubdivMult_: driven by yMode==0 (Y->LFO-Y sync) OR xMode==4 (X->LFO-Y sync)
                if (yMode != 0 && xMode != 4) lfoYSubdivMult_.store(1.0f, std::memory_order_relaxed);
            }

            // When gate mode is selected but joystick is idle, keep gateLengthDisplay_
            // in sync with the APVTS knob so the arp Gate Length slider always works.
            if (!liveGamepad && (xMode == 3 || yMode == 3))
                gateLengthDisplay_.store(
                    apvts.getRawParameterValue(ParamID::gateLength)->load(),
                    std::memory_order_relaxed);

            // When not modulating via joystick, reset sync rate display to APVTS value
            // so the rate slider shows the correct un-modulated position.
            if (!liveGamepad)
            {
                if (xMode == 0 && *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f)
                    lfoXRateDisplay_.store(
                        (float)(int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv),
                        std::memory_order_relaxed);
                if (yMode == 0 && *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f)
                    lfoYRateDisplay_.store(
                        (float)(int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv),
                        std::memory_order_relaxed);
                // Cross-LFO: X stick drives LFO-Y Freq sync -> reset lfoYRateDisplay_ when idle
                if (xMode == 4 && *apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f)
                    lfoYRateDisplay_.store(
                        (float)(int)*apvts.getRawParameterValue(ParamID::lfoYSubdiv),
                        std::memory_order_relaxed);
                // Cross-LFO: Y stick drives LFO-X Freq sync -> reset lfoXRateDisplay_ when idle
                if (yMode == 4 && *apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f)
                    lfoXRateDisplay_.store(
                        (float)(int)*apvts.getRawParameterValue(ParamID::lfoXSubdiv),
                        std::memory_order_relaxed);
            }

            // No-gamepad display tracking: when filterMod is ON but stick is idle (no gamepad or
            // stick in deadzone), MOD FIX knob turns still update all LFO display atoms.
            // Uses additive model: base APVTS value + (offset/50)*sensitivity. At noon (offset=0)
            // effective == base, so switching modes never causes a jump.
            if (!liveGamepad && filterModActive_.load(std::memory_order_relaxed))
            {
                const float xOffset = apvts.getRawParameterValue(ParamID::filterXOffset)->load();
                const float yOffset = apvts.getRawParameterValue(ParamID::filterYOffset)->load();
                static const juce::NormalisableRange<float> kLfoRange(0.01f, 20.0f, 0.0f, 0.35f);

                // ── X mode ─────────────────────────────────────────────────────────────
                switch (xMode)
                {
                    case 0:  // X -> LFO-X Freq
                        if (!(*apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f))
                        {
                            const float base = apvts.getRawParameterValue(ParamID::lfoXRate)->load();
                            const float norm = kLfoRange.convertTo0to1(base);
                            const float actual = kLfoRange.convertFrom0to1(
                                juce::jlimit(0.0f, 1.0f, norm + (xOffset / 50.0f) * 0.5f));
                            lfoXRateDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXRateOverride_ = actual;
                        }
                        break;
                    case 1:  // X -> LFO-X Phase
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoXPhase)->load();
                        const float actual = juce::jlimit(0.0f, 360.0f, base + (xOffset / 50.0f) * 180.0f);
                        lfoXPhaseDisplay_.store(actual, std::memory_order_relaxed);
                        lfoXPhaseOverride_ = actual;
                        break;
                    }
                    case 2:  // X -> LFO-X Level
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoXLevel)->load();
                        const float actual = juce::jlimit(0.0f, 1.0f, base + (xOffset / 50.0f) * 0.5f);
                        lfoXLevelDisplay_.store(actual, std::memory_order_relaxed);
                        lfoXLevelOverride_ = actual;
                        break;
                    }
                    case 4:  // X -> LFO-Y Freq (cross-axis)
                        if (!(*apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f))
                        {
                            const float base = apvts.getRawParameterValue(ParamID::lfoYRate)->load();
                            const float norm = kLfoRange.convertTo0to1(base);
                            const float actual = kLfoRange.convertFrom0to1(
                                juce::jlimit(0.0f, 1.0f, norm + (xOffset / 50.0f) * 0.5f));
                            lfoYRateDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYRateOverride_ = actual;
                        }
                        break;
                    case 5:  // X -> LFO-Y Phase (cross-axis)
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoYPhase)->load();
                        const float actual = juce::jlimit(0.0f, 360.0f, base + (xOffset / 50.0f) * 180.0f);
                        lfoYPhaseDisplay_.store(actual, std::memory_order_relaxed);
                        lfoYPhaseOverride_ = actual;
                        break;
                    }
                    case 6: // X -> LFO-Y Level (cross-axis)
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoYLevel)->load();
                        const float actual = juce::jlimit(0.0f, 1.0f, base + (xOffset / 50.0f) * 0.5f);
                        lfoYLevelDisplay_.store(actual, std::memory_order_relaxed);
                        lfoYLevelOverride_ = actual;
                        break;
                    }
                    default: break;
                }

                // ── Y mode ─────────────────────────────────────────────────────────────
                switch (yMode)
                {
                    case 0:  // Y -> LFO-Y Freq
                        if (!(*apvts.getRawParameterValue(ParamID::lfoYSync) > 0.5f))
                        {
                            const float base = apvts.getRawParameterValue(ParamID::lfoYRate)->load();
                            const float norm = kLfoRange.convertTo0to1(base);
                            const float actual = kLfoRange.convertFrom0to1(
                                juce::jlimit(0.0f, 1.0f, norm + (yOffset / 50.0f) * 0.5f));
                            lfoYRateDisplay_.store(actual, std::memory_order_relaxed);
                            lfoYRateOverride_ = actual;
                        }
                        break;
                    case 1:  // Y -> LFO-Y Phase
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoYPhase)->load();
                        const float actual = juce::jlimit(0.0f, 360.0f, base + (yOffset / 50.0f) * 180.0f);
                        lfoYPhaseDisplay_.store(actual, std::memory_order_relaxed);
                        lfoYPhaseOverride_ = actual;
                        break;
                    }
                    case 2:  // Y -> LFO-Y Level
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoYLevel)->load();
                        const float actual = juce::jlimit(0.0f, 1.0f, base + (yOffset / 50.0f) * 0.5f);
                        lfoYLevelDisplay_.store(actual, std::memory_order_relaxed);
                        lfoYLevelOverride_ = actual;
                        break;
                    }
                    case 4:  // Y -> LFO-X Freq (cross-axis)
                        if (!(*apvts.getRawParameterValue(ParamID::lfoXSync) > 0.5f))
                        {
                            const float base = apvts.getRawParameterValue(ParamID::lfoXRate)->load();
                            const float norm = kLfoRange.convertTo0to1(base);
                            const float actual = kLfoRange.convertFrom0to1(
                                juce::jlimit(0.0f, 1.0f, norm + (yOffset / 50.0f) * 0.5f));
                            lfoXRateDisplay_.store(actual, std::memory_order_relaxed);
                            lfoXRateOverride_ = actual;
                        }
                        break;
                    case 5:  // Y -> LFO-X Phase (cross-axis)
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoXPhase)->load();
                        const float actual = juce::jlimit(0.0f, 360.0f, base + (yOffset / 50.0f) * 180.0f);
                        lfoXPhaseDisplay_.store(actual, std::memory_order_relaxed);
                        lfoXPhaseOverride_ = actual;
                        break;
                    }
                    case 6: // Y -> LFO-X Level (cross-axis)
                    {
                        const float base   = apvts.getRawParameterValue(ParamID::lfoXLevel)->load();
                        const float actual = juce::jlimit(0.0f, 1.0f, base + (yOffset / 50.0f) * 0.5f);
                        lfoXLevelDisplay_.store(actual, std::memory_order_relaxed);
                        lfoXLevelOverride_ = actual;
                        break;
                    }
                    default: break;
                }
            }
        }
    }

    // ── LFO direct CC output ──────────────────────────────────────────────────
    {
        const int fCh    = effectiveFilterChannel();
        const bool xOn   = *apvts.getRawParameterValue("lfoXEnabled") > 0.5f;
        const bool yOn   = *apvts.getRawParameterValue("lfoYEnabled") > 0.5f;
        const int lfoXCustom = (int)apvts.getRawParameterValue("lfoXCustomCc")->load();
        const int lfoYCustom = (int)apvts.getRawParameterValue("lfoYCustomCc")->load();
        const int xCcNum = ccDestToNumber((int)*apvts.getRawParameterValue("lfoXCcDest"), lfoXCustom);
        const int yCcNum = ccDestToNumber((int)*apvts.getRawParameterValue("lfoYCcDest"), lfoYCustom);

        // LFO oscillates around the joystick's current CC position rather than a fixed 64.
        // Determine which filter-joystick axis maps to each CC number.
        const int  xFMode  = (int)apvts.getRawParameterValue("filterXMode")->load();
        const int  yFMode  = (int)apvts.getRawParameterValue("filterYMode")->load();
        const int  fxCustom = (int)apvts.getRawParameterValue("filterXCustomCc")->load();
        const int  fyCustom = (int)apvts.getRawParameterValue("filterYCustomCc")->load();
        const int  ccXfilt = resolveFilterCcMode(xFMode, fxCustom);
        const int  ccYfilt = resolveFilterCcMode(yFMode, fyCustom);
        const float xFAtten  = apvts.getRawParameterValue(ParamID::filterXAtten)->load() / 100.0f;
        const float yFAtten  = apvts.getRawParameterValue(ParamID::filterYAtten)->load() / 100.0f;
        const float xFOffset = apvts.getRawParameterValue(ParamID::filterXOffset)->load();
        const float yFOffset = apvts.getRawParameterValue(ParamID::filterYOffset)->load();
        const float safeFilterX = (prevFilterX_ > -2.0f) ? prevFilterX_ : 0.0f;
        const float safeFilterY = (prevFilterY_ > -2.0f) ? prevFilterY_ : 0.0f;
        auto joyCenterForCc = [&](int ccNum) -> float {
            if (ccNum >= 0 && ccNum == ccXfilt)
                return juce::jlimit(0.f, 127.f, 64.f + xFOffset + safeFilterX * 63.5f * xFAtten);
            if (ccNum >= 0 && ccNum == ccYfilt)
                return juce::jlimit(0.f, 127.f, 64.f + yFOffset + safeFilterY * 63.5f * yFAtten);
            return 64.0f;
        };

        if (xOn && xCcNum >= 0)
        {
            const int v = juce::jlimit(0, 127, (int)std::roundf(joyCenterForCc(xCcNum) + lfoXRampOut_ * 63.5f));
            if (v != prevLfoCcX_) { midi.addEvent(juce::MidiMessage::controllerEvent(fCh, xCcNum, v), 0); prevLfoCcX_ = v; }
        }
        if (yOn && yCcNum >= 0)
        {
            const int v = juce::jlimit(0, 127, (int)std::roundf(joyCenterForCc(yCcNum) + lfoYRampOut_ * 63.5f));
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
    {
        // ── Migration: randomClockSync (bool) → randomSyncMode (Choice 0=FREE/1=INT/2=DAW)
        if (auto* oldParam = xml->getChildByAttribute("id", "randomClockSync"))
        {
            const bool wasSync = (oldParam->getDoubleAttribute("value") > 0.5);
            xml->removeChildElement(oldParam, true);
            auto* newParam = xml->createNewChildElement("PARAM");
            newParam->setAttribute("id", "randomSyncMode");
            newParam->setAttribute("value", wasSync ? 1.0 : 0.0);  // INT=1, FREE=0
        }
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
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
