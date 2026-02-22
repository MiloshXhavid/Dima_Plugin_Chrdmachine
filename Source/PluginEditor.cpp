#include "PluginEditor.h"
#include "ScaleQuantizer.h"
#include <cmath>

// ─── Colours ─────────────────────────────────────────────────────────────────
namespace Clr
{
    static const juce::Colour bg        { 0xFF1A1A2E };
    static const juce::Colour panel     { 0xFF16213E };
    static const juce::Colour accent    { 0xFF0F3460 };
    static const juce::Colour highlight { 0xFFE94560 };
    static const juce::Colour text      { 0xFFEAEAEA };
    static const juce::Colour textDim   { 0xFF888899 };
    static const juce::Colour gateOn    { 0xFF4CAF50 };
    static const juce::Colour gateOff   { 0xFF333355 };
}

// ═══════════════════════════════════════════════════════════════════════════════
// JoystickPad
// ═══════════════════════════════════════════════════════════════════════════════

JoystickPad::JoystickPad(PluginProcessor& p) : proc_(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void JoystickPad::updateFromMouse(const juce::MouseEvent& e)
{
    const float w  = static_cast<float>(getWidth());
    const float h  = static_cast<float>(getHeight());
    const float nx = juce::jlimit(-1.0f, 1.0f, (e.x / w) * 2.0f - 1.0f);
    const float ny = juce::jlimit(-1.0f, 1.0f, 1.0f - (e.y / h) * 2.0f);  // Y flipped
    proc_.joystickX.store(nx);
    proc_.joystickY.store(ny);
    repaint();
}

void JoystickPad::mouseDown (const juce::MouseEvent& e) { updateFromMouse(e); }
void JoystickPad::mouseDrag (const juce::MouseEvent& e) { updateFromMouse(e); }
void JoystickPad::mouseUp   (const juce::MouseEvent& e) { (void)e; }

void JoystickPad::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(Clr::accent);
    g.fillRoundedRectangle(b, 6.0f);

    // Grid lines
    g.setColour(Clr::panel);
    g.drawLine(b.getCentreX(), b.getY(),    b.getCentreX(), b.getBottom(), 1.0f);
    g.drawLine(b.getX(),       b.getCentreY(), b.getRight(), b.getCentreY(), 1.0f);

    // Cursor dot
    const float cx = (proc_.joystickX.load() + 1.0f) * 0.5f * b.getWidth()  + b.getX();
    const float cy = (1.0f - (proc_.joystickY.load() + 1.0f) * 0.5f) * b.getHeight() + b.getY();

    g.setColour(Clr::highlight);
    g.fillEllipse(cx - 8, cy - 8, 16, 16);
    g.setColour(Clr::text);
    g.drawEllipse(cx - 8, cy - 8, 16, 16, 1.5f);

    // Border
    g.setColour(Clr::highlight.withAlpha(0.5f));
    g.drawRoundedRectangle(b.reduced(1), 6.0f, 1.5f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// TouchPlate
// ═══════════════════════════════════════════════════════════════════════════════

TouchPlate::TouchPlate(PluginProcessor& p, int voice, const juce::String& label)
    : proc_(p), voice_(voice), label_(label) {}

void TouchPlate::mouseDown(const juce::MouseEvent&)
{
    const int src = static_cast<int>(
        proc_.apvts.getRawParameterValue("triggerSource" + juce::String(voice_))->load());
    if (src == 0)  // only active in PAD mode
    {
        proc_.setPadState(voice_, true);
        repaint();
    }
}

void TouchPlate::mouseUp(const juce::MouseEvent&)
{
    const int src = static_cast<int>(
        proc_.apvts.getRawParameterValue("triggerSource" + juce::String(voice_))->load());
    if (src == 0 && !proc_.padHold_[voice_].load())
        proc_.setPadState(voice_, false);
    repaint();  // always repaint so dim state stays current
}

void TouchPlate::paint(juce::Graphics& g)
{
    const int src = static_cast<int>(
        proc_.apvts.getRawParameterValue("triggerSource" + juce::String(voice_))->load());
    const bool isPadMode = (src == 0);  // 0 = TouchPlate
    const bool active    = isPadMode && proc_.isGateOpen(voice_);

    const juce::Colour fillClr = isPadMode
        ? (active ? Clr::gateOn : Clr::gateOff)
        : Clr::gateOff.darker(0.3f);  // dimmed in JOY or RND mode

    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    g.setColour(isPadMode ? Clr::text : Clr::textDim);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);

    g.setColour((active ? Clr::gateOn : Clr::accent).brighter(isPadMode ? 0.2f : 0.0f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.5f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GlobalTouchPlate
// ═══════════════════════════════════════════════════════════════════════════════

GlobalTouchPlate::GlobalTouchPlate(PluginProcessor& p) : proc_(p) {}

void GlobalTouchPlate::mouseDown(const juce::MouseEvent&)
{
    pressed_ = true;
    // Trigger only voices that are in PAD mode (triggerSource == 0)
    for (int v = 0; v < 4; ++v)
    {
        const int src = static_cast<int>(
            proc_.apvts.getRawParameterValue("triggerSource" + juce::String(v))->load());
        if (src == 0)
            proc_.setPadState(v, true);
    }
    repaint();
}

void GlobalTouchPlate::mouseUp(const juce::MouseEvent&)
{
    pressed_ = false;
    for (int v = 0; v < 4; ++v)
    {
        const int src = static_cast<int>(
            proc_.apvts.getRawParameterValue("triggerSource" + juce::String(v))->load());
        if (src == 0)
            proc_.setPadState(v, false);
    }
    repaint();
}

void GlobalTouchPlate::paint(juce::Graphics& g)
{
    // Check if at least one voice is in PAD mode
    bool anyPadMode = false;
    for (int v = 0; v < 4; ++v)
    {
        const int src = static_cast<int>(
            proc_.apvts.getRawParameterValue("triggerSource" + juce::String(v))->load());
        if (src == 0) { anyPadMode = true; break; }
    }

    // Distinct colour: a teal/cyan hue, different from the per-voice pads
    static const juce::Colour allGateOff  { 0xFF1A3A5C };  // teal-navy
    static const juce::Colour allGateOn   { 0xFF00BCD4 };  // cyan/teal when pressed
    static const juce::Colour allDisabled { 0xFF1A1A2E };  // same as bg when all JOY/RND

    const juce::Colour fillClr = !anyPadMode ? allDisabled
                                : pressed_   ? allGateOn
                                             : allGateOff;

    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    g.setColour(anyPadMode ? Clr::text : Clr::textDim);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("ALL", getLocalBounds(), juce::Justification::centred);

    const juce::Colour borderClr = pressed_ ? allGateOn.brighter(0.3f)
                                             : (anyPadMode ? allGateOff.brighter(0.4f)
                                                           : Clr::accent);
    g.setColour(borderClr);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.5f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ScaleKeyboard
// ═══════════════════════════════════════════════════════════════════════════════

const bool ScaleKeyboard::kIsBlack[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

ScaleKeyboard::ScaleKeyboard(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    // Listen for scale/transpose parameter changes so we can repaint highlights
    apvts_.addParameterListener("scalePreset",    this);
    apvts_.addParameterListener("useCustomScale", this);
    apvts_.addParameterListener("globalTranspose", this);
    for (int i = 0; i < 12; ++i)
        apvts_.addParameterListener("scaleNote" + juce::String(i), this);

    // Compute initial mask
    updateScaleMask();
}

ScaleKeyboard::~ScaleKeyboard()
{
    apvts_.removeParameterListener("scalePreset",    this);
    apvts_.removeParameterListener("useCustomScale", this);
    apvts_.removeParameterListener("globalTranspose", this);
    for (int i = 0; i < 12; ++i)
        apvts_.removeParameterListener("scaleNote" + juce::String(i), this);
}

void ScaleKeyboard::parameterChanged(const juce::String& /*paramID*/, float /*newValue*/)
{
    updateScaleMask();
    // Parameter listeners fire on a background thread — repaint must be on
    // the message thread.
    juce::MessageManager::callAsync([this] { repaint(); });
}

void ScaleKeyboard::updateScaleMask()
{
    const bool useCustom = apvts_.getRawParameterValue("useCustomScale")->load() > 0.5f;
    const int  transpose = static_cast<int>(
        apvts_.getRawParameterValue("globalTranspose")->load());

    // Build a set of which pitch classes are in the scale (before transpose)
    bool pitchClasses[12] = {};

    if (useCustom)
    {
        for (int i = 0; i < 12; ++i)
            pitchClasses[i] =
                apvts_.getRawParameterValue("scaleNote" + juce::String(i))->load() > 0.5f;
    }
    else
    {
        const int presetIdx = static_cast<int>(
            apvts_.getRawParameterValue("scalePreset")->load());
        const ScalePreset preset = static_cast<ScalePreset>(
            juce::jlimit(0, (int)ScalePreset::COUNT - 1, presetIdx));
        const int* pattern = ScaleQuantizer::getScalePattern(preset);
        const int  size    = ScaleQuantizer::getScaleSize(preset);
        for (int i = 0; i < size; ++i)
            pitchClasses[pattern[i]] = true;
    }

    // Apply transpose: shift each active pitch class by transpose semitones (mod 12)
    uint16_t mask = 0;
    for (int pc = 0; pc < 12; ++pc)
    {
        if (pitchClasses[pc])
        {
            int shifted = ((pc + transpose) % 12 + 12) % 12;
            mask |= static_cast<uint16_t>(1u << shifted);
        }
    }
    activeScaleMask_ = mask;
}

bool ScaleKeyboard::isNoteActive(int note) const
{
    return apvts_.getRawParameterValue("scaleNote" + juce::String(note))->load() > 0.5f;
}

juce::Rectangle<float> ScaleKeyboard::noteRect(int note) const
{
    const float w    = static_cast<float>(getWidth());
    const float h    = static_cast<float>(getHeight());
    const float wKey = w / 7.0f;

    // White key positions (chromatic → white-key-index)
    static const int whiteIdx[12] = {0,0,1,1,2,3,3,4,4,5,5,6};
    static const bool isBlack[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

    if (!isBlack[note])
    {
        float x = whiteIdx[note] * wKey;
        return { x, 0, wKey - 1, h };
    }
    else
    {
        float x = whiteIdx[note] * wKey + wKey * 0.6f;
        return { x, 0, wKey * 0.7f, h * 0.6f };
    }
}

void ScaleKeyboard::mouseDown(const juce::MouseEvent& e)
{
    // Find which key was clicked and toggle it
    for (int n = 0; n < 12; ++n)
    {
        if (noteRect(n).contains(e.position))
        {
            const bool cur = isNoteActive(n);
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                    apvts_.getParameter("scaleNote" + juce::String(n))))
                *p = !cur;
            repaint();
            return;
        }
    }
}

void ScaleKeyboard::paint(juce::Graphics& g)
{
    // Colours for scale-highlight states:
    //   custom-toggle active (editing mode)  -> use highlight red as before
    //   scale preset active note             -> teal / active-teal
    //   normal inactive                      -> white / black
    static const juce::Colour scaleNatural { 0xFF4DD0E1 };   // light cyan-teal (white key in scale)
    static const juce::Colour scaleSharp   { 0xFF00838F };   // darker teal (black key in scale)

    // When Custom toggle is on, show user-toggled notes (edit mode).
    // When off, show preset scale highlights shifted by transpose.
    const bool editingCustom =
        apvts_.getRawParameterValue("useCustomScale")->load() > 0.5f;

    // White keys first
    for (int n = 0; n < 12; ++n)
    {
        if (kIsBlack[n]) continue;
        auto r = noteRect(n);

        juce::Colour fill;
        if (editingCustom)
        {
            // Custom editing mode: highlight = user-toggled note (red)
            fill = isNoteActive(n) ? Clr::highlight : juce::Colours::white;
        }
        else
        {
            // Preset mode: highlight = scale pitch class after transpose
            const bool inScale = (activeScaleMask_ >> n) & 1;
            fill = inScale ? scaleNatural : juce::Colours::white;
        }

        g.setColour(fill);
        g.fillRect(r);
        g.setColour(juce::Colours::black);
        g.drawRect(r);
    }
    // Black keys on top
    for (int n = 0; n < 12; ++n)
    {
        if (!kIsBlack[n]) continue;
        auto r = noteRect(n);

        juce::Colour fill;
        if (editingCustom)
        {
            fill = isNoteActive(n) ? Clr::highlight : juce::Colours::black;
        }
        else
        {
            const bool inScale = (activeScaleMask_ >> n) & 1;
            fill = inScale ? scaleSharp : juce::Colours::black;
        }

        g.setColour(fill);
        g.fillRect(r);
    }

    // Draw borders on black keys for visibility
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    for (int n = 0; n < 12; ++n)
    {
        if (!kIsBlack[n]) continue;
        g.drawRect(noteRect(n), 0.5f);
    }
}

void ScaleKeyboard::resized() {}

// ═══════════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════════

static void styleKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    s.setColour(juce::Slider::thumbColourId,             Clr::text);
    s.setColour(juce::Slider::textBoxTextColourId,       Clr::textDim);
    s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
}

static void styleLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(11.0f));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, Clr::textDim);
}

static void styleCombo(juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId, Clr::accent);
    c.setColour(juce::ComboBox::textColourId,       Clr::text);
    c.setColour(juce::ComboBox::arrowColourId,      Clr::highlight);
    c.setColour(juce::ComboBox::outlineColourId,    Clr::accent.brighter(0.2f));
}

