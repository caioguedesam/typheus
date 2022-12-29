#pragma once
#include "engine/common/common.hpp"

namespace Ty
{

// This currently supports: Win32 windows, with OpenGL context.
// TODO(caio)#PLATFORM: Add support for X11 windows, and Vulkan rendering contexts when needed.
struct Window
{
    u32 width           = 0;
    u32 height          = 0;
    HWND handle         = NULL;
    HDC deviceContext   = NULL;
    HGLRC glContext     = NULL;

    bool shouldClose    = false;
};

void    Window_Init(u32 w, u32 h, const char* name, Window* outWindow);
void    Window_InitRenderContext(Window& window);
void    Window_Destroy(Window& window);
void    Window_Show(const Window& window);

void    Window_ProcessMessages(const Window& window);
void    Window_SwapBuffers(const Window& window);

}   // namespace Ty
