// ========================================================
// RENDER
// Real-time rendering API implemented with Vulkan.
// Will support more features in the future, such as compute and raytracing?
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/memory.hpp"
#include "engine/core/ds.hpp"
#include "engine/render/window.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"
#include "vma/vk_mem_alloc.h"

namespace ty
{
namespace render
{

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)

#define RENDER_CONTEXT_MEMORY MB(1)
#define RENDER_CONCURRENT_FRAMES 2
#define RENDER_MAX_COMMAND_BUFFERS 16
#define RENDER_MAX_RENDER_PASSES 8
#define RENDER_MAX_VERTEX_LAYOUTS 8
#define RENDER_MAX_SHADERS 32
#define RENDER_MAX_BUFFERS 256
#define RENDER_MAX_TEXTURES 1024
#define RENDER_MAX_GRAPHICS_PIPELINES 32

enum Format
{
    FORMAT_INVALID              = VK_FORMAT_UNDEFINED,
    FORMAT_RGBA8_SRGB           = VK_FORMAT_R8G8B8A8_SRGB,
    FORMAT_BGRA8_SRGB           = VK_FORMAT_B8G8R8A8_SRGB,
    FORMAT_RG32_FLOAT           = VK_FORMAT_R32G32_SFLOAT,
    FORMAT_RGB32_FLOAT          = VK_FORMAT_R32G32B32_SFLOAT,
    FORMAT_D32_FLOAT            = VK_FORMAT_D32_SFLOAT,
};

enum ImageLayout
{
    IMAGE_LAYOUT_UNDEFINED                  = VK_IMAGE_LAYOUT_UNDEFINED,
    IMAGE_LAYOUT_GENERAL                    = VK_IMAGE_LAYOUT_GENERAL,
    IMAGE_LAYOUT_COLOR_OUTPUT               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    IMAGE_LAYOUT_PRESENT_SRC                = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    IMAGE_LAYOUT_TRANSFER_SRC               = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    IMAGE_LAYOUT_TRANSFER_DST               = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    IMAGE_LAYOUT_SHADER_READ_ONLY           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    IMAGE_LAYOUT_DEPTH_STENCIL_OUTPUT       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
};

enum LoadOp
{
    LOAD_OP_DONT_CARE   = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    LOAD_OP_LOAD        = VK_ATTACHMENT_LOAD_OP_LOAD,
    LOAD_OP_CLEAR       = VK_ATTACHMENT_LOAD_OP_CLEAR,
};

enum StoreOp
{
    STORE_OP_DONT_CARE  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    STORE_OP_STORE      = VK_ATTACHMENT_STORE_OP_STORE,
};

enum VertexAttribute
{
    VERTEX_ATTR_INVALID = 0,
    VERTEX_ATTR_V2F,
    VERTEX_ATTR_V3F,

