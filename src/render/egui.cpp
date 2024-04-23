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

    ImGui_ImplWin32_Init(renderCtx->window->winHandle);

    Context ctx = {};
    ctx.renderCtx = renderCtx;

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

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("./resources/fonts/CascadiaCode.ttf", 16);
    //TODO(caio): CONTINUE
    // - Bug with ImGui cursor being slightly offset from real cursor.
    // Likely because ImGui is taking title bar into account when it shouldn't.
    RECT clientRect;
	//GetClientRect(window->winHandle, &clientRect);
	GetWindowRect(renderCtx->window->winHandle, &clientRect);
    io.DisplaySize = ImVec2(clientRect.right, clientRect.bottom);

    // Create font atlas texture for ImGui
    handle hCb = render::GetAvailableCommandBuffer(renderCtx, render::COMMAND_BUFFER_IMMEDIATE);
    render::BeginCommandBuffer(renderCtx, hCb);
    ImGui_ImplVulkan_CreateFontsTexture(renderCtx->commandBuffers[hCb].vkHandle);
    render::EndCommandBuffer(renderCtx, hCb);
    render::SubmitImmediate(renderCtx, hCb);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // Style
	// Comfy style by Giuseppe from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.1000000014901161f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 1.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(30.0f, 30.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(5.0f, 3.5f);
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(5.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 5.0f;
	style.ColumnsMinSpacing = 5.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 15.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.TabBorderSize = 1.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.3605149984359741f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.54935622215271f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3803921639919281f, 0.4235294163227081f, 0.572549045085907f, 0.5490196347236633f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2588235437870026f, 0.2588235437870026f, 0.2588235437870026f, 0.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2352941185235977f, 0.2352941185235977f, 0.2352941185235977f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509803950786591f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 0.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.2918455004692078f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433477878570557f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);

    return ctx;
}

void DestroyEGUIContext(Context* ctx)
{
    vkDeviceWaitIdle(ctx->renderCtx->vkDevice);
    vkDestroyDescriptorPool(ctx->renderCtx->vkDevice, ctx->vkDescriptorPool, NULL);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    *ctx = {};
}

void BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DrawFrame(Context* ctx, handle hCb)
{
    render::CommandBuffer& cb = ctx->renderCtx->commandBuffers[hCb];
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