static void styleButton(juce::TextButton& b, juce::Colour c = Clr::accent)
{
    b.setColour(juce::TextButton::buttonColourId,     c);
    b.setColour(juce::TextButton::buttonOnColourId,   Clr::highlight);
    b.setColour(juce::TextButton::textColourOffId,    Clr::text);
    b.setColour(juce::TextButton::textColourOnId,     Clr::text);
}

// ═══════════════════════════════════════════════════════════════════════════════
// PluginEditor constructor
// ═══════════════════════════════════════════════════════════════════════════════

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), proc_(p),
      joystickPad_(p),
      padRoot_   (p, 0, "ROOT"),
      padThird_  (p, 1, "3RD"),
      padFifth_  (p, 2, "5TH"),
      padTension_(p, 3, "TEN"),
      padAll_    (p),
      scaleKeys_ (p.apvts)
{
    setSize(920, 700);

    // ── Joystick ──────────────────────────────────────────────────────────────
    addAndMakeVisible(joystickPad_);
    styleKnob(joyXAttenKnob_); styleLabel(joyXAttenLabel_, "X Range");
    styleKnob(joyYAttenKnob_); styleLabel(joyYAttenLabel_, "Y Range");
    addAndMakeVisible(joyXAttenKnob_); addAndMakeVisible(joyXAttenLabel_);
    addAndMakeVisible(joyYAttenKnob_); addAndMakeVisible(joyYAttenLabel_);

    joyXAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "joystickXAtten", joyXAttenKnob_);
    joyYAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "joystickYAtten", joyYAttenKnob_);

    // ── Touchplates ───────────────────────────────────────────────────────────
    addAndMakeVisible(padRoot_); addAndMakeVisible(padThird_);
    addAndMakeVisible(padFifth_); addAndMakeVisible(padTension_);
    addAndMakeVisible(padAll_);

    // ── Per-voice hold buttons (overlaid on top of each touchplate) ───────────
    for (int v = 0; v < 4; ++v)
    {
        padHoldBtn_[v].setButtonText("HOLD");
        padHoldBtn_[v].setClickingTogglesState(true);
        padHoldBtn_[v].setTooltip("Hold — pad stays on after release. Toggle off to send note-off.");
        styleButton(padHoldBtn_[v]);
        addAndMakeVisible(padHoldBtn_[v]);
        padHoldBtn_[v].onClick = [this, v]()
        {
            const bool nowHeld = padHoldBtn_[v].getToggleState();
            proc_.padHold_[v].store(nowHeld);
            if (nowHeld)
                proc_.setPadState(v, true);   // note-on when hold activates
            else
                proc_.setPadState(v, false);  // note-off when hold releases
        };
    }

    // ── Chord intervals ───────────────────────────────────────────────────────
    styleKnob(transposeKnob_);   styleLabel(transposeLabel_,  "Transpose");
    styleKnob(thirdIntKnob_);    styleLabel(thirdIntLabel_,   "3rd Intv");
    styleKnob(fifthIntKnob_);    styleLabel(fifthIntLabel_,   "5th Intv");
    styleKnob(tensionIntKnob_);  styleLabel(tensionIntLabel_, "Ten Intv");
    addAndMakeVisible(transposeKnob_);  addAndMakeVisible(transposeLabel_);
    addAndMakeVisible(thirdIntKnob_);   addAndMakeVisible(thirdIntLabel_);
    addAndMakeVisible(fifthIntKnob_);   addAndMakeVisible(fifthIntLabel_);
    addAndMakeVisible(tensionIntKnob_); addAndMakeVisible(tensionIntLabel_);

    transposeAtt_  = std::make_unique<SliderAtt>(p.apvts, "globalTranspose", transposeKnob_);
    thirdIntAtt_   = std::make_unique<SliderAtt>(p.apvts, "thirdInterval",   thirdIntKnob_);
    fifthIntAtt_   = std::make_unique<SliderAtt>(p.apvts, "fifthInterval",   fifthIntKnob_);
    tensionIntAtt_ = std::make_unique<SliderAtt>(p.apvts, "tensionInterval", tensionIntKnob_);

    // ── Octave offsets ────────────────────────────────────────────────────────
    styleKnob(rootOctKnob_);    styleLabel(rootOctLabel_,    "Root Oct");
    styleKnob(thirdOctKnob_);   styleLabel(thirdOctLabel_,   "3rd Oct");
    styleKnob(fifthOctKnob_);   styleLabel(fifthOctLabel_,   "5th Oct");
    styleKnob(tensionOctKnob_); styleLabel(tensionOctLabel_, "Ten Oct");
    addAndMakeVisible(rootOctKnob_);    addAndMakeVisible(rootOctLabel_);
    addAndMakeVisible(thirdOctKnob_);   addAndMakeVisible(thirdOctLabel_);
    addAndMakeVisible(fifthOctKnob_);   addAndMakeVisible(fifthOctLabel_);
    addAndMakeVisible(tensionOctKnob_); addAndMakeVisible(tensionOctLabel_);

    rootOctAtt_    = std::make_unique<SliderAtt>(p.apvts, "rootOctave",    rootOctKnob_);
    thirdOctAtt_   = std::make_unique<SliderAtt>(p.apvts, "thirdOctave",   thirdOctKnob_);
    fifthOctAtt_   = std::make_unique<SliderAtt>(p.apvts, "fifthOctave",   fifthOctKnob_);
    tensionOctAtt_ = std::make_unique<SliderAtt>(p.apvts, "tensionOctave", tensionOctKnob_);

    // ── Scale ─────────────────────────────────────────────────────────────────
    addAndMakeVisible(scaleKeys_);
    addAndMakeVisible(scalePresetBox_);
    addAndMakeVisible(scalePresetLabel_);
    addAndMakeVisible(customScaleToggle_);

    styleLabel(scalePresetLabel_, "Scale Preset");
    customScaleToggle_.setButtonText("Custom");
    customScaleToggle_.setColour(juce::ToggleButton::textColourId, Clr::text);

    for (int i = 0; i < (int)ScalePreset::COUNT; ++i)
        scalePresetBox_.addItem(ScaleQuantizer::getScaleName(static_cast<ScalePreset>(i)), i + 1);

    styleCombo(scalePresetBox_);
    scalePresetAtt_ = std::make_unique<ComboAtt>(p.apvts, "scalePreset", scalePresetBox_);
    customScaleAtt_ = std::make_unique<ButtonAtt>(p.apvts, "useCustomScale", customScaleToggle_);

    // ── Trigger sources ───────────────────────────────────────────────────────
    static const juce::String trigLabels[4] = {"Root Trig", "3rd Trig", "5th Trig", "Ten Trig"};
    static const juce::String trigSrcIDs[4] = {
        "triggerSource0","triggerSource1","triggerSource2","triggerSource3" };

    for (int i = 0; i < 4; ++i)
    {
        trigSrc_[i].addItem("Pad",      1);
        trigSrc_[i].addItem("Joystick", 2);
        trigSrc_[i].addItem("Random",   3);
        styleCombo(trigSrc_[i]);
        styleLabel(trigSrcLabel_[i], trigLabels[i]);
        addAndMakeVisible(trigSrc_[i]);
        addAndMakeVisible(trigSrcLabel_[i]);
        trigSrcAtt_[i] = std::make_unique<ComboAtt>(p.apvts, trigSrcIDs[i], trigSrc_[i]);
    }

    randomDensityKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomDensityKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    randomDensityKnob_.setTooltip("Random density (hits per bar, 1-8)");
    randomDensityKnob_.setColour(juce::Slider::rotarySliderFillColourId,   Clr::highlight);
    randomDensityKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomDensityKnob_.setColour(juce::Slider::thumbColourId,              Clr::text);
    addAndMakeVisible(randomDensityKnob_);
    randomDensityAtt_ = std::make_unique<SliderAtt>(p.apvts, "randomDensity", randomDensityKnob_);

    styleLabel(randomSubdivLabel_, "SUBDIV");
    addAndMakeVisible(randomSubdivLabel_);
    {
        const juce::StringArray subdivChoices { "1/4", "1/8", "1/16", "1/32" };
        for (int v = 0; v < 4; ++v)
        {
            randomSubdivBox_[v].addItemList(subdivChoices, 1);
            randomSubdivBox_[v].setSelectedItemIndex(1, juce::dontSendNotification);
            styleCombo(randomSubdivBox_[v]);
            addAndMakeVisible(randomSubdivBox_[v]);
            randomSubdivAtt_[v] = std::make_unique<juce::ComboBoxParameterAttachment>(
                *p.apvts.getParameter("randomSubdiv" + juce::String(v)), randomSubdivBox_[v]);
        }
    }

    randomGateTimeKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomGateTimeKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    randomGateTimeKnob_.setTooltip("Random gate time (fraction of subdivision)");
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomGateTimeKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    addAndMakeVisible(randomGateTimeKnob_);
    randomGateTimeKnobAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomGateTime"), randomGateTimeKnob_);

    // Sync toggle
    randomSyncButton_.setButtonText("SYNC");
    randomSyncButton_.setClickingTogglesState(true);
    randomSyncButton_.setToggleState(true, juce::dontSendNotification);
    randomSyncButton_.setTooltip("When ON: random triggers only fire while DAW plays. When OFF: free tempo.");
    styleButton(randomSyncButton_);
    addAndMakeVisible(randomSyncButton_);
    randomSyncButtonAtt_ = std::make_unique<juce::ButtonParameterAttachment>(
        *p.apvts.getParameter("randomClockSync"), randomSyncButton_);

    // Free tempo knob
    randomFreeTempoKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomFreeTempoKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    randomFreeTempoKnob_.setTooltip("Random free tempo (BPM, 30-240). Active when SYNC is OFF.");
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomFreeTempoKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    addAndMakeVisible(randomFreeTempoKnob_);
    randomFreeTempoKnobAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomFreeTempo"), randomFreeTempoKnob_);

    // ── Filter attenuators ────────────────────────────────────────────────────
    styleKnob(filterXAttenKnob_); styleLabel(filterXAttenLabel_, "Cut Atten");
    styleKnob(filterYAttenKnob_); styleLabel(filterYAttenLabel_, "Res Atten");
    addAndMakeVisible(filterXAttenKnob_); addAndMakeVisible(filterXAttenLabel_);
    addAndMakeVisible(filterYAttenKnob_); addAndMakeVisible(filterYAttenLabel_);
    filterXAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXAtten", filterXAttenKnob_);
    filterYAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYAtten", filterYAttenKnob_);

    // ── Joystick threshold slider ─────────────────────────────────────────────
    thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider_.setTooltip("Joystick motion threshold");
    thresholdSlider_.setColour(juce::Slider::trackColourId,      Clr::highlight);
    thresholdSlider_.setColour(juce::Slider::backgroundColourId, Clr::accent);
    thresholdSlider_.setColour(juce::Slider::thumbColourId,      Clr::text);
    addAndMakeVisible(thresholdSlider_);
    thresholdSliderAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("joystickThreshold"), thresholdSlider_);

    // ── Looper ────────────────────────────────────────────────────────────────
    loopPlayBtn_.setButtonText("PLAY");  loopPlayBtn_.setClickingTogglesState(true);
    loopRecBtn_.setButtonText("REC");    loopRecBtn_.setClickingTogglesState(true);
    loopResetBtn_.setButtonText("RST");
    loopDeleteBtn_.setButtonText("DEL");

    styleButton(loopPlayBtn_);  styleButton(loopRecBtn_, juce::Colour(0xFF8B0000));
    styleButton(loopResetBtn_); styleButton(loopDeleteBtn_);

    loopPlayBtn_.onClick  = [this] { proc_.looperStartStop(); };
    loopRecBtn_.onClick   = [this] { proc_.looperRecord();    };
    loopResetBtn_.onClick = [this] { proc_.looperReset();     };
    loopDeleteBtn_.onClick = [this] { proc_.looperDelete();   };

    addAndMakeVisible(loopPlayBtn_); addAndMakeVisible(loopRecBtn_);
    addAndMakeVisible(loopResetBtn_); addAndMakeVisible(loopDeleteBtn_);

    // New looper mode buttons (Phase 05)
    loopRecJoyBtn_.setButtonText("REC JOY");
    loopRecGatesBtn_.setButtonText("REC GATES");
    loopSyncBtn_.setButtonText("SYNC");

    loopRecJoyBtn_.setClickingTogglesState(true);
    loopRecGatesBtn_.setClickingTogglesState(true);
    loopSyncBtn_.setClickingTogglesState(true);

    styleButton(loopRecJoyBtn_,   juce::Colour(0xFF8B0000));
    styleButton(loopRecGatesBtn_, juce::Colour(0xFF8B0000));
    styleButton(loopSyncBtn_);

    loopRecJoyBtn_.onClick = [this] {
        const bool newVal = !proc_.looperIsRecJoy();
        proc_.looperSetRecJoy(newVal);
        loopRecJoyBtn_.setToggleState(newVal, juce::dontSendNotification);
    };
    loopRecGatesBtn_.onClick = [this] {
        const bool newVal = !proc_.looperIsRecGates();
        proc_.looperSetRecGates(newVal);
        loopRecGatesBtn_.setToggleState(newVal, juce::dontSendNotification);
    };
    loopSyncBtn_.onClick = [this] {
        const bool newVal = !proc_.looperIsSyncToDaw();
        proc_.looperSetSyncToDaw(newVal);
        loopSyncBtn_.setToggleState(newVal, juce::dontSendNotification);
    };

    addAndMakeVisible(loopRecJoyBtn_);
    addAndMakeVisible(loopRecGatesBtn_);
    addAndMakeVisible(loopSyncBtn_);

    loopSubdivBox_.addItem("3/4", 1); loopSubdivBox_.addItem("4/4", 2);
    loopSubdivBox_.addItem("5/4", 3); loopSubdivBox_.addItem("7/8", 4);
    loopSubdivBox_.addItem("9/8", 5); loopSubdivBox_.addItem("11/8", 6);
    styleCombo(loopSubdivBox_);
    styleLabel(loopSubdivLabel_, "Time Sig");
    addAndMakeVisible(loopSubdivBox_); addAndMakeVisible(loopSubdivLabel_);
    loopSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "looperSubdiv", loopSubdivBox_);

    styleKnob(loopLengthKnob_); styleLabel(loopLengthLabel_, "Bars");
    addAndMakeVisible(loopLengthKnob_); addAndMakeVisible(loopLengthLabel_);
    loopLengthAtt_ = std::make_unique<SliderAtt>(p.apvts, "looperLength", loopLengthKnob_);

    // Gamepad status
    gamepadStatusLabel_.setText("Gamepad: --", juce::dontSendNotification);
    gamepadStatusLabel_.setFont(juce::Font(10.0f));
    gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(gamepadStatusLabel_);

    p.getGamepad().onConnectionChange = [this](bool connected)
    {
        juce::MessageManager::callAsync([this, connected]
        {
            gamepadStatusLabel_.setText(connected ? "Gamepad: Connected" : "Gamepad: --",
                                        juce::dontSendNotification);
        });
    };

    // Channel conflict warning
    channelConflictLabel_.setText("! Channel conflict", juce::dontSendNotification);
    channelConflictLabel_.setFont(juce::Font(10.0f, juce::Font::bold));
    channelConflictLabel_.setJustificationType(juce::Justification::centred);
    channelConflictLabel_.setColour(juce::Label::textColourId, Clr::highlight);
    channelConflictLabel_.setVisible(false);
    addAndMakeVisible(channelConflictLabel_);

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