    VERTEX_ATTR_COUNT,
};

enum ShaderType
{
    SHADER_TYPE_VERTEX  = VK_SHADER_STAGE_VERTEX_BIT,
    SHADER_TYPE_PIXEL   = VK_SHADER_STAGE_FRAGMENT_BIT,
    //TODO(caio): Support compute
};

enum BufferType
{
    BUFFER_TYPE_VERTEX  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    BUFFER_TYPE_INDEX   = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    //TODO(caio): Uniform buffers, storage buffers
};

enum ImageType
{
    IMAGE_TYPE_2D       = VK_IMAGE_TYPE_2D,
};

enum ImageViewType
{
    IMAGE_VIEW_TYPE_2D  = VK_IMAGE_VIEW_TYPE_2D,
};

enum ImageUsageFlags : u32
{
    IMAGE_USAGE_COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    IMAGE_USAGE_DEPTH_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    IMAGE_USAGE_TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    IMAGE_USAGE_TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    IMAGE_USAGE_SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT,
};

enum Primitive
{
    PRIMITIVE_TRIANGLE_LIST     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    //TODO(caio): Support other primitives?
};

enum FillMode
{
    FILL_MODE_SOLID     = VK_POLYGON_MODE_FILL,
    FILL_MODE_LINE      = VK_POLYGON_MODE_LINE,
    FILL_MODE_POINT     = VK_POLYGON_MODE_POINT,
};

enum CullMode
{
    CULL_MODE_NONE      = VK_CULL_MODE_NONE,
    CULL_MODE_FRONT     = VK_CULL_MODE_FRONT_BIT,
    CULL_MODE_BACK      = VK_CULL_MODE_BACK_BIT,
    CULL_MODE_ALL       = VK_CULL_MODE_FRONT_AND_BACK,
};

enum FrontFace
{
    FRONT_FACE_CW       = VK_FRONT_FACE_CLOCKWISE,
    FRONT_FACE_CCW      = VK_FRONT_FACE_COUNTER_CLOCKWISE,
};

enum PipelineStage
{
    PIPELINE_STAGE_TOP                  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    PIPELINE_STAGE_VERTEX_INPUT         = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    PIPELINE_STAGE_VERTEX_SHADER        = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    PIPELINE_STAGE_FRAGMENT_SHADER      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    PIPELINE_STAGE_COMPUTE_SHADER       = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    PIPELINE_STAGE_COLOR_OUTPUT         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    PIPELINE_STAGE_TRANSFER             = VK_PIPELINE_STAGE_TRANSFER_BIT,
    PIPELINE_STAGE_BOTTOM               = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
};

enum MemoryAccess : u32
{
    MEMORY_ACCESS_NONE                  = VK_ACCESS_NONE,
    MEMORY_ACCESS_SHADER_READ           = VK_ACCESS_SHADER_READ_BIT,
    MEMORY_ACCESS_SHADER_WRITE          = VK_ACCESS_SHADER_WRITE_BIT,
    MEMORY_ACCESS_TRANSFER_READ         = VK_ACCESS_TRANSFER_READ_BIT,
    MEMORY_ACCESS_TRANSFER_WRITE        = VK_ACCESS_TRANSFER_WRITE_BIT,
    MEMORY_ACCESS_COLOR_OUTPUT_READ     = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
    MEMORY_ACCESS_COLOR_OUTPUT_WRITE    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
};

struct Barrier
{
    MemoryAccess    srcAccess   = MEMORY_ACCESS_NONE;
    MemoryAccess    dstAccess   = MEMORY_ACCESS_NONE;
    PipelineStage   srcStage    = PIPELINE_STAGE_TOP;
    PipelineStage   dstStage    = PIPELINE_STAGE_TOP;
};

enum CommandBufferState
{
    COMMAND_BUFFER_INVALID = 0,
    COMMAND_BUFFER_IDLE,
    COMMAND_BUFFER_RECORDING,
    COMMAND_BUFFER_RECORDED,
    COMMAND_BUFFER_PENDING,
};

struct CommandBuffer
{
    VkCommandBuffer vkHandle = VK_NULL_HANDLE;
    //bool isAvailable = false;   // Not recording or submitting
    CommandBufferState state = COMMAND_BUFFER_INVALID;
    VkFence vkFence = VK_NULL_HANDLE;
};

struct Context
{
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT vkDebugMessenger = VK_NULL_HANDLE;
#endif
    VmaAllocator vkAllocator = VK_NULL_HANDLE;

    // Command queue/buffer
    i32 vkCommandQueueFamily = -1;
    VkQueue vkCommandQueue = VK_NULL_HANDLE;
    VkCommandPool vkCommandPool = VK_NULL_HANDLE;
    //VkCommandPool vkSingleTimeCommandPool = VK_NULL_HANDLE;
    //Array<VkCommandBuffer> vkCommandBuffers;
    //VkCommandBuffer vkSingleTimeCommandBuffer = VK_NULL_HANDLE;
    //CommandBuffer singleTimeCommandBuffer;

    // Sync primitives
    Array<VkSemaphore> vkRenderSemaphores;
    Array<VkSemaphore> vkPresentSemaphores;
    Array<VkFence> vkRenderFences;
    VkFence vkImmediateFence = VK_NULL_HANDLE;
};
Context MakeContext(Window* window);
void DestroyContext(Context* ctx);

void MakeCommandBuffers();
Handle<CommandBuffer> GetAvailableCommandBuffer();

struct SwapChain
{
    VkSwapchainKHR vkHandle = VK_NULL_HANDLE;

    // Swap chain settings
    VkFormat vkFormat;
    VkColorSpaceKHR vkColorSpace;
    VkPresentModeKHR vkPresentMode;
    VkExtent2D vkExtents;

