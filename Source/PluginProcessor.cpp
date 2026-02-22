#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    // Helper to create NormalisableRange with skew for musical parameters
    template<typename T>
    juce::NormalisableRange<T> createLogarithmicRange(T min, T max, T skew = T{1})
    {
        return juce::NormalisableRange<T>(min, max, skew);
    }
}

// ============================================================
// Scale Patterns (semitones from root, 0-11)
// ============================================================
const int ScaleQuantizer::majorPattern[] = {0, 2, 4, 5, 7, 9, 11};
const int ScaleQuantizer::minorPattern[] = {0, 2, 3, 5, 7, 8, 10};
const int ScaleQuantizer::harmonicMinorPattern[] = {0, 2, 3, 5, 7, 8, 11};
const int ScaleQuantizer::melodicMinorPattern[] = {0, 2, 3, 5, 7, 9, 11};
const int ScaleQuantizer::dorianPattern[] = {0, 2, 3, 5, 7, 9, 10};
const int ScaleQuantizer::phrygianPattern[] = {0, 1, 3, 5, 7, 8, 10};
const int ScaleQuantizer::lydianPattern[] = {0, 2, 4, 6, 7, 9, 11};
const int ScaleQuantizer::mixolydianPattern[] = {0, 2, 4, 5, 7, 9, 10};
const int ScaleQuantizer::pentatonicMajorPattern[] = {0, 2, 4, 7, 9};
const int ScaleQuantizer::pentatonicMinorPattern[] = {0, 3, 5, 7, 10};
const int ScaleQuantizer::bluesPattern[] = {0, 3, 5, 6, 7, 10};
const int ScaleQuantizer::chromaticPattern[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

// ============================================================
// ScaleQuantizer Implementation
// ============================================================
const int* ScaleQuantizer::getScalePattern(ScalePreset scale)
{
    switch (scale)
    {
        case ScalePreset::Major: return majorPattern;
        case ScalePreset::Minor: return minorPattern;
        case ScalePreset::HarmonicMinor: return harmonicMinorPattern;
        case ScalePreset::MelodicMinor: return melodicMinorPattern;
        case ScalePreset::Dorian: return dorianPattern;
        case ScalePreset::Phrygian: return phrygianPattern;
        case ScalePreset::Lydian: return lydianPattern;
        case ScalePreset::Mixolydian: return mixolydianPattern;
        case ScalePreset::PentatonicMajor: return pentatonicMajorPattern;
        case ScalePreset::PentatonicMinor: return pentatonicMinorPattern;
        case ScalePreset::Blues: return bluesPattern;
        case ScalePreset::Chromatic: return chromaticPattern;
        default: return majorPattern;
    }
}

int ScaleQuantizer::getScaleSize(ScalePreset scale)
{
    switch (scale)
    {
        case ScalePreset::PentatonicMajor:
        case ScalePreset::PentatonicMinor:
            return 5;
        case ScalePreset::Blues:
            return 6;
        case ScalePreset::Chromatic:
            return 12;
        default:
            return 7;
    }
}

void ScaleQuantizer::buildCustomPattern(const bool scaleNotes[12], int customPattern[12], int& customSize)
{
    customSize = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (scaleNotes[i])
        {
            customPattern[customSize++] = i;
        }
    }
    // Ensure at least one note in the scale
    if (customSize == 0)
    {
        customPattern[0] = 0;
        customSize = 1;
    }
}

int ScaleQuantizer::quantizeToScale(int rawPitch, const int* scalePattern, int scaleSize, int transpose)
{
    // Extract octave and semitone from raw pitch
    int octave = rawPitch / 12;
    int semitone = rawPitch % 12;
    
    // Apply transpose to semitone
    int transposedSemitone = (semitone + transpose) % 12;
    if (transposedSemitone < 0) transposedSemitone += 12;
    
    // Adjust octave if transpose wrapped around
    octave += (semitone + transpose) / 12;
    
    // Find nearest note in scale
    int closestScaleNote = scalePattern[0];
    int minDistance = 12;
    
    for (int i = 0; i < scaleSize; ++i)
    {
        int distance = std::abs(scalePattern[i] - transposedSemitone);
        // Handle wraparound
        if (distance > 6) distance = 12 - distance;
        
        if (distance < minDistance)
        {
            minDistance = distance;
            closestScaleNote = scalePattern[i];
        }
    }
    
    // Calculate final MIDI note
    int midiNote = octave * 12 + closestScaleNote;
    
    // Clamp to valid MIDI range
    if (midiNote < 0) midiNote = 0;
    if (midiNote > 127) midiNote = 127;
    
    return midiNote;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo()))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

