#include "typheus.hpp"
#include "typheus.cpp"

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Ty::InitPlatform();
    Ty::Window* window = Ty::WindowCreate(&Ty::memArena_Perm, 800, 600, Ty::Str("TestApp"));
    Ty::InitRenderer(window);
    Ty::WindowShow(window);

    while(!window->shouldClose)
    {
        Ty::ProcessMessages(window);
        Ty::Update();
        Ty::Render();
        Ty::SwapBuffers(window);
    }

    Ty::WindowDestroy(window);
    Ty::DestroyRenderer();
    Ty::DestroyPlatform();
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
