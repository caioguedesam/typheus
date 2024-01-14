#include "../core/base.hpp"
#include "../core/debug.hpp"
#include "../core/ds.hpp"
#include "../core/memory.hpp"
#include "./render.hpp"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

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

u32 vertexAttributeSizes[] =
{
    0,
    2 * sizeof(f32),
    3 * sizeof(f32),
};
STATIC_ASSERT(ARR_LEN(vertexAttributeSizes) == VERTEX_ATTR_COUNT);
VkFormat vertexAttributeFormats[] =
{
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
};
STATIC_ASSERT(ARR_LEN(vertexAttributeFormats) == VERTEX_ATTR_COUNT);

void Init(Window* window)
{
    renderHeap = mem::MakeHeapAllocator(RENDER_CONTEXT_MEMORY);
    mem::SetContext(&renderHeap);

    ctx = MakeContext(window);
    swapChain = MakeSwapChain(window);

    commandBuffers = MakeHArray<CommandBuffer>(RENDER_MAX_COMMAND_BUFFERS);
    shaders = MakeHArray<Shader>(RENDER_MAX_SHADERS);
    buffers = MakeHArray<Buffer>(RENDER_MAX_BUFFERS);
    textures = MakeHArray<Texture>(RENDER_MAX_TEXTURES);
    samplers = MakeHArray<Sampler>(RENDER_MAX_SAMPLERS);
    renderTargets = MakeHArray<RenderTarget>(RENDER_MAX_RENDER_TARGETS);
    renderPasses = MakeHArray<RenderPass>(RENDER_MAX_RENDER_PASSES);
    vertexLayouts = MakeHArray<VertexLayout>(RENDER_MAX_VERTEX_LAYOUTS);
    resourceSetLayouts = MakeHArray<ResourceSetLayout>(RENDER_MAX_RESOURCE_SET_LAYOUTS);
    resourceSets = MakeHArray<ResourceSet>(RENDER_MAX_RESOURCE_SETS);
    graphicsPipelines = MakeHArray<GraphicsPipeline>(RENDER_MAX_GRAPHICS_PIPELINES);
    computePipelines = MakeHArray<ComputePipeline>(RENDER_MAX_COMPUTE_PIPELINES);

    MakeCommandBuffers();
}

void Shutdown()
{
    mem::SetContext(&renderHeap);

    vkDeviceWaitIdle(ctx.vkDevice);
    ForHArray(renderPasses, hRenderPass)
    {
        DestroyRenderPass(&renderPasses[hRenderPass]);
    }
    ForHArray(vertexLayouts, hVertexLayout)
    {
        DestroyVertexLayout(&vertexLayouts[hVertexLayout]);
    }
    ForHArray(shaders, hShader)
    {
        DestroyShader(&shaders[hShader]);
    }
    ForHArray(buffers, hBuffer)
    {
        DestroyBuffer(&buffers[hBuffer]);
    }
    ForHArray(textures, hTexture)
    {
        DestroyTexture(&textures[hTexture]);
    }
    ForHArray(samplers, hSampler)
    {
        DestroySampler(&samplers[hSampler]);
    }
    ForHArray(resourceSetLayouts, hResourceSetLayout)
    {
        DestroyResourceSetLayout(&resourceSetLayouts[hResourceSetLayout]);
    }
    ForHArray(resourceSets, hResourceSet)
    {
        DestroyResourceSet(&resourceSets[hResourceSet]);
    }
    ForHArray(graphicsPipelines, hGraphicsPipeline)
    {
        DestroyGraphicsPipeline(&graphicsPipelines[hGraphicsPipeline]);
    }
    ForHArray(computePipelines, hComputePipeline)
    {
        DestroyComputePipeline(&computePipelines[hComputePipeline]);
    }
    DestroyHArray(&commandBuffers);
    DestroyHArray(&renderPasses);
    DestroyHArray(&vertexLayouts);
    DestroyHArray(&shaders);
    DestroyHArray(&buffers);
    DestroyHArray(&textures);
    DestroyHArray(&resourceSetLayouts);
    DestroyHArray(&resourceSets);
    DestroyHArray(&graphicsPipelines);
    DestroyHArray(&computePipelines);

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
#ifdef TY_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    instanceInfo.enabledExtensionCount = ARR_LEN(extensionNames);
    instanceInfo.ppEnabledExtensionNames = extensionNames;
#ifdef TY_DEBUG
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
#ifdef TY_DEBUG
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
    vkGetPhysicalDeviceProperties(ctx->vkPhysicalDevice, &ctx->vkPhysicalDeviceProperties);
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

void MakeContext_CreatePools(Context* ctx)
{
    ASSERT(ctx);

    // Command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx->vkCommandQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkCommandPool commandPool;
    VkResult ret = vkCreateCommandPool(ctx->vkDevice, &poolInfo, NULL, &commandPool);
    ASSERTVK(ret);

    ctx->vkCommandPool = commandPool;

    // Descriptor pool
    VkDescriptorPoolSize descriptorPoolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = ARR_LEN(descriptorPoolSizes);
    descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolInfo.maxSets = 100;
    VkDescriptorPool descriptorPool;
    ret = vkCreateDescriptorPool(ctx->vkDevice, &descriptorPoolInfo, NULL, &descriptorPool);
    ASSERTVK(ret);

    ctx->vkDescriptorPool = descriptorPool;
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
        result.vkFence = VK_NULL_HANDLE;
        result.state = COMMAND_BUFFER_IDLE;
        commandBuffers.Insert(result);
    }
}

Handle<CommandBuffer> GetAvailableCommandBuffer(CommandBufferType type, i32 frame)
{
    VkFence resultFence;
    VkResult ret;
    if(type == COMMAND_BUFFER_FRAME)
    {
        // Frame command buffers wait for the frame's signal here before they start
        // recording again.
        resultFence = ctx.vkRenderFences[frame % RENDER_CONCURRENT_FRAMES];
        ret = vkWaitForFences(ctx.vkDevice, 1, &resultFence, VK_TRUE, MAX_U64);
        ASSERTVK(ret);
    }
    else if(type == COMMAND_BUFFER_IMMEDIATE)
    {
        // Immediate command buffers don't need waiting since they wait right after
        // submit.
        resultFence = ctx.vkImmediateFence;
    }

    // First look for an IDLE command buffer
    ForHArray(commandBuffers, hCmd)
    {
        CommandBuffer& commandBuffer = commandBuffers[hCmd];
        if(commandBuffer.state == COMMAND_BUFFER_IDLE)
        {
            ret = vkResetCommandBuffer(commandBuffer.vkHandle, 0);
            ASSERTVK(ret);
            commandBuffer.vkFence = resultFence;
            ret = vkResetFences(ctx.vkDevice, 1, &commandBuffer.vkFence);
            ASSERTVK(ret);
            return hCmd;
        }
    }

    // If no IDLE command buffers left, look for a PENDING one which has already
    // finished submission and should be IDLE.
    ForHArray(commandBuffers, hCmd)
    {
        CommandBuffer& commandBuffer = commandBuffers[hCmd];
        if(commandBuffer.state == COMMAND_BUFFER_PENDING)
        {
            ASSERT(commandBuffer.vkFence);
            ret = vkGetFenceStatus(ctx.vkDevice, commandBuffer.vkFence);
            if(ret == VK_SUCCESS)
            {
                ret = vkResetCommandBuffer(commandBuffer.vkHandle, 0);
                ASSERTVK(ret);
                commandBuffer.vkFence = resultFence;
                commandBuffer.state = COMMAND_BUFFER_IDLE;
                ret = vkResetFences(ctx.vkDevice, 1, &commandBuffer.vkFence);
                ASSERTVK(ret);
                return hCmd;
            }
        }
    }
    
    ASSERT(0);
    return HANDLE_INVALID_VALUE;
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
    ctx.window = window;
    MakeContext_CreateInstance(&ctx);
    MakeContext_SetupValidation(&ctx);
    MakeContext_CreateSurface(&ctx, window);
    MakeContext_GetPhysicalDevice(&ctx);
    MakeContext_CreateDeviceAndCommandQueue(&ctx);
    MakeContext_CreateAllocator(&ctx);
    MakeContext_CreatePools(&ctx);
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
    vkDestroyDescriptorPool(ctx->vkDevice, ctx->vkDescriptorPool, NULL);
    vmaDestroyAllocator(ctx->vkAllocator);
    vkDestroyDevice(ctx->vkDevice, NULL);
    vkDestroySurfaceKHR(ctx->vkInstance, ctx->vkSurface, NULL);
#ifdef TY_DEBUG
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
    swapChain->imageLayouts = MakeArray<ImageLayout>(imageCount);

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
        swapChain->imageLayouts.Push(IMAGE_LAYOUT_UNDEFINED);
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
    mem::SetContext(&renderHeap);
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
    ASSERT(window->state == WINDOW_RESIZING);

    while(window->w == 0 || window->h == 0)
    {
        window->PollMessages();
    }

    DestroySwapChain(swapChain);
    *swapChain = MakeSwapChain(window);
    window->state = WINDOW_IDLE;
}

