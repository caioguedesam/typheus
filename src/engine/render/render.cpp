#include "engine/render/render.hpp"
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/ds.hpp"
#include "engine/core/memory.hpp"
#include "vulkan/vulkan_core.h"
#include <vcruntime_string.h>

namespace ty
{
namespace render
{

// Helper to match cstrings in Vulkan property-style structs,
// e.g: i32 match = FIND_STRING_IN_PROPERTIES(props, propName, "myPropName");
#define FIND_STRING_IN_VK_PROPERTIES(ARR, MEMBER_NAME, MATCH) \
    ({\
     i32 result = -1;\
     i32 arrLen = ARR_LEN((ARR));\
     for(i32 iProp = 0; i < arrLen; iProp++) {\
        if(strcmp(MATCH, ARR[iProp].MEMBER_NAME) == 0) {\
            result = iProp;\
            break;\
        }\
     }\
     result;\
     })\

VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
{
    LOGLF("VULKAN DEBUG", "%s", callbackData->pMessage);
    ASSERT(!(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT));

    return VK_FALSE;
}

VkFormat formatToVk[] =
{
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_D32_SFLOAT,
};
STATIC_ASSERT(ARR_LEN(formatToVk) == FORMAT_COUNT);
VkImageLayout imageLayoutToVk[] =
{
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
};
STATIC_ASSERT(ARR_LEN(imageLayoutToVk) == IMAGE_LAYOUT_COUNT);
VkAttachmentLoadOp loadOpToVk[] =
{
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
};
STATIC_ASSERT(ARR_LEN(loadOpToVk) == LOAD_OP_COUNT);
VkAttachmentStoreOp storeOpToVk[] =
{
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_STORE,
};
STATIC_ASSERT(ARR_LEN(storeOpToVk) == STORE_OP_COUNT);
u32 vertexAttributeSizes[] =
{
    0,
    2,
    3,
};
STATIC_ASSERT(ARR_LEN(vertexAttributeSizes) == VERTEX_ATTR_COUNT);
VkFormat vertexAttributeFormats[] =
{
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
};
STATIC_ASSERT(ARR_LEN(vertexAttributeFormats) == VERTEX_ATTR_COUNT);
// VkShaderStageFlagBits shaderTypeToVk[] =
// {
//     VK_SHADER_STAGE_VERTEX_BIT,
//     VK_SHADER_STAGE_FRAGMENT_BIT,
// };
// STATIC_ASSERT(ARR_LEN(shaderTypeToVk) == SHADER_TYPE_COUNT);
VkPrimitiveTopology primitiveToVk[] =
{
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};
STATIC_ASSERT(ARR_LEN(primitiveToVk) == PRIMITIVE_COUNT);
VkPolygonMode fillModeToVk[] =
{
    VK_POLYGON_MODE_FILL,
    VK_POLYGON_MODE_LINE,
    VK_POLYGON_MODE_POINT,
};
STATIC_ASSERT(ARR_LEN(fillModeToVk) == FILL_MODE_COUNT);
VkCullModeFlagBits cullModeToVk[] =
{
    VK_CULL_MODE_NONE,
    VK_CULL_MODE_FRONT_BIT,
    VK_CULL_MODE_BACK_BIT,
    VK_CULL_MODE_FRONT_AND_BACK,
};
STATIC_ASSERT(ARR_LEN(cullModeToVk) == CULL_MODE_COUNT);
VkFrontFace frontFaceToVk[] =
{
    VK_FRONT_FACE_CLOCKWISE,
    VK_FRONT_FACE_COUNTER_CLOCKWISE,
};
STATIC_ASSERT(ARR_LEN(frontFaceToVk) == FRONT_FACE_COUNT);
VkBufferUsageFlagBits bufferTypeToVk[] =
{
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
};
STATIC_ASSERT(ARR_LEN(bufferTypeToVk) == BUFFER_TYPE_COUNT);
VkImageType imageTypeToVk[] =
{
    VK_IMAGE_TYPE_2D,
};
STATIC_ASSERT(ARR_LEN(imageTypeToVk) == IMAGE_TYPE_COUNT);
VkImageViewType imageViewTypeToVk[] =
{
    VK_IMAGE_VIEW_TYPE_2D,
};
STATIC_ASSERT(ARR_LEN(imageViewTypeToVk) == IMAGE_TYPE_COUNT);

void Init(Window* window)
{
    renderHeap = mem::MakeHeapAllocator(RENDER_CONTEXT_MEMORY);
    mem::SetContext(&renderHeap);

    ctx = MakeContext(window);
    swapChain = MakeSwapChain(window);

    commandBuffers = MakeArray<CommandBuffer>(RENDER_MAX_COMMAND_BUFFERS);
    renderPasses = MakeArray<RenderPass>(RENDER_MAX_RENDER_PASSES);
    vertexLayouts = MakeArray<VertexLayout>(RENDER_MAX_VERTEX_LAYOUTS);
    shaders = MakeArray<Shader>(RENDER_MAX_SHADERS);
    buffers = MakeArray<Buffer>(RENDER_MAX_BUFFERS);
    textures = MakeArray<Texture>(RENDER_MAX_TEXTURES);
    graphicsPipelines = MakeArray<GraphicsPipeline>(RENDER_MAX_GRAPHICS_PIPELINES);

    MakeCommandBuffers();
}

void Shutdown()
{
    mem::SetContext(&renderHeap);

    vkDeviceWaitIdle(ctx.vkDevice);
    for(i32 i = 0; i < renderPasses.count; i++)
    {
        DestroyRenderPass(&renderPasses[i]);
    }
    for(i32 i = 0; i < vertexLayouts.count; i++)
    {
        DestroyVertexLayout(&vertexLayouts[i]);
    }
    for(i32 i = 0; i < shaders.count; i++)
    {
        DestroyShader(&shaders[i]);
    }
    for(i32 i = 0; i < buffers.count; i++)
    {
        DestroyBuffer(&buffers[i]);
    }
    for(i32 i = 0; i < textures.count; i++)
    {
        DestroyTexture(&textures[i]);
    }
    for(i32 i = 0; i < graphicsPipelines.count; i++)
    {
        DestroyGraphicsPipeline(&graphicsPipelines[i]);
    }
    DestroyArray(&commandBuffers);
    DestroyArray(&renderPasses);
    DestroyArray(&vertexLayouts);
    DestroyArray(&shaders);
    DestroyArray(&buffers);
    DestroyArray(&textures);
    DestroyArray(&graphicsPipelines);

    DestroySwapChain(&swapChain);
    DestroyContext(&ctx);
    mem::DestroyHeapAllocator(&renderHeap);
}

void MakeContext_CreateInstance(Context* ctx)
{
    ASSERT(ctx);
    // Application info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vkappname";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "vkenginename";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    // Instance info
    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    //      Extensions
    const char* extensionNames[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    instanceInfo.enabledExtensionCount = ARR_LEN(extensionNames);
    instanceInfo.ppEnabledExtensionNames = extensionNames;
#ifdef _DEBUG
    //      Validation layers
    const char* layerNames[] =
    {
        "VK_LAYER_KHRONOS_validation",
    };
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    ASSERT(layerCount);
    VkLayerProperties layers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, layers);
    for(i32 i = 0; i < ARR_LEN(layerNames); i++)
    {
        i32 match = FIND_STRING_IN_VK_PROPERTIES(layers, layerName, layerNames[i]);
        ASSERT(match != -1);
    }
    instanceInfo.enabledLayerCount = ARR_LEN(layerNames);
    instanceInfo.ppEnabledLayerNames = layerNames;
#endif

    // Instance
    VkInstance instance;
    VkResult ret = vkCreateInstance(&instanceInfo, NULL, &instance);
    ASSERTVK(ret);

    ctx->vkInstance = instance;
}

void MakeContext_SetupValidation(Context* ctx)
{
#ifdef _DEBUG
    ASSERT(ctx);
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
    messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerInfo.messageType =
          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerInfo.pfnUserCallback = ValidationLayerDebugCallback;
    messengerInfo.pUserData = NULL;

    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->vkInstance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(fn);
    VkDebugUtilsMessengerEXT messenger;
    VkResult ret = fn(ctx->vkInstance, &messengerInfo, NULL, &messenger);
    ASSERTVK(ret);
    ctx->vkDebugMessenger = messenger;
#endif
}

void MakeContext_CreateSurface(Context* ctx, Window* window)
{
    ASSERT(ctx);
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = window->winHandle;
    surfaceInfo.hinstance = window->winInstance;
    
    VkSurfaceKHR surface;
    VkResult ret = vkCreateWin32SurfaceKHR(ctx->vkInstance, &surfaceInfo, NULL, &surface);
    ASSERTVK(ret);
    ctx->vkSurface = surface;
}

void MakeContext_GetPhysicalDevice(Context* ctx)
{
    ASSERT(ctx);
    // Selecting the first device to match requirements
    // Beware if this selects least powerful GPU
    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx->vkInstance, &deviceCount, NULL);
    ASSERT(deviceCount);
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(ctx->vkInstance, &deviceCount, devices);

