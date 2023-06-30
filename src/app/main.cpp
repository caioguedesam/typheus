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

    // Spot texture
    file::Path spotTexturePath = file::MakePath(IStr("./resources/textures/viking_room.png"));
    Handle<asset::Image> hSpotTextureAsset = asset::LoadImageFile(file::MakePath(IStr("./resources/models/spot/spot_texture.png")));
    asset::Image& spotTextureAsset = asset::images[hSpotTextureAsset];
    u64 spotTextureSize = spotTextureAsset.width * spotTextureAsset.height * 4;

    //  Create staging buffer to store image memory from CPU
    Handle<render::Buffer> hSpotTextureStagingBuffer = render::MakeBuffer(
            render::BUFFER_TYPE_STAGING,
            spotTextureSize,
            spotTextureSize,
            spotTextureAsset.data);
    render::CopyMemoryToBuffer(hSpotTextureStagingBuffer, spotTextureSize, spotTextureAsset.data);

    //  Create GPU texture
    render::TextureDesc spotTextureDesc = {};
    spotTextureDesc.type = render::IMAGE_TYPE_2D;
    spotTextureDesc.width = spotTextureAsset.width;
    spotTextureDesc.height = spotTextureAsset.height;
    spotTextureDesc.mipLevels = render::GetMaxMipLevels(spotTextureAsset.width, spotTextureAsset.height);
    spotTextureDesc.format = render::FORMAT_RGBA8_SRGB;
    spotTextureDesc.viewType = render::IMAGE_VIEW_TYPE_2D;
    spotTextureDesc.layout = render::IMAGE_LAYOUT_UNDEFINED;
    spotTextureDesc.usageFlags = ENUM_FLAGS(render::ImageUsageFlags,
            render::IMAGE_USAGE_SAMPLED
            | render::IMAGE_USAGE_TRANSFER_DST
            | render::IMAGE_USAGE_TRANSFER_SRC);
    Handle<render::Texture> hSpotTexture = render::MakeTexture(spotTextureDesc);

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
            hSpotTexture, 
            ty::render::IMAGE_LAYOUT_TRANSFER_DST,
            barrier);
    render::CmdCopyBufferToTexture(hCmd, hSpotTextureStagingBuffer, hSpotTexture);
    render::CmdGenerateMipmaps(hCmd, hSpotTexture);
    barrier.srcAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.dstAccess = render::MEMORY_ACCESS_SHADER_READ;
    barrier.srcStage = render::PIPELINE_STAGE_TRANSFER;
    barrier.dstStage = render::PIPELINE_STAGE_FRAGMENT_SHADER;
    render::CmdPipelineBarrierTextureLayout(
            hCmd,
            hSpotTexture, 
            ty::render::IMAGE_LAYOUT_SHADER_READ_ONLY,
            barrier);
    render::EndCommandBuffer(hCmd);
    render::SubmitImmediate(hCmd);

    // Default sampler
    render::SamplerDesc defaultSamplerDesc = {};
    Handle<render::Sampler> hDefaultSampler = MakeSampler(defaultSamplerDesc);

    // // Cube buffers
    // f32 cubeVertices[] =
    // {
    //     // Front face
    //     -1.f, -1.f, 1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //      1.f, -1.f, 1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //      1.f,  1.f, 1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //     -1.f,  1.f, 1.f, 0, 0, 1, 0.f, 0.f,   // TL

    //     // Back face
    //      1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //     -1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //     -1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //      1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

    //     // Top face
    //     -1.f,  1.f,  1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //      1.f,  1.f,  1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //      1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //     -1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

    //     // Bottom face
    //     -1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //      1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //      1.f, -1.f,  1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //     -1.f, -1.f,  1.f, 0, 0, 1, 0.f, 0.f,   // TL

    //     // Left face
    //     -1.f, -1.f, -1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //     -1.f, -1.f,  1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //     -1.f,  1.f,  1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //     -1.f,  1.f, -1.f, 0, 0, 1, 0.f, 0.f,   // TL

    //     // Right face
    //      1.f, -1.f,  1.f, 1, 0, 0, 0.f, 1.f,   // BL
    //      1.f, -1.f, -1.f, 0, 1, 0, 1.f, 1.f,   // BR
    //      1.f,  1.f, -1.f, 1, 1, 1, 1.f, 0.f,   // TR
    //      1.f,  1.f,  1.f, 0, 0, 1, 0.f, 0.f,   // TL
    // };

    // u32 cubeIndices[] =
    // {
    //     0, 1, 2, 0, 2, 3,
    //     4, 5, 6, 4, 6, 7,
    //     8, 9, 10, 8, 10, 11,
    //     12, 13, 14, 12, 14, 15,
    //     16, 17, 18, 16, 18, 19,
    //     20, 21, 22, 20, 22, 23,
    // };

    //Handle<render::Buffer> hCubeVB = render::MakeBuffer(render::BUFFER_TYPE_VERTEX, sizeof(cubeVertices), sizeof(f32), cubeVertices);
    //Handle<render::Buffer> hCubeIB = render::MakeBuffer(render::BUFFER_TYPE_INDEX, sizeof(cubeIndices), sizeof(u32), cubeIndices);

    //file::Path bunnyPath = file::MakePath(IStr("./resources/models/bunny/bunny.obj"));
    file::Path spotModelPath = file::MakePath(IStr("./resources/models/spot/spot.obj"));
    Handle<asset::Model> hSpotModelAsset = asset::LoadModelOBJ(spotModelPath);
    asset::Model& spotModelAsset = asset::models[hSpotModelAsset];
    Handle<render::Buffer> hSpotVB = render::MakeBuffer(
            render::BUFFER_TYPE_VERTEX,
            spotModelAsset.vertices.count * sizeof(f32),
            sizeof(f32),
            spotModelAsset.vertices.data);
    Handle<render::Buffer> hSpotIB = render::MakeBuffer(
            render::BUFFER_TYPE_INDEX,
            spotModelAsset.groups[0].indices.count * sizeof(u32),
            sizeof(u32),
            spotModelAsset.groups[0].indices.data);

    struct SceneData
    {
        math::m4f view = {};
        math::m4f proj = {};
    };
    SceneData sceneData;
    sceneData.view = math::Transpose(math::LookAt({0, 0, -5}, {0, 0, 0}, {0, 1, 0}));
    sceneData.proj = math::Transpose(math::Perspective(TO_RAD(45.f), (f32)appWidth/(f32)appHeight, 0.1f, 1000.f));

    struct PerInstanceData
    {
        math::m4f world = {};
        math::v4f color = {};
    };
    const i32 instanceCount = 512;
    PerInstanceData instanceData[instanceCount];
    // Initializing all instances to have random rotation and color
    for(i32 i = 0; i < instanceCount; i++)
    {
        //f32 randomRotationAngle = math::RandomUniformF32(-360.f, 360.f);
        //math::v3f randomRotationAxis =
        //{
            //math::RandomUniformF32(),
            //math::RandomUniformF32(),
            //math::RandomUniformF32(),
        //};
        //randomRotationAxis = math::Normalize(randomRotationAxis); 
        math::v3f randomPosition =
        {
            //math::RandomUniformF32(-10.f, 10.f),
            //math::RandomUniformF32(-10.f, 10.f),
            //math::RandomUniformF32(-10.f, 10.f),
            (f32)i * 0.01f, 
            (f32)i * 0.01f,
            (f32)i * 0.01f,
        };
        //instanceData[i].world = math::Transpose(math::TranslationMatrix(randomPosition) * math::RotationMatrix(randomRotationAngle, randomRotationAxis));
        instanceData[i].world = math::Transpose(math::TranslationMatrix(randomPosition));
        instanceData[i].color =
        {
            //math::RandomUniformF32(),
            //math::RandomUniformF32(),
            //math::RandomUniformF32(),
            1.f - ((f32)i / 200.f),
            1.f - ((f32)i / 200.f),
            1.f - ((f32)i / 200.f),
            1,
        };
    }

    Handle<render::Buffer> hSceneDataBuffer = render::MakeBuffer(
            render::BUFFER_TYPE_UNIFORM, 
            sizeof(SceneData),
            sizeof(SceneData), 
            &sceneData);
    Handle<render::Buffer> hInstanceDataBuffer = render::MakeBuffer(
            render::BUFFER_TYPE_UNIFORM, 
            sizeof(PerInstanceData) * instanceCount,
            sizeof(PerInstanceData), 
            &instanceData);

    // struct ObjectData
    // {
    //     math::m4f world = {};
    // };
    // ObjectData cube1Data;
    // ObjectData cube2Data;
    // cube2Data.world = math::Transpose(math::RotationMatrix(TO_RAD(-90.f), {0,1,0}) * math::RotationMatrix(TO_RAD(-90.f), {1,0,0}) * math::ScaleMatrix({2,2,2})) * math::Identity();
    // cube1Data.world = math::Transpose(math::TranslationMatrix({-5, 5, 5}) * math::Identity());
    // Handle<render::Buffer> hSceneDataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(SceneData), sizeof(SceneData), &sceneData);
    // Handle<render::Buffer> hCube1DataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(ObjectData), sizeof(ObjectData), &cube1Data);
    // Handle<render::Buffer> hCube2DataBuffer = render::MakeBuffer(render::BUFFER_TYPE_UNIFORM, sizeof(ObjectData), sizeof(ObjectData), &cube2Data);

    // Resource bindings
    render::ResourceBinding bindings[3];
    bindings[0].resourceType = render::RESOURCE_UNIFORM_BUFFER;
    bindings[0].stages = render::SHADER_TYPE_VERTEX;
    bindings[0].hBuffer = hSceneDataBuffer;
    bindings[1].resourceType = render::RESOURCE_SAMPLED_TEXTURE;
    bindings[1].stages = render::SHADER_TYPE_PIXEL;
    bindings[1].hTexture = hSpotTexture;
    bindings[1].hSampler = hDefaultSampler;
    bindings[2].resourceType = render::RESOURCE_UNIFORM_BUFFER;
    bindings[2].stages = ENUM_FLAGS(render::ShaderType,
            render::SHADER_TYPE_VERTEX | render::SHADER_TYPE_PIXEL);
    bindings[2].hBuffer = hInstanceDataBuffer;
    Handle<render::BindGroup> hBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, ARR_LEN(bindings), bindings);

    // bindings[0].hBuffer = hCube1DataBuffer;
    // Handle<render::BindGroup> hCube1DataBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 1, bindings);
    // bindings[0].hBuffer = hCube2DataBuffer;
    // Handle<render::BindGroup> hCube2DataBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 1, bindings);

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
        hBindGroup,
    };
    Handle<render::GraphicsPipeline> hGraphicsPipelineDefault = render::MakeGraphicsPipeline(hRenderPassMain,
            defaultPipelineDesc,
            ARR_LEN(hGraphicsPipelineBindGroups),
            hGraphicsPipelineBindGroups);

    //egui::Init(hRenderPassMain);

    i32 frame = 0;
    time::Timer frameTimer;
    math::v3f colorTest = {0, 0, 0};
    i32 testDragI32 = 0;
    f32 testDragF32 = 0.f;
    i32 testSliderI32 = 0;
    f32 testSliderF32 = 0.f;
    f32 testAngle = 0.f;
    while(window.state != render::WINDOW_CLOSED)
    {
        PROFILE_FRAME;
        frameTimer.Start();

        window.PollMessages();

        // Frame setup
        render::BeginFrame(frame);
        Handle<render::CommandBuffer> hCmd = render::GetAvailableCommandBuffer();
        render::BeginCommandBuffer(hCmd);

        //egui::BeginFrame();

        // Frame commands 
        Handle<render::Texture> hRenderPassMainOutput = render::GetRenderPassOutput(hRenderPassMain, 0);

        render::Barrier barrier = {};
        barrier.srcAccess = render::MEMORY_ACCESS_NONE;
        barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
        barrier.srcStage = render::PIPELINE_STAGE_TOP;
        barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
        render::CmdPipelineBarrierTextureLayout(hCmd, hRenderPassMainOutput, 
                ty::render::IMAGE_LAYOUT_TRANSFER_DST,
                barrier);
        //math::v3f clearColor =
        //{
            //math::Lerp(1, 0, (f32)(frame % 5000)/5000.f),
            //math::Lerp(0, 1, (f32)(frame % 5000)/5000.f),
            //1,
        //};
        //render::CmdClearColorTexture(hCmd, hRenderPassMainOutput, clearColor.r, clearColor.g, clearColor.b, 1);

        render::BeginRenderPass(hCmd, hRenderPassMain);
        render::CmdBindPipeline(hCmd, hGraphicsPipelineDefault);
        render::CmdSetViewport(hCmd, hRenderPassMain);
        render::CmdSetScissor(hCmd, hRenderPassMain);
        render::CmdBindResources(hCmd, hBindGroup, 0, hGraphicsPipelineDefault);

        // Draw all instances
        render::CmdBindVertexBuffer(hCmd, hSpotVB);
        render::CmdBindIndexBuffer(hCmd, hSpotIB);
        render::CmdDrawIndexed(hCmd, hSpotIB, 512);

        //// Draw cube 1
        //render::CmdBindVertexBuffer(cmd, hCubeVB);
        //render::CmdBindIndexBuffer(cmd, hCubeIB);
        //render::CmdBindResources(cmd, hCube1DataBindGroup, 1, hGraphicsPipelineDefault);
        //render::CmdDrawIndexed(cmd, hCubeIB);
        //// Draw cube 2
        //render::CmdBindResources(cmd, hCube2DataBindGroup, 1, hGraphicsPipelineDefault);
        //render::CmdBindVertexBuffer(cmd, hSpotVB);
        //render::CmdBindIndexBuffer(cmd, hSpotIB);
        //render::CmdDrawIndexed(cmd, hSpotIB);
        ////egui::ShowDemo();
        //egui::Text(IStr("Hi EGUI!"));
        //egui::Text(IStr("Hi EGUI!"));
        //egui::Color(IStr("Color test"), &colorTest.r, &colorTest.g, &colorTest.b);
        //egui::Tooltip(IStr("This is a color selector"));
        //egui::SliderAngle(IStr("Angle test"), &testAngle);
        //egui::SliderF32(IStr("Slider test"), &testSliderF32, 0.f, 1.f);
        //egui::SliderV3F(IStr("Slider v3f test"), &colorTest, 0.f, 1.f);
        //egui::DragI32(IStr("Drag i32 test"), &testDragI32, 0.1f);
        //if(egui::Button(IStr("Increment drag value")))
        //{
            //testDragI32++;
        //}
        //egui::DrawFrame(cmd);
        render::EndRenderPass(hCmd, hRenderPassMain);

        barrier.srcAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
        barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_READ;
        barrier.srcStage = render::PIPELINE_STAGE_TRANSFER;
        barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
        render::CmdPipelineBarrierTextureLayout(hCmd, hRenderPassMainOutput, 
                ty::render::IMAGE_LAYOUT_TRANSFER_SRC,
                barrier);
        render::CmdCopyToSwapChain(hCmd, hRenderPassMainOutput);

        // Frame teardown
        render::EndCommandBuffer(hCmd);
        render::EndFrame(frame, hCmd);

        // Present
        render::Present(frame);

        frameTimer.Stop();
        LOGF("Frame %d: %.4lf ms", frame, (f32)frameTimer.GetElapsedMS());
        frame++;
    }

    //egui::Shutdown();
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
// - Instancing*
//      > Test: many single models with individual instance data: 1 model matrix (64 bytes) + 1 color (16 bytes?)
// - Compute
// - Improving resource binding somehow?
//      > IMPORTANT: Per frame resources and descriptors (e.g. per frame UBO copied from CPU every frame)
// - Draw indirect?
// - Dynamic uniform buffers with offsets?
// - Runtime shader compilation and shader hot reload
//      > Will likely need to change asset lib a little
// - Window maximizing (vkCmdBlitImage can't upscale)
