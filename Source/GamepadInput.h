#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cstdint>
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
//   L1 (LeftShoulder)  → voice 0 (Root)
//   L2 (LeftTrigger)   → voice 1 (Third)
//   R1 (RightShoulder) → voice 2 (Fifth)
//   R2 (RightTrigger)  → voice 3 (Tension)
//   Left stick Y       → filter cutoff (CC74)
//   Left stick X       → filter resonance (CC71)
//   L3 (LeftStick btn) → all-notes trigger
//   Cross (A)          → looper start/stop
//   Square (X)         → looper reset
//   Triangle (Y)       → looper record
//   Circle (B)         → looper delete
//   R3 (RightStick btn)→ MIDI panic

class GamepadInput : private juce::Timer
{
public:
    GamepadInput();
    ~GamepadInput() override;

    // ── Normalised joystick values (-1..+1), updated at 60Hz ─────────────────
    float getPitchX()      const { return pitchX_.load(); }
    float getPitchY()      const { return pitchY_.load(); }
    float getFilterX()     const { return filterX_.load(); }
    float getFilterY()     const { return filterY_.load(); }
    // Raw Y (no sample-and-hold): 0 when stick is in dead zone.
    // Use for CC64 sustain so releasing the stick sends CC64=0.
    float getFilterYRaw()  const { return filterYRaw_.load(); }

    // ── Voice trigger rising-edge flags (consume = clear after read) ──────────
    bool consumeVoiceTrigger(int voice);  // returns true once per press
    bool getVoiceHeld(int v) const { return (v >= 0 && v < 4) ? voiceHeld_[v].load() : false; }

    // ── Special button rising-edge flags ─────────────────────────────────────
    bool consumeAllNotesTrigger();
    // Held state for L3 — true while left stick is physically pressed
    bool getAllNotesHeld() const { return allNotesHeld_.load(std::memory_order_relaxed); }
    bool consumeLooperStartStop();
    bool consumeLooperRecord();
    bool consumeLooperReset();
    bool consumeLooperDelete();
    bool consumeDpadUp()    { return dpadUpTrig_.exchange(false); }
    bool consumeDpadDown()  { return dpadDownTrig_.exchange(false); }
    bool consumeDpadLeft()  { return dpadLeftTrig_.exchange(false); }
    bool consumeDpadRight() { return dpadRightTrig_.exchange(false); }
    bool consumeRightStickTrigger() { return rightStickTrig_.exchange(false); }

    // Preset-scroll mode state — read by PluginProcessor / PluginEditor
    bool isPresetScrollActive() const { return presetScrollActive_; }

    // Consume a pending Program Change delta (+1 or -1).
    // Returns 0 if nothing pending. Processor reads this instead of pendingBpmDelta_ when in preset-scroll mode.
    int consumePcDelta() { return pendingPcDelta_.exchange(0, std::memory_order_relaxed); }

    // Whether a gamepad is connected
    bool isConnected() const { return controller_ != nullptr; }

    // Returns the SDL controller name string if connected, empty string if not.
    // Used by PluginEditor constructor for synchronous initial label update.
    juce::String getControllerName() const;

    // ── Dead zone setter ──────────────────────────────────────────────────────
    // Called from processBlock via PluginProcessor — sets dead zone threshold
    // from the joystickThreshold APVTS parameter (single source of truth).
    void setDeadZone(float t) { deadZone_.store(t, std::memory_order_relaxed); }

    // ── Connection callbacks — two independent slots ──────────────────────────
    // Called on message thread when a controller connects or disconnects.
    // PluginProcessor ctor sets this slot to handle pendingAllNotesOff_ / pendingCcReset_.
    std::function<void(bool)> onConnectionChange;

    // Called on message thread when a controller connects or disconnects.
    // Passes the controller name string on connect (e.g., "DualShock 4 Wireless Controller"),
    // or an empty juce::String on disconnect.
    // PluginEditor sets this slot to update gamepadStatusLabel_ with PS4/PS5/Xbox/generic text.
    // Kept separate from onConnectionChange so both fire independently.
    std::function<void(juce::String)> onConnectionChangeUI;

private:
    void timerCallback() override;
    void tryOpenController();
    void closeController();
    float normaliseAxis(int16_t raw) const;
    bool  triggerPressed(int16_t axisValue) const;  // L2/R2 are axes

    SDL_GameController* controller_ = nullptr;

    // Dead zone — set from processBlock via setDeadZone(); replaces hard-coded kDeadzone
    std::atomic<float> deadZone_ { 0.08f };  // conservative default until first setDeadZone call

    std::atomic<float> pitchX_     {0.0f};
    std::atomic<float> pitchY_     {0.0f};
    std::atomic<float> filterX_    {0.0f};
    std::atomic<float> filterY_    {0.0f};
    std::atomic<float> filterYRaw_ {0.0f};  // 0 when in dead zone (no S&H)

    // Sample-and-hold for filter: holds last non-dead-zone value when stick returns to center.
    // Pitch joystick does NOT use S&H — it returns to 0 when stick is released.
    float lastFilterX_ = 0.0f;
    float lastFilterY_ = 0.0f;

    std::atomic<bool>  voiceTrig_[4]    {};
    std::atomic<bool>  voiceHeld_[4]    {};  // current debounced held state (for note-off)

    std::atomic<bool>  dpadUpTrig_    {false};
    std::atomic<bool>  dpadDownTrig_  {false};
    std::atomic<bool>  dpadLeftTrig_  {false};
    std::atomic<bool>  dpadRightTrig_ {false};

    std::atomic<bool>  allNotesTrig_    {false};
    std::atomic<bool>  allNotesHeld_    {false};
    std::atomic<bool>  looperStartStop_ {false};
    std::atomic<bool>  looperRecord_    {false};
    std::atomic<bool>  looperReset_     {false};
    std::atomic<bool>  looperDelete_    {false};
    std::atomic<bool>  rightStickTrig_  {false};

    // 20ms button debounce — each button tracks previous state and last-change timestamp
    struct ButtonState
    {
        bool     prev   = false;
        uint32_t lastMs = 0;  // SDL_GetTicks() ms timestamp of last state change
    };
    static constexpr uint32_t kDebounceMsBtn = 20;

    ButtonState btnVoice_[4];
    ButtonState btnAllNotes_;
    ButtonState btnStartStop_;
    ButtonState btnRecord_;
    ButtonState btnReset_;
    ButtonState btnDelete_;
    ButtonState btnDpadUp_;
    ButtonState btnDpadDown_;
    ButtonState btnDpadLeft_;
    ButtonState btnDpadRight_;
    ButtonState btnRightStick_;

    // Preset-scroll mode — toggled by Option/Guide button
    bool presetScrollActive_ = false;  // not persisted; starts false on plugin load
    bool optionFrameFired_   = false;  // one-frame lockout: true in the frame Option toggles
    ButtonState btnOption_;            // debounce state for SDL_CONTROLLER_BUTTON_GUIDE

    // Pending PC delta — set by timerCallback when presetScrollActive_; consumed by processor
    std::atomic<int> pendingPcDelta_ { 0 };

    bool sdlInitialised_    = false;  // guard: SdlContext::release() only if acquire() succeeded
};
