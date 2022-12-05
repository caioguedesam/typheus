// TYPHEUS ENGINE
// Project headers
#include "base.hpp"
#include "input.hpp"
#include <stdio.h>

// Compiling just one file to dramatically speed up compile times
// Dependencies
#if _PROFILE
#include "TracyClient.cpp"
#endif

// Project files
#include "base.cpp"
#include "time.cpp"
#include "math.cpp"
#include "file.cpp"
#include "input.cpp"

#define APP_NAME "Typheus"
#define APP_WINDOWCLASS "TypheusWindowClass"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    // Window class
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = APP_WINDOWCLASS;
    RegisterClassA(&windowClass);

    // Window
    HWND hWnd = CreateWindowEx(
            0,
            APP_WINDOWCLASS,
            APP_NAME,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL, NULL,
            hInstance,
            NULL);
    ASSERT(hWnd);

    ShowWindow(hWnd, nCmdShow);

    while(true)
    {
        // Message loop
        MSG msg = {};
        while(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) > 0)
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
    }
}

int main()
{
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
