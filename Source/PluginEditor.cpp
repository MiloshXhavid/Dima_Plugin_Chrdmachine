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

static juce::String computeChordName(const int pitches[4])
{
    return juce::String(computeChordNameStr(pitches));
}

// ─── PixelLookAndFeel implementation ─────────────────────────────────────────

void PixelLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
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

    // Detect bipolar knobs (min == -max, e.g. MOD FIX -50..+50).
    // For these, draw the arc from noon (centre angle) to current position,
    // coloured red (positive) or blue (negative).
    const float sMin = (float)slider.getMinimum();
    const float sMax = (float)slider.getMaximum();
    const bool isBipolar = (sMin < 0.0f && sMax > 0.0f &&
                            std::abs(sMin + sMax) < 0.01f);
    if (isBipolar)
    {
        const float centreAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;
        const float val = (float)slider.getValue();
        if (std::abs(val) > 0.5f)
        {
            const bool positive   = val > 0.0f;
            const float arcStart  = positive ? centreAngle : endAngle;
            const float arcEnd    = positive ? endAngle    : centreAngle;
            const juce::Colour arcClr = positive
                ? juce::Colour(0xFFE03030)   // red
                : juce::Colour(0xFF3060FF);  // blue
            juce::Path fillArc;
            fillArc.addArc(cx - trackR, cy - trackR, trackR * 2.0f, trackR * 2.0f,
                           arcStart, arcEnd, true);
            g.setColour(arcClr.withAlpha(0.90f));
            g.strokePath(fillArc, thinStroke);
        }
    }
    else
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
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        // Fall back for any vertical linear sliders (none expected but be safe)
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
            sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const float trackY  = y + height * 0.5f - 3.0f;
    const float trackH  = 6.0f;
    const float trackL  = (float)x;
    const float trackR  = (float)(x + width);

    // Background track
    g.setColour(Clr::gateOff);
    g.fillRect(juce::Rectangle<float>(trackL, trackY, (float)width, trackH));

    // Filled portion (value fill, 0 → thumb)
    g.setColour(slider.findColour(juce::Slider::trackColourId));
    g.fillRect(juce::Rectangle<float>(trackL, trackY, sliderPos - trackL, trackH));

    // Modulation anchor indicator: fills from anchor (pre-modulation position) to current thumb.
    // Grows as fader moves away from anchor, shrinks back as it returns.
    // "modAnchor" property is set by timerCallback only on the active slider.
    const auto anchorVar = slider.getProperties()["modAnchor"];
    if (!anchorVar.isVoid())
    {
        const double anchorVal  = (double)anchorVar;
        const float  anchorProp = (float)slider.valueToProportionOfLength(anchorVal);
        const float  anchorPix  = trackL + anchorProp * (float)width;

        const juce::Colour posClr(0xFFE03030);   // red  — fader moved right of anchor
        const juce::Colour negClr(0xFF3060FF);   // blue — fader moved left  of anchor

        if (sliderPos > anchorPix + 1.0f)
        {
            g.setColour(posClr.withAlpha(0.75f));
            g.fillRect(juce::Rectangle<float>(anchorPix, trackY, sliderPos - anchorPix, trackH));
        }
        else if (sliderPos < anchorPix - 1.0f)
        {
            g.setColour(negClr.withAlpha(0.75f));
            g.fillRect(juce::Rectangle<float>(sliderPos, trackY, anchorPix - sliderPos, trackH));
        }
    }

    // Thumb (3px wide white rect) — drawn last so it sits on top of the delta bar
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
// GamepadDisplayComponent
// ═══════════════════════════════════════════════════════════════════════════════

// Sticks must exceed this threshold (post-dead-zone rescaling) to be treated as
// "intentionally active" for mouse-bypass. GamepadInput now rescales pitchX/Y to 0
// inside the dead zone, so a small threshold suffices without jitter risk.
static constexpr float kStickBypassThreshold = 0.05f;

void GamepadDisplayComponent::setGamepadState(uint32_t mask, float lx, float ly,
                                               float rx, float ry, bool connected)
{
    if (mask == buttonMask_ && lx == leftX_ && ly == leftY_ &&
        rx == rightX_ && ry == rightY_ && connected == connected_)
        return;
    buttonMask_ = mask;
    leftX_  = lx;  leftY_  = ly;
    rightX_ = rx;  rightY_ = ry;
    connected_ = connected;

    // Update bypass flags. Values passed in are already post-dead-zone,
    // so zero means within dead zone. We use a higher threshold to
    // ignore PS4 drift that lingers just above the dead zone.
    physicalRightStickActive_ = connected && (std::abs(rx) > kStickBypassThreshold ||
                                              std::abs(ry) > kStickBypassThreshold);
    physicalLeftStickActive_  = connected && (std::abs(lx) > kStickBypassThreshold ||
                                              std::abs(ly) > kStickBypassThreshold);
    repaint();
}

void GamepadDisplayComponent::mouseMove(const juce::MouseEvent& e)
{
    setTooltip(computeRegionTooltip(e.x, e.y));

    // Show hand cursor over draggable sticks (always — mouse can drag even when connected)
    if (proc_ != nullptr)
    {
        const float w = (float)getWidth(), h = (float)getHeight();
        const float bodyY = (float)bodyOffset_;
        const float bodyH = h - bodyY - 4.0f;
        const float lsX = w * 0.22f, lsY = bodyY + bodyH * 0.28f;
        const float lsR = juce::jmin(w * 0.07f, bodyH * 0.12f) * 1.5f;
        const float rsX = w * 0.78f, rsY = bodyY + bodyH * 0.70f;
        const float rsR = juce::jmin(w * 0.065f, bodyH * 0.115f) * 1.5f;
        const float dx = (float)e.x, dy = (float)e.y;
        if (std::hypot(dx - lsX, dy - lsY) < lsR || std::hypot(dx - rsX, dy - rsY) < rsR)
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
            return;
        }
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void GamepadDisplayComponent::mouseExit(const juce::MouseEvent&)
{
    setTooltip({});
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void GamepadDisplayComponent::updateDrag(const juce::MouseEvent& e)
{
    if (proc_ == nullptr || draggingStick_ < 0) return;

    const float w = (float)getWidth(), h = (float)getHeight();
    const float bodyY = (float)bodyOffset_;
    const float bodyH = h - bodyY - 4.0f;
    auto bY = [&](float frac) { return bodyY + bodyH * frac; };

    float cx, cy, radius;
    if (draggingStick_ == 0)
    {
        cx     = w * 0.22f;
        cy     = bY(0.28f);
        radius = juce::jmax(8.0f, juce::jmin(w * 0.07f, bodyH * 0.12f));
    }
    else
    {
        cx     = w * 0.78f;
        cy     = bY(0.70f);
        radius = juce::jmax(8.0f, juce::jmin(w * 0.065f, bodyH * 0.115f));
    }

    dragNormX_ = juce::jlimit(-1.0f, 1.0f, ((float)e.x - cx) / radius);
    dragNormY_ = juce::jlimit(-1.0f, 1.0f, -((float)e.y - cy) / radius);

    if (draggingStick_ == 0)
    {
        // Left stick → filter. Only write if physical left stick is still (not bypassed).
        if (!physicalLeftStickActive_)
            proc_->getGamepad().setMouseFilterOverride(dragNormY_, dragNormX_, true);
    }
    else
    {
        // Right stick → pitch. Only write if physical right stick is still (not bypassed).
        if (!physicalRightStickActive_)
        {
            proc_->joystickX.store(dragNormX_, std::memory_order_relaxed);
            proc_->joystickY.store(dragNormY_, std::memory_order_relaxed);
        }
    }
    repaint();
}

void GamepadDisplayComponent::mouseDown(const juce::MouseEvent& e)
{
    if (proc_ == nullptr) return;

    // ── SWAP button ───────────────────────────────────────────────────────────
    if (ctrlBtnR_ > 0.0f)
    {
        const float dxSw = e.x - swapBtnCentre_.x;
        const float dySw = e.y - swapBtnCentre_.y;
        if (dxSw * dxSw + dySw * dySw <= ctrlBtnR_ * ctrlBtnR_)
        {
            auto* p = proc_->apvts.getParameter("stickSwap");
            if (p) p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
            repaint();
            return;
        }
        const float dxIn = e.x - invBtnCentre_.x;
        const float dyIn = e.y - invBtnCentre_.y;
        if (dxIn * dxIn + dyIn * dyIn <= ctrlBtnR_ * ctrlBtnR_)
        {
            auto* p = proc_->apvts.getParameter("stickInvert");
            if (p) p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
            repaint();
            return;
        }
    }

    const float w = (float)getWidth(), h = (float)getHeight();
    const float bodyY = (float)bodyOffset_;
    const float bodyH = h - bodyY - 4.0f;
    auto bY = [&](float frac) { return bodyY + bodyH * frac; };
    auto dist = [](float ax, float ay, float bx, float by) { return std::hypot(ax - bx, ay - by); };

    const float lsX = w * 0.22f, lsY = bY(0.28f);
    const float lsR = juce::jmin(w * 0.07f, bodyH * 0.12f) * 1.5f;
    const float rsX = w * 0.78f, rsY = bY(0.70f);
    const float rsR = juce::jmin(w * 0.065f, bodyH * 0.115f) * 1.5f;

    if (dist((float)e.x, (float)e.y, lsX, lsY) < lsR)
        draggingStick_ = 0;
    else if (dist((float)e.x, (float)e.y, rsX, rsY) < rsR)
        draggingStick_ = 1;

    if (draggingStick_ >= 0)
        updateDrag(e);
}

void GamepadDisplayComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingStick_ < 0 || proc_ == nullptr) return;
    updateDrag(e);
}

void GamepadDisplayComponent::mouseUp(const juce::MouseEvent&)
{
    if (proc_ == nullptr) return;

    if (draggingStick_ == 0)
        proc_->getGamepad().setMouseFilterOverride(0.0f, 0.0f, false);
    // Right stick: joystickX/Y holds last position (natural S&H like JoystickPad)

    draggingStick_ = -1;
    dragNormX_     = 0.0f;
    dragNormY_     = 0.0f;
    repaint();
}

juce::String GamepadDisplayComponent::computeRegionTooltip(int mx, int my) const
{
    const float w = (float)getWidth();
    const float h = (float)getHeight();
    const float x = (float)mx;
    const float y = (float)my;

    // Mirror the geometry used in paint() exactly.
    const float bodyY = (float)bodyOffset_;
    const float bodyH = h - bodyY - 4.0f;
    auto bY = [&](float frac) { return bodyY + bodyH * frac; };
    auto d  = [](float ax, float ay, float bx, float by)
              { return std::hypot(ax - bx, ay - by); };

    // ── Shoulder buttons (floating above body) ────────────────────────────────
    const float sW    = 56.0f, sGap = 4.0f;
    const float shBot = bodyY - 2.0f;
    const float shH1  = (shBot - 6.0f) * 0.55f;
    const float shY1  = 6.0f;
    const float shY2  = shY1 + shH1 + 2.0f;
    if (x >= sGap && x < sGap + sW) {
        if (y >= shY1 && y < shY2)  return "L2 - trigger Third voice";
        if (y >= shY2 && y < shBot) return "L1 - trigger Root voice";
    }
    if (x >= w - sW - sGap && x < w - sGap) {
        if (y >= shY1 && y < shY2)  return "R2 - trigger Tension voice";
        if (y >= shY2 && y < shBot) return "R1 - trigger Fifth voice";
    }

    // ── Left stick ────────────────────────────────────────────────────────────
    const float lsX = w * 0.22f, lsY = bY(0.28f);
    const float lsR = juce::jmin(w * 0.07f, bodyH * 0.12f) * 1.6f;
    if (d(x, y, lsX, lsY) < lsR)
        return "Left Stick - X: CC74 Filter Cutoff  |  Y: CC71 Resonance  |  Click (L3): trigger all voices";

    // ── Right stick ───────────────────────────────────────────────────────────
    const float rsX = w * 0.78f, rsY = bY(0.70f);
    const float rsR = juce::jmin(w * 0.065f, bodyH * 0.115f) * 1.6f;
    if (d(x, y, rsX, rsY) < rsR)
        return "Right Stick - XY pitch control, mirrors the on-screen joystick pad";

    // ── D-pad ─────────────────────────────────────────────────────────────────
    const float dpX = w * 0.17f, dpY = bY(0.70f);
    const float dpR = juce::jmin(w * 0.042f, bodyH * 0.075f);
    const float dpSp = dpR * 1.95f;
    if (d(x, y, dpX,        dpY - dpSp) < dpR * 1.6f) return "D-Pad Up - next scale preset";
    if (d(x, y, dpX,        dpY + dpSp) < dpR * 1.6f) return "D-Pad Down - previous scale preset";
    if (d(x, y, dpX - dpSp, dpY)        < dpR * 1.6f) return "D-Pad Left - transpose key down one semitone";
    if (d(x, y, dpX + dpSp, dpY)        < dpR * 1.6f) return "D-Pad Right - transpose key up one semitone";

    // ── Face buttons ──────────────────────────────────────────────────────────
    const float fbCX = w * 0.78f, fbCY = bY(0.28f);
    const float fbR  = juce::jmin(w * 0.048f, bodyH * 0.088f);
    const float fbSp = fbR * 1.95f;
    if (d(x, y, fbCX,        fbCY - fbSp) < fbR * 1.6f) return "Triangle  -  arm / record the looper";
    if (d(x, y, fbCX - fbSp, fbCY)        < fbR * 1.6f) return "Square  -  reset loop to bar 1";
    if (d(x, y, fbCX + fbSp, fbCY)        < fbR * 1.6f) return "Circle  -  erase loop contents";
    if (d(x, y, fbCX,        fbCY + fbSp) < fbR * 1.6f) return "Cross  -  start / stop looper playback";

    // ── Options ───────────────────────────────────────────────────────────────
    const float optX = w * 0.5f - 22.0f, optY = bY(0.50f);
    const float optR = juce::jmin(w * 0.033f, bodyH * 0.058f) * 1.6f;
    if (d(x, y, optX, optY) < optR)
        return "Options - toggle preset-scroll mode (OPTION indicator lights green when active)";

    // ── SWAP button (below OPT) ───────────────────────────────────────────────
    if (d(x, y, optX, optY + optR * 3.2f) < optR)
        return "SWAP - swap left/right stick routing: left stick controls pitch, right stick controls filter CC";

    // ── PS button ─────────────────────────────────────────────────────────────
    const float psX = w * 0.5f + 22.0f, psY = bY(0.50f);
    const float psR = juce::jmin(w * 0.038f, bodyH * 0.068f) * 1.6f;
    if (d(x, y, psX, psY) < psR)
        return "PS - soft disconnect / reconnect the controller";

    // ── INV button (below PS) ─────────────────────────────────────────────────
    if (d(x, y, psX, psY + optR * 3.2f) < optR)
        return "INV - invert both axes on both sticks (sign flip); also swaps X/Y labels in Modulation and Quantizer panels";

    return connected_ ? "PS Controller - hover over a button to see its function"
                      : "No controller connected - plug in a PS4/PS5 controller via USB or Bluetooth";
}