    // Present images
    Array<VkImage> vkImages;
    Array<VkImageView> vkImageViews;
    Array<ImageLayout> imageLayouts;
    u32 activeImage = 0;
};

SwapChain MakeSwapChain(Window* window);
void DestroySwapChain(SwapChain* swapChain);
void ResizeSwapChain(Window* window, SwapChain* swapChain);

struct VertexLayout
{
    VkVertexInputBindingDescription vkBindingDescription = {};
    Array<VkVertexInputAttributeDescription> vkAttributeDescriptions;
    Array<VertexAttribute> attributes;
};

Handle<VertexLayout> MakeVertexLayout(u32 attrCount, VertexAttribute* attributes);
void DestroyVertexLayout(VertexLayout* vertexLayout);

struct Shader
{
    ShaderType type;
    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
};

Handle<Shader> MakeShader(ShaderType type, u64 bytecodeSize, u8* bytecode);
void DestroyShader(Shader* shader);

struct Buffer
{
    VkBuffer vkHandle = VK_NULL_HANDLE;
    VmaAllocation vkAllocation = VK_NULL_HANDLE;

    BufferType type;
    u64 size = 0;
    u64 stride = 0;
    u64 count = 0;
};

Handle<Buffer> MakeBuffer(BufferType type, u64 size, u64 stride, void* data = NULL);
void DestroyBuffer(Buffer* buffer);
void CopyMemoryToBuffer(Handle<Buffer> hDstBuffer, u64 size, void* data);

struct TextureDesc
{
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    ImageUsageFlags usageFlags;
    ImageType type = IMAGE_TYPE_2D;
    ImageViewType viewType = IMAGE_VIEW_TYPE_2D;
    Format format = FORMAT_INVALID;
    ImageLayout layout = IMAGE_LAYOUT_UNDEFINED;
};

struct Texture
{
    VkImage vkHandle = VK_NULL_HANDLE;
    VkImageView vkImageView = VK_NULL_HANDLE;   // TODO(caio): This will bite me in the ass later
    VmaAllocation vkAllocation = VK_NULL_HANDLE;

    TextureDesc desc = {};
};

Handle<Texture> MakeTexture(TextureDesc desc);
void DestroyTexture(Texture* texture);
//TODO(caio): Copy Memory to Texture from CPU (after command buffer stuff works)

struct RenderPassDesc
{
    u32         width           = 0;
    u32         height          = 0;
    LoadOp      loadOp          = LOAD_OP_DONT_CARE;
    StoreOp     storeOp         = STORE_OP_DONT_CARE;
    ImageLayout initialLayout   = IMAGE_LAYOUT_UNDEFINED;
    ImageLayout finalLayout     = IMAGE_LAYOUT_UNDEFINED;
};

struct RenderPass
{
    VkRenderPass vkHandle = VK_NULL_HANDLE;
    VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;

    RenderPassDesc desc = {};

    u32 colorImageCount = 0;
    // Array<Format> outputImageFormats;
    // Array<VkImage> vkOutputImages;
    // Array<VkImageView> vkOutputImageViews;
    // Array<VmaAllocation> vkOutputImageAllocations;
    Array<Handle<Texture>> outputs;
};

Handle<RenderPass> MakeRenderPass(RenderPassDesc desc, u32 colorImageCount, Format* colorImageFormats, Format depthImageFormat);
void DestroyRenderPass(RenderPass* renderPass);
Handle<Texture> GetRenderPassOutput(Handle<RenderPass> hRenderPass, u32 outputIndex);

struct GraphicsPipelineDesc
{
    // Programmable pipeline
    Handle<Shader> hShaderVertex;
    Handle<Shader> hShaderPixel;

    // Fixed pipeline
    Handle<VertexLayout> hVertexLayout;
    Primitive primitive = PRIMITIVE_TRIANGLE_LIST;
    FillMode fillMode = FILL_MODE_SOLID;
    CullMode cullMode = CULL_MODE_BACK;
    FrontFace frontFace = FRONT_FACE_CCW;
    //TODO(caio): Blending modes, depth testing modes...
};

struct GraphicsPipeline
{
    VkPipeline vkPipeline = VK_NULL_HANDLE;
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;

