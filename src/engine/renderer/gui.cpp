#include "engine/renderer/gui.hpp"
#include "engine/renderer/window.hpp"

#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl3.h"

namespace Ty
{

void GUI_Init(Window* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(window->handle);
    ImGui_ImplOpenGL3_Init("#version 460");

    ImGui::StyleColorsDark();
}

void GUI_BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void GUI_EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace Ty
