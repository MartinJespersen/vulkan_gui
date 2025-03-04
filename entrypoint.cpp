// domain: hpp
#include "entrypoint.hpp"

// domain: cpp
#include "base/base.cpp"
#include "ui/ui.cpp"

// profiler
#include "profiler/tracy/Tracy.hpp"
#include "profiler/tracy/TracyVulkan.hpp"

#ifdef PROFILING_ENABLE
BufferImpl(TracyVkCtx);
#endif

VkResult
CreateDebugUtilsMessengerEXT(VkInstance instance,
                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void
DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void
InitContext(Context* context)
{
    context->cpuFreq = EstimateCPUTimerFreq();

    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    glyphAtlas->fontArena = (Arena*)ArenaAlloc(FONT_ARENA_SIZE);

    UI_State* uiState = context->uiState;
    uiState->arena = (Arena*)ArenaAlloc(GIGABYTE(1));
    uiState->widgetCacheSize = 1; // 4096;
    uiState->widgetSlot = PushArray(uiState->arena, UI_WidgetSlot, uiState->widgetCacheSize);
}

void
DeleteContext(Context* context)
{
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    ArenaDealloc(glyphAtlas->fontArena);
}

void
IndexBufferAlloc(VulkanContext* vulkanContext, GlyphAtlas* glyphAtlas)
{
    const u16 indices[] = {0, 1, 2, 2, 3, 0};
    vulkanContext->indices = u16_Buffer_Alloc(vulkanContext->arena, ArrayCount(indices));
    glyphAtlas->indices = u16_Buffer_Alloc(glyphAtlas->fontArena, ArrayCount(indices));

    for (u32 i = 0; i < vulkanContext->indices.size; i++)
    {
        vulkanContext->indices.data[i] = indices[i];
        glyphAtlas->indices.data[i] = indices[i];
    }
}

void
VulkanInit(Context* context)
{
    ArenaTemp scratchArena = ArenaScratchBegin();

    VulkanContext* vulkanContext = context->vulkanContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;
    vulkanContext->arena = ArenaAlloc(GIGABYTE(4));

    IndexBufferAlloc(vulkanContext, glyphAtlas);
    createInstance(vulkanContext);
    setupDebugMessenger(vulkanContext);
    createSurface(vulkanContext);
    pickPhysicalDevice(vulkanContext);
    createLogicalDevice(vulkanContext);
    SwapChainInfo swapChainInfo = SwapChainCreate(scratchArena.arena, vulkanContext);
    u32 swapChainImageCount = SwapChainImageCountGet(vulkanContext);
    vulkanContext->swapChainImages =
        VkImage_Buffer_Alloc(vulkanContext->arena, (u64)swapChainImageCount);
    vulkanContext->swapChainImageViews =
        VkImageView_Buffer_Alloc(vulkanContext->arena, (u64)swapChainImageCount);
    vulkanContext->swapChainFramebuffers =
        VkFramebuffer_Buffer_Alloc(vulkanContext->arena, (u64)swapChainImageCount);

    SwapChainImagesCreate(vulkanContext, swapChainInfo, swapChainImageCount);
    SwapChainImageViewsCreate(vulkanContext);
    createCommandPool(vulkanContext);

    vulkanContext->boxRenderPass = createRenderPass(
        vulkanContext->device, vulkanContext->swapChainImageFormat, vulkanContext->msaaSamples,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vulkanContext->fontRenderPass =
        createRenderPass(vulkanContext->device, vulkanContext->swapChainImageFormat,
                         vulkanContext->msaaSamples, VK_ATTACHMENT_LOAD_OP_LOAD,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vulkanContext->resolutionInfo.offset = 0;
    vulkanContext->resolutionInfo.size = sizeof(float) * 2;

    createGraphicsPipeline(&boxContext->pipelineLayout, &boxContext->graphicsPipeline,
                           vulkanContext->device, vulkanContext->swapChainExtent,
                           vulkanContext->fontRenderPass, VK_NULL_HANDLE,
                           vulkanContext->msaaSamples, Vulkan_BoxInstance::getBindingDescription(),
                           Vulkan_BoxInstance::getAttributeDescriptions(vulkanContext->arena),
                           vulkanContext->resolutionInfo, "shaders/vert.spv", "shaders/frag.spv",
                           VK_SHADER_STAGE_FRAGMENT_BIT);

    vulkanContext->colorImageView = createColorResources(
        vulkanContext->physicalDevice, vulkanContext->device, vulkanContext->swapChainImageFormat,
        vulkanContext->swapChainExtent, vulkanContext->msaaSamples, vulkanContext->colorImage,
        vulkanContext->colorImageMemory);
    createFramebuffers(vulkanContext->swapChainFramebuffers, vulkanContext->device,
                       vulkanContext->colorImageView, vulkanContext->fontRenderPass,
                       vulkanContext->swapChainExtent, vulkanContext->swapChainImageViews);
    BoxIndexBufferCreate(context->boxContext, vulkanContext->physicalDevice, vulkanContext->device,
                         vulkanContext->commandPool, vulkanContext->graphicsQueue,
                         vulkanContext->indices);

    createCommandBuffers(context);
    createSyncObjects(vulkanContext);

    ArenaTempEnd(scratchArena);
}

void
cleanup(Context* context)
{
    VulkanContext* vulkanContext = context->vulkanContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;

    vkDeviceWaitIdle(vulkanContext->device);

    if (vulkanContext->enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(vulkanContext->instance, vulkanContext->debugMessenger,
                                      nullptr);
    }
    cleanupSwapChain(vulkanContext);

    cleanupFontResources(glyphAtlas, vulkanContext->device);
    BoxCleanup(boxContext, vulkanContext->device);

    vkDestroyRenderPass(vulkanContext->device, vulkanContext->boxRenderPass, nullptr);
    vkDestroyRenderPass(vulkanContext->device, vulkanContext->fontRenderPass, nullptr);

#ifdef PROFILING_ENABLE
    for (u32 i = 0; i < context->profilingContext->tracyContexts.size; i++)
    {
        TracyVkDestroy(context->profilingContext->tracyContexts.data[i]);
    }
#endif
    vkDestroyCommandPool(vulkanContext->device, vulkanContext->commandPool, nullptr);

    vkDestroySurfaceKHR(vulkanContext->instance, vulkanContext->surface, nullptr);

    for (u32 i = 0; i < (u32)vulkanContext->MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(vulkanContext->device, vulkanContext->renderFinishedSemaphores.data[i],
                           nullptr);
        vkDestroySemaphore(vulkanContext->device, vulkanContext->imageAvailableSemaphores.data[i],
                           nullptr);
        vkDestroyFence(vulkanContext->device, vulkanContext->inFlightFences.data[i], nullptr);
    }

    vkDestroyDevice(vulkanContext->device, nullptr);
    vkDestroyInstance(vulkanContext->instance, nullptr);

    glfwDestroyWindow(vulkanContext->window);

    glfwTerminate();
}

void
cleanupColorResources(VulkanContext* vulkanContext)
{
    vkDestroyImageView(vulkanContext->device, vulkanContext->colorImageView, nullptr);
    vkDestroyImage(vulkanContext->device, vulkanContext->colorImage, nullptr);
    vkFreeMemory(vulkanContext->device, vulkanContext->colorImageMemory, nullptr);
}

void
cleanupSwapChain(VulkanContext* vulkanContext)
{
    cleanupColorResources(vulkanContext);

    for (size_t i = 0; i < vulkanContext->swapChainFramebuffers.size; i++)
    {
        vkDestroyFramebuffer(vulkanContext->device, vulkanContext->swapChainFramebuffers.data[i],
                             nullptr);
    }

    for (size_t i = 0; i < vulkanContext->swapChainImageViews.size; i++)
    {
        vkDestroyImageView(vulkanContext->device, vulkanContext->swapChainImageViews.data[i],
                           nullptr);
    }

    vkDestroySwapchainKHR(vulkanContext->device, vulkanContext->swapChain, nullptr);
}

void
createSyncObjects(VulkanContext* vulkanContext)
{
    vulkanContext->imageAvailableSemaphores =
        VkSemaphore_Buffer_Alloc(vulkanContext->arena, vulkanContext->MAX_FRAMES_IN_FLIGHT);
    vulkanContext->renderFinishedSemaphores =
        VkSemaphore_Buffer_Alloc(vulkanContext->arena, vulkanContext->MAX_FRAMES_IN_FLIGHT);
    vulkanContext->inFlightFences =
        VkFence_Buffer_Alloc(vulkanContext->arena, vulkanContext->MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < (u32)vulkanContext->MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vulkanContext->device, &semaphoreInfo, nullptr,
                              &vulkanContext->imageAvailableSemaphores.data[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanContext->device, &semaphoreInfo, nullptr,
                              &vulkanContext->renderFinishedSemaphores.data[i]) != VK_SUCCESS ||
            vkCreateFence(vulkanContext->device, &fenceInfo, nullptr,
                          &vulkanContext->inFlightFences.data[i]) != VK_SUCCESS)
        {
            exitWithError("failed to create synchronization objects for a frame!");
        }
    }
}

void
createCommandBuffers(Context* context)
{
    VulkanContext* vulkanContext = context->vulkanContext;

    vulkanContext->commandBuffers = VkCommandBuffer_Buffer_Alloc(
        vulkanContext->arena, vulkanContext->swapChainFramebuffers.size);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanContext->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)vulkanContext->commandBuffers.size;

    if (vkAllocateCommandBuffers(vulkanContext->device, &allocInfo,
                                 vulkanContext->commandBuffers.data) != VK_SUCCESS)
    {
        exitWithError("failed to allocate command buffers!");
    }

#ifdef PROFILING_ENABLE
    context->profilingContext->tracyContexts =
        TracyVkCtx_Buffer_Alloc(vulkanContext->arena, vulkanContext->swapChainFramebuffers.size);
    for (u32 i = 0; i < vulkanContext->commandBuffers.size; i++)
    {
        context->profilingContext->tracyContexts.data[i] =
            TracyVkContext(vulkanContext->physicalDevice, vulkanContext->device,
                           vulkanContext->graphicsQueue, vulkanContext->commandBuffers.data[i]);
    }
#endif
}

void
createCommandPool(VulkanContext* vulkanContext)
{
    QueueFamilyIndices queueFamilyIndices = vulkanContext->queueFamilyIndices;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;

    if (vkCreateCommandPool(vulkanContext->device, &poolInfo, nullptr,
                            &vulkanContext->commandPool) != VK_SUCCESS)
    {
        exitWithError("failed to create command pool!");
    }
}

void
SwapChainImageViewsCreate(VulkanContext* vulkanContext)
{
    for (uint32_t i = 0; i < vulkanContext->swapChainImages.size; i++)
    {
        vulkanContext->swapChainImageViews.data[i] =
            createImageView(vulkanContext->device, vulkanContext->swapChainImages.data[i],
                            vulkanContext->swapChainImageFormat);
    }
}

void
SwapChainImagesCreate(VulkanContext* vulkanContext, SwapChainInfo swapChainInfo, u32 imageCount)
{
    vkGetSwapchainImagesKHR(vulkanContext->device, vulkanContext->swapChain, &imageCount,
                            vulkanContext->swapChainImages.data);

    vulkanContext->swapChainImageFormat = swapChainInfo.surfaceFormat.format;
    vulkanContext->swapChainExtent = swapChainInfo.extent;
}

u32
SwapChainImageCountGet(VulkanContext* vulkanContext)
{
    u32 imageCount = {0};
    vkGetSwapchainImagesKHR(vulkanContext->device, vulkanContext->swapChain, &imageCount, nullptr);
    return imageCount;
}

SwapChainInfo
SwapChainCreate(Arena* arena, VulkanContext* vulkanContext)
{
    SwapChainInfo swapChainInfo = {0};

    swapChainInfo.swapChainSupport =
        querySwapChainSupport(arena, vulkanContext, vulkanContext->physicalDevice);

    swapChainInfo.surfaceFormat = chooseSwapSurfaceFormat(swapChainInfo.swapChainSupport.formats);
    swapChainInfo.presentMode = chooseSwapPresentMode(swapChainInfo.swapChainSupport.presentModes);
    swapChainInfo.extent =
        chooseSwapExtent(vulkanContext, swapChainInfo.swapChainSupport.capabilities);

    u32 imageCount = swapChainInfo.swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainInfo.swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainInfo.swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainInfo.swapChainSupport.capabilities.maxImageCount;
    }

    QueueFamilyIndices queueFamilyIndices = vulkanContext->queueFamilyIndices;
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkanContext->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainInfo.surfaceFormat.format;
    createInfo.imageColorSpace = swapChainInfo.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainInfo.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queueFamilyIndices.graphicsFamilyIndex != queueFamilyIndices.presentFamilyIndex)
    {
        u32 queueFamilyIndices[] = {vulkanContext->queueFamilyIndices.graphicsFamilyIndex,
                                    vulkanContext->queueFamilyIndices.presentFamilyIndex};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainInfo.swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapChainInfo.presentMode;
    createInfo.clipped = VK_TRUE;
    // It is possible to specify the old swap chain to be replaced by a new one
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vulkanContext->device, &createInfo, nullptr,
                             &vulkanContext->swapChain) != VK_SUCCESS)
    {
        exitWithError("failed to create swap chain!");
    }

    return swapChainInfo;
}

