#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// ============================================================
// Scale Preset Enum
// ============================================================
enum class ScalePreset
{
    Major = 0,
    Minor = 1,
    HarmonicMinor = 2,
    MelodicMinor = 3,
    Dorian = 4,
    Phrygian = 5,
    Lydian = 6,
    Mixolydian = 7,
    PentatonicMajor = 8,
    PentatonicMinor = 9,
    Blues = 10,
    Chromatic = 11
};

// ============================================================
// Scale Quantization Helper
// ============================================================
class ScaleQuantizer
{
public:
    // Returns array of semitones from root (0-11) for each scale preset
    static const int* getScalePattern(ScalePreset scale);
    static int getScaleSize(ScalePreset scale);
    
    // Build custom scale from 12 boolean flags (one per semitone)
    static void buildCustomPattern(const bool scaleNotes[12], int customPattern[12], int& customSize);
    
    // Quantize a raw MIDI pitch to the nearest note in the given scale
    static int quantizeToScale(int rawPitch, const int* scalePattern, int scaleSize, int transpose = 0);
    
private:
    static const int majorPattern[];
    static const int minorPattern[];
    static const int harmonicMinorPattern[];
    static const int melodicMinorPattern[];
    static const int dorianPattern[];
    static const int phrygianPattern[];
    static const int lydianPattern[];
    static const int mixolydianPattern[];
    static const int pentatonicMajorPattern[];
    static const int pentatonicMinorPattern[];
    static const int bluesPattern[];
    static const int chromaticPattern[];
};

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double, int) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout&) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Parameter access helpers
    float getParameterValue(const juce::String& paramID) const;
    bool getBoolParameterValue(const juce::String& paramID) const;
    int getIntParameterValue(const juce::String& paramID) const;

    // Scale quantization methods
    ScalePreset getCurrentScale() const;
    int getQuantizedNote(int rawPitch) const;
    int getScaleDegree(int degree) const;
    int getCustomScaleNote(int index) const;

private:
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
