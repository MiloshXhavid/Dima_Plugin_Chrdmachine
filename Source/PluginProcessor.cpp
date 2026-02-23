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
    static const juce::String filterMidiCh     = "filterMidiCh";

    // MIDI channels
    static const juce::String voiceCh0         = "voiceCh0";
    static const juce::String voiceCh1         = "voiceCh1";
    static const juce::String voiceCh2         = "voiceCh2";
    static const juce::String voiceCh3         = "voiceCh3";

    // Looper
    static const juce::String looperSubdiv     = "looperSubdiv";
    static const juce::String looperLength     = "looperLength";

    // Slew (portamento per voice)
    static const juce::String slewVoice0       = "slewVoice0";
    static const juce::String slewVoice1       = "slewVoice1";
    static const juce::String slewVoice2       = "slewVoice2";
    static const juce::String slewVoice3       = "slewVoice3";
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
    addInt  (ParamID::globalTranspose, "Global Transpose", -24, 24,  0);
    addInt  (ParamID::thirdInterval,   "Third Interval",    0, 12,  4);
    addInt  (ParamID::fifthInterval,   "Fifth Interval",    0, 12,  7);
    addInt  (ParamID::tensionInterval, "Tension Interval",  0, 12, 10);
    addInt  (ParamID::rootOctave,      "Root Octave",       0, 12,  3);
    addInt  (ParamID::thirdOctave,     "Third Octave",      0, 12,  4);
    addInt  (ParamID::fifthOctave,     "Fifth Octave",      0, 12,  3);
    addInt  (ParamID::tensionOctave,   "Tension Octave",    0, 12,  3);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickXAtten, "Joy X Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamID::joystickYAtten, "Joy Y Attenuator",
        juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 12.0f));
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
    addFloat(ParamID::filterXAtten, "Filter Cutoff Attenuator",    0.0f, 127.0f, 64.0f);
    addFloat(ParamID::filterYAtten, "Filter Resonance Attenuator", 0.0f, 127.0f, 64.0f);
    addInt  (ParamID::filterMidiCh, "Filter MIDI Channel",  1, 16, 1);

    // ── MIDI routing ──────────────────────────────────────────────────────────
    addInt(ParamID::voiceCh0, "Root MIDI Channel",    1, 16, 1);
    addInt(ParamID::voiceCh1, "Third MIDI Channel",   1, 16, 2);
    addInt(ParamID::voiceCh2, "Fifth MIDI Channel",   1, 16, 3);
    addInt(ParamID::voiceCh3, "Tension MIDI Channel", 1, 16, 4);

    // ── Looper ────────────────────────────────────────────────────────────────
    addChoice(ParamID::looperSubdiv, "Loop Time Signature",
              { "3/4", "4/4", "5/4", "7/8", "9/8", "11/8" }, 1);
    addInt   (ParamID::looperLength, "Loop Length (bars)", 1, 16, 2);

    // ── Filter CC live display (read-only from DAW perspective) ──────────────
    // Updated every timer tick from audio-thread atomics so DAW can see stick movement.
    addFloat("filterCutLive", "Filter Cut CC", 0.0f, 127.0f, 0.0f);
    addFloat("filterResLive", "Filter Res CC", 0.0f, 127.0f, 0.0f);

    // ── Slew (portamento per voice) ───────────────────────────────────────────
    addInt(ParamID::slewVoice0, "Root Slew",    0, 127, 0);
    addInt(ParamID::slewVoice1, "Third Slew",   0, 127, 0);
    addInt(ParamID::slewVoice2, "Fifth Slew",   0, 127, 0);
    addInt(ParamID::slewVoice3, "Tension Slew", 0, 127, 0);

    return layout;
}