void GamepadDisplayComponent::paint(juce::Graphics& g)
{
    const float w   = (float)getWidth();
    const float h   = (float)getHeight();
    const bool  dim = !connected_;

    // ── Stick-routing param states ────────────────────────────────────────────
    const bool swapActive = proc_ && *proc_->apvts.getRawParameterValue("stickSwap")   > 0.5f;
    const bool invActive  = proc_ && *proc_->apvts.getRawParameterValue("stickInvert") > 0.5f;

    // ── Palette ────────────────────────────────────────────────────────────────
    const auto bgFill   = juce::Colour(dim ? 0xFF0E0F18u : 0xFF131525u);
    const auto body     = juce::Colour(dim ? 0xFF0A0B0Fu : 0xFF0A0B14u);
    const auto outline  = juce::Colour(dim ? 0xFF252535u : 0xFF3A4068u);
    const auto btnIdle  = juce::Colour(dim ? 0xFF1C1E28u : 0xFF252840u);
    const auto btnText  = juce::Colour(dim ? 0xFF3A3C50u : 0xFF7880A8u);
    const auto stickRim = juce::Colour(dim ? 0xFF252535u : 0xFF3A4068u);
    const auto stickDot = juce::Colour(dim ? 0xFF3A3C50u : 0xFFCCCEE8u);
    const auto pink     = juce::Colour(dim ? 0xFF252840u : 0xFFFF3D6Eu);

    auto bit = [&](int n) -> bool { return !dim && ((buttonMask_ >> n) & 1u) != 0; };

    g.setColour(bgFill);
    g.fillRect(getLocalBounds());

    // ── Shoulder triggers & buttons (top strip) ────────────────────────────────
    // Drawn at fixed positions above the body — they float in the free space above.
    auto drawShoulder = [&](float x, float y, float bw, float bh,
                             bool lit, const juce::String& lbl)
    {
        g.setColour(lit ? pink : btnIdle);
        g.fillRoundedRectangle(x, y, bw, bh, 4.0f);
        g.setColour(lit ? pink.brighter(0.5f) : outline);
        g.drawRoundedRectangle(x, y, bw, bh, 4.0f, 1.0f);
        g.setColour(lit ? juce::Colours::white : btnText);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(lbl, (int)x, (int)y, (int)bw, (int)bh, juce::Justification::centred);
    };

    // Shoulder buttons sit 6px below the component top and are each 18px tall,
    // with a 2px gap between L2→L1 (and R2→R1), ending 2px above the body.
    const float sW     = 56.0f;
    const float sGap   = 4.0f;
    const float bodyY  = (float)bodyOffset_;          // driven by resized()
    const float shBot  = bodyY - 2.0f;               // bottom of L1/R1 row
    const float shH1   = (shBot - 6.0f) * 0.55f;    // L2/R2 height (~larger trigger)
    const float shH2   = shBot - 6.0f - shH1 - 2.0f; // L1/R1 height
    const float shY1   = 6.0f;
    const float shY2   = shY1 + shH1 + 2.0f;
    drawShoulder(sGap,          shY1, sW, shH1, bit(1), "L2");
    drawShoulder(sGap,          shY2, sW, shH2, bit(0), "L1");
    drawShoulder(w - sW - sGap, shY1, sW, shH1, bit(3), "R2");
    drawShoulder(w - sW - sGap, shY2, sW, shH2, bit(2), "R1");

    // ── Main body ──────────────────────────────────────────────────────────────
    const float bodyH = h - bodyY - 4.0f;
    g.setColour(body);
    g.fillRoundedRectangle(2.0f, bodyY, w - 4.0f, bodyH, 8.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(2.0f, bodyY, w - 4.0f, bodyH, 8.0f, 1.0f);

    // Convenience: absolute Y from a fractional position inside the body
    auto bY = [&](float frac) { return bodyY + bodyH * frac; };

    // ── Left stick — L3 + live dot ─────────────────────────────────────────────
    const float lsX = w * 0.22f;
    const float lsY = bY(0.28f);
    const float lsR = juce::jmin(w * 0.07f, bodyH * 0.12f);
    {
        const bool l3 = bit(4);
        g.setColour(l3 ? pink.withAlpha(0.35f) : stickRim.withAlpha(0.25f));
        g.fillEllipse(lsX - lsR, lsY - lsR, lsR * 2, lsR * 2);
        g.setColour(l3 ? pink : stickRim);
        g.drawEllipse(lsX - lsR, lsY - lsR, lsR * 2, lsR * 2, 1.2f);
        const float dR = lsR * 0.32f;
        // Show mouse-drag position unless physical stick is intentionally active (bypass)
        const float dispLX = (!physicalLeftStickActive_ && draggingStick_ == 0) ? dragNormX_ : leftX_;
        const float dispLY = (!physicalLeftStickActive_ && draggingStick_ == 0) ? dragNormY_ : leftY_;
        g.setColour(stickDot);
        g.fillEllipse(lsX + dispLX * (lsR - dR) - dR,
                      lsY - dispLY * (lsR - dR) - dR,
                      dR * 2, dR * 2);
        // L3 label — shows function context based on swap state
        g.setColour(btnText.withAlpha(0.6f));
        g.setFont(juce::Font(7.5f));
        g.drawText(swapActive ? "PITCH" : "MOD WHL",
                   (int)(lsX - lsR * 1.5f), (int)(lsY + lsR + 1),
                   (int)(lsR * 3), 10, juce::Justification::centred);
    }

    // ── D-pad ──────────────────────────────────────────────────────────────────
    const float dpX  = w * 0.17f;
    const float dpY  = bY(0.70f);
    const float dpR  = juce::jmin(w * 0.042f, bodyH * 0.075f);
    const float dpSp = dpR * 1.95f;

    auto drawDpad = [&](float cx, float cy, bool lit, juce::juce_wchar sym)
    {
        g.setColour(lit ? pink : btnIdle);
        g.fillEllipse(cx - dpR, cy - dpR, dpR * 2, dpR * 2);
        g.setColour(lit ? pink.brighter(0.5f) : outline);
        g.drawEllipse(cx - dpR, cy - dpR, dpR * 2, dpR * 2, 0.8f);
        g.setColour(lit ? juce::Colours::white : btnText);
        g.setFont(juce::Font(dpR * 1.1f));
        g.drawText(juce::String::charToString(sym),
                   (int)(cx - dpR), (int)(cy - dpR),
                   (int)(dpR * 2), (int)(dpR * 2),
                   juce::Justification::centred);
    };
    drawDpad(dpX,        dpY - dpSp, bit(10), 0x25B2);
    drawDpad(dpX,        dpY + dpSp, bit(11), 0x25BC);
    drawDpad(dpX - dpSp, dpY,        bit(12), 0x25C4);
    drawDpad(dpX + dpSp, dpY,        bit(13), 0x25BA);

    // ── Touchpad (centre) ──────────────────────────────────────────────────────
    const float tpW = w * 0.36f;
    const float tpH = bodyH * 0.20f;
    const float tpX = (w - tpW) * 0.5f;
    const float tpY = bY(0.06f);
    g.setColour(btnIdle.brighter(0.15f));
    g.fillRoundedRectangle(tpX, tpY, tpW, tpH, 5.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(tpX, tpY, tpW, tpH, 5.0f, 0.8f);
    g.setFont(juce::Font(8.5f));
    g.setColour(btnText);
    g.drawText("TOUCH", (int)tpX, (int)tpY, (int)tpW, (int)tpH, juce::Justification::centred);

    // ── Options (≡) — bit 14 ──────────────────────────────────────────────────
    const float optX = w * 0.5f - 22.0f;
    const float optY = bY(0.50f);
    const float optR = juce::jmin(w * 0.033f, bodyH * 0.058f);
    {
        const bool lit = bit(14);
        g.setColour(lit ? pink : btnIdle);
        g.fillEllipse(optX - optR, optY - optR, optR * 2, optR * 2);
        g.setColour(lit ? pink.brighter(0.5f) : outline);
        g.drawEllipse(optX - optR, optY - optR, optR * 2, optR * 2, 0.8f);
        g.setColour(lit ? juce::Colours::white : btnText);
        for (int i = 0; i < 3; ++i)
            g.fillRect(juce::Rectangle<float>(optX - optR * 0.6f,
                                              optY - optR * 0.45f + i * optR * 0.42f,
                                              optR * 1.2f, optR * 0.22f));
        g.setFont(juce::Font(7.0f));
        g.setColour(btnText.withAlpha(0.6f));
        g.drawText("OPT", (int)(optX - optR * 1.5f), (int)(optY + optR + 1),
                   (int)(optR * 3), 10, juce::Justification::centred);
    }

    // ── PS button — bit 15 ─────────────────────────────────────────────────────
    const float psX = w * 0.5f + 22.0f;
    const float psY = bY(0.50f);
    const float psR = juce::jmin(w * 0.038f, bodyH * 0.068f);
    {
        const bool lit = bit(15);
        g.setColour(lit ? pink : btnIdle.brighter(0.1f));
        g.fillEllipse(psX - psR, psY - psR, psR * 2, psR * 2);
        g.setColour(lit ? pink.brighter(0.5f) : outline);
        g.drawEllipse(psX - psR, psY - psR, psR * 2, psR * 2, 0.8f);
        g.setColour(lit ? juce::Colours::white : btnText);
        g.setFont(juce::Font(psR * 0.9f, juce::Font::bold));
        g.drawText("PS", (int)(psX - psR), (int)(psY - psR),
                   (int)(psR * 2), (int)(psR * 2), juce::Justification::centred);
        g.setFont(juce::Font(7.0f));
        g.setColour(btnText.withAlpha(0.6f));
        g.drawText("PS", (int)(psX - psR * 1.5f), (int)(psY + psR + 1),
                   (int)(psR * 3), 10, juce::Justification::centred);
    }

    // ── SWAP button — below OPT ───────────────────────────────────────────────
    {
        const float swapX = optX;
        const float swapY = optY + optR * 3.2f;
        const auto  swapClr = swapActive ? juce::Colour(0xFF00CFFF) : btnIdle;
        g.setColour(swapClr);
        g.fillEllipse(swapX - optR, swapY - optR, optR * 2, optR * 2);
        g.setColour(swapActive ? juce::Colour(0xFF00CFFF).brighter(0.5f) : outline);
        g.drawEllipse(swapX - optR, swapY - optR, optR * 2, optR * 2, 0.8f);
        g.setColour(swapActive ? juce::Colours::white : btnText);
        g.setFont(juce::Font(optR * 0.75f, juce::Font::bold));
        g.drawText("SW", (int)(swapX - optR), (int)(swapY - optR),
                   (int)(optR * 2), (int)(optR * 2), juce::Justification::centred);
        g.setFont(juce::Font(7.0f));
        g.setColour((swapActive ? juce::Colour(0xFF00CFFF) : btnText).withAlpha(0.8f));
        g.drawText("SWAP", (int)(swapX - optR * 1.5f), (int)(swapY + optR + 1),
                   (int)(optR * 3), 10, juce::Justification::centred);
        // Store for hit-testing
        const_cast<GamepadDisplayComponent*>(this)->swapBtnCentre_ = { swapX, swapY };
        const_cast<GamepadDisplayComponent*>(this)->ctrlBtnR_       = optR;
    }

    // ── INV button — below PS ─────────────────────────────────────────────────
    {
        const float invX = psX;
        const float invY = psY + optR * 3.2f;
        const auto  invClr = invActive ? juce::Colour(0xFFFF9900) : btnIdle;
        g.setColour(invClr);
        g.fillEllipse(invX - optR, invY - optR, optR * 2, optR * 2);
        g.setColour(invActive ? juce::Colour(0xFFFF9900).brighter(0.5f) : outline);
        g.drawEllipse(invX - optR, invY - optR, optR * 2, optR * 2, 0.8f);
        g.setColour(invActive ? juce::Colours::white : btnText);
        g.setFont(juce::Font(optR * 0.75f, juce::Font::bold));
        g.drawText("INV", (int)(invX - optR), (int)(invY - optR),
                   (int)(optR * 2), (int)(optR * 2), juce::Justification::centred);
        g.setFont(juce::Font(7.0f));
        g.setColour((invActive ? juce::Colour(0xFFFF9900) : btnText).withAlpha(0.8f));
        g.drawText("INV", (int)(invX - optR * 1.5f), (int)(invY + optR + 1),
                   (int)(optR * 3), 10, juce::Justification::centred);
        // Store for hit-testing
        const_cast<GamepadDisplayComponent*>(this)->invBtnCentre_ = { invX, invY };
    }

    // ── Right stick — R3 + live dot ────────────────────────────────────────────
    const float rsX = w * 0.78f;
    const float rsY = bY(0.70f);
    const float rsR = juce::jmin(w * 0.065f, bodyH * 0.115f);
    {
        const bool r3 = bit(5);
        g.setColour(r3 ? pink.withAlpha(0.35f) : stickRim.withAlpha(0.25f));
        g.fillEllipse(rsX - rsR, rsY - rsR, rsR * 2, rsR * 2);
        g.setColour(r3 ? pink : stickRim);
        g.drawEllipse(rsX - rsR, rsY - rsR, rsR * 2, rsR * 2, 1.2f);
        const float dR = rsR * 0.30f;
        // Base: physical position when connected, otherwise mirror JoystickPad
        const float baseRX = connected_ ? rightX_
                           : (proc_ ? proc_->joystickX.load(std::memory_order_relaxed) : rightX_);
        const float baseRY = connected_ ? rightY_
                           : (proc_ ? proc_->joystickY.load(std::memory_order_relaxed) : rightY_);
        // Show mouse-drag position unless physical stick is intentionally active (bypass)
        const float dispRX = (!physicalRightStickActive_ && draggingStick_ == 1) ? dragNormX_ : baseRX;
        const float dispRY = (!physicalRightStickActive_ && draggingStick_ == 1) ? dragNormY_ : baseRY;
        g.setColour(stickDot);
        g.fillEllipse(rsX + dispRX * (rsR - dR) - dR,
                      rsY - dispRY * (rsR - dR) - dR,
                      dR * 2, dR * 2);
        g.setColour(btnText.withAlpha(0.6f));
        g.setFont(juce::Font(7.5f));
        g.drawText(swapActive ? "MOD WHL" : "PITCH",
                   (int)(rsX - rsR * 1.5f), (int)(rsY + rsR + 1),
                   (int)(rsR * 3), 10, juce::Justification::centred);
    }

    // ── Face buttons — classic PS colours ──────────────────────────────────────
    const float fbCX  = w * 0.78f;
    const float fbCY  = bY(0.28f);
    const float fbR   = juce::jmin(w * 0.048f, bodyH * 0.088f);
    const float fbSp  = fbR * 1.95f;

    const auto clrTri = juce::Colour(dim ? 0xFF252840u : 0xFF59C3AFu);  // teal
    const auto clrSq  = juce::Colour(dim ? 0xFF252840u : 0xFFEB70B3u);  // pink
    const auto clrCrc = juce::Colour(dim ? 0xFF252840u : 0xFFE84040u);  // red
    const auto clrCrs = juce::Colour(dim ? 0xFF252840u : 0xFF4499FFu);

    auto drawFace = [&](float cx, float cy, bool lit, juce::Colour litC, juce::juce_wchar sym)
    {
        g.setColour(lit ? litC : btnIdle);
        g.fillEllipse(cx - fbR, cy - fbR, fbR * 2, fbR * 2);
        g.setColour(lit ? litC.brighter(0.5f) : outline);
        g.drawEllipse(cx - fbR, cy - fbR, fbR * 2, fbR * 2, 0.8f);
        g.setColour(lit ? juce::Colours::white : btnText.brighter(0.2f));
        g.setFont(juce::Font(fbR * 1.1f));
        g.drawText(juce::String::charToString(sym),
                   (int)(cx - fbR), (int)(cy - fbR),
                   (int)(fbR * 2), (int)(fbR * 2),
                   juce::Justification::centred);
    };
    drawFace(fbCX,        fbCY - fbSp, bit(8), clrTri, 0x25B3);  // △ top
    drawFace(fbCX - fbSp, fbCY,        bit(7), clrSq,  0x25A1);  // □ left
    drawFace(fbCX + fbSp, fbCY,        bit(9), clrCrc, 0x25CB);  // ○ right
    drawFace(fbCX,        fbCY + fbSp, bit(6), clrCrs, 0x00D7);  // × bottom

    // ── "NO CTRL" overlay when disconnected ────────────────────────────────────
    if (!connected_)
    {
        // Light overlay — sticks remain visible and mouse-interactive
        g.setColour(juce::Colour(0x55131525u));
        g.fillRoundedRectangle(2.0f, bodyY, w - 4.0f, bodyH, 8.0f);
        g.setColour(juce::Colour(0xFF3A3C50u));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText("NO CTRL",
                   (int)tpX, (int)(tpY + tpH * 0.5f - 8),
                   (int)tpW, 16,
                   juce::Justification::centred);

        // Subtle dashed ring around sticks to hint they are mouse-draggable
        if (proc_ != nullptr)
        {
            const float lsX2 = w * 0.22f, lsY2 = bY(0.28f);
            const float lsR2 = juce::jmin(w * 0.07f, bodyH * 0.12f);
            const float rsX2 = w * 0.78f, rsY2 = bY(0.70f);
            const float rsR2 = juce::jmin(w * 0.065f, bodyH * 0.115f);
            g.setColour(juce::Colour(0x66FFFFFF));
            g.drawEllipse(lsX2 - lsR2 * 1.25f, lsY2 - lsR2 * 1.25f,
                          lsR2 * 1.8f, lsR2 * 1.8f, 1.0f);
            g.drawEllipse(rsX2 - rsR2 * 1.25f, rsY2 - rsR2 * 1.25f,
                          rsR2 * 1.8f, rsR2 * 1.8f, 1.0f);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// JoystickPad
// ═══════════════════════════════════════════════════════════════════════════════

JoystickPad::JoystickPad(PluginProcessor& p) : proc_(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    particles_.reserve(128);
    spaceRawImage_ = juce::ImageCache::getFromMemory(BinaryData::SpaceBackground_png,
                                                     BinaryData::SpaceBackground_pngSize);
    startTimerHz(60);
}

void JoystickPad::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    if (w < 2 || h < 2) return;

    // ── Bake spaceBgBaked_ (photo with desaturation + radial vignette) ────────
    // Meteors lose their colours; edges/corners darken toward the bg colour.
    spaceBgBaked_ = juce::Image(juce::Image::ARGB, w, h, true);
    if (spaceRawImage_.isValid())
    {
        juce::Image scaled = spaceRawImage_.rescaled(w, h, juce::Graphics::mediumResamplingQuality);
        const float cx = w * 0.5f, cy = h * 0.5f;
        const float maxR = std::sqrt(cx * cx + cy * cy);
        juce::Image::BitmapData src(scaled,        juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dst(spaceBgBaked_, juce::Image::BitmapData::writeOnly);
        for (int py = 0; py < h; ++py)
        {
            for (int px = 0; px < w; ++px)
            {
                juce::Colour c = src.getPixelColour(px, py);
                // Boost brightness so faint galaxy/stars become visible (blacks stay black)
                const float r0 = juce::jlimit(0.0f, 1.0f, c.getFloatRed()   * 1.8f);
                const float g0 = juce::jlimit(0.0f, 1.0f, c.getFloatGreen() * 1.8f);
                const float b0 = juce::jlimit(0.0f, 1.0f, c.getFloatBlue()  * 1.8f);
                dst.setPixelColour(px, py,
                    juce::Colour((uint8_t)(r0 * 255.0f),
                                 (uint8_t)(g0 * 255.0f),
                                 (uint8_t)(b0 * 255.0f),
                                 (uint8_t)(255.0f)));
            }
        }
    }

    // ── Bake milkyWayCache_ ───────────────────────────────────────────────────
    // Wide, foggy diagonal band — sigmas enlarged for dispersed nebula feel.
    milkyWayCache_ = juce::Image(juce::Image::ARGB, w, h, true);
    {
        juce::Image::BitmapData bmp(milkyWayCache_, juce::Image::BitmapData::writeOnly);
        const float bx0 = 0.0f, by0 = (float)h * 0.35f;
        const float bx1 = (float)w, by1 = (float)h * 0.65f;
        const float lineLen = std::sqrt((bx1 - bx0) * (bx1 - bx0) + (by1 - by0) * (by1 - by0));
        const float nx = -(by1 - by0) / lineLen;
        const float ny =  (bx1 - bx0) / lineLen;
        const float tx = (bx1 - bx0) / lineLen;
        const float ty = (by1 - by0) / lineLen;

        auto gauss = [](float d, float sigma) -> float {
            return std::exp(-(d * d) / (2.0f * sigma * sigma));
        };
        // Wider sigmas = foggy dispersed nebula rather than a sharp stripe
        const float outerSigma = (float)h * 0.80f;
        const float midSigma   = (float)h * 0.38f;
        const float coreSigma  = (float)h * 0.12f;

        for (int py = 0; py < h; ++py)
        {
            for (int px = 0; px < w; ++px)
            {
                const float dx    = (float)px - bx0;
                const float dy    = (float)py - by0;
                const float dist  = std::abs(dx * nx + dy * ny);
                const float along = (dx * tx + dy * ty) / lineLen;

                const float k = juce::MathConstants<float>::twoPi;
                const float density =
                    1.0f
                    + 0.22f * std::sin(along * k * 2.8f + 0.4f)
                    + 0.13f * std::sin(along * k * 5.5f + 1.2f)
                    + 0.07f * std::sin(along * k * 11.3f + 2.6f);

                const float outer = gauss(dist, outerSigma) * 0.18f;
                const float mid   = gauss(dist, midSigma)   * 0.40f;
                const float core  = gauss(dist, coreSigma)  * 0.55f;
                const float brightness = juce::jlimit(0.0f, 1.0f, (outer + mid + core) * density);

                const uint8_t R = (uint8_t)(brightness * 0.84f * 255.0f);
                const uint8_t G = (uint8_t)(brightness * 0.90f * 255.0f);
                const uint8_t B = (uint8_t)(brightness          * 255.0f);
                const uint8_t A = (uint8_t)(juce::jlimit(0.0f, 1.0f, brightness * 1.4f) * 255.0f);
                bmp.setPixelColour(px, py, juce::Colour(R, G, B, A));
            }
        }
    }

    // ── Generate starfield_ ───────────────────────────────────────────────────
    // Two depth layers: 200 background (dim) + 50 foreground (clearly visible).
    starfield_.clear();
    starfield_.reserve(250);
    juce::Random rng(0xDEADBEEF12345678LL);

    // Background layer — subdued but clearly visible, slow drift
    for (int i = 0; i < 200; ++i)
    {
        StarDot s;
        s.x = rng.nextFloat();
        s.y = rng.nextFloat();
        s.r = 0.50f + std::pow(rng.nextFloat(), 2.0f) * 0.7f;  // 0.5–1.2px
        const float broll = std::pow(rng.nextFloat(), 1.5f);
        const uint8_t alf = (uint8_t)(60 + broll * 80);  // 60–140
        if (rng.nextFloat() < 0.88f)
            s.c = juce::Colour((uint8_t)242, (uint8_t)245, (uint8_t)255, alf);
        else
            s.c = juce::Colour((uint8_t)190, (uint8_t)215, (uint8_t)255, alf);
        // Random direction, slow speed (normalized units/frame)
        const float angle = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float speed = 0.00008f + rng.nextFloat() * 0.00012f;  // ~0.03–0.08 px/frame @400px
        s.vx = std::cos(angle) * speed;
        s.vy = std::sin(angle) * speed;
        starfield_.push_back(s);
    }
    // Foreground layer — brighter dots, faster drift
    for (int i = 0; i < 50; ++i)
    {
        StarDot s;
        s.x = rng.nextFloat();
        s.y = rng.nextFloat();
        s.r = 0.90f + rng.nextFloat() * 0.70f;  // 0.9–1.6px
        const float broll = 0.5f + rng.nextFloat() * 0.5f;
        const uint8_t alf = (uint8_t)(150 + broll * 70);  // 150–220
        if (rng.nextFloat() < 0.80f)
            s.c = juce::Colour((uint8_t)248, (uint8_t)250, (uint8_t)255, alf);
        else
            s.c = juce::Colour((uint8_t)200, (uint8_t)220, (uint8_t)255, alf);
        // Random direction, faster speed for depth parallax
        const float angle = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float speed = 0.00022f + rng.nextFloat() * 0.00028f;  // ~0.09–0.20 px/frame @400px
        s.vx = std::cos(angle) * speed;
        s.vy = std::sin(angle) * speed;
        starfield_.push_back(s);
    }
    // Shuffle to random order — Population knob reveals stars in mixed size/brightness order
    for (int i = (int)starfield_.size() - 1; i > 0; --i)
        std::swap(starfield_[i], starfield_[(int)(rng.nextFloat() * (i + 1))]);

    // Bug 1 fix: snap cursor to pad center on resize so it does not render at (0,0)
    // until the first timerCallback fires with valid dimensions.
    displayCx_  = getWidth()  * 0.5f;
    displayCy_  = getHeight() * 0.5f;
    springVelX_ = 0.0f;
    springVelY_ = 0.0f;
}

void JoystickPad::resetGlowPhase() { glowPhase_ = 0.0f; }

void JoystickPad::timerCallback()
{
    // ── Update particles ─────────────────────────────────────────────────────
    for (auto& p : particles_)
    {
        p.x  += p.vx;
        p.y  += p.vy;
        p.vy += 0.045f;          // subtle gravity for gold dust
        p.vx *= 0.97f;
        p.vy *= 0.97f;
        p.life -= p.decay;
        p.size *= 0.975f;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
        [](const JoyParticle& p){ return p.life <= 0.0f || p.size < 0.3f; }),
        particles_.end());

    // ── Compute cursor pixel position ────────────────────────────────────────
    const float w = (float)getWidth();
    const float h = (float)getHeight();
    if (w < 1.0f || h < 1.0f) return;

    const bool lfoXActive = *proc_.apvts.getRawParameterValue("lfoXEnabled") > 0.5f;
    const bool lfoYActive = *proc_.apvts.getRawParameterValue("lfoYEnabled") > 0.5f;
    const bool tcbInvOn = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;
    const float dispX = (lfoXActive || lfoYActive)
        ? proc_.modulatedJoyX_.load(std::memory_order_relaxed)
        : proc_.joystickX.load(std::memory_order_relaxed);
    const float dispY_base = (lfoXActive || lfoYActive)
        ? proc_.modulatedJoyY_.load(std::memory_order_relaxed)
        : proc_.joystickY.load(std::memory_order_relaxed);
    // INV: negate dispY so cursor goes UP when rawY=+1 after 90° CCW rotation.
    // Signal (pitch) routing is unchanged — only display position is affected.
    const float dispY = tcbInvOn ? -dispY_base : dispY_base;

    constexpr float dotR = 7.0f, brdr = 2.0f;
    const float cx = juce::jlimit(dotR + brdr, w - dotR - brdr, (dispX + 1.0f) * 0.5f * w);
    const float cy = juce::jlimit(dotR + brdr, h - dotR - brdr, (1.0f - (dispY + 1.0f) * 0.5f) * h);

    // ── toVisPos: raw pixel → visual pixel (CCW rotation only) ──────────────────
    // Used to keep particles at the same position as the visible cursor dot.
    // Y-flip removed: dispY negation above handles INV direction correction.
    auto toVisPos = [&](float px, float py) -> std::pair<float, float>
    {
        float vx = px, vy = py;
        if (bgRotAngle_ != 0.0f)
        {
            const float rad  = bgRotAngle_ * juce::MathConstants<float>::pi / 180.0f;
            const float cx0  = w * 0.5f, cy0 = h * 0.5f;
            const float ddx  = px - cx0, ddy = py - cy0;
            const float cosA = std::cos(rad), sinA = std::sin(rad);
            vx = cx0 + ddx * cosA + ddy * sinA;
            vy = cy0 - ddx * sinA + ddy * cosA;
        }
        return { vx, vy };
    };

    // ── Spawn gold movement particles ─────────────────────────────────────────
    if (prevCx_ > -999.0f)
    {
        const float dx    = cx - prevCx_;
        const float dy    = cy - prevCy_;
        const float speed = std::sqrt(dx * dx + dy * dy);
        if (speed > 1.2f)
        {
            const auto [gvx, gvy] = toVisPos(cx, cy);
            spawnGoldParticles(gvx, gvy, dx, dy, speed);
        }
    }
    prevCx_ = cx;
    prevCy_ = cy;

    // ── Cursor display position — spring-damper ──────────────────────────────
    // While dragging: snap instantly so the cursor never lags behind the mouse.
    // On release: underdamped spring (ζ≈0.45) snaps back with visible overshoot.
    if (mouseIsDown_)
    {
        displayCx_  = cx;
        displayCy_  = cy;
        springVelX_ = 0.0f;
        springVelY_ = 0.0f;
    }
    else
    {
        constexpr float dt = 1.0f / 60.0f;
        constexpr float k  = 320.0f;
        constexpr float d  = 16.0f;   // ζ = d / (2*sqrt(k)) ≈ 0.45
        const float ax = -k * (displayCx_ - cx) - d * springVelX_;
        const float ay = -k * (displayCy_ - cy) - d * springVelY_;
        springVelX_ += ax * dt;
        springVelY_ += ay * dt;
        displayCx_  += springVelX_ * dt;
        displayCy_  += springVelY_ * dt;
    }

    // Visual position of the spring-smoothed cursor dot (for bursts + hold-glow).
    const auto [partVisX, partVisY] = toVisPos(displayCx_, displayCy_);

    // ── Poll voice note-on bursts ─────────────────────────────────────────────
    static const juce::Colour kVoiceClr[4] = {
        juce::Colour(0xFFFF3333),  // Voice 0 (Root):    Red
        juce::Colour(0xFF3366FF),  // Voice 1 (Third):   Blue
        juce::Colour(0xFF00DD55),  // Voice 2 (Fifth):   Green
        juce::Colour(0xFFFFDD00),  // Voice 3 (Tension): Yellow
    };
    for (int v = 0; v < 4; ++v)
    {
        if (proc_.voiceTriggerFlash_[v].exchange(0, std::memory_order_relaxed) > 0)
            spawnBurst(partVisX, partVisY, kVoiceClr[v], 20);
    }

    // ── Continuous hold-glow: emit soft particles while a gate is open ────────
    // Gentler than the trigger burst: fewer, slower, smaller, longer lifetime.
    // Gives a living ambient glow as long as the note is held.
    {
        static juce::Random rng;
        for (int v = 0; v < 4; ++v)
        {
            if (!proc_.isGateOpen(v)) continue;
            for (int i = 0; i < 2; ++i)
            {
                if (particles_.size() >= 250) break;
                JoyParticle p;
                p.x     = partVisX + rng.nextFloat() * 4.0f - 2.0f;
                p.y     = partVisY + rng.nextFloat() * 4.0f - 2.0f;
                const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float s = rng.nextFloat() * 0.5f + 0.25f;
                p.vx    = std::cos(a) * s;
                p.vy    = std::sin(a) * s;
                p.life  = 1.0f;
                p.decay = 1.0f / (0.6f * 60.0f);
                p.size  = rng.nextFloat() * 0.9f + 0.5f;
                p.color = kVoiceClr[v];
                particles_.push_back(p);
            }
        }
    }

    // ── Cursor breathing animation ────────────────────────────────────────────
    // glowPhase_ (0..1) drives the breathing glow ring in paint().
    // Beat sync: resetGlowPhase() is called by PluginEditor::timerCallback() on beat.
    // Gate-open: hold at peak brightness (full inhale). Resume on release.
    {
        bool anyGateOpen = false;
        for (int v = 0; v < 4; ++v)
            anyGateOpen |= proc_.isGateOpen(v);

        if (anyGateOpen)
            glowPhase_ = 1.0f;  // locked to full glow while a gate is open
        else
        {
            // One glow cycle = one beat at the effective (looper/DAW) BPM
            const float bpm = proc_.getEffectiveBpm();
            glowPhase_ = std::fmod(glowPhase_ + bpm / 3600.0f, 1.0f);
        }
    }

    // ── Animate starfield positions ───────────────────────────────────────────
    for (auto& s : starfield_)
    {
        s.x += s.vx;
        s.y += s.vy;
        // Wrap around edges (normalized 0..1)
        if (s.x < 0.0f) s.x += 1.0f;
        if (s.x > 1.0f) s.x -= 1.0f;
        if (s.y < 0.0f) s.y += 1.0f;
        if (s.y > 1.0f) s.y -= 1.0f;
    }

    // ── Space background fly-through scroll (0.15px/frame) ──────────────────
    if (spaceBgBaked_.isValid())
    {
        const float imgH = (float)spaceBgBaked_.getHeight();
        bgScrollY_ = std::fmod(bgScrollY_ + 0.15f, imgH);
    }

    // ── INV visual rotation — smootherstep ease-in-out (12 s) + spring settle ─
    {
        const bool  invOn     = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;
        const float newTarget = invOn ? 90.0f : 0.0f;

        if (!bgRotInitialized_)
        {
            // Snap to target on first tick so preset load doesn't trigger a full 12 s spin.
            bgRotAngle_        = newTarget;
            bgRotEndAngle_     = newTarget;
            bgRotProgress_     = 1.0f;
            bgRotSpringActive_ = false;
            bgRotInvLast_      = invOn;
            bgRotInitialized_  = true;
        }
        else if (invOn != bgRotInvLast_)
        {
            // INV toggled — start new journey from current visual position.
            bgRotInvLast_      = invOn;
            bgRotStartAngle_   = bgRotAngle_;
            bgRotEndAngle_     = newTarget;
            bgRotProgress_     = 0.0f;
            bgRotSpringActive_ = false;
            bgRotSpringVel_    = 0.0f;
            // Scale duration proportionally: full 12 s for 90°, shorter for mid-anim reversal.
            const float journeyDeg  = std::abs(bgRotEndAngle_ - bgRotStartAngle_);
            const float framesTotal = (journeyDeg / 90.0f) * 600.0f;  // 10 s @ 60 Hz = 600 frames
            bgRotProgressStep_ = (framesTotal > 0.5f) ? (1.0f / framesTotal) : 1.0f;
        }

        if (!bgRotSpringActive_ && bgRotProgress_ < 1.0f)
        {
            // Smootherstep ease-in-out: S(t) = 6t⁵ − 15t⁴ + 10t³
            bgRotProgress_ = juce::jmin(1.0f, bgRotProgress_ + bgRotProgressStep_);
            const float t  = bgRotProgress_;
            const float s  = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            bgRotAngle_    = bgRotStartAngle_ + (bgRotEndAngle_ - bgRotStartAngle_) * s;

            if (bgRotProgress_ >= 1.0f)
            {
                // Journey complete — hand off to spring for a gentle overshoot settle.
                bgRotAngle_        = bgRotEndAngle_;
                bgRotSpringActive_ = true;
                const float dir    = (bgRotEndAngle_ >= bgRotStartAngle_) ? 1.0f : -1.0f;
                bgRotSpringVel_    = dir * 1.5f;    // initial kick → peaks at ≈ 93° overshoot
            }
        }
        else if (bgRotSpringActive_)
        {
            bgRotSpringVel_ += (bgRotEndAngle_ - bgRotAngle_) * 0.06f;
            bgRotSpringVel_ *= 0.78f;
            bgRotAngle_     += bgRotSpringVel_;
            if (std::abs(bgRotSpringVel_) < 0.004f && std::abs(bgRotAngle_ - bgRotEndAngle_) < 0.015f)
            {
                bgRotAngle_        = bgRotEndAngle_;
                bgRotSpringActive_ = false;
            }
        }
    }

    repaint();
}

void JoystickPad::spawnGoldParticles(float cx, float cy, float dx, float dy, float speed)
{
    static juce::Random rng;
    const int count = juce::jmin(6, (int)(speed * 0.55f));
    const juce::Colour gold(0xFFFFCC44);
    const float angle0 = std::atan2(-dy, -dx);  // trail behind movement
    for (int i = 0; i < count; ++i)
    {
        if (particles_.size() >= 200) break;
        JoyParticle p;
        p.x     = cx + rng.nextFloat() * 3.0f - 1.5f;
        p.y     = cy + rng.nextFloat() * 3.0f - 1.5f;
        const float a = angle0 + (rng.nextFloat() - 0.5f) * 1.8f;
        const float s = rng.nextFloat() * 0.9f + 0.2f;
        p.vx    = std::cos(a) * s;
        p.vy    = std::sin(a) * s;
        p.life  = 1.0f;
        p.decay = 1.0f / (0.28f * 60.0f);
        p.size  = rng.nextFloat() * 1.0f + 0.7f;
        p.color = gold;
        particles_.push_back(p);
    }
}

void JoystickPad::spawnBurst(float cx, float cy, juce::Colour color, int count)
{
    static juce::Random rng;
    for (int i = 0; i < count; ++i)
    {
        if (particles_.size() >= 250) break;
        JoyParticle p;
        p.x     = cx + rng.nextFloat() * 5.0f - 2.5f;
        p.y     = cy + rng.nextFloat() * 5.0f - 2.5f;
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float s = rng.nextFloat() * 2.2f + 0.8f;
        p.vx    = std::cos(a) * s;
        p.vy    = std::sin(a) * s;
        p.life  = 1.0f;
        p.decay = 1.0f / (0.48f * 60.0f);
        p.size  = rng.nextFloat() * 1.4f + 1.0f;
        p.color = color;
        particles_.push_back(p);
    }
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

    // INV mode: physical stick X drives logical Y axis and vice versa.
    if (*proc_.apvts.getRawParameterValue("stickInvert") > 0.5f)
        std::swap(nx, ny);
    proc_.joystickX.store(nx);
    proc_.joystickY.store(ny);
    repaint();
}

void JoystickPad::mouseDown(const juce::MouseEvent& e)
{
    mouseIsDown_ = true;
    mousePixX_   = (float)e.x;
    mousePixY_   = (float)e.y;
    updateFromMouse(e);
}

void JoystickPad::mouseDrag(const juce::MouseEvent& e)
{
    mousePixX_ = (float)e.x;
    mousePixY_ = (float)e.y;
    updateFromMouse(e);
}

void JoystickPad::mouseUp(const juce::MouseEvent&)
{
    mouseIsDown_ = false;
}
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
    const int w  = getWidth(), h = getHeight();
    juce::ignoreUnused(w, h);

    // ── Layer 1: Near-black background ───────────────────────────────────────
    g.setColour(juce::Colour(0xff05050f));
    g.fillRect(b);

    // Mid-rotation pulse: bell curve 0→1→0 over the 12 s journey (0 at rest).
    // Used by layers 1.5 and 1.6 for subtle opacity boost and vignette deepening.
    const float bgTransPulse = (bgRotProgress_ < 1.0f)
        ? std::sin(bgRotProgress_ * juce::MathConstants<float>::pi)
        : 0.0f;

    // INV Y-flip state — snaps instantly; applies to compass labels, grid, cursor.
    const bool invOn = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;

    // ── Layer 1.5: Space background photo (scrolling fly-through) ───────────
    if (spaceBgBaked_.isValid())
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(b.toNearestInt());
        // INV visual rotation — rotates CCW around pad centre over 12 s.
        // Scroll continues normally throughout.
        if (bgRotAngle_ != 0.0f)
        {
            const float rad = bgRotAngle_ * juce::MathConstants<float>::pi / 180.0f;
            g.addTransform(juce::AffineTransform::rotation(-rad, b.getCentreX(), b.getCentreY()));
        }
        // Opacity: 0.88 at rest, peaks to 1.0 at mid-rotation — starfield feels more present.
        g.setOpacity(0.88f + bgTransPulse * 0.12f);
        const int imgH  = spaceBgBaked_.getHeight();
        const int offset = (int)bgScrollY_ % imgH;
        g.drawImageAt(spaceBgBaked_, (int)b.getX(), (int)b.getY() - offset);
        g.drawImageAt(spaceBgBaked_, (int)b.getX(), (int)b.getY() - offset + imgH);
    }

    // ── Layer 1.6: Mid-rotation vignette deepening ───────────────────────────
    // Radial gradient darkens the edges during the rotation, fades out at rest.
    if (bgTransPulse > 0.002f)
    {
        juce::Graphics::ScopedSaveState vsave(g);
        const float gradR = juce::jmin(b.getWidth(), b.getHeight()) * 0.62f;
        juce::ColourGradient vig(
            juce::Colour(0x00000000),
            b.getCentreX(), b.getCentreY(),
            juce::Colour(0xFF000000).withAlpha(bgTransPulse * 0.22f),
            b.getCentreX() + gradR, b.getCentreY(),
            true);   // isRadial
        g.setGradientFill(vig);
        g.fillRect(b);
    }

    // ── Layer 2: Milky way band (brightness driven by randomProbability 0-1) ─
    if (milkyWayCache_.isValid())
    {
        const float prob = proc_.apvts.getRawParameterValue("randomProbability")->load();
        const float bandAlpha = juce::jmap(prob, 0.0f, 1.0f, 0.04f, 0.22f);
        juce::Graphics::ScopedSaveState ss(g);
        g.setOpacity(bandAlpha);
        g.drawImage(milkyWayCache_, b);
    }

    // ── Layer 3: Starfield (count driven by randomPopulation 1-64) ───────────
    if (!starfield_.empty())
    {
        const float pop = proc_.apvts.getRawParameterValue("randomPopulation")->load();
        const int visCount = (int)std::floor((float)starfield_.size() * (pop - 1.0f) / 63.0f);
        const int count    = juce::jmin(visCount, (int)starfield_.size());
        for (int i = 0; i < count; ++i)
        {
            const auto& s = starfield_[i];
            g.setColour(s.c);
            g.fillEllipse(s.x * b.getWidth()  - s.r,
                          s.y * b.getHeight() - s.r,
                          s.r * 2.0f, s.r * 2.0f);
        }
    }

    // ── Layer 4: Joystick range circle — cyan glow, BPM-breathing ────────────
    {
        const float circleR = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
        const juce::Rectangle<float> circleRect(
            b.getCentreX() - circleR, b.getCentreY() - circleR,
            circleR * 2.0f, circleR * 2.0f);
        // Breathe with glowPhase_ — same sine as cursor glow
        const float breath = 0.5f + 0.5f * std::sin(glowPhase_ * juce::MathConstants<float>::twoPi);
        // Outer soft glow
        g.setColour(juce::Colour(0xFF00CFFF).withAlpha(0.10f + breath * 0.12f));
        g.drawEllipse(circleRect.expanded(3.0f), 5.0f);
        // Mid glow
        g.setColour(juce::Colour(0xFF00CFFF).withAlpha(0.22f + breath * 0.15f));
        g.drawEllipse(circleRect.expanded(1.0f), 2.5f);
        // Inner crisp rim
        g.setColour(juce::Colour(0xFF00CFFF).withAlpha(0.50f + breath * 0.20f));
        g.drawEllipse(circleRect, 1.5f);
    }

    // ── Layer 4b: Perimeter arc — raw joystick polar angle ────────────────────
    // Reads proc_.joystickX/Y directly (NOT displayCx_/Cy_) so arc always shows
    // the true joystick direction even while the dot is still catching up.
    // Arc span: ±20° (40° total). Color: cyan 0xFF00CFFF at alpha 0.55.
    // Skipped when joystick magnitude < 0.08 (near center, no meaningful direction).
    {
        const float rawX = proc_.joystickX.load(std::memory_order_relaxed);
        const float rawY = proc_.joystickY.load(std::memory_order_relaxed);
        const float mag  = std::sqrt(rawX * rawX + rawY * rawY);

        if (mag >= 0.08f)
        {
            const float arcCx     = b.getCentreX();
            const float arcCy     = b.getCentreY();
            const float arcR      = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
            const juce::Rectangle<float> arcRect(arcCx - arcR, arcCy - arcR, arcR * 2.0f, arcR * 2.0f);

            // atan2(-rawY, rawX): Y negated because joystick +Y = screen up (JUCE Y flipped)
            const float angle     = std::atan2(-rawY, rawX);
            const float spanRad   = juce::degreesToRadians(20.0f);

            juce::Path arc;
            // addArc(x, y, w, h, fromAngle, toAngle, startAsNewSubpath)
            arc.addArc(arcRect.getX(), arcRect.getY(), arcRect.getWidth(), arcRect.getHeight(),
                       angle - spanRad, angle + spanRad, true);

            juce::PathStrokeType stroke(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            g.setColour(juce::Colour(0xFF00CFFF).withAlpha(0.55f));
            g.strokePath(arc, stroke);
        }
    }

    // ── Layer 4c: Note-label compass — Root/Third/Fifth/Tension at cardinals ──
    // Idle alpha 0.38, active (within ±50° of arc angle) alpha 0.75.
    // Positions: Root=bottom, Third=top, Fifth=left, Tension=right.
    // Labels sit ~12px outside the inscribed circle radius.
    {
        const float rawX     = proc_.joystickX.load(std::memory_order_relaxed);
        const float rawY     = proc_.joystickY.load(std::memory_order_relaxed);
        const float mag      = std::sqrt(rawX * rawX + rawY * rawY);
        const float arcAngle = (mag >= 0.08f) ? std::atan2(-rawY, rawX) : -999.0f;

        const float labelR   = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f + 12.0f;
        const float cx0      = b.getCentreX();
        const float cy0      = b.getCentreY();
        const float lw       = 36.0f;   // label bounding box width
        const float lh       = 14.0f;   // label bounding box height

        // Cardinal angles in JUCE screen space (atan2(-rawY, rawX) convention):
        //   Root    = bottom = -PI/2 (joyY=-1, screen down)  → angle = PI/2
        //   Third   = top    = +PI/2 (joyY=+1, screen up)    → angle = -PI/2
        //   Fifth   = left   (joyX=-1)                        → angle = PI
        //   Tension = right  (joyX=+1)                        → angle = 0
        struct LabelDef { const char* name; float angle; };
        const LabelDef labels[4] = {
            { "Root",    juce::MathConstants<float>::pi * 0.5f  },   // bottom
            { "Third",  -juce::MathConstants<float>::pi * 0.5f  },   // top
            { "Fifth",   juce::MathConstants<float>::pi         },   // left
            { "Tension", 0.0f                                   },   // right
        };

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain));

        const float activeThreshRad = juce::degreesToRadians(50.0f);

        for (const auto& lbl : labels)
        {
            // Check angular proximity to arc angle
            bool active = false;
            if (arcAngle > -998.0f)
            {
                float diff = lbl.angle - arcAngle;
                // Normalize diff to -PI..PI
                while (diff >  juce::MathConstants<float>::pi) diff -= juce::MathConstants<float>::twoPi;
                while (diff < -juce::MathConstants<float>::pi) diff += juce::MathConstants<float>::twoPi;
                active = std::abs(diff) <= activeThreshRad;
            }

            const float alpha = active ? 0.75f : 0.38f;
            g.setColour(Clr::text.withAlpha(alpha));

            // Position: label center at labelR from pad center along label's cardinal angle.
            // Offset box so center lands on the cardinal point.
            const float lx = cx0 + std::cos(lbl.angle) * labelR - lw * 0.5f;
            const float ly = cy0 - std::sin(lbl.angle) * labelR - lh * 0.5f;
            g.drawText(lbl.name, juce::Rectangle<float>(lx, ly, lw, lh),
                       juce::Justification::centred, false);
        }
    }

    // ── Layer 5: Semitone grid ────────────────────────────────────────────────
    {
        const int xSemitones = juce::jmax(0, (int)proc_.apvts.getRawParameterValue("joystickXAtten")->load());
        const int ySemitones = juce::jmax(0, (int)proc_.apvts.getRawParameterValue("joystickYAtten")->load());

        // Build 12-bit scale bitmask from APVTS (pre-transposed by globalTranspose)
        const int transpose = (int)proc_.apvts.getRawParameterValue("globalTranspose")->load();
        uint16_t scaleMask = 0;
        for (int n = 0; n < 12; ++n)
        {
            if (proc_.apvts.getRawParameterValue("scaleNote" + juce::String(n))->load() > 0.5f)
                scaleMask |= (uint16_t)(1 << n);
        }

        // Step: cap display density at 48 or 96 semitones
        auto getStep = [](int semis) -> int {
            if (semis <= 48) return 1;
            if (semis <= 96) return 2;
            return 4;
        };

        // Vertical lines — solid, driven by joystickXAtten (X axis semitone range)
        const int xStep = getStep(xSemitones);
        if (xSemitones >= 1)
        {
            for (int i = 0; i < xSemitones; i += xStep)
            {
                const float t     = (float)i / (float)xSemitones;
                const float px    = b.getX() + t * b.getWidth();
                // Center ramp: lines fade toward center (t=0.5), peak at edges (t=0 or 1)
                const float ramp  = 1.0f - (1.0f - std::abs(t * 2.0f - 1.0f)) * 0.6f;
                const int pitchClass = ((i % 12) + transpose) % 12;
                const bool inScale   = ((scaleMask >> pitchClass) & 1) != 0;
                const float alpha    = inScale ? (0.28f * ramp) : (0.10f * ramp);
                const float thick    = inScale ? 1.2f : 0.8f;
                g.setColour(juce::Colours::white.withAlpha(alpha));
                g.drawLine(px, b.getY(), px, b.getBottom(), thick);
            }
        }
        else
        {
            // Default centre vertical line — glow pass then crisp pass
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.drawLine(b.getCentreX(), b.getY(), b.getCentreX(), b.getBottom(), 5.0f);
            g.setColour(juce::Colours::white.withAlpha(0.65f));
            g.drawLine(b.getCentreX(), b.getY(), b.getCentreX(), b.getBottom(), 1.8f);
        }

        // Horizontal lines — dashed, driven by joystickYAtten (Y axis semitone range)
        const int yStep = getStep(ySemitones);
        if (ySemitones >= 1)
        {
            const float dashLengths[] = { 3.0f, 3.0f };
            for (int i = 0; i < ySemitones; i += yStep)
            {
                const float t     = (float)i / (float)ySemitones;
                const float py    = b.getY() + t * b.getHeight();
                const float ramp  = 1.0f - (1.0f - std::abs(t * 2.0f - 1.0f)) * 0.6f;
                const int pitchClass = ((i % 12) + transpose) % 12;
                const bool inScale   = ((scaleMask >> pitchClass) & 1) != 0;
                // Horizontal lines are visually brighter than vertical (X-axis dominant feel — VIS-12)
                const float alpha    = inScale ? (0.38f * ramp) : (0.14f * ramp);
                const float thick    = inScale ? 1.2f : 0.8f;
                g.setColour(juce::Colours::white.withAlpha(alpha));
                g.drawDashedLine(juce::Line<float>(b.getX(), py, b.getRight(), py),
                                 dashLengths, 2, thick);
            }
        }
        else
        {
            // Default centre horizontal line — glow pass then crisp pass
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.drawLine(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY(), 5.0f);
            g.setColour(juce::Colours::white.withAlpha(0.65f));
            g.drawLine(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY(), 1.8f);
        }
    }

    // ── Layer 6: Particles (clipped to pad bounds, no spill) — UNCHANGED ─────
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(getLocalBounds());
        for (const auto& p : particles_)
        {
            if (p.life <= 0.0f) continue;
            const float alpha = std::sqrt(p.life) * 0.88f;
            g.setColour(p.color.withAlpha(alpha));
            g.fillEllipse(p.x - p.size, p.y - p.size, p.size * 2.0f, p.size * 2.0f);
        }
    }

    // ── Cursor position (shared by layers 7a-7c) ─────────────────────────────
    // Phase 32: cx/cy are now the spring-smoothed display positions (displayCx_/Cy_).
    // The spring target (raw LFO-aware pixel position) is computed in timerCallback().
    // Perimeter arc reads raw proc_.joystickX/Y directly below (NOT the smoothed dot).
    // INV visual rotation: cursor orbits CCW in sync with the background over 12 s.
    constexpr float dotR    = 7.0f;
    constexpr float tickLen = 5.0f;
    float cx = displayCx_;
    float cy = displayCy_;
    if (bgRotAngle_ != 0.0f)
    {
        const float rad  = bgRotAngle_ * juce::MathConstants<float>::pi / 180.0f;
        const float cx0  = b.getCentreX();
        const float cy0  = b.getCentreY();
        const float dx   = displayCx_ - cx0;
        const float dy   = displayCy_ - cy0;
        const float cosA = std::cos(rad);
        const float sinA = std::sin(rad);
        // CCW rotation: negate the sin cross-terms relative to CW formula
        cx = cx0 + dx * cosA + dy * sinA;
        cy = cy0 - dx * sinA + dy * cosA;
    }
    // INV cursor direction is handled by dispY negation in timerCallback — no Y-flip here.

    // ── Layer 7a: Static centre reference dot — UNCHANGED ────────────────────
    {
        const float ox = b.getCentreX(), oy = b.getCentreY();
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawEllipse(ox - dotR, oy - dotR, dotR * 2.0f, dotR * 2.0f, 1.5f);
        g.drawLine(ox - dotR - tickLen, oy, ox - dotR - 1.0f, oy, 1.0f);
        g.drawLine(ox + dotR + 1.0f,   oy, ox + dotR + tickLen, oy, 1.0f);
        g.drawLine(ox, oy - dotR - tickLen, ox, oy - dotR - 1.0f, 1.0f);
        g.drawLine(ox, oy + dotR + 1.0f,   ox, oy + dotR + tickLen, 1.0f);
    }

    // ── Layer 7b: Cursor — dark halo ─────────────────────────────────────────
    constexpr float haloR = dotR + 6.0f;
    g.setColour(juce::Colour(0xff05050f).withAlpha(0.45f));
    g.fillEllipse(cx - haloR, cy - haloR, haloR * 2.0f, haloR * 2.0f);

    // ── Layer 7c: Cursor dot + outline + ticks — UNCHANGED ───────────────────
    g.setColour(Clr::highlight);
    g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour(Clr::text);
    g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.5f);
    g.setColour(Clr::text.withAlpha(0.7f));
    g.drawLine(cx - dotR - tickLen, cy, cx - dotR - 1.0f, cy, 1.0f);
    g.drawLine(cx + dotR + 1.0f,   cy, cx + dotR + tickLen, cy, 1.0f);
    g.drawLine(cx, cy - dotR - tickLen, cx, cy - dotR - 1.0f, 1.0f);
    g.drawLine(cx, cy + dotR + 1.0f,   cx, cy + dotR + tickLen, 1.0f);

    // (no border)
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
    if (src == 0)  // TouchPlate: hold-mode-aware
    {
        // Hold mode is inverted: press = mute (note-off), release = note-on (hold resumes).
        if (proc_.padHold_[voice_].load())
            proc_.setPadState(voice_, false);
        else
            proc_.setPadState(voice_, true);
        repaint();
    }
    else if (src == 3)  // RandomHold: touch = trigger (rising edge), no hold-mode inversion
    {
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
    else if (src == 3)  // RandomHold: release = hard-cut
    {
        proc_.setPadState(voice_, false);
    }
    repaint();  // always repaint so dim state stays current
}

