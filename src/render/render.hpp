// ========================================================
// RENDER
// Real-time rendering API implemented with Vulkan.
// Will support more features in the future, such as compute and raytracing?
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "../core/base.hpp"
#include "../core/debug.hpp"
#include "../core/memory.hpp"
#include "../core/ds.hpp"
#include "../core/string.hpp"
#include "./window.hpp"
#include "vulkan/vulkan_core.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"
#include "vma/vk_mem_alloc.h"

namespace ty
{
namespace render
{

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)

enum Format
{
    FORMAT_INVALID              = VK_FORMAT_UNDEFINED,
    FORMAT_RGB8_SRGB            = VK_FORMAT_R8G8B8_SRGB,
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

enum ShaderType : u32
{
    SHADER_TYPE_VERTEX  = VK_SHADER_STAGE_VERTEX_BIT,
    SHADER_TYPE_PIXEL   = VK_SHADER_STAGE_FRAGMENT_BIT,
    SHADER_TYPE_COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT,
};

enum BufferType : u32
{
    BUFFER_TYPE_VERTEX  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    BUFFER_TYPE_INDEX   = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    BUFFER_TYPE_UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    BUFFER_TYPE_STAGING = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    BUFFER_TYPE_STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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

enum SamplerFilter
{
    SAMPLER_FILTER_LINEAR = VK_FILTER_LINEAR,
    SAMPLER_FILTER_NEAREST = VK_FILTER_NEAREST,
};

enum SamplerAddressMode
{
    SAMPLER_ADDRESS_REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    SAMPLER_ADDRESS_CLAMP = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
};

enum ResourceType
{
    RESOURCE_UNIFORM_BUFFER             = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    RESOURCE_DYNAMIC_UNIFORM_BUFFER     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    RESOURCE_STORAGE_BUFFER             = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    RESOURCE_DYNAMIC_STORAGE_BUFFER     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
    RESOURCE_SAMPLED_TEXTURE            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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

enum CommandBufferType
{
    COMMAND_BUFFER_FRAME,
    COMMAND_BUFFER_IMMEDIATE,
};

struct CommandBuffer
{
    VkCommandBuffer vkHandle = VK_NULL_HANDLE;
    CommandBufferState state = COMMAND_BUFFER_INVALID;
    VkFence vkFence = VK_NULL_HANDLE;
};

struct SwapChain
{
    VkSwapchainKHR vkHandle = VK_NULL_HANDLE;

    // Swap chain settings
    VkFormat vkFormat;
    VkColorSpaceKHR vkColorSpace;
    VkPresentModeKHR vkPresentMode;
    VkExtent2D vkExtents;

    // Present images
    SArray<VkImage> vkImages;
    SArray<VkImageView> vkImageViews;
    SArray<ImageLayout> imageLayouts;
    u32 activeImage = 0;
};

struct Shader
{
    ShaderType type;
    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
};

struct Buffer
{
    VkBuffer vkHandle = VK_NULL_HANDLE;
    VmaAllocation vkAllocation = VK_NULL_HANDLE;

