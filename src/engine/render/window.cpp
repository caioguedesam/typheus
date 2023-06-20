#include "engine/render/window.hpp"

namespace ty
{
namespace render
{

void MakeWindow(Window* window, i32 w, i32 h, const char* title)
{
    ASSERT(window);
    ASSERT(window->state == WINDOW_INVALID);
    WNDCLASSA windowClass = {};
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpszClassName = "wndclassname";
    windowClass.lpfnWndProc = Win32WndProc;
    windowClass.hInstance = GetModuleHandle(NULL);  // Active executable
    RegisterClassA(&windowClass);

    HWND handle = CreateWindowEx(
            0, windowClass.lpszClassName,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            w, h,
            NULL, NULL,
            windowClass.hInstance,
            window);
    ASSERT(handle);

    window->winHandle = handle;
    window->winInstance = windowClass.hInstance;
    window->w = w;
    window->h = h;
    window->state = WINDOW_IDLE;

    //// WndProc needs to be able to access and modify the window
    //LONG_PTR wndprocptr = SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)window);
    //ASSERT(wndprocptr);

    ShowWindow(handle, SW_SHOWNORMAL);
}

void DestroyWindow(Window* window)
{
    DestroyWindow(window->winHandle);
}

void Window::PollMessages()
{
    MSG msg = {};
    while(true)
    {
        i32 ret = PeekMessage(&msg, winHandle, 0, 0, PM_REMOVE);
        ASSERT(ret >= 0);
        if(!ret) break;
        DispatchMessage(&msg);
    }
}

};
};

void Win32WndProcSetWindow(HWND hwnd, LPARAM lparam)
{
    using namespace ty;
    using namespace render;

    CREATESTRUCT* cs = (CREATESTRUCT*)lparam;
    Window* window = (Window*)(cs->lpCreateParams);
    // Win32 is stupid: SetWindowLongPtr can return 0 and still be a success...
    SetLastError(0);
    i32 ret = SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    i32 err = GetLastError();
    if(!ret) ASSERT(!err);
}

LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    using namespace ty;
    using namespace render;
    if(umsg == WM_NCCREATE)
    {
        Win32WndProcSetWindow(hwnd, lparam);
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }

    Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!window)
    {
        // Win32 is stupid: somehow other WM messages may come before WM_CREATE/WM_NCCREATE...
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }

    switch(umsg)
    {
        case WM_CLOSE:
            {
                ASSERT(window->state != WINDOW_INVALID);
                window->state = WINDOW_CLOSED;
                return FALSE;
            } break;
        case WM_SIZE:
            {
                ASSERT(window->state != WINDOW_INVALID);
                window->w = LOWORD(lparam);
                window->h = HIWORD(lparam);
                // Resize needs to be consumed by renderer,
                // which sets window state back to IDLE
                window->state = WINDOW_RESIZING;
            }; break;
        default: break;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
