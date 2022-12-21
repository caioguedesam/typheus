#pragma once
#include "core/window.hpp"
#include "core/math.hpp"

namespace Ty
{

struct MeshVertex
{
    v3f position = {};
    v3f normal = {};
    v2f texcoord = {};
};

void InitRenderer(Window* window);

void RenderFrame();

} // namespace Ty
