#include "solanum.hpp"
#include "solanum.cpp"

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Sol::InitPlatform();
    Sol::Window* window = Sol::WindowCreate(&Sol::memArena_Perm, 800, 600, Sol::Str("TestApp"));
    Sol::InitRenderer(window);
    Sol::WindowShow(window);

    while(!window->shouldClose)
    {
        Sol::ProcessMessages(window);
        Sol::Update();
        Sol::Render();
        Sol::SwapBuffers(window);
    }

    Sol::WindowDestroy(window);
    Sol::DestroyRenderer();
    Sol::DestroyPlatform();
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
