#pragma once
#include "core/base.hpp"
#include "core/window.hpp"

// Unity build (improving dramatically compile times)
// Dependencies
namespace Ty
{

inline MemArena memArena_Perm;
inline MemArena memArena_Frame;
inline Window* appWindow;

void InitApp(u32 windowWidth, u32 windowHeight, const char* appTitle);

void Update();

void DestroyApp();

} // namespace Ty
