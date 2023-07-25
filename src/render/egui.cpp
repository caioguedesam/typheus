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

VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;

void Init(Handle<render::RenderPass> hRenderPass)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    render::Context* ctx = &render::ctx;
    render::SwapChain* swapChain = &render::swapChain;

    ImGui_ImplWin32_Init(ctx->window->winHandle);

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
    VkResult ret = vkCreateDescriptorPool(ctx->vkDevice, &poolInfo, NULL, &vkDescriptorPool);
    ASSERTVK(ret);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = ctx->vkInstance;
    initInfo.PhysicalDevice = ctx->vkPhysicalDevice;
    initInfo.Device = ctx->vkDevice;
    initInfo.QueueFamily = ctx->vkCommandQueueFamily;
    initInfo.Queue = ctx->vkCommandQueue;
    initInfo.DescriptorPool = vkDescriptorPool;
    initInfo.MinImageCount = swapChain->vkImages.count;
    initInfo.ImageCount = swapChain->vkImages.count;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo, render::renderPasses[hRenderPass].vkHandle);

    // Create font atlas texture for ImGui
    Handle<render::CommandBuffer> hCmd = render::GetAvailableCommandBuffer(render::COMMAND_BUFFER_IMMEDIATE);
    render::BeginCommandBuffer(hCmd);
    ImGui_ImplVulkan_CreateFontsTexture(render::commandBuffers[hCmd].vkHandle);
    render::EndCommandBuffer(hCmd);
    render::SubmitImmediate(hCmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Shutdown()
{
    vkDeviceWaitIdle(render::ctx.vkDevice);
    vkDestroyDescriptorPool(render::ctx.vkDevice, vkDescriptorPool, NULL);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DrawFrame(Handle<render::CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    render::CommandBuffer& cmd = render::commandBuffers[hCmd];
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.vkHandle);
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
    ToCStr(text, cstr);
    ImGui::Text("%s", cstr);
}

bool Button(String label)
{
    ToCStr(label, labelcstr);
    return ImGui::Button(labelcstr);
}

void Checkbox(String label, bool* result)
{
    ToCStr(label, labelcstr);
    ImGui::Checkbox(labelcstr, result);
}

void DragI32(String label, i32* result, f32 speed, i32 start, i32 end)
{
    ToCStr(label, labelcstr);
    ImGui::DragInt(labelcstr, result, speed, start, end);
}

void DragF32(String label, f32* result, f32 speed, f32 start, f32 end)
{
    ToCStr(label, labelcstr);
    ImGui::DragFloat(labelcstr, result, speed, start, end);
}

void SliderI32(String label, i32* result, i32 start, i32 end)
{
    ToCStr(label, labelcstr);
    ImGui::SliderInt(labelcstr, result, start, end);
}

void SliderF32(String label, f32* result, f32 start, f32 end)
{
    ToCStr(label, labelcstr);
    ImGui::SliderFloat(labelcstr, result, start, end);
}

void SliderV2F(String label, math::v2f* result, f32 start, f32 end)
{
    ToCStr(label, labelcstr);
    ImGui::SliderFloat2(labelcstr, result->data, start, end);
}

void SliderV3F(String label, math::v3f* result, f32 start, f32 end)
{
    ToCStr(label, labelcstr);
    ImGui::SliderFloat3(labelcstr, result->data, start, end);
}

void SliderAngle(String label, f32* result_radians)
{
    ToCStr(label, labelcstr);
    ImGui::SliderAngle(labelcstr, result_radians);
}

void Color(String label, f32* r, f32* g, f32* b)
{
    ToCStr(label, labelcstr);
    f32 color[3] = {*r, *g, *b};
    ImGui::ColorEdit3(labelcstr, color);
    *r = color[0];
    *g = color[1];
    *b = color[2];
}

void Color(String label, f32* r, f32* g, f32* b, f32* a)
{
    ToCStr(label, labelcstr);
    f32 color[4] = {*r, *g, *b, *a};
    ImGui::ColorEdit4(labelcstr, color);
    *r = color[0];
    *g = color[1];
    *b = color[2];
    *a = color[3];
}

void Tooltip(String text)
{
    if(ImGui::IsItemHovered())
    {
        ToCStr(text, textcstr);
        ImGui::SetTooltip("%s", textcstr);
    }
}

};
};
