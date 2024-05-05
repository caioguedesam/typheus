#include "../core/string.hpp"
#include "./egui.hpp"
#include "./render.hpp"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_vulkan.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
#include "vulkan/vulkan_core.h"

namespace ty
{
namespace egui
{

Context MakeEGUIContext(render::Context* renderCtx, handle hRenderPass)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    RECT clientRect;
	GetWindowRect(renderCtx->window->winHandle, &clientRect);
    io.DisplaySize = ImVec2((f32)(clientRect.right - clientRect.left), (f32)(clientRect.bottom - clientRect.top));
    io.DisplayFramebufferScale = ImVec2(io.DisplaySize.x/renderCtx->window->w,
        io.DisplaySize.y/renderCtx->window->h);

    ImGui_ImplWin32_Init(renderCtx->window->winHandle);

    Context ctx = {};

    // Descriptor pool just for ImGui
    // Sizes copied from ImGui demo
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = ARR_LEN(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1000;
    VkResult ret = vkCreateDescriptorPool(renderCtx->vkDevice, &poolInfo, NULL, &ctx.vkDescriptorPool);
    ASSERTVK(ret);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = renderCtx->vkInstance;
    initInfo.PhysicalDevice = renderCtx->vkPhysicalDevice;
    initInfo.Device = renderCtx->vkDevice;
    initInfo.QueueFamily = renderCtx->vkCommandQueueFamily;
    initInfo.Queue = renderCtx->vkCommandQueue;
    initInfo.DescriptorPool = ctx.vkDescriptorPool;
    initInfo.MinImageCount = renderCtx->swapChain.vkImages.count;
    initInfo.ImageCount = renderCtx->swapChain.vkImages.count;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo, renderCtx->renderPasses[hRenderPass].vkHandle);


    // Create font atlas texture for ImGui
    handle hCb = render::GetAvailableCommandBuffer(renderCtx, render::COMMAND_BUFFER_IMMEDIATE);
    render::BeginCommandBuffer(renderCtx, hCb);
    ImGui_ImplVulkan_CreateFontsTexture(renderCtx->commandBuffers[hCb].vkHandle);
    render::EndCommandBuffer(renderCtx, hCb);
    render::SubmitImmediate(renderCtx, hCb);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return ctx;
}

void DestroyEGUIContext(Context* ctx, render::Context* renderCtx)
{
    vkDeviceWaitIdle(renderCtx->vkDevice);
    vkDestroyDescriptorPool(renderCtx->vkDevice, ctx->vkDescriptorPool, NULL);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    *ctx = {};
}

void BeginFrame(render::Context* renderCtx)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
}

void DrawFrame(render::Context* renderCtx, handle hCb)
{
    render::CommandBuffer& cb = renderCtx->commandBuffers[hCb];
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb.vkHandle);
}

void ShowDemo()
{
    ImGui::ShowDemoWindow();
}

void SameLine()
{
    return ImGui::SameLine();
}

void Text(String text)
{
    ImGui::Text("%s", text.CStr());
}

bool Button(String label)
{
    return ImGui::Button(label.CStr());
}

void Checkbox(String label, bool* result)
{
    ImGui::Checkbox(label.CStr(), result);
}

void DragI32(String label, i32* result, f32 speed, i32 start, i32 end)
{
    ImGui::DragInt(label.CStr(), result, speed, start, end);
}

void DragF32(String label, f32* result, f32 speed, f32 start, f32 end)
{
    ImGui::DragFloat(label.CStr(), result, speed, start, end);
}

void SliderI32(String label, i32* result, i32 start, i32 end)
{
    ImGui::SliderInt(label.CStr(), result, start, end);
}

void SliderF32(String label, f32* result, f32 start, f32 end)
{
    ImGui::SliderFloat(label.CStr(), result, start, end);
}

void SliderV2F(String label, math::v2f* result, f32 start, f32 end)
{
    ImGui::SliderFloat2(label.CStr(), result->data, start, end);
}

void SliderV3F(String label, math::v3f* result, f32 start, f32 end)
{
    ImGui::SliderFloat3(label.CStr(), result->data, start, end);
}

void SliderAngle(String label, f32* result_radians)
{
    ImGui::SliderAngle(label.CStr(), result_radians);
}

void Color(String label, f32* r, f32* g, f32* b)
{
    f32 color[3] = {*r, *g, *b};
    ImGui::ColorEdit3(label.CStr(), color);
    *r = color[0];
    *g = color[1];
    *b = color[2];
}

void Color(String label, f32* r, f32* g, f32* b, f32* a)
{
    f32 color[4] = {*r, *g, *b, *a};
    ImGui::ColorEdit4(label.CStr(), color);
    *r = color[0];
    *g = color[1];
    *b = color[2];
    *a = color[3];
}

void Tooltip(String text)
{
    if(ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text.CStr());
    }
}

};
};