void
createSurface(VulkanContext* vulkanContext)
{
    if (glfwCreateWindowSurface(vulkanContext->instance, vulkanContext->window, nullptr,
                                &vulkanContext->surface) != VK_SUCCESS)
    {
        exitWithError("failed to create window surface!");
    }
}

void
createInstance(VulkanContext* vulkanContext)
{
    ArenaTemp scratchArena = ArenaScratchBegin();
    if (vulkanContext->enableValidationLayers && !checkValidationLayerSupport(vulkanContext))
    {
        exitWithError("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    String8_Buffer extensions = getRequiredExtensions(vulkanContext);

    createInfo.enabledExtensionCount = (u32)extensions.size;
    createInfo.ppEnabledExtensionNames =
        StrArrFromStr8Buffer(scratchArena.arena, extensions.data, extensions.size);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (vulkanContext->enableValidationLayers)
    {
        createInfo.enabledLayerCount = (u32)ArrayCount(vulkanContext->validationLayers);
        createInfo.ppEnabledLayerNames = vulkanContext->validationLayers;

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &vulkanContext->instance) != VK_SUCCESS)
    {
        exitWithError("failed to create instance!");
    }

    ArenaTempEnd(scratchArena);
}

void
createLogicalDevice(VulkanContext* vulkanContext)
{
    QueueFamilyIndices queueFamilyIndicies = vulkanContext->queueFamilyIndices;

    u32 uniqueQueueFamiliesCount = 1;
    if (queueFamilyIndicies.graphicsFamilyIndex != queueFamilyIndicies.presentFamilyIndex)
    {
        uniqueQueueFamiliesCount++;
    }

    u32 uniqueQueueFamilies[] = {queueFamilyIndicies.graphicsFamilyIndex,
                                 queueFamilyIndicies.presentFamilyIndex};

    VkDeviceQueueCreateInfo queueCreateInfos[uniqueQueueFamiliesCount];
    float queuePriority = 1.0f;
    for (u32 i = 0; i < uniqueQueueFamiliesCount; i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos;

    createInfo.queueCreateInfoCount = uniqueQueueFamiliesCount;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = (u32)ArrayCount(vulkanContext->deviceExtensions);
    createInfo.ppEnabledExtensionNames = vulkanContext->deviceExtensions;

    // NOTE: This if statement is no longer necessary on newer versions
    if (vulkanContext->enableValidationLayers)
    {
        createInfo.enabledLayerCount = (u32)ArrayCount(vulkanContext->validationLayers);
        createInfo.ppEnabledLayerNames = vulkanContext->validationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(vulkanContext->physicalDevice, &createInfo, nullptr,
                       &vulkanContext->device) != VK_SUCCESS)
    {
        exitWithError("failed to create logical device!");
    }

    vkGetDeviceQueue(vulkanContext->device, queueFamilyIndicies.graphicsFamilyIndex, 0,
                     &vulkanContext->graphicsQueue);
    vkGetDeviceQueue(vulkanContext->device, queueFamilyIndicies.presentFamilyIndex, 0,
                     &vulkanContext->presentQueue);
}

void
pickPhysicalDevice(VulkanContext* vulkanContext)
{
    ArenaTemp scratchArena = ArenaScratchBegin();

    vulkanContext->physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanContext->instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        exitWithError("failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* devices = PushArray(scratchArena.arena, VkPhysicalDevice, deviceCount);
    vkEnumeratePhysicalDevices(vulkanContext->instance, &deviceCount, devices);

    for (u32 i = 0; i < deviceCount; i++)
    {
        QueueFamilyIndexBits familyIndexBits = QueueFamiliesFind(vulkanContext, devices[i]);
        if (isDeviceSuitable(vulkanContext, devices[i], familyIndexBits))
        {
            vulkanContext->physicalDevice = devices[i];
            vulkanContext->msaaSamples = getMaxUsableSampleCount(devices[i]);
            vulkanContext->queueFamilyIndices = QueueFamilyIndicesFromBitFields(familyIndexBits);
            break;
        }
    }

    if (vulkanContext->physicalDevice == VK_NULL_HANDLE)
    {
        exitWithError("failed to find a suitable GPU!");
    }

    ArenaTempEnd(scratchArena);
}

bool
isDeviceSuitable(VulkanContext* vulkanContext, VkPhysicalDevice device,
                 QueueFamilyIndexBits indexBits)
{
    // NOTE: This is where you would implement your own checks to see if the
    // device is suitable for your needs

    bool extensionsSupported = checkDeviceExtensionSupport(vulkanContext, device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainDetails =
            querySwapChainSupport(vulkanContext->arena, vulkanContext, device);
        swapChainAdequate = swapChainDetails.formats.size && swapChainDetails.presentModes.size;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return QueueFamilyIsComplete(indexBits) && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}

bool
checkValidationLayerSupport(VulkanContext* vulkanContext)
{
    ArenaTemp scratchArena = ArenaScratchBegin();
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties* availableLayers =
        PushArray(scratchArena.arena, VkLayerProperties, layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool layerFound = false;
    for (const char* layerName : vulkanContext->validationLayers)
    {
        layerFound = false;
        for (u32 i = 0; i < layerCount; i++)
        {
            if (strcmp(layerName, availableLayers[i].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            break;
        }
    }

    ArenaTempEnd(scratchArena);
    return layerFound;
}

String8_Buffer
getRequiredExtensions(VulkanContext* vulkanContext)
{
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    u32 extensionCount = glfwExtensionCount;
    if (vulkanContext->enableValidationLayers)
    {
        extensionCount++;
    }

    String8_Buffer extensions = String8_Buffer_Alloc(vulkanContext->arena, extensionCount);

    for (u32 i = 0; i < glfwExtensionCount; i++)
    {
        extensions.data[i] = Str8(vulkanContext->arena, glfwExtensions[i]);
    }

    if (vulkanContext->enableValidationLayers)
    {
        extensions.data[glfwExtensionCount] =
            Str8(vulkanContext->arena, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    (void)pUserData;
    (void)messageType;
    (void)messageSeverity;
    printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

void
setupDebugMessenger(VulkanContext* vulkanContext)
{
    if (!vulkanContext->enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(vulkanContext->instance, &createInfo, nullptr,
                                     &vulkanContext->debugMessenger) != VK_SUCCESS)
    {
        exitWithError("failed to set up debug messenger!");
    }
}

void
populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

bool
checkDeviceExtensionSupport(VulkanContext* vulkanContext, VkPhysicalDevice device)
{
    ArenaTemp scratchArena = ArenaScratchBegin();
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties* availableExtensions =
        PushArrayZero(scratchArena.arena, VkExtensionProperties, extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);
    const u64 numberOfRequiredExtenstions = ArrayCount(vulkanContext->deviceExtensions);
    u64 numberOfRequiredExtenstionsLeft = numberOfRequiredExtenstions;
    for (u32 i = 0; i < extensionCount; i++)
    {
        for (u32 j = 0; j < numberOfRequiredExtenstions; j++)
        {
            if (CStrEqual(vulkanContext->deviceExtensions[j], availableExtensions[i].extensionName))
            {
                numberOfRequiredExtenstionsLeft--;
                break;
            }
        }
    }

    return numberOfRequiredExtenstionsLeft == 0;
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(VkSurfaceFormatKHR_Buffer availableFormats)
{
    for (u32 i = 0; i < availableFormats.size; i++)
    {
        if (availableFormats.data[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats.data[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats.data[i];
        }
    }
    return availableFormats.data[0];
}

VkPresentModeKHR
chooseSwapPresentMode(VkPresentModeKHR_Buffer availablePresentModes)
{
    for (u32 i = 0; i < availablePresentModes.size; i++)
    {
        if (availablePresentModes.data[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes.data[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
chooseSwapExtent(VulkanContext* vulkanContext, const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(vulkanContext->window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = Clamp<u32>(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = Clamp<u32>(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkSampleCountFlagBits
getMaxUsableSampleCount(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

void
recordCommandBuffer(Context* context, u32 imageIndex, u32 currentFrame)
{
    ZoneScoped;
    ArenaTemp frameArena = ArenaScratchBegin();
    Arena* arena = frameArena.arena;

    VulkanContext* vulkanContext = context->vulkanContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;
    ProfilingContext* profilingContext = context->profilingContext;
    (void)profilingContext;

    FontFrameReset(arena, glyphAtlas);
    BoxFrameReset(arena, boxContext);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    // add buttom that is only a rectangle and text at the moment
    {
        ZoneScopedN("Create Buttom");
        // temporary way of choosing font
        F32Vec4 positions = {0.2f, 0.2f, 0.3f, 0.3f};
        F32Vec4 color = {0.0f, 0.8f, 0.8f, 0.1f};
        UI_WidgetFlags flags = UI_WidgetFlag_Clickable;
        for (u32 btn_i = 0; btn_i < 8; btn_i++)
        {
            f32 width = positions.point.p1.x - positions.point.p0.x;
            positions.point.p0.x += width;
            positions.point.p1.x += width;
            color.axis.x = width;
            String8 name = Str8(arena, "test_name %u", btn_i);
            String8 text = Str8(arena, "%u", btn_i);
            AddButton(name, context->uiState, glyphAtlas, arena, boxContext,
                      vulkanContext->swapChainExtent, color, text, 1.0f, 10.0f, 5.0f, context->io,
                      positions, flags, 30);
        }
    }
    // recording rectangles
    {
        ZoneScopedN("Rectangle CPU");
        boxContext->numInstances =
            InstanceBufferFromBoxes(boxContext->boxList, boxContext->boxInstances);
        InstanceBufferFillFromBoxes(boxContext, vulkanContext->physicalDevice,
                                    vulkanContext->device);
    }
    // recording text
    {
        ZoneScopedN("Text CPU");

        glyphAtlas->numInstances =
            InstanceBufferFromFontBuffers(glyphAtlas->glyphInstanceBuffer, glyphAtlas->fontLL);
        mapGlyphInstancesToBuffer(glyphAtlas, vulkanContext->physicalDevice, vulkanContext->device,
                                  vulkanContext->graphicsQueue);
    }

    if (vkBeginCommandBuffer(vulkanContext->commandBuffers.data[currentFrame], &beginInfo) !=
        VK_SUCCESS)
    {
        exitWithError("failed to begin recording command buffer!");
    }

    TracyVkCollect(profilingContext->tracyContexts.data[currentFrame],
                   vulkanContext->commandBuffers.data[currentFrame]);

    {
        TracyVkZoneC(profilingContext->tracyContexts.data[currentFrame],
                     vulkanContext->commandBuffers.data[currentFrame], "Rectangles GPU", 0xff0000);
        BoxRenderPassBegin(boxContext, vulkanContext, imageIndex, currentFrame);
    }
    {
        TracyVkZoneC(profilingContext->tracyContexts.data[currentFrame],
                     vulkanContext->commandBuffers.data[currentFrame], "Text GPU", 0x00FF00);
        beginGlyphAtlasRenderPass(glyphAtlas, vulkanContext, imageIndex, currentFrame);
    }

    if (vkEndCommandBuffer(vulkanContext->commandBuffers.data[currentFrame]) != VK_SUCCESS)
    {
        exitWithError("failed to record command buffer!");
    }
    ArenaTempEnd(frameArena);
}

inline f64
CalculateFrameRate(u64* prevFrameTickCount, u64 cpuFreq)
{
    u64 tick = ReadCPUTimer();
    u64 tickDelta = tick - *prevFrameTickCount;
    *prevFrameTickCount = tick;

    return (f64)cpuFreq / (f64)tickDelta;
}

void
drawFrame(Context* context)
{
    VulkanContext* vulkanContext = context->vulkanContext;

    ZoneScoped;
    {
        ZoneScopedN("Wait for frame");
        vkWaitForFences(vulkanContext->device, 1,
                        &vulkanContext->inFlightFences.data[vulkanContext->currentFrame], VK_TRUE,
                        UINT64_MAX);
    }

    // TODO: This is not correct as it only looks at cpu time
    context->frameRate = CalculateFrameRate(&context->frameTickPrev, context->cpuFreq);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        vulkanContext->device, vulkanContext->swapChain, UINT64_MAX,
        vulkanContext->imageAvailableSemaphores.data[vulkanContext->currentFrame], VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapChain(vulkanContext);
        return;
    }
    else if (result != VK_SUCCESS)
    {
        exitWithError("failed to acquire swap chain image!");
    }

    vkResetFences(vulkanContext->device, 1,
                  &vulkanContext->inFlightFences.data[vulkanContext->currentFrame]);
    vkResetCommandBuffer(vulkanContext->commandBuffers.data[vulkanContext->currentFrame], 0);

    recordCommandBuffer(context, imageIndex, vulkanContext->currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {
        vulkanContext->imageAvailableSemaphores.data[vulkanContext->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanContext->commandBuffers.data[vulkanContext->currentFrame];

    VkSemaphore signalSemaphores[] = {
        vulkanContext->renderFinishedSemaphores.data[vulkanContext->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanContext->graphicsQueue, 1, &submitInfo,
                      vulkanContext->inFlightFences.data[vulkanContext->currentFrame]) !=
        VK_SUCCESS)
    {
        exitWithError("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {vulkanContext->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(vulkanContext->presentQueue, &presentInfo);
    // TracyVkCollect(tracyContexts[currentFrame], commandBuffers[currentFrame]);
    FrameMark; // end of frame is assumed to be here
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        vulkanContext->framebufferResized)
    {
        vulkanContext->framebufferResized = 0;
        recreateSwapChain(vulkanContext);
    }
    else if (result != VK_SUCCESS)
    {
        exitWithError("failed to present swap chain image!");
    }

    vulkanContext->currentFrame =
        (vulkanContext->currentFrame + 1) % vulkanContext->MAX_FRAMES_IN_FLIGHT;
}

void
recreateSwapChain(VulkanContext* vulkanContext)
{
    ArenaTemp scratchArena = ArenaScratchBegin();
    int width = 0, height = 0;
    glfwGetFramebufferSize(vulkanContext->window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(vulkanContext->window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vulkanContext->device);

    cleanupSwapChain(vulkanContext);

    SwapChainInfo swapChainInfo = SwapChainCreate(scratchArena.arena, vulkanContext);
    u32 swapChainImageCount = SwapChainImageCountGet(vulkanContext);
    SwapChainImagesCreate(vulkanContext, swapChainInfo, swapChainImageCount);

    SwapChainImageViewsCreate(vulkanContext);
    vulkanContext->colorImageView = createColorResources(
        vulkanContext->physicalDevice, vulkanContext->device, vulkanContext->swapChainImageFormat,
        vulkanContext->swapChainExtent, vulkanContext->msaaSamples, vulkanContext->colorImage,
        vulkanContext->colorImageMemory);
    createFramebuffers(vulkanContext->swapChainFramebuffers, vulkanContext->device,
                       vulkanContext->colorImageView, vulkanContext->fontRenderPass,
                       vulkanContext->swapChainExtent, vulkanContext->swapChainImageViews);
    ArenaTempEnd(scratchArena);
}