Handle<RenderTarget> MakeRenderTarget(RenderTargetDesc desc)
{
    mem::SetContext(&renderHeap);
    RenderTarget result = {};

    result.outputs = MakeArray<Handle<Texture>>(desc.colorImageCount + 1);
    // Create images for color attachments
    for(i32 i = 0; i < desc.colorImageCount; i++)
    {
        TextureDesc colorOutputDesc = {};
        colorOutputDesc.width = desc.width;
        colorOutputDesc.height = desc.height;
        colorOutputDesc.depth = 1;
        colorOutputDesc.type = IMAGE_TYPE_2D;
        colorOutputDesc.viewType = IMAGE_VIEW_TYPE_2D;
        colorOutputDesc.usageFlags = ENUM_FLAGS(ImageUsageFlags, 
                IMAGE_USAGE_COLOR_ATTACHMENT
                | IMAGE_USAGE_TRANSFER_SRC
                | IMAGE_USAGE_TRANSFER_DST
                | IMAGE_USAGE_SAMPLED);
        colorOutputDesc.format = desc.colorImageFormats[i];
        colorOutputDesc.layout = IMAGE_LAYOUT_UNDEFINED;
        Handle<Texture> hColorOutput = MakeTexture(colorOutputDesc);

        result.outputs.Push(hColorOutput);
    }
    // Then create image for depth attachment (currently all RTs have depth targets)
    TextureDesc depthOutputDesc = {};
    depthOutputDesc.width = desc.width;
    depthOutputDesc.height = desc.height;
    depthOutputDesc.depth = 1;
    depthOutputDesc.type = IMAGE_TYPE_2D;
    depthOutputDesc.viewType = IMAGE_VIEW_TYPE_2D;
    depthOutputDesc.usageFlags = ENUM_FLAGS(ImageUsageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT);
    depthOutputDesc.format = desc.depthImageFormat;
    depthOutputDesc.layout = IMAGE_LAYOUT_UNDEFINED;
    Handle<Texture> hDepthOutput = MakeTexture(depthOutputDesc);

    result.outputs.Push(hDepthOutput);
    result.desc = desc;

    return renderTargets.Insert(result);
}

void DestroyRenderTarget(RenderTarget* renderTarget)
{
    ASSERT(renderTarget);
    // Render target textures are destroyed along with all other textures
    DestroyArray(&renderTarget->outputs);
    *renderTarget = {};
}

Handle<Texture> GetColorOutput(Handle<RenderTarget> hRenderTarget, u32 outputIndex)
{
    ASSERT(hRenderTarget.IsValid());
    RenderTarget& rt = renderTargets[hRenderTarget];
    ASSERT(outputIndex < rt.desc.colorImageCount);
    return rt.outputs[outputIndex];
}

Handle<Texture> GetDepthOutput(Handle<RenderTarget> hRenderTarget)
{
    ASSERT(hRenderTarget.IsValid());
    RenderTarget& rt = renderTargets[hRenderTarget];
    return rt.outputs[rt.desc.colorImageCount];
}

void MakeRenderPass_CreateRenderPass(Context* ctx, RenderPassDesc desc, RenderTarget* renderTarget, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    // Color attachments
    VkAttachmentDescription colorAttachments[renderTarget->desc.colorImageCount];
    VkAttachmentReference colorAttachmentRefs[renderTarget->desc.colorImageCount];
    for(i32 i = 0; i < renderTarget->desc.colorImageCount; i++)
    {
        colorAttachments[i] = {};
        colorAttachments[i].format = (VkFormat)renderTarget->desc.colorImageFormats[i];
        colorAttachments[i].initialLayout = (VkImageLayout)desc.initialLayout;
        colorAttachments[i].finalLayout = (VkImageLayout)desc.finalLayout;
        colorAttachments[i].loadOp = (VkAttachmentLoadOp)desc.loadOp;
        colorAttachments[i].storeOp = (VkAttachmentStoreOp)desc.storeOp;
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
    depthAttachment.format = (VkFormat)renderTarget->desc.depthImageFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentRef.attachment = renderTarget->desc.colorImageCount;    // Depth comes after all color attachments
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass (only supports a single subpass)
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = renderTarget->desc.colorImageCount;
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
    VkAttachmentDescription attachments[renderTarget->desc.colorImageCount + 1];
    for(i32 i = 0; i < renderTarget->desc.colorImageCount; i++)
    {
        attachments[i] = colorAttachments[i];
    }
    attachments[renderTarget->desc.colorImageCount] = depthAttachment;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.attachmentCount = renderTarget->desc.colorImageCount + 1;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;
    VkRenderPass handle;
    VkResult ret = vkCreateRenderPass(ctx->vkDevice, &renderPassInfo, NULL, &handle);
    ASSERTVK(ret);

    renderPass->vkHandle = handle;
    renderPass->desc = desc;
}

void MakeRenderPass_CreateFramebuffer(Context* ctx, RenderTarget* renderTarget, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass->vkHandle;
    framebufferInfo.width = renderTarget->desc.width;
    framebufferInfo.height = renderTarget->desc.height;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = renderTarget->desc.colorImageCount + 1;
    VkImageView attachmentViews[renderTarget->desc.colorImageCount + 1];
    for(i32 i = 0; i < renderTarget->desc.colorImageCount + 1; i++)
    {
        Texture& texture = textures[renderTarget->outputs[i]];
        attachmentViews[i] = texture.vkImageView;
    }
    framebufferInfo.pAttachments = attachmentViews;

    VkFramebuffer framebuffer;
    VkResult ret = vkCreateFramebuffer(ctx->vkDevice, &framebufferInfo, NULL, &framebuffer);
    ASSERTVK(ret);

    renderPass->vkFramebuffer = framebuffer;
}

Handle<RenderPass> MakeRenderPass(RenderPassDesc desc, Handle<RenderTarget> hRenderTarget)
{
    ASSERT(hRenderTarget.IsValid());
    RenderTarget& renderTarget = renderTargets[hRenderTarget];
    mem::SetContext(&renderHeap);

    RenderPass result = {};

    MakeRenderPass_CreateRenderPass(&ctx, desc, &renderTarget, &result);
    MakeRenderPass_CreateFramebuffer(&ctx, &renderTarget, &result);
    result.hRenderTarget = hRenderTarget;

    return renderPasses.Insert(result);
}

void DestroyRenderPass(RenderPass *renderPass)
{
    ASSERT(renderPass);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && renderPass->vkHandle != VK_NULL_HANDLE);
    vkDestroyFramebuffer(ctx.vkDevice, renderPass->vkFramebuffer, NULL);
    vkDestroyRenderPass(ctx.vkDevice, renderPass->vkHandle, NULL);

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

        VkVertexInputAttributeDescription attributeDesc = {};
        attributeDesc.binding = 0;              // Change this if needed later?
        attributeDesc.location = i;
        attributeDesc.format = vertexAttributeFormats[attr];
        attributeDesc.offset = result.vkBindingDescription.stride;

        result.vkBindingDescription.stride += vertexAttributeSizes[attr];
        
        result.attributes.Push(attr);
        result.vkAttributeDescriptions.Push(attributeDesc);
    }

    return vertexLayouts.Insert(result);
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

    return shaders.Insert(result);
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
    bufferInfo.usage = (VkBufferUsageFlags)type;
    
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

    Handle<Buffer> hResult = buffers.Insert(result);

    if(data) CopyMemoryToBuffer(hResult, 0, size, data);
    return hResult;
}