PluginProcessor::~PluginProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ============================================================
    // QUANTIZER GROUP (12 scale buttons for chromatic selection)
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("quantizer", "Quantizer", "|");
        
        for (int i = 0; i < 12; ++i)
        {
            auto paramID = "scale_" + juce::String(i);
            auto param = std::make_unique<juce::AudioParameterBool>(paramID, 
                "Scale " + juce::String(i), 
                i == 0);  // Default: C selected
            group->addChild(std::move(param));
        }
        
        layout.add(std::move(group));
    }

    // ============================================================
    // INTERVALS GROUP (intervals, octaves, presets, transpose)
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("intervals", "Intervals", "|");
        
        // Interval semitones (0-12)
        auto intervalRange = juce::NormalisableRange<float>(0.0f, 12.0f, 1.0f);
        
        group->addChild(std::make_unique<juce::AudioParameterFloat>("root_interval", "Root Interval", intervalRange, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("third_interval", "Third Interval", intervalRange, 4.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("fifth_interval", "Fifth Interval", intervalRange, 7.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("tension_interval", "Tension Interval", intervalRange, 11.0f));
        
        // Octave offsets (-2 to +2)
        auto octaveRange = juce::NormalisableRange<float>(-2.0f, 2.0f, 1.0f);
        
        group->addChild(std::make_unique<juce::AudioParameterFloat>("root_octave", "Root Octave", octaveRange, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("third_octave", "Third Octave", octaveRange, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("fifth_octave", "Fifth Octave", octaveRange, 0.0f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("tension_octave", "Tension Octave", octaveRange, 0.0f));
        
        // Scale preset (0-10)
        group->addChild(std::make_unique<juce::AudioParameterInt>("scale_preset", "Scale Preset", 0, 10, 0));
        
        // Global transpose (0-12)
        auto transposeRange = juce::NormalisableRange<float>(0.0f, 12.0f, 1.0f);
        group->addChild(std::make_unique<juce::AudioParameterFloat>("global_transpose", "Global Transpose", transposeRange, 0.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // TRIGGERS GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("triggers", "Triggers", "|");
        
        // Trigger source: 0=touchplate, 1=joystick, 2=random
        group->addChild(std::make_unique<juce::AudioParameterInt>("trigger_source", "Trigger Source", 0, 2, 0));
        
        // Velocity sensitivity (0-1)
        auto velRange = juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f);
        group->addChild(std::make_unique<juce::AudioParameterFloat>("velocity_sensitivity", "Velocity Sensitivity", velRange, 1.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // JOYSTICK GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("joystick", "Joystick", "|");
        
        // XY Attenuator (0-24 dB)
        auto attenRange = juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f);
        group->addChild(std::make_unique<juce::AudioParameterFloat>("xy_attenuator", "XY Attenuator", attenRange, 12.0f));
        
        // Simulated joystick position (0-1)
        auto joyRange = juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f);
        group->addChild(std::make_unique<juce::AudioParameterFloat>("joystick_x", "Joystick X", joyRange, 0.5f));
        group->addChild(std::make_unique<juce::AudioParameterFloat>("joystick_y", "Joystick Y", joyRange, 0.5f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // SWITCHES GROUP (clock)
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("switches", "Switches", "|");
        
        // Clock source: 0=internal, 1=external/DAW
        group->addChild(std::make_unique<juce::AudioParameterInt>("clock_source", "Clock Source", 0, 1, 0));
        
        // BPM (40-240) for standalone mode
        auto bpmRange = juce::NormalisableRange<float>(40.0f, 240.0f, 1.0f);
        group->addChild(std::make_unique<juce::AudioParameterFloat>("bpm", "BPM", bpmRange, 120.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // LOOPER GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("looper", "Looper", "|");
        
        group->addChild(std::make_unique<juce::AudioParameterBool>("looper_enabled", "Looper Enabled", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("looper_record", "Looper Record", false));
        group->addChild(std::make_unique<juce::AudioParameterBool>("looper_reset", "Looper Reset", false));
        
        // Loop length in bars (1-16)
        group->addChild(std::make_unique<juce::AudioParameterInt>("loop_length_bars", "Loop Length Bars", 1, 16, 4));
        
        // Loop subdivision: 0=3/4, 1=4/4, 2=5/4, 3=7/8, 4=9/8, 5=11/8
        group->addChild(std::make_unique<juce::AudioParameterInt>("loop_subdivision", "Loop Subdivision", 0, 5, 1));
        
        layout.add(std::move(group));
    }

    return layout;
}

// Helper method to get float parameter value by ID
float PluginProcessor::getParameterValue(const juce::String& paramID) const
{
    if (auto* param = apvts.getParameter(paramID))
    {
        return param->getValue();
    }
    return 0.0f;
}

// Helper method to get bool parameter value by ID
bool PluginProcessor::getBoolParameterValue(const juce::String& paramID) const
{
    if (auto* param = apvts.getParameter(paramID))
    {
        return param->getValue() > 0.5f;
    }
    return false;
}

// Helper method to get int parameter value by ID
int PluginProcessor::getIntParameterValue(const juce::String& paramID) const
{
    if (auto* param = apvts.getParameter(paramID))
    {
        return static_cast<int>(param->getValue() * 1000.0f);  // Approximate for now
    }
    return 0;
}

// ============================================================
// Scale Quantization Methods
// ============================================================

// Get the currently selected scale preset
ScalePreset PluginProcessor::getCurrentScale() const
{
    int presetValue = static_cast<int>(getParameterValue("scale_preset"));
    if (presetValue < 0) presetValue = 0;
    if (presetValue > 10) presetValue = 10;
    return static_cast<ScalePreset>(presetValue);
}

// Get custom scale note by index (for custom chromatic selection)
int PluginProcessor::getCustomScaleNote(int index) const
{
    if (index < 0 || index > 11) return 0;
    auto paramID = "scale_" + juce::String(index);
    return getBoolParameterValue(paramID) ? index : -1;
}

// Get the scale degree (0-6) from the current scale
int PluginProcessor::getScaleDegree(int degree) const
{
    ScalePreset scale = getCurrentScale();
    const int* pattern = ScaleQuantizer::getScalePattern(scale);
    int size = ScaleQuantizer::getScaleSize(scale);
    
    if (degree < 0) degree = 0;
    if (degree >= size) degree = degree % size;
    
    return pattern[degree];
}

// Quantize a raw MIDI pitch to the selected scale
int PluginProcessor::getQuantizedNote(int rawPitch) const
{
    // Check for custom scale first
    bool customNotes[12];
    bool hasCustom = false;
    for (int i = 0; i < 12; ++i)
    {
        customNotes[i] = getBoolParameterValue("scale_" + juce::String(i));
        if (customNotes[i]) hasCustom = true;
    }
    
    // Get global transpose
    int transpose = static_cast<int>(getParameterValue("global_transpose"));
    
    // Use custom scale if any notes are selected
    if (hasCustom)
    {
        int customPattern[12];
        int customSize;
        ScaleQuantizer::buildCustomPattern(customNotes, customPattern, customSize);
        return ScaleQuantizer::quantizeToScale(rawPitch, customPattern, customSize, transpose);
    }
    
    // Otherwise use preset scale
    ScalePreset scale = getCurrentScale();
    const int* pattern = ScaleQuantizer::getScalePattern(scale);
    int size = ScaleQuantizer::getScaleSize(scale);
    
    return ScaleQuantizer::quantizeToScale(rawPitch, pattern, size, transpose);
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Verify APVTS parameters are accessible
    // These calls will log parameter values if debugging is enabled
    auto rootInterval = getParameterValue("root_interval");
    auto triggerSource = getIntParameterValue("trigger_source");
    auto looperEnabled = getBoolParameterValue("looper_enabled");
    
    (void)sampleRate;    // Suppress unused warning
    (void)samplesPerBlock;
    (void)rootInterval;
    (void)triggerSource;
    (void)looperEnabled;
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

const juce::String PluginProcessor::getName() const
{
    return "ChordJoystick";
}

bool PluginProcessor::producesMidi() const
{
    return true;
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::isMidiEffect() const
{
    return true;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int)
{
}

const juce::String PluginProcessor::getProgramName(int)
{
    return {};
}

void PluginProcessor::changeProgramName(int, const juce::String&)
{
}

void PluginProcessor::getStateInformation(juce::MemoryBlock&)
{
}

void PluginProcessor::setStateInformation(const void*, int)
{
}

juce::AudioProcessor* createPluginFilter()
{
    return new PluginProcessor();
}