    i32 selectedDevice = -1;
    for(i32 i = 0; i < deviceCount; i++)
    {
        VkPhysicalDevice device = devices[i];

        // Check for extension support
        u32 extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
        ASSERT(extensionCount);
        VkExtensionProperties deviceExtensions[extensionCount];
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, deviceExtensions);
        
        bool supportsExtensions = true;
        for(i32 j = 0; j < ARR_LEN(extensions); j++)
        {
            const char* ext = extensions[j];
            i32 match = FIND_STRING_IN_VK_PROPERTIES(deviceExtensions, extensionName, ext);
            if(match == -1)
            {
                supportsExtensions = false;
                break;
            }
        }
        if(!supportsExtensions) continue;

        // Check for surface properties support
        u32 surfaceFormatCount = 0;
        u32 surfacePresentModeCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx->vkSurface, &surfaceFormatCount, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx->vkSurface, &surfacePresentModeCount, NULL);
        if(!surfaceFormatCount || !surfacePresentModeCount) continue;

        // Check for desired application features support
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);
        if(properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) continue;
        if(!features.samplerAnisotropy) continue;

        selectedDevice = i;
        break;
    }
    ASSERT(selectedDevice != -1);
    ctx->vkPhysicalDevice = devices[selectedDevice];
}

void MakeContext_CreateDeviceAndCommandQueue(Context* ctx)
{
    ASSERT(ctx);

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->vkPhysicalDevice, &queueFamilyCount, NULL);
    ASSERT(queueFamilyCount);
    VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties);

    // Get first command queue family that supports required command types
    // (graphics, compute, present)
    i32 queueFamily = -1;
    for(i32 i = 0; i < queueFamilyCount; i++)
    {
        VkQueueFamilyProperties properties = queueFamilyProperties[i];
        if(!(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                || !(properties.queueFlags & VK_QUEUE_COMPUTE_BIT)) continue;
        
        auto fn = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(ctx->vkInstance, "vkGetPhysicalDeviceSurfaceSupportKHR");
        ASSERT(fn);
        VkBool32 supportsPresent = VK_FALSE;
        fn(ctx->vkPhysicalDevice, i, ctx->vkSurface, &supportsPresent);
        if(supportsPresent == VK_FALSE) continue;

        queueFamily = i;
        break;
    }
    ASSERT(queueFamily != -1);

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    f32 priority = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = VK_TRUE;

    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = ARR_LEN(extensions);
    deviceInfo.ppEnabledExtensionNames = extensions;

    VkDevice device;
    VkResult ret = vkCreateDevice(ctx->vkPhysicalDevice, &deviceInfo, NULL, &device);
    ASSERTVK(ret);

    VkQueue queue;
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    ctx->vkDevice = device;
    ctx->vkCommandQueueFamily = queueFamily;
    ctx->vkCommandQueue = queue;
}