void DestroyBuffer(Buffer* buffer)
{
    ASSERT(buffer);
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    vmaDestroyBuffer(ctx.vkAllocator, buffer->vkHandle, buffer->vkAllocation);
    *buffer = {};
}

void CopyMemoryToBuffer(Handle<Buffer> hDstBuffer, u64 dstOffset, u64 srcSize, void* srcData)
{
    ASSERT(hDstBuffer.IsValid());
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    ASSERT(srcData);

    Buffer& buffer = buffers[hDstBuffer];
    void* mapping = NULL;
    VkResult ret = vmaMapMemory(ctx.vkAllocator, buffer.vkAllocation, &mapping);
    //memcpy(mapping, data, size);
    ASSERTVK(ret);
    memcpy((void*)((u64)mapping + dstOffset), srcData, srcSize);
    vmaUnmapMemory(ctx.vkAllocator, buffer.vkAllocation);
}

u32 GetBufferTypeAlignment(BufferType type)
{
    ASSERT(ctx.vkPhysicalDevice != VK_NULL_HANDLE);
    switch (type)
    {
        case BUFFER_TYPE_UNIFORM: return ctx.vkPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        case BUFFER_TYPE_STORAGE: return ctx.vkPhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
        default: return 1;
    }
}

Handle<Texture> MakeTexture(TextureDesc desc)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE && ctx.vkAllocator != VK_NULL_HANDLE);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.usage = desc.usageFlags;
    imageInfo.format = (VkFormat)desc.format;
    imageInfo.initialLayout = (VkImageLayout)desc.layout;
    imageInfo.imageType = (VkImageType)desc.type;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = desc.depth;
    imageInfo.mipLevels = desc.mipLevels;
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
    imageViewInfo.viewType = (VkImageViewType)desc.viewType;
    imageViewInfo.format = (VkFormat)desc.format;
    imageViewInfo.subresourceRange.aspectMask = ENUM_HAS_FLAG(desc.usageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = desc.mipLevels;
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

    return textures.Insert(result);
}

void DestroyTexture(Texture* texture)
{
    ASSERT(texture);
    ASSERT(ctx.vkAllocator != VK_NULL_HANDLE);
    vkDestroyImageView(ctx.vkDevice, texture->vkImageView, NULL);
    vmaDestroyImage(ctx.vkAllocator, texture->vkHandle, texture->vkAllocation);
    *texture = {};
}

u32 GetMaxMipLevels(u32 w, u32 h)
{
    return (u32)(floorf(log2f(MAX(w, h))));
}

Handle<Sampler> MakeSampler(SamplerDesc desc)
{
    ASSERT(ctx.vkPhysicalDevice != VK_NULL_HANDLE);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = (VkFilter)desc.minFilter;
    samplerInfo.magFilter = (VkFilter)desc.magFilter;
    samplerInfo.addressModeU = (VkSamplerAddressMode)desc.addressModeU;
    samplerInfo.addressModeV = (VkSamplerAddressMode)desc.addressModeV;
    samplerInfo.addressModeW = (VkSamplerAddressMode)desc.addressModeW;
    samplerInfo.anisotropyEnable = VK_TRUE;     // Anisotropy hardcoded
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties(ctx.vkPhysicalDevice, &vkPhysicalDeviceProperties);
    samplerInfo.maxAnisotropy = vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = (VkSamplerMipmapMode)desc.mipFilter;
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 1000.f;    // Just for clamping purposes.
    
    VkSampler sampler;
    VkResult ret = vkCreateSampler(ctx.vkDevice, &samplerInfo, NULL, &sampler);
    ASSERTVK(ret);

    Sampler result = {};
    result.vkHandle = sampler;
    result.desc = desc;

    return samplers.Insert(result);
}

void DestroySampler(Sampler* sampler)
{
    ASSERT(sampler);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    vkDestroySampler(ctx.vkDevice, sampler->vkHandle, NULL);
    *sampler = {};
}

Handle<ResourceSetLayout> MakeResourceSetLayout(i32 entryCount, ResourceSetLayout::Entry *entries)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    mem::SetContext(&renderHeap);

    VkDescriptorSetLayoutBinding vkLayoutBindings[entryCount];
    for(i32 i = 0; i < entryCount; i++)
    {
        ResourceSetLayout::Entry entry = entries[i];
        VkDescriptorSetLayoutBinding vkBinding = {};
        vkBinding.binding = i;
        vkBinding.stageFlags = entry.shaderStages;
        vkBinding.descriptorType = (VkDescriptorType)entry.type;
        vkBinding.descriptorCount = entry.count;
        vkLayoutBindings[i] = vkBinding;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = entryCount;   // TODO(caio): Multiple descriptors for single binding
    layoutInfo.pBindings = vkLayoutBindings;
    VkDescriptorSetLayout vkLayout;
    VkResult ret = vkCreateDescriptorSetLayout(ctx.vkDevice, &layoutInfo, NULL, &vkLayout);
    ASSERTVK(ret);

    ResourceSetLayout result = {};
    result.vkDescriptorSetLayout = vkLayout;
    result.entries = MakeArray<ResourceSetLayout::Entry>(entryCount);
    for(i32 i = 0; i < entryCount; i++)
    {
        result.entries.Push(entries[i]);
    }

    return resourceSetLayouts.Insert(result);
}

void DestroyResourceSetLayout(ResourceSetLayout *resourceSetLayout)
{
    ASSERT(resourceSetLayout);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(ctx.vkDevice, resourceSetLayout->vkDescriptorSetLayout, NULL);
    DestroyArray(&resourceSetLayout->entries);
    *resourceSetLayout = {};
}