void TouchPlate::paint(juce::Graphics& g)
{
    const int  src      = static_cast<int>(
        proc_.apvts.getRawParameterValue("triggerSource" + juce::String(voice_))->load());
    const bool gateOpen = proc_.isGateOpen(voice_);
    const bool arpStep  = (proc_.getArpCurrentVoice() == voice_);
    const bool looperOn = proc_.isLooperVoiceActive(voice_);

    // Per-source base and active colours
    // 0 = Pad (TouchPlate), 1 = Joystick, 2 = RandomFree, 3 = RandomHold
    juce::Colour baseClr, activeClr;
    switch (src)
    {
        case 1:  // Joystick
            baseClr   = juce::Colour(0xFF2D1B4E);
            activeClr = juce::Colour(0xFFB06EFF);
            break;
        case 2:  // Random Free
            baseClr   = juce::Colour(0xFF3D2000);
            activeClr = juce::Colour(0xFFFF9800);
            break;
        case 3:  // Random Hold
            baseClr   = juce::Colour(0xFF1A3D2A);
            activeClr = Clr::gateOn;
            break;
        default: // 0 = Pad
            baseClr   = Clr::gateOff;
            activeClr = Clr::gateOn;
            break;
    }

    juce::Colour fillClr;
    if (arpStep)
        fillClr = juce::Colour(0xFF00C8A8);     // arp step: cyan (overrides all)
    else if (looperOn || gateOpen)
        fillClr = activeClr;
    else
        fillClr = baseClr;

    constexpr float padR = 4.0f;
    g.setColour(fillClr);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), padR);

    g.setColour(Clr::text);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);

    juce::Colour borderClr;
    if (arpStep)
        borderClr = juce::Colour(0xFF00E8C8);
    else if (looperOn || gateOpen)
        borderClr = activeClr.brighter(0.35f);
    else
        borderClr = baseClr.brighter(0.4f);
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
    // Trigger voices in PAD mode (src==0) or RandomHold (src==3).
    for (int v = 0; v < 4; ++v)
    {
        const int src = static_cast<int>(
            proc_.apvts.getRawParameterValue("triggerSource" + juce::String(v))->load());
        if (src == 0)
            proc_.setPadState(v, !proc_.padHold_[v].load());  // hold-mode aware
        else if (src == 3)
            proc_.setPadState(v, true);  // RandomHold: touch = trigger
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
            proc_.setPadState(v, proc_.padHold_[v].load());  // hold-mode aware
        else if (src == 3)
            proc_.setPadState(v, false);  // RandomHold: release = hard-cut
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
        if (src == 0 || src == 3) { anyPadMode = true; break; }
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
    // Two-pass hit test: black keys first (painted on top), then white keys.
    // Prevents the white-key rect from stealing clicks inside a black key's area.
    static const int kBlackNotes[] = { 1, 3, 6, 8, 10 };
    static const int kWhiteNotes[] = { 0, 2, 4, 5, 7, 9, 11 };

    auto tryToggle = [&](int n) -> bool
    {
        if (!noteRect(n).contains(e.position))
            return false;

        // Translate the visual (transposed) key position back to the base pitch class
        const int transpose = static_cast<int>(
            apvts_.getRawParameterValue("globalTranspose")->load());
        const int baseNote = ((n - transpose) % 12 + 12) % 12;

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

        // Toggle the base pitch class corresponding to the clicked visual key
        const bool cur = isNoteActive(baseNote);
        if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                apvts_.getParameter("scaleNote" + juce::String(baseNote))))
            *p = !cur;
        repaint();
        return true;
    };

    for (int n : kBlackNotes) { if (tryToggle(n)) return; }
    for (int n : kWhiteNotes) { if (tryToggle(n)) return; }
}