void MakeContext_CreateAllocator(Context* ctx)
{
    ASSERT(ctx);
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = ctx->vkInstance;
    allocatorInfo.physicalDevice = ctx->vkPhysicalDevice;
    allocatorInfo.device = ctx->vkDevice;

    VmaAllocator allocator;
    VkResult ret = vmaCreateAllocator(&allocatorInfo, &allocator);
    ASSERTVK(ret);

    ctx->vkAllocator = allocator;
}

void MakeContext_CreateCommandPool(Context* ctx)
{
    ASSERT(ctx);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx->vkCommandQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkCommandPool commandPool;
    VkResult ret = vkCreateCommandPool(ctx->vkDevice, &poolInfo, NULL, &commandPool);
    ASSERTVK(ret);
    // VkCommandPool singleTimeCommandPool;
    // ret = vkCreateCommandPool(ctx->vkDevice, &poolInfo, NULL, &singleTimeCommandPool);
    // ASSERTVK(ret);

    ctx->vkCommandPool = commandPool;

    // ctx->vkSingleTimeCommandPool = singleTimeCommandPool;

    // // One command buffer for each concurrent frame, and an additional one
    // // for single time commands like buffer copies
    // ctx->vkCommandBuffers = MakeArray<VkCommandBuffer>(RENDER_CONCURRENT_FRAMES);
    // VkCommandBufferAllocateInfo bufferInfo = {};
    // bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // bufferInfo.commandBufferCount = 1;
    // bufferInfo.commandPool = commandPool;
    // bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // VkCommandBuffer commandBuffer;
    // for(i32 i = 0; i < RENDER_CONCURRENT_FRAMES; i++)
    // {
    //     ret = vkAllocateCommandBuffers(ctx->vkDevice, &bufferInfo, &commandBuffer);
    //     ASSERTVK(ret);
    //     ctx->vkCommandBuffers.Push(commandBuffer);
    // }
    // bufferInfo.commandPool = singleTimeCommandPool;
    // ret = vkAllocateCommandBuffers(ctx->vkDevice, &bufferInfo, &commandBuffer);
    // ASSERTVK(ret);
    // ctx->vkSingleTimeCommandBuffer = commandBuffer;
}

void MakeCommandBuffers()
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    ASSERT(ctx.vkCommandPool != VK_NULL_HANDLE);
    for(i32 i = 0; i < RENDER_MAX_COMMAND_BUFFERS; i++)
    {
        VkCommandBufferAllocateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        bufferInfo.commandPool = ctx.vkCommandPool;
        bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufferInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        VkResult ret = vkAllocateCommandBuffers(ctx.vkDevice, &bufferInfo, &commandBuffer);
        ASSERTVK(ret);

        CommandBuffer result = {};
        result.vkHandle = commandBuffer;
        result.isAvailable = true;
        commandBuffers.Push(result);
    }
}

Handle<CommandBuffer> GetAvailableCommandBuffer()
{
    for(i32 i = 0; i < commandBuffers.count; i++)
    {
        CommandBuffer& commandBuffer = commandBuffers[i];
        if(commandBuffer.isAvailable)
        {
            VkResult ret = vkResetCommandBuffer(commandBuffer.vkHandle, 0);
            ASSERTVK(ret);
            return { (u32)i };
        }
    }
    ASSERT(0);
    return {};
}

void MakeContext_CreateSyncPrimitives(Context* ctx)
{
    ASSERT(ctx);

    ctx->vkRenderSemaphores = MakeArray<VkSemaphore>(RENDER_CONCURRENT_FRAMES);
    ctx->vkPresentSemaphores = MakeArray<VkSemaphore>(RENDER_CONCURRENT_FRAMES);
    ctx->vkRenderFences = MakeArray<VkFence>(RENDER_CONCURRENT_FRAMES);

    VkResult ret;
    for(i32 i = 0; i < RENDER_CONCURRENT_FRAMES; i++)
    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore semaphore;
        ret = vkCreateSemaphore(ctx->vkDevice, &semaphoreInfo, NULL, &semaphore);
        ASSERTVK(ret);
        ctx->vkRenderSemaphores.Push(semaphore);
        ret = vkCreateSemaphore(ctx->vkDevice, &semaphoreInfo, NULL, &semaphore);
        ASSERTVK(ret);
        ctx->vkPresentSemaphores.Push(semaphore);

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence;
        ret = vkCreateFence(ctx->vkDevice, &fenceInfo, NULL, &fence);
        ASSERTVK(ret);
        ctx->vkRenderFences.Push(fence);
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    ret = vkCreateFence(ctx->vkDevice, &fenceInfo, NULL, &fence);
    ASSERTVK(ret);
    ctx->vkImmediateFence = fence;
}

Context MakeContext(Window *window)
{
    mem::SetContext(&renderHeap);

    Context ctx = {};
    MakeContext_CreateInstance(&ctx);
    MakeContext_SetupValidation(&ctx);
    MakeContext_CreateSurface(&ctx, window);
    MakeContext_GetPhysicalDevice(&ctx);
    MakeContext_CreateDeviceAndCommandQueue(&ctx);
    MakeContext_CreateAllocator(&ctx);
    MakeContext_CreateCommandPool(&ctx);
    MakeContext_CreateSyncPrimitives(&ctx);

    return ctx;
}

