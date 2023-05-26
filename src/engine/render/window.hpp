// ========================================================
// WINDOW
// OS-specific Window types and procedures, interfaces with Vulkan renderer.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"

namespace ty
{
namespace render
{

enum WindowState
{
    WINDOW_INVALID,
    WINDOW_IDLE,
    WINDOW_RESIZING,
    WINDOW_MINIMIZED,
    WINDOW_CLOSED,
};

struct Window
{
    HWND handle = NULL;
    i32 w = 0;
    i32 h = 0;
    WindowState state = WINDOW_INVALID;

    void PollMessages();
};

void    InitWindow(Window* window, i32 w, i32 h, const char* title);
void    DestroyWindow(Window* window);
void    ResizeWindow(Window* window);

};
};

LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
