#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ─── JoystickPad ─────────────────────────────────────────────────────────────
// Mouse-draggable XY pad that writes into processor.joystickX/Y

class JoystickPad : public juce::Component
{
public:
    explicit JoystickPad(PluginProcessor& p);
    void paint(juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

private:
    PluginProcessor& proc_;
    void updateFromMouse(const juce::MouseEvent& e);
};

// ─── TouchPlate ───────────────────────────────────────────────────────────────

class TouchPlate : public juce::Component
{
public:
    TouchPlate(PluginProcessor& p, int voice, const juce::String& label);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp  (const juce::MouseEvent&) override;

private:
    PluginProcessor& proc_;
    int              voice_;
    juce::String     label_;
};

// ─── ScaleKeyboard ────────────────────────────────────────────────────────────
// 12 piano-style toggle buttons for custom scale entry

class ScaleKeyboard : public juce::Component
{
public:
    explicit ScaleKeyboard(juce::AudioProcessorValueTreeState& apvts);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    juce::AudioProcessorValueTreeState& apvts_;

    static const bool kIsBlack[12];
    bool isNoteActive(int note) const;
    juce::Rectangle<float> noteRect(int note) const;
};

// ─── PluginEditor ─────────────────────────────────────────────────────────────

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& proc_;

    void timerCallback() override;

    // ── Joystick ──────────────────────────────────────────────────────────────
    JoystickPad joystickPad_;

    juce::Slider  joyXAttenKnob_, joyYAttenKnob_;
    juce::Label   joyXAttenLabel_, joyYAttenLabel_;

    // ── Chord intervals ───────────────────────────────────────────────────────
    juce::Slider  transposeKnob_, thirdIntKnob_, fifthIntKnob_, tensionIntKnob_;
    juce::Label   transposeLabel_, thirdIntLabel_, fifthIntLabel_, tensionIntLabel_;

    // ── Octave offsets ────────────────────────────────────────────────────────
    juce::Slider  rootOctKnob_, thirdOctKnob_, fifthOctKnob_, tensionOctKnob_;
    juce::Label   rootOctLabel_, thirdOctLabel_, fifthOctLabel_, tensionOctLabel_;

    // ── Touchplates ───────────────────────────────────────────────────────────
    TouchPlate    padRoot_, padThird_, padFifth_, padTension_;

    // ── Scale ─────────────────────────────────────────────────────────────────
    ScaleKeyboard scaleKeys_;
    juce::ComboBox scalePresetBox_;
    juce::Label    scalePresetLabel_;
    juce::ToggleButton customScaleToggle_;

    // ── Trigger source per voice ──────────────────────────────────────────────
    juce::ComboBox trigSrc_[4];
    juce::Label    trigSrcLabel_[4];
    juce::Slider   randomDensityKnob_;
    juce::Label    randomDensityLabel_;
    juce::ComboBox randomSubdivBox_;
    juce::Label    randomSubdivLabel_;

    // ── Filter (gamepad) ─────────────────────────────────────────────────────
    juce::Slider filterXAttenKnob_, filterYAttenKnob_;
    juce::Label  filterXAttenLabel_, filterYAttenLabel_;

    // ── Looper ────────────────────────────────────────────────────────────────
    juce::TextButton  loopPlayBtn_, loopRecBtn_, loopResetBtn_, loopDeleteBtn_;
    juce::ComboBox    loopSubdivBox_;
    juce::Label       loopSubdivLabel_;
    juce::Slider      loopLengthKnob_;
    juce::Label       loopLengthLabel_;
    juce::Label       gamepadStatusLabel_;

    // ── MIDI channel conflict warning ─────────────────────────────────────────
    juce::Label channelConflictLabel_;
    bool        channelConflictShown_ = false;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAtt  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt   = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt  = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> joyXAttenAtt_, joyYAttenAtt_;
    std::unique_ptr<SliderAtt> transposeAtt_, thirdIntAtt_, fifthIntAtt_, tensionIntAtt_;
    std::unique_ptr<SliderAtt> rootOctAtt_, thirdOctAtt_, fifthOctAtt_, tensionOctAtt_;
    std::unique_ptr<SliderAtt> randomDensityAtt_, loopLengthAtt_;
    std::unique_ptr<SliderAtt> filterXAttenAtt_, filterYAttenAtt_;
    std::unique_ptr<ComboAtt>  scalePresetAtt_;
    std::unique_ptr<ButtonAtt> customScaleAtt_;
    std::unique_ptr<ComboAtt>  trigSrcAtt_[4];
    std::unique_ptr<ComboAtt>  randomSubdivAtt_, loopSubdivAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