void DestroyContext(Context *ctx)
{
    ASSERT(ctx);

    for(i32 i = 0; i < RENDER_CONCURRENT_FRAMES; i++)
    {
        vkDestroySemaphore(ctx->vkDevice, ctx->vkRenderSemaphores[i], NULL);
        vkDestroySemaphore(ctx->vkDevice, ctx->vkPresentSemaphores[i], NULL);
        vkDestroyFence(ctx->vkDevice, ctx->vkRenderFences[i], NULL);
    }
    vkDestroyFence(ctx->vkDevice, ctx->vkImmediateFence, NULL);
    vkDestroyCommandPool(ctx->vkDevice, ctx->vkCommandPool, NULL);
    //vkDestroyCommandPool(ctx->vkDevice, ctx->vkSingleTimeCommandPool, NULL);
    vmaDestroyAllocator(ctx->vkAllocator);
    vkDestroyDevice(ctx->vkDevice, NULL);
    vkDestroySurfaceKHR(ctx->vkInstance, ctx->vkSurface, NULL);
#ifdef _DEBUG
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    ASSERT(fn);
    fn(ctx->vkInstance, ctx->vkDebugMessenger, NULL);
#endif
    vkDestroyInstance(ctx->vkInstance, NULL);
    DestroyArray(&ctx->vkRenderSemaphores);
    DestroyArray(&ctx->vkPresentSemaphores);
    DestroyArray(&ctx->vkRenderFences);

    *ctx = {};
}

void MakeSwapChain_CreateSwapChain(Context* ctx, SwapChain* swapChain)
{
    ASSERT(ctx && swapChain);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && ctx->vkSurface != VK_NULL_HANDLE);

    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode;
    VkExtent2D extents;
    u32 imageCount = 0;

    // Check for required format/color space (BGRA8 SRGB, SRGB non linear)
    // if not found, just use first one available
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vkPhysicalDevice, ctx->vkSurface, &formatCount, NULL);
    ASSERT(formatCount);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vkPhysicalDevice, ctx->vkSurface, &formatCount, formats);

    format = formats[0].format;
    colorSpace = formats[0].colorSpace;
    for(i32 i = 0; i < formatCount; i++)
    {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
            && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            format = formats[i].format;
            colorSpace = formats[i].colorSpace;
            break;
        }
    }

    // Check for required present mode (MAILBOX)
    // if not found, just use FIFO
    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vkPhysicalDevice, ctx->vkSurface, &presentModeCount, NULL);
    ASSERT(presentModeCount);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vkPhysicalDevice, ctx->vkSurface, &presentModeCount, presentModes);

    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(i32 i = 0; i < presentModeCount; i++)
    {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = presentModes[i];
            break;
        }
    }

    // Getting swap chain image details
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vkPhysicalDevice, ctx->vkSurface, &capabilities);

    ASSERT(capabilities.currentExtent.width != -1);
    extents = capabilities.currentExtent;
    imageCount = capabilities.minImageCount + 1;
    if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    // Creating swap chain object
    VkSwapchainCreateInfoKHR swapChainInfo = {};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = ctx->vkSurface;
    swapChainInfo.minImageCount = imageCount;
    swapChainInfo.imageFormat = format;
    swapChainInfo.imageColorSpace = colorSpace;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageExtent = extents;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;     // Final render copied to swapchain image
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = capabilities.currentTransform;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = VK_TRUE;
    swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR handle;
    VkResult ret = vkCreateSwapchainKHR(ctx->vkDevice, &swapChainInfo, NULL, &handle);
    ASSERTVK(ret);

    swapChain->vkHandle = handle;
    swapChain->vkFormat = format;
    swapChain->vkPresentMode = presentMode;
    swapChain->vkColorSpace = colorSpace;
    swapChain->vkExtents = extents;
}

void MakeSwapChain_CreateImages(Context* ctx, SwapChain* swapChain)
{
    ASSERT(ctx && swapChain);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && swapChain->vkHandle != VK_NULL_HANDLE);

    u32 imageCount = 0;
    VkResult ret = vkGetSwapchainImagesKHR(ctx->vkDevice, swapChain->vkHandle, &imageCount, NULL);
    ASSERTVK(ret);
    ASSERT(imageCount);
    VkImage images[imageCount];
    ret = vkGetSwapchainImagesKHR(ctx->vkDevice, swapChain->vkHandle, &imageCount, images);
    ASSERTVK(ret);

    swapChain->vkImages = MakeArray<VkImage>(imageCount);
    swapChain->vkImageViews = MakeArray<VkImageView>(imageCount);

    // Color images (already created in vkCreateSwapchainKHR)
    for(i32 i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo imageViewInfo = {};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = images[i];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = swapChain->vkFormat;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        VkImageView imageView;
        ret = vkCreateImageView(ctx->vkDevice, &imageViewInfo, NULL, &imageView);
        ASSERTVK(ret);

        swapChain->vkImages.Push(images[i]);
        swapChain->vkImageViews.Push(imageView);
    }
}

SwapChain MakeSwapChain(Window* window)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && ctx.vkSurface != VK_NULL_HANDLE);
    mem::SetContext(&renderHeap);

    SwapChain result = {};
    MakeSwapChain_CreateSwapChain(&ctx, &result);
    MakeSwapChain_CreateImages(&ctx, &result);
    return result;
}

void DestroySwapChain(SwapChain* swapChain)
{
    ASSERT(swapChain);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && swapChain->vkHandle != VK_NULL_HANDLE);

    for(i32 i = 0; i < swapChain->vkImageViews.count; i++)
    {
        vkDestroyImageView(ctx.vkDevice, swapChain->vkImageViews[i], NULL);
    }
    DestroyArray(&swapChain->vkImageViews);
    DestroyArray(&swapChain->vkImages);
    vkDestroySwapchainKHR(ctx.vkDevice, swapChain->vkHandle, NULL);

    *swapChain = {};
}

