#include "PluginEditor.h"
#include "ScaleQuantizer.h"
#include <BinaryData.h>
#include <cmath>

// ─── Colours ─────────────────────────────────────────────────────────────────
namespace Clr
{
    static const juce::Colour bg        { 0xFF131525 };  // deep navy
    static const juce::Colour panel     { 0xFF1A1D35 };  // panel background
    static const juce::Colour accent    { 0xFF1E3A6E };  // mid-blue (borders, inactive arcs)
    static const juce::Colour highlight { 0xFFFF3D6E };  // bright pink-red (active arcs, pointer)
    static const juce::Colour text      { 0xFFF0F0F8 };  // near-white
    static const juce::Colour textDim   { 0xFF7A8AB0 };  // blue-grey labels
    static const juce::Colour gateOn    { 0xFF00E676 };  // bright green
    static const juce::Colour gateOff   { 0xFF252843 };  // dark inactive pads
}

// ─── PixelLookAndFeel implementation ─────────────────────────────────────────

void PixelLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& /*slider*/)
{
    const float cx = x + width  * 0.5f;
    const float cy = y + height * 0.5f;
    const float r  = juce::jmin(width, height) * 0.5f - 4.0f;

    // ── Thin track ring outside the knob body ────────────────────────────────
    const float trackR = r + 3.0f;
    const juce::PathStrokeType thinStroke(1.5f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded);
    {
        juce::Path trackArc;
        trackArc.addArc(cx - trackR, cy - trackR, trackR * 2.0f, trackR * 2.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xFF2A3050));
        g.strokePath(trackArc, thinStroke);
    }
    const float endAngle = rotaryStartAngle
        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    {
        juce::Path fillArc;
        fillArc.addArc(cx - trackR, cy - trackR, trackR * 2.0f, trackR * 2.0f,
                       rotaryStartAngle, endAngle, true);
        g.setColour(Clr::highlight.withAlpha(0.85f));
        g.strokePath(fillArc, thinStroke);
    }

    // ── Drop shadow ──────────────────────────────────────────────────────────
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - r + 1.5f, cy - r + 1.5f, r * 2.0f, r * 2.0f);

    // ── Knob body — radial gradient for 3D depth (Roland-style dark cap) ─────
    {
        juce::ColourGradient grad(
            juce::Colour(0xFF424550), cx - r * 0.35f, cy - r * 0.35f,
            juce::Colour(0xFF181A22), cx + r * 0.4f,  cy + r * 0.5f,
            false);
        grad.addColour(0.55, juce::Colour(0xFF252830));
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // ── Rim highlight (top-left bright edge) ─────────────────────────────────
    g.setColour(juce::Colour(0xFF606878));
    g.drawEllipse(cx - r + 0.5f, cy - r + 0.5f, (r - 0.5f) * 2.0f, (r - 0.5f) * 2.0f, 1.0f);
    // Bottom-right shadow rim
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawEllipse(cx - r + 1.2f, cy - r + 1.2f, (r - 1.2f) * 2.0f, (r - 1.2f) * 2.0f, 0.5f);

    // ── Indicator line — bold white tick (Roland-style) ──────────────────────
    const float innerR = r * 0.28f;   // starts near centre
    const float outerR = r - 4.0f;    // ends near rim
    const float px1 = cx + innerR * std::sin(endAngle);
    const float py1 = cy - innerR * std::cos(endAngle);
    const float px2 = cx + outerR * std::sin(endAngle);
    const float py2 = cy - outerR * std::cos(endAngle);
    // Shadow pass
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(px1 + 0.8f, py1 + 0.8f, px2 + 0.8f, py2 + 0.8f, 2.5f);
    // White indicator
    g.setColour(juce::Colours::white);
    g.drawLine(px1, py1, px2, py2, 2.5f);

    // ── Centre cap (small recessed dot) ──────────────────────────────────────
    g.setColour(juce::Colour(0xFF141620));
    g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
    g.setColour(juce::Colour(0xFF484C5C));
    g.drawEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f, 1.0f);
}

void PixelLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool isOn = button.getToggleState();

    juce::Colour fill = isOn ? Clr::accent.brighter(0.3f) : backgroundColour;
    if (shouldDrawButtonAsDown)      fill = fill.darker(0.25f);
    else if (shouldDrawButtonAsHighlighted) fill = fill.brighter(0.12f);

    if (button.getName() == "round")
    {
        // Draw as circle (for RND SYNC button)
        const float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4.0f;
        const auto eb = bounds.withSizeKeepingCentre(size, size);
        g.setColour(fill);
        g.fillEllipse(eb);
        g.setColour(isOn ? Clr::highlight : Clr::accent.brighter(0.5f));
        g.drawEllipse(eb.reduced(0.75f), 2.0f);
    }
    else if (button.getName() == "lrtoggle")
    {
        // Pill toggle: left option | right option, active side highlighted
        const juce::String text = button.getButtonText();
        const int sep = text.indexOf("|");
        const juce::String leftLabel  = sep >= 0 ? text.substring(0, sep).trim() : text;
        const juce::String rightLabel = sep >= 0 ? text.substring(sep + 1).trim() : "";
        const bool rightActive = button.getToggleState();

        constexpr float pr = 8.0f;

        // Dark pill background
        g.setColour(Clr::panel.brighter(0.12f));
        g.fillRoundedRectangle(bounds, pr);

        // Active half — clip the rounded rect to just that half for clean edges
        {
            const juce::Rectangle<float> activeHalf = rightActive
                ? bounds.withLeft(bounds.getCentreX())
                : bounds.withRight(bounds.getCentreX());
            juce::Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(activeHalf.toNearestInt());
            g.setColour(Clr::highlight.withAlpha(0.70f));
            g.fillRoundedRectangle(bounds, pr);
        }

        // Pill outline
        g.setColour(Clr::accent.brighter(0.5f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), pr, 1.0f);

        // Centre divider
        g.setColour(Clr::accent.brighter(0.4f));
        g.drawLine(bounds.getCentreX(), bounds.getY() + 3.0f,
                   bounds.getCentreX(), bounds.getBottom() - 3.0f, 1.0f);

        // Option text (drawn here; drawButtonText is suppressed for lrtoggle)
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));

        g.setColour(!rightActive ? Clr::text : Clr::textDim.withAlpha(0.55f));
        g.drawText(leftLabel,  bounds.withRight(bounds.getCentreX()), juce::Justification::centred, true);

        g.setColour(rightActive ? Clr::text : Clr::textDim.withAlpha(0.55f));
        g.drawText(rightLabel, bounds.withLeft(bounds.getCentreX()),  juce::Justification::centred, true);
    }
    else
    {
        constexpr float cr = 4.0f;
        g.setColour(fill);
        g.fillRoundedRectangle(bounds, cr);
        g.setColour(isOn ? Clr::highlight : Clr::accent.brighter(0.5f));
        g.drawRoundedRectangle(bounds.reduced(0.75f), cr, 1.5f);
    }
}

void PixelLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
    bool isHighlighted, bool isDown)
{
    // "lrtoggle" and "round" buttons draw their own content in drawButtonBackground
    if (button.getName() == "lrtoggle" || button.getName() == "round")
        return;
    LookAndFeel_V4::drawButtonText(g, button, isHighlighted, isDown);
}

void PixelLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    constexpr float cr = 3.0f;

    g.setColour(Clr::panel.brighter(0.1f));
    g.fillRoundedRectangle(bounds, cr);

    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cr, 1.0f);

    // Arrow (downward triangle)
    const float ax = width - 14.0f;
    const float ay = height * 0.5f - 3.0f;
    g.setColour(box.isEnabled() ? Clr::highlight : Clr::textDim);
    juce::Path arrow;
    arrow.addTriangle(ax, ay, ax + 8.0f, ay, ax + 4.0f, ay + 6.0f);
    g.fillPath(arrow);
}

void PixelLookAndFeel::drawLinearSlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
    juce::Slider::SliderStyle style, juce::Slider& /*slider*/)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        // Fall back for any vertical linear sliders (none expected but be safe)
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
            sliderPos, 0.0f, 0.0f, style, *reinterpret_cast<juce::Slider*>(nullptr));
        return;
    }

    const float trackY  = y + height * 0.5f - 3.0f;
    const float trackH  = 6.0f;

    // Background track
    g.setColour(Clr::gateOff);
    g.fillRect(juce::Rectangle<float>((float)x, trackY, (float)width, trackH));

    // Filled portion
    g.setColour(Clr::accent);
    g.fillRect(juce::Rectangle<float>((float)x, trackY, sliderPos - x, trackH));

    // Thumb (3px wide white rect)
    g.setColour(Clr::text);
    g.fillRect(juce::Rectangle<float>(sliderPos - 1.5f, (float)y, 3.0f, (float)height));
}

juce::Font PixelLookAndFeel::getLabelFont(juce::Label& label)
{
    // Use the label's own height/style but with a clean sans-serif typeface.
    auto f = label.getFont();
    return juce::Font(juce::Font::getDefaultSansSerifFontName(),
                      f.getHeight(), f.getStyleFlags());
}

juce::Font PixelLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::plain);
}

