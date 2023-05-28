#include "engine/render/render.hpp"
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

void Init(Window* window)
{
    ctx = InitContext(window);
}

void Shutdown()
{
    DestroyContext(&ctx);
}

void InitContextCreateInstance(Context* ctx)
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

void InitContextSetupValidation(Context* ctx)
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

void InitContextCreateSurface(Context* ctx, Window* window)
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

void InitContextGetPhysicalDevice(Context* ctx)
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

void InitContextCreateDeviceAndCommandQueue(Context* ctx)
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

void InitContextCreateAllocator(Context* ctx)
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

void InitContextCreateCommandBuffers(Context* ctx)
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

void InitContextCreateSyncPrimitives(Context* ctx)
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
    contextArena = mem::InitArenaAllocator(RENDER_CONTEXT_MEMORY);
    mem::SetContext(&contextArena);

    // Creating vulkan instance
    Context ctx = {};
    InitContextCreateInstance(&ctx);
    InitContextSetupValidation(&ctx);
    InitContextCreateSurface(&ctx, window);
    InitContextGetPhysicalDevice(&ctx);
    InitContextCreateDeviceAndCommandQueue(&ctx);
    InitContextCreateAllocator(&ctx);
    InitContextCreateCommandBuffers(&ctx);
    InitContextCreateSyncPrimitives(&ctx);

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
    mem::DestroyArenaAllocator(&contextArena);

    *ctx = {};
}

};
};
