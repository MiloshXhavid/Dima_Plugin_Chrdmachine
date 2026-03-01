#include "PluginEditor.h"
#include "ScaleQuantizer.h"
#include "ChordNameHelper.h"
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

// ─── Interval snap helper ────────────────────────────────────────────────────
// Returns the nearest in-scale semitone to 'iv' (0..12), with downward preference.
// mask = ScaleKeyboard::getActiveScaleMask() (bit N = pitch-class N is in scale, pre-transposed).
// transpose = globalTranspose parameter value (0..11).

static int snapIntervalToScale(int iv, uint16_t mask, int transpose)
{
    iv = juce::jlimit(0, 12, iv);
    auto valid = [&](int x) -> bool
    {
        if (x < 0 || x > 12) return false;
        return ((mask >> ((transpose + x + 1200) % 12)) & 1) != 0;
    };
    if (valid(iv)) return iv;
    for (int r = 1; r <= 12; ++r)
    {
        if (valid(iv - r)) return iv - r;  // downward preference
        if (valid(iv + r)) return iv + r;
    }
    return iv;
}

// ─── Chord name helper ───────────────────────────────────────────────────────
// Thin JUCE wrapper around computeChordNameStr (see ChordNameHelper.h).

static juce::String computeChordName(const int pitches[4], int transposePc)
{
    return juce::String(computeChordNameStr(pitches, transposePc));
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
        // Draw as circle (for RND SYNC button) — smaller than a knob, like the knob body
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

juce::Font PixelLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    if (button.getName() == "small")
        return juce::Font(juce::Font::getDefaultSansSerifFontName(),
                          juce::jmin(8.0f, (float)buttonHeight * 0.38f),
                          juce::Font::bold);
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
    float nx = juce::jlimit(-1.0f, 1.0f, (e.x / w) * 2.0f - 1.0f);
    float ny = juce::jlimit(-1.0f, 1.0f, 1.0f - (e.y / h) * 2.0f);  // Y flipped

    // Center dead zone — snaps to 0 when within ±5% of centre
    constexpr float kSnap = 0.05f;
    if (std::abs(nx) < kSnap) nx = 0.0f;
    if (std::abs(ny) < kSnap) ny = 0.0f;

    proc_.joystickX.store(nx);
    proc_.joystickY.store(ny);
    repaint();
}

void JoystickPad::mouseDown       (const juce::MouseEvent& e) { updateFromMouse(e); }
void JoystickPad::mouseDrag       (const juce::MouseEvent& e) { updateFromMouse(e); }
void JoystickPad::mouseUp         (const juce::MouseEvent& e) { (void)e; }
void JoystickPad::mouseDoubleClick(const juce::MouseEvent& /*e*/)
{
    // Double-click resets joystick to exact centre (0, 0)
    proc_.joystickX.store(0.0f);
    proc_.joystickY.store(0.0f);
    repaint();
}

void JoystickPad::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Background
    g.setColour(Clr::accent);
    g.fillRect(b);

    // Circle inscribed in the pad (square) — shows the physical joystick reach area
    const float circleR = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const juce::Rectangle<float> circleRect(
        b.getCentreX() - circleR, b.getCentreY() - circleR,
        circleR * 2.0f, circleR * 2.0f);

    // Grid: N areas → N-1 equal-spaced lines (i/N for i=1..N-1).
    // N=0/1/2: only the centre cross is shown (see below).
    // N>=3: draw N-1 grid lines; centre cross suppressed so no phantom extra division.
    {
        const int xN = juce::jmax(0, (int)proc_.apvts.getRawParameterValue("joystickXAtten")->load());
        const int yN = juce::jmax(0, (int)proc_.apvts.getRawParameterValue("joystickYAtten")->load());

        g.setColour(juce::Colours::white.withAlpha(0.20f));
        for (int i = 1; xN >= 3 && i < xN; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(xN);
            g.drawLine(b.getX() + t * b.getWidth(), b.getY(),
                       b.getX() + t * b.getWidth(), b.getBottom(), 1.0f);
        }
        for (int i = 1; yN >= 3 && i < yN; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(yN);
            g.drawLine(b.getX(), b.getY() + t * b.getHeight(),
                       b.getRight(), b.getY() + t * b.getHeight(), 1.0f);
        }

        // Centre cross: reference for N<=2 only. For N>=3 the grid itself structures the pad.
        g.setColour(juce::Colours::white.withAlpha(0.38f));
        if (xN < 3)
            g.drawLine(b.getCentreX(), b.getY(), b.getCentreX(), b.getBottom(), 1.5f);
        if (yN < 3)
            g.drawLine(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY(), 1.5f);
    }

    // When gamepad is active, dim the corners that the physical stick can't reach
    if (proc_.isGamepadActive())
    {
        juce::Path cornerMask;
        cornerMask.addRectangle(b);
        cornerMask.addEllipse(circleRect);
        cornerMask.setUsingNonZeroWinding(false);  // ellipse punches a hole in the rect
        g.setColour(Clr::bg.withAlpha(0.60f));
        g.fillPath(cornerMask);
    }

    // Circle outline — always visible, shows joystick reach boundary
    g.setColour(Clr::accent.brighter(0.9f));
    g.drawEllipse(circleRect, 1.5f);

    // Cursor dot + crosshair ticks
    // Use LFO-modulated position whenever either LFO is enabled — at level=0 the ramp
    // holds lfoXRampOut_=0, so modulatedJoyX_ == joystickX anyway.  Checking only
    // "enabled" (not level > 0) prevents a hard display-mode switch as the level slider
    // first crosses zero, which was causing the cursor to jump.
    const bool lfoXActive = *proc_.apvts.getRawParameterValue("lfoXEnabled") > 0.5f;
    const bool lfoYActive = *proc_.apvts.getRawParameterValue("lfoYEnabled") > 0.5f;
    const float displayX = (lfoXActive || lfoYActive)
        ? proc_.modulatedJoyX_.load(std::memory_order_relaxed)
        : proc_.joystickX.load(std::memory_order_relaxed);
    const float displayY = (lfoXActive || lfoYActive)
        ? proc_.modulatedJoyY_.load(std::memory_order_relaxed)
        : proc_.joystickY.load(std::memory_order_relaxed);

    constexpr float dotR    = 7.0f;
    constexpr float tickLen = 5.0f;
    // Border inner edge is 2px from the component boundary (drawRect b.reduced(1), 2px thick).
    // Adding this margin ensures the dot edge just touches the border at ±1.0 display value
    // and never visually crosses it — even when unclipped LFO value exceeds ±1.
    constexpr float brdr    = 2.0f;

    const float cx = juce::jlimit(b.getX() + dotR + brdr, b.getRight()  - dotR - brdr,
                                  (displayX + 1.0f) * 0.5f * b.getWidth()  + b.getX());
    const float cy = juce::jlimit(b.getY() + dotR + brdr, b.getBottom() - dotR - brdr,
                                  (1.0f - (displayY + 1.0f) * 0.5f) * b.getHeight() + b.getY());

    // Static centre reference — same shape as cursor, always visible
    {
        const float ox = b.getCentreX(), oy = b.getCentreY();
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawEllipse(ox - dotR, oy - dotR, dotR * 2.0f, dotR * 2.0f, 1.5f);
        g.drawLine(ox - dotR - tickLen, oy, ox - dotR - 1.0f, oy, 1.0f);
        g.drawLine(ox + dotR + 1.0f,   oy, ox + dotR + tickLen, oy, 1.0f);
        g.drawLine(ox, oy - dotR - tickLen, ox, oy - dotR - 1.0f, 1.0f);
        g.drawLine(ox, oy + dotR + 1.0f,   ox, oy + dotR + tickLen, 1.0f);
    }

    g.setColour(Clr::highlight.withAlpha(0.25f));
    g.fillEllipse(cx - dotR - 3.0f, cy - dotR - 3.0f, (dotR + 3.0f) * 2.0f, (dotR + 3.0f) * 2.0f);
    g.setColour(Clr::highlight);
    g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour(Clr::text);
    g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.5f);
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
    const bool arpStep   = (proc_.getArpCurrentVoice() == voice_);
    const bool looperOn  = proc_.isLooperVoiceActive(voice_);

    juce::Colour fillClr;
    if (active)
        fillClr = Clr::gateOn;                          // live pad held: bright green
    else if (looperOn)
        fillClr = Clr::gateOn;                          // looper playback: bright green
    else if (arpStep)
        fillClr = juce::Colour(0xFF00C8A8);             // arp step: cyan/teal
    else if (isPadMode)
        fillClr = Clr::gateOff;
    else
        fillClr = Clr::gateOff.darker(0.3f);            // dimmed in JOY or RND mode

    constexpr float padR = 4.0f;
    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), padR);

    g.setColour(isPadMode ? Clr::text : Clr::textDim);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);

    juce::Colour borderClr;
    if (active || looperOn)
        borderClr = Clr::gateOn.brighter(0.35f);
    else if (arpStep)
        borderClr = juce::Colour(0xFF00E8C8);
    else
        borderClr = (isPadMode ? Clr::accent : Clr::accent.darker(0.3f));
    g.setColour(borderClr);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), padR, 1.5f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GlobalTouchPlate
// ═══════════════════════════════════════════════════════════════════════════════

GlobalTouchPlate::GlobalTouchPlate(PluginProcessor& p) : proc_(p) {}