    BufferType type;
    u64 size = 0;
    u64 stride = 0;
    u64 count = 0;
};

struct TextureDesc
{
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 mipLevels = 1;
    SamplerFilter mipSamplerFilter = SAMPLER_FILTER_LINEAR;
    ImageUsageFlags usageFlags;
    ImageType type = IMAGE_TYPE_2D;
    ImageViewType viewType = IMAGE_VIEW_TYPE_2D;
    Format format = FORMAT_INVALID;
    ImageLayout layout = IMAGE_LAYOUT_UNDEFINED;
};

struct Texture
{
    VkImage vkHandle = VK_NULL_HANDLE;
    VkImageView vkImageView = VK_NULL_HANDLE;   // TODO(caio): This might bite me in the ass later
    VmaAllocation vkAllocation = VK_NULL_HANDLE;
    TextureDesc desc = {};
};

struct SamplerDesc
{
    SamplerFilter minFilter = SAMPLER_FILTER_LINEAR;
    SamplerFilter magFilter = SAMPLER_FILTER_LINEAR;
    SamplerFilter mipFilter = SAMPLER_FILTER_LINEAR;
    SamplerAddressMode addressModeU = SAMPLER_ADDRESS_REPEAT;
    SamplerAddressMode addressModeV = SAMPLER_ADDRESS_REPEAT;
    SamplerAddressMode addressModeW = SAMPLER_ADDRESS_REPEAT;
};

struct Sampler
{
    VkSampler vkHandle = VK_NULL_HANDLE;
    SamplerDesc desc = {};
};

#define TY_RENDER_MAX_FORMATS_PER_RT 8
struct RenderTargetDesc
{
    u32 width   = 0;
    u32 height  = 0;
    u32 colorImageCount = 0;
    Format colorImageFormats[TY_RENDER_MAX_FORMATS_PER_RT];
    Format depthImageFormat = FORMAT_INVALID;
};

struct RenderTarget
{
    RenderTargetDesc desc = {};
    SArray<handle> outputTextures;
};

struct RenderPassDesc
{
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
    handle hRenderTarget = HANDLE_INVALID;
};

struct VertexLayout
{
    VkVertexInputBindingDescription vkBindingDescription = {};
    SArray<VkVertexInputAttributeDescription> vkAttributeDescriptions;
    SArray<VertexAttribute> attributes;
};

struct ResourceDesc
{
    // NOTE(caio): All resources are by default partially bound
    String name;
    ResourceType type;
    ShaderType shaderStages;
    u32 count = 1;
};

struct SampledTextureHandle
{
    handle hTexture = HANDLE_INVALID;
    handle hSampler = HANDLE_INVALID;
};

struct Resource
{
    ResourceDesc desc = {};

    handle hBuffer = HANDLE_INVALID;
    handle hTexture = HANDLE_INVALID;
    handle hSampler = HANDLE_INVALID;
    SArray<SampledTextureHandle> hTextureArray = {};
};

struct ResourceSet
{
    SArray<Resource> resources = {};

    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
};

struct PushConstantRange
{
    // TODO(caio): Push constant offsets? Currently only doing single block from offset 0
    u64 offset = 0;
    u64 size = 0;
    ShaderType shaderStages;
};

#define TY_RENDER_MAX_PUSH_CONSTANT_RANGES_PER_PIPELINE 8
struct GraphicsPipelineDesc
{
    // Programmable pipeline
    handle hShaderVertex = HANDLE_INVALID;
    handle hShaderPixel = HANDLE_INVALID;

    // Fixed pipeline
    handle hVertexLayout = HANDLE_INVALID;
    Primitive primitive = PRIMITIVE_TRIANGLE_LIST;
    FillMode fillMode = FILL_MODE_SOLID;
    CullMode cullMode = CULL_MODE_BACK;
    FrontFace frontFace = FRONT_FACE_CCW;
    //TODO(caio): Blending modes, depth testing modes...

    u32 pushConstantRangeCount = 0;
    PushConstantRange pushConstantRanges[TY_RENDER_MAX_PUSH_CONSTANT_RANGES_PER_PIPELINE];

    void PushPushConstantRange(PushConstantRange pushConstantRange);
};

struct GraphicsPipeline
{
    VkPipeline vkPipeline = VK_NULL_HANDLE;
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    GraphicsPipelineDesc desc = {};
};

struct ComputePipelineDesc
{
    handle hShaderCompute = HANDLE_INVALID;
    u32 pushConstantRangeCount = 0;
    PushConstantRange pushConstantRanges[TY_RENDER_MAX_PUSH_CONSTANT_RANGES_PER_PIPELINE];