void ResizeSwapChain(Window* window, SwapChain* swapChain)
{
    // Destroys and recreates swap chain
    ASSERT(swapChain);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && swapChain->vkHandle != VK_NULL_HANDLE);

    DestroySwapChain(swapChain);
    *swapChain = MakeSwapChain(window);
}

void MakeRenderPass_CreateRenderPass(Context* ctx, RenderPassDesc desc, u32 colorImageCount, Format* colorImageFormats, Format depthImageFormat, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    // Color attachments
    VkAttachmentDescription colorAttachments[colorImageCount];
    VkAttachmentReference colorAttachmentRefs[colorImageCount];
    for(i32 i = 0; i < colorImageCount; i++)
    {
        colorAttachments[i] = {};
        colorAttachments[i].format = formatToVk[colorImageFormats[i]];
        colorAttachments[i].initialLayout = imageLayoutToVk[desc.initialLayout];
        colorAttachments[i].finalLayout = imageLayoutToVk[desc.finalLayout];
        colorAttachments[i].loadOp = loadOpToVk[desc.loadOp];
        colorAttachments[i].storeOp = storeOpToVk[desc.storeOp];
        colorAttachments[i].samples = VK_SAMPLE_COUNT_1_BIT;    // TODO(caio): Support multisampling
        colorAttachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachmentRefs[i] = {};
        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // Depth attachment
    VkAttachmentDescription depthAttachment = {};
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachment.format = formatToVk[depthImageFormat];
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentRef.attachment = colorImageCount;    // Depth comes after all color attachments
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass (only supports a single subpass)
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorImageCount;
    subpass.pColorAttachments = colorAttachmentRefs;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Render pass
    VkAttachmentDescription attachments[colorImageCount + 1];
    for(i32 i = 0; i < colorImageCount; i++)
    {
        attachments[i] = colorAttachments[i];
    }
    attachments[colorImageCount] = depthAttachment;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.attachmentCount = colorImageCount + 1;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;
    VkRenderPass handle;
    VkResult ret = vkCreateRenderPass(ctx->vkDevice, &renderPassInfo, NULL, &handle);
    ASSERTVK(ret);

    renderPass->vkHandle = handle;
    renderPass->desc = desc;
    renderPass->colorImageCount = colorImageCount;
    // renderPass->outputImageFormats = MakeArray<Format>(colorImageCount + 1);
    // for(i32 i = 0; i < colorImageCount; i++)
    // {
    //     renderPass->outputImageFormats.Push(colorImageFormats[i]);
    // }
    // renderPass->outputImageFormats.Push(depthImageFormat);
}

void MakeRenderPass_CreateOutputImages(Context* ctx, Format* colorImageFormats, Format depthImageFormat, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    // renderPass->vkOutputImages = MakeArray<VkImage>(renderPass->colorImageCount + 1);
    // renderPass->vkOutputImageViews = MakeArray<VkImageView>(renderPass->colorImageCount + 1);
    // renderPass->vkOutputImageAllocations = MakeArray<VmaAllocation>(renderPass->colorImageCount + 1);
    renderPass->outputs = MakeArray<Handle<Texture>>(renderPass->colorImageCount + 1);

    // Create images for color attachments
    for(i32 i = 0; i < renderPass->colorImageCount; i++)
    {
        TextureDesc colorOutputDesc = {};
        colorOutputDesc.width = renderPass->desc.width;
        colorOutputDesc.height = renderPass->desc.height;
        colorOutputDesc.depth = 1;
        colorOutputDesc.usageFlags = ENUM_FLAGS(ImageUsageFlags, 
                IMAGE_USAGE_COLOR_ATTACHMENT
                | IMAGE_USAGE_TRANSFER_SRC
                | IMAGE_USAGE_SAMPLED);
        colorOutputDesc.format = colorImageFormats[i];
        colorOutputDesc.layout = IMAGE_LAYOUT_UNDEFINED;
        Handle<Texture> hColorOutput = MakeTexture(colorOutputDesc);

        renderPass->outputs.Push(hColorOutput);

        // VkImageCreateInfo imageInfo = {};
        // imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        // imageInfo.imageType = VK_IMAGE_TYPE_2D;
        // imageInfo.extent.width = renderPass->desc.width;
        // imageInfo.extent.height = renderPass->desc.height;
        // imageInfo.extent.depth = 1;
        // imageInfo.mipLevels = 1;
        // imageInfo.arrayLayers = 1;
        // imageInfo.format = formatToVk[renderPass->outputImageFormats[i]];
        // imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        // imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        //     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        //     | VK_IMAGE_USAGE_SAMPLED_BIT;
        // imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;      // TODO(caio): Support multisampling
        // imageInfo.flags = 0;
        // VmaAllocationCreateInfo allocationInfo = {};
        // allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        // 
        // VkImage image;
        // VmaAllocation imageAllocation;
        // VkResult ret = vmaCreateImage(ctx->vkAllocator, &imageInfo, &allocationInfo, &image, &imageAllocation, NULL);
        // ASSERTVK(ret);

        // VkImageViewCreateInfo imageViewInfo = {};
        // imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        // imageViewInfo.image = image;
        // imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        // imageViewInfo.format = formatToVk[renderPass->outputImageFormats[i]];
        // imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // imageViewInfo.subresourceRange.baseMipLevel = 0;
        // imageViewInfo.subresourceRange.levelCount = 1;
        // imageViewInfo.subresourceRange.baseArrayLayer = 0;
        // imageViewInfo.subresourceRange.layerCount = 1;

        // VkImageView imageView;
        // ret = vkCreateImageView(ctx->vkDevice, &imageViewInfo, NULL, &imageView);
        // ASSERTVK(ret);

        // renderPass->vkOutputImages.Push(image);
        // renderPass->vkOutputImageViews.Push(imageView);
        // renderPass->vkOutputImageAllocations.Push(imageAllocation);
    }

    // Create image for depth attachment
    // VkImageCreateInfo imageInfo = {};
    // imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // imageInfo.imageType = VK_IMAGE_TYPE_2D;
    // imageInfo.extent.width = renderPass->desc.width;
    // imageInfo.extent.height = renderPass->desc.height;
    // imageInfo.extent.depth = 1;
    // imageInfo.mipLevels = 1;
    // imageInfo.arrayLayers = 1;
    // imageInfo.format = formatToVk[renderPass->outputImageFormats[renderPass->colorImageCount]];
    // imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;     // Maybe expose this later
    // imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;      // TODO(caio): Support multisampling
    // imageInfo.flags = 0;
    // VmaAllocationCreateInfo allocationInfo = {};
    // allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    // 
    // VkImage image;
    // VmaAllocation imageAllocation;
    // VkResult ret = vmaCreateImage(ctx->vkAllocator, &imageInfo, &allocationInfo, &image, &imageAllocation, NULL);
    // ASSERTVK(ret);

    // VkImageViewCreateInfo imageViewInfo = {};
    // imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // imageViewInfo.image = image;
    // imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // imageViewInfo.format = formatToVk[renderPass->outputImageFormats[renderPass->colorImageCount]];
    // imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // imageViewInfo.subresourceRange.baseMipLevel = 0;
    // imageViewInfo.subresourceRange.levelCount = 1;
    // imageViewInfo.subresourceRange.baseArrayLayer = 0;
    // imageViewInfo.subresourceRange.layerCount = 1;

    // VkImageView imageView;
    // ret = vkCreateImageView(ctx->vkDevice, &imageViewInfo, NULL, &imageView);
    // ASSERTVK(ret);

    // renderPass->vkOutputImages.Push(image);
    // renderPass->vkOutputImageViews.Push(imageView);
    // renderPass->vkOutputImageAllocations.Push(imageAllocation);

    TextureDesc colorOutputDesc = {};
    colorOutputDesc.width = renderPass->desc.width;
    colorOutputDesc.height = renderPass->desc.height;
    colorOutputDesc.depth = 1;
    colorOutputDesc.usageFlags = ENUM_FLAGS(ImageUsageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT);
    colorOutputDesc.format = depthImageFormat;
    colorOutputDesc.layout = IMAGE_LAYOUT_UNDEFINED;
    Handle<Texture> hColorOutput = MakeTexture(colorOutputDesc);

    renderPass->outputs.Push(hColorOutput);
}

void MakeRenderPass_CreateFramebuffer(Context* ctx, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass->vkHandle;
    framebufferInfo.width = renderPass->desc.width;
    framebufferInfo.height = renderPass->desc.height;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = renderPass->colorImageCount + 1;
    VkImageView attachmentViews[renderPass->colorImageCount + 1];
    for(i32 i = 0; i < renderPass->colorImageCount + 1; i++)
    {
        Texture& texture = textures[renderPass->outputs[i]];
        attachmentViews[i] = texture.vkImageView;
    }
    framebufferInfo.pAttachments = attachmentViews;

    VkFramebuffer framebuffer;
    VkResult ret = vkCreateFramebuffer(ctx->vkDevice, &framebufferInfo, NULL, &framebuffer);
    ASSERTVK(ret);

    renderPass->vkFramebuffer = framebuffer;
}

Handle<RenderPass> MakeRenderPass(RenderPassDesc desc, u32 colorImageCount, Format* colorImageFormats, Format depthImageFormat)
{
    ASSERT(colorImageFormats);
    mem::SetContext(&renderHeap);

    RenderPass result = {};

    MakeRenderPass_CreateRenderPass(&ctx, desc, colorImageCount, colorImageFormats, depthImageFormat, &result);
    MakeRenderPass_CreateOutputImages(&ctx, colorImageFormats, depthImageFormat, &result);
    MakeRenderPass_CreateFramebuffer(&ctx, &result);

    renderPasses.Push(result);
    return { (u32)renderPasses.count - 1 };
}

void DestroyRenderPass(RenderPass *renderPass)
{
    ASSERT(renderPass);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && renderPass->vkHandle != VK_NULL_HANDLE);
    // for(i32 i = 0; i < renderPass->colorImageCount + 1; i++)
    // {
    //     vkDestroyImageView(ctx.vkDevice, renderPass->vkOutputImageViews[i], NULL);
    //     vmaDestroyImage(ctx.vkAllocator, renderPass->vkOutputImages[i], renderPass->vkOutputImageAllocations[i]);
    // }
    // No need to destroy these anymore, since they're destroyed along with all other textures
    vkDestroyFramebuffer(ctx.vkDevice, renderPass->vkFramebuffer, NULL);
    vkDestroyRenderPass(ctx.vkDevice, renderPass->vkHandle, NULL);

    // DestroyArray(&renderPass->outputImageFormats);
    // DestroyArray(&renderPass->vkOutputImages);
    // DestroyArray(&renderPass->vkOutputImageViews);
    // DestroyArray(&renderPass->vkOutputImageAllocations);
    DestroyArray(&renderPass->outputs);

    *renderPass = {};
}