Handle<ResourceSet> MakeResourceSet(Handle<ResourceSetLayout> hResourceSetLayout)
{
    ASSERT(hResourceSetLayout.IsValid());
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    mem::SetContext(&renderHeap);
    ResourceSetLayout& layout = resourceSetLayouts[hResourceSetLayout];

    VkDescriptorSetAllocateInfo vkDescriptorSetAllocInfo = {};
    vkDescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vkDescriptorSetAllocInfo.descriptorPool = ctx.vkDescriptorPool;
    vkDescriptorSetAllocInfo.descriptorSetCount = 1;
    vkDescriptorSetAllocInfo.pSetLayouts = &layout.vkDescriptorSetLayout;
    VkDescriptorSet vkDescriptorSet;
    VkResult ret = vkAllocateDescriptorSets(ctx.vkDevice, &vkDescriptorSetAllocInfo, &vkDescriptorSet);
    ASSERTVK(ret);

    ResourceSet result = {};
    result.vkDescriptorSet = vkDescriptorSet;
    result.resourceCount = 0;
    result.hBuffers = MakeArray<Handle<Buffer>>(RENDER_MAX_RESOURCE_SET_BUFFERS);
    result.hTextures = MakeArray<Handle<Texture>>(RENDER_MAX_RESOURCE_SET_TEXTURES);
    result.hSamplers = MakeArray<Handle<Sampler>>(RENDER_MAX_RESOURCE_SET_SAMPLERS);

    return resourceSets.Insert(result);
}

void AddToResourceSet(Handle<ResourceSet> hRS, Handle<Buffer> hBuffer, ResourceType bufferType)
{
    ResourceSet& resourceSet = resourceSets[hRS];
    u32 i = resourceSet.resourceCount++;
    resourceSet.hBuffers.Push(hBuffer);
    resourceSet.resources[i].binding = i;
    resourceSet.resources[i].type = bufferType;
    resourceSet.resources[i].index = resourceSet.hBuffers.count - 1;
    resourceSet.resources[i].len = 1;
}

void AddToResourceSet(Handle<ResourceSet> hRS, Handle<Texture> hTexture)
{
    ResourceSet& resourceSet = resourceSets[hRS];
    u32 i = resourceSet.resourceCount++;
    resourceSet.hTextures.Push(hTexture);
    resourceSet.resources[i].binding = i;
    resourceSet.resources[i].type = RESOURCE_SAMPLED_TEXTURE;
    resourceSet.resources[i].index = resourceSet.hTextures.count - 1;
    resourceSet.resources[i].len = 1;
}

//void AddToResourceSet(Handle<ResourceSet> hRS, Handle<Sampler> hSampler)
//{
//    ResourceSet& resourceSet = resourceSets[hRS];
//    u32 i = resourceSet.resourceCount++;
//    resourceSet.hSamplers.Push(hSampler);
//    resourceSet.resources[i].binding = i;
//    resourceSet.resources[i].type = RESOURCE_SAMPLER;
//    resourceSet.resources[i].index = resourceSet.hSamplers.count - 1;
//    resourceSet.resources[i].len = 1;
//}

void AddToResourceSet(Handle<ResourceSet> hRS, u32 textureCount, Handle<Texture>* hTextureArray, Handle<Sampler>* hSamplerArray)
{
    ASSERT(textureCount > 0);
    ASSERT(hTextureArray);
    ASSERT(hSamplerArray);
    ResourceSet& resourceSet = resourceSets[hRS];
    u32 i = resourceSet.resourceCount++;
    resourceSet.resources[i].binding = i;
    resourceSet.resources[i].type = RESOURCE_SAMPLED_TEXTURE;
    resourceSet.resources[i].index = resourceSet.hTextures.count;
    resourceSet.resources[i].index2 = resourceSet.hSamplers.count;
    resourceSet.resources[i].len = textureCount;

    for(i32 j = 0; j < textureCount; j++)
    {
        resourceSet.hTextures.Push(hTextureArray[j]);
        resourceSet.hSamplers.Push(hSamplerArray[j]);
    }
}

void AddToResourceSet(Handle<ResourceSet> hRS, Array<Handle<Texture>>& hTextureArray, Array<Handle<Sampler>>& hSamplerArray)
{
    ASSERT(hTextureArray.count == hSamplerArray.count);
    return AddToResourceSet(hRS, hTextureArray.count, hTextureArray.data, hSamplerArray.data);
}

void UpdateResourceSet(Handle<ResourceSet> hRS)
{
    ResourceSet& resourceSet = resourceSets[hRS];

    VkWriteDescriptorSet vkDescriptorSetWrites[resourceSet.resourceCount];
    VkDescriptorBufferInfo bufferInfos[resourceSet.hBuffers.count];
    VkDescriptorImageInfo imageInfos[resourceSet.hTextures.count];
    u32 bufferInfoCursor = 0;
    u32 imageInfoCursor = 0;

    for(i32 i = 0; i < resourceSet.resourceCount; i++)
    {
        ResourceSet::Entry resource = resourceSet.resources[i];
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = resourceSet.vkDescriptorSet;
        write.dstBinding = resource.binding;
        write.descriptorCount = resource.len;
        write.descriptorType = (VkDescriptorType)resource.type;

        switch(resource.type)
        {
            case RESOURCE_UNIFORM_BUFFER:
            case RESOURCE_STORAGE_BUFFER:
            case RESOURCE_DYNAMIC_UNIFORM_BUFFER:
            case RESOURCE_DYNAMIC_STORAGE_BUFFER:
            {
                Buffer& buffer = buffers[resourceSet.hBuffers[resource.index]];

                VkDescriptorBufferInfo* bufferInfo = &bufferInfos[bufferInfoCursor];
                *bufferInfo = {};
                bufferInfo->buffer = buffer.vkHandle;
                bufferInfo->offset = 0;
                bufferInfo->range = buffer.stride;

                write.pBufferInfo = bufferInfo;
                bufferInfoCursor++;
            } break;
            case RESOURCE_SAMPLED_TEXTURE:
            {
                u32 startImageCursor = imageInfoCursor;
                for(i32 j = 0; j < resource.len; j++)
                {
                    Texture& texture = textures[resourceSet.hTextures[resource.index + j]];
                    Sampler& sampler = samplers[resourceSet.hSamplers[resource.index2 + j]];

                    VkDescriptorImageInfo* imageInfo = &imageInfos[imageInfoCursor];
                    *imageInfo = {};
                    imageInfo->imageView = texture.vkImageView;
                    imageInfo->imageLayout = (VkImageLayout)texture.desc.layout;
                    imageInfo->sampler = sampler.vkHandle;

                    imageInfoCursor++;
                }

                write.pImageInfo = &imageInfos[startImageCursor];
            } break;
            default: ASSERT(0);
        }
        vkDescriptorSetWrites[i] = write;
    }
    vkUpdateDescriptorSets(ctx.vkDevice, resourceSet.resourceCount, vkDescriptorSetWrites, 0, NULL);
}

void DestroyResourceSet(ResourceSet *resourceSet)
{
    ASSERT(resourceSet);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    //DestroyArray(&resourceSet->resources);
    DestroyArray(&resourceSet->hBuffers);
    DestroyArray(&resourceSet->hTextures);
    DestroyArray(&resourceSet->hSamplers);
    *resourceSet = {};
}

