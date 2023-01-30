// [UNITY BUILD]
// Including all app source files here to improve build times by using only one compilation unit.

// [DEPENDENCIES]
// Implementation defines
#define STB_IMAGE_IMPLEMENTATION
#define FAST_OBJ_IMPLEMENTATION
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM     // Need this for unity build since
                                            // ImGui has own OpenGL function loader

// Header files
#if _PROFILE
#include "tracy/Tracy.hpp"
#endif
#include "glad/glad.h"
#include "stb_image.h"
#include "fast_obj.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl3.h"

#undef STB_IMAGE_IMPLEMENTATION
#undef FAST_OBJ_IMPLEMENTATION
// Source files
#if _PROFILE
#include "TracyClient.cpp"
#endif
#include "glad/glad.c"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "backends/imgui_impl_win32.cpp"
#include "backends/imgui_impl_opengl3.cpp"
#undef IMGUI_IMPL_OPENGL_LOADER_CUSTOM

// [PROJECT]
// Header files
#include "engine/common/common.hpp"
#include "engine/common/asset.hpp"
#include "engine/renderer/window.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/renderer/gui.hpp"

#include "app/app.hpp"

// Source files

#include "engine/common/common.cpp"
#include "engine/common/asset.cpp"
#include "engine/renderer/window.cpp"
#include "engine/renderer/renderer.cpp"
#include "engine/renderer/gui.cpp"

#include "app/app.cpp"

// ===============================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Ty::App_Init(1280, 720, "Test app");

    while(!Ty::appWindow.shouldClose)
    {
        Ty::Window_ProcessMessages(Ty::appWindow);
        Ty::App_Update();
        Ty::App_Render();
    }

    Ty::App_Destroy();
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