void ScaleKeyboard::paint(juce::Graphics& g)
{
    // activeScaleMask_ always has the transpose-shifted pitch classes applied,
    // for both preset and custom scales.  Use it for display in all modes.

    // White keys first
    for (int n = 0; n < 12; ++n)
    {
        if (kIsBlack[n]) continue;
        auto r = noteRect(n);

        const bool inScale = (activeScaleMask_ >> n) & 1;
        const juce::Colour fill = inScale ? Clr::gateOn : juce::Colours::white;

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

        const bool inScale = (activeScaleMask_ >> n) & 1;
        const juce::Colour fill = inScale ? Clr::gateOn.darker(0.3f) : juce::Colour(0xFF1A1D2E);

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

    // ── DIMEA logo ────────────────────────────────────────────────────────────
    logoImage_ = juce::ImageCache::getFromMemory(BinaryData::DimeaLogo_png,
                                                 BinaryData::DimeaLogo_pngSize);

    setSize(1120, 840);

    // ── Tooltip window ────────────────────────────────────────────────────────
    addAndMakeVisible(tooltipWindow_);
    tooltipWindow_.setMillisecondsBeforeTipAppears(600);

    // ── Joystick ──────────────────────────────────────────────────────────────
    joystickPad_.setTooltip("XY Pad  -  drag to control pitch: horizontal = 5th/Tension, vertical = Root/Third  |  Double-click to centre  |  Right stick on controller does the same");
    addAndMakeVisible(joystickPad_);
    styleKnob(joyXAttenKnob_); joyXAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14); joyXAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyXAttenLabel_, "SEMITONE X");
    styleKnob(joyYAttenKnob_); joyYAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14); joyYAttenKnob_.setTextValueSuffix(" st"); styleLabel(joyYAttenLabel_, "SEMITONE Y");
    joyXAttenKnob_.setTooltip("SEMITONE X  -  semitone spread for the 5th and Tension voices on the horizontal joystick/right stick axis");
    joyYAttenKnob_.setTooltip("SEMITONE Y  -  semitone spread for the Root and Third voices on the vertical joystick/right stick axis");
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
        padHoldBtn_[v].setTooltip("Hold  -  keep gate open after releasing the pad; click again to close");
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
        padSubOctBtn_[v].setTooltip("Sub Oct  -  also emit a note one octave below this voice");
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
    transposeKnob_.setTooltip("Key  -  global root note (C through B); shifts the scale and all chord pitches");
    thirdIntKnob_.setTooltip("Third  -  semitones above root (4 = major 3rd, 3 = minor 3rd)");
    fifthIntKnob_.setTooltip("Fifth  -  semitones above root (7 = perfect 5th)");
    tensionIntKnob_.setTooltip("Tension  -  upper voice semitones above root (11 = maj 7th, 10 = min 7th, 14 = maj 9th)");
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
    rootOctKnob_   .setTooltip("Root Octave  -  shift Root voice up/down by whole octaves");
    thirdOctKnob_  .setTooltip("Third Octave  -  shift Third voice up/down by whole octaves");
    fifthOctKnob_  .setTooltip("Fifth Octave  -  shift Fifth voice up/down by whole octaves");
    tensionOctKnob_.setTooltip("Tension Octave  -  shift Tension voice up/down by whole octaves");
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
    scalePresetBox_.setTooltip("Scale Preset  -  choose a musical scale; only notes in the scale are reachable by the joystick");
    customScaleToggle_.setTooltip("Custom Scale  -  enable the keyboard below to build your own scale note-by-note");
    scalePresetAtt_ = std::make_unique<ComboAtt>(p.apvts, "scalePreset", scalePresetBox_);
    customScaleAtt_ = std::make_unique<ButtonAtt>(p.apvts, "useCustomScale", customScaleToggle_);

    // ── Trigger sources ───────────────────────────────────────────────────────
    static const juce::String trigLabels[4] = {"Root Trig", "3rd Trig", "5th Trig", "Ten Trig"};
    static const juce::String trigSrcIDs[4] = {
        "triggerSource0","triggerSource1","triggerSource2","triggerSource3" };
    static const juce::String trigTooltips[4] = {
        "Root Trigger  -  how the Root voice fires: Pad touch, Joystick movement, Rnd Free (random, free-running), or Rnd Hold (random, holds until re-triggered)",
        "Third Trigger  -  how the Third voice fires: Pad touch, Joystick movement, Rnd Free, or Rnd Hold",
        "Fifth Trigger  -  how the Fifth voice fires: Pad touch, Joystick movement, Rnd Free, or Rnd Hold",
        "Tension Trigger  -  how the Tension voice fires: Pad touch, Joystick movement, Rnd Free, or Rnd Hold" };

    for (int i = 0; i < 4; ++i)
    {
        trigSrc_[i].addItem("Pad",      1);
        trigSrc_[i].addItem("Joystick", 2);
        trigSrc_[i].addItem("Rnd Free", 3);  // was "Random" — label rename only; saved index 2 loads correctly
        trigSrc_[i].addItem("Rnd Hold", 4);  // new item appended at index 3
        styleCombo(trigSrc_[i]);
        trigSrc_[i].setTooltip(trigTooltips[i]);
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

    styleKnob(randomDensityKnob_);
    randomDensityKnob_.setTooltip("Population  -  gate density in Random trigger mode (0% = min, 100% = max)");
    addAndMakeVisible(randomDensityKnob_);
    randomPopulationAtt_ = std::make_unique<SliderAtt>(p.apvts, "randomPopulation", randomDensityKnob_);
    // Set AFTER attachment so it isn't overwritten by the APVTS range setup
    randomDensityKnob_.textFromValueFunction = [](double v) {
        return juce::String(juce::roundToInt(((v - 1.0) / 63.0) * 100.0)) + "%";
    };
    randomDensityKnob_.valueFromTextFunction = [](const juce::String& s) {
        const double pct = s.retainCharacters("0123456789.").getDoubleValue();
        return 1.0 + (pct / 100.0) * 63.0;
    };
    randomDensityKnob_.updateText();

    styleLabel(randomDensityLabel_, "POPULA");
    randomDensityLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(randomDensityLabel_);

    // ── Probability knob ─────────────────────────────────────────────────────────
    styleKnob(randomProbabilityKnob_);
    randomProbabilityKnob_.setTooltip("Probability  -  per-slot fire chance (0% = never, 100% = always)");
    addAndMakeVisible(randomProbabilityKnob_);
    randomProbabilityAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomProbability"), randomProbabilityKnob_);
    // Set AFTER attachment so it isn't overwritten by the APVTS range setup
    randomProbabilityKnob_.textFromValueFunction = [](double v) {
        return juce::String(juce::roundToInt(v * 100.0)) + "%";
    };
    randomProbabilityKnob_.valueFromTextFunction = [](const juce::String& s) {
        return s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    randomProbabilityKnob_.updateText();

    styleLabel(randomProbabilityLabel_, "PROBABLY");
    randomProbabilityLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(randomProbabilityLabel_);

    styleLabel(randomSubdivLabel_, "RND SYNC");
    addAndMakeVisible(randomSubdivLabel_);
    {
        const juce::StringArray subdivChoices {
            "4/1", "2/1",
            "1/1", "1/1T",
            "1/2", "1/2T",
            "1/4", "1/4T",
            "1/8", "1/8T",
            "1/16", "1/16T",
            "1/32", "1/32T",
            "1/64",
            "2/1T", "4/1T"   // indices 15-16: match APVTS to fix combo/parameter normalization
        };
        for (int v = 0; v < 4; ++v)
        {
            randomSubdivBox_[v].addItemList(subdivChoices, 1);
            randomSubdivBox_[v].setSelectedItemIndex(8, juce::dontSendNotification);
            styleCombo(randomSubdivBox_[v]);
            addAndMakeVisible(randomSubdivBox_[v]);
            randomSubdivAtt_[v] = std::make_unique<juce::ComboBoxParameterAttachment>(
                *p.apvts.getParameter("randomSubdiv" + juce::String(v)), randomSubdivBox_[v]);
        }
    }

    // Sync toggle — compact rectangular button like the looper mode buttons
    randomSyncButton_.setButtonText("RND SYNC");
    randomSyncButton_.setName("round");
    randomSyncButton_.setClickingTogglesState(true);
    randomSyncButton_.setToggleState(true, juce::dontSendNotification);
    randomSyncButton_.setTooltip("Sync  -  ON: triggers fire during DAW playback only  |  OFF: use the Free BPM clock");
    styleButton(randomSyncButton_);
    addAndMakeVisible(randomSyncButton_);
    randomSyncButtonAtt_ = std::make_unique<juce::ButtonParameterAttachment>(
        *p.apvts.getParameter("randomClockSync"), randomSyncButton_);

    // Free tempo knob
    randomFreeTempoKnob_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    randomFreeTempoKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    randomFreeTempoKnob_.setTooltip("Free BPM  -  random trigger rate when DAW Sync is off");
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderFillColourId,  Clr::highlight);
    randomFreeTempoKnob_.setColour(juce::Slider::rotarySliderOutlineColourId, Clr::accent);
    randomFreeTempoKnob_.setColour(juce::Slider::thumbColourId,             Clr::text);
    addAndMakeVisible(randomFreeTempoKnob_);
    randomFreeTempoKnobAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("randomFreeTempo"), randomFreeTempoKnob_);

    // ── Filter attenuators (smaller — grouped visually with their Base knob) ──
    styleKnob(filterXAttenKnob_); filterXAttenKnob_.setTextValueSuffix(" %");
    filterXAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    styleLabel(filterXAttenLabel_, "-/+ MOD X");
    styleKnob(filterYAttenKnob_); filterYAttenKnob_.setTextValueSuffix(" %");
    filterYAttenKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 14);
    styleLabel(filterYAttenLabel_, "-/+ MOD Y");
    filterXAttenKnob_.setTooltip("Cutoff Depth  -  how much left stick X drives CC74 Filter Cutoff away from the base value");
    filterYAttenKnob_.setTooltip("Resonance Depth  -  how much left stick Y drives CC71 Resonance away from the base value");
    addAndMakeVisible(filterXAttenKnob_); addAndMakeVisible(filterXAttenLabel_);
    addAndMakeVisible(filterYAttenKnob_); addAndMakeVisible(filterYAttenLabel_);
    filterXAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXAtten", filterXAttenKnob_);
    filterYAttenAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYAtten", filterYAttenKnob_);

    // ── Filter CC base — resting CC when stick is centred ──────────────────
    styleKnob(filterXOffsetKnob_); filterXOffsetKnob_.setTextValueSuffix("");
    styleLabel(filterXOffsetLabel_, "MOD FIX X");
    styleKnob(filterYOffsetKnob_); filterYOffsetKnob_.setTextValueSuffix("");
    styleLabel(filterYOffsetLabel_, "MOD FIX Y");
    filterXOffsetKnob_.setTooltip("MOD FIX X  -  Center offset for left stick X (CC modes). Tracks joystick live. Resets to 0 when LFO mode is selected.");
    filterYOffsetKnob_.setTooltip("MOD FIX Y  -  Center offset for left stick Y (CC modes). Tracks joystick live. Resets to 0 when LFO mode is selected.");
    addAndMakeVisible(filterXOffsetKnob_); addAndMakeVisible(filterXOffsetLabel_);
    addAndMakeVisible(filterYOffsetKnob_); addAndMakeVisible(filterYOffsetLabel_);
    filterXOffsetAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterXOffset", filterXOffsetKnob_);
    filterYOffsetAtt_ = std::make_unique<SliderAtt>(p.apvts, "filterYOffset", filterYOffsetKnob_);
    filterXOffsetKnob_.onDragStart = [this] { filterXOffsetDragging_ = true;  };
    filterXOffsetKnob_.onDragEnd   = [this] { filterXOffsetDragging_ = false; };
    filterYOffsetKnob_.onDragStart = [this] { filterYOffsetDragging_ = true;  };
    filterYOffsetKnob_.onDragEnd   = [this] { filterYOffsetDragging_ = false; };

    styleLabel(filterXModeLabel_, "Left X");
    filterXModeLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(filterXModeLabel_);
    styleLabel(filterYModeLabel_, "Left Y");
    filterYModeLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(filterYModeLabel_);

    // ── Joystick threshold slider ─────────────────────────────────────────────
    thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider_.setTooltip("Trig Threshold  -  minimum stick displacement needed to fire a note in Joystick trigger mode");
    thresholdSlider_.setColour(juce::Slider::thumbColourId,       Clr::highlight);
    thresholdSlider_.setColour(juce::Slider::trackColourId,       Clr::accent);
    thresholdSlider_.setColour(juce::Slider::backgroundColourId,  Clr::gateOff);
    // addAndMakeVisible deferred — must come after gamepadDisplay_ to paint on top
    thresholdSliderAtt_ = std::make_unique<juce::SliderParameterAttachment>(
        *p.apvts.getParameter("joystickThreshold"), thresholdSlider_);

    // ── Looper ────────────────────────────────────────────────────────────────
    loopPlayBtn_.setButtonText("PLAY");  loopPlayBtn_.setClickingTogglesState(true);
    loopRecBtn_.setButtonText("REC");    loopRecBtn_.setClickingTogglesState(true);
    loopResetBtn_.setButtonText("RST");
    loopDeleteBtn_.setButtonText("DEL");

    styleButton(loopPlayBtn_);  styleButton(loopRecBtn_);
    styleButton(loopResetBtn_); styleButton(loopDeleteBtn_);

    loopPlayBtn_.setTooltip("Play  -  start or stop looper playback (Gamepad: Cross)");
    loopRecBtn_.setTooltip("Rec  -  arm recording; capture begins at next bar boundary when DAW Sync is on (Gamepad: Triangle)");
    loopResetBtn_.setTooltip("Reset  -  return loop playhead to bar 1 (Gamepad: Square)");
    loopDeleteBtn_.setTooltip("Delete  -  erase all recorded loop data (Gamepad: Circle)");

    loopPlayBtn_.onClick  = [this] { proc_.looperStartStop(); };
    loopRecBtn_.onClick   = [this] {
        const bool xArmed = proc_.getLfoXRecState() == LfoRecState::Armed;
        const bool yArmed = proc_.getLfoYRecState() == LfoRecState::Armed;
        if ((xArmed || yArmed) && proc_.looperIsPlaying())
            proc_.triggerLfoInstantCapture();
        else
            proc_.looperRecord();
    };
    loopResetBtn_.onClick = [this] { proc_.looperReset();     };
    loopDeleteBtn_.onClick = [this] { proc_.looperDelete();   };

    addAndMakeVisible(loopPlayBtn_); addAndMakeVisible(loopRecBtn_);
    addAndMakeVisible(loopResetBtn_); addAndMakeVisible(loopDeleteBtn_);

    // New looper mode buttons (Phase 05)
    loopRecJoyBtn_.setButtonText("REC JOY");
    loopRecGatesBtn_.setButtonText("REC GATES");
    loopSyncBtn_.setButtonText("DAW SYNC");
    loopRecJoyBtn_.setTooltip("Rec Joystick  -  capture joystick XY pitch movements into the loop");
    loopRecGatesBtn_.setTooltip("Rec Gates  -  capture pad/trigger gate events into the loop");
    loopSyncBtn_.setTooltip("DAW Sync  -  align loop length and start point to the DAW bar grid");

    loopRecJoyBtn_.setClickingTogglesState(false);
    loopRecGatesBtn_.setClickingTogglesState(false);
    loopSyncBtn_.setClickingTogglesState(true);

    styleButton(loopRecJoyBtn_);
    styleButton(loopRecGatesBtn_);
    styleButton(loopSyncBtn_);

    // Initial colors: both start ON/armed (looper not yet recording)
    loopRecJoyBtn_  .setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
    loopRecGatesBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));

    loopRecJoyBtn_.onClick = [this] {
        proc_.looperSetRecJoy(!proc_.looperIsRecJoy());
    };
    loopRecGatesBtn_.onClick = [this] {
        proc_.looperSetRecGates(!proc_.looperIsRecGates());
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
    loopRecWaitBtn_.setTooltip("Rec on Touch  -  wait for the next pad hit before starting to record");
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
    loopSubdivBox_.setTooltip("Time Signature  -  sets the bar length used by the looper and random triggers");
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
    loopLengthKnob_.setTooltip("Loop Length  -  number of bars to record/loop (1-16)");
    styleLabel(loopLengthLabel_, "Bars");
    addAndMakeVisible(loopLengthKnob_); addAndMakeVisible(loopLengthLabel_);
    loopLengthAtt_ = std::make_unique<SliderAtt>(p.apvts, "looperLength", loopLengthKnob_);

    // ── Quantize mode buttons (Off / Live / Post) ─────────────────────────────
    constexpr int kQuantizeRadioGroup = 1;  // No other radio groups in this editor

    quantizeOffBtn_ .setButtonText("OFF");
    quantizeLiveBtn_.setButtonText("LIVE");
    quantizePostBtn_.setButtonText("POST");
    quantizeOffBtn_ .setTooltip("Quantize Off  -  play loop events exactly as recorded, no grid snapping");
    quantizeLiveBtn_.setTooltip("Quantize Live  -  snap incoming triggers to the nearest grid division in real time");
    quantizePostBtn_.setTooltip("Quantize Post  -  retroactively snap already-recorded loop events to the grid");

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
    quantizeSubdivBox_.addItem("1/1",   1);
    quantizeSubdivBox_.addItem("1/1T",  2);
    quantizeSubdivBox_.addItem("1/2",   3);
    quantizeSubdivBox_.addItem("1/2T",  4);
    quantizeSubdivBox_.addItem("1/4",   5);
    quantizeSubdivBox_.addItem("1/4T",  6);
    quantizeSubdivBox_.addItem("1/8",   7);
    quantizeSubdivBox_.addItem("1/8T",  8);
    quantizeSubdivBox_.addItem("1/16",  9);
    quantizeSubdivBox_.addItem("1/16T", 10);
    quantizeSubdivBox_.addItem("1/32",  11);
    quantizeSubdivBox_.addItem("1/32T", 12);
    styleCombo(quantizeSubdivBox_);
    quantizeSubdivBox_.setTooltip("Quantize Grid  -  subdivision used for live and post quantisation (1/1 to 1/32T)");
    addAndMakeVisible(quantizeSubdivBox_);
    quantizeSubdivAtt_ = std::make_unique<ComboAtt>(p.apvts, "quantizeSubdiv", quantizeSubdivBox_);

    // Gamepad status
    gamepadStatusLabel_.setText("No controller", juce::dontSendNotification);
    gamepadStatusLabel_.setFont(juce::Font(10.0f));
    gamepadStatusLabel_.setJustificationType(juce::Justification::centredLeft);
    gamepadStatusLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(gamepadStatusLabel_);

    // OPTION mode indicator — always visible, grayed out until preset-scroll activates
    optionLabel_.setText("OPTION", juce::dontSendNotification);
    optionLabel_.setFont(juce::Font(10.0f));
    optionLabel_.setJustificationType(juce::Justification::centredLeft);
    optionLabel_.setColour(juce::Label::textColourId, Clr::textDim);
    addAndMakeVisible(optionLabel_);

    // Chord name display — centered in the middle LFO column strip, large bold font
    chordNameLabel_.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 48.0f, juce::Font::bold));
    chordNameLabel_.setJustificationType(juce::Justification::centred);
    chordNameLabel_.setColour(juce::Label::textColourId,       juce::Colour(0xFF131525));  // dark text
    chordNameLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xFFF0F0F8));  // near-white bg
    chordNameLabel_.setColour(juce::Label::outlineColourId,    Clr::highlight);             // pink border
    addAndMakeVisible(chordNameLabel_);
    addAndMakeVisible(gamepadDisplay_);
    gamepadDisplay_.setProcessor(&proc_);  // enables mouse-drag of sticks when no hardware
    addAndMakeVisible(thresholdSlider_);   // must be after gamepadDisplay_ to paint on top

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
    gamepadActiveBtn_.setTooltip("Gamepad Active  -  enable/disable the PS controller for this instance (turn off when using multiple ChordJoystick instances)");
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
    // Color scheme matches DAW SYNC: styleButton() base (Clr::accent), lit = Clr::highlight.
    filterModBtn_.setButtonText("FILTER MOD ON");
    filterModBtn_.setTooltip("Filter Mod  -  enable left stick CC output (cutoff, resonance, etc.)");
    filterModBtn_.setClickingTogglesState(true);
    filterModBtn_.setToggleState(true, juce::dontSendNotification);
    proc_.setFilterModActive(true);
    styleButton(filterModBtn_);
    filterModBtn_.onClick = [this]
    {
        proc_.setFilterModActive(!proc_.isFilterModActive());
    };
    addAndMakeVisible(filterModBtn_);

    // REC MOD WHL button — when ON: looper records and plays back filter/CC movements.
    // When OFF: live stick always drives the filter freely, even during looper playback.
    // Color scheme matches REC GATES / REC JOY: dim green base, bright on recording.
    filterRecBtn_.setButtonText("REC MOD WHL");
    filterRecBtn_.setTooltip("Rec Mod Whl  -  record left stick CC movements into the loop; OFF = filter plays live over loop playback");
    filterRecBtn_.setClickingTogglesState(false);
    styleButton(filterRecBtn_);
    filterRecBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
    filterRecBtn_.onClick = [this]
    {
        proc_.looperSetRecFilter(!proc_.looperIsRecFilter());
    };
    addAndMakeVisible(filterRecBtn_);

    panicBtn_.setButtonText("PANIC!");
    panicBtn_.setTooltip("Panic  -  send All Notes Off + All Sound Off on every MIDI channel; use when notes get stuck");
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
    filterYModeBox_.addItem("LFO Rate (CC76)",  2);
    filterYModeBox_.addItem("Cutoff (CC74)",    3);
    filterYModeBox_.addItem("VCF LFO (CC12)",   4);
    filterYModeBox_.addItem("LFO-Y Freq",       5);
    filterYModeBox_.addItem("LFO-Y Phase",      6);
    filterYModeBox_.addItem("LFO-Y Level",      7);
    filterYModeBox_.addItem("Gate Length",      8);
    filterYModeBox_.setTooltip("Left Stick Y Mode  -  Resonance (CC71), LFO Rate (CC76), Cutoff (CC74), VCF LFO (CC12), LFO-Y Freq/Phase/Level, or Gate Length");
    styleCombo(filterYModeBox_);
    addAndMakeVisible(filterYModeBox_);
    filterYModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterYMode", filterYModeBox_);

    filterXModeBox_.addItem("Cutoff (CC74)",    1);
    filterXModeBox_.addItem("VCF LFO (CC12)",   2);
    filterXModeBox_.addItem("Resonance (CC71)", 3);
    filterXModeBox_.addItem("LFO Rate (CC76)",  4);
    filterXModeBox_.addItem("LFO-X Freq",       5);
    filterXModeBox_.addItem("LFO-X Phase",      6);
    filterXModeBox_.addItem("LFO-X Level",      7);
    filterXModeBox_.addItem("Gate Length",      8);
    filterXModeBox_.setTooltip("Left Stick X Mode  -  Cutoff (CC74), VCF LFO (CC12), Resonance (CC71), LFO Rate (CC76), LFO-X Freq/Phase/Level, or Gate Length");
    styleCombo(filterXModeBox_);
    addAndMakeVisible(filterXModeBox_);
    filterXModeAtt_ = std::make_unique<ComboAtt>(p.apvts, "filterXMode", filterXModeBox_);
    filterXModeBox_.onChange = [this]
    {
        if (filterXModeBox_.getSelectedItemIndex() >= 4)  // LFO / Gate modes — drop MOD FIX to 0
            if (auto* px = proc_.apvts.getParameter("filterXOffset"))
                px->setValueNotifyingHost(0.5f);  // 0.5 = centre of -50..+50 = value 0
    };
    filterYModeBox_.onChange = [this]
    {
        if (filterYModeBox_.getSelectedItemIndex() >= 4)
            if (auto* py = proc_.apvts.getParameter("filterYOffset"))
                py->setValueNotifyingHost(0.5f);
    };

    // ── Filter Mod hint label (bottom-right) ─────────────────────────────────
    filterModHintLabel_.setText(
        "Left stick sends filter CC to your synth. "
        "Filter Mod enables it. "
        "REC MOD WHL records stick moves into the looper. "
        "Turn it OFF for live filter control during playback.",
        juce::dontSendNotification);
    filterModHintLabel_.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::plain));
    filterModHintLabel_.setJustificationType(juce::Justification::topLeft);
    filterModHintLabel_.setColour(juce::Label::textColourId, Clr::textDim.brighter(0.3f));
    addAndMakeVisible(filterModHintLabel_);

    // ── Arpeggiator ───────────────────────────────────────────────────────────
    arpEnabledBtn_.setButtonText("ARP OFF");
    arpEnabledBtn_.setClickingTogglesState(true);
    arpEnabledBtn_.setTooltip("ARP  -  arpeggiate active voices in sequence at the selected rate; hold the chord, let ARP cycle through it");
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
    arpSubdivBox_.setTooltip("ARP Rate  -  time between arpeggiator steps (1/4 to 1/32)");
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
    arpOrderBox_.setTooltip("ARP Order  -  voice cycling direction: Up, Down, Up-Down, or Random");
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
    arpGateTimeKnob_.setTooltip("Gate Length  -  note-on duration as a fraction of the ARP step (0 = held manually, 1 = full step)");
    addAndMakeVisible(arpGateTimeKnob_);
    styleLabel(arpGateTimeLabel_, "GATE LEN");
    addAndMakeVisible(arpGateTimeLabel_);
    arpGateTimeAtt_ = std::make_unique<SliderAtt>(p.apvts, "gateLength", arpGateTimeKnob_);
    arpGateTimeKnob_.onDragStart = [this] { gateDragging_ = true;  };
    arpGateTimeKnob_.onDragEnd   = [this] { gateDragging_ = false; };

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
    lfoXShapeBox_.setTooltip("LFO X Shape  -  waveform: Sine, Triangle, Saw, Square, S&H (sample-and-hold random)");
    addAndMakeVisible(lfoXShapeBox_);
    lfoXShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXWaveform", lfoXShapeBox_);

    // Rate slider (initial state: free mode attached to lfoXRate)
    lfoXRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXRateSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXRateSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXRateSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXRateSlider_.setTooltip("LFO X Rate  -  Hz in free mode, or musical subdivision when Sync is on");
    lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
        return juce::String(v, 2) + " Hz";
    };
    addAndMakeVisible(lfoXRateSlider_);
    if (auto* param = p.apvts.getParameter("lfoXRate"))
        lfoXRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(*param, lfoXRateSlider_, nullptr);
    lfoXRateSlider_.onDragStart = [this] { lfoXRateDragging_ = true;  };
    lfoXRateSlider_.onDragEnd   = [this] { lfoXRateDragging_ = false; lfoXRateAnchor_  = lfoXRateSlider_.getValue(); };

    lfoXSyncSubdivLabel_.setFont(juce::Font(10.0f));
    lfoXSyncSubdivLabel_.setJustificationType(juce::Justification::centredLeft);
    lfoXSyncSubdivLabel_.setColour(juce::Label::textColourId, Clr::highlight);
    addAndMakeVisible(lfoXSyncSubdivLabel_);

    // Phase slider
    lfoXPhaseSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXPhaseSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXPhaseSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXPhaseSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXPhaseSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXPhaseSlider_.setTooltip("LFO X Phase  -  start-phase offset (0-360 deg); shifts the waveform in time");
    addAndMakeVisible(lfoXPhaseSlider_);
    lfoXPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXPhase", lfoXPhaseSlider_);
    lfoXPhaseSlider_.onDragStart = [this] { lfoXPhaseDragging_ = true;  };
    lfoXPhaseSlider_.onDragEnd   = [this] { lfoXPhaseDragging_ = false; lfoXPhaseAnchor_ = lfoXPhaseSlider_.getValue(); };

    // Level slider
    lfoXLevelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXLevelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXLevelSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXLevelSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXLevelSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXLevelSlider_.setTooltip("LFO X Level  -  modulation depth (0 = off, 1.0 = full joystick X range, 2.0 = double)");
    addAndMakeVisible(lfoXLevelSlider_);
    lfoXLevelAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXLevel", lfoXLevelSlider_);
    lfoXLevelSlider_.onDragStart = [this] { lfoXLevelDragging_ = true;  };
    lfoXLevelSlider_.onDragEnd   = [this] { lfoXLevelDragging_ = false; lfoXLevelAnchor_ = lfoXLevelSlider_.getValue(); };

    // Distortion slider
    lfoXDistSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoXDistSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoXDistSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoXDistSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoXDistSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoXDistSlider_.setTooltip("LFO X Dist  -  waveform noise/jitter (0 = clean, 1 = fully random)");
    addAndMakeVisible(lfoXDistSlider_);
    lfoXDistAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoXDistortion", lfoXDistSlider_);

    // CC Dest ComboBox
    {
        static const juce::StringArray ccDests { "Off","CC1 \xe2\x80\x93 Mod Wheel","CC2 \xe2\x80\x93 Breath",
            "CC5 \xe2\x80\x93 Portamento","CC7 \xe2\x80\x93 Volume","CC10 \xe2\x80\x93 Pan",
            "CC11 \xe2\x80\x93 Expression","CC12 \xe2\x80\x93 VCF LFO",
            "CC16 \xe2\x80\x93 GenPurp 1","CC17 \xe2\x80\x93 GenPurp 2",
            "CC71 \xe2\x80\x93 Resonance","CC72 \xe2\x80\x93 Release",
            "CC73 \xe2\x80\x93 Attack","CC74 \xe2\x80\x93 Filter Cut","CC75 \xe2\x80\x93 Decay",
            "CC76 \xe2\x80\x93 Vibrato Rate","CC77 \xe2\x80\x93 Vibrato Depth",
            "CC91 \xe2\x80\x93 Reverb","CC93 \xe2\x80\x93 Chorus" };
        lfoXCcDestBox_.addItemList(ccDests, 1);
        styleCombo(lfoXCcDestBox_);
        lfoXCcDestBox_.setTooltip("LFO X CC Dest  -  route LFO X to a MIDI CC on the filter channel instead of driving joystick X");
        addAndMakeVisible(lfoXCcDestBox_);
        lfoXCcDestAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXCcDest", lfoXCcDestBox_);
    }

    // Sister dest: LFO X output → LFO Y parameter
    lfoXSisterBox_.addItem("Sister: None",  1);
    lfoXSisterBox_.addItem("Sister: Rate",  2);
    lfoXSisterBox_.addItem("Sister: Phase", 3);
    lfoXSisterBox_.addItem("Sister: Level", 4);
    lfoXSisterBox_.addItem("Sister: Dist",  5);
    styleCombo(lfoXSisterBox_);
    lfoXSisterBox_.setTooltip("LFO X Sister  -  route LFO X output to modulate LFO Y's selected parameter");
    addAndMakeVisible(lfoXSisterBox_);
    lfoXSisterAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoXSister", lfoXSisterBox_);

    // Sync button
    lfoXSyncBtn_.setButtonText("SYNC");
    lfoXSyncBtn_.setClickingTogglesState(true);
    styleButton(lfoXSyncBtn_);
    lfoXSyncBtn_.setTooltip("LFO X Sync  -  lock rate to tempo; rate selector shows subdivisions instead of Hz");
    addAndMakeVisible(lfoXSyncBtn_);
    lfoXSyncAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXSync", lfoXSyncBtn_);

    // Sync toggle: swap Rate attachment between free (lfoXRate) and sync (lfoXSubdiv).
    // Bug 1: also called on load to initialise display from saved APVTS state.
    // Bug 2: resets anchor to NaN so timerCallback re-initialises it in the correct range.
    lfoXSyncBtn_.onClick = [this]()
    {
        const bool syncOn = lfoXSyncBtn_.getToggleState();
        lfoXRateAtt_.reset();
        lfoXRateAnchor_ = std::numeric_limits<double>::quiet_NaN();
        if (syncOn)
        {
            lfoXRateSlider_.setRange(0.0, 17.0, 1.0);
            lfoXRateSlider_.setNumDecimalPlacesToDisplay(0);
            lfoXRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                static const char* names[] = {"16/1","8/1","4/1","4/1T","2/1","2/1T",
                                              "1/1","1/1T","1/2","1/4","1/4T",
                                              "1/8","1/16.","1/8T","1/16","1/16T","1/32","1/32T"};
                return names[juce::jlimit(0, 17, (int)std::round(v))];
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
    lfoXSyncBtn_.onClick();  // Bug 1: apply saved sync state on load

    // Enabled: hidden ToggleButton with ButtonAttachment — clickable LED area in paint()
    lfoXEnabledBtn_.setButtonText("ON");
    lfoXEnabledBtn_.setClickingTogglesState(true);
    styleButton(lfoXEnabledBtn_);
    lfoXEnabledBtn_.setTooltip("LFO X  -  enable/disable LFO X; when off, joystick X position is used directly");
    addAndMakeVisible(lfoXEnabledBtn_);
    lfoXEnabledAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXEnabled", lfoXEnabledBtn_);

    lfoXLinkBtn_.setButtonText("JOY");
    lfoXLinkBtn_.setClickingTogglesState(true);
    lfoXLinkBtn_.setToggleState(true, juce::dontSendNotification);
    styleButton(lfoXLinkBtn_);
    lfoXLinkBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOff);
    lfoXLinkBtn_.setTooltip("LFO X Cursor Link  -  when lit, LFO X modulates the joystick cursor and pitch; when dim, LFO X sends CC only");
    addAndMakeVisible(lfoXLinkBtn_);
    lfoXLinkAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoXCursorLink", lfoXLinkBtn_);

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
    lfoYShapeBox_.setTooltip("LFO Y Shape  -  waveform: Sine, Triangle, Saw, Square, S&H (sample-and-hold random)");
    addAndMakeVisible(lfoYShapeBox_);
    lfoYShapeAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoYWaveform", lfoYShapeBox_);

    // Rate slider (initial state: free mode attached to lfoYRate)
    lfoYRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYRateSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYRateSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYRateSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYRateSlider_.setTooltip("LFO Y Rate  -  Hz in free mode, or musical subdivision when Sync is on");
    lfoYRateSlider_.textFromValueFunction = [](double v) -> juce::String {
        return juce::String(v, 2) + " Hz";
    };
    addAndMakeVisible(lfoYRateSlider_);
    if (auto* param = p.apvts.getParameter("lfoYRate"))
        lfoYRateAtt_ = std::make_unique<juce::SliderParameterAttachment>(*param, lfoYRateSlider_, nullptr);
    lfoYRateSlider_.onDragStart = [this] { lfoYRateDragging_ = true;  };
    lfoYRateSlider_.onDragEnd   = [this] { lfoYRateDragging_ = false; lfoYRateAnchor_  = lfoYRateSlider_.getValue(); };

    lfoYSyncSubdivLabel_.setFont(juce::Font(10.0f));
    lfoYSyncSubdivLabel_.setJustificationType(juce::Justification::centredLeft);
    lfoYSyncSubdivLabel_.setColour(juce::Label::textColourId, Clr::highlight);
    addAndMakeVisible(lfoYSyncSubdivLabel_);

    // Phase slider
    lfoYPhaseSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYPhaseSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYPhaseSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYPhaseSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYPhaseSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYPhaseSlider_.setTooltip("LFO Y Phase  -  start-phase offset (0-360 deg); shifts the waveform in time");
    addAndMakeVisible(lfoYPhaseSlider_);
    lfoYPhaseAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYPhase", lfoYPhaseSlider_);
    lfoYPhaseSlider_.onDragStart = [this] { lfoYPhaseDragging_ = true;  };
    lfoYPhaseSlider_.onDragEnd   = [this] { lfoYPhaseDragging_ = false; lfoYPhaseAnchor_ = lfoYPhaseSlider_.getValue(); };

    // Level slider
    lfoYLevelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYLevelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYLevelSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYLevelSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYLevelSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYLevelSlider_.setTooltip("LFO Y Level  -  modulation depth (0 = off, 1.0 = full joystick Y range, 2.0 = double)");
    addAndMakeVisible(lfoYLevelSlider_);
    lfoYLevelAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYLevel", lfoYLevelSlider_);
    lfoYLevelSlider_.onDragStart = [this] { lfoYLevelDragging_ = true;  };
    lfoYLevelSlider_.onDragEnd   = [this] { lfoYLevelDragging_ = false; lfoYLevelAnchor_ = lfoYLevelSlider_.getValue(); };

    // Distortion slider
    lfoYDistSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoYDistSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lfoYDistSlider_.setColour(juce::Slider::thumbColourId,      Clr::highlight);
    lfoYDistSlider_.setColour(juce::Slider::trackColourId,      Clr::accent);
    lfoYDistSlider_.setColour(juce::Slider::backgroundColourId, Clr::gateOff);
    lfoYDistSlider_.setTooltip("LFO Y Dist  -  waveform noise/jitter (0 = clean, 1 = fully random)");
    addAndMakeVisible(lfoYDistSlider_);
    lfoYDistAtt_ = std::make_unique<SliderAtt>(p.apvts, "lfoYDistortion", lfoYDistSlider_);

    // CC Dest ComboBox
    {
        static const juce::StringArray ccDests { "Off","CC1 \xe2\x80\x93 Mod Wheel","CC2 \xe2\x80\x93 Breath",
            "CC5 \xe2\x80\x93 Portamento","CC7 \xe2\x80\x93 Volume","CC10 \xe2\x80\x93 Pan",
            "CC11 \xe2\x80\x93 Expression","CC12 \xe2\x80\x93 VCF LFO",
            "CC16 \xe2\x80\x93 GenPurp 1","CC17 \xe2\x80\x93 GenPurp 2",
            "CC71 \xe2\x80\x93 Resonance","CC72 \xe2\x80\x93 Release",
            "CC73 \xe2\x80\x93 Attack","CC74 \xe2\x80\x93 Filter Cut","CC75 \xe2\x80\x93 Decay",
            "CC76 \xe2\x80\x93 Vibrato Rate","CC77 \xe2\x80\x93 Vibrato Depth",
            "CC91 \xe2\x80\x93 Reverb","CC93 \xe2\x80\x93 Chorus" };
        lfoYCcDestBox_.addItemList(ccDests, 1);
        styleCombo(lfoYCcDestBox_);
        lfoYCcDestBox_.setTooltip("LFO Y CC Dest  -  route LFO Y to a MIDI CC on the filter channel instead of driving joystick Y");
        addAndMakeVisible(lfoYCcDestBox_);
        lfoYCcDestAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoYCcDest", lfoYCcDestBox_);
    }

    // Sister dest: LFO Y output → LFO X parameter
    lfoYSisterBox_.addItem("Sister: None",  1);
    lfoYSisterBox_.addItem("Sister: Rate",  2);
    lfoYSisterBox_.addItem("Sister: Phase", 3);
    lfoYSisterBox_.addItem("Sister: Level", 4);
    lfoYSisterBox_.addItem("Sister: Dist",  5);
    styleCombo(lfoYSisterBox_);
    lfoYSisterBox_.setTooltip("LFO Y Sister  -  route LFO Y output to modulate LFO X's selected parameter");
    addAndMakeVisible(lfoYSisterBox_);
    lfoYSisterAtt_ = std::make_unique<ComboAtt>(p.apvts, "lfoYSister", lfoYSisterBox_);

    // Sync button
    lfoYSyncBtn_.setButtonText("SYNC");
    lfoYSyncBtn_.setClickingTogglesState(true);
    styleButton(lfoYSyncBtn_);
    lfoYSyncBtn_.setTooltip("LFO Y Sync  -  lock rate to tempo; rate selector shows subdivisions instead of Hz");
    addAndMakeVisible(lfoYSyncBtn_);
    lfoYSyncAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoYSync", lfoYSyncBtn_);

    // Sync toggle: swap Rate attachment between free (lfoYRate) and sync (lfoYSubdiv).
    // Bug 1: also called on load to initialise display from saved APVTS state.
    // Bug 2: resets anchor to NaN so timerCallback re-initialises it in the correct range.
    lfoYSyncBtn_.onClick = [this]()
    {
        const bool syncOn = lfoYSyncBtn_.getToggleState();
        lfoYRateAtt_.reset();
        lfoYRateAnchor_ = std::numeric_limits<double>::quiet_NaN();
        if (syncOn)
        {
            lfoYRateSlider_.setRange(0.0, 17.0, 1.0);
            lfoYRateSlider_.setNumDecimalPlacesToDisplay(0);
            lfoYRateSlider_.textFromValueFunction = [](double v) -> juce::String {
                static const char* names[] = {"16/1","8/1","4/1","4/1T","2/1","2/1T",
                                              "1/1","1/1T","1/2","1/4","1/4T",
                                              "1/8","1/16.","1/8T","1/16","1/16T","1/32","1/32T"};
                return names[juce::jlimit(0, 17, (int)std::round(v))];
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
    lfoYSyncBtn_.onClick();  // Bug 1: apply saved sync state on load

    // Enabled: hidden ToggleButton with ButtonAttachment — clickable LED area in paint()
    lfoYEnabledBtn_.setButtonText("ON");
    lfoYEnabledBtn_.setClickingTogglesState(true);
    styleButton(lfoYEnabledBtn_);
    lfoYEnabledBtn_.setTooltip("LFO Y  -  enable/disable LFO Y; when off, joystick Y position is used directly");
    addAndMakeVisible(lfoYEnabledBtn_);
    lfoYEnabledAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoYEnabled", lfoYEnabledBtn_);

    lfoYLinkBtn_.setButtonText("JOY");
    lfoYLinkBtn_.setClickingTogglesState(true);
    lfoYLinkBtn_.setToggleState(true, juce::dontSendNotification);
    styleButton(lfoYLinkBtn_);
    lfoYLinkBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOff);
    lfoYLinkBtn_.setTooltip("LFO Y Cursor Link  -  when lit, LFO Y modulates the joystick cursor and pitch; when dim, LFO Y sends CC only");
    addAndMakeVisible(lfoYLinkBtn_);
    lfoYLinkAtt_ = std::make_unique<ButtonAtt>(p.apvts, "lfoYCursorLink", lfoYLinkBtn_);

    // ── LFO X Recording buttons ────────────────────────────────────────────────
    lfoXArmBtn_.setButtonText("ARM");
    lfoXArmBtn_.setClickingTogglesState(false);  // timerCallback drives toggle state
    lfoXArmBtn_.setTooltip("LFO X Arm  -  arm to record the next joystick X gesture into the loop");
    styleButton(lfoXArmBtn_);
    addAndMakeVisible(lfoXArmBtn_);
    lfoXArmBtn_.onClick = [this]() { proc_.armLfoX(); };

    lfoXClearBtn_.setButtonText("CLR");
    lfoXClearBtn_.setClickingTogglesState(false);
    lfoXClearBtn_.setTooltip("LFO X Clear  -  erase the LFO X loop recording");
    styleButton(lfoXClearBtn_);
    addAndMakeVisible(lfoXClearBtn_);
    lfoXClearBtn_.onClick = [this]() { proc_.clearLfoXRecording(); };

    // ── LFO Y Recording buttons ────────────────────────────────────────────────
    lfoYArmBtn_.setButtonText("ARM");
    lfoYArmBtn_.setClickingTogglesState(false);
    lfoYArmBtn_.setTooltip("LFO Y Arm  -  arm to record the next joystick Y gesture into the loop");
    styleButton(lfoYArmBtn_);
    addAndMakeVisible(lfoYArmBtn_);
    lfoYArmBtn_.onClick = [this]() { proc_.armLfoY(); };

    lfoYClearBtn_.setButtonText("CLR");
    lfoYClearBtn_.setClickingTogglesState(false);
    lfoYClearBtn_.setTooltip("LFO Y Clear  -  erase the LFO Y loop recording");
    styleButton(lfoYClearBtn_);
    addAndMakeVisible(lfoYClearBtn_);
    lfoYClearBtn_.onClick = [this]() { proc_.clearLfoYRecording(); };

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
    area.removeFromLeft(8);  // wider gap between LFO Y and right column (visual division)

    // Remaining right area (joystick + knobs + pads)
    auto right = area;

    dividerX_  = lfoXCol.getX();           // between left col and LFO columns
    dividerX2_ = lfoYCol.getRight() + 4;   // between LFO columns and right column

    // ── RIGHT COLUMN — TRIGGER and MODULATION pinned by absolute Y to align with left column ──
    // TRIGGER top  == RANDOM PARAMS top  (twoBoxY)
    // MODULATION top == LOOPER top       (looperSectionY)

    // Absolute-positioned labels (window-relative, not column-relative)
    filterModHintLabel_.setBounds(0, 0, 0, 0);

    // Save column bottom before routing panel consumes from it
    const int colBottom = right.getBottom();

    // ── Routing panel — footer area, below separator line; row1=dropdowns, row2=status labels ──
    {
        const int rPanelX = right.getX();
        const int rPanelW = right.getWidth();

        // Box top 4px below the separator line (separator at colBottom+9, box at colBottom+14)
        const int boxTop = colBottom + 14;
        const int boxBot = getLocalBounds().reduced(8).getBottom();
        routingBoxBounds_ = juce::Rectangle<int>(rPanelX, boxTop, rPanelW, juce::jmax(0, boxBot - boxTop));

        routingLabel_.setBounds(0, 0, 0, 0);  // hidden — box knockout title serves as label

        // Single row: [routingModeBox_ | voiceChBox_[0..3]] with 8px header clearance
        constexpr int comboH = 18;
        const int ry = boxTop + 8;
        const int modeW = rPanelW * 2 / 5;        // ~40% for mode combo
        const int voiceW = rPanelW - modeW - 2;   // remaining for 4 voice combos
        routingModeBox_.setBounds(rPanelX, ry, modeW, comboH);
        singleChanTargetBox_.setBounds(rPanelX + modeW + 2, ry, voiceW, comboH);
        const int slotW = voiceW / 4;
        for (int v = 0; v < 4; ++v)
            voiceChBox_[v].setBounds(rPanelX + modeW + 2 + v * slotW, ry, slotW - 1, comboH);
        for (int v = 0; v < 4; ++v)
            voiceChLabel_[v].setBounds(0, 0, 0, 0);

        // Status labels — second row inside routing box, below the dropdowns
        const int labelY = ry + comboH + 2;
        constexpr int labelH = 14;
        constexpr int kBattW = 30;   // reserved pixels for battery icon
        gamepadStatusLabel_.setBounds(rPanelX,             labelY, modeW - kBattW, labelH);
        optionLabel_.setBounds       (rPanelX + modeW + 2, labelY, voiceW,         labelH);
    }

    // Reference Y values — mirror left-column anchors (same constants as left column below)
    //   kArpH=84, kLooperArpGap=14, kLooperSectionH=138, kTrigLooperGap=14, kTrigRndH=160
    const int modTopY  = colBottom - 84 - 14 - 130;   // == looperPanelBounds_.getY() (looper drawn box top)
    const int trigTopY = modTopY - 8 - 14 - 160;       // == twoBoxY in left column (modTopY = looperSectionY+8, so -8 corrects)

    // ── TRIGGER Box — top aligned with RANDOM PARAMS top ─────────────────────
    {
        constexpr int kTrigBoxH = 160;   // == kTrigRndH — matches left-column twoBox height
        auto trigBox = juce::Rectangle<int>(right.getX(), trigTopY, right.getWidth(), kTrigBoxH);
        triggerBoxBounds_ = trigBox;

        trigBox.removeFromTop(8);   // clearance for "TRIGGER" knockout label

        // Touchplates + HOLD / SUB-OCT buttons
        {
            auto row = trigBox.removeFromTop(70);
            const int pw = (row.getWidth() - 9) / 4;
            juce::Component* pads[4] = { &padRoot_, &padThird_, &padFifth_, &padTension_ };
            for (int v = 0; v < 4; ++v)
            {
                auto b = row.removeFromLeft(pw);
                pads[v]->setBounds(b);
                auto holdStrip = b.removeFromTop(18).reduced(2, 2);
                auto holdLeft  = holdStrip.removeFromLeft(holdStrip.getWidth() / 2);
                padHoldBtn_[v]   .setBounds(holdLeft);
                padSubOctBtn_[v] .setBounds(holdStrip);
                if (v < 3) row.removeFromLeft(3);
            }
        }

        trigBox.removeFromTop(3);

        // Global ALL pad
        padAll_.setBounds(trigBox.removeFromTop(32));

        trigBox.removeFromTop(18);  // clear space for "QUANTIZE TRIGGER" label above buttons

        // Quantize trigger — [Off][Live][Post][subdiv]
        {
            auto qRow = trigBox.removeFromTop(18);
            constexpr int kDropW = 52;
            constexpr int kGap   = 2;
            const int btnW = (qRow.getWidth() - kDropW - 3 * kGap) / 3;
            quantizeOffBtn_ .setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
            quantizeLiveBtn_.setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
            quantizePostBtn_.setBounds(qRow.removeFromLeft(btnW)); qRow.removeFromLeft(kGap);
            quantizeSubdivBox_.setBounds(qRow);
        }
    }

    // ── MODULATION Box — top aligned with LOOPER top ──────────────────────────
    {
        constexpr int kModH = 228;  // extends to colBottom == arpBlockBounds_.getBottom()
        auto modBox = juce::Rectangle<int>(right.getX(), modTopY, right.getWidth(), kModH);
        modulationBoxBounds_ = modBox;

        modBox.removeFromTop(8);   // clearance for "MODULATION" knockout label

        // Row 1: [GAMEPAD ON][FILTER MOD][REC FILTER][PANIC]
        {
            auto row = modBox.removeFromTop(20);
            const int bW = (row.getWidth() - 3 * 4) / 4;
            gamepadActiveBtn_.setBounds(row.removeFromLeft(bW)); row.removeFromLeft(4);
            filterModBtn_    .setBounds(row.removeFromLeft(bW)); row.removeFromLeft(4);
            filterRecBtn_    .setBounds(row.removeFromLeft(bW)); row.removeFromLeft(4);
            panicBtn_        .setBounds(row);
        }

        modBox.removeFromTop(3);

        // Row 2: 12px clearance for painted "LEFT X" / "LEFT Y" labels above combo boxes
        {
            modBox.removeFromTop(12);
            filterXModeLabel_.setBounds(0, 0, 0, 0);
            filterYModeLabel_.setBounds(0, 0, 0, 0);
        }

        // Row 3: filterXModeBox_ | filterYModeBox_
        {
            auto comboRow = modBox.removeFromTop(22);
            const int hw = comboRow.getWidth() / 2;
            filterXModeBox_.setBounds(comboRow.removeFromLeft(hw).reduced(1, 0));
            filterYModeBox_.setBounds(comboRow.reduced(1, 0));
        }

        modBox.removeFromTop(3);

        // CUTOFF | RESONANCE knob groups — 2-row layout per side:
        // Top row:    [MOD Atten] [Base]   — side-by-side
        // Bottom row: [SEMITONE]           — under the Atten column
        {
            auto row = modBox;
            const int colW = (row.getWidth() - 3) / 2;
            const int rowH = row.getHeight() / 2;   // ~80px per row

            // Left X — CUTOFF
            {
                auto col = row.removeFromLeft(colW); row.removeFromLeft(3);
                filterCutGroupBounds_ = col;
                const int halfW = (col.getWidth() - 2) / 2;
                auto topRow = col.removeFromTop(rowH);
                auto botRow = col;

                // Top row: Atten X (left) | Base X (right)
                auto attenCol = topRow.removeFromLeft(halfW); topRow.removeFromLeft(2);
                auto baseCol  = topRow;
                filterXAttenLabel_ .setBounds(attenCol.removeFromTop(14)); filterXAttenKnob_ .setBounds(attenCol);
                filterXOffsetLabel_.setBounds(baseCol.removeFromTop(14));  filterXOffsetKnob_.setBounds(baseCol);

                // Bottom row: Semitone X — centered in the full X-side width
                joyXAttenLabel_.setBounds(botRow.removeFromTop(14)); joyXAttenKnob_.setBounds(botRow);
            }

            // Left Y — RESONANCE
            {
                auto col = row;
                filterResGroupBounds_ = col;
                const int halfW = (col.getWidth() - 2) / 2;
                auto topRow = col.removeFromTop(rowH);
                auto botRow = col;

                // Top row: Atten Y (left) | Base Y (right)
                auto attenCol = topRow.removeFromLeft(halfW); topRow.removeFromLeft(2);
                auto baseCol  = topRow;
                filterYAttenLabel_ .setBounds(attenCol.removeFromTop(14)); filterYAttenKnob_ .setBounds(attenCol);
                filterYOffsetLabel_.setBounds(baseCol.removeFromTop(14));  filterYOffsetKnob_.setBounds(baseCol);

                // Bottom row: Semitone Y — centered in the full Y-side width
                joyYAttenLabel_.setBounds(botRow.removeFromTop(14)); joyYAttenKnob_.setBounds(botRow);
            }
        }

        // X / Y sub-panel bounds — each half from below the button row to box bottom
        // (8 header + 20 btn row + 3 gap = 31px consumed above)
        {
            const int subTop = modTopY + 31;
            const int subBot = modTopY + kModH;
            const int halfW  = right.getWidth() / 2;
            modXSubBounds_ = juce::Rectangle<int>(right.getX(),          subTop, halfW,                    subBot - subTop);
            modYSubBounds_ = juce::Rectangle<int>(right.getX() + halfW,  subTop, right.getWidth() - halfW, subBot - subTop);
        }
    }

    // ── Joystick pad — top at LFO box top, bottom at quantizer (scale display) bottom ──
    {
        // LFO box top = area.getY() + 4 (lfoXEnabledBtn Y=area.getY()+14, panel expanded -10)
        // Quantizer bottom = area.getY() + 4 + 308 (left col starts at area.getY(), +4 offset, +308 height)
        const int padTopY  = right.getY() + 4;          // == lfoXPanelBounds_.getY()
        const int padBotY  = right.getY() + 4 + 308;    // == quantizerBoxBounds_.getBottom()
        const int padH     = padBotY - padTopY;          // == 308
        const int padSize  = juce::jmin(right.getWidth(), padH);
        const int padX     = right.getX() + (right.getWidth() - padSize) / 2;
        joystickPad_.setBounds(padX, padTopY, padSize, padSize);
    }

    // ── LEFT COLUMN ───────────────────────────────────────────────────────────

    // ── QUANTIZER box ─────────────────────────────────────────────────────────
    // Scale preset + keyboard + 8 knobs in two 2×2 sub-panels (INTERVAL | OCTAVE)
    left.removeFromTop(4);  // align QUANTIZER top with LFO X panel top (lfoXPanelBounds_.getY() = area.getY()+4)
    {
        constexpr int kQuantizerH = 308;
        auto qBox = left.removeFromTop(kQuantizerH);
        quantizerBoxBounds_ = qBox;

        qBox.removeFromTop(8);   // clearance for "QUANTIZER" knockout label on top border

        // Scale preset label + combo — single row
        {
            auto scaleRow = qBox.removeFromTop(22);
            scalePresetLabel_.setBounds(scaleRow.removeFromLeft(84));
            scalePresetBox_  .setBounds(scaleRow);
        }
        qBox.removeFromTop(4);

        // Scale keyboard — taller now that label+combo share one row (saves 22px)
        scaleKeys_.setBounds(qBox.removeFromTop(106));
        qBox.removeFromTop(8);   // gap between keyboard and sub-panels

        // Two sub-panels side by side: INTERVAL (left) | OCTAVE (right)
        const int subW = (qBox.getWidth() - 4) / 2;
        intervalSubBounds_ = qBox.removeFromLeft(subW);
        qBox.removeFromLeft(4);
        octaveSubBounds_ = qBox;

        // INTERVAL SELECT — 2×2: [Transpose | 3rd] top, [5th | Tension] bottom
        {
            auto r = intervalSubBounds_;
            r.removeFromTop(14);  // sub-panel title drawn in paint()
            r.removeFromTop(2);   // gap below title
            const int hw = r.getWidth() / 2;

            auto row1 = r.removeFromTop(70);
            auto c1 = row1.removeFromLeft(hw);
            transposeLabel_.setBounds(c1.removeFromTop(14)); transposeKnob_.setBounds(c1);
            thirdIntLabel_ .setBounds(row1.removeFromTop(14)); thirdIntKnob_.setBounds(row1);

            r.removeFromTop(4);

            auto c3 = r.removeFromLeft(hw);
            fifthIntLabel_   .setBounds(c3.removeFromTop(14)); fifthIntKnob_.setBounds(c3);
            tensionIntLabel_ .setBounds(r.removeFromTop(14));  tensionIntKnob_.setBounds(r);
        }

        // OCTAVE SELECT — 2×2: [Root | 3rd] top, [5th | Tension] bottom
        {
            auto r = octaveSubBounds_;
            r.removeFromTop(14);  // sub-panel title drawn in paint()
            r.removeFromTop(2);   // gap below title
            const int hw = r.getWidth() / 2;

            auto row1 = r.removeFromTop(70);
            auto c1 = row1.removeFromLeft(hw);
            rootOctLabel_   .setBounds(c1.removeFromTop(14)); rootOctKnob_.setBounds(c1);
            thirdOctLabel_  .setBounds(row1.removeFromTop(14)); thirdOctKnob_.setBounds(row1);

            r.removeFromTop(4);

            auto c3 = r.removeFromLeft(hw);
            fifthOctLabel_   .setBounds(c3.removeFromTop(14)); fifthOctKnob_.setBounds(c3);
            tensionOctLabel_ .setBounds(r.removeFromTop(14));  tensionOctKnob_.setBounds(r);
        }
    }

    left.removeFromTop(6);

    // ── ARP / LOOPER / TRIGGER SELECTION — all anchored from the bottom up ─────
    // Arp block: anchored to bottom of left column
    constexpr int kArpH = 84;
    arpBlockBounds_ = juce::Rectangle<int>(
        left.getX(), left.getBottom() - kArpH, left.getWidth(), kArpH);

    // Looper: fixed height, anchored just above ARP
    constexpr int kLooperSectionH = 138;  // 14px header clearance + 124px controls
    constexpr int kLooperArpGap   = 14;   // gives ~4px between looper panel bottom and arp top
    const int looperSectionY = arpBlockBounds_.getY() - kLooperArpGap - kLooperSectionH;
    {
        auto section = juce::Rectangle<int>(left.getX(), looperSectionY, left.getWidth(), kLooperSectionH);
        section.removeFromTop(14);  // clearance for "LOOPER" knockout label

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

        looperPositionBarBounds_ = {};  // replaced by perimeter bar — no strip layout needed
        section.removeFromTop(8);       // preserve 8px gap so controls below keep their position

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

        // Looper panel bounds — spans all looper controls (play row → subdiv/length row)
        {
            const int looperTop    = loopPlayBtn_.getY();
            const int looperBottom = juce::jmax(loopLengthKnob_.getBottom(), loopSubdivBox_.getBottom());
            looperPanelBounds_ = juce::Rectangle<int>(
                left.getX(), looperTop - 6, left.getWidth(), looperBottom - looperTop + 10);
        }
    }

    // TRIGGER SELECTION + RANDOM PARAMS: anchored just above LOOPER
    constexpr int kTrigRndH      = 160;
    constexpr int kTrigLooperGap = 14;  // keeps LOOPER knockout label clear of box border
    const int twoBoxY = looperSectionY - kTrigLooperGap - kTrigRndH;
    {
        auto twoBoxRow = juce::Rectangle<int>(left.getX(), twoBoxY, left.getWidth(), kTrigRndH);

        const int halfW = (twoBoxRow.getWidth() - 4) / 2;
        auto trigArea = twoBoxRow.removeFromLeft(halfW);
        twoBoxRow.removeFromLeft(4);
        auto rndArea  = twoBoxRow;

        trigSelBounds_   = trigArea;
        randomBoxBounds_ = rndArea;

        // TRIGGER SELECTION: 4 stacked rows — [trigger source combo | subdiv combo]
        {
            auto r = trigArea;
            r.removeFromTop(14);  // panel label clearance
            const int trigW = (r.getWidth() * 2) / 3;
            for (int v = 0; v < 4; ++v)
            {
                trigSrcLabel_[v].setBounds(r.removeFromTop(10));
                auto row = r.removeFromTop(22);
                trigSrc_[v]         .setBounds(row.removeFromLeft(trigW));
                row.removeFromLeft(4);
                randomSubdivBox_[v] .setBounds(row);
                if (v < 3) r.removeFromTop(4);
            }
        }

        // RANDOM PARAMS: [Pop | Prob] top row, [RND SYNC | FREE BPM + Tempo label] bottom row
        {
            auto r = rndArea;
            r.removeFromTop(14);  // panel label clearance
            const int hw = r.getWidth() / 2;

            // Row 1: Population (left) | Probability (right) — label above, knob below (matches Transpose size)
            auto row1 = r.removeFromTop(70);
            {
                auto leftHalf = row1.removeFromLeft(hw);
                randomDensityLabel_.setBounds(leftHalf.removeFromTop(14).reduced(2, 0));
                randomDensityKnob_ .setBounds(leftHalf.reduced(2, 0));
            }
            {
                auto rightHalf = row1;
                randomProbabilityLabel_.setBounds(rightHalf.removeFromTop(14).reduced(2, 0));
                randomProbabilityKnob_ .setBounds(rightHalf.reduced(2, 0));
            }

            r.removeFromTop(4);

            // Row 2: RND SYNC (CLR-style, centred on knob) | FREE BPM knob (right)
            auto row2 = r.removeFromTop(72);
            auto syncHalf = row2.removeFromLeft(hw);
            auto freeTempoHalf = row2;
            const auto knobBounds = freeTempoHalf.removeFromTop(56).reduced(2, 2);
            randomFreeTempoKnob_.setBounds(knobBounds);
            // CLR-style button vertically centred on the tempo knob, label 2px below
            constexpr int btnH = 22;
            const int btnY = knobBounds.getCentreY() - btnH / 2;
            randomSyncButton_.setBounds(syncHalf.getX() + 2, btnY, syncHalf.getWidth() - 4, btnH);
            randomSubdivLabel_.setBounds(syncHalf.getX() + 2, btnY + btnH + 2,
                                         syncHalf.getWidth() - 4, 12);
            freeTempoHalf.removeFromTop(2);
            bpmDisplayLabel_.setBounds(freeTempoHalf.removeFromTop(12));
        }
    }

    // Arpeggiator block — bottom-left panel
    // Layout: [ARP ON/OFF button full-width]
    //         [RATE label | ORDER label | GATE% label]
    //         [RATE combo | ORDER combo | GATE slider]
    {
        auto r = arpBlockBounds_;
        r.removeFromTop(14);  // clearance for drawLfoPanel knockout label on top border
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

        // Row 0: ON button — full width (bigger than before; JOY moved to Row 6)
        lfoXEnabledBtn_.setBounds(col.removeFromTop(26));
        col.removeFromTop(4);

        // Row 1: Shape ComboBox (full width)
        lfoXShapeBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1b: CC Dest ComboBox
        lfoXCcDestBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1c: Sister dest ComboBox
        lfoXSisterBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 2: Rate slider (left 34px = label, right 40px = sync subdiv label when active)
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoXSyncSubdivLabel_.setBounds(row.removeFromRight(40));
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

        // Row 6: SYNC button (left half) + JOY cursor-link toggle (right half)
        // Waveform visualiser moves to the new space below the REC button.
        {
            auto row = col.removeFromTop(22);
            const int btnW = (row.getWidth() - 4) / 2;
            lfoXSyncBtn_.setBounds(row.removeFromLeft(btnW));
            row.removeFromLeft(4);
            lfoXLinkBtn_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 7: REC button (wider, ~70%) + CLR (remaining ~30%)
        {
            auto row = col.removeFromTop(26);
            const int clrW = row.getWidth() / 3;
            lfoXArmBtn_ .setBounds(row.removeFromLeft(row.getWidth() - clrW - 3));
            row.removeFromLeft(3);
            lfoXClearBtn_.setBounds(row);
        }
        col.removeFromTop(4);

        // Oscilloscope: fills remaining space down to quantizer bottom (spaceship style)
        {
            const int oscBottom = quantizerBoxBounds_.getBottom();
            const int oscH = juce::jmax(0, oscBottom - col.getY());
            lfoXVisBounds_ = col.removeFromTop(oscH);
        }

        // Panel bounds: from ON button top to quantizer bottom (no overflow below).
        lfoXPanelBounds_ = lfoXEnabledBtn_.getBounds()
                              .getUnion(lfoXVisBounds_)
                              .withX(lfoXCol.getX())
                              .withWidth(lfoXCol.getWidth())
                              .expanded(0, 10)
                              .withBottom(quantizerBoxBounds_.getBottom());
    }

    // ── LFO Y panel layout ─────────────────────────────────────────────────────
    {
        auto col = lfoYCol;

        col.removeFromTop(14);  // clear header area

        // Row 0: ON button — full width (bigger than before; JOY moved to Row 6)
        lfoYEnabledBtn_.setBounds(col.removeFromTop(26));
        col.removeFromTop(4);

        // Row 1: Shape ComboBox (full width)
        lfoYShapeBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1b: CC Dest ComboBox
        lfoYCcDestBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 1c: Sister dest ComboBox
        lfoYSisterBox_.setBounds(col.removeFromTop(22));
        col.removeFromTop(4);

        // Row 2: Rate slider (left 34px = label, right 40px = sync subdiv label when active)
        {
            auto row = col.removeFromTop(18);
            row.removeFromLeft(34);
            lfoYSyncSubdivLabel_.setBounds(row.removeFromRight(40));
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

        // Row 6: SYNC button (left half) + JOY cursor-link toggle (right half)
        // Waveform visualiser moves to the new space below the REC button.
        {
            auto row = col.removeFromTop(22);
            const int btnW = (row.getWidth() - 4) / 2;
            lfoYSyncBtn_.setBounds(row.removeFromLeft(btnW));
            row.removeFromLeft(4);
            lfoYLinkBtn_.setBounds(row);
        }
        col.removeFromTop(4);

        // Row 7: REC button (wider, ~70%) + CLR (remaining ~30%)
        {
            auto row = col.removeFromTop(26);
            const int clrW = row.getWidth() / 3;
            lfoYArmBtn_ .setBounds(row.removeFromLeft(row.getWidth() - clrW - 3));
            row.removeFromLeft(3);
            lfoYClearBtn_.setBounds(row);
        }
        col.removeFromTop(4);

        // Oscilloscope: fills remaining space down to quantizer bottom (spaceship style)
        {
            const int oscBottom = quantizerBoxBounds_.getBottom();
            const int oscH = juce::jmax(0, oscBottom - col.getY());
            lfoYVisBounds_ = col.removeFromTop(oscH);
        }

        // Panel bounds: from ON button top to quantizer bottom (no overflow below).
        lfoYPanelBounds_ = lfoYEnabledBtn_.getBounds()
                              .getUnion(lfoYVisBounds_)
                              .withX(lfoYCol.getX())
                              .withWidth(lfoYCol.getWidth())
                              .expanded(0, 10)
                              .withBottom(quantizerBoxBounds_.getBottom());
    }
    // ── Chord name label + Gamepad display ───────────────────────────────────
    // Chord display: fixed 50px, sitting just above the gamepad (below LFO boxes, below logo).
    // Logo: from trigSelBounds_.getY() down to chord display top (drawn in paint()).
    // Gamepad display fills remaining space down to the arpeggiator bottom.
    {
        const int chordX = lfoXCol.getX();
        const int chordW = lfoYCol.getRight() - chordX;   // lfoX + gap + lfoY = 304px
        // Component top at the DEL/ALL button row (shoulder buttons float in logo space above body).
        const int gpY = loopDeleteBtn_.getY();
        // Chord display: fixed height, bottom anchored at randomBoxBounds_ bottom (same
        // reference as before the LFO redesign). Independent of LFO panel size.
        constexpr int kChordH = 64;
        chordNameLabel_.setBounds(chordX, randomBoxBounds_.getBottom() - kChordH,
                                  chordW, kChordH);
        gamepadDisplay_.setBounds(chordX, gpY,
                                  chordW, arpBlockBounds_.getBottom() - gpY);
        // Body rectangle aligned with GAMEPAD ON button row.
        gamepadDisplay_.setBodyOffset(gamepadActiveBtn_.getY() - gamepadDisplay_.getY());
        // Threshold slider: centred under right joystick (rsX=78%, rsY=bY(0.70))
        {
            const int bodyOffsetPx = gamepadActiveBtn_.getY() - gpY;
            const int bodyHPx      = arpBlockBounds_.getBottom() - (gpY + bodyOffsetPx) - 4;
            const float rsR        = juce::jmin(chordW * 0.065f, bodyHPx * 0.115f) * 1.6f;
            const int rsX_abs      = chordX + juce::roundToInt(chordW * 0.78f);
            const int rsY_abs      = gpY + bodyOffsetPx + juce::roundToInt(bodyHPx * 0.70f);

            constexpr int kTrackH  = 7;
            const int sliderW      = juce::roundToInt(chordW * 0.26f);
            const int sliderX      = rsX_abs - sliderW / 2;
            const int sliderY      = rsY_abs + (int)rsR + 4;

            // Align vertical center of threshold track with vertical center of gate length knob
            const int alignedY = arpGateTimeKnob_.getY()
                                 + (arpGateTimeKnob_.getHeight() - kTrackH) / 2;
            thresholdSlider_.setBounds(sliderX, alignedY, sliderW, kTrackH);
        }
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
    g.drawText("Arcade Chord Control (BETA-Test)", header, juce::Justification::centred);

    // Outer border
    g.setColour(Clr::accent.brighter(0.5f));
    g.drawRect(getLocalBounds().reduced(2), 1);

    // Vertical dividers — left col | LFO cols | right col
    if (joystickPad_.isVisible())
    {
        g.setColour(Clr::accent.withAlpha(0.5f));
        g.drawLine((float)(dividerX_ - 4),  32.0f, (float)(dividerX_ - 4),  (float)getHeight() - 8, 1.0f);
        g.drawLine((float)dividerX2_,        32.0f, (float)dividerX2_,        (float)getHeight() - 8, 1.0f);
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

    // ── Logo — top aligned with Trigger box, bottom at chord display top ──
    if (logoImage_.isValid() && !quantizerBoxBounds_.isEmpty())
    {
        const int logoY1 = trigSelBounds_.getY();
        const int logoY2 = chordNameLabel_.getY();
        if (logoY2 > logoY1)
        {
            const auto logoDest = juce::Rectangle<float>((float)dividerX_, (float)logoY1,
                                                          (float)(dividerX2_ - dividerX_),
                                                          (float)(logoY2 - logoY1));

            // Scale to fill zone width, maintain aspect ratio, center vertically.
            const float aspect = (float)logoImage_.getWidth() / (float)logoImage_.getHeight();
            float drawW = logoDest.getWidth();
            float drawH = drawW / aspect;
            if (drawH > logoDest.getHeight())
            {
                drawH = logoDest.getHeight();
                drawW = drawH * aspect;
            }
            const float drawX = logoDest.getX() + (logoDest.getWidth()  - drawW) * 0.5f;
            const float drawY = logoDest.getY() + (logoDest.getHeight() - drawH) * 0.5f;
            const auto imageRect = juce::Rectangle<float>(drawX, drawY, drawW, drawH);

            // 1. Draw PNG (dim the baked-in slogan slightly so the bright overlay dominates)
            g.setOpacity(0.85f);
            g.drawImage(logoImage_, imageRect, juce::RectanglePlacement::stretchToFit);
            g.setOpacity(1.0f);

            // 2. Seamless fade: top and bottom edges blend into background
            const float fadeH = logoDest.getHeight() * 0.30f;

            juce::ColourGradient topFade(Clr::bg, 0.0f, logoDest.getY(),
                                         Clr::bg.withAlpha(0.0f), 0.0f, logoDest.getY() + fadeH,
                                         false);
            g.setGradientFill(topFade);
            g.fillRect(juce::Rectangle<float>(logoDest.getX(), logoDest.getY(),
                                              logoDest.getWidth(), fadeH));

            juce::ColourGradient botFade(Clr::bg.withAlpha(0.0f), 0.0f, logoDest.getBottom() - fadeH,
                                         Clr::bg, 0.0f, logoDest.getBottom(),
                                         false);
            g.setGradientFill(botFade);
            g.fillRect(juce::Rectangle<float>(logoDest.getX(), logoDest.getBottom() - fadeH,
                                              logoDest.getWidth(), fadeH));

            // 3. Slogan overlay — full middle-row width, brand pink colour.
            {
                const float sloganTop = imageRect.getY() + imageRect.getHeight() * 0.820f;
                const float sloganH   = imageRect.getHeight() * 0.110f;
                const float fontSize  = juce::jlimit(7.0f, 16.0f, sloganH * 0.85f);

                // Span the full middle-row width, not just the image rect
                const auto sloganRect = juce::Rectangle<float>(
                    logoDest.getX(), sloganTop, logoDest.getWidth(), sloganH);

                const auto sloganFont = juce::Font(
                    juce::Font::getDefaultSansSerifFontName(), fontSize, juce::Font::plain)
                    .withExtraKerningFactor(0.28f);

                // Glow pass — soft cyan halo matching logo underline
                g.setFont(sloganFont);
                g.setColour(juce::Colour(0xFF00D8FF).withAlpha(0.30f));
                g.drawText("BRINGING HARMONY TO THE PEOPLE",
                           sloganRect.expanded(1.5f, 1.5f),
                           juce::Justification::centred, false);

                // Main pass — cyan matching logo underline
                g.setColour(juce::Colour(0xFF00D8FF).withAlpha(0.95f));
                g.drawText("BRINGING HARMONY TO THE PEOPLE",
                           sloganRect, juce::Justification::centred, false);
            }
        }
    }

    // ── Filter knob group panels ─────────────────────────────────────────────
    // Drawn behind child components to create a visual grouping for each filter pair.
    // Arpeggiator panel — drawn after drawLfoPanel is defined (see below)

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

    // Right-column panels omitted: the floating title fights with drawAbove labels
    // and the button/label text already identifies those sections clearly.

    // ── LFO panel drawing (also used for Looper box) ──────────────────────────
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

    drawLfoPanel(quantizerBoxBounds_,  "QUANTIZER");
    drawLfoPanel(trigSelBounds_,       "TRIGGER SELECTION");
    drawLfoPanel(randomBoxBounds_,     "RANDOM PARAMS");
    drawLfoPanel(lfoXPanelBounds_,    "LFO X");
    drawLfoPanel(lfoYPanelBounds_,    "LFO Y");
    drawLfoPanel(looperPanelBounds_,  "LOOPER");
    drawLfoPanel(arpBlockBounds_,     "ARPEGGIATOR");
    drawLfoPanel(triggerBoxBounds_,    "TRIGGER");
    drawLfoPanel(modulationBoxBounds_, "MODULATION");
    drawLfoPanel(routingBoxBounds_,    "ROUTING");

    // ── Battery icon — drawn right of gamepad status label ────────────────────
    if (batteryLevel_ >= -1)
    {
        const auto lb  = gamepadStatusLabel_.getBounds();
        const float bx = (float)(lb.getRight() + 4);
        const float by = (float)(lb.getY() + (lb.getHeight() - 10) / 2);
        constexpr float bw = 19.0f;
        constexpr float bh = 10.0f;

        // Outline colour based on highest lit stripe
        juce::Colour bClr;
        int litCount = 0;
        switch (batteryLevel_)
        {
            case 0:  bClr = juce::Colour(0xFFFF3333); litCount = 0; break; // empty
            case 1:  bClr = juce::Colour(0xFFFF3333); litCount = 1; break; // low → red
            case 2:  bClr = juce::Colour(0xFFFF8800); litCount = 2; break; // medium → orange
            case 3:  bClr = Clr::gateOn;              litCount = 3; break; // full → green
            case 4:  bClr = juce::Colour(0xFF00D8FF); litCount = 3; break; // wired → cyan
            default: bClr = Clr::textDim;             litCount = 0; break; // unknown / -2
        }

        if (batteryLevel_ != -2) // don't draw icon if no controller
        {
            // Body outline
            g.setColour(bClr.withAlpha(0.25f));
            g.fillRoundedRectangle(bx, by, bw, bh, 2.0f);
            g.setColour(bClr.withAlpha(0.75f));
            g.drawRoundedRectangle(bx + 0.5f, by + 0.5f, bw - 1.0f, bh - 1.0f, 2.0f, 1.0f);

            // Nib (right side, positive terminal)
            g.setColour(bClr.withAlpha(0.65f));
            g.fillRoundedRectangle(bx + bw, by + bh * 0.3f, 3.0f, bh * 0.4f, 1.0f);

            if (batteryLevel_ == -1)
            {
                // Unknown: "?" centred
                g.setColour(Clr::textDim);
                g.setFont(juce::Font(7.0f));
                g.drawText("?", (int)bx, (int)by, (int)bw, (int)bh, juce::Justification::centred);
            }
            else
            {
                // 3 vertical stripe blocks
                constexpr float innerX = 2.0f;   // padding inside body
                constexpr float innerY = 2.0f;
                constexpr float stripeW = 4.0f;
                constexpr float stripeH = 6.0f;  // bh - 2*innerY
                constexpr float stripeGap = 1.0f;

                for (int i = 0; i < 3; ++i)
                {
                    const float sx = bx + innerX + i * (stripeW + stripeGap);
                    const float sy = by + innerY;
                    const juce::Colour sc = (i < litCount) ? bClr : Clr::textDim.withAlpha(0.4f);
                    g.setColour(sc);
                    g.fillRoundedRectangle(sx, sy, stripeW, stripeH, 1.0f);
                }
            }
        }
    }

    // INTERVAL and OCTAVE sub-panels inside QUANTIZER box
    auto drawSubPanel = [&](juce::Rectangle<int> bounds, const juce::String& title)
    {
        if (bounds.isEmpty()) return;
        const auto fb = bounds.toFloat().reduced(0.5f);
        g.setColour(Clr::panel.brighter(0.05f));
        g.fillRoundedRectangle(fb, 4.0f);
        g.setColour(Clr::accent.withAlpha(0.35f));
        g.drawRoundedRectangle(fb.reduced(0.25f), 4.0f, 0.75f);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::bold));
        g.setColour(Clr::text.withAlpha(0.85f));
        g.drawText(title, (int)fb.getX(), (int)fb.getY() + 2, (int)fb.getWidth(), 12,
                   juce::Justification::centred);
    };
    drawSubPanel(intervalSubBounds_, "INTERVAL");
    drawSubPanel(octaveSubBounds_,   "OCTAVE");

    // X and Y sub-panels inside MODULATION box (drawn after main box, before divider)
    // Titles omitted — red "LEFT X" / "LEFT Y" labels above the combo boxes serve as headers
    {
        auto drawSubPanelNoTitle = [&](juce::Rectangle<int> bounds)
        {
            if (bounds.isEmpty()) return;
            const auto fb = bounds.toFloat().reduced(0.5f);
            g.setColour(Clr::panel.brighter(0.05f));
            g.fillRoundedRectangle(fb, 4.0f);
            g.setColour(Clr::accent.withAlpha(0.35f));
            g.drawRoundedRectangle(fb.reduced(0.25f), 4.0f, 0.75f);
        };
        drawSubPanelNoTitle(modXSubBounds_);
        drawSubPanelNoTitle(modYSubBounds_);
    }

    // Vertical dashed divider in MODULATION box — separates X (left) from Y (right)
    if (!filterCutGroupBounds_.isEmpty() && !filterResGroupBounds_.isEmpty())
    {
        const float divX = (filterCutGroupBounds_.getRight() + filterResGroupBounds_.getX()) * 0.5f;
        const float y1   = (float)modulationBoxBounds_.getY()      + 8.0f;
        const float y2   = (float)modulationBoxBounds_.getBottom() - 4.0f;
        juce::Path linePath, dashedPath;
        linePath.startNewSubPath(divX, y1);
        linePath.lineTo(divX, y2);
        const float dashes[] = { 5.0f, 3.5f };
        juce::PathStrokeType(1.0f).createDashedStroke(dashedPath, linePath, dashes, 2);
        g.setColour(Clr::accent.brighter(0.5f).withAlpha(0.55f));
        g.fillPath(dashedPath);
    }

    // ── Y / X axis divider ─ splits top row (Y axis: Transpose+3rd) from bottom (X axis: 5th+Tension)
    if (!intervalSubBounds_.isEmpty() && transposeKnob_.getHeight() > 0)
    {
        const float divY   = (float)(transposeKnob_.getBottom() + 2);
        const float lineX1 = (float)intervalSubBounds_.getX()   + 5.0f;
        const float lineX2 = (float)octaveSubBounds_.getRight()  - 5.0f;

        // Dashed horizontal line
        {
            juce::Path linePath, dashedPath;
            linePath.startNewSubPath(lineX1, divY);
            linePath.lineTo(lineX2, divY);
            const float dashes[] = { 5.0f, 3.5f };
            juce::PathStrokeType(1.0f).createDashedStroke(dashedPath, linePath, dashes, 2);
            g.setColour(Clr::accent.brighter(0.5f).withAlpha(0.55f));
            g.fillPath(dashedPath);
        }

        // "Y" label just above the line (left end) — marks the Y-axis (Joystick Y) knobs
        // "X" label just below the line (left end) — marks the X-axis (Joystick X) knobs
        // In INV mode the axes are swapped so the labels must reflect that.
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.setColour(Clr::highlight.withAlpha(0.70f));
        {
            const bool invOn = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;
            g.drawText(invOn ? "X" : "Y", (int)lineX1, (int)divY - 9, 10, 9, juce::Justification::left);
            g.drawText(invOn ? "Y" : "X", (int)lineX1, (int)divY + 2, 10, 9, juce::Justification::left);
        }
    }

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

    // ── LFO waveform visualizers ──────────────────────────────────────────────
    // Spaceship-style oscilloscope below the REC button in each LFO panel.
    // Ring buffer holds kLfoVisBufSize samples; oldest sample is at writeIdx.
    // Values are raw process() output in [-level, +level].
    auto drawLfoVisualizer = [&](juce::Rectangle<int> bounds,
                                 const std::array<float, kLfoVisBufSize>& buf,
                                 int writeIdx)
    {
        if (bounds.isEmpty()) return;
        const auto fb = bounds.toFloat().reduced(1.5f);
        if (fb.getHeight() < 4.0f) return;

        // ── Background — deep space blue-black ──────────────────────────────
        g.setColour(juce::Colour(0xff020310));
        g.fillRoundedRectangle(fb, 4.0f);

        // Subtle border — dim cyan
        g.setColour(juce::Colour(0xff1a4060).withAlpha(0.7f));
        g.drawRoundedRectangle(fb.reduced(0.5f), 4.0f, 0.75f);

        // ── Grid — horizontal thirds + vertical quarters ─────────────────────
        const float cy  = fb.getCentreY();
        const float gy1 = fb.getY()  + fb.getHeight() * 0.333f;
        const float gy2 = fb.getBottom() - fb.getHeight() * 0.333f;
        const juce::Colour gridCol = juce::Colour(0xff0a3050).withAlpha(0.6f);
        // Centre line slightly brighter
        g.setColour(juce::Colour(0xff0d4a70).withAlpha(0.8f));
        g.drawHorizontalLine((int)cy, fb.getX() + 2.0f, fb.getRight() - 2.0f);
        g.setColour(gridCol);
        g.drawHorizontalLine((int)gy1, fb.getX() + 2.0f, fb.getRight() - 2.0f);
        g.drawHorizontalLine((int)gy2, fb.getX() + 2.0f, fb.getRight() - 2.0f);
        // Vertical quarter lines
        for (int qi = 1; qi <= 3; ++qi)
        {
            const float vx = fb.getX() + fb.getWidth() * (float)qi * 0.25f;
            g.drawVerticalLine((int)vx, fb.getY() + 2.0f, fb.getBottom() - 2.0f);
        }

        // ── Waveform ─────────────────────────────────────────────────────────
        // Scale fixed at ±1.0: level=0 → flat line, level=1 → full height.
        const float ampH = (fb.getHeight() * 0.5f) - 2.0f;

        // Blink: positive half-cycle = bright, negative = dim
        const int newestIdx = (writeIdx - 1 + kLfoVisBufSize) % kLfoVisBufSize;
        const float newestVal = buf[newestIdx];
        const float blink_t   = juce::jmax(0.0f, juce::jmin(1.0f, newestVal));
        const float lineAlpha = 0.30f + 0.70f * blink_t;

        const float xStep = fb.getWidth() / (float)(kLfoVisBufSize - 1);

        // Glow pass — thick, dim cyan
        {
            juce::Path wavePath;
            bool started = false;
            for (int i = 0; i < kLfoVisBufSize; ++i)
            {
                const int sIdx = (writeIdx + i) % kLfoVisBufSize;
                const float sNorm = juce::jlimit(-1.0f, 1.0f, buf[sIdx]);
                const float px = fb.getX() + (float)i * xStep;
                const float py = cy - sNorm * ampH;
                if (!started) { wavePath.startNewSubPath(px, py); started = true; }
                else           { wavePath.lineTo(px, py); }
            }
            g.setColour(juce::Colour(0xff00c8e8).withAlpha(lineAlpha * 0.25f));
            juce::PathStrokeType stroke(4.0f);
            stroke.createStrokedPath(wavePath, wavePath);
            g.fillPath(wavePath);
        }

        // Crisp pass — thin, bright cyan
        {
            juce::Path wavePath;
            bool started = false;
            for (int i = 0; i < kLfoVisBufSize; ++i)
            {
                const int sIdx = (writeIdx + i) % kLfoVisBufSize;
                const float sNorm = juce::jlimit(-1.0f, 1.0f, buf[sIdx]);
                const float px = fb.getX() + (float)i * xStep;
                const float py = cy - sNorm * ampH;
                if (!started) { wavePath.startNewSubPath(px, py); started = true; }
                else           { wavePath.lineTo(px, py); }
            }
            g.setColour(juce::Colour(0xff00e8ff).withAlpha(lineAlpha));
            g.strokePath(wavePath, juce::PathStrokeType(1.0f));
        }
    };

    if (!lfoXVisBounds_.isEmpty())
        drawLfoVisualizer(lfoXVisBounds_, lfoXVisBuf_, lfoXVisBufIdx_);
    if (!lfoYVisBounds_.isEmpty())
        drawLfoVisualizer(lfoYVisBounds_, lfoYVisBuf_, lfoYVisBufIdx_);

    // ── Looper perimeter bar ──────────────────────────────────────────────────
    // Clockwise circuit of looperPanelBounds_ perimeter. 2px stroke, Clr::gateOn.
    // Ghost ring shown when stopped; animated bar shown when playing or recording.
    if (!looperPanelBounds_.isEmpty())
    {
        const auto b = looperPanelBounds_.toFloat().reduced(1.0f); // inset so 2px stroke stays inside border
        const float W = b.getWidth();
        const float H = b.getHeight();
        const float totalPerim = 2.0f * (W + H);

        // Maps a perimeter distance [0, totalPerim) to an (x,y) point clockwise from top-left.
        // Handles negative distances and wrapping correctly.
        auto perimPoint = [&](float dist) -> juce::Point<float>
        {
            dist = std::fmod(dist, totalPerim);
            if (dist < 0.0f) dist += totalPerim;
            if (dist < W)             return { b.getX() + dist,        b.getY() };          // top: left→right
            dist -= W;
            if (dist < H)             return { b.getRight(),            b.getY() + dist };   // right: top→bottom
            dist -= H;
            if (dist < W)             return { b.getRight() - dist,     b.getBottom() };     // bottom: right→left
            dist -= W;
            return                           { b.getX(),                b.getBottom() - dist }; // left: bottom→top
        };

        // Compute label exclusion zone — mirrors drawLfoPanel constants for "LOOPER"
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
        const int labelW = g.getCurrentFont().getStringWidth("LOOPER") + 10;
        const int labelH = 14;
        const int labelX = (int)b.getX() + 6;
        const int labelY = (int)b.getY() - labelH / 2; // = b.getY() - 7

        const juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::PathStrokeType ghostStroke(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::Rectangle<int> labelExclude(labelX, labelY, labelW, labelH);

        const bool isActive = proc_.looperIsPlaying() || proc_.looperIsRecording();

        if (!isActive)
        {
            // Ghost ring: full perimeter at low alpha, 1px stroke
            juce::Path ghostPath;
            constexpr int kGhostSegs = 40;
            ghostPath.startNewSubPath(perimPoint(0.0f));
            for (int i = 1; i <= kGhostSegs; ++i)
                ghostPath.lineTo(perimPoint(totalPerim * i / kGhostSegs));
            ghostPath.closeSubPath();

            juce::Graphics::ScopedSaveState ss(g);
            g.excludeClipRegion(labelExclude);
            g.setColour(Clr::gateOff.brighter(0.3f));
            g.strokePath(ghostPath, ghostStroke);
        }
        else
        {
            // Animated perimeter bar with tail
            const double len  = proc_.getLooperLengthBeats();
            const double beat = proc_.getLooperPlaybackBeat();
            if (len > 0.0)
            {
                const float fraction = juce::jlimit(0.0f, 1.0f, static_cast<float>(beat / len));
                const float d = fraction * totalPerim; // leading-edge distance along perimeter

                constexpr float kTailPx = 40.0f;       // tail length in perimeter pixels
                constexpr float kHeadPx = 12.0f;        // "hot" head segment drawn at full alpha

                // Draw tail segment at reduced alpha
                {
                    juce::Path tailPath;
                    constexpr int kTailSegs = 16;
                    tailPath.startNewSubPath(perimPoint(d - kTailPx));
                    for (int i = 1; i <= kTailSegs; ++i)
                        tailPath.lineTo(perimPoint(d - kTailPx + kTailPx * i / kTailSegs));

                    juce::Graphics::ScopedSaveState ss(g);
                    g.excludeClipRegion(labelExclude);
                    g.setColour(Clr::gateOn.withAlpha(0.28f));
                    g.strokePath(tailPath, stroke);
                }

                // Overdraw head segment at full alpha
                {
                    juce::Path headPath;
                    constexpr int kHeadSegs = 6;
                    headPath.startNewSubPath(perimPoint(d - kHeadPx));
                    for (int i = 1; i <= kHeadSegs; ++i)
                        headPath.lineTo(perimPoint(d - kHeadPx + kHeadPx * i / kHeadSegs));

                    juce::Graphics::ScopedSaveState ss(g);
                    g.excludeClipRegion(labelExclude);
                    g.setColour(Clr::gateOn);
                    g.strokePath(headPath, stroke);
                }
            }
        }
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
    // "LEFT X" / "LEFT Y" — labels follow their component's position.
    // Physical bounds swap in timerCallback (INV toggle) handles visual exchange.
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
    g.setColour(Clr::highlight.withAlpha(0.70f));
    if (filterYModeBox_.isVisible())
        g.drawText("LEFT Y", filterYModeBox_.getX(), filterYModeBox_.getY() - 12,
                   filterYModeBox_.getWidth(), 12, juce::Justification::centred);
    if (filterXModeBox_.isVisible())
        g.drawText("LEFT X", filterXModeBox_.getX(), filterXModeBox_.getY() - 12,
                   filterXModeBox_.getWidth(), 12, juce::Justification::centred);

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

        // Separator line (left column)
        const float sepY = (float)footer.getY() + 1.0f;
        g.setColour(Clr::accent.withAlpha(0.5f));
        g.drawLine((float)footer.getX(), sepY, (float)footer.getRight(), sepY, 1.0f);
        // Same separator line in middle (LFO) and right columns
        g.drawLine((float)dividerX_,       sepY, (float)(dividerX2_ - 4), sepY, 1.0f);
        g.drawLine((float)(dividerX2_ + 4), sepY, (float)(getWidth() - 8), sepY, 1.0f);

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
        const int rw = dividerX2_ - rx - 4;  // constrained to LFO columns only
        int ry = getHeight() - 55;  // identical to first left-footer row Y

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain));
        g.setColour(Clr::textDim.withAlpha(0.8f));  // same white as left footer

        const juce::String hintRows[] = {
            "Left stick sends filter CC to your synth.",
            "Filter Mod enables it. REC MOD WHL records into looper.",
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
    // ── INV label swaps ───────────────────────────────────────────────────────
    {
        const bool invOn = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;
        filterXModeLabel_.setText(invOn ? "Left Y" : "Left X", juce::dontSendNotification);
        filterYModeLabel_.setText(invOn ? "Left X" : "Left Y", juce::dontSendNotification);
        joyXAttenLabel_.setText("SEMITONE X", juce::dontSendNotification);
        joyYAttenLabel_.setText("SEMITONE Y", juce::dontSendNotification);

        // INV attachment swap — fires only when INV state changes, not every timer tick.
        if (invOn != prevInvState_)
        {
            prevInvState_ = invOn;

            filterXModeAtt_.reset();
            filterYModeAtt_.reset();
            filterXAttenAtt_.reset();
            filterYAttenAtt_.reset();

            if (invOn)
            {
                // Physical stick X now drives logical Y axis — cross-wire the attachments.
                filterXModeAtt_  = std::make_unique<ComboAtt>(proc_.apvts, "filterYMode", filterXModeBox_);
                filterYModeAtt_  = std::make_unique<ComboAtt>(proc_.apvts, "filterXMode", filterYModeBox_);
                filterXAttenAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "filterYAtten", filterXAttenKnob_);
                filterYAttenAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "filterXAtten", filterYAttenKnob_);
            }
            else
            {
                filterXModeAtt_  = std::make_unique<ComboAtt>(proc_.apvts, "filterXMode", filterXModeBox_);
                filterYModeAtt_  = std::make_unique<ComboAtt>(proc_.apvts, "filterYMode", filterYModeBox_);
                filterXAttenAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "filterXAtten", filterXAttenKnob_);
                filterYAttenAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "filterYAtten", filterYAttenKnob_);
            }

            // Physically swap LEFT X and LEFT Y column positions so the rows
            // exchange sides when INV is toggled. Repaint updates all paint()-drawn labels.
            auto swapBounds = [](juce::Component& a, juce::Component& b) {
                auto tmp = a.getBounds();
                a.setBounds(b.getBounds());
                b.setBounds(tmp);
            };
            swapBounds(filterXModeBox_,     filterYModeBox_);
            swapBounds(filterXAttenLabel_,  filterYAttenLabel_);
            swapBounds(filterXAttenKnob_,   filterYAttenKnob_);
            swapBounds(filterXOffsetLabel_, filterYOffsetLabel_);
            swapBounds(filterXOffsetKnob_,  filterYOffsetKnob_);
            swapBounds(joyXAttenLabel_,     joyYAttenLabel_);
            swapBounds(joyXAttenKnob_,      joyYAttenKnob_);
            std::swap(filterCutGroupBounds_, filterResGroupBounds_);
            repaint();
        }
    }

    // Routing panel visibility
    const bool isSingle = (*proc_.apvts.getRawParameterValue("singleChanMode") > 0.5f);
    singleChanTargetBox_.setVisible(isSingle);
    for (int v = 0; v < 4; ++v)
    {
        voiceChLabel_[v].setVisible(!isSingle);
        voiceChBox_[v].setVisible(!isSingle);
    }

    // Battery level — polled at 30 Hz; triggers repaint of routing box area
    const int newBatt = proc_.getGamepad().isConnected()
                            ? proc_.getGamepad().getBatteryLevel()
                            : -2;
    if (newBatt != batteryLevel_)
    {
        batteryLevel_ = newBatt;
        if (!routingBoxBounds_.isEmpty())
            repaint(routingBoxBounds_);
    }

    // Partial repaint for looper perimeter bar (driven by this 30 Hz timer)
    // Only call repaint() if the panel area is valid — avoids repainting the full editor.
    if (!looperPanelBounds_.isEmpty())
        repaint(looperPanelBounds_);

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

    // ── LFO X recording UI ────────────────────────────────────────────────────
    {
        const LfoRecState xState = proc_.getLfoXRecState();

        // REC button visual: Unarmed=off, Armed=dim "ARM", Recording=bright "REC", Playback=dim "REC"
        lfoXArmBlinkCounter_ = 0;
        lfoXArmBtn_.setToggleState(false, juce::dontSendNotification);
        if (xState == LfoRecState::Unarmed)
        {
            lfoXArmBtn_.setButtonText("REC");
            lfoXArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOff);
        }
        else if (xState == LfoRecState::Armed)
        {
            lfoXArmBtn_.setButtonText("ARM");
            lfoXArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
        }
        else if (xState == LfoRecState::Recording)
        {
            lfoXArmBtn_.setButtonText("REC");
            lfoXArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn);
        }
        else  // Playback — dim lit to signal "recorded, playing back"
        {
            lfoXArmBtn_.setButtonText("REC");
            lfoXArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
        }

        // ARM button enabled only when LFO X is ON.
        // If LFO X just went enabled→disabled while Armed/Recording/Playback,
        // call clearRecording() so state snaps back to Unarmed immediately.
        const bool lfoXOn = (*proc_.apvts.getRawParameterValue("lfoXEnabled") > 0.5f);
        if (prevLfoXOn_ && !lfoXOn) {
            proc_.clearLfoXRecording();
            proc_.resetLfoXPhase();   // reset phase on ON→OFF so next enable starts clean
        }
        prevLfoXOn_ = lfoXOn;
        lfoXArmBtn_.setEnabled(lfoXOn);

        // Push LFO X output into visualizer ring buffer (drawn in paint())
        {
            const float xOut = proc_.lfoXOutputDisplay_.load(std::memory_order_relaxed);
            lfoXVisBuf_[lfoXVisBufIdx_] = xOut;
            lfoXVisBufIdx_ = (lfoXVisBufIdx_ + 1) % kLfoVisBufSize;
        }

        // Control grayout during Playback.
        // setEnabled() blocks interaction; setAlpha() provides the visual dim because
        // PixelLookAndFeel::drawLinearSlider and drawButtonBackground do not check
        // isEnabled() for color — JUCE's default alpha fade does not apply here.
        const bool xPlayback = (xState == LfoRecState::Playback);
        const float xDimAlpha = xPlayback ? 0.35f : 1.0f;
        lfoXShapeBox_   .setEnabled(!xPlayback);  lfoXShapeBox_   .setAlpha(xDimAlpha);
        lfoXRateSlider_ .setEnabled(!xPlayback);  lfoXRateSlider_ .setAlpha(xDimAlpha);
        lfoXPhaseSlider_.setEnabled(!xPlayback);  lfoXPhaseSlider_.setAlpha(xDimAlpha);
        lfoXLevelSlider_.setEnabled(!xPlayback);  lfoXLevelSlider_.setAlpha(xDimAlpha);
        lfoXSyncBtn_    .setEnabled(!xPlayback);  lfoXSyncBtn_    .setAlpha(xDimAlpha);
        // lfoXDistSlider_ always enabled (Distort stays live during playback)
        // lfoXClearBtn_ always enabled (must be clickable to exit playback)
    }

    // ── LFO Y recording UI ────────────────────────────────────────────────────
    {
        const LfoRecState yState = proc_.getLfoYRecState();

        // REC button visual: Unarmed=off, Armed=dim "ARM", Recording=bright "REC", Playback=dim "REC"
        lfoYArmBlinkCounter_ = 0;
        lfoYArmBtn_.setToggleState(false, juce::dontSendNotification);
        if (yState == LfoRecState::Unarmed)
        {
            lfoYArmBtn_.setButtonText("REC");
            lfoYArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOff);
        }
        else if (yState == LfoRecState::Armed)
        {
            lfoYArmBtn_.setButtonText("ARM");
            lfoYArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
        }
        else if (yState == LfoRecState::Recording)
        {
            lfoYArmBtn_.setButtonText("REC");
            lfoYArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn);
        }
        else  // Playback — dim lit to signal "recorded, playing back"
        {
            lfoYArmBtn_.setButtonText("REC");
            lfoYArmBtn_.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
        }

        // If LFO Y just went enabled→disabled while Armed/Recording/Playback,
        // call clearRecording() so state snaps back to Unarmed immediately.
        const bool lfoYOn = (*proc_.apvts.getRawParameterValue("lfoYEnabled") > 0.5f);
        if (prevLfoYOn_ && !lfoYOn) {
            proc_.clearLfoYRecording();
            proc_.resetLfoYPhase();   // reset phase on ON→OFF so next enable starts clean
        }
        prevLfoYOn_ = lfoYOn;
        lfoYArmBtn_.setEnabled(lfoYOn);

        // Push LFO Y output into visualizer ring buffer (drawn in paint())
        {
            const float yOut = proc_.lfoYOutputDisplay_.load(std::memory_order_relaxed);
            lfoYVisBuf_[lfoYVisBufIdx_] = yOut;
            lfoYVisBufIdx_ = (lfoYVisBufIdx_ + 1) % kLfoVisBufSize;
        }

        const bool yPlayback = (yState == LfoRecState::Playback);
        const float yDimAlpha = yPlayback ? 0.35f : 1.0f;
        lfoYShapeBox_   .setEnabled(!yPlayback);  lfoYShapeBox_   .setAlpha(yDimAlpha);
        lfoYRateSlider_ .setEnabled(!yPlayback);  lfoYRateSlider_ .setAlpha(yDimAlpha);
        lfoYPhaseSlider_.setEnabled(!yPlayback);  lfoYPhaseSlider_.setAlpha(yDimAlpha);
        lfoYLevelSlider_.setEnabled(!yPlayback);  lfoYLevelSlider_.setAlpha(yDimAlpha);
        lfoYSyncBtn_    .setEnabled(!yPlayback);  lfoYSyncBtn_    .setAlpha(yDimAlpha);
        // lfoYDistSlider_ and lfoYClearBtn_ always enabled
    }

    // ── LFO / Gate Length joystick display tracking ────────────────────────
    // Reads display atomics published by the audio thread dispatch block.
    // Updates slider thumbs visually without writing to APVTS (no feedback loop).
    // Guarded by xPlayback / yPlayback — grayed-out sliders do not receive updates.
    {
        const bool xPlayback = (proc_.getLfoXRecState() == LfoRecState::Playback);
        const bool yPlayback = (proc_.getLfoYRecState() == LfoRecState::Playback);
        const int  xMode     = static_cast<int>(proc_.apvts.getRawParameterValue("filterXMode")->load());
        const int  yMode     = static_cast<int>(proc_.apvts.getRawParameterValue("filterYMode")->load());
        const bool xSyncOn   = proc_.apvts.getRawParameterValue("lfoXSync")->load() > 0.5f;
        const bool ySyncOn   = proc_.apvts.getRawParameterValue("lfoYSync")->load() > 0.5f;

        // X axis targets
        if (!xPlayback)
        {
            switch (xMode)
            {
                case 4:  // LFO-X Freq — update in both free and sync mode to show modulated position
                    if (!lfoXRateDragging_)
                        lfoXRateSlider_.setValue(
                            proc_.lfoXRateDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 5:  // LFO-X Phase
                    if (!lfoXPhaseDragging_)
                        lfoXPhaseSlider_.setValue(
                            proc_.lfoXPhaseDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 6:  // LFO-X Level
                    if (!lfoXLevelDragging_)
                        lfoXLevelSlider_.setValue(
                            proc_.lfoXLevelDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 7:  // Gate Length
                    if (!gateDragging_)
                        arpGateTimeKnob_.setValue(
                            proc_.gateLengthDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                default: break;
            }
        }
        // MOD FIX X knob always tracks live joystick regardless of LFO playback state.
        // Not guarded by xPlayback — should always show the live modulated position.
        if (!filterXOffsetDragging_)
            filterXOffsetKnob_.setValue(
                proc_.filterXOffsetDisplay_.load(std::memory_order_relaxed),
                juce::dontSendNotification);

        // Y axis targets
        if (!yPlayback)
        {
            switch (yMode)
            {
                case 4:  // LFO-Y Freq — update in both free and sync mode
                    if (!lfoYRateDragging_)
                        lfoYRateSlider_.setValue(
                            proc_.lfoYRateDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 5:  // LFO-Y Phase
                    if (!lfoYPhaseDragging_)
                        lfoYPhaseSlider_.setValue(
                            proc_.lfoYPhaseDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 6:  // LFO-Y Level
                    if (!lfoYLevelDragging_)
                        lfoYLevelSlider_.setValue(
                            proc_.lfoYLevelDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                case 7:  // Gate Length — Y also writes gateLengthDisplay_; last writer wins (known limitation)
                    if (!gateDragging_)
                        arpGateTimeKnob_.setValue(
                            proc_.gateLengthDisplay_.load(std::memory_order_relaxed),
                            juce::dontSendNotification);
                    break;
                default: break;
            }
        }
        // MOD FIX Y knob always tracks live joystick regardless of LFO playback state.
        // Not guarded by yPlayback — should always show the live modulated position.
        if (!filterYOffsetDragging_)
            filterYOffsetKnob_.setValue(
                proc_.filterYOffsetDisplay_.load(std::memory_order_relaxed),
                juce::dontSendNotification);

        // ── Rate label — Hz in free mode, subdivision name in sync mode ──
        {
            static const char* kSubdivNames[] = {
                "16/1","8/1","4/1","4/1T","2/1","2/1T",
                "1/1","1/1T","1/2","1/4","1/4T",
                "1/8","1/16.","1/8T","1/16","1/16T","1/32","1/32T"
            };
            auto freeHzStr = [](double hz) -> juce::String {
                if (hz >= 10.0) return juce::String((int)std::round(hz)) + " Hz";
                if (hz >= 1.0)  return juce::String(hz, 1) + " Hz";
                return juce::String(hz, 2) + " Hz";
            };
            if (xSyncOn) {
                const int idx = juce::jlimit(0, 17, (int)std::round(lfoXRateSlider_.getValue()));
                lfoXSyncSubdivLabel_.setText(kSubdivNames[idx], juce::dontSendNotification);
            } else {
                lfoXSyncSubdivLabel_.setText(freeHzStr(lfoXRateSlider_.getValue()), juce::dontSendNotification);
            }
            if (ySyncOn) {
                const int idx = juce::jlimit(0, 17, (int)std::round(lfoYRateSlider_.getValue()));
                lfoYSyncSubdivLabel_.setText(kSubdivNames[idx], juce::dontSendNotification);
            } else {
                lfoYSyncSubdivLabel_.setText(freeHzStr(lfoYRateSlider_.getValue()), juce::dontSendNotification);
            }
        }

        // ── LFO fader anchor indicator from live left stick ───────────────────
        // Fills from the anchor position (the fader value at first stick engagement,
        // or the last manually dragged position) to the current thumb.
        // Anchor is fixed — it does NOT chase the fader.
        // On stick release the fill hides but the anchor is preserved so the fill
        // reappears from the same reference point on next engagement.
        // The anchor is re-chosen by dragging the fader (onDragEnd updates it).
        {
            const float lx = proc_.leftStickXDisplay_.load(std::memory_order_relaxed);
            const float ly = proc_.leftStickYDisplay_.load(std::memory_order_relaxed);
            constexpr float kThresh = 0.05f;

            // Pre-set rate anchors from APVTS base on first stick engagement, before
            // setValue() has already moved the slider to the modulated position.
            if (xMode == 4 && std::abs(lx) > kThresh && std::isnan(lfoXRateAnchor_))
                lfoXRateAnchor_ = xSyncOn
                    ? proc_.apvts.getRawParameterValue("lfoXSubdiv")->load()
                    : proc_.apvts.getRawParameterValue("lfoXRate")->load();
            if (yMode == 4 && std::abs(ly) > kThresh && std::isnan(lfoYRateAnchor_))
                lfoYRateAnchor_ = ySyncOn
                    ? proc_.apvts.getRawParameterValue("lfoYSubdiv")->load()
                    : proc_.apvts.getRawParameterValue("lfoYRate")->load();

            auto updateMod = [kThresh](juce::Slider& s, float delta, double& anchor) {
                if (std::abs(delta) > kThresh)
                {
                    // First engagement: snap anchor to current value (no fill yet this tick).
                    // Note: for rate sliders the anchor is pre-set above from APVTS base.
                    if (std::isnan(anchor))
                        anchor = s.getValue();
                    // Show fill from static anchor to current thumb
                    s.getProperties().set("modAnchor", anchor);
                    s.repaint();
                }
                else
                {
                    // Stick released: hide fill, keep anchor for next engagement
                    s.getProperties().remove("modAnchor");
                    s.repaint();
                }
            };

            updateMod(lfoXRateSlider_,  xMode == 4 ? lx : 0.0f, lfoXRateAnchor_);
            updateMod(lfoXPhaseSlider_, xMode == 5 ? lx : 0.0f, lfoXPhaseAnchor_);
            updateMod(lfoXLevelSlider_, xMode == 6 ? lx : 0.0f, lfoXLevelAnchor_);
            updateMod(lfoYRateSlider_,  yMode == 4 ? ly : 0.0f, lfoYRateAnchor_);
            updateMod(lfoYPhaseSlider_, yMode == 5 ? ly : 0.0f, lfoYPhaseAnchor_);
            updateMod(lfoYLevelSlider_, yMode == 6 ? ly : 0.0f, lfoYLevelAnchor_);
        }
    }

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

    // ── Left Stick Atten label update per mode ────────────────────────────────
    // Modes 0-1 are CC targets (show "Atten" + " %" suffix).
    // Modes 2-5 are LFO/Gate targets (show unit label, no "%" suffix).
    {
        const int xMode = (int)*proc_.apvts.getRawParameterValue("filterXMode");
        const juce::String xLabel = (xMode == 4) ? "Hz"
                                  : (xMode == 5) ? "Phase"
                                  : (xMode == 6) ? "Level"
                                  : (xMode == 7) ? "Gate"
                                  : "-/+ MOD X";
        if (filterXAttenLabel_.getText() != xLabel)
            styleLabel(filterXAttenLabel_, xLabel);
        const juce::String xSuffix = (xMode >= 4) ? "" : " %";
        if (filterXAttenKnob_.getTextValueSuffix() != xSuffix)
            filterXAttenKnob_.setTextValueSuffix(xSuffix);

        const int yMode = (int)*proc_.apvts.getRawParameterValue("filterYMode");
        const juce::String yLabel = (yMode == 4) ? "Hz"
                                  : (yMode == 5) ? "Phase"
                                  : (yMode == 6) ? "Level"
                                  : (yMode == 7) ? "Gate"
                                  : "-/+ MOD Y";
        if (filterYAttenLabel_.getText() != yLabel)
            styleLabel(filterYAttenLabel_, yLabel);
        const juce::String ySuffix = (yMode >= 4) ? "" : " %";
        if (filterYAttenKnob_.getTextValueSuffix() != ySuffix)
            filterYAttenKnob_.setTextValueSuffix(ySuffix);
    }

    // ARP button: blinks while armed (ARP ON but waiting for play to launch),
    // solid when actually running, off when disabled.
    // Read APVTS directly — getToggleState() can be stale after blink sets it false.
    const bool arpActuallyOn = (*proc_.apvts.getRawParameterValue("arpEnabled") > 0.5f);
    if (arpActuallyOn && proc_.isArpWaitingForPlay())
    {
        const bool on = ((++arpBlinkCounter_) / 5) % 2 == 0;  // ~3 blinks/sec at 30 Hz
        arpEnabledBtn_.setToggleState(on, juce::dontSendNotification);
        arpEnabledBtn_.setButtonText("ARP ARMED");
    }
    else
    {
        arpBlinkCounter_ = 0;
        // Re-sync toggle state in case blink left it false while arp is still running.
        if (arpEnabledBtn_.getToggleState() != arpActuallyOn)
            arpEnabledBtn_.setToggleState(arpActuallyOn, juce::dontSendNotification);
        arpEnabledBtn_.setButtonText(arpActuallyOn ? "ARP ON" : "ARP OFF");
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

    // Update OPTION indicator — dim=off, green=arp mode, red=interval mode
    {
        const int optMode = proc_.getGamepad().getOptionMode();
        const juce::String newText = (optMode == 0) ? "OPTION"
                                   : (optMode == 1) ? "ARP"
                                   :                  "INTRVL";
        const juce::Colour newCol  = (optMode == 0) ? Clr::textDim
                                   : (optMode == 1) ? Clr::gateOn
                                   :                  juce::Colour(0xFFFF4444);  // red for mode 2
        if (optionLabel_.getText() != newText)
            optionLabel_.setText(newText, juce::dontSendNotification);
        optionLabel_.setColour(juce::Label::textColourId, newCol);

        // Per-mode control highlight — only update when optMode changes (avoids 30Hz redundant repaints)
        if (optMode != lastHighlightMode_)
        {
            lastHighlightMode_ = optMode;

            // Mode 1 highlight colour (green tint) / Mode 2 highlight colour (muted blue) / no highlight
            const auto highlightM1 = Clr::gateOn.withAlpha(0.18f);
            const auto highlightM2 = juce::Colour(0x2d5588cc);   // muted blue at ~18% alpha
            const auto noHighlight  = juce::Colour(0x00000000);  // transparent = default

            // Mode 1 controls: arp section labels + octave knob labels
            juce::Label* mode1Labels[] = {
                &arpSubdivLabel_,
                &arpOrderLabel_,
                &rootOctLabel_,
                &thirdOctLabel_,
                &fifthOctLabel_,
                &tensionOctLabel_,
            };
            const auto m1Colour = (optMode == 1) ? highlightM1 : noHighlight;
            for (auto* lbl : mode1Labels)
            {
                lbl->setColour(juce::Label::backgroundColourId, m1Colour);
                lbl->repaint();
            }

            // Mode 2 controls: transpose label + interval knob labels
            juce::Label* mode2Labels[] = {
                &transposeLabel_,
                &thirdIntLabel_,
                &fifthIntLabel_,
                &tensionIntLabel_,
            };
            const auto m2Colour = (optMode == 2) ? highlightM2 : noHighlight;
            for (auto* lbl : mode2Labels)
            {
                lbl->setColour(juce::Label::backgroundColourId, m2Colour);
                lbl->repaint();
            }
        }
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

        // Update gamepad silhouette display
        auto& gp = proc_.getGamepad();
        gamepadDisplay_.setGamepadState(gp.getButtonHeldMask(),
                                        gp.getLeftStickXLive(),
                                        gp.getLeftStickYLive(),
                                        gp.getPitchX(),
                                        gp.getPitchY(),
                                        connected);
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

    // Sync looper recording buttons (gamepad D-pad may have toggled them from audio thread).
    // Visual: OFF=dark, ON+looper-not-recording=dim lit (armed/waiting), ON+recording=bright lit.
    {
        const bool isRecording = proc_.looperIsRecording();
        auto setRecBtnBrightness = [&](juce::TextButton& btn, bool featureOn)
        {
            btn.setToggleState(false, juce::dontSendNotification);
            if (!featureOn)
                btn.setColour(juce::TextButton::buttonColourId, Clr::gateOff);
            else if (isRecording)
                btn.setColour(juce::TextButton::buttonColourId, Clr::gateOn);
            else
                btn.setColour(juce::TextButton::buttonColourId, Clr::gateOn.darker(0.45f));
        };
        setRecBtnBrightness(loopRecGatesBtn_, proc_.looperIsRecGates());
        setRecBtnBrightness(loopRecJoyBtn_,   proc_.looperIsRecJoy());
        setRecBtnBrightness(filterRecBtn_,    proc_.looperIsRecFilter());
        // filterModBtn_ uses DAW SYNC toggle style: buttonOnColourId drives the lit state.
        filterModBtn_.setToggleState(proc_.isFilterModActive(), juce::dontSendNotification);
    }

    // Mirror the pitch-control stick to joystickX/Y (cursor + chord input).
    // SWAP off (default): right stick is pitch.  SWAP on: left stick is pitch.
    // INV: swap X↔Y axes on the chosen stick.
    // - When the stick moves: write its position and remember it was last writer.
    // - When the stick returns to centre: write 0 once (so the cursor snaps back),
    //   then clear the flag so subsequent mouse clicks are not overridden.
    // - When the stick is at centre and was never moved (flag=false): leave
    //   joystickX/Y untouched so mouse clicks stay in effect.
    if (proc_.getGamepad().isConnected() && proc_.isGamepadActive())
    {
        const bool  swapOn = *proc_.apvts.getRawParameterValue("stickSwap")   > 0.5f;
        const bool  invOn  = *proc_.apvts.getRawParameterValue("stickInvert") > 0.5f;
        const float rawX = swapOn ? proc_.getGamepad().getFilterX() : proc_.getGamepad().getPitchX();
        const float rawY = swapOn ? proc_.getGamepad().getFilterY() : proc_.getGamepad().getPitchY();
        const float gpX  = invOn ? rawY : rawX;
        const float gpY  = invOn ? rawX : rawY;

        // Use the same bypass threshold as GamepadDisplayComponent so jitter
        // that lingers just above the dead zone does not override mouse input.
        const bool intentional = std::abs(gpX) > kStickBypassThreshold ||
                                 std::abs(gpY) > kStickBypassThreshold;

        if (intentional)
        {
            proc_.joystickX.store(gpX);
            proc_.joystickY.store(gpY);
            gamepadWasLastPitchWriter_ = true;
        }
        else if (gamepadWasLastPitchWriter_)
        {
            // Stick just returned to centre — snap cursor back to 0, unless
            // the user is currently mouse-dragging the right stick illustration
            // (in which case the mouse takes over immediately).
            if (!gamepadDisplay_.isRightStickMouseDragging())
            {
                proc_.joystickX.store(0.0f);
                proc_.joystickY.store(0.0f);
            }
            gamepadWasLastPitchWriter_ = false;
        }
        // else: stick in jitter zone and no prior write — leave joystickX/Y to mouse
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
    {
        beatPulse_ = 1.0f;
        joystickPad_.resetGlowPhase();  // sync cursor breathing to beat
    }
    else if (beatPulse_ > 0.0f)
    {
        beatPulse_ = juce::jmax(0.0f, beatPulse_ - 0.11f);
    }

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
        chordNameLabel_.setText(computeChordName(pitches.data()), juce::dontSendNotification);
    }

}
