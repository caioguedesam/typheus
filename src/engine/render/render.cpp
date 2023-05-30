#include "engine/render/render.hpp"
#include "engine/core/ds.hpp"
#include "engine/core/memory.hpp"
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

VkFormat formatToVk[] =
{
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_B8G8R8A8_SRGB,
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

void Init(Window* window)
{
    renderHeap = mem::InitHeapAllocator(RENDER_CONTEXT_MEMORY);
    mem::SetContext(&renderHeap);

    ctx = InitContext(window);
    swapChain = InitSwapChain(&ctx, window);

    renderPasses = MakeArray<RenderPass>(RENDER_MAX_RENDER_PASSES);
}

void Shutdown()
{
    mem::SetContext(&renderHeap);

    for(i32 i = 0; i < renderPasses.count; i++)
    {
        DestroyRenderPass(&ctx, &renderPasses[i]);
    }
    DestroyArray(&renderPasses);

    DestroySwapChain(&ctx, &swapChain);
    DestroyContext(&ctx);
    mem::DestroyHeapAllocator(&renderHeap);
}

void InitContext_CreateInstance(Context* ctx)
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

void InitContext_SetupValidation(Context* ctx)
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

void InitContext_CreateSurface(Context* ctx, Window* window)
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

void InitContext_GetPhysicalDevice(Context* ctx)
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

void InitContext_CreateDeviceAndCommandQueue(Context* ctx)
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

void InitContext_CreateAllocator(Context* ctx)
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

void InitContext_CreateCommandBuffers(Context* ctx)
{
    ASSERT(ctx);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx->vkCommandQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkCommandPool commandPool;
    VkResult ret = vkCreateCommandPool(ctx->vkDevice, &poolInfo, NULL, &commandPool);
    ASSERTVK(ret);
    VkCommandPool singleTimeCommandPool;
    ret = vkCreateCommandPool(ctx->vkDevice, &poolInfo, NULL, &singleTimeCommandPool);
    ASSERTVK(ret);

    ctx->vkCommandPool = commandPool;
    ctx->vkSingleTimeCommandPool = singleTimeCommandPool;

    // One command buffer for each concurrent frame, and an additional one
    // for single time commands like buffer copies
    ctx->vkCommandBuffers = MakeArray<VkCommandBuffer>(RENDER_CONCURRENT_FRAMES);
    VkCommandBufferAllocateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferInfo.commandBufferCount = 1;
    bufferInfo.commandPool = commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VkCommandBuffer commandBuffer;
    for(i32 i = 0; i < RENDER_CONCURRENT_FRAMES; i++)
    {
        ret = vkAllocateCommandBuffers(ctx->vkDevice, &bufferInfo, &commandBuffer);
        ASSERTVK(ret);
        ctx->vkCommandBuffers.Push(commandBuffer);
    }
    bufferInfo.commandPool = singleTimeCommandPool;
    ret = vkAllocateCommandBuffers(ctx->vkDevice, &bufferInfo, &commandBuffer);
    ASSERTVK(ret);
    ctx->vkSingleTimeCommandBuffer = commandBuffer;
}

void InitContext_CreateSyncPrimitives(Context* ctx)
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
    ctx->vkSingleTimeCommandFence = fence;
}