juce::Font PixelLookAndFeel::getTextButtonFont(juce::TextButton& /*button*/, int buttonHeight)
{
    return juce::Font(juce::Font::getDefaultSansSerifFontName(),
                      juce::jmin(11.0f, (float)buttonHeight * 0.5f),
                      juce::Font::bold);
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
    g.fillRect(b);

    // 12-step grid: 11 interior lines per axis (one per semitone at 1-octave range)
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    for (int i = 1; i < 12; ++i)
    {
        const float t = static_cast<float>(i) / 12.0f;
        g.drawLine(b.getX() + t * b.getWidth(),  b.getY(), b.getX() + t * b.getWidth(),  b.getBottom(), 1.0f);
        g.drawLine(b.getX(), b.getY() + t * b.getHeight(), b.getRight(), b.getY() + t * b.getHeight(), 1.0f);
    }
    // Centre cross brighter (root/unison reference)
    g.setColour(juce::Colours::white.withAlpha(0.38f));
    g.drawLine(b.getCentreX(), b.getY(),    b.getCentreX(), b.getBottom(), 1.5f);
    g.drawLine(b.getX(),       b.getCentreY(), b.getRight(), b.getCentreY(), 1.5f);

    // Cursor dot + crosshair ticks
    const float cx = (proc_.joystickX.load() + 1.0f) * 0.5f * b.getWidth()  + b.getX();
    const float cy = (1.0f - (proc_.joystickY.load() + 1.0f) * 0.5f) * b.getHeight() + b.getY();

    constexpr float dotR = 7.0f;
    constexpr float tickLen = 5.0f;
    // Outer glow
    g.setColour(Clr::highlight.withAlpha(0.25f));
    g.fillEllipse(cx - dotR - 3.0f, cy - dotR - 3.0f, (dotR + 3.0f) * 2.0f, (dotR + 3.0f) * 2.0f);
    // Dot fill
    g.setColour(Clr::highlight);
    g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
    // Dot outline
    g.setColour(Clr::text);
    g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.5f);
    // Crosshair ticks
    g.setColour(Clr::text.withAlpha(0.7f));
    g.drawLine(cx - dotR - tickLen, cy, cx - dotR - 1.0f, cy, 1.0f);
    g.drawLine(cx + dotR + 1.0f,   cy, cx + dotR + tickLen, cy, 1.0f);
    g.drawLine(cx, cy - dotR - tickLen, cx, cy - dotR - 1.0f, 1.0f);
    g.drawLine(cx, cy + dotR + 1.0f,   cx, cy + dotR + tickLen, 1.0f);

    // Border
    g.setColour(Clr::highlight.withAlpha(0.5f));
    g.drawRect(b.reduced(1), 2.0f);
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
        // Hold mode is inverted: press = mute (note-off), release = note-on (hold resumes).
        if (proc_.padHold_[voice_].load())
            proc_.setPadState(voice_, false);
        else
            proc_.setPadState(voice_, true);
        repaint();
    }
}

void TouchPlate::mouseUp(const juce::MouseEvent&)
{
    const int src = static_cast<int>(
        proc_.apvts.getRawParameterValue("triggerSource" + juce::String(voice_))->load());
    if (src == 0)
    {
        // Hold mode: release = note-on (hold resumes). Normal: release = note-off.
        if (proc_.padHold_[voice_].load())
            proc_.setPadState(voice_, true);
        else
            proc_.setPadState(voice_, false);
    }
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

    constexpr float padR = 4.0f;
    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), padR);

    g.setColour(isPadMode ? Clr::text : Clr::textDim);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);

    g.setColour((active ? Clr::gateOn : Clr::accent).brighter(isPadMode ? 0.35f : 0.0f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), padR, 1.5f);
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

    constexpr float allR = 4.0f;
    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), allR);

    g.setColour(anyPadMode ? Clr::text : Clr::textDim);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText("ALL", getLocalBounds(), juce::Justification::centred);

    const juce::Colour borderClr = pressed_ ? allGateOn.brighter(0.3f)
                                             : (anyPadMode ? allGateOff.brighter(0.5f)
                                                           : Clr::accent);
    g.setColour(borderClr);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), allR, 1.5f);
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
    for (int n = 0; n < 12; ++n)
    {
        if (!noteRect(n).contains(e.position))
            continue;

        // If in preset mode, copy the current preset (pre-transpose) into
        // scaleNote0..11 before switching to edit mode, so the first click
        // feels like "edit this preset" rather than "start from blank".
        const bool useCustom = apvts_.getRawParameterValue("useCustomScale")->load() > 0.5f;
        if (!useCustom)
        {
            const int presetIdx = static_cast<int>(
                apvts_.getRawParameterValue("scalePreset")->load());
            const ScalePreset preset = static_cast<ScalePreset>(
                juce::jlimit(0, (int)ScalePreset::COUNT - 1, presetIdx));
            const int* pattern = ScaleQuantizer::getScalePattern(preset);
            const int  size    = ScaleQuantizer::getScaleSize(preset);
            bool pitchClasses[12] = {};
            for (int i = 0; i < size; ++i)
                pitchClasses[pattern[i]] = true;
            for (int i = 0; i < 12; ++i)
            {
                if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                        apvts_.getParameter("scaleNote" + juce::String(i))))
                    *p = pitchClasses[i];
            }
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                    apvts_.getParameter("useCustomScale")))
                *p = true;
        }

        // Toggle the clicked note
        const bool cur = isNoteActive(n);
        if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                apvts_.getParameter("scaleNote" + juce::String(n))))
            *p = !cur;
        repaint();
        return;
    }
}