// ─── Layout ───────────────────────────────────────────────────────────────────

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    const int rowH   = area.getHeight();
    const int colW   = area.getWidth();

    // Layout: Left column (controls) | Right column (joystick + pads)
    auto left  = area.removeFromLeft(colW / 2 - 4);
    area.removeFromLeft(8);
    auto right = area;

    // ── RIGHT COLUMN ──────────────────────────────────────────────────────────

    // Joystick pad (top right, square-ish)
    const int padSize = juce::jmin(right.getWidth(), right.getHeight() / 2);
    joystickPad_.setBounds(right.removeFromTop(padSize));

    right.removeFromTop(6);

    // Attenuators
    {
        auto row = right.removeFromTop(70);
        joyXAttenLabel_.setBounds(row.removeFromLeft(60).removeFromTop(16));
        joyXAttenKnob_ .setBounds(row.removeFromLeft(60));
        row.removeFromLeft(8);
        joyYAttenLabel_.setBounds(row.removeFromLeft(60).removeFromTop(16));
        joyYAttenKnob_ .setBounds(row.removeFromLeft(60));
    }

    right.removeFromTop(6);

    // Touchplates + hold buttons (HOLD overlaid on top 18px of each pad)
    {
        auto row = right.removeFromTop(70);
        const int pw = (row.getWidth() - 9) / 4;
        juce::Component* pads[4] = { &padRoot_, &padThird_, &padFifth_, &padTension_ };
        for (int v = 0; v < 4; ++v)
        {
            auto b = row.removeFromLeft(pw);
            pads[v]->setBounds(b);
            padHoldBtn_[v].setBounds(b.removeFromTop(18).reduced(2, 2));
            if (v < 3) row.removeFromLeft(3);
        }
    }

    right.removeFromTop(3);

    // Global ALL pad (wide bar spanning the full width of the individual pads)
    {
        auto row = right.removeFromTop(36);
        padAll_.setBounds(row);
    }

    right.removeFromTop(3);

    // Filter attenuators
    {
        auto row = right.removeFromTop(70);
        filterXAttenLabel_.setBounds(row.removeFromLeft(60).removeFromTop(16));
        filterXAttenKnob_ .setBounds(row.removeFromLeft(60));
        row.removeFromLeft(8);
        filterYAttenLabel_.setBounds(row.removeFromLeft(60).removeFromTop(16));
        filterYAttenKnob_ .setBounds(row.removeFromLeft(60));
    }

    right.removeFromTop(6);

    // Threshold slider row (THRESH label + horizontal slider)
    {
        auto threshRow = right.removeFromTop(24);
        thresholdSlider_.setBounds(threshRow);
    }

    right.removeFromTop(4);

    // Gamepad status
    gamepadStatusLabel_.setBounds(right.removeFromTop(16));

    // Channel conflict warning
    channelConflictLabel_.setBounds(right.removeFromTop(16));

    // ── LEFT COLUMN ───────────────────────────────────────────────────────────

    // Scale section
    {
        auto section = left.removeFromTop(100);
        scalePresetLabel_.setBounds(section.removeFromTop(16));
        scalePresetBox_  .setBounds(section.removeFromTop(24));
        customScaleToggle_.setBounds(section.removeFromLeft(70).removeFromTop(22));
        section.removeFromTop(4);
        scaleKeys_.setBounds(section.removeFromTop(50));
    }

    left.removeFromTop(6);

    // Chord intervals
    {
        auto section = left.removeFromTop(80);
        const int kw = section.getWidth() / 4;
        auto placeKnob = [&](juce::Slider& k, juce::Label& l)
        {
            auto col = section.removeFromLeft(kw);
            l.setBounds(col.removeFromTop(14));
            k.setBounds(col);
        };
        placeKnob(transposeKnob_,  transposeLabel_);
        placeKnob(thirdIntKnob_,   thirdIntLabel_);
        placeKnob(fifthIntKnob_,   fifthIntLabel_);
        placeKnob(tensionIntKnob_, tensionIntLabel_);
    }

    left.removeFromTop(6);

    // Octave offsets
    {
        auto section = left.removeFromTop(80);
        const int kw = section.getWidth() / 4;
        auto placeKnob = [&](juce::Slider& k, juce::Label& l)
        {
            auto col = section.removeFromLeft(kw);
            l.setBounds(col.removeFromTop(14));
            k.setBounds(col);
        };
        placeKnob(rootOctKnob_,    rootOctLabel_);
        placeKnob(thirdOctKnob_,   thirdOctLabel_);
        placeKnob(fifthOctKnob_,   fifthOctLabel_);
        placeKnob(tensionOctKnob_, tensionOctLabel_);
    }

    left.removeFromTop(6);

    // Trigger sources
    {
        const int cw = left.getWidth() / 4;

        // Trigger source combo row
        {
            auto section = left.removeFromTop(36);
            for (int i = 0; i < 4; ++i)
            {
                auto col = section.removeFromLeft(cw);
                trigSrcLabel_[i].setBounds(col.removeFromTop(14));
                trigSrc_[i].setBounds(col.removeFromTop(22));
            }
        }

        // Per-voice subdiv combo row (column-aligned with trigger source combos)
        {
            auto subdivHeaderRow = left.removeFromTop(14);
            randomSubdivLabel_.setBounds(subdivHeaderRow);

            auto subdivRow = left.removeFromTop(22);
            for (int v = 0; v < 4; ++v)
            {
                auto col = subdivRow.removeFromLeft(cw);
                randomSubdivBox_[v].setBounds(col.reduced(1, 0));
            }
        }

        left.removeFromTop(14);

        // Random controls row — one control per voice column, aligned under subdiv combos
        // [DENS knob] | [GATE knob] | [FREE BPM knob] | [SYNC button]
        {
            auto rndRow = left.removeFromTop(60);
            randomDensityKnob_ .setBounds(rndRow.removeFromLeft(cw));
            randomGateTimeKnob_.setBounds(rndRow.removeFromLeft(cw));
            randomFreeTempoKnob_.setBounds(rndRow.removeFromLeft(cw));
            randomSyncButton_  .setBounds(rndRow);
        }
    }

    left.removeFromTop(6);

    // Looper
    {
        auto section = left;

        // Buttons row 1: PLAY / REC / RST / DEL
        auto btnRow = section.removeFromTop(36);
        const int bw = btnRow.getWidth() / 4 - 2;
        loopPlayBtn_  .setBounds(btnRow.removeFromLeft(bw)); btnRow.removeFromLeft(2);
        loopRecBtn_   .setBounds(btnRow.removeFromLeft(bw)); btnRow.removeFromLeft(2);
        loopResetBtn_ .setBounds(btnRow.removeFromLeft(bw)); btnRow.removeFromLeft(2);
        loopDeleteBtn_.setBounds(btnRow.removeFromLeft(bw));

        section.removeFromTop(2);

        // Buttons row 2: REC JOY / REC GATES / SYNC (three columns)
        {
            auto row2 = section.removeFromTop(28);
            const int bw3 = (row2.getWidth() - 4) / 3;
            loopRecJoyBtn_  .setBounds(row2.removeFromLeft(bw3)); row2.removeFromLeft(2);
            loopRecGatesBtn_.setBounds(row2.removeFromLeft(bw3)); row2.removeFromLeft(2);
            loopSyncBtn_    .setBounds(row2);
        }

        section.removeFromTop(4);

        // Subdiv + length
        auto ctrlRow = section.removeFromTop(60);
        loopSubdivLabel_.setBounds(ctrlRow.removeFromLeft(70).removeFromTop(14));
        loopSubdivBox_  .setBounds(ctrlRow.removeFromLeft(80).removeFromTop(22));
        ctrlRow.removeFromLeft(10);
        loopLengthLabel_.setBounds(ctrlRow.removeFromLeft(60).removeFromTop(14));
        loopLengthKnob_ .setBounds(ctrlRow.removeFromLeft(60));
    }
}