// ─── Constructor ──────────────────────────────────────────────────────────────

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()),
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
    // Pure MIDI effect — no audio buses
    return layouts.getMainInputChannels() == 0
        && layouts.getMainOutputChannels() == 0;
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

    // Prefer gamepad if any axis is active (|value| > 0.05)
    const bool gpActive = (std::abs(gpX) + std::abs(gpY)) > 0.05f;
    p.joystickX = gpActive ? gpX : jx;
    p.joystickY = gpActive ? gpY : jy;

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
        // Momentary voice gates: rising edge → note-on, falling edge → note-off
        for (int v = 0; v < 4; ++v)
        {
            const bool held = gamepad_.getVoiceHeld(v);
            if (held && !gamepadVoiceWasHeld_[v])       trigger_.setPadState(v, true);
            else if (!held && gamepadVoiceWasHeld_[v])  trigger_.setPadState(v, false);
            gamepadVoiceWasHeld_[v] = held;
        }

        if (gamepad_.consumeAllNotesTrigger()) trigger_.triggerAllNotes();

        if (gamepad_.consumeLooperStartStop()) looper_.startStop();
        if (gamepad_.consumeLooperRecord())    looper_.record();
        if (gamepad_.consumeLooperReset())     looper_.reset();
        if (gamepad_.consumeLooperDelete())    looper_.deleteLoop();

        // D-pad: BPM and looper recording toggles
        if (gamepad_.consumeDpadUp())    pendingBpmDelta_.fetch_add(1,  std::memory_order_relaxed);
        if (gamepad_.consumeDpadDown())  pendingBpmDelta_.fetch_add(-1, std::memory_order_relaxed);
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
    const ChordEngine::Params chordP = buildChordParams();

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

    // If DAW stopped while SYNC was on, send all-notes-off to prevent hanging notes.
    if (loopOut.dawStopped)
    {
        for (int v = 0; v < 4; ++v)
            midi.addEvent(juce::MidiMessage::allNotesOff(voiceChs[v]), 0);
    }

    // Looper gate playback: emit MIDI directly, bypassing TriggerSystem.
    // This satisfies the locked decision: live pad input passes through independently
    // because TriggerSystem is not informed of looper events — it only sees live pad state.
    for (int v = 0; v < 4; ++v)
    {
        const int ch0 = voiceChs[v] - 1;  // 0-based for MIDI message
        if (loopOut.gateOn[v])
        {
            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, heldPitch_[v], (uint8_t)100), 0);
        }
        if (loopOut.gateOff[v])
        {
            midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, heldPitch_[v], (uint8_t)0), 0);
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

    TriggerSystem::ProcessParams tp;
    tp.onNote = [&](int voice, int pitch, bool isOn, int sampleOff)
    {
        const int ch0 = voiceChs[voice] - 1;  // 0-based
        if (isOn)
        {
            // Slew (portamento): CC65 (on/off) + CC5 (time) before note-on
            const int slewVal = (int)apvts.getRawParameterValue(
                "slewVoice" + juce::String(voice))->load();
            if (slewVal > 0)
            {
                midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 65, 127), sampleOff);  // portamento on
                midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1,  5, slewVal), sampleOff);  // portamento time
            }
            else
            {
                midi.addEvent(juce::MidiMessage::controllerEvent(ch0 + 1, 65, 0), sampleOff);  // portamento off
            }

            // Record gate event in looper (using JUCE 8 ppqPos variable)
            const double beatPos = (hasDaw && ppqPos >= 0.0)
                ? ppqPos
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, true);

            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100),
                          sampleOff);
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
    tp.randomDensity  = apvts.getRawParameterValue(ParamID::randomDensity)->load();
    tp.randomGateTime = apvts.getRawParameterValue(ParamID::randomGateTime)->load();
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

    trigger_.processBlock(tp);

    // ── Looper joystick recording ─────────────────────────────────────────────
    {
        const double beatPos = (isDawPlaying && ppqPos >= 0.0)
            ? ppqPos : looper_.getPlaybackBeat();
        looper_.recordJoystick(beatPos, chordP.joystickX, chordP.joystickY);
    }

    // ── Filter CC from gamepad left stick ─────────────────────────────────────
    // Gated on isConnected() AND gamepadActive_.
    // CC reset on disconnect via pendingCcReset_ atomic flag (set from message thread).
    // All-notes-off on disconnect via pendingAllNotesOff_ (set from message thread).
    {
        const int fCh = (int)apvts.getRawParameterValue(ParamID::filterMidiCh)->load();

        // Handle disconnect events — emit MIDI from audio thread via pending flags.
        if (pendingAllNotesOff_.exchange(false, std::memory_order_acq_rel))
        {
            for (int v = 0; v < 4; ++v)
                midi.addEvent(juce::MidiMessage::allNotesOff(voiceChs[v]), 0);
        }

        if (pendingCcReset_.exchange(false, std::memory_order_acq_rel))
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, 0), 0);
            prevCcCut_.store(0, std::memory_order_relaxed);
            prevCcRes_.store(0, std::memory_order_relaxed);
        }

        // Normal CC emission — only when connected AND this instance is active.
        if (gamepad_.isConnected() && gamepadActive_.load(std::memory_order_relaxed))
        {
            const float xAtten = apvts.getRawParameterValue(ParamID::filterXAtten)->load();
            const float yAtten = apvts.getRawParameterValue(ParamID::filterYAtten)->load();

            const float gfx = gamepad_.getFilterX();
            const float gfy = gamepad_.getFilterY();

            // Map (-1..+1) → (0..atten), clamp to 0..127
            const int ccCut = juce::jlimit(0, 127, (int)(((gfx + 1.0f) * 0.5f) * xAtten));
            const int ccRes = juce::jlimit(0, 127, (int)(((gfy + 1.0f) * 0.5f) * yAtten));

            // Value-change dedup: only emit if integer value changed by >= 1.
            // prevCcCut_/prevCcRes_ are atomic<int>; use load/store for thread safety.
            if (ccCut != prevCcCut_.load(std::memory_order_relaxed))
            {
                midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCut), 0);
                prevCcCut_.store(ccCut, std::memory_order_relaxed);
            }
            if (ccRes != prevCcRes_.load(std::memory_order_relaxed))
            {
                midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, ccRes), 0);
                prevCcRes_.store(ccRes, std::memory_order_relaxed);
            }
            // Expose live CC values to DAW parameter display (consumed on message thread).
            filterCutDisplay_.store(static_cast<float>(ccCut), std::memory_order_relaxed);
            filterResDisplay_.store(static_cast<float>(ccRes), std::memory_order_relaxed);
        }
    }

    // ── Looper config sync ────────────────────────────────────────────────────
    looper_.setSubdiv(static_cast<LooperSubdiv>(
        (int)apvts.getRawParameterValue(ParamID::looperSubdiv)->load()));
    looper_.setLoopLengthBars(
        (int)apvts.getRawParameterValue(ParamID::looperLength)->load());
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
