#pragma once
#include "engine/renderer/window.hpp"
#include "imgui.h"

namespace Ty
{

void GUI_Init(Window* window, const char* fontAssetPath, u32 fontSize);
void GUI_BeginFrame();
void GUI_EndFrame();

} // namespace Ty
