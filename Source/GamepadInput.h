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
//   Right stick X/Y    → pitch joystick
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
//   PS / Guide button  → soft disconnect / reconnect (toggles gamepad on/off)

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

    // Option mode: 0=off (BPM/looper), 1=octaves (green), 2=transpose+intervals (red)
    int  getOptionMode()       const { return optionMode_.load(std::memory_order_relaxed); }
    bool isPresetScrollActive() const { return getOptionMode() != 0; }  // legacy compat

    // Consume a pending D-pad delta in option mode (modes 1 or 2). 0=up 1=down 2=left 3=right.
    // Returns +1 (single press), -1 (fast double-click within kDpadDoubleClickMs), or 0 (nothing).
    int consumeOptionDpadDelta(int dir)
    {
        if (dir < 0 || dir >= 4) return 0;
        return pendingOptionDpadDelta_[dir].exchange(0, std::memory_order_relaxed);
    }

    // Option Mode 1 face-button arp dispatch consumers (called from processBlock optMode==1 block)
    int  consumeArpRateDelta()  { return pendingArpRateDelta_.exchange(0, std::memory_order_relaxed); }
    int  consumeArpOrderDelta() { return pendingArpOrderDelta_.exchange(0, std::memory_order_relaxed); }
    bool consumeArpCircle()     { return pendingArpCircle_.exchange(false, std::memory_order_relaxed); }
    bool consumeRndSyncToggle() { return pendingRndSyncToggle_.exchange(false, std::memory_order_relaxed); }

    // Whether a gamepad is connected and not soft-disconnected via PS button
    bool isConnected() const { return controller_ != nullptr && virtuallyConnected_; }

    // Battery level: -2=no controller, -1=unknown, 0=empty, 1=low, 2=medium, 3=full, 4=wired
    int getBatteryLevel() const;

    // Returns the SDL controller name string if connected, empty string if not.
    // Used by PluginEditor constructor for synchronous initial label update.
    juce::String getControllerName() const;

    // ── Button held-state bitmask (UI visualization — updated at 60 Hz) ─────────
    // Bit positions: L1=0 L2=1 R1=2 R2=3 L3=4 R3=5 Cross=6 Square=7 Triangle=8
    //               Circle=9 DpadUp=10 DpadDown=11 DpadLeft=12 DpadRight=13
    //               Options=14 PS=15
    uint32_t getButtonHeldMask() const { return buttonHeldMask_.load(std::memory_order_relaxed); }

    // Left stick live position (-1..+1), no sample-and-hold. +X = right, +Y = up.
    float getLeftStickXLive() const { return leftStickXLive_.load(std::memory_order_relaxed); }
    float getLeftStickYLive() const { return leftStickYLive_.load(std::memory_order_relaxed); }

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

    // Option mode — cycles 0→1→2→0 on each Option button press
    std::atomic<int> optionMode_ { 0 };  // 0=off  1=octaves(green)  2=transpose+intervals(red)
    bool optionFrameFired_   = false;    // one-frame lockout: true in the frame Option toggles
    ButtonState btnOption_;              // debounce state for SDL_CONTROLLER_BUTTON_START

    // ── Option-mode D-pad delta signals ──────────────────────────────────────
    // Set on rising edge in mode 1 or 2; consumed by PluginProcessor each block.
    // +1 = single press, -1 = fast double-click (second press within kDpadDoubleClickMs ms).
    static constexpr uint32_t kDpadDoubleClickMs = 300;
    std::atomic<int>  pendingOptionDpadDelta_[4] {};
    uint32_t          optDpadLastPressMs_[4] = {};

    // ── Option Mode 1 face-button arp dispatch signals ────────────────────────
    // pendingArpRateDelta_: +1 = single Triangle press, -2 = second press within kDpadDoubleClickMs
    // pendingArpOrderDelta_: same pattern for Square
    // pendingArpCircle_: Circle rising edge (arp toggle)
    // pendingRndSyncToggle_: Cross rising edge (RND Sync toggle)
    std::atomic<int>  pendingArpRateDelta_  {0};
    std::atomic<int>  pendingArpOrderDelta_ {0};
    std::atomic<bool> pendingArpCircle_     {false};
    std::atomic<bool> pendingRndSyncToggle_ {false};

    uint32_t          arpRateLastPressMs_   = 0;
    uint32_t          arpOrderLastPressMs_  = 0;

    // Separate ButtonState trackers for Mode 1 face buttons (separate from looper button states
    // so that looper atoms continue firing in all modes from the unconditional looper block).
    ButtonState btnMode1Circle_;
    ButtonState btnMode1Triangle_;
    ButtonState btnMode1Square_;
    ButtonState btnMode1Cross_;
    ButtonState btnGuide_;   // PS / Guide button — soft connect/disconnect toggle

    // True while the controller is physically open AND the user has not
    // soft-disconnected via the PS button. isConnected() checks both.
    bool virtuallyConnected_ = false;

    bool sdlInitialised_    = false;  // guard: SdlContext::release() only if acquire() succeeded
    bool pendingReopenTick_ = false;  // deferred BT open: set on SDL_CONTROLLERDEVICEADDED, consumed after event loop

    // ── UI visualization state ────────────────────────────────────────────────
    std::atomic<uint32_t> buttonHeldMask_ {0};
    std::atomic<float>    leftStickXLive_ {0.0f};
    std::atomic<float>    leftStickYLive_ {0.0f};
};