// ─── Paint ────────────────────────────────────────────────────────────────────

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Clr::bg);

    // Section labels
    g.setColour(Clr::textDim);
    g.setFont(juce::Font(9.5f, juce::Font::bold));

    auto drawSectionTitle = [&](const juce::String& t, juce::Rectangle<int> r)
    {
        g.drawText(t, r.getX(), r.getY() - 12, r.getWidth(), 12,
                   juce::Justification::left);
    };
    (void)drawSectionTitle;

    // THRESH label above threshold slider
    if (thresholdSlider_.isVisible())
    {
        g.setColour(Clr::textDim);
        g.setFont(juce::Font(9.5f));
        g.drawText("THRESH",
                   thresholdSlider_.getX(), thresholdSlider_.getY() - 12,
                   60, 12, juce::Justification::left);
    }

    // Labels above the 3 random knobs (SYNC button has its own text)
    g.setColour(Clr::textDim);
    g.setFont(juce::Font(9.5f));
    auto drawAbove = [&](juce::Component& c, const juce::String& t)
    {
        if (c.isVisible())
            g.drawText(t, c.getX(), c.getY() - 12, c.getWidth(), 12,
                       juce::Justification::centred);
    };
    drawAbove(randomDensityKnob_,  "DENS");
    drawAbove(randomGateTimeKnob_, "GATE");
    drawAbove(randomFreeTempoKnob_, "FREE BPM");

    g.setColour(Clr::accent);
    g.drawRect(getLocalBounds().reduced(4), 1);
}

