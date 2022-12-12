// Project headers
#include "base.hpp"
#include "input.hpp"
#include "window.hpp"
#include <stdio.h>

// Compiling just one file to dramatically speed up compile times
// Dependencies
#include "glad/glad.h"
#include "glad/glad.c"
#if _PROFILE
#include "tracy/TracyClient.cpp"
#endif

// Project files
#include "base.cpp"
#include "time.cpp"
#include "math.cpp"
#include "file.cpp"
#include "input.cpp"
#include "window.cpp"

#define APP_NAME "Solanum"
#define APP_WINDOWCLASS "SolanumWindowClass"

void Update()
{
    UpdateInputState();

    v2i pos = GetMousePosition();
    v2f delta = GetMouseDelta();
    if(delta.x < -EPSILON_F32 || delta.x > EPSILON_F32
    || delta.y < -EPSILON_F32 || delta.y > EPSILON_F32)
    {
        printf("Cursor position: %d %d (%.2f,%.2f)\n", pos.x, pos.y, delta.x, delta.y);
    }
}

void Render()
{
    glClearColor(1.f, 0.5f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Window window = WindowCreate(800, 600, Str("Test window"));
    WindowInitGLContext(&window);
    WindowShow(&window);

    while(!shouldQuit)
    {
        WindowPollMessages(&window);
        Update();
        Render();
        WindowSwapBuffers(&window);
    }

    WindowDestroy(&window);
}

int main()
{
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);

    // First Core API pass layout:
    // int main()
    // {
    //      // First, initialize core systems for OS
    //      Sol::InitWin32();
    //      
    //      // Then, create window to render on top of
    //      Sol::Window window = Sol::CreateWindow(w, h, "name");
    //
    //      // Initialize rendering with associated window (and possibly other systems)
    //      Sol::InitRenderer(window);
    //
    //      // Create any kind of user defined stuffs for the application (load models, textures, shaders, place objects in scene, set configurations, etc)
    //      // TODO(caio)#API: This will require further refinement in core API (for mem alloc, file access, etc) but I'll do this whenever I have
    //      // more relevant user land code
    //      
    //      // Application loop
    //      while(!window.closed)
    //      {
    //          Sol::ProcessMessages(window);    // Poll and process any messages sent from OS to window
    //          Sol::Update();                   // Update app state
    //          Sol::Render();                   // Render app state
    //          Sol::SwapBuffers(window);        // Swap backbuffers to render next frame
    //      }
    //
    //      // Teardown previously initialized systems
    //      Sol::DestroyRenderer();
    //      
    //      // Destroy window and finish application
    //      Sol::DestroyWindow(&window);
    //
    //      return 0;
    // }
}