    void PushPushConstantRange(PushConstantRange pushConstantRange);
};

struct ComputePipeline
{
    VkPipeline vkPipeline = VK_NULL_HANDLE;
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;

    ComputePipelineDesc desc = {};
};

struct Context
{
    mem::Arena* arena = NULL;
    Window* window = NULL;

    // Vulkan API context
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties = {};
    VkDevice vkDevice = VK_NULL_HANDLE;
#ifdef TY_DEBUG
    VkDebugUtilsMessengerEXT vkDebugMessenger = VK_NULL_HANDLE;
#endif
    VmaAllocator vkAllocator = VK_NULL_HANDLE;

    i32 vkCommandQueueFamily = -1;
    VkQueue vkCommandQueue = VK_NULL_HANDLE;
    VkCommandPool vkCommandPool = VK_NULL_HANDLE;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;

    SArray<VkSemaphore> vkRenderSemaphores;
    SArray<VkSemaphore> vkPresentSemaphores;
    SArray<VkFence> vkRenderFences;
    VkFence vkImmediateFence = VK_NULL_HANDLE;

    // Render context
    SwapChain swapChain;
    SArray<CommandBuffer> commandBuffers;

    SArray<Shader> resourceShaders;
    SArray<Buffer> resourceBuffers;
    SArray<Texture> resourceTextures;
    SArray<Sampler> resourceSamplers;
    SArray<RenderTarget> renderTargets;
    SArray<RenderPass> renderPasses;
    SArray<VertexLayout> vertexLayouts;
    SArray<ResourceSet> resourceSets;
    SArray<GraphicsPipeline> pipelinesGraphics;
    SArray<ComputePipeline> pipelinesCompute;
};

Context MakeRenderContext(u64 arenaSize, Window* window);
void DestroyRenderContext(Context* ctx);

handle MakeShaderResource(Context* ctx, ShaderType type, u64 bytecodeSize, byte* bytecode);
handle MakeBufferResource(Context* ctx, BufferType type, u64 size, u64 stride, void* data = NULL);
handle MakeTextureResource(Context* ctx, TextureDesc desc);
handle MakeSamplerResource(Context* ctx, SamplerDesc desc);
handle MakeVertexLayout(Context* ctx, u32 attrCount, VertexAttribute* attrs);
void DestroyShaderResource(Context* ctx, handle hShader);
void DestroyBufferResource(Context* ctx, handle hBuffer);
void DestroyTextureResource(Context* ctx, handle hTexture);
void DestroySamplerResource(Context* ctx, handle hSampler);

void CopyMemoryToBuffer(Context* ctx, handle hDst, u64 dstOffset, u64 srcSize, void* srcData);
u32 GetBufferTypeAlignment(Context* ctx, BufferType type);
u32 GetMaxMipLevels(u32 w, u32 h);

handle MakeResourceSet(Context* ctx, u32 resourceCount, ResourceDesc* resourceDescs);
void UploadResourceSet(Context* ctx, handle hSet);
void DestroyResourceSet(Context* ctx, handle hSet);
handle GetResource(Context* ctx, handle hSet, String resourceName);
void SetBufferResource(Context* ctx, handle hSet, String resourceName, handle hBuffer);
void SetTextureResource(Context* ctx, handle hSet, String resourceName, handle hTexture, handle hSampler);
void SetTextureArrayResource(Context* ctx, handle hSet, String resourceName, u32 arraySize, SampledTextureHandle* hTextures);
void SetTextureToArrayResource(Context* ctx, handle hSet, String resourceName, u32 resourceIndex, handle hTexture, handle hSampler);

handle MakeRenderTarget(Context* ctx, RenderTargetDesc desc);
handle MakeRenderPass(Context* ctx, RenderPassDesc desc, handle hRTarget);
handle MakeGraphicsPipeline(Context* ctx, handle hRenderPass, GraphicsPipelineDesc desc, u32 resourceSetCount, handle* hResourceSets);
handle MakeComputePipeline(Context* ctx, ComputePipelineDesc desc, u32 resourceSetCount, handle* hResourceSets);
void DestroyRenderPass(Context* ctx, handle hRPass);
void DestroyGraphicsPipeline(Context* ctx, handle hPipeline);
void DestroyComputePipeline(Context* ctx, handle hPipeline);

handle GetRenderTargetOutput(Context* ctx, handle hRenderTarget, u32 outputIndex);
handle GetRenderTargetDepthOutput(Context* ctx, handle hRenderTarget);

handle GetAvailableCommandBuffer(Context* ctx, CommandBufferType type, i32 frame = 0);
void BeginCommandBuffer(Context* ctx, handle hCb);
void EndCommandBuffer(Context* ctx, handle hCb);
void SubmitImmediate(Context* ctx, handle hCb);

void BeginRenderPass(Context* ctx, handle hCb, handle hRenderPass);
void EndRenderPass(Context* ctx, handle hCb, handle hRenderPass);

void BeginFrame(Context* ctx, u32 frame);
void EndFrame(Context* ctx, u32 frame, handle hCb);
void Present(Context* ctx, u32 frame);

void CmdPipelineBarrier(Context* ctx, handle hCb, Barrier barrier);
void CmdPipelineBarrierTextureLayout(Context* ctx, handle hCb, handle hTexture, ImageLayout newLayout, Barrier barrier);
//TODO(caio): Should I have each mip's layout tracked? So both these commands can't break
void CmdPipelineBarrierTextureMipLayout(Context* ctx, handle hCb, handle hTexture, ImageLayout oldLayout, ImageLayout newLayout, Barrier barrier, u32 mipLevel);
void CmdGenerateMipmaps(Context* ctx, handle hCb, handle hTexture);
void CmdCopyBufferToTexture(Context* ctx, handle hCb, handle hSrc, handle hDst);
void CmdClearColorTexture(Context* ctx, handle hCb, handle hTexture, f32 r, f32 g, f32 b, f32 a);
void CmdBindGraphicsPipeline(Context* ctx, handle hCb, handle hPipeline);
void CmdBindComputePipeline(Context* ctx, handle hCb, handle hPipeline);
void CmdBindGraphicsResources(Context* ctx, handle hCb, handle hPipeline, handle hResourceSet, u32 resourceSetIndex, u32 dynamicOffsetCount = 0, u32* dynamicOffsets = NULL);
void CmdBindComputeResources(Context* ctx, handle hCb, handle hPipeline, handle hResourceSet, u32 resourceSetIndex, u32 dynamicOffsetCount = 0, u32* dynamicOffsets = NULL);
void CmdUpdateGraphicsPushConstantRange(Context* ctx, handle hCb, u32 rangeIndex, void* data, handle hPipeline);
void CmdUpdateComputePushConstantRange(Context* ctx, handle hCb, u32 rangeIndex, void* data, handle hPipeline);
void CmdSetViewport(Context* ctx, handle hCb, f32 offsetX, f32 offsetY, f32 width, f32 height, f32 minDepth = 0, f32 maxDepth = 1);
void CmdSetDefaultViewport(Context* ctx, handle hCb, handle hRenderPass);
void CmdSetScissor(Context* ctx, handle hCb, i32 offsetX, i32 offsetY, i32 width, i32 height);
void CmdSetDefaultScissor(Context* ctx, handle hCb, handle hRenderPass);
void CmdBindVertexBuffer(Context* ctx, handle hCb, handle hVB);
void CmdBindIndexBuffer(Context* ctx, handle hCb, handle hIB);
void CmdDrawIndexed(Context* ctx, handle hCb, handle hIB, i32 instanceCount);
void CmdDispatch(Context* ctx, handle hCb, u32 x, u32 y, u32 z);
void CmdCopyToSwapChain(Context* ctx, handle hCb, handle hSrc);

};  // namespace render
};  // namespace ty
