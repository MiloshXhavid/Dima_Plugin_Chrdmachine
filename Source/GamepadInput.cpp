#include "GamepadInput.h"

// Include SDL2 only in the .cpp to avoid polluting headers
#include <SDL.h>

static constexpr int   kTriggerThreshold = 8000;  // SDL axis > this = pressed
static constexpr float kDeadzone         = 0.08f;

GamepadInput::GamepadInput()
{
    for (auto& a : voiceTrig_)    a.store(false);

    // Background thread for joystick I/O — must be set before SDL_Init.
    // Without this, SDL2 on Windows tries to process WM_DEVICECHANGE on the
    // calling thread. DAW plugin threads have no message loop → crash on load.
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
    {
        DBG("SDL_Init failed: " << SDL_GetError());
        return;
    }
    sdlInitialised_ = true;

    tryOpenController();
    startTimerHz(60);
}

GamepadInput::~GamepadInput()
{
    stopTimer();
    closeController();
    if (sdlInitialised_)
        SDL_Quit();
}

void GamepadInput::tryOpenController()
{
    const int n = SDL_NumJoysticks();
    for (int i = 0; i < n; ++i)
    {
        if (SDL_IsGameController(i))
        {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_ && onConnectionChange)
                onConnectionChange(true);
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
        if (onConnectionChange)
            onConnectionChange(false);
    }
}

float GamepadInput::normaliseAxis(int16_t raw) const
{
    const float v = static_cast<float>(raw) / 32767.0f;
    return std::abs(v) < kDeadzone ? 0.0f : v;
}

bool GamepadInput::triggerPressed(int16_t axisValue) const
{
    return axisValue > kTriggerThreshold;
}

// ─── Timer callback (main thread, ~60 Hz) ────────────────────────────────────

void GamepadInput::timerCallback()
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        if (ev.type == SDL_CONTROLLERDEVICEADDED)
            tryOpenController();
        else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
            closeController();
    }

    if (!controller_) return;

    // ── Right stick → pitch joystick ─────────────────────────────────────────
    {
        const float x =  normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTX));
        const float y = -normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTY));
        pitchX_.store(x);
        pitchY_.store(y);
    }

    // ── Left stick → filter ───────────────────────────────────────────────────
    {
        const float x =  normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX));
        const float y = -normaliseAxis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY));
        filterX_.store(x);
        filterY_.store(y);
    }

    // ── Voice triggers: R1/R2/L1/L2 ──────────────────────────────────────────
    // R1 = right shoulder button  → voice 0 (Root)
    // R2 = right trigger (axis)   → voice 1 (Third)
    // L1 = left  shoulder button  → voice 2 (Fifth)
    // L2 = left  trigger  (axis)  → voice 3 (Tension)
    const bool curVoice[4] =
    {
        SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0,
        triggerPressed(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)),
        SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)  != 0,
        triggerPressed(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERLEFT)),
    };

    for (int i = 0; i < 4; ++i)
    {
        if (curVoice[i] && !prevVoice_[i])
            voiceTrig_[i].store(true);   // rising edge
        prevVoice_[i] = curVoice[i];
    }

    // ── L3 (left stick click) → all-notes trigger ─────────────────────────────
    {
        const bool cur = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_LEFTSTICK) != 0;
        if (cur && !prevAllNotes_)
            allNotesTrig_.store(true);
        prevAllNotes_ = cur;
    }

    // ── Looper buttons: Cross/Square/Triangle/Circle ──────────────────────────
    // SDL A=Cross(X), B=Circle, X=Square, Y=Triangle
    {
        const bool curSS  = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_A) != 0;
        const bool curDel = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_B) != 0;
        const bool curRst = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_X) != 0;
        const bool curRec = SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_Y) != 0;

        if (curSS  && !prevStartStop_) looperStartStop_.store(true);
        if (curDel && !prevDelete_)    looperDelete_.store(true);
        if (curRst && !prevReset_)     looperReset_.store(true);
        if (curRec && !prevRecord_)    looperRecord_.store(true);

        prevStartStop_ = curSS;
        prevDelete_    = curDel;
        prevReset_     = curRst;
        prevRecord_    = curRec;
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
