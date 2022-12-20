#include "app.hpp"
#include "core/time.hpp"
#include "core/input.hpp"
#include "render/renderer.hpp"

namespace Ty
{

void InitApp(u32 windowWidth, u32 windowHeight, const char* appTitle)
{
    // Initializing memory arenas
    MemArenaInit(&memArena_Perm, MB(1));
    MemArenaInit(&memArena_Frame, MB(1));

    // Initializing engine systems
    InitTime();

    // Creating window
    appWindow = WindowCreate(&memArena_Perm, windowWidth, windowHeight, Str(appTitle));

    // Initialize rendering context and rendering system
    InitRenderer(appWindow);

    // Display window
    WindowShow(appWindow);
}

void Update()
{
    // Clear frame temporary memory
    MemClear(&memArena_Frame);

    // Update engine systems
    UpdateInputState();
}

void DestroyApp()
{
    // Destroy app window
    WindowDestroy(appWindow);

    // TODO(caio)#PLATFORM: Do I need to destroy more stuff? Maybe not
}

} // namespace Ty