Handle<GraphicsPipeline> MakeGraphicsPipeline(Handle<RenderPass> hRenderPass, GraphicsPipelineDesc desc, u32 resourceSetLayoutCount, Handle<ResourceSetLayout>* hResourceSetLayouts)
{
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);

    RenderPass& renderPass = renderPasses[hRenderPass];
    VertexLayout& vertexLayout = vertexLayouts[desc.hVertexLayout];
    RenderTarget& renderTarget = renderTargets[renderPass.hRenderTarget];
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
    inputAssemblyInfo.topology = (VkPrimitiveTopology)desc.primitive;
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
    rasterizationInfo.polygonMode = (VkPolygonMode)desc.fillMode;
    rasterizationInfo.cullMode = (VkCullModeFlags)desc.cullMode;
    rasterizationInfo.frontFace = (VkFrontFace)desc.frontFace;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.lineWidth = 1;
    rasterizationInfo.depthBiasClamp = VK_FALSE;

    // Blending
    //VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    //colorBlendAttachment.colorWriteMask =
    //    VK_COLOR_COMPONENT_R_BIT |
    //    VK_COLOR_COMPONENT_G_BIT |
    //    VK_COLOR_COMPONENT_B_BIT |
    //    VK_COLOR_COMPONENT_A_BIT;
    //colorBlendAttachment.blendEnable = VK_FALSE;     // No blending, overwrite color
    //VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    //colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    //colorBlendInfo.logicOpEnable = VK_FALSE;
    //colorBlendInfo.attachmentCount = 1;
    //colorBlendInfo.pAttachments = &colorBlendAttachment;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachments[renderTarget.desc.colorImageCount];
    for(i32 i = 0; i < renderTarget.desc.colorImageCount; i++)
    {
        //TODO(caio): support other blend modes rather than overwrite
        colorBlendAttachments[i] = {};
        colorBlendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments[i].blendEnable = VK_FALSE;     // No blending, overwrite color
    }
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.attachmentCount = renderTarget.desc.colorImageCount;
    colorBlendInfo.pAttachments = colorBlendAttachments;

    // Depth state
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    VkDescriptorSetLayout descriptorSetLayouts[RENDER_MAX_RESOURCE_SET_LAYOUTS_PER_PIPELINE];
    for(i32 i = 0; i < resourceSetLayoutCount; i++)
    {
        ASSERT(hResourceSetLayouts[i].IsValid());
        ResourceSetLayout& layout = resourceSetLayouts[hResourceSetLayouts[i]];
        descriptorSetLayouts[i] = layout.vkDescriptorSetLayout;
    }

    VkPushConstantRange vkPushConstantRanges[RENDER_MAX_PUSH_CONSTANT_RANGES];
    for(i32 i = 0; i < desc.pushConstantRangeCount; i++)
    {
        vkPushConstantRanges[i].offset = desc.pushConstantRanges[i].offset;
        vkPushConstantRanges[i].size = desc.pushConstantRanges[i].size;
        vkPushConstantRanges[i].stageFlags = (VkShaderStageFlags)desc.pushConstantRanges[i].shaderStages;
    }

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = resourceSetLayoutCount;
    layoutInfo.pSetLayouts = descriptorSetLayouts;
    layoutInfo.pushConstantRangeCount = desc.pushConstantRangeCount;
    layoutInfo.pPushConstantRanges = vkPushConstantRanges;
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

    return graphicsPipelines.Insert(result);
}

void DestroyGraphicsPipeline(GraphicsPipeline* pipeline)
{
    ASSERT(pipeline);
    ASSERT(ctx.vkDevice != VK_NULL_HANDLE);
    vkDestroyPipelineLayout(ctx.vkDevice, pipeline->vkPipelineLayout, NULL);
    vkDestroyPipeline(ctx.vkDevice, pipeline->vkPipeline, NULL);
}

Handle<ComputePipeline> MakeComputePipeline(ComputePipelineDesc desc, u32 resourceSetLayoutCount, Handle<ResourceSetLayout>* hResourceSetLayouts)
{ 
    Shader& cs = shaders[desc.hShaderCompute];
    ASSERT(cs.type == SHADER_TYPE_COMPUTE);

    // Shader stages
    VkPipelineShaderStageCreateInfo csStageInfo = {};
    csStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    csStageInfo.pName = "main";     // Shader entrypoint is always main
    csStageInfo.module = cs.vkShaderModule;
    csStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayout descriptorSetLayouts[resourceSetLayoutCount];
    for(i32 i = 0; i < resourceSetLayoutCount; i++)
    {
        ASSERT(hResourceSetLayouts[i].IsValid());
        ResourceSetLayout& layout = resourceSetLayouts[hResourceSetLayouts[i]];
        descriptorSetLayouts[i] = layout.vkDescriptorSetLayout;
    }

    VkPushConstantRange vkPushConstantRanges[RENDER_MAX_PUSH_CONSTANT_RANGES];
    for(i32 i = 0; i < desc.pushConstantRangeCount; i++)
    {
        vkPushConstantRanges[i].offset = desc.pushConstantRanges[i].offset;
        vkPushConstantRanges[i].size = desc.pushConstantRanges[i].size;
        vkPushConstantRanges[i].stageFlags = (VkShaderStageFlags)desc.pushConstantRanges[i].shaderStages;
    }

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = resourceSetLayoutCount;
    layoutInfo.pSetLayouts = descriptorSetLayouts;
    layoutInfo.pushConstantRangeCount = desc.pushConstantRangeCount;
    layoutInfo.pPushConstantRanges = vkPushConstantRanges;
    VkPipelineLayout pipelineLayout;
    VkResult ret = vkCreatePipelineLayout(ctx.vkDevice, &layoutInfo, NULL, &pipelineLayout);
    ASSERTVK(ret);

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage = csStageInfo;
    VkPipeline pipeline;
    ret = vkCreateComputePipelines(ctx.vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);
    ASSERTVK(ret);

    ComputePipeline result = {};
    result.vkPipeline = pipeline;
    result.vkPipelineLayout = pipelineLayout;
    result.desc = desc;

    return computePipelines.Insert(result);
}

void DestroyComputePipeline(ComputePipeline* pipeline)
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
    ASSERT(cmd.state == COMMAND_BUFFER_IDLE);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult ret = vkBeginCommandBuffer(cmd.vkHandle, &beginInfo);
    ASSERTVK(ret);

    cmd.state = COMMAND_BUFFER_RECORDING;
}

void EndCommandBuffer(Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(cmd.state == COMMAND_BUFFER_RECORDING);

    VkResult ret = vkEndCommandBuffer(cmd.vkHandle);
    ASSERTVK(ret);

    cmd.state = COMMAND_BUFFER_RECORDED;
}

void SubmitImmediate(Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(cmd.state == COMMAND_BUFFER_RECORDED);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.vkHandle;
    
    VkResult ret = vkQueueSubmit(ctx.vkCommandQueue, 1, &submitInfo, cmd.vkFence);
    ASSERTVK(ret);
    cmd.state = COMMAND_BUFFER_PENDING;

    ret = vkWaitForFences(ctx.vkDevice, 1, &cmd.vkFence, VK_TRUE, MAX_U64);
    ASSERTVK(ret);
}

