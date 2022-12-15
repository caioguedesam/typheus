#pragma once

// Unity build (improving dramatically compile times)
// Dependencies
#include "glad/glad.h"
#include "glad/glad.c"
#if _PROFILE
#include "tracy/TracyClient.cpp"
#endif

// Source files
#include "core/base.cpp"
#include "core/time.cpp"
#include "core/math.cpp"
#include "core/file.cpp"
#include "core/input.cpp"
#include "core/window.cpp"

namespace Ty
{

inline MemArena memArena_Perm;
inline MemArena memArena_Frame;

void InitPlatform();
void InitRenderer(Window* window);

void Update();
void Render();

void DestroyPlatform();
void DestroyRenderer();

} // namespace Ty
