// [UNITY BUILD]
// Including all app source files here to improve build times by using only one compilation unit.

// [DEPENDENCIES]
// Implementation defines
#define STB_IMAGE_IMPLEMENTATION
#define FAST_OBJ_IMPLEMENTATION

// Header files
#if _PROFILE
#include "tracy/Tracy.hpp"
#endif
#include "glad/glad.h"
#include "stb_image.h"
#include "fast_obj.h"

#undef STB_IMAGE_IMPLEMENTATION
#undef FAST_OBJ_IMPLEMENTATION
// Source files
#if _PROFILE
#include "TracyClient.cpp"
#endif
#include "glad/glad.c"

// [PROJECT]
// Header files
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"
#include "engine/renderer/renderer.hpp"

#include "app/app.hpp"

// Source files

#include "engine/common/common.cpp"
#include "engine/renderer/window.cpp"
#include "engine/renderer/renderer.cpp"

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
        Ty::Window_SwapBuffers(Ty::appWindow);
    }

    Ty::App_Destroy();
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