void BeginRenderPass(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass)
{
    ASSERT(hCmd.IsValid() && hRenderPass.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    RenderPass& renderPass = renderPasses[hRenderPass];
    RenderTarget& renderTarget = renderTargets[renderPass.hRenderTarget];

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass.vkHandle;
    beginInfo.framebuffer = renderPass.vkFramebuffer;
    beginInfo.renderArea.offset.x = 0;
    beginInfo.renderArea.offset.y = 0;
    beginInfo.renderArea.extent =
    {
        renderTarget.desc.width,
        renderTarget.desc.height,
    };

    //VkClearValue clearValues[2];
    //clearValues[0].color = {0, 0, 0, 1};
    //clearValues[1].depthStencil = {1, 0};
    //beginInfo.clearValueCount = ARR_LEN(clearValues);
    //beginInfo.pClearValues = clearValues;
    VkClearValue clearValues[renderTarget.desc.colorImageCount + 1];
    for(i32 i = 0; i < renderTarget.desc.colorImageCount; i++)
    {
        clearValues[i].color = {0,0,0,1};
    }
    clearValues[renderTarget.desc.colorImageCount].depthStencil = {1, 0};
    beginInfo.clearValueCount = renderTarget.desc.colorImageCount + 1;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd.vkHandle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Color image layouts are automatically transitioned within render passes.
    // When vkCmdBeginRenderPass is called, color outputs transition to initial layout,
    // then subpass 0 starts and they transition to COLOR_OUTPUT.
    for(i32 i = 0; i < renderTarget.desc.colorImageCount; i++)
    {
        ASSERT(GetColorOutput(renderPass.hRenderTarget, i).IsValid());
        Texture& colorOutput = textures[GetColorOutput(renderPass.hRenderTarget, i)];
        //colorOutput.desc.layout = renderPass.desc.initialLayout;
        colorOutput.desc.layout = IMAGE_LAYOUT_COLOR_OUTPUT;
    }
    // Depth image layouts are hardcoded in render passes. Initial is always
    // UNDEFINED, then subpass 0 sets it to DEPTH_STENCIL_OUTPUT.
    ASSERT(GetDepthOutput(renderPass.hRenderTarget).IsValid());
    Texture& depthOutput = textures[GetDepthOutput(renderPass.hRenderTarget)];
    depthOutput.desc.layout = IMAGE_LAYOUT_DEPTH_STENCIL_OUTPUT;
}

void EndRenderPass(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass)
{
    ASSERT(hCmd.IsValid() && hRenderPass.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    RenderPass& renderPass = renderPasses[hRenderPass];
    RenderTarget& renderTarget = renderTargets[renderPass.hRenderTarget];

    vkCmdEndRenderPass(cmd.vkHandle);

    // Color image layouts are automatically transitioned within render passes.
    // When vkCmdEndRenderPass is called, color outputs transition to final layout.
    for(i32 i = 0; i < renderTarget.desc.colorImageCount; i++)
    {
        ASSERT(GetColorOutput(renderPass.hRenderTarget, i).IsValid());
        Texture& colorOutput = textures[GetColorOutput(renderPass.hRenderTarget, i)];
        colorOutput.desc.layout = renderPass.desc.finalLayout;
    }

    // Depth image layouts are hardcoded in render passes.
    // Final layout is always DEPTH_STENCIL_OUTPUT.
    ASSERT(GetDepthOutput(renderPass.hRenderTarget).IsValid());
    Texture& depthOutput = textures[GetDepthOutput(renderPass.hRenderTarget)];
    depthOutput.desc.layout = IMAGE_LAYOUT_DEPTH_STENCIL_OUTPUT;
}

void CmdPipelineBarrier(Handle<CommandBuffer> hCmd, Barrier barrier)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];

    VkMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    vkBarrier.srcAccessMask = (VkAccessFlags)barrier.srcAccess;
    vkBarrier.dstAccessMask = (VkAccessFlags)barrier.dstAccess;

    vkCmdPipelineBarrier(cmd.vkHandle, (VkPipelineStageFlags)barrier.srcStage, (VkPipelineStageFlags)barrier.dstStage, 0,
            1, &vkBarrier, 0, NULL, 0, NULL);
}

