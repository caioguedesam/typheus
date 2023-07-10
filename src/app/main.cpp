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

    mem::HeapAllocator generalHeap = mem::MakeHeapAllocator(MB(128));

    // Shaders
    file::Path vsPath = file::MakePath(IStr("./build/default_quad_vert.spv"));
    file::Path psPath = file::MakePath(IStr("./build/default_quad_frag.spv"));
    file::Path csPath = file::MakePath(IStr("./build/update_instances_comp.spv"));
    Handle<asset::BinaryData> hAssetVs = asset::LoadBinaryFile(vsPath);
    Handle<asset::BinaryData> hAssetPs = asset::LoadBinaryFile(psPath);
    Handle<asset::BinaryData> hAssetCs = asset::LoadBinaryFile(csPath);
    asset::BinaryData& assetVs = asset::binaryDatas[hAssetVs];
    asset::BinaryData& assetPs = asset::binaryDatas[hAssetPs];
    asset::BinaryData& assetCs = asset::binaryDatas[hAssetCs];
    Handle<render::Shader> hVsDefault = render::MakeShader(render::SHADER_TYPE_VERTEX, assetVs.size, assetVs.data);
    Handle<render::Shader> hPsDefault = render::MakeShader(render::SHADER_TYPE_PIXEL, assetPs.size, assetPs.data);
    Handle<render::Shader> hCsDefault = render::MakeShader(render::SHADER_TYPE_COMPUTE, assetCs.size, assetCs.data);

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
    Handle<render::CommandBuffer> hUploadCmd = render::GetAvailableCommandBuffer(render::COMMAND_BUFFER_IMMEDIATE);
    render::BeginCommandBuffer(hUploadCmd);
    render::Barrier barrier = {};
    barrier.srcAccess = render::MEMORY_ACCESS_NONE;
    barrier.dstAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.srcStage = render::PIPELINE_STAGE_TOP;
    barrier.dstStage = render::PIPELINE_STAGE_TRANSFER;
    render::CmdPipelineBarrierTextureLayout(
            hUploadCmd,
            hSpotTexture, 
            ty::render::IMAGE_LAYOUT_TRANSFER_DST,
            barrier);
    render::CmdCopyBufferToTexture(hUploadCmd, hSpotTextureStagingBuffer, hSpotTexture);
    render::CmdGenerateMipmaps(hUploadCmd, hSpotTexture);
    barrier.srcAccess = render::MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.dstAccess = render::MEMORY_ACCESS_SHADER_READ;
    barrier.srcStage = render::PIPELINE_STAGE_TRANSFER;
    barrier.dstStage = render::PIPELINE_STAGE_FRAGMENT_SHADER;
    render::CmdPipelineBarrierTextureLayout(
            hUploadCmd,
            hSpotTexture, 
            ty::render::IMAGE_LAYOUT_SHADER_READ_ONLY,
            barrier);
    render::EndCommandBuffer(hUploadCmd);
    render::SubmitImmediate(hUploadCmd);

    // Default sampler
    render::SamplerDesc defaultSamplerDesc = {};
    Handle<render::Sampler> hDefaultSampler = MakeSampler(defaultSamplerDesc);

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
    const i32 instanceCount = 4096;
    //PerInstanceData instanceData[instanceCount];
    mem::SetContext(&generalHeap);
    Array<PerInstanceData> instanceData = MakeArray<PerInstanceData>(instanceCount, instanceCount, {});
    // Initializing all instances to have random rotation and color
    for(i32 i = 0; i < instanceCount; i++)
    {
        math::v3f randomPosition =
        {
            //(f32)i * 0.01f, 
            //(f32)i * 0.01f,
            //(f32)i * 0.01f,
            math::RandomUniformF32(-10.f, 10.f),
            math::RandomUniformF32(-10.f, 10.f),
            math::RandomUniformF32(10.f, 5.f),
        };
        //instanceData[i].world = math::Transpose(math::TranslationMatrix(randomPosition) * math::RotationMatrix(randomRotationAngle, randomRotationAxis));
        instanceData[i].world = math::Transpose(math::TranslationMatrix(randomPosition));
        instanceData[i].color =
        {
            //1.f - ((f32)i / 200.f),
            //1.f - ((f32)i / 200.f),
            //1.f - ((f32)i / 200.f),
            math::RandomUniformF32(),
            math::RandomUniformF32(),
            math::RandomUniformF32(),
            1,
        };
    }

    Handle<render::Buffer> hSceneDataBuffer = render::MakeBuffer(
            render::BUFFER_TYPE_UNIFORM, 
            sizeof(SceneData),
            sizeof(SceneData), 
            &sceneData);
    Handle<render::Buffer> hInstanceDataBuffer = render::MakeBuffer(
            //render::BUFFER_TYPE_UNIFORM, 
            render::BUFFER_TYPE_STORAGE, 
            sizeof(PerInstanceData) * instanceCount,
            sizeof(PerInstanceData), 
            instanceData.data);

    // Resource bindings
    render::ResourceBinding bindings[3];
    bindings[0].resourceType = render::RESOURCE_UNIFORM_BUFFER;
    bindings[0].stages = render::SHADER_TYPE_VERTEX;
    bindings[0].hBuffer = hSceneDataBuffer;
    bindings[1].resourceType = render::RESOURCE_SAMPLED_TEXTURE;
    bindings[1].stages = render::SHADER_TYPE_PIXEL;
    bindings[1].hTexture = hSpotTexture;
    bindings[1].hSampler = hDefaultSampler;
    bindings[2].resourceType = render::RESOURCE_STORAGE_BUFFER;
    bindings[2].stages = ENUM_FLAGS(render::ShaderType,
            render::SHADER_TYPE_VERTEX | render::SHADER_TYPE_PIXEL);
    bindings[2].hBuffer = hInstanceDataBuffer;
    Handle<render::BindGroup> hBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, ARR_LEN(bindings), bindings);

    render::ResourceBinding csBinding = {};
    csBinding.resourceType = render::RESOURCE_STORAGE_BUFFER;
    csBinding.stages = render::SHADER_TYPE_COMPUTE;
    csBinding.hBuffer = hInstanceDataBuffer;
    Handle<render::BindGroup> hCsBindGroup = render::MakeBindGroup(render::RESOURCE_BINDING_STATIC, 1, &csBinding);

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

    // Compute pipeline
    Handle<render::ComputePipeline> hComputePipelineDefault = render::MakeComputePipeline(hCsDefault, 1, &hCsBindGroup);

    //egui::Init(hRenderPassMain);

    i32 frame = 0;
    time::Timer frameTimer;
    while(window.state != render::WINDOW_CLOSED)
    {
        PROFILE_FRAME;
        frameTimer.Start();

        window.PollMessages();

        // Frame setup
        Handle<render::CommandBuffer> hCmd = render::GetAvailableCommandBuffer(render::COMMAND_BUFFER_FRAME, frame);
        render::BeginFrame(frame);
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
        math::v3f clearColor =
        {
            math::Lerp(1, 0, (f32)(frame % 5000)/5000.f),
            math::Lerp(0, 1, (f32)(frame % 5000)/5000.f),
            1,
        };
        render::CmdClearColorTexture(hCmd, hRenderPassMainOutput, clearColor.r, clearColor.g, clearColor.b, 1);

        // Update colors with compute pass
        render::CmdBindComputePipeline(hCmd, hComputePipelineDefault);
        render::CmdBindComputeResources(hCmd, hCsBindGroup, 0, hComputePipelineDefault);
        render::CmdDispatch(hCmd, instanceCount / 16, 1, 1);

        render::BeginRenderPass(hCmd, hRenderPassMain);
        render::CmdBindGraphicsPipeline(hCmd, hGraphicsPipelineDefault);
        render::CmdSetViewport(hCmd, hRenderPassMain);
        render::CmdSetScissor(hCmd, hRenderPassMain);
        render::CmdBindGraphicsResources(hCmd, hBindGroup, 0, hGraphicsPipelineDefault);

        // Draw all instances
        render::CmdBindVertexBuffer(hCmd, hSpotVB);
        render::CmdBindIndexBuffer(hCmd, hSpotIB);
        render::CmdDrawIndexed(hCmd, hSpotIB, instanceCount);

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

    mem::SetContext(&generalHeap);
    DestroyArray(&instanceData);
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
// - Compute*
//      > Barrier between compute and draw (buffer needs to update before drawing)
// - Fix unavailable command buffer assert bug
// - Improving resource binding somehow?
//      > IMPORTANT: Per frame resources and descriptors (e.g. per frame UBO copied from CPU every frame)
//          > This might be better to just use offsets instead of one resource per frame
//      > Push constants
// - Dynamic uniform buffers with offsets?
// - Upscale when resizing
//      > Do a render to texture instead of blit?
// - Make typheus into a submodule
// - Draw indirect?
// - Runtime shader compilation and shader hot reload
//      > Will likely need to change asset lib a little
// - Window maximizing (vkCmdBlitImage can't upscale)
