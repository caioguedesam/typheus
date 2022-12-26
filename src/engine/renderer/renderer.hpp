#pragma once
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"

namespace Ty
{

struct MeshVertex
{
    v3f position = {};
    v3f normal = {};
    v2f texcoord = {};
};

void    Renderer_Init(u32 windowWidth, u32 windowHeight, const char* windowName, Window* outWindow);

void    Renderer_RenderFrame();

} // namespace Ty
