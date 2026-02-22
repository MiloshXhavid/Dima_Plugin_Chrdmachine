#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <functional>

// SDL2 forward declarations (avoid including SDL headers in headers)
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

// ─── GamepadInput ─────────────────────────────────────────────────────────────
// Polls an SDL2 game controller at 60 Hz (via juce::Timer).
// Writes state to atomics so the audio thread and UI thread can read safely.
//
// PS Controller mapping:
//   Right stick X/Y  → pitch joystick
//   R1 (RightShoulder) → voice 0 (Root)
//   R2 (RightTrigger)  → voice 1 (Third)
//   L1 (LeftShoulder)  → voice 2 (Fifth)
//   L2 (LeftTrigger)   → voice 3 (Tension)
//   Left stick X       → filter cutoff (CC74)
//   Left stick Y       → filter resonance (CC71)
//   L3 (LeftStick btn) → all-notes trigger
//   Cross (A)          → looper start/stop
//   Square (X)         → looper reset
//   Triangle (Y)       → looper record
//   Circle (B)         → looper delete

class GamepadInput : private juce::Timer
{
public:
    GamepadInput();
    ~GamepadInput() override;

    // ── Normalised joystick values (-1..+1), updated at 60Hz ─────────────────
    float getPitchX()    const { return pitchX_.load(); }
    float getPitchY()    const { return pitchY_.load(); }
    float getFilterX()   const { return filterX_.load(); }
    float getFilterY()   const { return filterY_.load(); }

    // ── Voice trigger rising-edge flags (consume = clear after read) ──────────
    bool consumeVoiceTrigger(int voice);  // returns true once per press

    // ── Special button rising-edge flags ─────────────────────────────────────
    bool consumeAllNotesTrigger();
    bool consumeLooperStartStop();
    bool consumeLooperRecord();
    bool consumeLooperReset();
    bool consumeLooperDelete();

    // Whether a gamepad is connected
    bool isConnected() const { return controller_ != nullptr; }

    // Called when controller connects/disconnects (for UI feedback)
    std::function<void(bool)> onConnectionChange;

private:
    void timerCallback() override;
    void tryOpenController();
    void closeController();
    float normaliseAxis(int16_t raw) const;
    bool  triggerPressed(int16_t axisValue) const;  // L2/R2 are axes

    SDL_GameController* controller_ = nullptr;

    std::atomic<float> pitchX_  {0.0f};
    std::atomic<float> pitchY_  {0.0f};
    std::atomic<float> filterX_ {0.0f};
    std::atomic<float> filterY_ {0.0f};

    std::atomic<bool>  voiceTrig_[4]    {};
    std::atomic<bool>  allNotesTrig_    {false};
    std::atomic<bool>  looperStartStop_ {false};
    std::atomic<bool>  looperRecord_    {false};
    std::atomic<bool>  looperReset_     {false};
    std::atomic<bool>  looperDelete_    {false};

    // Previous button states for edge detection
    bool prevVoice_[4]      = {};
    bool prevAllNotes_      = false;
    bool prevStartStop_     = false;
    bool prevRecord_        = false;
    bool prevReset_         = false;
    bool prevDelete_        = false;

    bool sdlInitialised_    = false;  // guard: SDL_Quit() only if SDL_Init() succeeded
};
