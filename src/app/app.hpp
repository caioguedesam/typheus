#pragma once
//#include "core/base.hpp"
//#include "core/window.hpp"
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"

namespace Ty
{

//inline MemArena memArena_Perm;
//inline MemArena memArena_Frame;
//inline Window* appWindow;

inline Window appWindow;

void App_Init(u32 windowWidth, u32 windowHeight, const char* appTitle);

void App_Update();
void App_Render();

void App_Destroy();

} // namespace Ty