void GlobalTouchPlate::mouseDown(const juce::MouseEvent&)
{
    pressed_ = true;
    // Trigger only voices that are in PAD mode (triggerSource == 0).
    // Respect hold mode per voice: in hold mode press = note-off (mirrors individual TouchPlate).
    for (int v = 0; v < 4; ++v)
    {
        const int src = static_cast<int>(
            proc_.apvts.getRawParameterValue("triggerSource" + juce::String(v))->load());
        if (src == 0)
            proc_.setPadState(v, !proc_.padHold_[v].load());
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
            proc_.setPadState(v, proc_.padHold_[v].load());
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

void ScaleKeyboard::parameterChanged(const juce::String& paramID, float /*newValue*/)
{
    // Selecting a preset overrides any previous custom scale editing.
    if (paramID == "scalePreset")
    {
        if (apvts_.getRawParameterValue("useCustomScale")->load() > 0.5f)
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                    apvts_.getParameter("useCustomScale")))
                *p = false;
    }
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
            fill = isNoteActive(n) ? Clr::gateOn : juce::Colours::white;
        }
        else
        {
            const bool inScale = (activeScaleMask_ >> n) & 1;
            fill = inScale ? Clr::gateOn : juce::Colours::white;
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
            fill = isNoteActive(n) ? Clr::gateOn.darker(0.3f) : juce::Colour(0xFF1A1D2E);
        }
        else
        {
            const bool inScale = (activeScaleMask_ >> n) & 1;
            fill = inScale ? Clr::gateOn.darker(0.3f) : juce::Colour(0xFF1A1D2E);
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

    setSize(1120, 810);

    // ── Tooltip window ────────────────────────────────────────────────────────
    addAndMakeVisible(tooltipWindow_);
    tooltipWindow_.setMillisecondsBeforeTipAppears(600);

    // ── Joystick ──────────────────────────────────────────────────────────────
    addAndMakeVisible(joystickPad_);
    styleKnob(joyXAttenKnob_); joyXAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyXAttenLabel_, "X Range");
    styleKnob(joyYAttenKnob_); joyYAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyYAttenLabel_, "Y Range");
    joyXAttenKnob_.setTooltip("X axis pitch range (semitones)");
    joyYAttenKnob_.setTooltip("Y axis pitch range (semitones)");
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
        padHoldBtn_[v].setTooltip("Hold: note stays on after release");
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

    // ── Per-voice SUB8 buttons (right 50% of HOLD strip, wired via APVTS) ─────
    for (int v = 0; v < 4; ++v)
    {
        padSubOctBtn_[v].setButtonText("SUB8");
        padSubOctBtn_[v].setClickingTogglesState(true);
        padSubOctBtn_[v].setTooltip("Sub-octave: emit a note 12 semitones below the main pitch");
        styleButton(padSubOctBtn_[v]);
        subOctAttach_[v] = std::make_unique<juce::ButtonParameterAttachment>(
            *proc_.apvts.getParameter("subOct" + juce::String(v)),
            padSubOctBtn_[v]);
        addAndMakeVisible(padSubOctBtn_[v]);
    }

    // ── Chord intervals ───────────────────────────────────────────────────────
    styleKnob(transposeKnob_);   styleLabel(transposeLabel_,  "Transpose");
    styleKnob(thirdIntKnob_);    styleLabel(thirdIntLabel_,   "3rd Intv");
    styleKnob(fifthIntKnob_);    styleLabel(fifthIntLabel_,   "5th Intv");
    styleKnob(tensionIntKnob_);  styleLabel(tensionIntLabel_, "Ten Intv");
    transposeKnob_.setTooltip("Key (0=C, 11=B)");
    thirdIntKnob_.setTooltip("3rd interval (semitones above root)");
    fifthIntKnob_.setTooltip("5th interval (semitones above root)");
    tensionIntKnob_.setTooltip("Tension interval (semitones above root)");
    addAndMakeVisible(transposeKnob_);  addAndMakeVisible(transposeLabel_);
    addAndMakeVisible(thirdIntKnob_);   addAndMakeVisible(thirdIntLabel_);
    addAndMakeVisible(fifthIntKnob_);   addAndMakeVisible(fifthIntLabel_);
    addAndMakeVisible(tensionIntKnob_); addAndMakeVisible(tensionIntLabel_);

    transposeAtt_  = std::make_unique<SliderAtt>(p.apvts, "globalTranspose", transposeKnob_);
    thirdIntAtt_   = std::make_unique<SliderAtt>(p.apvts, "thirdInterval",   thirdIntKnob_);
    fifthIntAtt_   = std::make_unique<SliderAtt>(p.apvts, "fifthInterval",   fifthIntKnob_);
    tensionIntAtt_ = std::make_unique<SliderAtt>(p.apvts, "tensionInterval", tensionIntKnob_);

    // Override textFromValueFunction on the 3 interval knobs so they display the absolute
    // note name (e.g. "E") rather than a raw semitone count. Must be set AFTER attachment
    // creation because the attachment's constructor overwrites textFromValueFunction.
    {
        static const char* kN[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
        auto noteNameFn = [this](double val) -> juce::String
        {
            const int tr  = static_cast<int>(
                proc_.apvts.getRawParameterValue("globalTranspose")->load());
            const int idx = ((tr + static_cast<int>(std::round(val))) % 12 + 12) % 12;
            return juce::String(kN[idx]);
        };
        thirdIntKnob_  .textFromValueFunction = noteNameFn;
        fifthIntKnob_  .textFromValueFunction = noteNameFn;
        tensionIntKnob_.textFromValueFunction = noteNameFn;
    }

    // ── Octave offsets ────────────────────────────────────────────────────────
    styleKnob(rootOctKnob_);    styleLabel(rootOctLabel_,    "Root Oct");
    styleKnob(thirdOctKnob_);   styleLabel(thirdOctLabel_,   "3rd Oct");
    styleKnob(fifthOctKnob_);   styleLabel(fifthOctLabel_,   "5th Oct");
    styleKnob(tensionOctKnob_); styleLabel(tensionOctLabel_, "Ten Oct");
    rootOctKnob_   .setTooltip("Root octave shift");
    thirdOctKnob_  .setTooltip("3rd octave shift");
    fifthOctKnob_  .setTooltip("5th octave shift");
    tensionOctKnob_.setTooltip("Tension octave shift");
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

    styleLabel(scalePresetLabel_, "SCALE PRESET");
    scalePresetLabel_.setColour(juce::Label::textColourId, Clr::text);
    scalePresetLabel_.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
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
        trigSrc_[i].addItem("Rnd Free", 3);  // was "Random" — label rename only; saved index 2 loads correctly
        trigSrc_[i].addItem("Rnd Hold", 4);  // new item appended at index 3
        styleCombo(trigSrc_[i]);
        styleLabel(trigSrcLabel_[i], trigLabels[i]);
        addAndMakeVisible(trigSrc_[i]);
        addAndMakeVisible(trigSrcLabel_[i]);
        trigSrcAtt_[i] = std::make_unique<ComboAtt>(p.apvts, trigSrcIDs[i], trigSrc_[i]);
    }

    // ── Routing panel setup ───────────────────────────────────────────────────
    routingLabel_.setText("Routing", juce::dontSendNotification);
    routingLabel_.setFont(juce::Font(11.0f));
    addAndMakeVisible(routingLabel_);

    routingModeBox_.addItem("Multi-Channel",  1);
    routingModeBox_.addItem("Single Channel", 2);
    styleCombo(routingModeBox_);
    addAndMakeVisible(routingModeBox_);
    routingModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "singleChanMode", routingModeBox_);

    // Single Channel target dropdown (1-16)
    for (int ch = 1; ch <= 16; ++ch)
        singleChanTargetBox_.addItem("Ch " + juce::String(ch), ch);
    styleCombo(singleChanTargetBox_);
    addAndMakeVisible(singleChanTargetBox_);
    singleChanTargetAtt_ = std::make_unique<ComboAtt>(p.apvts, "singleChanTarget", singleChanTargetBox_);

    // Per-voice channel dropdowns (Multi-Channel mode)
    const char* voiceNames[4] = { "Root ch", "Third ch", "Fifth ch", "Tension ch" };
    for (int v = 0; v < 4; ++v)
    {
        voiceChLabel_[v].setText(voiceNames[v], juce::dontSendNotification);
        voiceChLabel_[v].setFont(juce::Font(10.0f));
        addAndMakeVisible(voiceChLabel_[v]);

        for (int ch = 1; ch <= 16; ++ch)
            voiceChBox_[v].addItem(juce::String(ch), ch);
        styleCombo(voiceChBox_[v]);
        addAndMakeVisible(voiceChBox_[v]);
        voiceChAtt_[v] = std::make_unique<ComboAtt>(p.apvts, "voiceCh" + juce::String(v), voiceChBox_[v]);
    }

    randomDensityKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomDensityKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    randomDensityKnob_.setNumDecimalPlacesToDisplay(0);
    randomDensityKnob_.setTooltip("Max gates per bar (1–64) — Population");
    randomDensityKnob_.setColour(juce::Slider::rotarySliderFillColourId,   Clr::highlight);
    randomDensityKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomDensityKnob_.setColour(juce::Slider::thumbColourId,              Clr::text);
    randomDensityKnob_.setColour(juce::Slider::textBoxTextColourId,        Clr::textDim);
    randomDensityKnob_.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    addAndMakeVisible(randomDensityKnob_);
    randomPopulationAtt_ = std::make_unique<SliderAtt>(p.apvts, "randomPopulation", randomDensityKnob_);

    // ── Probability knob ─────────────────────────────────────────────────────────
    randomProbabilityKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomProbabilityKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    randomProbabilityKnob_.setNumDecimalPlacesToDisplay(2);
    randomProbabilityKnob_.setTooltip("Per-slot fire chance (0=never, 1=always) — Probability");
    randomProbabilityKnob_.setColour(juce::Slider::rotarySliderFillColourId,   Clr::highlight);
    randomProbabilityKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomProbabilityKnob_.setColour(juce::Slider::thumbColourId,              Clr::text);
    randomProbabilityKnob_.setColour(juce::Slider::textBoxTextColourId,        Clr::textDim);
    randomProbabilityKnob_.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    addAndMakeVisible(randomProbabilityKnob_);
    randomProbabilityAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomProbability"), randomProbabilityKnob_);

    styleLabel(randomSubdivLabel_, "SUBDIV");
    addAndMakeVisible(randomSubdivLabel_);
    {
        const juce::StringArray subdivChoices { "1/4", "1/8", "1/16", "1/32", "1/64" };
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
    randomGateTimeKnob_.setTooltip("Gate length: 0=short, 1=full subdivision");
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomGateTimeKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomGateTimeKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    randomGateTimeKnob_.setColour(juce::Slider::textBoxTextColourId,       Clr::textDim);
    randomGateTimeKnob_.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible(randomGateTimeKnob_);
    gateLengthAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("gateLength"), randomGateTimeKnob_);

    // Sync toggle — compact rectangular button like the looper mode buttons
    randomSyncButton_.setButtonText("RND SYNC");
    randomSyncButton_.setClickingTogglesState(true);
    randomSyncButton_.setToggleState(true, juce::dontSendNotification);
    randomSyncButton_.setTooltip("ON: fires only while DAW plays. OFF: free tempo.");
    styleButton(randomSyncButton_);
    addAndMakeVisible(randomSyncButton_);
    randomSyncButtonAtt_ = std::make_unique<juce::ButtonParameterAttachment>(
        *p.apvts.getParameter("randomClockSync"), randomSyncButton_);

    // Free tempo knob
    randomFreeTempoKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomFreeTempoKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    randomFreeTempoKnob_.setTooltip("Free tempo BPM (active when Sync is OFF)");
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomFreeTempoKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    addAndMakeVisible(randomFreeTempoKnob_);
    randomFreeTempoKnobAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomFreeTempo"), randomFreeTempoKnob_);

    // ── Filter attenuators (smaller — grouped visually with their Base knob) ──
    styleKnob(filterXAttenKnob_); filterXAttenKnob_.setTextValueSuffix(" %");
    filterXAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    styleLabel(filterXAttenLabel_, "Atten");
    styleKnob(filterYAttenKnob_); filterYAttenKnob_.setTextValueSuffix(" %");
    filterYAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    styleLabel(filterYAttenLabel_, "Atten");
    filterXAttenKnob_.setTooltip("Cutoff mod depth (%)");
    filterYAttenKnob_.setTooltip("Resonance mod depth (%)");
    addAndMakeVisible(filterXAttenKnob_); addAndMakeVisible(filterXAttenLabel_);
    addAndMakeVisible(filterYAttenKnob_); addAndMakeVisible(filterYAttenLabel_);
    filterXAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXAtten", filterXAttenKnob_);
    filterYAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYAtten", filterYAttenKnob_);

    // ── Filter CC base — resting CC when stick is centred ──────────────────
    styleKnob(filterXOffsetKnob_); filterXOffsetKnob_.setTextValueSuffix("");
    styleLabel(filterXOffsetLabel_, "Cut Base");
    styleKnob(filterYOffsetKnob_); filterYOffsetKnob_.setTextValueSuffix("");
    styleLabel(filterYOffsetLabel_, "Res Base");
    filterXOffsetKnob_.setTooltip("Cutoff CC at stick rest (0–127)");
    filterYOffsetKnob_.setTooltip("Resonance CC at stick rest (0–127)");
    addAndMakeVisible(filterXOffsetKnob_); addAndMakeVisible(filterXOffsetLabel_);
    addAndMakeVisible(filterYOffsetKnob_); addAndMakeVisible(filterYOffsetLabel_);
    filterXOffsetAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXOffset", filterXOffsetKnob_);
    filterYOffsetAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYOffset", filterYOffsetKnob_);

    // ── Joystick threshold slider ─────────────────────────────────────────────
    thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider_.setTooltip("Motion threshold to trigger a note");
    thresholdSlider_.setColour(juce::Slider::trackColourId,      Clr::highlight);
    thresholdSlider_.setColour(juce::Slider::backgroundColourId, Clr::accent);
    thresholdSlider_.setColour(juce::Slider::thumbColourId,      Clr::text);
    addAndMakeVisible(thresholdSlider_);
    thresholdSliderAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("joystickThreshold"), thresholdSlider_);

    // ── Looper ────────────────────────────────────────────────────────────────
    loopPlayBtn_.setButtonText("PLAY");  loopPlayBtn_.setClickingTogglesState(true);
    loopRecBtn_.setButtonText("REC");    loopRecBtn_.setClickingTogglesState(true);
    loopResetBtn_.setButtonText("RST");   loopResetBtn_.setTooltip("Reset loop (Gamepad: □)");
    loopDeleteBtn_.setButtonText("DEL");  loopDeleteBtn_.setTooltip("Delete loop (Gamepad: ○)");

    styleButton(loopPlayBtn_);  styleButton(loopRecBtn_);
    styleButton(loopResetBtn_); styleButton(loopDeleteBtn_);

    loopPlayBtn_.setTooltip("Start / stop playback");
    loopRecBtn_.setTooltip("Arm recording (starts next bar with DAW sync)");
    loopResetBtn_.setTooltip("Reset to bar 1 (Gamepad: □)");
    loopDeleteBtn_.setTooltip("Erase loop (Gamepad: ○)");

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
    loopRecJoyBtn_.setTooltip("Record joystick pitch into loop");
    loopRecGatesBtn_.setTooltip("Record gate triggers into loop");
    loopSyncBtn_.setTooltip("Sync loop to DAW bar grid");

    loopRecJoyBtn_.setClickingTogglesState(true);
    loopRecGatesBtn_.setClickingTogglesState(true);
    loopSyncBtn_.setClickingTogglesState(true);

    styleButton(loopRecJoyBtn_);
    styleButton(loopRecGatesBtn_);
    styleButton(loopSyncBtn_);
    loopRecJoyBtn_  .setToggleState(true, juce::dontSendNotification);
    loopRecGatesBtn_.setToggleState(true, juce::dontSendNotification);

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

    loopRecWaitBtn_.setButtonText("START REC BY TOUCH");
    loopRecWaitBtn_.setTooltip("Recording starts on next trigger");
    loopRecWaitBtn_.setName("small");
    loopRecWaitBtn_.setClickingTogglesState(true);
    loopRecWaitBtn_.setTriggeredOnMouseDown(true);
    styleButton(loopRecWaitBtn_);
    loopRecWaitBtn_.onClick = [this]
    {
        proc_.looperArmWait();
        // When arming (not cancelling): auto-enable REC GATES if no content flags
        // are currently active, so the first touch actually captures something.
        if (proc_.looperIsRecWaitArmed()
            && !proc_.looperIsRecGates()
            && !proc_.looperIsRecJoy()
            && !proc_.looperIsRecFilter())
        {
            proc_.looperSetRecGates(true);
            loopRecGatesBtn_.setToggleState(true, juce::dontSendNotification);
        }
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

    // ── Quantize mode buttons (Off / Live / Post) ─────────────────────────────
    constexpr int kQuantizeRadioGroup = 1;  // No other radio groups in this editor

    quantizeOffBtn_ .setButtonText("OFF");
    quantizeLiveBtn_.setButtonText("LIVE");
    quantizePostBtn_.setButtonText("POST");

    for (auto* btn : { &quantizeOffBtn_, &quantizeLiveBtn_, &quantizePostBtn_ })
    {
        btn->setRadioGroupId(kQuantizeRadioGroup, juce::dontSendNotification);
        btn->setClickingTogglesState(true);
        styleButton(*btn);
        btn->setColour(juce::TextButton::buttonOnColourId, Clr::gateOn);
        btn->setColour(juce::TextButton::buttonColourId,   Clr::gateOff);
        addAndMakeVisible(*btn);
    }

    quantizeOffBtn_.onClick  = [this] {
        proc_.setQuantizeMode(0);
        proc_.looperRevertQuantize();
    };
    quantizeLiveBtn_.onClick = [this] { proc_.setQuantizeMode(1); };
    quantizePostBtn_.onClick = [this] {
        proc_.setQuantizeMode(2);
        proc_.looperApplyQuantize();
    };

    // Restore button toggle state from APVTS on load
    {
        const int savedMode = proc_.getQuantizeMode();
        quantizeOffBtn_ .setToggleState(savedMode == 0, juce::dontSendNotification);
        quantizeLiveBtn_.setToggleState(savedMode == 1, juce::dontSendNotification);
        quantizePostBtn_.setToggleState(savedMode == 2, juce::dontSendNotification);
    }

    // ── Quantize subdivision dropdown ─────────────────────────────────────────
    quantizeSubdivBox_.addItem("1/4",  1);
    quantizeSubdivBox_.addItem("1/8",  2);
    quantizeSubdivBox_.addItem("1/16", 3);
    quantizeSubdivBox_.addItem("1/32", 4);
    styleCombo(quantizeSubdivBox_);
    addAndMakeVisible(quantizeSubdivBox_);
    quantizeSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "quantizeSubdiv", quantizeSubdivBox_);

    // Gamepad status
    gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
    gamepadStatusLabel_.setFont(juce::Font(10.0f));
    gamepadStatusLabel_.setJustificationType(juce::Justification::centredRight);
    gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(gamepadStatusLabel_);

    // OPTION mode indicator — always visible, grayed out until preset-scroll activates
    optionLabel_.setText("OPTION", juce::dontSendNotification);
    optionLabel_.setFont(juce::Font(10.0f));
    optionLabel_.setJustificationType(juce::Justification::centredRight);
    optionLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(optionLabel_);

    // Chord name display — centered in the middle LFO column strip, large bold font
    chordNameLabel_.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 48.0f, juce::Font::bold));
    chordNameLabel_.setJustificationType(juce::Justification::centred);
    chordNameLabel_.setColour(juce::Label::textColourId,       juce::Colour(0xFF131525));  // dark text
    chordNameLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xFFF0F0F8));  // near-white bg
    chordNameLabel_.setColour(juce::Label::outlineColourId,    Clr::highlight);             // pink border
    addAndMakeVisible(chordNameLabel_);

    // Update gamepad status label with specific controller type on hot-plug events.
    // Passes controller name string (empty = disconnected) via onConnectionChangeUI.
    // Uses onConnectionChangeUI — the slot reserved for the editor.
    // Do NOT assign to onConnectionChange here — that slot is owned by PluginProcessor
    // (set in PluginProcessor ctor) and fires pendingAllNotesOff_ / pendingCcReset_.
    // Overwriting it would silently break stuck-note protection on controller unplug.
    p.getGamepad().onConnectionChangeUI = [this](juce::String controllerName)
    {
        juce::MessageManager::callAsync([this, controllerName]
        {
            if (controllerName.isEmpty())
            {
                gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
                gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
            }
            else
            {
                juce::String labelText;
                if (controllerName.containsIgnoreCase("DualSense"))
                    labelText = "PS5 Connected";
                else if (controllerName.containsIgnoreCase("DualShock 4") ||
                         controllerName.containsIgnoreCase("PS4"))
                    labelText = "PS4 Connected";
                else if (controllerName.containsIgnoreCase("Xbox") ||
                         controllerName.containsIgnoreCase("xinput"))
                    labelText = "Xbox Connected";
                else
                    labelText = "Controller Connected";

                gamepadStatusLabel_.setText(labelText, juce::dontSendNotification);
                gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::gateOn);
            }
        });
    };

    // Per-instance GAMEPAD ACTIVE toggle button.
    // Default ON — this instance responds to the controller.
    // Toggle OFF to silence this instance while another ChordJoystick instance
    // in the same DAW session takes over the controller.
    gamepadActiveBtn_.setButtonText("GAMEPAD ON");
    gamepadActiveBtn_.setTooltip("Enable/disable gamepad for this instance");
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

    // Sync label to current connection state on editor open using same name-detection logic.
    {
        const juce::String initName = p.getGamepad().getControllerName();
        if (!initName.isEmpty())
        {
            juce::String labelText;
            if (initName.containsIgnoreCase("DualSense"))
                labelText = "PS5 Connected";
            else if (initName.containsIgnoreCase("DualShock 4") ||
                     initName.containsIgnoreCase("PS4"))
                labelText = "PS4 Connected";
            else if (initName.containsIgnoreCase("Xbox") ||
                     initName.containsIgnoreCase("xinput"))
                labelText = "Xbox Connected";
            else
                labelText = "Controller Connected";

            gamepadStatusLabel_.setText(labelText, juce::dontSendNotification);
            gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::gateOn);
        }
        else
        {
            gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
        }
    }

    // Left-joystick filter mod button — default ON.
    // When OFF, no filter CC is sent and the LEFT X / LEFT Y dropdowns are greyed out.
    filterModBtn_.setButtonText("FILTER MOD ON");
    filterModBtn_.setTooltip("Enable left-stick filter CC");
    filterModBtn_.setClickingTogglesState(true);
    filterModBtn_.setToggleState(true, juce::dontSendNotification);
    proc_.setFilterModActive(true);
    filterModBtn_.setColour(juce::TextButton::buttonOnColourId,  Clr::gateOn);
    filterModBtn_.setColour(juce::TextButton::buttonColourId,    Clr::gateOff);
    filterModBtn_.onClick = [this]
    {
        const bool active = filterModBtn_.getToggleState();
        proc_.setFilterModActive(active);
        filterModBtn_.setButtonText(active ? "FILTER MOD ON" : "FILTER MOD OFF");
    };
    addAndMakeVisible(filterModBtn_);

    // REC FILTER button — when ON: looper records and plays back filter movements.
    // When OFF: live stick always drives the filter freely, even during looper playback.
    filterRecBtn_.setButtonText("REC FILTER");
    filterRecBtn_.setTooltip("Record filter into loop (OFF = live during playback)");
    filterRecBtn_.setClickingTogglesState(true);
    filterRecBtn_.setToggleState(proc_.looperIsRecFilter(), juce::dontSendNotification);
    styleButton(filterRecBtn_);
    filterRecBtn_.onClick = [this]
    {
        const bool newVal = !proc_.looperIsRecFilter();
        proc_.looperSetRecFilter(newVal);
        filterRecBtn_.setToggleState(newVal, juce::dontSendNotification);
    };
    addAndMakeVisible(filterRecBtn_);

    panicBtn_.setButtonText("PANIC!");
    panicBtn_.setTooltip("Send All Notes Off + All Sound Off on all 16 MIDI channels");
    // Do NOT call setClickingTogglesState — panic is a one-shot action, not a toggle
    styleButton(panicBtn_, Clr::accent);
    panicBtn_.onClick = [this]
    {
        proc_.triggerPanic();
        // flashPanic_ is incremented by the processor's pendingPanic_ handler;
        // timerCallback picks it up and starts the 5-frame flash counter.
    };
    addAndMakeVisible(panicBtn_);

    // Gamepad left-stick axis mode toggles — pill style: left=default, right=alt
    filterYModeBox_.addItem("Resonance (CC71)", 1);
    filterYModeBox_.addItem("LFO Rate  (CC76)", 2);
    filterYModeBox_.setTooltip("Left stick Y: Resonance or LFO Rate");
    styleCombo(filterYModeBox_);
    addAndMakeVisible(filterYModeBox_);
    filterYModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterYMode", filterYModeBox_);

    filterXModeBox_.addItem("Cutoff    (CC74)", 1);
    filterXModeBox_.addItem("VCF LFO   (CC12)", 2);
    filterXModeBox_.addItem("Mod Wheel (CC1)",  3);
    filterXModeBox_.setTooltip("Left stick X: what CC it controls");
    styleCombo(filterXModeBox_);
    addAndMakeVisible(filterXModeBox_);
    filterXModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterXMode", filterXModeBox_);

    // ── Filter Mod hint label (bottom-right) ─────────────────────────────────
    filterModHintLabel_.setText(
        "Left stick sends filter CC to your synth. "
        "Filter Mod enables it. "
        "REC FILTER records stick moves into the looper. "
        "Turn it OFF for live filter control during playback.",
        juce::dontSendNotification);
    filterModHintLabel_.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::plain));
    filterModHintLabel_.setJustificationType(juce::Justification::topLeft);
    filterModHintLabel_.setColour(juce::Label::textColourId, Clr::textDim.brighter(0.3f));
    addAndMakeVisible(filterModHintLabel_);

    // ── Arpeggiator ───────────────────────────────────────────────────────────
    arpEnabledBtn_.setButtonText("ARP OFF");
    arpEnabledBtn_.setClickingTogglesState(true);
    arpEnabledBtn_.setTooltip("Arpeggiator on/off — cycles active voices at selected rate");
    arpEnabledBtn_.setColour(juce::TextButton::buttonOnColourId,  Clr::gateOn);
    arpEnabledBtn_.setColour(juce::TextButton::buttonColourId,    Clr::gateOff);
    arpEnabledBtn_.onClick = [this]
    {
        arpEnabledBtn_.setButtonText(arpEnabledBtn_.getToggleState() ? "ARP ON" : "ARP OFF");
    };
    addAndMakeVisible(arpEnabledBtn_);
    arpEnabledAtt_ = std::make_unique<ButtonAtt>(p.apvts, "arpEnabled", arpEnabledBtn_);

    arpSubdivBox_.addItem("1/4",  1);
    arpSubdivBox_.addItem("1/8T", 2);
    arpSubdivBox_.addItem("1/8",  3);
    arpSubdivBox_.addItem("1/16T",4);
    arpSubdivBox_.addItem("1/16", 5);
    arpSubdivBox_.addItem("1/32", 6);
    arpSubdivBox_.setTooltip("Arpeggiator step rate");
    styleCombo(arpSubdivBox_);
    addAndMakeVisible(arpSubdivBox_);
    styleLabel(arpSubdivLabel_, "RATE");
    addAndMakeVisible(arpSubdivLabel_);
    arpSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "arpSubdiv", arpSubdivBox_);

    arpOrderBox_.addItem("Up",       1);
    arpOrderBox_.addItem("Down",     2);
    arpOrderBox_.addItem("Up+Down",  3);
    arpOrderBox_.addItem("Down+Up",  4);
    arpOrderBox_.addItem("Outer-In", 5);
    arpOrderBox_.addItem("Inner-Out",6);
    arpOrderBox_.addItem("Random",   7);
    arpOrderBox_.setTooltip("Arp voice order pattern");
    styleCombo(arpOrderBox_);
    addAndMakeVisible(arpOrderBox_);
    styleLabel(arpOrderLabel_, "ORDER");
    addAndMakeVisible(arpOrderLabel_);
    arpOrderAtt_ = std::make_unique<ComboAtt>(p.apvts, "arpOrder", arpOrderBox_);

    arpGateTimeKnob_.setSliderStyle(juce::Slider::LinearHorizontal);
    arpGateTimeKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    arpGateTimeKnob_.setRange(0.0, 1.0, 0.01);
    arpGateTimeKnob_.setColour(juce::Slider::thumbColourId,       Clr::highlight);
    arpGateTimeKnob_.setColour(juce::Slider::trackColourId,       Clr::accent);
    arpGateTimeKnob_.setColour(juce::Slider::backgroundColourId,  Clr::gateOff);
    arpGateTimeKnob_.setTooltip("Gate Length — fraction of subdivision (0=manual/open gate, 1=full step)");
    addAndMakeVisible(arpGateTimeKnob_);
    styleLabel(arpGateTimeLabel_, "GATE LEN");
    addAndMakeVisible(arpGateTimeLabel_);
    arpGateTimeAtt_ = std::make_unique<SliderAtt>(p.apvts, "gateLength", arpGateTimeKnob_);

    // ── LFO X panel ───────────────────────────────────────────────────────────
    // Shape ComboBox
    lfoXShapeBox_.addItem("Sine",     1);
    lfoXShapeBox_.addItem("Triangle", 2);
    lfoXShapeBox_.addItem("Saw Up",   3);
    lfoXShapeBox_.addItem("Saw Down", 4);
    lfoXShapeBox_.addItem("Square",   5);
    lfoXShapeBox_.addItem("S&H",      6);
    lfoXShapeBox_.addItem("Random",   7);
    styleCombo(lfoXShapeBox_);
    lfoXShapeBox_.setTooltip("LFO X waveform shape");
    addAndMakeVisible(lfoXShapeBox_);
    lfoXShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXWaveform", lfoXShapeBox_);

    // Rate slider (initial state: free mode attached to lfoXRate)
    lfoXRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXRateSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXRateSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXRateSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXRateSlider_.setTooltip("LFO X rate (Hz in free mode, subdivision in sync mode)");
    lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
        return juce::String(v, 2) + " Hz";
    };
    addAndMakeVisible(lfoXRateSlider_);
    if (auto* param = p.apvts.getParameter("lfoXRate"))
        lfoXRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(*param, lfoXRateSlider_, nullptr);

    // Phase slider
    lfoXPhaseSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXPhaseSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXPhaseSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXPhaseSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXPhaseSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXPhaseSlider_.setTooltip("LFO X phase offset (0-360 degrees)");
    addAndMakeVisible(lfoXPhaseSlider_);
    lfoXPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXPhase", lfoXPhaseSlider_);

    // Level slider
    lfoXLevelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXLevelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXLevelSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXLevelSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXLevelSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXLevelSlider_.setTooltip("LFO X level (0 = off, 1 = full range, 2 = double)");
    addAndMakeVisible(lfoXLevelSlider_);
    lfoXLevelAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXLevel", lfoXLevelSlider_);

    // Distortion slider
    lfoXDistSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXDistSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXDistSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXDistSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXDistSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXDistSlider_.setTooltip("LFO X distortion (jitter/noise amount 0-1)");
    addAndMakeVisible(lfoXDistSlider_);
    lfoXDistAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXDistortion", lfoXDistSlider_);

    // Sync button
    lfoXSyncBtn_.setButtonText("SYNC");
    lfoXSyncBtn_.setClickingTogglesState(true);
    styleButton(lfoXSyncBtn_);
    lfoXSyncBtn_.setTooltip("Sync LFO X to tempo (BPM or DAW)");
    addAndMakeVisible(lfoXSyncBtn_);
    lfoXSyncAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXSync", lfoXSyncBtn_);

    // Sync toggle: swap Rate attachment between free (lfoXRate) and sync (lfoXSubdiv)
    lfoXSyncBtn_.onClick = [this]()
    {
        const bool syncOn = lfoXSyncBtn_.getToggleState();
        lfoXRateAtt_.reset();
        if (syncOn)
        {
            lfoXRateSlider_.setRange(0.0, 5.0, 1.0);
            lfoXRateSlider_.setNumDecimalPlacesToDisplay(0);
            lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                static const char* names[] = {"1/1","1/2","1/4","1/8","1/16","1/32"};
                return names[juce::jlimit(0, 5, (int)std::round(v))];
            };
            if (auto* param = proc_.apvts.getParameter("lfoXSubdiv"))
                lfoXRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                    *param, lfoXRateSlider_, nullptr);
        }
        else
        {
            lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                return juce::String(v, 2) + " Hz";
            };
            if (auto* param = proc_.apvts.getParameter("lfoXRate"))
                lfoXRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                    *param, lfoXRateSlider_, nullptr);
        }
    };

    // Enabled: hidden ToggleButton with ButtonAttachment — clickable LED area in paint()
    lfoXEnabledBtn_.setButtonText("ON");
    lfoXEnabledBtn_.setClickingTogglesState(true);
    styleButton(lfoXEnabledBtn_);
    lfoXEnabledBtn_.setTooltip("Enable/disable LFO X");
    addAndMakeVisible(lfoXEnabledBtn_);
    lfoXEnabledAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXEnabled", lfoXEnabledBtn_);

    // ── LFO Y panel ───────────────────────────────────────────────────────────
    // Shape ComboBox
    lfoYShapeBox_.addItem("Sine",     1);
    lfoYShapeBox_.addItem("Triangle", 2);
    lfoYShapeBox_.addItem("Saw Up",   3);
    lfoYShapeBox_.addItem("Saw Down", 4);
    lfoYShapeBox_.addItem("Square",   5);
    lfoYShapeBox_.addItem("S&H",      6);
    lfoYShapeBox_.addItem("Random",   7);
    styleCombo(lfoYShapeBox_);
    lfoYShapeBox_.setTooltip("LFO Y waveform shape");
    addAndMakeVisible(lfoYShapeBox_);
    lfoYShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoYWaveform", lfoYShapeBox_);

    // Rate slider (initial state: free mode attached to lfoYRate)
    lfoYRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYRateSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYRateSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYRateSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYRateSlider_.setTooltip("LFO Y rate (Hz in free mode, subdivision in sync mode)");
    lfoYRateSlider_.textFromValueFunction = [](double v) -> juce::String {
        return juce::String(v, 2) + " Hz";
    };
    addAndMakeVisible(lfoYRateSlider_);
    if (auto* param = p.apvts.getParameter("lfoYRate"))
        lfoYRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(*param, lfoYRateSlider_, nullptr);

    // Phase slider
    lfoYPhaseSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYPhaseSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYPhaseSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYPhaseSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYPhaseSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYPhaseSlider_.setTooltip("LFO Y phase offset (0-360 degrees)");
    addAndMakeVisible(lfoYPhaseSlider_);
    lfoYPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYPhase", lfoYPhaseSlider_);

    // Level slider
    lfoYLevelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYLevelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYLevelSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYLevelSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYLevelSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYLevelSlider_.setTooltip("LFO Y level (0 = off, 1 = full range, 2 = double)");
    addAndMakeVisible(lfoYLevelSlider_);
    lfoYLevelAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYLevel", lfoYLevelSlider_);

    // Distortion slider
    lfoYDistSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYDistSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYDistSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYDistSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYDistSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYDistSlider_.setTooltip("LFO Y distortion (jitter/noise amount 0-1)");
    addAndMakeVisible(lfoYDistSlider_);
    lfoYDistAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYDistortion", lfoYDistSlider_);

    // Sync button
    lfoYSyncBtn_.setButtonText("SYNC");
    lfoYSyncBtn_.setClickingTogglesState(true);
    styleButton(lfoYSyncBtn_);
    lfoYSyncBtn_.setTooltip("Sync LFO Y to tempo (BPM or DAW)");
    addAndMakeVisible(lfoYSyncBtn_);
    lfoYSyncAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoYSync", lfoYSyncBtn_);

    // Sync toggle: swap Rate attachment between free (lfoYRate) and sync (lfoYSubdiv)
    lfoYSyncBtn_.onClick = [this]()
    {
        const bool syncOn = lfoYSyncBtn_.getToggleState();
        lfoYRateAtt_.reset();
        if (syncOn)
        {
            lfoYRateSlider_.setRange(0.0, 5.0, 1.0);
            lfoYRateSlider_.setNumDecimalPlacesToDisplay(0);
            lfoYRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                static const char* names[] = {"1/1","1/2","1/4","1/8","1/16","1/32"};
                return names[juce::jlimit(0, 5, (int)std::round(v))];
            };
            if (auto* param = proc_.apvts.getParameter("lfoYSubdiv"))
                lfoYRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                    *param, lfoYRateSlider_, nullptr);
        }
        else
        {
            lfoYRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                return juce::String(v, 2) + " Hz";
            };
            if (auto* param = proc_.apvts.getParameter("lfoYRate"))
                lfoYRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(
                    *param, lfoYRateSlider_, nullptr);
        }
    };

    // Enabled: hidden ToggleButton with ButtonAttachment — clickable LED area in paint()
    lfoYEnabledBtn_.setButtonText("ON");
    lfoYEnabledBtn_.setClickingTogglesState(true);
    styleButton(lfoYEnabledBtn_);
    lfoYEnabledBtn_.setTooltip("Enable/disable LFO Y");
    addAndMakeVisible(lfoYEnabledBtn_);
    lfoYEnabledAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoYEnabled", lfoYEnabledBtn_);

    // Capture current preset so the first timer tick doesn't misread a change.
    lastScalePreset_ = static_cast<int>(p.apvts.getRawParameterValue("scalePreset")->load());

    startTimerHz(60);  // 60 Hz for smooth LFO joystick tracking
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

    // Fixed left column width (448px = same as at 920px window width)
    constexpr int kLeftColW = 448;
    auto left = area.removeFromLeft(kLeftColW);
    area.removeFromLeft(8);  // gap between left column and LFO panels

    // LFO X panel column (150px — matches half of joystick pad width)
    auto lfoXCol = area.removeFromLeft(150);
    area.removeFromLeft(4);

    // LFO Y panel column (150px)
    auto lfoYCol = area.removeFromLeft(150);
    area.removeFromLeft(4);

    // Remaining right area (joystick + knobs + pads)
    auto right = area;

    dividerX_ = lfoXCol.getX();  // stays at ~464px from window left

    right.removeFromTop(14);  // push right column content down

    // ── RIGHT COLUMN ──────────────────────────────────────────────────────────

    // Joystick pad — square, centered horizontally in the right column
    const int padSize = juce::jmin(right.getWidth(), 300);
    {
        auto padRow = right.removeFromTop(padSize);
        const int padX = padRow.getX() + (padRow.getWidth() - padSize) / 2;
        joystickPad_.setBounds(padX, padRow.getY(), padSize, padSize);
    }

    right.removeFromTop(14);  // clearance so CUTOFF / RESONANCE titles are not hidden behind pad

    // Knob row: CUTOFF group | RESONANCE group
    // (X Range / Y Range knobs are now positioned under their LFO columns)
    {
        auto row = right.removeFromTop(76);
        const int colW = (row.getWidth() - 3) / 2;  // 2 equal columns, 1 gap × 3px

        // CUTOFF group: Atten (left 50%) | Base (right 50%)
        {
            auto col = row.removeFromLeft(colW); row.removeFromLeft(3);
            filterCutGroupBounds_ = col;
            const int atW = col.getWidth() / 2;
            auto aCol = col.removeFromLeft(atW); aCol.removeFromLeft(2);
            auto bCol = col; bCol.removeFromRight(1);
            filterXAttenLabel_.setBounds(aCol.removeFromTop(14)); filterXAttenKnob_.setBounds(aCol);
            filterXOffsetLabel_.setBounds(bCol.removeFromTop(14)); filterXOffsetKnob_.setBounds(bCol);
        }

        // RESONANCE group: Atten (left 50%) | Base (right 50%)
        {
            auto col = row;
            filterResGroupBounds_ = col;
            const int atW = col.getWidth() / 2;
            auto aCol = col.removeFromLeft(atW); aCol.removeFromLeft(2);
            auto bCol = col; bCol.removeFromRight(1);
            filterYAttenLabel_.setBounds(aCol.removeFromTop(14)); filterYAttenKnob_.setBounds(aCol);
            filterYOffsetLabel_.setBounds(bCol.removeFromTop(14)); filterYOffsetKnob_.setBounds(bCol);
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
            auto holdStrip = b.removeFromTop(18).reduced(2, 2);
            auto holdLeft  = holdStrip.removeFromLeft(holdStrip.getWidth() / 2);
            padHoldBtn_[v].setBounds(holdLeft);
            padSubOctBtn_[v].setBounds(holdStrip);  // remainder = right 50%
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

    // Gamepad status row: 4 equal-width buttons [GAMEPAD] [FILTER MOD] [REC FILTER] [PANIC]
    {
        auto row = right.removeFromTop(20);
        const int bW = (row.getWidth() - 3 * 4) / 4;  // equal width, 4px gaps
        gamepadActiveBtn_.setBounds(row.removeFromLeft(bW)); row.removeFromLeft(4);
        filterModBtn_.setBounds(row.removeFromLeft(bW));     row.removeFromLeft(4);
        filterRecBtn_.setBounds(row.removeFromLeft(bW));     row.removeFromLeft(4);
        panicBtn_.setBounds(row);  // takes remaining (same width as others)
        // Bottom-right strip: [controller 148px] [4px] [OPTION 48px] [8px margin]
        optionLabel_.setBounds       (getWidth() - 8 - 48,       getHeight() - 20, 48,  16);
        gamepadStatusLabel_.setBounds(getWidth() - 8 - 48 - 4 - 148, getHeight() - 20, 148, 16);
    }

    // GAMEPAD panel bounds not used — right column panels removed (label conflicts)

    // Left-stick axis mode combos — grouped under the FILTER MOD button
    // (greyed out when FILTER MOD is OFF)
    right.removeFromTop(12);
    filterYModeBox_.setBounds(right.removeFromTop(22));
    right.removeFromTop(12);
    filterXModeBox_.setBounds(right.removeFromTop(22));

    // Quantize trigger — all 4 controls in one row: [Off][Live][Post][subdiv]
    right.removeFromTop(20);
    {
        auto qRow = right.removeFromTop(18);
        constexpr int kDropW = 52;  // wide enough for "1/32"
        constexpr int kGap   = 2;
        const int btnW = (qRow.getWidth() - kDropW - 3 * kGap) / 3;
        quantizeOffBtn_ .setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
        quantizeLiveBtn_.setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
        quantizePostBtn_.setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
        quantizeSubdivBox_.setBounds(qRow);
    }

    // Routing panel — compact, below quantize trigger, above PS4 status label
    right.removeFromTop(4);
    {
        const int rPanelX = right.getX();
        const int rPanelW = right.getWidth();

        // "Routing" label line
        routingLabel_.setBounds(rPanelX, right.getY(), rPanelW, 12);
        right.removeFromTop(12);

        // Mode dropdown — full width, taller for prominence
        routingModeBox_.setBounds(right.removeFromTop(20));
        right.removeFromTop(2);

        // All 4 voice ch combos in one row + single-channel target — same band,
        // visibility toggled by timerCallback.
        constexpr int comboH = 16;
        const int rY = right.getY();
        const int slotW = rPanelW / 4;

        singleChanTargetBox_.setBounds(rPanelX, rY, rPanelW, comboH);
        for (int v = 0; v < 4; ++v)
            voiceChBox_[v].setBounds(rPanelX + v * slotW, rY, slotW - 2, comboH);
        right.removeFromTop(comboH);

        // Row labels not needed — collapse to zero
        for (int v = 0; v < 4; ++v)
            voiceChLabel_[v].setBounds(0, 0, 0, 0);
    }

    // FILTER MOD panel bounds not used — right column panels removed (label conflicts)

    // Filter Mod hint is now drawn directly in paint() aligned with the left footer rows.
    filterModHintLabel_.setBounds(0, 0, 0, 0);

    // ── LEFT COLUMN ───────────────────────────────────────────────────────────

    // Scale section
    {
        auto section = left.removeFromTop(136);
        scalePresetLabel_.setBounds(section.removeFromTop(18));
        scalePresetBox_  .setBounds(section.removeFromTop(30));
        section.removeFromTop(4);
        scaleKeys_       .setBounds(section.removeFromTop(84));
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

        // Random controls row — [POP] | [PROB] | [GATE] | [RND SYNC] | [FREE BPM knob]
        {
            auto rndRow = left.removeFromTop(60);
            const int cw5 = rndRow.getWidth() / 5;  // 5-column layout for random strip
            randomDensityKnob_     .setBounds(rndRow.removeFromLeft(cw5));   // POPULATION
            randomProbabilityKnob_ .setBounds(rndRow.removeFromLeft(cw5));   // PROBABILITY (new)
            randomGateTimeKnob_    .setBounds(rndRow.removeFromLeft(cw5));   // GATE LEN
            // Compact rectangular button, centred in its column and same height as looper mode buttons
            randomSyncButton_      .setBounds(rndRow.removeFromLeft(cw5).withSizeKeepingCentre(cw5 - 6, 26));
            randomFreeTempoKnob_   .setBounds(rndRow);                       // FREE BPM, 5th col
        }

        // BPM display: under FREE BPM knob (5th column in the random 5-col layout)
        {
            auto bpmRow = left.removeFromTop(16);
            const int skipW = (bpmRow.getWidth() * 4) / 5;  // skip POP + PROB + GATE + SYNC columns
            bpmRow.removeFromLeft(skipW);
            bpmDisplayLabel_.setBounds(bpmRow);
        }
    }

    // Reserve ARP block at the very bottom of the left column before the looper
    // consumes the rest. Height: 22 gap + 22 button + 4 gap + 14 label + 22 combo = 84px.
    // The 16px trim on the panel border puts the ARPEGGIATOR title safely below
    // the looper panel's bottom border line.
    constexpr int kArpH = 84;
    arpBlockBounds_ = left.removeFromBottom(kArpH);

    // Looper — top-aligned with padAll_ and chord display
    {
        auto section = juce::Rectangle<int>(left.getX(), padAll_.getY(),
                                            left.getWidth(), left.getBottom() - padAll_.getY());

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

        // Looper position bar — 4px strip between mode buttons and subdiv/length controls
        looperPositionBarBounds_ = section.removeFromTop(4);
        section.removeFromTop(4);  // 4px gap after bar before subdiv row

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

        // Looper panel bounds — computed from actual control bounds after layout
        // (quantize section moved to right column, so not included here)
        looperPanelBounds_ = loopPlayBtn_.getBounds()
            .withX(left.getX())
            .withWidth(left.getWidth())
            .expanded(0, 4);
    }

    // Arpeggiator block — bottom-left panel
    // Layout: [ARP ON/OFF button full-width]
    //         [RATE label | ORDER label | GATE% label]
    //         [RATE combo | ORDER combo | GATE slider]
    {
        auto r = arpBlockBounds_;
        r.removeFromTop(22);  // extra visual gap so ARPEGGIATOR label clears looper border
        arpEnabledBtn_.setBounds(r.removeFromTop(22));
        r.removeFromTop(4);
        const int third = r.getWidth() / 3;
        auto labelRow = r.removeFromTop(14);
        arpSubdivLabel_.setBounds(labelRow.removeFromLeft(third));
        arpOrderLabel_ .setBounds(labelRow.removeFromLeft(third));
        arpGateTimeLabel_.setBounds(labelRow);
        auto comboRow = r.removeFromTop(22);
        arpSubdivBox_    .setBounds(comboRow.removeFromLeft(third).reduced(1, 0));
        arpOrderBox_     .setBounds(comboRow.removeFromLeft(third).reduced(1, 0));
        arpGateTimeKnob_ .setBounds(comboRow.reduced(1, 0));
    }

    // ── LFO X panel layout ─────────────────────────────────────────────────────
    {
        auto col = lfoXCol;

        col.removeFromTop(14);  // clear header area

        // Row 0: ON button (full width)
        lfoXEnabledBtn_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1: Shape ComboBox (full width)
        lfoXShapeBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 2: Rate slider (left 34px = label space in paint())
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoXRateSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 3: Phase slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoXPhaseSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 4: Level slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoXLevelSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 5: Distortion slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoXDistSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 6: SYNC button (full width)
        lfoXSyncBtn_.setBounds(col.removeFromTop(22));

        // Panel bounds: wraps from ON button top to SYNC button bottom
        lfoXPanelBounds_ = lfoXEnabledBtn_.getBounds()
                              .getUnion(lfoXSyncBtn_.getBounds())
                              .withX(lfoXCol.getX())
                              .withWidth(lfoXCol.getWidth())
                              .expanded(0, 10);

        // X Range knob: directly below LFO X panel
        col.removeFromTop(16);
        joyXAttenLabel_.setBounds(lfoXCol.getX(), col.getY(), lfoXCol.getWidth(), 14);
        col.removeFromTop(14);
        joyXAttenKnob_.setBounds(lfoXCol.getX(), col.getY(), lfoXCol.getWidth(), 60);
    }

    // ── LFO Y panel layout ─────────────────────────────────────────────────────
    {
        auto col = lfoYCol;

        col.removeFromTop(14);  // clear header area

        // Row 0: ON button (full width)
        lfoYEnabledBtn_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1: Shape ComboBox (full width)
        lfoYShapeBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 2: Rate slider (left 34px = label space in paint())
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoYRateSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 3: Phase slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoYPhaseSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 4: Level slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoYLevelSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 5: Distortion slider
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoYDistSlider_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 6: SYNC button (full width)
        lfoYSyncBtn_.setBounds(col.removeFromTop(22));

        // Panel bounds: wraps from ON button top to SYNC button bottom
        lfoYPanelBounds_ = lfoYEnabledBtn_.getBounds()
                              .getUnion(lfoYSyncBtn_.getBounds())
                              .withX(lfoYCol.getX())
                              .withWidth(lfoYCol.getWidth())
                              .expanded(0, 10);

        // Y Range knob: directly below LFO Y panel
        col.removeFromTop(16);
        joyYAttenLabel_.setBounds(lfoYCol.getX(), col.getY(), lfoYCol.getWidth(), 14);
        col.removeFromTop(14);
        joyYAttenKnob_.setBounds(lfoYCol.getX(), col.getY(), lfoYCol.getWidth(), 60);
    }
    // ── JOYSTICK GATE% slider + Chord name label ─────────────────────────────
    // Slider spans both LFO columns, sits just below X/Y Range knobs.
    // Chord label is centred in the remaining free space below the slider.
    {
        const int chordX = lfoXCol.getX();
        const int chordW = lfoYCol.getRight() - chordX;   // lfoX + gap + lfoY = 304px
        thresholdSlider_.setBounds(chordX, joystickPad_.getBottom() + 4, chordW, 18);
        constexpr int chordH = 72;
        chordNameLabel_.setBounds(chordX, padAll_.getY(), chordW, chordH);
    }

    (void)rowH;
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
    g.drawText("DIMEA CHORD JOYSTICK MK2", header, juce::Justification::centred);

    // Outer border
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRect(getLocalBounds().reduced(2), 1);

    // Subtle vertical divider between left and right columns — fixed at column boundary
    if (joystickPad_.isVisible())
    {
        g.setColour(Clr::accent.withAlpha(0.5f));
        g.drawLine((float)(dividerX_ - 4), 32.0f, (float)(dividerX_ - 4), (float)getHeight() - 8, 1.0f);
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

    // ── Filter knob group panels ─────────────────────────────────────────────
    // Drawn behind child components to create a visual grouping for each filter pair.
    auto drawFilterGroup = [&](juce::Rectangle<int> bounds, const juce::String& title)
    {
        if (bounds.isEmpty()) return;
        const auto fb = bounds.toFloat().expanded(3.0f, 4.0f);
        g.setColour(Clr::panel.brighter(0.08f));
        g.fillRoundedRectangle(fb, 5.0f);
        g.setColour(Clr::accent.brighter(0.25f));
        g.drawRoundedRectangle(fb, 5.0f, 1.0f);
        // Group header above the panel
        g.setColour(Clr::textDim.brighter(0.3f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText(title, (int)fb.getX(), (int)fb.getY() - 12, (int)fb.getWidth(), 12,
                   juce::Justification::centred);
    };
    drawFilterGroup(filterCutGroupBounds_, "CUTOFF");
    drawFilterGroup(filterResGroupBounds_, "RESONANCE");

    // Arpeggiator panel border — trim the top 8px so the border starts below the looper
    if (!arpBlockBounds_.isEmpty())
    {
        const auto fb = arpBlockBounds_.toFloat().withTrimmedTop(16.0f).reduced(0.0f, 2.0f);
        g.setColour(Clr::panel.brighter(0.08f));
        g.fillRoundedRectangle(fb, 5.0f);
        g.setColour(Clr::accent.brighter(0.25f));
        g.drawRoundedRectangle(fb, 5.0f, 1.0f);
        g.setColour(Clr::text);  // white like scale preset
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText("ARPEGGIATOR", (int)fb.getX(), (int)fb.getY() - 12,
                   (int)fb.getWidth(), 12, juce::Justification::centred);
    }

    // ── Section panels: LOOPER / FILTER MOD / GAMEPAD ─────────────────────────
    auto drawSectionPanel = [&](juce::Rectangle<int> bounds, const juce::String& title)
    {
        if (bounds.isEmpty()) return;
        const auto fb = bounds.toFloat().reduced(1.0f, 0.0f);

        // Border only — no fill so child components and custom labels remain fully visible
        g.setColour(Clr::accent.brighter(0.5f));
        g.drawRoundedRectangle(fb.reduced(0.5f), 7.0f, 1.5f);

        // Measure title text width
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
        const int textW = g.getCurrentFont().getStringWidth(title) + 10;
        const int textH = 12;
        const int textX = (int)(fb.getCentreX()) - textW / 2;
        const int textY = (int)fb.getY() - textH / 2;  // centered on top border line

        // Knockout: erase border line behind label using background color
        g.setColour(Clr::bg);
        g.fillRect(textX, textY, textW, textH);

        // Draw label text — white like scale preset
        g.setColour(Clr::text);
        g.drawText(title, textX, textY, textW, textH, juce::Justification::centred);
    };

    drawSectionPanel(looperPanelBounds_,    "LOOPER");
    // Right-column panels omitted: the floating title fights with drawAbove labels
    // and the button/label text already identifies those sections clearly.

    // ── LFO panel drawing ─────────────────────────────────────────────────────
    auto drawLfoPanel = [&](juce::Rectangle<int> bounds, const juce::String& title)
    {
        if (bounds.isEmpty()) return;
        const auto fb = bounds.toFloat();

        // Panel fill
        g.setColour(Clr::panel.brighter(0.12f));
        g.fillRoundedRectangle(fb, 7.0f);

        // Border
        g.setColour(Clr::accent.brighter(0.5f));
        g.drawRoundedRectangle(fb.reduced(0.5f), 7.0f, 1.5f);

        // Knockout header label (left-aligned for narrow panels) — white, 12pt bold like scale preset
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
        const int textW = g.getCurrentFont().getStringWidth(title) + 10;
        const int textH = 14;
        const int textX = (int)fb.getX() + 6;
        const int textY = (int)fb.getY() - textH / 2;
        g.setColour(Clr::panel.brighter(0.12f));
        g.fillRect(textX, textY, textW, textH);
        g.setColour(Clr::text);
        g.drawText(title, textX, textY, textW, textH, juce::Justification::left);

    };

    drawLfoPanel(lfoXPanelBounds_, "LFO X");
    drawLfoPanel(lfoYPanelBounds_, "LFO Y");

    // LFO slider row labels (Rate, Phase, Level, Dist)
    auto drawSliderLabel = [&](juce::Rectangle<int> sliderBounds, const juce::String& text)
    {
        g.setFont(juce::Font(9.0f));
        g.setColour(Clr::textDim);
        g.drawText(text,
                   sliderBounds.getX() - 34, sliderBounds.getY(),
                   32, sliderBounds.getHeight(),
                   juce::Justification::right);
    };

    if (!lfoXPanelBounds_.isEmpty())
    {
        drawSliderLabel(lfoXRateSlider_.getBounds(),  "Rate");
        drawSliderLabel(lfoXPhaseSlider_.getBounds(), "Phase");
        drawSliderLabel(lfoXLevelSlider_.getBounds(), "Level");
        drawSliderLabel(lfoXDistSlider_.getBounds(),  "Dist");
    }
    if (!lfoYPanelBounds_.isEmpty())
    {
        drawSliderLabel(lfoYRateSlider_.getBounds(),  "Rate");
        drawSliderLabel(lfoYPhaseSlider_.getBounds(), "Phase");
        drawSliderLabel(lfoYLevelSlider_.getBounds(), "Level");
        drawSliderLabel(lfoYDistSlider_.getBounds(),  "Dist");
    }

    // ── Looper position bar ───────────────────────────────────────────────────
    if (!looperPositionBarBounds_.isEmpty())
    {
        const auto barB = looperPositionBarBounds_.toFloat();

        // Background track
        g.setColour(Clr::gateOff);
        g.fillRoundedRectangle(barB, 2.0f);

        // Filled sweep — only when looper is playing or recording
        if (proc_.looperIsPlaying() || proc_.looperIsRecording())
        {
            const double len  = proc_.getLooperLengthBeats();
            const double beat = proc_.getLooperPlaybackBeat();
            if (len > 0.0)
            {
                const float fraction = juce::jlimit(0.0f, 1.0f, static_cast<float>(beat / len));
                if (fraction > 0.0f)
                {
                    g.setColour(Clr::gateOn);
                    g.fillRoundedRectangle(barB.withWidth(barB.getWidth() * fraction), 2.0f);
                }
            }
        }
    }

    // Scale preset panel — aligned to exact left/right edges of the trigger dropdown columns
    if (scalePresetBox_.isVisible() && scaleKeys_.isVisible())
    {
        const juce::Rectangle<float> fb(
            (float)scalePresetLabel_.getX(),
            (float)scalePresetLabel_.getY() - 4,
            (float)scalePresetLabel_.getWidth(),
            (float)(scaleKeys_.getBottom() - scalePresetLabel_.getY()) + 6);
        g.setColour(Clr::panel.brighter(0.18f));
        g.fillRoundedRectangle(fb, 6.0f);
        g.setColour(Clr::gateOn.withAlpha(0.35f));
        g.drawRoundedRectangle(fb.reduced(0.5f), 6.0f, 1.5f);
    }

    // Labels above controls — uses drawAbove helper (draws 12px above the component)
    g.setColour(Clr::textDim.brighter(0.2f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
    auto drawAbove = [&](juce::Component& c, const juce::String& t)
    {
        if (c.isVisible())
            g.drawText(t, c.getX(), c.getY() - 12, c.getWidth(), 12,
                       juce::Justification::centred);
    };
    drawAbove(randomDensityKnob_,      "POPULATION");
    drawAbove(randomProbabilityKnob_,  "PROBABILITY");
    drawAbove(randomGateTimeKnob_,     "GATE LEN");
    drawAbove(randomFreeTempoKnob_, "FREE BPM");
    drawAbove(filterYModeBox_,      "LEFT Y");
    drawAbove(filterXModeBox_,      "LEFT X");
    drawAbove(thresholdSlider_,     "JOYSTICK GATE%");

    // "QUANTIZE TRIGGER" section label — spans from Off button left edge to subdiv box right edge
    if (quantizeOffBtn_.isVisible())
    {
        const int labelX = quantizeOffBtn_.getX();
        const int labelW = quantizeSubdivBox_.getRight() - labelX;
        g.setColour(Clr::text);  // white like scale preset
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
        g.drawText("QUANTIZE TRIGGER", labelX, quantizeOffBtn_.getY() - 13, labelW, 12,
                   juce::Justification::left);
    }

    // ── Footer: how-to-use instructions (left column only) ───────────────────
    {
        // Constrain to left column only (not the LFO columns)
        const int footerRight = dividerX_ - 8;
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

    // ── Footer: filter mod hint (right column, aligned with left footer rows) ───
    // Drawn here so the first line shares the same Y as "1. Plugin in MIDI Track",
    // and subsequent lines share the same 14px-row + 3px-gap spacing.
    if (joystickPad_.isVisible())
    {
        const int rx = dividerX_ + 8;
        const int rw = getWidth() - rx - 8;
        int ry = getHeight() - 55;  // identical to first left-footer row Y

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
        g.setColour(Clr::textDim.withAlpha(0.8f));  // same white as left footer

        const juce::String hintRows[] = {
            "Left stick sends filter CC to your synth.",
            "Filter Mod enables it. REC FILTER records into looper.",
            "Turn OFF for live filter control during playback."
        };
        for (const auto& line : hintRows)
        {
            g.drawFittedText(line, rx, ry, rw, 14, juce::Justification::centredLeft, 1, 0.75f);
            ry += 17;  // 14px row + 3px gap — matches left footer spacing exactly
        }
    }

}

// ─── paintOverChildren — beat pulse dot overlaid on top of knobs ──────────────

void PluginEditor::paintOverChildren(juce::Graphics& g)
{
    // Beat pulse dot: drawn ON TOP of the FREE BPM knob so it's always visible
    if (randomFreeTempoKnob_.isVisible() && !randomFreeTempoKnob_.getBounds().isEmpty())
    {
        const auto& kb = randomFreeTempoKnob_.getBounds();
        // Centre the dot in the knob's bounding box
        const float dotX = (float)kb.getCentreX() - 4.0f;
        const float dotY = (float)kb.getCentreY() - 4.0f;
        // Dim grey at beat=0, bright cyan at beat=1.0
        const juce::Colour offClr = juce::Colour(0xFF2A3050);
        const juce::Colour onClr  = juce::Colour(0xFF00E5FF);  // bright cyan
        const juce::Colour dotClr = offClr.interpolatedWith(onClr, beatPulse_)
                                          .withAlpha(0.3f + 0.7f * beatPulse_);
        g.setColour(dotClr);
        g.fillEllipse(dotX, dotY, 8.0f, 8.0f);
    }
}

// ─── Timer (UI refresh) ───────────────────────────────────────────────────────

void PluginEditor::timerCallback()
{
    // Routing panel visibility
    const bool isSingle = (*proc_.apvts.getRawParameterValue("singleChanMode") > 0.5f);
    singleChanTargetBox_.setVisible(isSingle);
    for (int v = 0; v < 4; ++v)
    {
        voiceChLabel_[v].setVisible(!isSingle);
        voiceChBox_[v].setVisible(!isSingle);
    }

    // Partial repaint for looper position bar (driven by this 30 Hz timer)
    // Only call repaint() if the bar area is valid — avoids repainting the full editor.
    if (!looperPositionBarBounds_.isEmpty())
        repaint(looperPositionBarBounds_);

    // Refresh gates, joystick position, looper buttons
    padRoot_.repaint(); padThird_.repaint(); padFifth_.repaint(); padTension_.repaint();
    padAll_.repaint();
    joystickPad_.repaint();

    // ── SUB8 button coloring (30 Hz poll) ────────────────────────────────────
    // bright orange = SUB8 enabled + gate open (note sounding)
    // dim orange    = SUB8 enabled, gate closed
    // darkgrey      = SUB8 disabled
    for (int v = 0; v < 4; ++v)
    {
        const bool subEnabled = (*proc_.apvts.getRawParameterValue("subOct" + juce::String(v)) > 0.5f);
        const bool gateOpen   = proc_.isGateOpen(v);

        if (subEnabled && gateOpen)
            padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId,
                                        juce::Colour(0xFFFF8C00)); // bright orange — sounding
        else if (subEnabled)
            padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId,
                                        juce::Colour(0xFFB36200)); // dim orange — enabled, not sounding
        else
            padSubOctBtn_[v].setColour(juce::TextButton::buttonColourId,
                                        juce::Colours::darkgrey);  // inactive

        padSubOctBtn_[v].repaint();
    }

    // PLAY button: blinks when armed — three cases:
    //   1. DAW sync on + not playing  → blink to show "waiting for DAW play"
    //   2. "start rec by touch" armed → blink
    //   3. ARP armed for looper (DAW sync OFF only) → blink
    // Solid when playing, off when stopped (non-sync).
    const bool syncArmed      = proc_.looperIsSyncToDaw() && !proc_.looperIsPlaying();
    const bool arpArmedForLooper = proc_.isArpWaitingForPlay() && !proc_.looperIsSyncToDaw();
    if (syncArmed || proc_.looperIsRecWaitArmed() || arpArmedForLooper)
    {
        const bool on = ((++playWaitBlinkCounter_) / 5) % 2 == 0;  // ~3 blinks/sec at 30 Hz
        loopPlayBtn_.setToggleState(on, juce::dontSendNotification);
    }
    else
    {
        playWaitBlinkCounter_ = 0;
        loopPlayBtn_.setToggleState(proc_.looperIsPlaying(), juce::dontSendNotification);
    }
    loopRecWaitBtn_.setToggleState(proc_.looperIsRecWaitArmed(), juce::dontSendNotification);

    // REC button: solid when recording, blinking when armed-but-waiting, off otherwise.
    if (proc_.looperIsRecording())
    {
        recBlinkCounter_ = 0;
        loopRecBtn_.setToggleState(true, juce::dontSendNotification);
    }
    else if (proc_.looperIsRecPending())
    {
        const bool on = ((++recBlinkCounter_) / 5) % 2 == 0;  // ~3 blinks/sec at 30Hz
        loopRecBtn_.setToggleState(on, juce::dontSendNotification);
    }
    else
    {
        recBlinkCounter_ = 0;
        loopRecBtn_.setToggleState(false, juce::dontSendNotification);
    }
    loopDeleteBtn_.setEnabled(!proc_.looperIsPlaying());

    // ── Gamepad button flash: RST (Square), DEL (Circle), PANIC (R3) ─────────
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

    // PANIC! button flash feedback
    if (proc_.flashPanic_.exchange(0, std::memory_order_relaxed) > 0)
        panicFlashCounter_ = 5;   // ~167ms at 30 Hz

    if (panicFlashCounter_ > 0)
    {
        --panicFlashCounter_;
        panicBtn_.setColour(juce::TextButton::buttonColourId,  Clr::highlight);
        panicBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        panicBtn_.setColour(juce::TextButton::buttonColourId,  Clr::accent);
        panicBtn_.setColour(juce::TextButton::textColourOffId, Clr::text);
    }

    // Grey out all filter controls when Filter Mod is off
    {
        const bool filterOn = proc_.isFilterModActive();
        filterYModeBox_   .setEnabled(filterOn);
        filterXModeBox_   .setEnabled(filterOn);
        filterXAttenKnob_ .setEnabled(filterOn);
        filterYAttenKnob_ .setEnabled(filterOn);
        filterXOffsetKnob_.setEnabled(filterOn);
        filterYOffsetKnob_.setEnabled(filterOn);
        filterRecBtn_     .setEnabled(filterOn);
    }

    // ARP button: blinks while armed (ARP ON but waiting for DAW play press to launch),
    // solid when actually running, off when disabled.
    if (arpEnabledBtn_.getToggleState() && proc_.isArpWaitingForPlay())
    {
        const bool on = ((++arpBlinkCounter_) / 5) % 2 == 0;  // ~3 blinks/sec at 30 Hz
        arpEnabledBtn_.setToggleState(on, juce::dontSendNotification);
        arpEnabledBtn_.setButtonText("ARP ARMED");
    }
    else
    {
        arpBlinkCounter_ = 0;
        arpEnabledBtn_.setButtonText(arpEnabledBtn_.getToggleState() ? "ARP ON" : "ARP OFF");
    }

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

    // Update BPM display
    {
        const float bpm = proc_.getEffectiveBpm();
        bpmDisplayLabel_.setText(juce::String(bpm, 1) + " BPM", juce::dontSendNotification);
    }

    // Update OPTION indicator — dim=off, green=octaves mode, red=interval mode
    {
        const int optMode = proc_.getGamepad().getOptionMode();
        const juce::String newText = (optMode == 0) ? "OPTION"
                                   : (optMode == 1) ? "OCTAVE"
                                   :                  "INTRVL";
        const juce::Colour newCol  = (optMode == 0) ? Clr::textDim
                                   : (optMode == 1) ? Clr::gateOn
                                   :                  juce::Colour(0xFFFF4444);  // red for mode 2
        if (optionLabel_.getText() != newText)
            optionLabel_.setText(newText, juce::dontSendNotification);
        optionLabel_.setColour(juce::Label::textColourId, newCol);
    }

    // Poll controller connection so the status label stays accurate even when the
    // SDL disconnect event is delayed (common with Bluetooth on Windows).
    {
        const bool connected = proc_.getGamepad().isConnected();
        if (!connected)
        {
            const juce::String cur = gamepadStatusLabel_.getText();
            if (cur != "No controller")
            {
                gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
                gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
            }
        }
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
    loopRecGatesBtn_.setToggleState(proc_.looperIsRecGates(),  juce::dontSendNotification);
    loopRecJoyBtn_  .setToggleState(proc_.looperIsRecJoy(),    juce::dontSendNotification);
    filterRecBtn_   .setToggleState(proc_.looperIsRecFilter(), juce::dontSendNotification);

    // Mirror gamepad right stick to joystickX/Y.
    // - When the stick moves: write its position and remember it was last writer.
    // - When the stick returns to centre: write 0 once (so the cursor snaps back),
    //   then clear the flag so subsequent mouse clicks are not overridden.
    // - When the stick is at centre and was never moved (flag=false): leave
    //   joystickX/Y untouched so mouse clicks stay in effect.
    if (proc_.getGamepad().isConnected() && proc_.isGamepadActive())
    {
        const float gpX = proc_.getGamepad().getPitchX();
        const float gpY = proc_.getGamepad().getPitchY();
        constexpr float kCenterSnap = 0.05f;
        const float snappedX = std::abs(gpX) < kCenterSnap ? 0.0f : gpX;
        const float snappedY = std::abs(gpY) < kCenterSnap ? 0.0f : gpY;

        if (snappedX != 0.0f || snappedY != 0.0f)
        {
            proc_.joystickX.store(snappedX);
            proc_.joystickY.store(snappedY);
            gamepadWasLastPitchWriter_ = true;
        }
        else if (gamepadWasLastPitchWriter_)
        {
            // Stick just returned to centre — snap cursor back to 0 once
            proc_.joystickX.store(0.0f);
            proc_.joystickY.store(0.0f);
            gamepadWasLastPitchWriter_ = false;
        }
        // else: stick at centre, mouse was last — leave joystickX/Y alone
    }
    else
    {
        // Gamepad disconnected or disabled — clear flag so mouse takes over cleanly
        gamepadWasLastPitchWriter_ = false;
    }

    // ── Quantize controls: disable while recording ────────────────────────────
    const bool isLooperRecording = proc_.looperIsRecording();
    quantizeOffBtn_ .setEnabled(!isLooperRecording);
    quantizeLiveBtn_.setEnabled(!isLooperRecording);
    quantizePostBtn_.setEnabled(!isLooperRecording);
    quantizeSubdivBox_.setEnabled(!isLooperRecording);

    // Sync mode button toggle state with APVTS (handles external state changes,
    // e.g., DAW automation or setStateInformation restoring a saved state)
    {
        const int curMode = proc_.getQuantizeMode();
        quantizeOffBtn_ .setToggleState(curMode == 0, juce::dontSendNotification);
        quantizeLiveBtn_.setToggleState(curMode == 1, juce::dontSendNotification);
        quantizePostBtn_.setToggleState(curMode == 2, juce::dontSendNotification);
    }

    // ── Beat pulse dot ────────────────────────────────────────────────────────
    // Read and clear the sticky beat flag; set beatPulse_ to 1.0 when triggered.
    // Decay by ~0.11 per tick (30 Hz × 9 frames ≈ 300ms to fade to 0).
    if (proc_.beatOccurred_.exchange(false, std::memory_order_relaxed))
        beatPulse_ = 1.0f;
    else if (beatPulse_ > 0.0f)
        beatPulse_ = juce::jmax(0.0f, beatPulse_ - 0.11f);

    // Repaint FREE BPM knob area to update beat dot (drawn by paintOverChildren)
    if (!randomFreeTempoKnob_.getBounds().isEmpty())
        repaint(randomFreeTempoKnob_.getBounds().expanded(2));

    // Repaint LFO panel header areas (title knockout may change)
    if (!lfoXPanelBounds_.isEmpty()) repaint(lfoXPanelBounds_);
    if (!lfoYPanelBounds_.isEmpty()) repaint(lfoYPanelBounds_);

    // ── Scale mask: snap slider info + auto-snap interval values on change ──────
    // Runs every tick; snap sliders get fresh mask for drag snapping.
    // When the mask itself changes (scale preset OR transpose changed), the stored
    // APVTS interval values are re-snapped to the nearest scale degree (downward
    // preference) so the knobs immediately display musically correct note names.
    {
        const int      curTr   = static_cast<int>(proc_.apvts.getRawParameterValue("globalTranspose")->load());
        const uint16_t curMask = scaleKeys_.getActiveScaleMask();

        // Always push fresh mask into drag-snap sliders
        thirdIntKnob_  .setScaleInfo(curMask, curTr);
        fifthIntKnob_  .setScaleInfo(curMask, curTr);
        tensionIntKnob_.setScaleInfo(curMask, curTr);

        // Refresh note-name text boxes when transpose changes.
        // repaint() only redraws knob graphics; updateText() re-evaluates
        // textFromValueFunction so the text box shows the new note name.
        if (curTr != lastTransposeForKnobs_)
        {
            lastTransposeForKnobs_ = curTr;
            thirdIntKnob_  .updateText();
            fifthIntKnob_  .updateText();
            tensionIntKnob_.updateText();
        }

        // When the scale PRESET changes (not a custom-scale note toggle), reset the
        // three interval knobs to their natural "function" positions — major 3rd (4),
        // perfect 5th (7), major 7th (11) — before the snap block below quantises
        // them to the actual scale degrees.  Custom scale is excluded: the user owns
        // those toggles and the intervals should not be disturbed.
        {
            const int  curPreset  = static_cast<int>(proc_.apvts.getRawParameterValue("scalePreset")->load());
            const bool customOn   = *proc_.apvts.getRawParameterValue("useCustomScale") > 0.5f;

            if (!customOn && curPreset != lastScalePreset_)
            {
                lastScalePreset_ = curPreset;
                auto resetParam = [&](const char* id, int value)
                {
                    auto* p = proc_.apvts.getParameter(id);
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));
                };
                resetParam("thirdInterval",   4);
                resetParam("fifthInterval",   7);
                resetParam("tensionInterval", 11);
            }
        }

        // Re-snap interval APVTS values when the scale mask changes
        if (curMask != lastScaleMaskForSnap_)
        {
            lastScaleMaskForSnap_ = curMask;

            auto snapParam = [&](const char* id)
            {
                const int cur     = static_cast<int>(proc_.apvts.getRawParameterValue(id)->load());
                const int snapped = snapIntervalToScale(cur, curMask, curTr);
                if (snapped != cur)
                {
                    auto* p = proc_.apvts.getParameter(id);
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(snapped)));
                }
            };
            snapParam("thirdInterval");
            snapParam("fifthInterval");
            snapParam("tensionInterval");
            // APVTS attachment update is async; call updateText() immediately so
            // the note-name text boxes reflect the snapped value right away.
            thirdIntKnob_  .updateText();
            fifthIntKnob_  .updateText();
            tensionIntKnob_.updateText();
        }
    }

    // ── Chord name display ────────────────────────────────────────────────────
    // heldPitch_ is updated every processBlock (post-LFO), so refreshing on
    // every timer tick keeps the label live: LFO sweeps, scale changes, and
    // transpose all reflect immediately.  Label::setText is a no-op when the
    // text hasn't changed, so there is no unnecessary repaint cost.
    {
        const auto pitches = proc_.getCurrentPitches();
        const int transposePc = (int)proc_.apvts.getRawParameterValue("globalTranspose")->load();
        chordNameLabel_.setText(computeChordName(pitches.data(), transposePc), juce::dontSendNotification);
    }

}
