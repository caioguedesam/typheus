#include "engine/renderer/window.hpp"
#include "glad/glad.h"
#include "wglext.h"

namespace Ty
{

std::unordered_map<HWND, Window*> activeWindows;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = activeWindows[hWnd];
    switch(uMsg)
    {
        case WM_CLOSE:
            {
                window->shouldClose = true;
            } break;
        case WM_SIZE:
            {
                window->width = LOWORD(lParam);
                window->height = HIWORD(lParam);
            }; break;
        default: break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Window_Init(u32 w, u32 h, const char* name, Window* outWindow)
{
    *outWindow = {};

    const char* windowClassName = "TypheusWindowClass";
    // Creating and registering window class
    {
        WNDCLASSA windowClass = {};
        windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        windowClass.lpszClassName = windowClassName;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = GetModuleHandle(NULL);

        RegisterClassA(&windowClass);
    }

    outWindow->handle = CreateWindowEx(
            0,
            windowClassName,
            name,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            w, h,
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL);
    ASSERT(outWindow->handle);

    outWindow->deviceContext = GetDC(outWindow->handle);
    ASSERT(outWindow->deviceContext);

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
    i32 ret = SetPixelFormat(outWindow->deviceContext,
            ChoosePixelFormat(outWindow->deviceContext, &pfd),
            &pfd);

    outWindow->width = w;
    outWindow->height = h;

    // Adding new window to active window list
    activeWindows[outWindow->handle] = outWindow;
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

void Window_Destroy(Window& window)
{
    ASSERT(window.handle);
    if(!activeWindows.count(window.handle)) return;

    // Destroy openGL context
    if(window.glContext)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(window.glContext);
    }

    // Destroying device context
    if(window.deviceContext)
    {
        DeleteDC(window.deviceContext);
    }

    // Destroying OS window
    DestroyWindow(window.handle);

    activeWindows.erase(window.handle);

    window = {};
}

void Window_InitRenderContext(Window& window)
{
    ASSERT(window.handle && window.deviceContext); 
    ASSERT(!window.glContext);

    // Win32 requires us to create a legacy OpenGL context before creating
    // an OpenGL context with the desired version (4.6)
    Window tempWindow;
    Window_Init(0,0,"", &tempWindow);

    tempWindow.glContext = wglCreateContext(tempWindow.deviceContext);
    ASSERT(tempWindow.glContext);

    i32 ret = wglMakeCurrent(tempWindow.deviceContext, tempWindow.glContext);
    ASSERT(ret);

    ret = gladLoadGLLoader((GLADloadproc)GLGetProc);
    ASSERT(ret);

    const i32 attributeList[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if _DEBUG
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0,
    };

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    ASSERT(wglCreateContextAttribsARB);

    Window_Destroy(tempWindow);

    window.glContext = wglCreateContextAttribsARB(window.deviceContext, NULL, attributeList);
    ASSERT(window.glContext);

    ret = wglMakeCurrent(window.deviceContext, window.glContext);
    ASSERT(ret);
}

void Window_Show(const Window& window)
{
    ASSERT(window.handle && window.deviceContext && window.glContext);
    ShowWindow(window.handle, SW_SHOWNORMAL);
}

void Window_ProcessMessages(const Window& window)
{
    MSG msg = {};
    while(true)
    {
        i32 ret = PeekMessage(&msg, window.handle, 0, 0, PM_REMOVE);
        ASSERT(ret >= 0);
        if(!ret) break;
        DispatchMessage(&msg);
    }
}

void Window_SwapBuffers(const Window& window)
{
    SwapBuffers(window.deviceContext);
    PROFILE_GPU_FRAME;
}

}   // namespace Ty