    GraphicsPipelineDesc desc = {};
};

Handle<GraphicsPipeline> MakeGraphicsPipeline(Handle<RenderPass> hRenderPass, GraphicsPipelineDesc desc);
void DestroyGraphicsPipeline(GraphicsPipeline* pipeline);

inline mem::HeapAllocator renderHeap;
inline Context ctx;
inline SwapChain swapChain;

inline Array<CommandBuffer> commandBuffers;
inline Array<RenderPass> renderPasses;
inline Array<VertexLayout> vertexLayouts;
inline Array<Shader> shaders;
inline Array<Buffer> buffers;
inline Array<Texture> textures;
inline Array<GraphicsPipeline> graphicsPipelines;

void Init(Window* window);
void Shutdown();

void BeginCommandBuffer(Handle<CommandBuffer> hCmd);
void EndCommandBuffer(Handle<CommandBuffer> hCmd);
void SubmitImmediate(Handle<CommandBuffer> hCmd);

void BeginRenderPass(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass);
void EndRenderPass(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass);

void BeginFrame(u32 frame);
void EndFrame(u32 frame, Handle<CommandBuffer> hCmd);
void Present(u32 frame);

void CmdPipelineBarrierTextureLayout(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture, ImageLayout newLayout, Barrier barrier);
//TODO(caio): Add a CmdPipelineBarrier that issues a global memory barrier if needed later.
// maybe I will need for more general compute syncs.
void CmdClearColorTexture(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture, f32 r, f32 g, f32 b, f32 a);
void CmdCopyToSwapChain(Handle<CommandBuffer> hCmd, Handle<Texture> hSrc);

};
};

