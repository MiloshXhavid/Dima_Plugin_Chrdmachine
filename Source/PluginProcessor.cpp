#include "PluginProcessor.h"

namespace
{
    // Helper to create NormalisableRange with skew for musical parameters
    template<typename T>
    juce::NormalisableRange<T> createLogarithmicRange(T min, T max, T skew = T{1})
    {
        return juce::NormalisableRange<T>(min, max, skew);
    }
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
            group->addParameter(std::move(param));
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
        
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("root_interval", "Root Interval", intervalRange, 0.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("third_interval", "Third Interval", intervalRange, 4.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("fifth_interval", "Fifth Interval", intervalRange, 7.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("tension_interval", "Tension Interval", intervalRange, 11.0f));
        
        // Octave offsets (-2 to +2)
        auto octaveRange = juce::NormalisableRange<float>(-2.0f, 2.0f, 1.0f);
        
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("root_octave", "Root Octave", octaveRange, 0.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("third_octave", "Third Octave", octaveRange, 0.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("fifth_octave", "Fifth Octave", octaveRange, 0.0f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("tension_octave", "Tension Octave", octaveRange, 0.0f));
        
        // Scale preset (0-10)
        group->addParameter(std::make_unique<juce::AudioParameterInt>("scale_preset", "Scale Preset", 0, 10, 0));
        
        // Global transpose (0-12)
        auto transposeRange = juce::NormalisableRange<float>(0.0f, 12.0f, 1.0f);
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("global_transpose", "Global Transpose", transposeRange, 0.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // TRIGGERS GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("triggers", "Triggers", "|");
        
        // Trigger source: 0=touchplate, 1=joystick, 2=random
        group->addParameter(std::make_unique<juce::AudioParameterInt>("trigger_source", "Trigger Source", 0, 2, 0));
        
        // Velocity sensitivity (0-1)
        auto velRange = juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f);
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("velocity_sensitivity", "Velocity Sensitivity", velRange, 1.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // JOYSTICK GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("joystick", "Joystick", "|");
        
        // XY Attenuator (0-24 dB)
        auto attenRange = juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f);
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("xy_attenuator", "XY Attenuator", attenRange, 12.0f));
        
        // Simulated joystick position (0-1)
        auto joyRange = juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f);
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("joystick_x", "Joystick X", joyRange, 0.5f));
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("joystick_y", "Joystick Y", joyRange, 0.5f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // SWITCHES GROUP (clock)
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("switches", "Switches", "|");
        
        // Clock source: 0=internal, 1=external/DAW
        group->addParameter(std::make_unique<juce::AudioParameterInt>("clock_source", "Clock Source", 0, 1, 0));
        
        // BPM (40-240) for standalone mode
        auto bpmRange = juce::NormalisableRange<float>(40.0f, 240.0f, 1.0f);
        group->addParameter(std::make_unique<juce::AudioParameterFloat>("bpm", "BPM", bpmRange, 120.0f));
        
        layout.add(std::move(group));
    }

    // ============================================================
    // LOOPER GROUP
    // ============================================================
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("looper", "Looper", "|");
        
        group->addParameter(std::make_unique<juce::AudioParameterBool>("looper_enabled", "Looper Enabled", false));
        group->addParameter(std::make_unique<juce::AudioParameterBool>("looper_record", "Looper Record", false));
        group->addParameter(std::make_unique<juce::AudioParameterBool>("looper_reset", "Looper Reset", false));
        
        // Loop length in bars (1-16)
        group->addParameter(std::make_unique<juce::AudioParameterInt>("loop_length_bars", "Loop Length Bars", 1, 16, 4));
        
        // Loop subdivision: 0=3/4, 1=4/4, 2=5/4, 3=7/8, 4=9/8, 5=11/8
        group->addParameter(std::make_unique<juce::AudioParameterInt>("loop_subdivision", "Loop Subdivision", 0, 5, 1));
        
        layout.add(std::move(group));
    }

    return layout;
}

void PluginProcessor::prepareToPlay(double, int)
{
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
    return new juce::AudioProcessorEditor(*this);
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

bool PluginProcessor::isMidiEffect() const
{
    return false;
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
