#include "typheus.cpp"

// TODO(caio)#PLATFORM: This is a Windows only main.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    Ty::InitApp(1280, 720, "TestApp");

    while(!Ty::appWindow->shouldClose)
    {
        Ty::ProcessMessages(Ty::appWindow);
        Ty::Update();
        Ty::RenderFrame();
        Ty::SwapBuffers(Ty::appWindow);
    }

    Ty::DestroyApp();
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}
