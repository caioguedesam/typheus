#pragma once
#include "core/base.hpp"

namespace Sol
{

// This currently supports: Win32 windows, with OpenGL context.
// TODO(caio)#PLATFORM: Add support for X11 windows, and Vulkan rendering contexts when needed.
struct Window
{
    HWND handle;
    HDC deviceContext;
    HGLRC glContext;

    bool shouldClose = false;
};

Window* WindowCreate(MemArena* arena, u32 width, u32 height, String title);
void    WindowDestroy(Window* window);
void    InitGLContext(Window* window);

void WindowShow(Window* window);
void ProcessMessages(Window* window);
void SwapBuffers(Window* window);

}   // namespace Sol
