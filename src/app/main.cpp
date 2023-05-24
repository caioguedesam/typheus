// Typheus app uses a unity build system to improve compilation
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/memory.hpp"
#include "engine/core/string.hpp"
#include "engine/core/math.hpp"
#include "engine/core/time.hpp"
#include "engine/core/input.hpp"
#include "engine/core/file.hpp"
#include "engine/core/async.hpp"
#include "engine/core/ds.hpp"

#include "engine/asset/asset.hpp"

#include "engine/core/debug.cpp"
#include "engine/core/memory.cpp"
#include "engine/core/string.cpp"
#include "engine/core/math.cpp"
#include "engine/core/time.cpp"
#include "engine/core/input.cpp"
#include "engine/core/file.cpp"
#include "engine/core/async.cpp"
#include "engine/core/ds.cpp"

#include "engine/asset/asset.cpp"

#include "engine/core/tests.cpp"
#include "engine/asset/tests.cpp"

// ===============================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    //Ty::App_Init(1280, 720, "Test app");

    //while(!Ty::appWindow.shouldClose)
    //{
      //Ty::Window_ProcessMessages(Ty::appWindow);
      //Ty::App_Update();
      //Ty::App_Render();
    //}

    //Ty::App_Destroy();

    ty::TestCore();
    ty::TestAssets();
    return 0;

}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