void ScaleKeyboard::paint(juce::Graphics& g)
{
    // Colours for scale-highlight states:
    //   custom-toggle active (editing mode)  -> red highlight (editing)
    //   scale preset active note             -> red highlight / darker red
    //   normal inactive                      -> white / black like piano keys

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
            fill = inScale ? Clr::highlight : juce::Colours::white;
        }

        g.setColour(fill);
        g.fillRoundedRectangle(r.reduced(0.5f), 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(r.reduced(0.5f), 2.0f, 0.8f);
    }
    // Black keys on top
    for (int n = 0; n < 12; ++n)
    {
        if (!kIsBlack[n]) continue;
        auto r = noteRect(n);

        juce::Colour fill;
        if (editingCustom)
        {
            fill = isNoteActive(n) ? Clr::highlight : juce::Colour(0xFF1A1D2E);
        }
        else
        {
            const bool inScale = (activeScaleMask_ >> n) & 1;
            fill = inScale ? Clr::highlight.darker(0.25f) : juce::Colour(0xFF1A1D2E);
        }

        g.setColour(fill);
        g.fillRoundedRectangle(r, 2.0f);
    }

    // Borders on black keys
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    for (int n = 0; n < 12; ++n)
    {
        if (!kIsBlack[n]) continue;
        g.drawRoundedRectangle(noteRect(n), 2.0f, 0.5f);
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
    l.setFont(juce::Font(11.0f));   // height passed through to getLabelFont
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
    // ── Pixel font + LookAndFeel ───────────────────────────────────────────────
    pixelFont_ = juce::Font(juce::Typeface::createSystemTypefaceFor(
        BinaryData::PressStart2PRegular_ttf,
        BinaryData::PressStart2PRegular_ttfSize)).withHeight(10.0f);
    pixelLaf_.setPixelFont(pixelFont_);
    setLookAndFeel(&pixelLaf_);

    setSize(920, 760);

    // ── Joystick ──────────────────────────────────────────────────────────────
    addAndMakeVisible(joystickPad_);
    styleKnob(joyXAttenKnob_); joyXAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyXAttenLabel_, "X Range");
    styleKnob(joyYAttenKnob_); joyYAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyYAttenLabel_, "Y Range");
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
    randomDensityKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    randomDensityKnob_.setTooltip("Random density (hits per bar, 1-8)");
    randomDensityKnob_.setColour(juce::Slider::rotarySliderFillColourId,   Clr::highlight);
    randomDensityKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomDensityKnob_.setColour(juce::Slider::thumbColourId,              Clr::text);
    randomDensityKnob_.setColour(juce::Slider::textBoxTextColourId,        Clr::textDim);
    randomDensityKnob_.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
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
    randomGateTimeKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    randomGateTimeKnob_.setTooltip("Random gate time (fraction of subdivision)");
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomGateTimeKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    randomGateTimeKnob_.setColour(juce::Slider::textBoxTextColourId,       Clr::textDim);
    randomGateTimeKnob_.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible(randomGateTimeKnob_);
    randomGateTimeKnobAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomGateTime"), randomGateTimeKnob_);

    // Sync toggle (round button — drawn as circle, same size as a knob)
    randomSyncButton_.setButtonText("RND SYNC");
    randomSyncButton_.setName("round");
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
    styleKnob(filterXAttenKnob_); styleLabel(filterXAttenLabel_, "Cutoff Atten");
    styleKnob(filterYAttenKnob_); styleLabel(filterYAttenLabel_, "Res Atten");
    addAndMakeVisible(filterXAttenKnob_); addAndMakeVisible(filterXAttenLabel_);
    addAndMakeVisible(filterYAttenKnob_); addAndMakeVisible(filterYAttenLabel_);
    filterXAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXAtten", filterXAttenKnob_);
    filterYAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYAtten", filterYAttenKnob_);

    // ── Joystick threshold slider ─────────────────────────────────────────────
    thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider_.setTooltip("Joystick motion threshold — higher = needs bigger movement to trigger");
    thresholdSlider_.setColour(juce::Slider::trackColourId,      Clr::highlight);
    thresholdSlider_.setColour(juce::Slider::backgroundColourId, Clr::accent);
    thresholdSlider_.setColour(juce::Slider::thumbColourId,      Clr::text);
    addAndMakeVisible(thresholdSlider_);
    thresholdSliderAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("joystickThreshold"), thresholdSlider_);

    // ── Joystick gate time slider ─────────────────────────────────────────────
    gateTimeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    gateTimeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gateTimeSlider_.setTooltip("Joystick gate time — seconds of stillness before note-off (0.05 - 5s)");
    gateTimeSlider_.setColour(juce::Slider::trackColourId,      Clr::highlight);
    gateTimeSlider_.setColour(juce::Slider::backgroundColourId, Clr::accent);
    gateTimeSlider_.setColour(juce::Slider::thumbColourId,      Clr::text);
    addAndMakeVisible(gateTimeSlider_);
    gateTimeSliderAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("joystickGateTime"), gateTimeSlider_);

    // ── Looper ────────────────────────────────────────────────────────────────
    loopPlayBtn_.setButtonText("PLAY");  loopPlayBtn_.setClickingTogglesState(true);
    loopRecBtn_.setButtonText("REC");    loopRecBtn_.setClickingTogglesState(true);
    loopResetBtn_.setButtonText("RST");   loopResetBtn_.setTooltip("Reset loop — gamepad: Square (□)");
    loopDeleteBtn_.setButtonText("DEL");  loopDeleteBtn_.setTooltip("Delete loop — gamepad: Circle (○)");

    styleButton(loopPlayBtn_);  styleButton(loopRecBtn_);
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
    loopSyncBtn_.setButtonText("DAW SYNC");

    loopRecJoyBtn_.setClickingTogglesState(true);
    loopRecGatesBtn_.setClickingTogglesState(true);
    loopSyncBtn_.setClickingTogglesState(true);

    styleButton(loopRecJoyBtn_);
    styleButton(loopRecGatesBtn_);
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

    loopRecWaitBtn_.setButtonText("REC TOUCH");
    loopRecWaitBtn_.setClickingTogglesState(true);
    styleButton(loopRecWaitBtn_);
    loopRecWaitBtn_.onClick = [this] {
        const bool newVal = !proc_.looperIsRecWaitForTrigger();
        proc_.looperSetRecWaitForTrigger(newVal);
        loopRecWaitBtn_.setToggleState(newVal, juce::dontSendNotification);
    };
    addAndMakeVisible(loopRecWaitBtn_);

    bpmDisplayLabel_.setText("120.0 BPM", juce::dontSendNotification);
    bpmDisplayLabel_.setJustificationType(juce::Justification::centred);
    bpmDisplayLabel_.setFont(juce::Font(11.0f));
    bpmDisplayLabel_.setColour(juce::Label::textColourId, Clr::text.withAlpha(0.75f));
    addAndMakeVisible(bpmDisplayLabel_);

    loopSubdivBox_.addItem("3/4", 1); loopSubdivBox_.addItem("4/4", 2);
    loopSubdivBox_.addItem("5/4", 3); loopSubdivBox_.addItem("7/8", 4);
    loopSubdivBox_.addItem("9/8", 5); loopSubdivBox_.addItem("11/8", 6);
    styleCombo(loopSubdivBox_);
    styleLabel(loopSubdivLabel_, "Time Sig");
    addAndMakeVisible(loopSubdivBox_); addAndMakeVisible(loopSubdivLabel_);
    loopSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "looperSubdiv", loopSubdivBox_);

    loopLengthKnob_.setSliderStyle(juce::Slider::LinearHorizontal);
    loopLengthKnob_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 28, 18);
    loopLengthKnob_.setColour(juce::Slider::trackColourId,          Clr::highlight);
    loopLengthKnob_.setColour(juce::Slider::backgroundColourId,     Clr::accent);
    loopLengthKnob_.setColour(juce::Slider::thumbColourId,          Clr::text);
    loopLengthKnob_.setColour(juce::Slider::textBoxTextColourId,    Clr::textDim);
    loopLengthKnob_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    styleLabel(loopLengthLabel_, "Bars");
    addAndMakeVisible(loopLengthKnob_); addAndMakeVisible(loopLengthLabel_);
    loopLengthAtt_ = std::make_unique<SliderAtt>(p.apvts, "looperLength", loopLengthKnob_);

    // Gamepad status
    gamepadStatusLabel_.setText("GAMEPAD: none", juce::dontSendNotification);
    gamepadStatusLabel_.setFont(juce::Font(10.0f));
    gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(gamepadStatusLabel_);

    // Update label text on hot-plug events (fires on message thread via callAsync).
    // Exact strings are locked decisions: "GAMEPAD: connected" / "GAMEPAD: none".
    // Uses onConnectionChangeUI — the slot reserved for the editor.
    // Do NOT assign to onConnectionChange here — that slot is owned by PluginProcessor
    // (set in PluginProcessor ctor) and fires pendingAllNotesOff_ / pendingCcReset_.
    // Overwriting it would silently break stuck-note protection on controller unplug.
    p.getGamepad().onConnectionChangeUI = [this](bool connected)
    {
        juce::MessageManager::callAsync([this, connected]
        {
            gamepadStatusLabel_.setText(
                connected ? "GAMEPAD: connected" : "GAMEPAD: none",
                juce::dontSendNotification);
            // Sync toggle button visual state on disconnect (gamepadActive_ stays true,
            // but the button should reflect that there's no controller).
            // No state change needed — button stays as-is; label communicates status.
        });
    };

    // Per-instance GAMEPAD ACTIVE toggle button.
    // Default ON — this instance responds to the controller.
    // Toggle OFF to silence this instance while another ChordJoystick instance
    // in the same DAW session takes over the controller.
    gamepadActiveBtn_.setButtonText("GAMEPAD ON");
    gamepadActiveBtn_.setClickingTogglesState(true);
    gamepadActiveBtn_.setToggleState(true, juce::dontSendNotification);
    gamepadActiveBtn_.setColour(juce::TextButton::buttonOnColourId,  Clr::gateOn);
    gamepadActiveBtn_.setColour(juce::TextButton::buttonColourId,    Clr::gateOff);
    gamepadActiveBtn_.onClick = [this]
    {
        const bool active = gamepadActiveBtn_.getToggleState();
        proc_.setGamepadActive(active);
        gamepadActiveBtn_.setButtonText(active ? "GAMEPAD ON" : "GAMEPAD OFF");
    };
    addAndMakeVisible(gamepadActiveBtn_);

    // Sync label to current connection state on editor open.
    if (p.getGamepad().isConnected())
        gamepadStatusLabel_.setText("GAMEPAD: connected", juce::dontSendNotification);

    // Gamepad left-stick axis mode toggles — pill style: left=default, right=alt
    filterYModeBox_.addItem("Resonance (CC71)", 1);
    filterYModeBox_.addItem("LFO Rate  (CC76)", 2);
    filterYModeBox_.setTooltip("Left stick Y axis: CC71 Resonance or CC76 LFO Rate (scale with Res Atten knob)");
    styleCombo(filterYModeBox_);
    addAndMakeVisible(filterYModeBox_);
    filterYModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterYMode", filterYModeBox_);

    filterXModeBox_.addItem("Cutoff    (CC74)", 1);
    filterXModeBox_.addItem("VCF LFO   (CC12)", 2);
    filterXModeBox_.addItem("Mod Wheel (CC1)",  3);
    filterXModeBox_.setTooltip("Left stick X axis: what CC the X axis controls");
    styleCombo(filterXModeBox_);
    addAndMakeVisible(filterXModeBox_);
    filterXModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterXMode", filterXModeBox_);

    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

