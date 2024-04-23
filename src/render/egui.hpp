// ========================================================
// EDITOR GUI
// Functions for editor GUI implementation, to be used along with
// the rendering API. Currently uses Dear ImGui.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "../core/string.hpp"
#include "../core/math.hpp"
#include "./render.hpp"
#include "./window.hpp"

namespace ty
{
namespace egui
{

struct Context
{
    render::Context* renderCtx = NULL;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
};

Context MakeEGUIContext(render::Context* renderCtx, handle hRenderPass);
void DestroyEGUIContext(Context* ctx);

void SameLine();
void Text(String text);
bool Button(String label);
void Checkbox(String label, bool* result);
void DragI32(String label, i32* result, f32 speed = 1.f, i32 start = 0, i32 end = 100);
void DragF32(String label, f32* result, f32 speed = 1.f, f32 start = 0, f32 end = 100);
void SliderI32(String label, i32* result, i32 start, i32 end);
void SliderF32(String label, f32* result, f32 start, f32 end);
void SliderV2F(String label, math::v2f* result, f32 start, f32 end);
void SliderV3F(String label, math::v3f* result, f32 start, f32 end);
void SliderAngle(String label, f32* result_radians);
void Color(String label, f32* r, f32* g, f32* b);
void Color(String label, f32* r, f32* g, f32* b, f32* a);
void Tooltip(String text);

void ShowDemo();

//TODO(caio): Add support for more, such as displaying textures and hierarchies.

void BeginFrame();
void DrawFrame(Context* ctx, handle hCb);

};
};
