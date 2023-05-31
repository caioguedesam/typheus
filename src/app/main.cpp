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
    asset::Init();

    const u32 appWidth = 1280;
    const u32 appHeight = 720;

    render::Window window;
    render::MakeWindow(&window, appWidth, appHeight, "Typheus");

    render::Init(&window);

    // Main render pass (output will be copied to swap chain image)
    render::RenderPassDesc mainRenderPassDesc = {};
    mainRenderPassDesc.width = appWidth;
    mainRenderPassDesc.height = appHeight;
    mainRenderPassDesc.loadOp = render::LOAD_OP_CLEAR;
    mainRenderPassDesc.storeOp = render::STORE_OP_STORE;
    mainRenderPassDesc.initialLayout = render::IMAGE_LAYOUT_UNDEFINED;
    mainRenderPassDesc.finalLayout = render::IMAGE_LAYOUT_TRANSFER_SRC;
    render::Format mainRenderPassColorFormats[] =
    {
        render::FORMAT_RGBA8_SRGB,
    };
    Handle<render::RenderPass> mainRenderPass = render::MakeRenderPass(mainRenderPassDesc, 1, mainRenderPassColorFormats, render::FORMAT_D32_FLOAT);

    // Shaders
    file::Path testShaderPath = file::MakePath(IStr("./resources/shaders/test.spv"));
    Handle<asset::BinaryData> h_assetTestShader = asset::LoadBinaryFile(testShaderPath);
    asset::BinaryData& assetTestShader = asset::assetDatabase.binaryDataAssets[h_assetTestShader];
    Handle<render::Shader> testShader = render::MakeShader(ty::render::SHADER_TYPE_VERTEX, assetTestShader.size, assetTestShader.data);

    // Graphics pipeline 
    render::VertexAttribute defaultVertexAttributes[] =
    {
        render::VERTEX_ATTR_V3F,
        render::VERTEX_ATTR_V3F,
        render::VERTEX_ATTR_V2F,
    };
    Handle<render::VertexLayout> defaultVertexLayout = render::MakeVertexLayout(ARR_LEN(defaultVertexAttributes), defaultVertexAttributes);

    // Program flow:
    // > Render to main render pass
    // > Transition swap chain image to transfer dst
    // > Copy main render pass output image to swap chain image
    // > Transition swap chain image to present src
    // > Present

    i32 frame = 0;
    time::Timer frameTimer;
    while(window.state != render::WINDOW_CLOSED)
    {
        PROFILE_FRAME;
        frameTimer.Start();

        window.PollMessages();

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
