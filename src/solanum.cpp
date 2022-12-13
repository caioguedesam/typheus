#include "solanum.hpp"

namespace Sol
{

void InitPlatform()
{
    // Initialize memory arenas
    MemArenaInit(&memArena_Perm, MB(1));
    MemArenaInit(&memArena_Frame, MB(1));

    // Initialize engine systems
    InitTime();
}

void InitRenderer(Window* window)
{
    // Start OpenGL context
    InitGLContext(window);
}

void Update()
{
    // Clear frame temporary memory
    MemClear(&memArena_Frame);

    // Update engine systems
    UpdateInputState();
}

void Render()
{
    glClearColor(1.f, 0.5f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void DestroyPlatform() {}

void DestroyRenderer() {}

} // namespace Sol
