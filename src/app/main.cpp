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
#include "engine/render/egui.hpp"

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
#include "engine/render/egui.cpp"

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

    mem::HeapAllocator generalHeap = mem::MakeHeapAllocator(MB(1));

    // Shaders
    file::Path vsPath = file::MakePath(IStr("./build/default_quad_vert.spv"));
    file::Path psPath = file::MakePath(IStr("./build/default_quad_frag.spv"));
    Handle<asset::BinaryData> hAssetVs = asset::LoadBinaryFile(vsPath);
    Handle<asset::BinaryData> hAssetPs = asset::LoadBinaryFile(psPath);
    asset::BinaryData& assetVs = asset::binaryDatas[hAssetVs];
    asset::BinaryData& assetPs = asset::binaryDatas[hAssetPs];
    Handle<render::Shader> hVsDefault = render::MakeShader(render::SHADER_TYPE_VERTEX, assetVs.size, assetVs.data);
    Handle<render::Shader> hPsDefault = render::MakeShader(render::SHADER_TYPE_PIXEL, assetPs.size, assetPs.data);

    // Checker texture
    //file::Path checkersPath = file::MakePath(IStr("./resources/textures/checkers.png"));
    file::Path checkersPath = file::MakePath(IStr("./resources/textures/viking_room.png"));
    Handle<asset::Image> hAssetCheckersTexture = asset::LoadImageFile(checkersPath, true);
    asset::Image& assetCheckersTexture = asset::images[hAssetCheckersTexture];
    u64 assetCheckersTextureSize = assetCheckersTexture.width * assetCheckersTexture.height * 4;

    //  Create staging buffer to store image memory from CPU
    Handle<render::Buffer> hCheckersTextureStagingBuffer = render::MakeBuffer(
            render::BUFFER_TYPE_STAGING,
            assetCheckersTextureSize,
            assetCheckersTextureSize,
            assetCheckersTexture.data);
    render::CopyMemoryToBuffer(hCheckersTextureStagingBuffer, assetCheckersTextureSize, assetCheckersTexture.data);

    //  Create GPU texture
    render::TextureDesc checkersTextureDesc = {};
    checkersTextureDesc.type = render::IMAGE_TYPE_2D;
    checkersTextureDesc.width = assetCheckersTexture.width;
    checkersTextureDesc.height = assetCheckersTexture.height;
    checkersTextureDesc.mipLevels = render::GetMaxMipLevels(assetCheckersTexture.width, assetCheckersTexture.height);
    checkersTextureDesc.format = render::FORMAT_RGBA8_SRGB;
    checkersTextureDesc.viewType = render::IMAGE_VIEW_TYPE_2D;
    checkersTextureDesc.layout = render::IMAGE_LAYOUT_UNDEFINED;
    checkersTextureDesc.usageFlags = ENUM_FLAGS(render::ImageUsageFlags,
            render::IMAGE_USAGE_SAMPLED
            | render::IMAGE_USAGE_TRANSFER_DST
            | render::IMAGE_USAGE_TRANSFER_SRC);
    Handle<render::Texture> hCheckersTexture = render::MakeTexture(checkersTextureDesc);

    //  Copy texture memory from CPU on staging buffer to GPU texture
    Handle<render::CommandBuffer> hCmd = render::GetAvailableCommandBuffer();
    render::BeginCommandBuffer(hCmd);
    render::Barrier barrier = {};
    barrier.srcAccess = render::MEMORY_ACCESS_NONE;
    barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.srcStage = render::PIPELINE_STAGE_TOP;
    barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
    render::CmdPipelineBarrierTextureLayout(
            hCmd,
            hCheckersTexture, 
            ty::render::IMAGE_LAYOUT_TRANSFER_DST,
            barrier);
    render::CmdCopyBufferToTexture(hCmd, hCheckersTextureStagingBuffer, hCheckersTexture);
    render::CmdGenerateMipmaps(hCmd, hCheckersTexture);
    barrier.srcAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.dstAccess = render::MEMORY_ACCESS_SHADER_READ;
    barrier.srcStage = render::PIPELINE_STAGE_TRANSFER;
    barrier.dstStage = render::PIPELINE_STAGE_FRAGMENT_SHADER;
    render::CmdPipelineBarrierTextureLayout(
            hCmd,
            hCheckersTexture, 
            ty::render::IMAGE_LAYOUT_SHADER_READ_ONLY,
            barrier);
    render::EndCommandBuffer(hCmd);
    render::SubmitImmediate(hCmd);

    // Default sampler
    render::SamplerDesc defaultSamplerDesc = {};
    Handle<render::Sampler> hDefaultSampler = MakeSampler(defaultSamplerDesc);

    // Cube buffers
    f32 cubeVertices[] =
    {
        // Front face
        -1.f, -1.f, 1.f, 1, 0, 0, 0.f, 1.f,   // BL
         1.f, -1.f, 1.f, 0, 1, 0, 1.f, 1.f,   // BR
         1.f,  1.f, 1.f, 1, 1, 1, 1.f, 0.f,   // TR
        -1.f,  1.f, 1.f, 0, 0, 1, 0.f, 0.f,   // TL

        // Back face
         1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
        -1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
        -1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
         1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

        // Top face
        -1.f,  1.f,  1.f, 1, 0, 0, 0.f, 1.f,   // BL
         1.f,  1.f,  1.f, 0, 1, 0, 1.f, 1.f,   // BR
         1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
        -1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

        // Bottom face
        -1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
         1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
         1.f, -1.f,  1.f, 1, 1, 1, 1.f, 0.f,   // TR
        -1.f, -1.f,  1.f, 0, 0, 1, 0.f, 0.f,   // TL

        // Left face
        -1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
        -1.f, -1.f,  1.f, 0, 1, 0, 1.f, 1.f,   // BR
        -1.f,  1.f,  1.f, 1, 1, 1, 1.f, 0.f,   // TR
        -1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

        // Right face
         1.f, -1.f,  1.f, 1, 0, 0, 0.f, 1.f,   // BL
         1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
         1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
         1.f,  1.f,  1.f, 0, 0, 1, 0.f, 0.f,   // TL
    };

    u32 cubeIndices[] =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    Handle<render::Buffer> hCubeVB = render::MakeBuffer(render::BUFFER_TYPE_VERTEX, sizeof(cubeVertices), sizeof(f32), cubeVertices);
    Handle<render::Buffer> hCubeIB = render::MakeBuffer(render::BUFFER_TYPE_INDEX, sizeof(cubeIndices), sizeof(u32), cubeIndices);

    //file::Path bunnyPath = file::MakePath(IStr("./resources/models/bunny/bunny.obj"));
    file::Path bunnyPath = file::MakePath(IStr("./resources/models/viking/viking_room.obj"));
    Handle<asset::Model> hAssetBunnyModel = asset::LoadModelOBJ(bunnyPath);
    asset::Model& assetBunnyModel = asset::models[hAssetBunnyModel];
    Handle<render::Buffer> hBunnyVB = render::MakeBuffer(
            render::BUFFER_TYPE_VERTEX,
            assetBunnyModel.vertices.count * sizeof(f32),
            sizeof(f32),
            assetBunnyModel.vertices.data);
    Handle<render::Buffer> hBunnyIB = render::MakeBuffer(
            render::BUFFER_TYPE_INDEX,
            assetBunnyModel.groups[0].indices.count * sizeof(u32),
            sizeof(u32),
            assetBunnyModel.groups[0].indices.data);

    struct SceneData
    {
        math::m4f view = {};
        math::m4f proj = {};
    };
    SceneData sceneData;
    //sceneData.view = math::Transpose(math::LookAt({2, 2, 5}, {0, 0, 0}, {0, 1, 0}));
    sceneData.view = math::Transpose(math::LookAt({1, 3, 5}, {0, 0, 0}, {0, 1, 0}));
    sceneData.proj = math::Transpose(math::Perspective(TO_RAD(45.f), (f32)appWidth/(f32)appHeight, 0.1f, 1000.f));

    struct ObjectData
    {
        math::m4f world = {};
    };
    ObjectData cube1Data;
    ObjectData cube2Data;
    cube2Data.world = math::Transpose(math::RotationMatrix(TO_RAD(-90.f), {0,1,0}) * math::RotationMatrix(TO_RAD(-90.f), {1,0,0}) * math::ScaleMatrix({2,2,2})) * math::Identity();
    cube1Data.world = math::Transpose(math::TranslationMatrix({-5, 5, 5}) * math::Identity());
    Handle<render::Buffer> hSceneDataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(SceneData), sizeof(SceneData), &sceneData);
    Handle<render::Buffer> hCube1DataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(ObjectData), sizeof(ObjectData), &cube1Data);
    Handle<render::Buffer> hCube2DataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(ObjectData), sizeof(ObjectData), &cube2Data);

    // Resource bindings
    render::ResourceBinding bindings[2];
    bindings[0].resourceType = render::RESOURCE_UNIFORM_BUFFER;
    bindings[0].stages = render::SHADER_TYPE_VERTEX;
    bindings[0].hBuffer = hSceneDataBuffer;
    bindings[1].resourceType = render::RESOURCE_SAMPLED_TEXTURE;
    bindings[1].stages = render::SHADER_TYPE_PIXEL;
    bindings[1].hTexture = hCheckersTexture;
    bindings[1].hSampler = hDefaultSampler;
    Handle<render::BindGroup> hSceneDataBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 2, bindings);

    bindings[0].hBuffer = hCube1DataBuffer;
    Handle<render::BindGroup> hCube1DataBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 1, bindings);
    bindings[0].hBuffer = hCube2DataBuffer;
    Handle<render::BindGroup> hCube2DataBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 1, bindings);

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
    Handle<render::BindGroup> hGraphicsPipelineBindGroups[] =
    {
        //TODO(caio): I feel there might be a better way to set "compatible"
        //bind group layouts instead of passing bind groups directly
        //(hCube1DataBindGroup is compatible with hCube2DataBindGroup)
        hSceneDataBindGroup, hCube1DataBindGroup,
    };
    Handle<render::GraphicsPipeline> hGraphicsPipelineDefault = render::MakeGraphicsPipeline(hRenderPassMain,
            defaultPipelineDesc,
            ARR_LEN(hGraphicsPipelineBindGroups),
            hGraphicsPipelineBindGroups);

    egui::Init(hRenderPassMain);

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

        egui::BeginFrame();

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
        render::CmdBindPipeline(cmd, hGraphicsPipelineDefault);
        render::CmdSetViewport(cmd, hRenderPassMain);
        render::CmdSetScissor(cmd, hRenderPassMain);
        //render::CmdBindResources(cmd, hBindGroup, 0, hGraphicsPipelineDefault);
        render::CmdBindResources(cmd, hSceneDataBindGroup, 0, hGraphicsPipelineDefault);
        // Draw cube 1
        render::CmdBindVertexBuffer(cmd, hCubeVB);
        render::CmdBindIndexBuffer(cmd, hCubeIB);
        render::CmdBindResources(cmd, hCube1DataBindGroup, 1, hGraphicsPipelineDefault);
        render::CmdDrawIndexed(cmd, hCubeIB);
        // Draw cube 2
        render::CmdBindResources(cmd, hCube2DataBindGroup, 1, hGraphicsPipelineDefault);
        render::CmdBindVertexBuffer(cmd, hBunnyVB);
        render::CmdBindIndexBuffer(cmd, hBunnyIB);
        render::CmdDrawIndexed(cmd, hBunnyIB);
        egui::ShowDemo();
        egui::DrawFrame(cmd);
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

    egui::Shutdown();
    render::Shutdown();
    render::DestroyWindow(&window);

    mem::DestroyHeapAllocator(&generalHeap);

    PROFILE_END;
    return 0;
}

int main()
{
    // TODO(caio)#PLATFORM: Hack to show console along with window app.
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}

// TODO(caio): CONTINUE
// - ImGui
// - Shader hot reload
// - Instancing
// - Compute
// - Dynamic uniform buffers with offsets?
// - Improving resource binding somehow?
// - Window maximizing (vkCmdBlitImage can't upscale)
