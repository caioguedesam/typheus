#include "window.hpp"
#include "glad/glad.h"
#include "wglext.h"

namespace Sol
{

#define WINDOW_CLASS_NAME "WindowClass"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch(uMsg)
    {
    case WM_CLOSE:
    {
        window->shouldClose = true;
    } break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Window* WindowCreate(MemArena* arena, u32 width, u32 height, String title)
{
    // Creating and registering window class
    {
        WNDCLASSA windowClass = {};
        windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        windowClass.lpszClassName = WINDOW_CLASS_NAME;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = GetModuleHandle(NULL);

        RegisterClassA(&windowClass);
    }

    Window* result = (Window*)MemAlloc(arena, sizeof(Window));
    result->handle = CreateWindowEx(
            0,
            WINDOW_CLASS_NAME,
            ToCStr(title),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width,
            height,
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL);
    ASSERT(result->handle);

    // Passing Window instance to WindowProc
    SetWindowLongPtr(result->handle, GWLP_USERDATA, (LONG_PTR)result);

    result->deviceContext = GetDC(result->handle);
    ASSERT(result->deviceContext);

    PIXELFORMATDESCRIPTOR pfd =
	{
	    sizeof(PIXELFORMATDESCRIPTOR),
	    1,
	    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
	    PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
	    32,                   // Colordepth of the framebuffer.
	    0, 0, 0, 0, 0, 0,
	    0,
	    0,
	    0,
	    0, 0, 0, 0,
	    24,                   // Number of bits for the depthbuffer
	    8,                    // Number of bits for the stencilbuffer
	    0,                    // Number of Aux buffers in the framebuffer.
	    PFD_MAIN_PLANE,
	    0,
	    0, 0, 0
	};
    i32 ret = SetPixelFormat(result->deviceContext,
            ChoosePixelFormat(result->deviceContext, &pfd),
            &pfd);

    return result;
}

void* GLGetProc(const char* fn)
{
    HMODULE lib = LoadLibraryA("opengl32.dll");
    ASSERT(lib);

    // First try to fetch procedure from OpenGL 1.1 and below (from opengl32.dll)
    void* proc = (void*)GetProcAddress(lib, fn);
    if(proc) return proc;
    // If not found, then it's OpenGL 1.2 or above
    proc = (void*)wglGetProcAddress(fn);
    ASSERT(proc);

    return proc;
}

void WindowInitGLContext(Window* window)
{
    ASSERT(!window->glContext);

    // Win32 requires us to create a legacy OpenGL context before creating
    // an OpenGL context with the desired version (4.6)
    MemArena tempArena;
    MemArenaInit(&tempArena, sizeof(Window));
    Window* tempWindow = WindowCreate(&tempArena, 0,0,Str(""));

    tempWindow->glContext = wglCreateContext(tempWindow->deviceContext);
    ASSERT(tempWindow->glContext);

    i32 ret = wglMakeCurrent(tempWindow->deviceContext, tempWindow->glContext);
    ASSERT(ret);

    ret = gladLoadGLLoader((GLADloadproc)GLGetProc);
    ASSERT(ret);

    const i32 attributeList[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    ASSERT(wglCreateContextAttribsARB);

    WindowDestroy(tempWindow);
    MemArenaDestroy(&tempArena);

    window->glContext = wglCreateContextAttribsARB(window->deviceContext, NULL, attributeList);
    ASSERT(window->glContext);

    ret = wglMakeCurrent(window->deviceContext, window->glContext);
    ASSERT(ret);
}

void WindowDestroy(Window* window)
{
    // Destroy openGL context
    if(window->glContext)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(window->glContext);
    }

    // Destroying device context
    if(window->deviceContext)
    {
        DeleteDC(window->deviceContext);
    }

    // Destroying window
    DestroyWindow(window->handle);

    *window = {};
}

void WindowShow(Window* window)
{
    ShowWindow(window->handle, SW_SHOWNORMAL);
}

void WindowPollMessages(Window* window)
{
    MSG msg = {};
    while(true)
    {
        i32 ret = PeekMessage(&msg, window->handle, 0, 0, PM_REMOVE);
        ASSERT(ret >= 0);
        if(!ret) break;
        DispatchMessage(&msg);
    }
}

void WindowSwapBuffers(Window* window)
{
    SwapBuffers(window->deviceContext);
}

}   // namespace Sol