void CmdPipelineBarrierTextureLayout(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture, ImageLayout newLayout, Barrier barrier)
{
    ASSERT(hCmd.IsValid() && hTexture.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Texture& texture = textures[hTexture];

    //if(texture.desc.layout == newLayout) return;
    // I believe I can't just skip the transition in the above case,
    // since memory access between pipeline stages still need to synchronize.
    // I might be wrong.

    VkImageLayout vkOldLayout = (VkImageLayout)texture.desc.layout;
    VkImageLayout vkNewLayout = (VkImageLayout)newLayout;
    VkImageMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.oldLayout = vkOldLayout;
    vkBarrier.newLayout = vkNewLayout;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = texture.vkHandle;
    vkBarrier.subresourceRange.aspectMask = ENUM_HAS_FLAG(texture.desc.usageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    vkBarrier.subresourceRange.baseMipLevel = 0;
    vkBarrier.subresourceRange.levelCount = texture.desc.mipLevels;
    vkBarrier.subresourceRange.baseArrayLayer = 0;
    vkBarrier.subresourceRange.layerCount = 1;
    vkBarrier.srcAccessMask = (VkAccessFlags)barrier.srcAccess;
    vkBarrier.dstAccessMask = (VkAccessFlags)barrier.dstAccess;

    vkCmdPipelineBarrier(cmd.vkHandle, (VkPipelineStageFlags)barrier.srcStage, (VkPipelineStageFlags)barrier.dstStage, 0,
            0, NULL, 0, NULL, 1, &vkBarrier);

    texture.desc.layout = newLayout;
}

void CmdPipelineBarrierTextureMipLayout(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture, ImageLayout oldLayout, ImageLayout newLayout, Barrier barrier, u32 mipLevel)
{
    ASSERT(hCmd.IsValid() && hTexture.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Texture& texture = textures[hTexture];

    //TODO(caio): mip layouts
    // This will fire a validation error when oldLayout is actually not the correct layout.
    // I currently can't verify this in code since I'm not tracking layouts per mip, but only
    // per image.
    VkImageLayout vkOldLayout = (VkImageLayout)oldLayout;
    VkImageLayout vkNewLayout = (VkImageLayout)newLayout;
    VkImageMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.oldLayout = vkOldLayout;
    vkBarrier.newLayout = vkNewLayout;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = texture.vkHandle;
    vkBarrier.subresourceRange.aspectMask = ENUM_HAS_FLAG(texture.desc.usageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    vkBarrier.subresourceRange.baseMipLevel = mipLevel;
    vkBarrier.subresourceRange.levelCount = 1;
    vkBarrier.subresourceRange.baseArrayLayer = 0;
    vkBarrier.subresourceRange.layerCount = 1;
    vkBarrier.srcAccessMask = (VkAccessFlags)barrier.srcAccess;
    vkBarrier.dstAccessMask = (VkAccessFlags)barrier.dstAccess;

    vkCmdPipelineBarrier(cmd.vkHandle, (VkPipelineStageFlags)barrier.srcStage, (VkPipelineStageFlags)barrier.dstStage, 0, 0, NULL, 0, NULL, 1, &vkBarrier);

    //texture.desc.layout = newLayout;
}

void CmdGenerateMipmaps(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture)
{
    ASSERT(ctx.vkPhysicalDevice != VK_NULL_HANDLE);
    ASSERT(hCmd.IsValid() && hTexture.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Texture& texture = textures[hTexture];

    // Check for linear blit support for this texture format
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(ctx.vkPhysicalDevice, (VkFormat)texture.desc.format, &properties);
    ASSERT(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    // First, all levels must be in TRANSFER_DST
    Barrier barrier = {};
    barrier.srcAccess = MEMORY_ACCESS_TRANSFER_WRITE;
    barrier.dstAccess = MEMORY_ACCESS_TRANSFER_READ;
    barrier.srcStage = PIPELINE_STAGE_TRANSFER;
    barrier.dstStage = PIPELINE_STAGE_TRANSFER;
    CmdPipelineBarrierTextureLayout(hCmd, hTexture, IMAGE_LAYOUT_TRANSFER_DST, barrier);

    // Then, starting from level 1, blit past level into current level
    i32 mipWidth = texture.desc.width;
    i32 mipHeight = texture.desc.height;
    for(i32 i = 1; i < texture.desc.mipLevels; i++)
    {
        CmdPipelineBarrierTextureMipLayout(hCmd, hTexture, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_TRANSFER_SRC, barrier, i-1);

        VkImageBlit blitRegion = {};
        blitRegion.srcOffsets[0] = {0, 0, 0};
        blitRegion.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.mipLevel = i - 1;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.dstOffsets[0] = {0, 0, 0};
        blitRegion.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.mipLevel = i;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        vkCmdBlitImage(cmd.vkHandle,
                texture.vkHandle,
                (VkImageLayout)IMAGE_LAYOUT_TRANSFER_SRC,
                texture.vkHandle,
                (VkImageLayout)IMAGE_LAYOUT_TRANSFER_DST,
                1, &blitRegion,
                (VkFilter)texture.desc.mipSamplerFilter);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    // TODO(caio): mip layouts
    // At the end, all mips will have layout set to TRANSFER_SRC, so do that manually here
    // since I'm not tracking mip layouts individually
    CmdPipelineBarrierTextureMipLayout(hCmd, hTexture, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_TRANSFER_SRC, barrier, texture.desc.mipLevels - 1);
    texture.desc.layout = IMAGE_LAYOUT_TRANSFER_SRC;
}

void CmdCopyBufferToTexture(Handle<CommandBuffer> hCmd, Handle<Buffer> hSrc, Handle<Texture> hDst)
{
    ASSERT(hCmd.IsValid() && hSrc.IsValid() && hDst.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Buffer& src = buffers[hSrc];
    Texture& dst = textures[hDst];
    ASSERT(dst.desc.layout == IMAGE_LAYOUT_TRANSFER_DST);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;   // TODO(caio): Support mipmaps
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent =
    {
        dst.desc.width, dst.desc.height, dst.desc.depth,
    };
    vkCmdCopyBufferToImage(cmd.vkHandle, src.vkHandle, dst.vkHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CmdClearColorTexture(Handle<CommandBuffer> hCmd, Handle<Texture> hTexture, f32 r, f32 g, f32 b, f32 a)
{
    ASSERT(hCmd.IsValid() && hTexture.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Texture& texture = textures[hTexture];
    ASSERT(texture.desc.layout == IMAGE_LAYOUT_GENERAL || texture.desc.layout == IMAGE_LAYOUT_TRANSFER_DST);

    VkClearColorValue clearValue = {r, g, b, a};
    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.baseMipLevel = 0;
    imageRange.layerCount = 1;
    imageRange.baseArrayLayer = 0;

    vkCmdClearColorImage(cmd.vkHandle, texture.vkHandle, (VkImageLayout)texture.desc.layout, &clearValue, 1, &imageRange);
}

void CmdCopyToSwapChain(Handle<CommandBuffer> hCmd, Handle<Texture> hSrc)
{
    ASSERT(hCmd.IsValid() && hSrc.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Texture& src = textures[hSrc];
    ASSERT(src.desc.layout == IMAGE_LAYOUT_GENERAL || src.desc.layout == IMAGE_LAYOUT_TRANSFER_SRC);

    VkImage swapChainImage = swapChain.vkImages[swapChain.activeImage];
    

    // Transition swap chain dst image to transfer dst (manually)
    VkImageMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.oldLayout = (VkImageLayout)swapChain.imageLayouts[swapChain.activeImage];
    vkBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = swapChainImage;
    vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkBarrier.subresourceRange.baseMipLevel = 0;
    vkBarrier.subresourceRange.levelCount = 1;
    vkBarrier.subresourceRange.baseArrayLayer = 0;
    vkBarrier.subresourceRange.layerCount = 1;
    vkBarrier.srcAccessMask = VK_ACCESS_NONE;
    vkBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd.vkHandle,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            1,
            &vkBarrier);

    // Blit src to swap chain image
    VkOffset3D blitSrcSize = {};
    blitSrcSize.x = src.desc.width;
    blitSrcSize.y = src.desc.height;
    blitSrcSize.z = 1;
    VkOffset3D blitDstSize = {};
    blitDstSize.x = swapChain.vkExtents.width;
    blitDstSize.y = swapChain.vkExtents.height;
    blitDstSize.z = 1;
    VkImageBlit blitRegion = {};
    blitRegion.srcSubresource.aspectMask = ENUM_HAS_FLAG(src.desc.usageFlags, IMAGE_USAGE_DEPTH_ATTACHMENT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcOffsets[1] = blitSrcSize;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstOffsets[1] = blitDstSize;

    vkCmdBlitImage(cmd.vkHandle,
            src.vkHandle,
            (VkImageLayout)src.desc.layout,
            swapChainImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blitRegion,
            VK_FILTER_NEAREST);

    // Transition swap chain dst image to present src (manually)
    vkBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    vkBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = swapChainImage;
    vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkBarrier.subresourceRange.baseMipLevel = 0;
    vkBarrier.subresourceRange.levelCount = 1;
    vkBarrier.subresourceRange.baseArrayLayer = 0;
    vkBarrier.subresourceRange.layerCount = 1;
    vkBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(cmd.vkHandle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            1,
            &vkBarrier);
    swapChain.imageLayouts[swapChain.activeImage] = IMAGE_LAYOUT_PRESENT_SRC;
}

void CmdBindGraphicsPipeline(Handle<CommandBuffer> hCmd, Handle<GraphicsPipeline> hPipeline)
{
    ASSERT(hCmd.IsValid() && hPipeline.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    GraphicsPipeline& pipeline = graphicsPipelines[hPipeline];
    vkCmdBindPipeline(cmd.vkHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vkPipeline);
}

void CmdBindComputePipeline(Handle<CommandBuffer> hCmd, Handle<ComputePipeline> hPipeline)
{
    ASSERT(hCmd.IsValid() && hPipeline.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ComputePipeline& pipeline = computePipelines[hPipeline];
    vkCmdBindPipeline(cmd.vkHandle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.vkPipeline);
}

void CmdBindGraphicsResources(Handle<CommandBuffer> hCmd, Handle<GraphicsPipeline> hPipeline, Handle<ResourceSet> hResourceSet, u32 resourceSetIndex, u32 dynamicOffsetCount, u32* dynamicOffsets)
{
    ASSERT(hCmd.IsValid() && hResourceSet.IsValid() && hPipeline.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ResourceSet& resourceSet = resourceSets[hResourceSet];
    GraphicsPipeline& pipeline = graphicsPipelines[hPipeline];
    vkCmdBindDescriptorSets(cmd.vkHandle, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipeline.vkPipelineLayout, 
            resourceSetIndex, 
            1, 
            &resourceSet.vkDescriptorSet,
            dynamicOffsetCount,
            dynamicOffsets);
}

void CmdBindComputeResources(Handle<CommandBuffer> hCmd, Handle<ComputePipeline> hPipeline, Handle<ResourceSet> hResourceSet, u32 resourceSetIndex, u32 dynamicOffsetCount, u32* dynamicOffsets)
{
    ASSERT(hCmd.IsValid() && hResourceSet.IsValid() && hPipeline.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ResourceSet& resourceSet = resourceSets[hResourceSet];
    ComputePipeline& pipeline = computePipelines[hPipeline];
    vkCmdBindDescriptorSets(cmd.vkHandle, 
            VK_PIPELINE_BIND_POINT_COMPUTE, 
            pipeline.vkPipelineLayout, 
            resourceSetIndex, 
            1, 
            &resourceSet.vkDescriptorSet,
            dynamicOffsetCount,
            dynamicOffsets);
}

void CmdUpdatePushConstantRange(Handle<CommandBuffer> hCmd, u32 rangeIndex, void* data, Handle<GraphicsPipeline> hPipeline)
{
    ASSERT(hCmd.IsValid() && hPipeline.IsValid());
    ASSERT(data);
    CommandBuffer& cmd = commandBuffers[hCmd];
    GraphicsPipeline& pipeline = graphicsPipelines[hPipeline];
    ASSERT(rangeIndex < pipeline.desc.pushConstantRangeCount);
    PushConstantRange pushConstantRange = pipeline.desc.pushConstantRanges[rangeIndex];
    vkCmdPushConstants(cmd.vkHandle, pipeline.vkPipelineLayout, 
            (VkShaderStageFlags)pushConstantRange.shaderStages,
            pushConstantRange.offset,
            pushConstantRange.size,
            data);
}

void CmdUpdatePushConstantRange(Handle<CommandBuffer> hCmd, u32 rangeIndex, void* data, Handle<ComputePipeline> hPipeline)
{
    ASSERT(hCmd.IsValid() && hPipeline.IsValid());
    ASSERT(data);
    CommandBuffer& cmd = commandBuffers[hCmd];
    ComputePipeline& pipeline = computePipelines[hPipeline];
    ASSERT(rangeIndex < pipeline.desc.pushConstantRangeCount);
    PushConstantRange pushConstantRange = pipeline.desc.pushConstantRanges[rangeIndex];
    vkCmdPushConstants(cmd.vkHandle, pipeline.vkPipelineLayout, 
            (VkShaderStageFlags)pushConstantRange.shaderStages,
            pushConstantRange.offset,
            pushConstantRange.size,
            data);
}

void CmdSetViewport(Handle<CommandBuffer> hCmd, f32 offsetX, f32 offsetY, f32 width, f32 height, f32 minDepth, f32 maxDepth)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    VkViewport viewport = {};
    viewport.x = offsetX;
    viewport.y = offsetY;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    vkCmdSetViewport(cmd.vkHandle, 0, 1, &viewport);
}

void CmdSetViewport(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass)
{
    //TODO(caio): Should this use RenderPass or RenderTarget as arg?
    ASSERT(hRenderPass.IsValid());
    RenderPass& renderPass = renderPasses[hRenderPass];
    RenderTarget& renderTarget = renderTargets[renderPass.hRenderTarget];
    CmdSetViewport(hCmd, 0, 0, renderTarget.desc.width, renderTarget.desc.height);
}

void CmdSetScissor(Handle<CommandBuffer> hCmd, i32 offsetX, i32 offsetY, i32 width, i32 height)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    VkRect2D rect = {};
    rect.offset.x = offsetX;
    rect.offset.y = offsetY;
    rect.extent.width = width;
    rect.extent.height = height;
    vkCmdSetScissor(cmd.vkHandle, 0, 1, &rect);
}

void CmdSetScissor(Handle<CommandBuffer> hCmd, Handle<RenderPass> hRenderPass)
{
    //TODO(caio): Should this use RenderPass or RenderTarget as arg?
    ASSERT(hRenderPass.IsValid());
    RenderPass& renderPass = renderPasses[hRenderPass];
    RenderTarget& renderTarget = renderTargets[renderPass.hRenderTarget];
    CmdSetScissor(hCmd, 0, 0, renderTarget.desc.width, renderTarget.desc.height);
}

void CmdBindVertexBuffer(Handle<CommandBuffer> hCmd, Handle<Buffer> hVB)
{
    ASSERT(hCmd.IsValid() && hVB.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Buffer& vb = buffers[hVB];
    ASSERT(vb.type == BUFFER_TYPE_VERTEX);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd.vkHandle, 0, 1, &vb.vkHandle, &offset);
}

void CmdBindIndexBuffer(Handle<CommandBuffer> hCmd, Handle<Buffer> hIB)
{
    ASSERT(hCmd.IsValid() && hIB.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Buffer& ib = buffers[hIB];
    ASSERT(ib.type == BUFFER_TYPE_INDEX);

    vkCmdBindIndexBuffer(cmd.vkHandle, ib.vkHandle, 0, VK_INDEX_TYPE_UINT32);
}

void CmdDrawIndexed(Handle<CommandBuffer> hCmd, Handle<Buffer> hIB, i32 instanceCount)
{
    ASSERT(hCmd.IsValid() && hIB.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    Buffer& ib = buffers[hIB];
    ASSERT(ib.type == BUFFER_TYPE_INDEX);

    vkCmdDrawIndexed(cmd.vkHandle, ib.count, instanceCount, 0, 0, 0);
}

void CmdDispatch(Handle<CommandBuffer> hCmd, u32 x, u32 y, u32 z)
{
    ASSERT(hCmd.IsValid());
    ASSERT(x > 0 && y > 0 && z > 0);
    CommandBuffer& cmd = commandBuffers[hCmd];

    vkCmdDispatch(cmd.vkHandle, x, y, z);
}

void BeginFrame(u32 frame)
{
    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;
    
    //// Wait for last occurrence of frame to finish
    //VkFence fence = ctx.vkRenderFences[inFlightFrame];
    //vkWaitForFences(ctx.vkDevice, 1, &fence, VK_TRUE, MAX_U64);

    VkSemaphore presentSemaphore = ctx.vkPresentSemaphores[inFlightFrame];
    VkResult ret = vkAcquireNextImageKHR(ctx.vkDevice, swapChain.vkHandle, MAX_U64, presentSemaphore, VK_NULL_HANDLE, &swapChain.activeImage);
    if(ret == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vkDeviceWaitIdle(ctx.vkDevice);
        ResizeSwapChain(ctx.window, &swapChain);
    }
    else ASSERTVK(ret);

    // // Reset fence to start recording work
    // vkResetFences(ctx.vkDevice, 1, &fence);
}

void EndFrame(u32 frame, Handle<CommandBuffer> hCmd)
{
    ASSERT(hCmd.IsValid());
    CommandBuffer& cmd = commandBuffers[hCmd];
    ASSERT(cmd.state == COMMAND_BUFFER_RECORDED);

    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.vkHandle;

    VkSemaphore presentSemaphore = ctx.vkPresentSemaphores[inFlightFrame];
    VkSemaphore renderSemaphore = ctx.vkRenderSemaphores[inFlightFrame];
    //VkFence renderFence = ctx.vkRenderFences[inFlightFrame];
    
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;

    VkResult ret = vkQueueSubmit(ctx.vkCommandQueue, 1, &submitInfo, cmd.vkFence);
    ASSERTVK(ret);

    cmd.state = COMMAND_BUFFER_PENDING;
}

void Present(u32 frame)
{
    ASSERT(ctx.vkCommandQueue != VK_NULL_HANDLE);
    u32 inFlightFrame = frame % RENDER_CONCURRENT_FRAMES;
    VkSemaphore renderSemaphore = ctx.vkRenderSemaphores[inFlightFrame];

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain.vkHandle;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.pImageIndices = &swapChain.activeImage;
    VkResult ret = vkQueuePresentKHR(ctx.vkCommandQueue, &presentInfo);
    if(ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR || ctx.window->state == WINDOW_RESIZING)
    {
        vkDeviceWaitIdle(ctx.vkDevice);
        ResizeSwapChain(ctx.window, &swapChain);
    }
    else ASSERTVK(ret);
}

};
};