// Renderer API draft v3 (for Vulkan backend)
//
//  [1. RENDER N TEXTURED TRIANGLES ON SCREEN WITH DIFFERENT WORLD MATRICES]
//
//  [1.1. INIT]
//  concurrentFrames = 3;
//  Init(concurrentFrames, windowRect)                          // Initializes render context, swap chain, command queues, buffers and sync primitives for each concurrent frame
//
//  // Assets
//  ShaderAsset assetTriangleVS = LoadShaderAsset("triangleVS.vert");
//  ShaderAsset assetTrianglePS = LoadShaderAsset("trianglePS.frag");
//  TextureAsset assetCheckerTexture = LoadTextureAsset("checker.png");
//  f32 triangleVertices[] = {...};
//  u32 triangleIndices[] = {...};
//
//  struct SceneData
//  {
//      m4f view;
//      m4f proj;
//  } sceneData;
//
//  int triangleCount = 2;
//  struct ObjectData
//  {
//      m4f model;
//  } objectData[triangleCount];
//
//  // Render resources
//  Shader triangleVS = CreateShaderResource(SHADER_TYPE_VERTEX, assetTriangleVS);
//  Shader trianglePS = CreateShaderResource(SHADER_TYPE_PIXEL, assetTrianglePS);
//  
//  TextureDesc checkerTextureDesc = {};
//  checkerTextureDesc.type = TEXTURE_TYPE_2D;
//  checkerTextureDesc.format = FORMAT_RGBA8_SRGB;
//  checkerTextureDesc.width = assetCheckerTexture.width;
//  checkerTextureDesc.height = assetCheckerTexture.height;
//  checkerTextureDesc.mipCount = 1;
//  Texture checkerTexture = CreateTextureResource(checkerTextureDesc, assetCheckerTexture.data);
//
//  SamplerDesc defaultSamplerDesc = {};
//  defaultSamplerDesc.minFilter = SAMPLER_FILTER_LINEAR;
//  defaultSamplerDesc.magFilter = SAMPLER_FILTER_LINEAR;
//  defaultSamplerDesc.wrapU = SAMPLER_WRAP_REPEAT;
//  defaultSamplerDesc.wrapV = SAMPLER_WRAP_REPEAT;
//  defaultSamplerDesc.enableAnisotropy = true;
//  Sampler defaultSampler = CreateSamplerResource(defaultSamplerDesc);
//
//  VertexAttribute triangleVBAttributes[] =
//  {
//      VERTEX_ATTRIB_V3F, VERTEX_ATTRIB_V3F, VERTEX_ATTRIB_V2F,
//  };
//  VertexLayout triangleVBLayout = CreateVertexLayout(triangleVBAttributes);
//  Buffer triangleVB = CreateVertexBufferResource(triangleVBLayout, triangleVertices);
//  Buffer triangleIB = CreateIndexBufferResource(triangleIndices);
//
//  Buffer sceneDataUB = CreateUniformBuffer(sceneData);
//  Buffer objectDataUB = CreateDynamicUniformBuffer(objectData);
//
//  // Render pass
//  RenderPassDesc mainPassDesc = {};
//  mainPassDesc.outputCount = 1;
//  mainPassDesc.outputFormats = { FORMAT_RGBA8_SRGB };
//  mainPassDesc.hasDepthOutput = true;
//  mainPassDesc.depthFormats = { FORMAT_D32_FLOAT };
//  mainPassDesc.outputWidth = windowRect.width;
//  mainPassDesc.outputHeight = windowRect.height;
//  RenderPass mainPass = CreateRenderPass(mainPassDesc, concurrentFrames);
//
//  // Resource binding
//  // ResourceBindLayout --> VkDescriptorSetLayout
//  // ResourceBindSet --> multiple VkDescriptorSets (one per frame, matching layout)
//  //                 --> STATIC: VkDescriptorSet contains fixed offsets. DYNAMIC: Offset is set on bind time (such as for dynamic UBOs)
//  ResourceBindLayout globalResourceBindLayout[] =
//  {
//      { RESOURCE_TYPE_UNIFORM_BUFFER, SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL },           // (set = 0, binding = 0)
//      { RESOURCE_TYPE_SAMPLED_TEXTURE, SHADER_TYPE_PIXEL },                               // (set = 0, binding = 1)
//  };
//  ResourceBindLayout objectResourceBindLayout[] =
//  {
//      { RESOURCE_TYPE_DYNAMIC_UNIFORM_BUFFER, SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL },   // (set = 1, binding = 0)
//  };
//
//  ResourceBindSet globalResourceBindSet = CreateResourceBindSet(RESOURCE_BIND_STATIC, globalResourceBindLayout, { sceneDataUB, checkerTexture });
//  ResourceBindSet objectResourceBindSet = CreateResourceBindSet(RESOURCE_BIND_DYNAMIC, objectResourceBindLayout, { objectDataUB });
//
//  // Graphics pipeline
//  GraphicsPipelineDesc pipelineDesc = {};
//  pipelineDesc.renderPass = mainPass;
//  pipelineDesc.vertexLayout = triangleVBLayout;
//  pipelineDesc.vertexShader = triangleVS;
//  pipelineDesc.pixelShader = trianglePS;
//  pipelineDesc.resourceBindSetCount = 2;
//  pipelineDesc.resourceBindLayouts = { globalResourceBindLayout, objectResourceBindLayout };
//  pipelineDesc.primitive = PRIMITIVE_TRIANGLE_LIST;
//  pipelineDesc.fillMode = FILL_MODE_SOLID;
//  pipelineDesc.cullMode = CULL_MODE_BACK;
//  pipelineDesc.frontFace = FRONT_FACE_CCW;
//  GraphicsPipeline mainPassPipeline = CreateGraphicsPipeline(pipelineDesc);
//
//
//  [1.2. RENDER LOOP]
//  
//  BeginFrame(frameIndex);                                     // Updates sync primitives and swap chain image to match the current frame
//  CommandBuffer cmd = GetCommandBuffer(frameIndex);           // Gets the command buffer allocated for working in the given frame (commands can also be immediate)
//  BeginRenderPass(cmd, mainPass);
//  
//  // Update uniform buffer resources (CPU -> GPU)
//  UploadUniformBufferData(sceneDataUB, ...);      // View data
//  UploadUniformBufferData(objectDataUB, ...);     // World matrices for each scene object
//
//  BindGraphicsPipeline(cmd, mainPassPipeline);
//  SetViewport(cmd, ...);
//  SetScissorRect(cmd, ...);
//
//  BindResourceSet(cmd, globalResourceBindSet);            // Binds with no offset/static offset
//  BindVertexBuffer(cmd, triangleVB);
//  BindIndexBuffer(cmd, triangleIB);
//
//  for(int i = 0; i < triangleCount; i++)
//  {
//      BindResourceSet(cmd, objectResourceBindSet, i);     // Binds with dynamic offset
//      DrawIndexed(cmd, triangleIB.count);
//  }
//
//  EndRenderPass(cmd, mainPass);
//  
//  SubmitCommandBuffer(cmd);
//  frameIndex = EndFrame(frameIndex);                          // When frame ends, frame index is incremented to the next concurrent frame id
