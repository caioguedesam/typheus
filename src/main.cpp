// TYPHEUS ENGINE
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

#define APP_NAME "Typheus"
#define APP_WINDOWCLASS "TypheusWindowClass"

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Window window = WindowCreate(800, 600, Str("Test window"));
    WindowInitGLContext(&window);

    ShowWindow(window.handle, nCmdShow);

    while(true)
    {
        // Message loop
        MSG msg = {};
        while(PeekMessage(&msg, window.handle, 0, 0, PM_REMOVE) > 0)
        {
            DispatchMessage(&msg);
        }

        // Update input
        UpdateInputState();

        v2i pos = GetMousePosition();
        v2f delta = GetMouseDelta();
        if(delta.x < -EPSILON_F32 || delta.x > EPSILON_F32
        || delta.y < -EPSILON_F32 || delta.y > EPSILON_F32)
        {
            printf("Cursor position: %d %d (%.2f,%.2f)\n", pos.x, pos.y, delta.x, delta.y);
        }

        glClearColor(1.f, 0.5f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        WindowSwapBuffers(&window);
    }
}

int main()
{
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
