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
    static const juce::String randomSubdiv     = "randomSubdiv";

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
    addInt  (ParamID::rootOctave,      "Root Octave",      -3,  3,  0);
    addInt  (ParamID::thirdOctave,     "Third Octave",     -3,  3,  0);
    addInt  (ParamID::fifthOctave,     "Fifth Octave",     -3,  3,  0);
    addInt  (ParamID::tensionOctave,   "Tension Octave",   -3,  3,  0);
    addFloat(ParamID::joystickXAtten,     "Joy X Attenuator",   0.0f, 127.0f, 48.0f);
    addFloat(ParamID::joystickYAtten,     "Joy Y Attenuator",   0.0f, 127.0f, 48.0f);
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
    addFloat (ParamID::randomDensity,  "Random Density",   0.0f, 1.0f, 0.5f);
    addChoice(ParamID::randomSubdiv,   "Random Subdivison",
              { "1/4", "1/8", "1/16", "1/32" }, 1);

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

    return layout;
}

// ─── Constructor ──────────────────────────────────────────────────────────────

PluginProcessor::PluginProcessor()
    : AudioProcessor(
          juce::PluginHostType().isAbletonLive()
              ? BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), false)
              : BusesProperties()),
      apvts(*this, nullptr, "ChordJoystick", createParameterLayout())
{
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

        // Reset pitch bend to centre before note-off so receiving synth is clean.
        midi.addEvent(juce::MidiMessage::pitchWheel(ch, 0), 0);

        const int pitch = trigger_.getActivePitch(v);
        if (pitch >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(ch, pitch, (uint8_t)0), 0);
    }
    trigger_.resetAllGates();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // No audio input ever
    if (layouts.getMainInputChannels() != 0) return false;
    // Allow zero output (Reaper/Bitwig/standalone) or stereo dummy output (Ableton Live)
    const int outCh = layouts.getMainOutputChannels();
    if (outCh != 0 && outCh != 2) return false;
    return true;
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

    const int blockSize = audio.getNumSamples();

    // ── Poll gamepad triggers (consume edge-flags) ────────────────────────────
    for (int v = 0; v < 4; ++v)
        if (gamepad_.consumeVoiceTrigger(v))
            trigger_.setPadState(v, true);    // instant press

    if (gamepad_.consumeAllNotesTrigger())
        trigger_.triggerAllNotes();

    if (gamepad_.consumeLooperStartStop()) looper_.startStop();
    if (gamepad_.consumeLooperRecord())    looper_.record();
    if (gamepad_.consumeLooperReset())     looper_.reset();
    if (gamepad_.consumeLooperDelete())    looper_.deleteLoop();

    // ── Looper ────────────────────────────────────────────────────────────────
    juce::AudioPlayHead::CurrentPositionInfo pos {};
    const bool hasDaw = getPlayHead() && getPlayHead()->getCurrentPosition(pos);

    LooperEngine::ProcessParams lp;
    lp.sampleRate   = sampleRate_;
    lp.bpm          = (hasDaw && pos.bpm > 0.0) ? pos.bpm : 120.0;
    lp.ppqPosition  = (hasDaw && pos.isPlaying) ? pos.ppqPosition : -1.0;
    lp.blockSize    = blockSize;
    lp.isDawPlaying = hasDaw && pos.isPlaying;

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

    // Looper gate overrides
    for (int v = 0; v < 4; ++v)
    {
        if (loopOut.gateOn[v])  trigger_.setPadState(v, true);
        if (loopOut.gateOff[v]) trigger_.setPadState(v, false);
    }

    // ── Trigger system ────────────────────────────────────────────────────────
    const int voiceChs[4] =
    {
        (int)apvts.getRawParameterValue(ParamID::voiceCh0)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh1)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh2)->load(),
        (int)apvts.getRawParameterValue(ParamID::voiceCh3)->load(),
    };

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
            // Record gate event in looper
            const double beatPos = (hasDaw && pos.isPlaying)
                ? pos.ppqPosition
                : looper_.getPlaybackBeat();
            looper_.recordGate(beatPos, voice, true);

            midi.addEvent(juce::MidiMessage::noteOn(ch0 + 1, pitch, (uint8_t)100),
                          sampleOff);
        }
        else
        {
            looper_.recordGate(looper_.getPlaybackBeat(), voice, false);
            midi.addEvent(juce::MidiMessage::noteOff(ch0 + 1, pitch, (uint8_t)0), sampleOff);
        }
    };
    tp.onMidi = [&](const juce::MidiMessage& msg, int sampleOff)
    {
        midi.addEvent(msg, sampleOff);
    };

    tp.blockSize         = blockSize;
    tp.sampleRate        = sampleRate_;
    tp.bpm               = lp.bpm;
    tp.joystickX         = chordP.joystickX;
    tp.joystickY         = chordP.joystickY;
    tp.joystickThreshold = apvts.getRawParameterValue(ParamID::joystickThreshold)->load();
    for (int v = 0; v < 4; ++v)
    {
        tp.sources[v]      = src[v];
        tp.heldPitches[v]  = heldPitch_[v];
        tp.midiChannels[v] = voiceChs[v];
    }
    tp.randomDensity = apvts.getRawParameterValue(ParamID::randomDensity)->load();
    tp.randomSubdiv  = static_cast<RandomSubdiv>(
        (int)apvts.getRawParameterValue(ParamID::randomSubdiv)->load());

    trigger_.processBlock(tp);

    // ── Looper joystick recording ─────────────────────────────────────────────
    {
        const double beatPos = (hasDaw && pos.isPlaying)
            ? pos.ppqPosition : looper_.getPlaybackBeat();
        looper_.recordJoystick(beatPos, chordP.joystickX, chordP.joystickY);
    }

    // ── Filter CC from gamepad left stick ─────────────────────────────────────
    {
        const float xAtten = apvts.getRawParameterValue(ParamID::filterXAtten)->load();
        const float yAtten = apvts.getRawParameterValue(ParamID::filterYAtten)->load();
        const int   fCh    = (int)apvts.getRawParameterValue(ParamID::filterMidiCh)->load();

        // Map (-1..+1) → (0..atten), clamp to 0..127
        const float gfx = gamepad_.getFilterX();
        const float gfy = gamepad_.getFilterY();
        const int ccCut  = juce::jlimit(0, 127, (int)(((gfx + 1.0f) * 0.5f) * xAtten));
        const int ccRes  = juce::jlimit(0, 127, (int)(((gfy + 1.0f) * 0.5f) * yAtten));

        midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 74, ccCut), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(fCh, 71, ccRes), 0);
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
