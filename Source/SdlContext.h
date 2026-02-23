#pragma once

// Process-level SDL2 lifecycle guard.
//
// Call SdlContext::acquire() from GamepadInput ctor.
// Call SdlContext::release() from GamepadInput dtor.
//
// SDL_Init(SDL_INIT_GAMECONTROLLER) is called exactly once per process
// on the first acquire(); SDL_Quit() is called when the last caller releases.
// This prevents the SDL_Init/SDL_Quit race when multiple plugin instances
// exist in the same DAW process.

struct SdlContext
{
    // Returns true if SDL is now (or already was) initialised.
    // Sets SDL_HINT_JOYSTICK_THREAD and SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS
    // before SDL_Init on the first call.
    static bool acquire();

    // Must be called once for every successful acquire().
    static void release();

    // True if SDL was successfully initialised in this process.
    static bool isAvailable();
};