// ─── Layout ───────────────────────────────────────────────────────────────────

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(28);   // header bar
    area.removeFromBottom(60); // footer instructions
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

    // Attenuator knobs: one per pad column — X Range / Y Range / Cut / Res
    {
        auto row = right.removeFromTop(90);
        const int pw = (row.getWidth() - 9) / 4;
        juce::Slider* knobs[4]  = { &joyXAttenKnob_,   &joyYAttenKnob_,
                                    &filterXAttenKnob_, &filterYAttenKnob_ };
        juce::Label*  lbls[4]   = { &joyXAttenLabel_,  &joyYAttenLabel_,
                                    &filterXAttenLabel_, &filterYAttenLabel_ };
        for (int v = 0; v < 4; ++v)
        {
            auto col = row.removeFromLeft(pw);
            lbls[v]->setBounds(col.removeFromTop(14));
            knobs[v]->setBounds(col);
            if (v < 3) row.removeFromLeft(3);
        }
    }

    right.removeFromTop(4);

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

    right.removeFromTop(6);

    // Gamepad status row: [GAMEPAD ON/OFF] button + status label side by side
    {
        auto row = right.removeFromTop(20);
        gamepadActiveBtn_.setBounds(row.removeFromLeft(90));
        row.removeFromLeft(4);  // gap
        gamepadStatusLabel_.setBounds(row);
    }

    // Gamepad left-stick axis mode combos (label drawn above via drawAbove in paint())
    right.removeFromTop(12);
    filterYModeBox_.setBounds(right.removeFromTop(22));
    right.removeFromTop(12);
    filterXModeBox_.setBounds(right.removeFromTop(22));

    // Joystick gate time + threshold sliders (labels drawn via drawAbove in paint())
    right.removeFromTop(14);
    gateTimeSlider_  .setBounds(right.removeFromTop(18));
    right.removeFromTop(10);
    thresholdSlider_ .setBounds(right.removeFromTop(18));

    // ── LEFT COLUMN ───────────────────────────────────────────────────────────

    // Scale section
    {
        auto section = left.removeFromTop(120);
        scalePresetLabel_.setBounds(section.removeFromTop(16));
        scalePresetBox_  .setBounds(section.removeFromTop(24));
        scaleKeys_       .setBounds(section.removeFromTop(80));
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

        // Random controls row — [DENS] | [GATE] | [RND SYNC (round)] | [FREE BPM knob]
        {
            auto rndRow = left.removeFromTop(60);
            randomDensityKnob_ .setBounds(rndRow.removeFromLeft(cw));
            randomGateTimeKnob_.setBounds(rndRow.removeFromLeft(cw));
            randomSyncButton_  .setBounds(rndRow.removeFromLeft(cw));  // round button, 3rd col
            randomFreeTempoKnob_.setBounds(rndRow);                    // FREE BPM, 4th col
        }

        // BPM display: under FREE BPM knob (4th column)
        {
            auto bpmRow = left.removeFromTop(16);
            bpmRow.removeFromLeft(cw * 3);          // skip DENS + GATE + SYNC columns
            bpmDisplayLabel_.setBounds(bpmRow);
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

        // Buttons row 2: REC GATES / REC JOY / REC TOUCH / DAW SYNC (four columns)
        {
            auto row2 = section.removeFromTop(28);
            const int bw4 = (row2.getWidth() - 6) / 4;
            loopRecGatesBtn_.setBounds(row2.removeFromLeft(bw4)); row2.removeFromLeft(2);
            loopRecJoyBtn_  .setBounds(row2.removeFromLeft(bw4)); row2.removeFromLeft(2);
            loopRecWaitBtn_ .setBounds(row2.removeFromLeft(bw4)); row2.removeFromLeft(2);
            loopSyncBtn_    .setBounds(row2);
        }

        section.removeFromTop(4);

        // Subdiv + length: narrow combo for time sig, slider for bars
        {
            auto ctrlRow = section.removeFromTop(46);
            const int subdivW = 58;  // enough for "11/8" + arrow

            auto col1 = ctrlRow.removeFromLeft(subdivW);
            loopSubdivLabel_.setBounds(col1.removeFromTop(14));
            loopSubdivBox_  .setBounds(col1.removeFromTop(22));

            ctrlRow.removeFromLeft(6);
            loopLengthLabel_.setBounds(ctrlRow.removeFromTop(14));
            loopLengthKnob_.setBounds(ctrlRow.removeFromTop(22));
        }
    }
}

