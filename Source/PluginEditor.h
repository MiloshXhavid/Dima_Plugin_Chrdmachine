#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ─── PixelLookAndFeel ─────────────────────────────────────────────────────────
// Arcade / 8-bit aesthetic: flat fills, sharp corners, pixel font, cyan + magenta.

class PixelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

    void setPixelFont(const juce::Font& f) { pixelFont_ = f; }

private:
    juce::Font pixelFont_ { 10.0f };
};

// ─── JoystickPad ─────────────────────────────────────────────────────────────
// Mouse-draggable XY pad that writes into processor.joystickX/Y

class JoystickPad : public juce::Component
{
public:
    explicit JoystickPad(PluginProcessor& p);
    void paint(juce::Graphics& g) override;
    void mouseDown        (const juce::MouseEvent& e) override;
    void mouseDrag        (const juce::MouseEvent& e) override;
    void mouseUp          (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

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

// ─── GlobalTouchPlate ─────────────────────────────────────────────────────────
// Wide pad that triggers all PAD-mode voices simultaneously.

class GlobalTouchPlate : public juce::Component
{
public:
    explicit GlobalTouchPlate(PluginProcessor& p);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp  (const juce::MouseEvent&) override;

private:
    PluginProcessor& proc_;
    bool pressed_ = false;
};

// ─── ScaleKeyboard ────────────────────────────────────────────────────────────
// 12 piano-style toggle buttons for custom scale entry.
// Also highlights which pitch classes are active in the current scale preset,
// shifted by the globalTranspose parameter.

class ScaleKeyboard : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit ScaleKeyboard(juce::AudioProcessorValueTreeState& apvts);
    ~ScaleKeyboard() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Returns the current scale bitmask (bit N = pitch class N is in scale, already transposed).
    uint16_t getActiveScaleMask() const { return activeScaleMask_; }

private:
    juce::AudioProcessorValueTreeState& apvts_;

    // Bitmask of which pitch classes (0..11) are in the active scale
    // after applying transpose. Bit 0 = C, bit 1 = C#, etc.
    uint16_t activeScaleMask_ = 0;

    void parameterChanged(const juce::String& paramID, float newValue) override;
    void updateScaleMask();

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
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& proc_;

    // ── ScaleSnapSlider ───────────────────────────────────────────────────────
    // Snaps drag values to only pitch classes present in the current scale.
    // scaleMask_ bit N = 1 means absolute pitch class N is in the scale (same
    // format as ScaleKeyboard::activeScaleMask_). transpose_ = globalTranspose.
    class ScaleSnapSlider : public juce::Slider
    {
    public:
        void setScaleInfo(uint16_t mask, int tr) { scaleMask_ = mask; transpose_ = tr; }
        double snapValue(double v, DragMode) override
        {
            const int iv = juce::jlimit(0, 12, static_cast<int>(std::round(v)));
            auto valid = [&](int x) -> bool
            {
                if (x < 0 || x > 12) return false;
                return ((scaleMask_ >> ((transpose_ + x + 1200) % 12)) & 1) != 0;
            };
            if (valid(iv)) return iv;
            for (int r = 1; r <= 12; ++r)
            {
                if (valid(iv + r)) return iv + r;
                if (valid(iv - r)) return iv - r;
            }
            return iv;
        }
    private:
        uint16_t scaleMask_ = 0xFFF;  // all notes valid by default
        int      transpose_ = 0;
    };

    PixelLookAndFeel pixelLaf_;
    juce::Font       pixelFont_ { 10.0f };

    void timerCallback() override;

    // ── Joystick ──────────────────────────────────────────────────────────────
    JoystickPad joystickPad_;

    juce::Slider  joyXAttenKnob_, joyYAttenKnob_;
    juce::Label   joyXAttenLabel_, joyYAttenLabel_;

    // ── Chord intervals ───────────────────────────────────────────────────────
    juce::Slider    transposeKnob_;
    ScaleSnapSlider thirdIntKnob_, fifthIntKnob_, tensionIntKnob_;
    juce::Label   transposeLabel_, thirdIntLabel_, fifthIntLabel_, tensionIntLabel_;

    // ── Octave offsets ────────────────────────────────────────────────────────
    juce::Slider  rootOctKnob_, thirdOctKnob_, fifthOctKnob_, tensionOctKnob_;
    juce::Label   rootOctLabel_, thirdOctLabel_, fifthOctLabel_, tensionOctLabel_;

    // ── Touchplates ───────────────────────────────────────────────────────────
    TouchPlate       padRoot_, padThird_, padFifth_, padTension_;
    GlobalTouchPlate padAll_;
    std::array<juce::TextButton, 4> padHoldBtn_;
    juce::TextButton padSubOctBtn_[4];
    std::unique_ptr<juce::ButtonParameterAttachment> subOctAttach_[4];

    // ── Scale ─────────────────────────────────────────────────────────────────
    ScaleKeyboard scaleKeys_;
    juce::ComboBox scalePresetBox_;
    juce::Label    scalePresetLabel_;
    juce::ToggleButton customScaleToggle_;

    // ── Trigger source per voice ──────────────────────────────────────────────
    juce::ComboBox trigSrc_[4];
    juce::Label    trigSrcLabel_[4];

    // ── Routing panel ─────────────────────────────────────────────────────────
    juce::Label    routingLabel_;            // "Routing:" section header
    juce::ComboBox routingModeBox_;          // Multi-Channel / Single Channel
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routingModeAtt_;

    juce::ComboBox singleChanTargetBox_;     // visible in Single Channel mode
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> singleChanTargetAtt_;

    // Per-voice channel dropdowns: visible in Multi-Channel mode only.
    // First UI for the existing voiceCh0..3 APVTS params.
    juce::Label    voiceChLabel_[4];         // "Root ch:", "Third ch:", etc.
    juce::ComboBox voiceChBox_[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceChAtt_[4];
    juce::Slider   randomDensityKnob_;
    juce::Label    randomDensityLabel_;
    std::array<juce::ComboBox, 4>                                    randomSubdivBox_;
    std::array<std::unique_ptr<juce::ComboBoxParameterAttachment>, 4> randomSubdivAtt_;
    juce::Slider                                                     randomGateTimeKnob_;
    std::unique_ptr<juce::SliderParameterAttachment>                  randomGateTimeKnobAtt_;
    juce::Label    randomSubdivLabel_;
    juce::TextButton                                                 randomSyncButton_;
    juce::Slider                                                     randomFreeTempoKnob_;
    std::unique_ptr<juce::ButtonParameterAttachment>                 randomSyncButtonAtt_;
    std::unique_ptr<juce::SliderParameterAttachment>                 randomFreeTempoKnobAtt_;

    // ── Joystick threshold ────────────────────────────────────────────────────
    juce::Slider thresholdSlider_;
    std::unique_ptr<juce::SliderParameterAttachment> thresholdSliderAtt_;

    // Last-seen globalTranspose — used to detect changes and repaint interval knob text boxes.
    int lastTransposeForKnobs_ = -1;

    // Last-seen scale mask — used to auto-snap interval values when scale/key changes.
    uint16_t lastScaleMaskForSnap_ = 0xFFFF;

    // Last-seen scale preset index — detects preset changes so intervals can be
    // reset to natural positions (3rd=4, 5th=7, tension=11) before snapping.
    // Initialised to current value in constructor so loading a saved state never triggers.
    int lastScalePreset_ = -1;

    // Flash counters for gamepad button feedback (decremented by timer)
    int resetFlashCounter_    = 0;
    int deleteFlashCounter_   = 0;
    int panicFlashCounter_    = 0;
    int recBlinkCounter_      = 0;  // blink REC button while armed-but-not-recording
    int playWaitBlinkCounter_ = 0;  // blink PLAY button while wait-for-touch is armed
    int arpBlinkCounter_      = 0;  // blink ARP button while armed, waiting for DAW play
    float beatPulse_          = 0.0f;  // 0.0-1.0 — decays in timerCallback for beat dot

    // Divider line X position — set in resized(), used in paint() so it's
    // independent of the joystick pad's centered position.
    int dividerX_ = 460;

    // True while the physical stick is (or was recently) the last writer of
    // joystickX/Y — lets us correctly return the cursor to 0 on stick release
    // while still allowing mouse clicks to "stick" when the stick is at rest.
    bool gamepadWasLastPitchWriter_ = false;

    // Tooltip window — shows setTooltip() text on hover for all child controls
    juce::TooltipWindow tooltipWindow_;

    // Bounds of the filter knob group panels (set in resized, drawn in paint)
    juce::Rectangle<int> filterCutGroupBounds_, filterResGroupBounds_;

    // ── Filter (gamepad) ─────────────────────────────────────────────────────
    juce::Slider filterXAttenKnob_,  filterYAttenKnob_;
    juce::Slider filterXOffsetKnob_, filterYOffsetKnob_;
    juce::Label  filterXAttenLabel_,  filterYAttenLabel_;
    juce::Label  filterXOffsetLabel_, filterYOffsetLabel_;
    juce::TextButton filterModBtn_;   // [FILTER MOD ON] / [FILTER MOD OFF]
    juce::TextButton filterRecBtn_;   // [REC FILTER] — record filter movements into the looper

    // ── Looper ────────────────────────────────────────────────────────────────
    juce::TextButton  loopPlayBtn_, loopRecBtn_, loopResetBtn_, loopDeleteBtn_;
    juce::TextButton  loopRecJoyBtn_;    // [REC JOY]
    juce::TextButton  loopRecGatesBtn_;  // [REC GATES]
    juce::TextButton  loopSyncBtn_;      // [DAW SYNC]
    juce::TextButton  loopRecWaitBtn_;   // [START REC BY TOUCH] starts recording immediately on touch
    juce::Label       bpmDisplayLabel_;  // shows effective BPM (free tempo or DAW BPM)
    juce::ComboBox    loopSubdivBox_;
    juce::Label       loopSubdivLabel_;
    juce::Slider      loopLengthKnob_;
    juce::Label       loopLengthLabel_;
    juce::Label       gamepadStatusLabel_;
    juce::Label       optionLabel_;       // "OPTION" indicator — green when preset-scroll active
    juce::Label       chordNameLabel_;    // chord name display (e.g. "Cmaj7"), bottom-right
    juce::TextButton  gamepadActiveBtn_;  // [GAMEPAD ON] / [GAMEPAD OFF] per-instance toggle
    juce::TextButton panicBtn_;   // [PANIC!] — one-shot MIDI all-notes-off on all 16 channels
    juce::Label       filterModHintLabel_;  // bottom-right hint text for Filter Mod behaviour
    juce::ComboBox    filterYModeBox_;    // Y axis: Resonance / LFO Rate / Mod Wheel
    juce::ComboBox    filterXModeBox_;    // X axis: Cutoff / VCF LFO / Mod Wheel

    // ── Quantize (inside Looper section) ─────────────────────────────────────
    juce::TextButton quantizeOffBtn_;   // "Off"
    juce::TextButton quantizeLiveBtn_;  // "Live"
    juce::TextButton quantizePostBtn_;  // "Post"
    juce::ComboBox   quantizeSubdivBox_;

    // ── Arpeggiator ───────────────────────────────────────────────────────────
    juce::TextButton arpEnabledBtn_;
    juce::ComboBox   arpSubdivBox_;
    juce::Label      arpSubdivLabel_;
    juce::ComboBox   arpOrderBox_;
    juce::Label      arpOrderLabel_;
    juce::Slider     arpGateTimeKnob_;
    juce::Label      arpGateTimeLabel_;
    juce::Rectangle<int> arpBlockBounds_;  // for drawing the panel in paint()

    // ── LFO panels ─────────────────────────────────────────────────────────────
    juce::ComboBox   lfoXShapeBox_,    lfoYShapeBox_;
    juce::Slider     lfoXRateSlider_,  lfoYRateSlider_;
    juce::Slider     lfoXPhaseSlider_, lfoYPhaseSlider_;
    juce::Slider     lfoXLevelSlider_, lfoYLevelSlider_;
    juce::Slider     lfoXDistSlider_,  lfoYDistSlider_;
    juce::TextButton lfoXSyncBtn_,     lfoYSyncBtn_;

    // Panel bounds (set in resized(), drawn in paint())
    juce::Rectangle<int> lfoXPanelBounds_, lfoYPanelBounds_;

    // Enabled toggle buttons — visible ON/OFF buttons with ButtonAttachment
    juce::TextButton lfoXEnabledBtn_, lfoYEnabledBtn_;

    // Panel bounds for section drawing — set in resized(), read in paint()
    juce::Rectangle<int> looperPanelBounds_;       // LOOPER section panel
    juce::Rectangle<int> filterModPanelBounds_;    // FILTER MOD section panel
    juce::Rectangle<int> gamepadPanelBounds_;      // GAMEPAD section panel
    juce::Rectangle<int> looperPositionBarBounds_; // progress bar strip (inside LOOPER panel)

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAtt  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt   = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt  = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> joyXAttenAtt_, joyYAttenAtt_;
    std::unique_ptr<SliderAtt> transposeAtt_, thirdIntAtt_, fifthIntAtt_, tensionIntAtt_;
    std::unique_ptr<SliderAtt> rootOctAtt_, thirdOctAtt_, fifthOctAtt_, tensionOctAtt_;
    std::unique_ptr<SliderAtt> randomDensityAtt_, loopLengthAtt_;
    std::unique_ptr<SliderAtt> filterXAttenAtt_,  filterYAttenAtt_;
    std::unique_ptr<SliderAtt> filterXOffsetAtt_, filterYOffsetAtt_;
    std::unique_ptr<ComboAtt>  scalePresetAtt_;
    std::unique_ptr<ButtonAtt> customScaleAtt_;
    std::unique_ptr<ComboAtt>  filterYModeAtt_, filterXModeAtt_;
    std::unique_ptr<ComboAtt>  trigSrcAtt_[4];
    std::unique_ptr<ComboAtt>  loopSubdivAtt_;
    std::unique_ptr<ComboAtt>  quantizeSubdivAtt_;

    std::unique_ptr<ButtonAtt>  arpEnabledAtt_;
    std::unique_ptr<ComboAtt>   arpSubdivAtt_;
    std::unique_ptr<ComboAtt>   arpOrderAtt_;
    std::unique_ptr<SliderAtt>  arpGateTimeAtt_;

    // LFO X
    std::unique_ptr<ComboAtt>  lfoXShapeAtt_;
    std::unique_ptr<SliderAtt> lfoXPhaseAtt_, lfoXLevelAtt_, lfoXDistAtt_;
    std::unique_ptr<ButtonAtt> lfoXSyncAtt_, lfoXEnabledAtt_;
    // Rate slider uses SliderParameterAttachment (swap on sync toggle)
    std::unique_ptr<juce::SliderParameterAttachment> lfoXRateAtt_;

    // LFO Y
    std::unique_ptr<ComboAtt>  lfoYShapeAtt_;
    std::unique_ptr<SliderAtt> lfoYPhaseAtt_, lfoYLevelAtt_, lfoYDistAtt_;
    std::unique_ptr<ButtonAtt> lfoYSyncAtt_, lfoYEnabledAtt_;
    std::unique_ptr<juce::SliderParameterAttachment> lfoYRateAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
