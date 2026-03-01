#include "SdlContext.h"
#include <SDL.h>
#include <atomic>

static std::atomic<int>  gRefCount  { 0 };
static std::atomic<bool> gAvailable { false };

bool SdlContext::acquire()
{
    if (gRefCount.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        // First caller — set hints then initialise SDL.
        // All hints MUST be set before SDL_Init.
        SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        // Disable HIDAPI and RawInput device scanning.  SDL's HIDAPI driver
        // enumerates every HID endpoint on the system at SDL_Init time; on
        // machines with a USB MIDI interface (e.g. iConnectivity M8U eX),
        // this deep scan can take several seconds and races with Ableton's
        // own MIDI port initialisation.  If the host process is killed while
        // both are running concurrently, Ableton records the exit as abnormal
        // and permanently disables the MIDI port.  Disabling these two
        // backends restricts SDL to XInput + DirectInput, which is sufficient
        // for PS4/PS5 controllers via DS4Windows or Steam.
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
        SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");

        const bool ok = (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
        if (!ok)
            (void)SDL_GetError();  // SDL_Init failed; error string available via SDL_GetError() in debugger
        gAvailable.store(ok, std::memory_order_release);
        return ok;
    }
    // Subsequent callers — SDL already initialised.
    return gAvailable.load(std::memory_order_acquire);
}

void SdlContext::release()
{
    if (gRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        // Last caller — shut SDL down.
        SDL_Quit();
        gAvailable.store(false, std::memory_order_release);
    }
}

bool SdlContext::isAvailable()
{
    return gAvailable.load(std::memory_order_acquire);
}
