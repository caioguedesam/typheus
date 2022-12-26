//#include "app.hpp"
//#include "core/time.hpp"
//#include "core/input.hpp"
//#include "render/renderer.hpp"
#include "app/app.hpp"
#include "engine/renderer/renderer.hpp"

namespace Ty
{

void App_Init(u32 windowWidth, u32 windowHeight, const char* appTitle)
{
    //// Initializing memory arenas
    //MemArenaInit(&memArena_Perm, MB(1));
    //MemArenaInit(&memArena_Frame, MB(1));

    //// Initializing engine systems
    //InitTime();

    //// Creating window
    //appWindow = WindowCreate(&memArena_Perm, windowWidth, windowHeight, Str(appTitle));

    //// Initialize rendering context and rendering system
    //InitRenderer(appWindow);

    //// Display window
    //WindowShow(appWindow);

    // Initializing common engine systems
    Time_Init();

    // Initializing renderer
    Renderer_Init(windowWidth, windowHeight, appTitle, &appWindow);
}

void App_Update()
{
    //// Clear frame temporary memory
    //MemClear(&memArena_Frame);

    // Update common systems
    Input_UpdateState();
}

void App_Render()
{
    Renderer_RenderFrame();
}

void App_Destroy()
{
    //// Destroy app window
    //WindowDestroy(appWindow);
    Window_Destroy(appWindow);
}

} // namespace Ty
