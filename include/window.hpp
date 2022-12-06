// CORE - WINDOW HANDLING
#pragma once
#include "base.hpp"

// This currently supports: Win32 windows, with OpenGL context.
// TODO(caio)#PLATFORM: Add support for X11 windows, and Vulkan rendering contexts when needed.
struct Window
{
    HWND handle;
    HDC deviceContext;
    HGLRC glContext;
};

Window WindowCreate(u32 width, u32 height, String title);
void WindowInitGLContext(Window* window);
void WindowDestroy(Window* window);

void WindowShow(Window* window);

void WindowPollMessages(Window* window);
void WindowSwapBuffers(Window* window);
