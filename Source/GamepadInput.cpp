#include "GamepadInput.h"

// Include SDL2 only in the .cpp to avoid polluting headers
#include <SDL.h>
#include "SdlContext.h"

static constexpr int kTriggerThreshold = 8000;  // SDL axis > this = pressed

GamepadInput::GamepadInput()
{
    for (auto& a : voiceTrig_)    a.store(false);

    // Acquire the process-level SDL singleton.
    // SdlContext sets SDL hints (JOYSTICK_THREAD, BACKGROUND_EVENTS) before
    // calling SDL_Init on the first instance — no per-instance hint/init needed.
    sdlInitialised_ = SdlContext::acquire();

    if (!sdlInitialised_)
    {
        DBG("GamepadInput: SdlContext::acquire() failed — no gamepad support");
        return;
    }

    tryOpenController();
    startTimerHz(60);
}

GamepadInput::~GamepadInput()
{
    stopTimer();
    closeController();
    if (sdlInitialised_)
        SdlContext::release();
}

void GamepadInput::tryOpenController()
{
    const int n = SDL_NumJoysticks();
    for (int i = 0; i < n; ++i)
    {
        if (SDL_IsGameController(i))
        {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_)
            {
                // Fire both callback slots — processor handles pending flags, editor handles UI
                if (onConnectionChange)   onConnectionChange(true);
                if (onConnectionChangeUI) onConnectionChangeUI(true);
            }
            return;
        }
    }
}

void GamepadInput::closeController()
{
    if (controller_)
    {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        // Fire both callback slots
        if (onConnectionChange)   onConnectionChange(false);
        if (onConnectionChangeUI) onConnectionChangeUI(false);
    }
}

float GamepadInput::normaliseAxis(int16_t raw) const
{
    const float v = static_cast<float>(raw) / 32767.0f;
    return std::abs(v) < deadZone_.load(std::memory_order_relaxed) ? 0.0f : v;
}

bool GamepadInput::triggerPressed(int16_t axisValue) const
{
    return axisValue > kTriggerThreshold;
}

// ─── Timer callback (main thread, ~60 Hz) ────────────────────────────────────

void GamepadInput::timerCallback()
{
    const uint32_t now = SDL_GetTicks();

    // Debounce helper: returns true on a confirmed state transition
    // (cur != prev AND at least kDebounceMsBtn ms have elapsed since last change).
    auto debounce = [&](bool cur, ButtonState& s) -> bool {
        if (cur != s.prev && (now - s.lastMs) >= kDebounceMsBtn)
        {
            s.prev   = cur;
            s.lastMs = now;
            return true;
        }
        return false;
    };

    // SDL event loop
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        if (ev.type == SDL_CONTROLLERDEVICEADDED)
            tryOpenController();
        else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
            closeController();
    }

    if (!controller_) return;

    // ── Right stick → pitch joystick (with sample-and-hold) ──────────────────
    {
        const float rawX =  normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTX));
        const float rawY = -normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTY));
        if (rawX != 0.0f) lastPitchX_ = rawX;
        if (rawY != 0.0f) lastPitchY_ = rawY;
        pitchX_.store(lastPitchX_, std::memory_order_relaxed);
        pitchY_.store(lastPitchY_, std::memory_order_relaxed);
    }

    // ── Left stick → filter (with sample-and-hold) ───────────────────────────
    {
        const float rawX =  normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX));
        const float rawY = -normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY));
        if (rawX != 0.0f) lastFilterX_ = rawX;
        if (rawY != 0.0f) lastFilterY_ = rawY;
        filterX_.store(lastFilterX_, std::memory_order_relaxed);
        filterY_.store(lastFilterY_, std::memory_order_relaxed);
        filterYRaw_.store(rawY, std::memory_order_relaxed);  // 0 when in dead zone (no S&H)
    }

    // ── Voice triggers: L1/L2/R1/R2 (debounced) ──────────────────────────────
    // L1 = left  shoulder button  → voice 0 (Root)
    // L2 = left  trigger  (axis)  → voice 1 (Third)
    // R1 = right shoulder button  → voice 2 (Fifth)
    // R2 = right trigger (axis)   → voice 3 (Tension)
    const bool curVoice[4] =
    {
        SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)  != 0,
        triggerPressed(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERLEFT)),
        SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0,
        triggerPressed(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)),
    };
    for (int i = 0; i < 4; ++i)
    {
        if (debounce(curVoice[i], btnVoice_[i]) && btnVoice_[i].prev)
            voiceTrig_[i].store(true);  // rising edge after debounce
        voiceHeld_[i].store(btnVoice_[i].prev, std::memory_order_relaxed);
    }

    // ── L3 (left stick click) → all-notes trigger + held state ──────────────
    {
        const bool cur = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_LEFTSTICK) != 0;
        if (debounce(cur, btnAllNotes_) && btnAllNotes_.prev)
            allNotesTrig_.store(true);
        allNotesHeld_.store(btnAllNotes_.prev, std::memory_order_relaxed);
    }

    // ── Looper buttons: Cross(A)/Circle(B)/Square(X)/Triangle(Y) ─────────────
    // SDL A=Cross(X), B=Circle, X=Square, Y=Triangle
    {
        const bool curSS  = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_A) != 0;
        const bool curDel = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_B) != 0;
        const bool curRst = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_X) != 0;
        const bool curRec = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_Y) != 0;

        if (debounce(curSS,  btnStartStop_) && btnStartStop_.prev) looperStartStop_.store(true);
        if (debounce(curDel, btnDelete_)    && btnDelete_.prev)    looperDelete_.store(true);
        if (debounce(curRst, btnReset_)     && btnReset_.prev)     looperReset_.store(true);
        if (debounce(curRec, btnRecord_)    && btnRecord_.prev)    looperRecord_.store(true);
    }

    // ── D-pad: BPM and looper recording toggles ──────────────────────────────
    {
        const bool curUp    = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_UP)    != 0;
        const bool curDown  = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_DOWN)  != 0;
        const bool curLeft  = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_LEFT)  != 0;
        const bool curRight = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;

        if (debounce(curUp,    btnDpadUp_)    && btnDpadUp_.prev)    dpadUpTrig_.store(true);
        if (debounce(curDown,  btnDpadDown_)  && btnDpadDown_.prev)  dpadDownTrig_.store(true);
        if (debounce(curLeft,  btnDpadLeft_)  && btnDpadLeft_.prev)  dpadLeftTrig_.store(true);
        if (debounce(curRight, btnDpadRight_) && btnDpadRight_.prev) dpadRightTrig_.store(true);
    }

    // ── R3 (right stick click) → MIDI panic ──────────────────────────────────
    {
        const bool cur = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_RIGHTSTICK) != 0;
        if (debounce(cur, btnRightStick_) && btnRightStick_.prev)
            rightStickTrig_.store(true);
    }
}

// ── Consume helpers ───────────────────────────────────────────────────────────

bool GamepadInput::consumeVoiceTrigger(int voice)
{
    if (voice < 0 || voice >= 4) return false;
    return voiceTrig_[voice].exchange(false);
}

bool GamepadInput::consumeAllNotesTrigger()   { return allNotesTrig_.exchange(false); }
bool GamepadInput::consumeLooperStartStop()   { return looperStartStop_.exchange(false); }
bool GamepadInput::consumeLooperRecord()      { return looperRecord_.exchange(false); }
bool GamepadInput::consumeLooperReset()       { return looperReset_.exchange(false); }
bool GamepadInput::consumeLooperDelete()      { return looperDelete_.exchange(false); }