Handle<VertexLayout> MakeVertexLayout(u32 attrCount, VertexAttribute* attributes)
{
    mem::SetContext(&renderHeap);

    VertexLayout result = {};
    result.vkBindingDescription = {};
    result.vkBindingDescription.binding = 0;    // Change this if needed later?
    result.vkBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;    //TODO(caio): Instancing
    result.vkBindingDescription.stride = 0;
    result.attributes = MakeArray<VertexAttribute>(attrCount);
    result.vkAttributeDescriptions = MakeArray<VkVertexInputAttributeDescription>(attrCount);
    
    for(i32 i = 0; i < attrCount; i++)
    {
        VertexAttribute attr = attributes[i];
        result.vkBindingDescription.stride += vertexAttributeSizes[attr];
        VkVertexInputAttributeDescription attributeDesc = {};
        attributeDesc.binding = 0;              // Change this if needed later?
        attributeDesc.location = i;
        attributeDesc.format = vertexAttributeFormats[attr];
        attributeDesc.offset = result.vkBindingDescription.stride;
        
        result.attributes.Push(attr);
        result.vkAttributeDescriptions.Push(attributeDesc);
    }

    vertexLayouts.Push(result);
    return { (u32)vertexLayouts.count - 1 };
}

void DestroyVertexLayout(VertexLayout* vertexLayout)
{
    ASSERT(vertexLayout);
    DestroyArray(&vertexLayout->attributes);
    DestroyArray(&vertexLayout->vkAttributeDescriptions);

    *vertexLayout = {};
}

