#include "render/renderer.hpp"
#include "glad/glad.h"

namespace Ty
{

void InitRenderer(Window* window)
{
    // TODO(caio)#RENDER: Init stuff like glViewport and other render state, allocate resources, etc.
    InitGLContext(window);
}

void RenderFrame()
{
    glClearColor(1.f, 0.5f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace Ty