// ─── Timer (UI refresh) ───────────────────────────────────────────────────────

void PluginEditor::timerCallback()
{
    // Refresh gates, joystick position, looper buttons
    padRoot_.repaint(); padThird_.repaint(); padFifth_.repaint(); padTension_.repaint();
    padAll_.repaint();
    joystickPad_.repaint();

    loopPlayBtn_ .setToggleState(proc_.looperIsPlaying(),   juce::dontSendNotification);
    loopRecBtn_  .setToggleState(proc_.looperIsRecording(), juce::dontSendNotification);
    loopDeleteBtn_.setEnabled(!proc_.looperIsPlaying());

    // Update REC JOY / REC GATES / SYNC toggle appearances
    loopRecJoyBtn_  .setToggleState(proc_.looperIsRecJoy(),    juce::dontSendNotification);
    loopRecGatesBtn_.setToggleState(proc_.looperIsRecGates(),  juce::dontSendNotification);
    loopSyncBtn_    .setToggleState(proc_.looperIsSyncToDaw(), juce::dontSendNotification);

    // Cap-reached visual indicator: flash the REC JOY / REC GATES button text when buffer is full
    static int capFlashCounter = 0;
    if (proc_.looperIsCapReached())
    {
        capFlashCounter = (capFlashCounter + 1) % 6;  // 30 Hz / 6 ≈ ~5 Hz flash
        const bool flashOn = (capFlashCounter < 3);
        loopRecJoyBtn_  .setButtonText(flashOn ? "CAP!" : "REC JOY");
        loopRecGatesBtn_.setButtonText(flashOn ? "CAP!" : "REC GATES");
    }
    else
    {
        loopRecJoyBtn_  .setButtonText("REC JOY");
        loopRecGatesBtn_.setButtonText("REC GATES");
        capFlashCounter = 0;
    }

    // Mirror gamepad right stick to joystick if active
    const float gpX = proc_.getGamepad().getPitchX();
    const float gpY = proc_.getGamepad().getPitchY();
    if ((std::abs(gpX) + std::abs(gpY)) > 0.05f)
    {
        proc_.joystickX.store(gpX);
        proc_.joystickY.store(gpY);
    }

    // Detect MIDI channel conflicts at 30 Hz
    {
        const int chs[4] = {
            (int)proc_.apvts.getRawParameterValue("voiceCh0")->load(),
            (int)proc_.apvts.getRawParameterValue("voiceCh1")->load(),
            (int)proc_.apvts.getRawParameterValue("voiceCh2")->load(),
            (int)proc_.apvts.getRawParameterValue("voiceCh3")->load()
        };
        bool conflict = false;
        for (int i = 0; i < 4 && !conflict; ++i)
            for (int j = i + 1; j < 4; ++j)
                if (chs[i] == chs[j]) { conflict = true; break; }

        if (conflict != channelConflictShown_)
        {
            channelConflictShown_ = conflict;
            channelConflictLabel_.setVisible(conflict);
        }
    }
}