Handle<Shader> MakeShader(ShaderType type, u64 bytecodeSize, u8* bytecode)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    ASSERT(bytecodeSize && bytecode);
    VkShaderModuleCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = bytecodeSize;
    shaderInfo.pCode = (u32*)bytecode; //TODO(caio): See if this bites my ass because of alignment
    VkShaderModule shaderModule;
    VkResult ret = vkCreateShaderModule(ctx.vkDevice, &shaderInfo, NULL, &shaderModule);
    ASSERTVK(ret);

    Shader result = {};
    result.type = type;
    result.vkShaderModule = shaderModule;

    shaders.Push(result);
    return { (u32) shaders.count - 1 };
}

void DestroyShader(Shader* shader)
{
    ASSERT(shader);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    vkDestroyShaderModule(ctx.vkDevice, shader->vkShaderModule, NULL);
    *shader = {};
}

Handle<Buffer> MakeBuffer(BufferType type, u64 size, u64 stride, void* data)
{
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    ASSERT(size >= stride);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = bufferTypeToVk[type];
    
    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  // Hardcoded

    VkBuffer buffer;
    VmaAllocation allocation;
    VkResult ret = vmaCreateBuffer(ctx.vkAllocator, &bufferInfo, &allocationInfo, &buffer, &allocation, NULL);
    ASSERTVK(ret);

    Buffer result = {};
    result.vkHandle = buffer;
    result.vkAllocation = allocation;
    result.type = type;
    result.size = size;
    result.stride = stride;
    result.count = size / stride;

    buffers.Push(result);

    Handle<Buffer> hResult = { (u32) buffers.count - 1 };
    if(data) CopyMemoryToBuffer(hResult, size, data);
    return hResult;
}

void DestroyBuffer(Buffer* buffer)
{
    ASSERT(buffer);
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    vmaDestroyBuffer(ctx.vkAllocator, buffer->vkHandle, buffer->vkAllocation);
    *buffer = {};
}

void CopyMemoryToBuffer(Handle<Buffer> hDstBuffer, u64 size, void* data)
{
    ASSERT(hDstBuffer.IsValid());
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    ASSERT(data);

    Buffer& buffer = buffers[hDstBuffer];
    void* mapping = NULL;
    VkResult ret = vmaMapMemory(ctx.vkAllocator, buffer.vkAllocation, &mapping);
    memcpy(mapping, data, size);
    ASSERTVK(ret);
    vmaUnmapMemory(ctx.vkAllocator, buffer.vkAllocation);
}

Handle<Texture> MakeTexture(TextureDesc desc)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && ctx.vkAllocator != VK_NULL_HANDLE);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.usage = desc.usageFlags;
    imageInfo.format = formatToVk[desc.format];
    imageInfo.initialLayout = imageLayoutToVk[desc.layout];
    imageInfo.imageType = imageTypeToVk[desc.type];
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = desc.depth;
    imageInfo.mipLevels = 1;                        // TODO(caio): Support mipmapping
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;      // TODO(caio): Support multisampling
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image;
    VmaAllocation allocation;
    VkResult ret = vmaCreateImage(ctx.vkAllocator, &imageInfo, &allocationInfo, &image, &allocation, NULL);
    ASSERTVK(ret);

    // Each image has one image view. This could change in the future,
    // then figure out where image view should fit in outside Texture
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = image;
    imageViewInfo.viewType = imageViewTypeToVk[desc.type];
    imageViewInfo.format = formatToVk[desc.format];
    imageViewInfo.subresourceRange.aspectMask = ENUM_HAS_FLAG(desc.usageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    
    VkImageView imageView;
    ret = vkCreateImageView(ctx.vkDevice, &imageViewInfo, NULL, &imageView);
    ASSERTVK(ret);

    Texture result = {};
    result.vkHandle = image;
    result.vkImageView = imageView;
    result.vkAllocation = allocation;
    result.desc = desc;

    textures.Push(result);
    return { (u32)textures.count - 1 };
}

void DestroyTexture(Texture* texture)
{
    ASSERT(texture);
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    vkDestroyImageView(ctx.vkDevice, texture->vkImageView, NULL);
    vmaDestroyImage(ctx.vkAllocator, texture->vkHandle, texture->vkAllocation);
    *texture = {};
}