Context InitContext(Window *window)
{
    Context ctx = {};
    InitContext_CreateInstance(&ctx);
    InitContext_SetupValidation(&ctx);
    InitContext_CreateSurface(&ctx, window);
    InitContext_GetPhysicalDevice(&ctx);
    InitContext_CreateDeviceAndCommandQueue(&ctx);
    InitContext_CreateAllocator(&ctx);
    InitContext_CreateCommandBuffers(&ctx);
    InitContext_CreateSyncPrimitives(&ctx);

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
    vkDestroyFence(ctx->vkDevice, ctx->vkSingleTimeCommandFence, NULL);
    vkDestroyCommandPool(ctx->vkDevice, ctx->vkCommandPool, NULL);
    vkDestroyCommandPool(ctx->vkDevice, ctx->vkSingleTimeCommandPool, NULL);
    vmaDestroyAllocator(ctx->vkAllocator);
    vkDestroyDevice(ctx->vkDevice, NULL);
    vkDestroySurfaceKHR(ctx->vkInstance, ctx->vkSurface, NULL);
#ifdef _DEBUG
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    ASSERT(fn);
    fn(ctx->vkInstance, ctx->vkDebugMessenger, NULL);
#endif
    vkDestroyInstance(ctx->vkInstance, NULL);
    DestroyArray(&ctx->vkCommandBuffers);
    DestroyArray(&ctx->vkRenderSemaphores);
    DestroyArray(&ctx->vkPresentSemaphores);
    DestroyArray(&ctx->vkRenderFences);

    *ctx = {};
}

void InitSwapChain_CreateSwapChain(Context* ctx, SwapChain* swapChain)
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

void InitSwapChain_CreateImages(Context* ctx, SwapChain* swapChain)
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

SwapChain InitSwapChain(Context* ctx, Window* window)
{
    ASSERT(ctx);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && ctx->vkSurface != VK_NULL_HANDLE);
    mem::SetContext(&renderHeap);

    SwapChain result = {};
    InitSwapChain_CreateSwapChain(ctx, &result);
    InitSwapChain_CreateImages(ctx, &result);
    return result;
}

void DestroySwapChain(Context* ctx, SwapChain* swapChain)
{
    ASSERT(ctx && swapChain);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && swapChain->vkHandle != VK_NULL_HANDLE);

    for(i32 i = 0; i < swapChain->vkImageViews.count; i++)
    {
        vkDestroyImageView(ctx->vkDevice, swapChain->vkImageViews[i], NULL);
    }
    DestroyArray(&swapChain->vkImageViews);
    DestroyArray(&swapChain->vkImages);
    vkDestroySwapchainKHR(ctx->vkDevice, swapChain->vkHandle, NULL);

    *swapChain = {};
}

void ResizeSwapChain(Context* ctx, Window* window, SwapChain* swapChain)
{
    // Destroys and recreates swap chain
    ASSERT(ctx && swapChain);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && swapChain->vkHandle != VK_NULL_HANDLE);

    DestroySwapChain(ctx, swapChain);
    *swapChain = InitSwapChain(ctx, window);
}

void InitRenderPass_CreateRenderPass(Context* ctx, RenderPassDesc desc, u32 colorImageCount, Format* colorImageFormats, Format depthImageFormat, RenderPass* renderPass)
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
    renderPass->outputImageFormats = MakeArray<Format>(colorImageCount + 1);
    for(i32 i = 0; i < colorImageCount; i++)
    {
        renderPass->outputImageFormats.Push(colorImageFormats[i]);
    }
    renderPass->outputImageFormats.Push(depthImageFormat);
}

