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
void WindowInitGLContext(Window* window);
void WindowDestroy(Window* window);

void WindowShow(Window* window);

void WindowPollMessages(Window* window);
void WindowSwapBuffers(Window* window);

}   // namespace Sol