// ─── Paint ────────────────────────────────────────────────────────────────────

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Clr::bg);

    // Header bar
    auto header = getLocalBounds().removeFromTop(28);
    // Subtle gradient from panel to slightly brighter
    g.setGradientFill(juce::ColourGradient(
        Clr::panel.brighter(0.08f), (float)header.getX(), (float)header.getY(),
        Clr::panel,                 (float)header.getX(), (float)header.getBottom(),
        false));
    g.fillRect(header);
    g.setColour(Clr::accent.brighter(0.6f));
    g.drawRect(header, 1);
    // Title: pixel font for branding, but bright/readable
    g.setFont(pixelFont_.withHeight(11.0f));
    g.setColour(Clr::text);
    g.drawText("CHORD JOYSTICK MK2", header, juce::Justification::centred);

    // Outer border
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRect(getLocalBounds().reduced(2), 1);

    // Subtle vertical divider between left and right columns
    if (joystickPad_.isVisible())
    {
        const int divX = joystickPad_.getX() - 4;
        g.setColour(Clr::accent.withAlpha(0.5f));
        g.drawLine((float)divX, 32.0f, (float)divX, (float)getHeight() - 8, 1.0f);
    }

    // Section labels
    g.setColour(Clr::textDim);
    g.setFont(juce::Font(9.5f, juce::Font::bold));

    auto drawSectionTitle = [&](const juce::String& t, juce::Rectangle<int> r)
    {
        g.drawText(t, r.getX(), r.getY() - 12, r.getWidth(), 12,
                   juce::Justification::left);
    };
    (void)drawSectionTitle;

    // Labels above controls — uses drawAbove helper (draws 12px above the component)
    g.setColour(Clr::textDim.brighter(0.2f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
    auto drawAbove = [&](juce::Component& c, const juce::String& t)
    {
        if (c.isVisible())
            g.drawText(t, c.getX(), c.getY() - 12, c.getWidth(), 12,
                       juce::Justification::centred);
    };
    drawAbove(randomDensityKnob_,   "RND DENS");
    drawAbove(randomGateTimeKnob_,  "RND GATE");
    drawAbove(randomSyncButton_,    "RND SYNC");
    drawAbove(randomFreeTempoKnob_, "FREE BPM");
    drawAbove(filterYModeBox_,      "LEFT Y");
    drawAbove(filterXModeBox_,      "LEFT X");
    drawAbove(gateTimeSlider_,      "JOYSTICK GATE LENGTH");
    drawAbove(thresholdSlider_,     "JOY THRESH");

    // ── Footer: how-to-use instructions (left column only) ───────────────────
    {
        // Constrain to left column — same right edge as the column divider
        const int footerRight = joystickPad_.isVisible() ? (joystickPad_.getX() - 4) : getWidth();
        auto footer = juce::Rectangle<int>(0, getHeight() - 60, footerRight, 60).reduced(8, 0);

        // Separator line
        g.setColour(Clr::accent.withAlpha(0.5f));
        g.drawLine((float)footer.getX(), (float)footer.getY() + 1.0f,
                   (float)footer.getRight(), (float)footer.getY() + 1.0f, 1.0f);

        footer.removeFromTop(5);

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
        g.setColour(Clr::textDim.withAlpha(0.8f));

        // 3 rows × 2 columns — each column is half the footer width so long text fits
        auto drawRow = [&](const juce::String& left, const juce::String& right)
        {
            auto row = footer.removeFromTop(14);
            const int w = row.getWidth() / 2;
            g.drawFittedText(left,  row.getX(),     row.getY(), w, row.getHeight(),
                             juce::Justification::centredLeft, 1, 0.75f);
            g.drawFittedText(right, row.getX() + w, row.getY(), w, row.getHeight(),
                             juce::Justification::centredLeft, 1, 0.75f);
            footer.removeFromTop(3);
        };

        drawRow("1. Plugin in MIDI Track",                              "2. Synth in separate Track");
        drawRow("3. Set MIDI In of Synth Track to Plugin MIDI Track",   "4. Arm Rec on Synth Track");
        drawRow("5. Set Synth Env. Sustain high",                       "6. Have fun!");
    }

}

// ─── Timer (UI refresh) ───────────────────────────────────────────────────────

void PluginEditor::timerCallback()
{
    // Refresh gates, joystick position, looper buttons
    padRoot_.repaint(); padThird_.repaint(); padFifth_.repaint(); padTension_.repaint();
    padAll_.repaint();
    joystickPad_.repaint();

    loopPlayBtn_ .setToggleState(proc_.looperIsPlaying(),   juce::dontSendNotification);
    loopRecBtn_  .setToggleState(proc_.looperIsRecordArmed(), juce::dontSendNotification);
    loopDeleteBtn_.setEnabled(!proc_.looperIsPlaying());

    // ── Gamepad button flash: RST (Square) and DEL (Circle) ──────────────────
    if (proc_.flashLoopReset_.exchange(0, std::memory_order_relaxed) > 0)
        resetFlashCounter_ = 5;   // ~167ms at 30Hz
    if (proc_.flashLoopDelete_.exchange(0, std::memory_order_relaxed) > 0)
        deleteFlashCounter_ = 5;

    if (resetFlashCounter_ > 0)
    {
        --resetFlashCounter_;
        loopResetBtn_.setColour(juce::TextButton::buttonColourId,  Clr::highlight);
        loopResetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        loopResetBtn_.setColour(juce::TextButton::buttonColourId,  Clr::accent);
        loopResetBtn_.setColour(juce::TextButton::textColourOffId, Clr::text);
    }

    if (deleteFlashCounter_ > 0)
    {
        --deleteFlashCounter_;
        loopDeleteBtn_.setColour(juce::TextButton::buttonColourId,  Clr::highlight);
        loopDeleteBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        loopDeleteBtn_.setColour(juce::TextButton::buttonColourId,  Clr::accent);
        loopDeleteBtn_.setColour(juce::TextButton::textColourOffId, Clr::text);
    }

    // Update REC JOY / REC GATES / SYNC / REC TOUCH toggle appearances
    loopRecJoyBtn_  .setToggleState(proc_.looperIsRecJoy(),             juce::dontSendNotification);
    loopRecGatesBtn_.setToggleState(proc_.looperIsRecGates(),           juce::dontSendNotification);
    loopSyncBtn_    .setToggleState(proc_.looperIsSyncToDaw(),          juce::dontSendNotification);
    loopRecWaitBtn_ .setToggleState(proc_.looperIsRecWaitForTrigger(),  juce::dontSendNotification);

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

    // Update BPM display
    {
        const float bpm = proc_.getEffectiveBpm();
        bpmDisplayLabel_.setText(juce::String(bpm, 1) + " BPM", juce::dontSendNotification);
    }

    // Update DAW-visible filter CC display parameters from live audio-thread values
    {
        auto* pCut = proc_.apvts.getParameter("filterCutLive");
        auto* pRes = proc_.apvts.getParameter("filterResLive");
        if (pCut && pRes)
        {
            pCut->setValueNotifyingHost(pCut->convertTo0to1(proc_.filterCutDisplay_.load(std::memory_order_relaxed)));
            pRes->setValueNotifyingHost(pRes->convertTo0to1(proc_.filterResDisplay_.load(std::memory_order_relaxed)));
        }
    }

    // Apply pending BPM delta from D-pad (audio thread sets, message thread applies)
    {
        const int bpmDelta = proc_.pendingBpmDelta_.exchange(0, std::memory_order_relaxed);
        if (bpmDelta != 0)
        {
            auto* p = proc_.apvts.getParameter("randomFreeTempo");
            const float cur = proc_.apvts.getRawParameterValue("randomFreeTempo")->load();
            const float newVal = juce::jlimit(30.0f, 240.0f, cur + static_cast<float>(bpmDelta));
            p->setValueNotifyingHost(p->convertTo0to1(newVal));
        }
    }

    // Sync looper recording buttons (gamepad D-pad may have toggled them from audio thread)
    loopRecGatesBtn_.setToggleState(proc_.looperIsRecGates(), juce::dontSendNotification);
    loopRecJoyBtn_  .setToggleState(proc_.looperIsRecJoy(),   juce::dontSendNotification);

    // Mirror gamepad right stick to joystick if active
    const float gpX = proc_.getGamepad().getPitchX();
    const float gpY = proc_.getGamepad().getPitchY();
    if ((std::abs(gpX) + std::abs(gpY)) > 0.05f)
    {
        proc_.joystickX.store(gpX);
        proc_.joystickY.store(gpY);
    }

}