void InitRenderPass_CreateOutputImages(Context* ctx, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    renderPass->vkOutputImages = MakeArray<VkImage>(renderPass->colorImageCount + 1);
    renderPass->vkOutputImageViews = MakeArray<VkImageView>(renderPass->colorImageCount + 1);
    renderPass->vkOutputImageAllocations = MakeArray<VmaAllocation>(renderPass->colorImageCount + 1);

    // Create images for color attachments
    for(i32 i = 0; i < renderPass->colorImageCount; i++)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = renderPass->desc.width;
        imageInfo.extent.height = renderPass->desc.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = formatToVk[renderPass->outputImageFormats[i]];
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;      // TODO(caio): Support multisampling
        imageInfo.flags = 0;
        VmaAllocationCreateInfo allocationInfo = {};
        allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        
        VkImage image;
        VmaAllocation imageAllocation;
        VkResult ret = vmaCreateImage(ctx->vkAllocator, &imageInfo, &allocationInfo, &image, &imageAllocation, NULL);
        ASSERTVK(ret);

        VkImageViewCreateInfo imageViewInfo = {};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = formatToVk[renderPass->outputImageFormats[i]];
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        ret = vkCreateImageView(ctx->vkDevice, &imageViewInfo, NULL, &imageView);
        ASSERTVK(ret);

        renderPass->vkOutputImages.Push(image);
        renderPass->vkOutputImageViews.Push(imageView);
        renderPass->vkOutputImageAllocations.Push(imageAllocation);
    }

    // Create image for depth attachment
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = renderPass->desc.width;
    imageInfo.extent.height = renderPass->desc.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = formatToVk[renderPass->outputImageFormats[renderPass->colorImageCount]];
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;     // Maybe expose this later
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;      // TODO(caio): Support multisampling
    imageInfo.flags = 0;
    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    
    VkImage image;
    VmaAllocation imageAllocation;
    VkResult ret = vmaCreateImage(ctx->vkAllocator, &imageInfo, &allocationInfo, &image, &imageAllocation, NULL);
    ASSERTVK(ret);

    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = formatToVk[renderPass->outputImageFormats[renderPass->colorImageCount]];
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    ret = vkCreateImageView(ctx->vkDevice, &imageViewInfo, NULL, &imageView);
    ASSERTVK(ret);

    renderPass->vkOutputImages.Push(image);
    renderPass->vkOutputImageViews.Push(imageView);
    renderPass->vkOutputImageAllocations.Push(imageAllocation);
}

void InitRenderPass_CreateFramebuffer(Context* ctx, RenderPass* renderPass)
{
    ASSERT(ctx && renderPass);
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass->vkHandle;
    framebufferInfo.width = renderPass->desc.width;
    framebufferInfo.height = renderPass->desc.height;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = renderPass->colorImageCount + 1;
    framebufferInfo.pAttachments = renderPass->vkOutputImageViews.data;

    VkFramebuffer framebuffer;
    VkResult ret = vkCreateFramebuffer(ctx->vkDevice, &framebufferInfo, NULL, &framebuffer);
    ASSERTVK(ret);

    renderPass->vkFramebuffer = framebuffer;
}

Handle<RenderPass> InitRenderPass(RenderPassDesc desc, u32 colorImageCount, Format* colorImageFormats, Format depthImageFormat)
{
    ASSERT(colorImageFormats);
    mem::SetContext(&renderHeap);

    RenderPass result = {};

    InitRenderPass_CreateRenderPass(&ctx, desc, colorImageCount, colorImageFormats, depthImageFormat, &result);
    InitRenderPass_CreateOutputImages(&ctx, &result);
    InitRenderPass_CreateFramebuffer(&ctx, &result);

    renderPasses.Push(result);
    return { (u32)renderPasses.count - 1 };
}

void DestroyRenderPass(Context *ctx, RenderPass *renderPass)
{
    ASSERT(ctx && renderPass);
    ASSERT(ctx->vkDevice != VK_NULL_HANDLE && renderPass->vkHandle != VK_NULL_HANDLE);
    for(i32 i = 0; i < renderPass->colorImageCount + 1; i++)
    {
        vkDestroyImageView(ctx->vkDevice, renderPass->vkOutputImageViews[i], NULL);
        vmaDestroyImage(ctx->vkAllocator, renderPass->vkOutputImages[i], renderPass->vkOutputImageAllocations[i]);
    }
    vkDestroyFramebuffer(ctx->vkDevice, renderPass->vkFramebuffer, NULL);
    vkDestroyRenderPass(ctx->vkDevice, renderPass->vkHandle, NULL);

    DestroyArray(&renderPass->outputImageFormats);
    DestroyArray(&renderPass->vkOutputImages);
    DestroyArray(&renderPass->vkOutputImageViews);
    DestroyArray(&renderPass->vkOutputImageAllocations);

    *renderPass = {};
}

};
};
