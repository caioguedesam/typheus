// Typheus app uses a unity build system to improve compilation
// ===============================================================
// [HEADER FILES]
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/profile.hpp"
#include "engine/core/memory.hpp"
#include "engine/core/string.hpp"
#include "engine/core/math.hpp"
#include "engine/core/time.hpp"
#include "engine/core/input.hpp"
#include "engine/core/file.hpp"
#include "engine/core/async.hpp"
#include "engine/core/ds.hpp"
#include "engine/asset/asset.hpp"
#include "engine/render/window.hpp"
#include "engine/render/render.hpp"

// ===============================================================
// [SOURCE FILES]
#include "engine/core/debug.cpp"
#include "engine/core/memory.cpp"
#include "engine/core/string.cpp"
#include "engine/core/math.cpp"
#include "engine/core/time.cpp"
#include "engine/core/input.cpp"
#include "engine/core/file.cpp"
#include "engine/core/async.cpp"
#include "engine/core/ds.cpp"
#include "engine/core/tests.cpp"
#include "engine/asset/asset.cpp"
#include "engine/asset/tests.cpp"
#include "engine/render/window.cpp"
#include "engine/render/render.cpp"

// ===============================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    using namespace ty;

    time::Init();

    render::Window window;
    render::InitWindow(&window, 800, 600, "Typheus");
    render::Init(&window);

    i32 frame = 0;
    time::Timer frameTimer;
    while(window.state != render::WINDOW_CLOSED)
    {
        frameTimer.Start();
        PROFILE_FRAME;
        window.PollMessages();
        if(!frame)
        {
            TestCore();
            TestAssets();
        }
        frameTimer.Stop();
        LOGF("Frame %d: %.4lf ms", frame, (f32)frameTimer.GetElapsedMS());
        frame++;
    }

    render::Shutdown();
    render::DestroyWindow(&window);

    PROFILE_END;
    return 0;
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