Handle<GraphicsPipeline> MakeGraphicsPipeline(Handle<RenderPass> hRenderPass, GraphicsPipelineDesc desc)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);

    RenderPass& renderPass = renderPasses[hRenderPass];
    VertexLayout& vertexLayout = vertexLayouts[desc.hVertexLayout];
    Shader& vs = shaders[desc.hShaderVertex];
    Shader& ps = shaders[desc.hShaderPixel];
    ASSERT(vs.type == SHADER_TYPE_VERTEX);
    ASSERT(ps.type == SHADER_TYPE_PIXEL);

    // Shader stages
    VkPipelineShaderStageCreateInfo vsStageInfo = {};
    vsStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsStageInfo.pName = "main";     // Shader entrypoint is always main
    vsStageInfo.module = vs.vkShaderModule;
    vsStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VkPipelineShaderStageCreateInfo psStageInfo = vsStageInfo;
    psStageInfo.module = ps.vkShaderModule;
    psStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkPipelineShaderStageCreateInfo shaderStageInfos[] =
    {
        vsStageInfo, psStageInfo
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = primitiveToVk[desc.primitive];
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexLayout.vkBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout.attributes.count;
    vertexInputInfo.pVertexAttributeDescriptions = vertexLayout.vkAttributeDescriptions.data;

    // Dynamic states
    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = ARR_LEN(dynamicStates);
    dynamicInfo.pDynamicStates = dynamicStates;

    // Viewport state (dynamic)
    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.polygonMode = fillModeToVk[desc.fillMode];
    rasterizationInfo.cullMode = cullModeToVk[desc.cullMode];
    rasterizationInfo.frontFace = frontFaceToVk[desc.frontFace];
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.lineWidth = 1;
    rasterizationInfo.depthBiasClamp = VK_FALSE;

    // Blending
    // //TODO(caio): support other blend modes rather than overwrite
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;     // No blending, overwrite color
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;

    // Depth state
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    //TODO(caio): Push constants
    //TODO(caio): Descriptor set layouts

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = NULL;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = NULL;
    VkPipelineLayout pipelineLayout;
    VkResult ret = vkCreatePipelineLayout(ctx.vkDevice, &layoutInfo, NULL, &pipelineLayout);
    ASSERTVK(ret);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = ARR_LEN(shaderStageInfos);
    pipelineInfo.pStages = shaderStageInfos;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass.vkHandle;
    pipelineInfo.subpass = 0;   // Only supports 1 subpass per render pass
    VkPipeline pipeline;
    ret = vkCreateGraphicsPipelines(ctx.vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);
    ASSERTVK(ret);

    GraphicsPipeline result = {};
    result.vkPipeline = pipeline;
    result.vkPipelineLayout = pipelineLayout;
    result.desc = desc;

    graphicsPipelines.Push(result);
    return { (u32)graphicsPipelines.count - 1 };
}

void DestroyGraphicsPipeline(GraphicsPipeline* pipeline)
{
    ASSERT(pipeline);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    vkDestroyPipelineLayout(ctx.vkDevice, pipeline->vkPipelineLayout, NULL);
    vkDestroyPipeline(ctx.vkDevice, pipeline->vkPipeline, NULL);
}

void BeginCommandBuffer(Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(cmd.isAvailable);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult ret = vkBeginCommandBuffer(cmd.vkHandle, &beginInfo);
    ASSERTVK(ret);

    cmd.isAvailable = false;
}

void EndCommandBuffer(Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(!cmd.isAvailable);

    VkResult ret = vkEndCommandBuffer(cmd.vkHandle);
    ASSERTVK(ret);
}

void SubmitImmediate(Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(!cmd.isAvailable);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.vkHandle;
    
    VkResult ret = vkQueueSubmit(ctx.vkCommandQueue, 1, &submitInfo, ctx.vkImmediateFence);
    ASSERTVK(ret);

    ret = vkWaitForFences(ctx.vkDevice, 1, &ctx.vkImmediateFence, VK_TRUE, MAX_U64);
    ASSERTVK(ret);
    ret = vkResetFences(ctx.vkDevice, 1, &ctx.vkImmediateFence);
    ASSERTVK(ret);
    ret = vkResetCommandPool(ctx.vkDevice, ctx.vkCommandPool, 0);
    ASSERTVK(ret);
}

void BeginFrame(u32 frame)
{
    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;
    
    // Wait for last occurrence of frame to finish
    VkFence fence = ctx.vkRenderFences[inFlightFrame];
    vkWaitForFences(ctx.vkDevice, 1, &fence, VK_TRUE, MAX_U64);

    VkSemaphore presentSemaphore = ctx.vkPresentSemaphores[inFlightFrame];
    VkResult ret = vkAcquireNextImageKHR(ctx.vkDevice, swapChain.vkHandle, MAX_U64, presentSemaphore, VK_NULL_HANDLE, &swapChain.activeImage);
    if(ret == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // TODO(caio): Resize swap chain
    }
    else ASSERTVK(ret);

    // Reset fence to start recording work
    vkResetFences(ctx.vkDevice, 1, &fence);

    // TODO(caio): CONTINUE
    // - Figure out begin/end frame workflow**
    // - Figure out how to handle swap chain image with vkAcquireNextImage
    //      - Maybe something like activeImage in SwapChain and BeginFrame
    //      updates this value
    // - Figure out how to handle submit**
    // - Clear command
    // - Copy to Swap Chain command
    // - Present
    // - Render blue screen
    // - Resize swap chain on begin and end frame
}

void EndFrame(u32 frame, Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(!cmd.isAvailable);

    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.vkHandle;

    VkSemaphore presentSemaphore = ctx.vkPresentSemaphores[inFlightFrame];
    VkSemaphore renderSemaphore = ctx.vkRenderSemaphores[inFlightFrame];
    VkFence renderFence = ctx.vkRenderFences[inFlightFrame];
    
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;

    VkResult ret = vkQueueSubmit(ctx.vkCommandQueue, 1, &submitInfo, renderFence);
    ASSERTVK(ret);
}

void Present(u32 frame)
{
    ASSERT(ctx.vkCommandQueue != VK_NULL_HANDLE);
    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;
    VkSemaphore presentSemaphore = ctx.vkPresentSemaphores[inFlightFrame];

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain.vkHandle;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &presentSemaphore;
    presentInfo.pImageIndices = &swapChain.activeImage;
    VkResult ret = vkQueuePresentKHR(ctx.vkCommandQueue, &presentInfo);
    if(ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR)
    {
        // TODO(caio): Resize swap chain
    }
    else ASSERTVK(ret);
}

};
};