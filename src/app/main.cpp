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
    mainRenderPassDesc.loadOp = render::LOAD_OP_LOAD;
    mainRenderPassDesc.storeOp = render::STORE_OP_STORE;
    mainRenderPassDesc.initialLayout = render::IMAGE_LAYOUT_TRANSFER_DST;
    mainRenderPassDesc.finalLayout = render::IMAGE_LAYOUT_TRANSFER_SRC;
    render::Format mainRenderPassColorFormats[] =
    {
        render::FORMAT_RGBA8_SRGB,
    };
    Handle<render::RenderPass> hRenderPassMain = render::MakeRenderPass(mainRenderPassDesc, 1, mainRenderPassColorFormats, render::FORMAT_D32_FLOAT);

    // Shaders
    file::Path vsPath = file::MakePath(IStr("./resources/shaders/bin/default_quad_vert.spv"));
    file::Path psPath = file::MakePath(IStr("./resources/shaders/bin/default_quad_frag.spv"));
    Handle<asset::BinaryData> hAssetVs = asset::LoadBinaryFile(vsPath);
    Handle<asset::BinaryData> hAssetPs = asset::LoadBinaryFile(psPath);
    asset::BinaryData& assetVs = asset::binaryDatas[hAssetVs];
    asset::BinaryData& assetPs = asset::binaryDatas[hAssetPs];
    Handle<render::Shader> hVsDefault = render::MakeShader(render::SHADER_TYPE_VERTEX, assetVs.size, assetVs.data);
    Handle<render::Shader> hPsDefault = render::MakeShader(render::SHADER_TYPE_PIXEL, assetPs.size, assetPs.data);

    // Buffers
    f32 quadVertices[] =
    {
        -0.5f, -0.5f, 0.5f, 0, 0, 0, 0.f, 1.f,   // BL
         0.5f, -0.5f, 0.5f, 0, 0, 0, 1.f, 1.f,   // BR
         0.5f,  0.5f, 0.5f, 0, 0, 0, 1.f, 0.f,   // TR
        -0.5f,  0.5f, 0.5f, 0, 0, 0, 0.f, 0.f,   // TL
    };
    
    u32 quadIndices[] =
    {
        0, 1, 2, 0, 2, 3,
    };
    Handle<render::Buffer> hQuadVB = render::MakeBuffer(render::BUFFER_TYPE_VERTEX, sizeof(quadVertices), sizeof(f32), quadVertices);
    Handle<render::Buffer> hQuadIB = render::MakeBuffer(render::BUFFER_TYPE_INDEX, sizeof(quadIndices), sizeof(u32), quadIndices);

    // Graphics pipeline 
    render::VertexAttribute defaultVertexAttributes[] =
    {
        render::VERTEX_ATTR_V3F,
        render::VERTEX_ATTR_V3F,
        render::VERTEX_ATTR_V2F,
    };
    Handle<render::VertexLayout> hVertexLayoutDefault = render::MakeVertexLayout(ARR_LEN(defaultVertexAttributes), defaultVertexAttributes);

    render::GraphicsPipelineDesc defaultPipelineDesc = {};
    defaultPipelineDesc.hVertexLayout = hVertexLayoutDefault;
    defaultPipelineDesc.hShaderVertex = hVsDefault;
    defaultPipelineDesc.hShaderPixel = hPsDefault;
    defaultPipelineDesc.frontFace = render::FRONT_FACE_CCW;
    defaultPipelineDesc.cullMode = render::CULL_MODE_BACK;
    defaultPipelineDesc.primitive = render::PRIMITIVE_TRIANGLE_LIST;
    defaultPipelineDesc.fillMode = render::FILL_MODE_SOLID;
    Handle<render::GraphicsPipeline> hGraphicsPipelineDefault = render::MakeGraphicsPipeline(hRenderPassMain, defaultPipelineDesc);

    i32 frame = 0;
    time::Timer frameTimer;
    while(window.state != render::WINDOW_CLOSED)
    {
        PROFILE_FRAME;
        frameTimer.Start();

        window.PollMessages();

        // Frame setup
        render::BeginFrame(frame);
        Handle<render::CommandBuffer> cmd = render::GetAvailableCommandBuffer();
        render::BeginCommandBuffer(cmd);

        // Frame commands 
        Handle<render::Texture> hRenderPassMainOutput = render::GetRenderPassOutput(hRenderPassMain, 0);

        render::Barrier barrier = {};
        barrier.srcAccess = render::MEMORY_ACCESS_NONE;
        barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
        barrier.srcStage = render::PIPELINE_STAGE_TOP;
        barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
        render::CmdPipelineBarrierTextureLayout(cmd, hRenderPassMainOutput, 
                ty::render::IMAGE_LAYOUT_TRANSFER_DST,
                barrier);
        math::v3f clearColor =
        {
            math::Lerp(1, 0, (f32)(frame % 5000)/5000.f),
            math::Lerp(0, 1, (f32)(frame % 5000)/5000.f),
            1,
        };
        render::CmdClearColorTexture(cmd, hRenderPassMainOutput, clearColor.r, clearColor.g, clearColor.b, 1);

        render::BeginRenderPass(cmd, hRenderPassMain);
        //TODO(caio): Rendering commands here
        render::EndRenderPass(cmd, hRenderPassMain);

        barrier.srcAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
        barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_READ;
        barrier.srcStage = render::PIPELINE_STAGE_TRANSFER;
        barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
        render::CmdPipelineBarrierTextureLayout(cmd, hRenderPassMainOutput, 
                ty::render::IMAGE_LAYOUT_TRANSFER_SRC,
                barrier);
        render::CmdCopyToSwapChain(cmd, hRenderPassMainOutput);

        // Frame teardown
        render::EndCommandBuffer(cmd);
        render::EndFrame(frame, cmd);

        // Present
        render::Present(frame);

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
