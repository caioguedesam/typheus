// [UNITY BUILD]
// Including all app source files here to improve build times by using only one compilation unit.

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
