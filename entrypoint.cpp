#include <GL/gl.h>
#include <ft2build.h>
#include <set>
#include <vulkan/vulkan_core.h>
#include FT_FREETYPE_H
#include "stb_image_wrapper.hpp"

#include <cstdio>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm> // Necessary for std::clamp

#include <cstdint> // Necessary for uint32_t
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits> // Necessary for std::numeric_limits

#include <stdexcept>

// profiler
#include "profiler/tracy/Tracy.hpp"
#include "profiler/tracy/TracyVulkan.hpp"

// domain: hpp
#include "entrypoint.hpp"

// domain: cpp
#include "base/base.cpp"
#include "box.cpp"
#include "button.cpp"
#include "fonts.cpp"
#include "ui/ui.cpp"
#include "vulkan_helpers.cpp"

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

ThreadCtx
InitContext(Context* context)
{
    context->cpuFreq = EstimateCPUTimerFreq();

    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    glyphAtlas->fontArena = (Arena*)AllocArena(FONT_ARENA_SIZE);
    glyphAtlas->fontList = &g_font;

    UI_State* uiState = context->uiState;
    uiState->arena = (Arena*)AllocArena(GIGABYTE(1));
    uiState->widgetCacheSize = 1; // 4096;
    uiState->widgetSlot = PushArray(uiState->arena, UI_WidgetSlot, uiState->widgetCacheSize);
    uiState->root = &g_UI_Widget;

    return ThreadContextAlloc();
}

void
DeleteContext(Context* context)
{
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    DeallocArena(glyphAtlas->fontArena);

    ThreadContextDealloc(&(context->threadCtx));
}

void
initVulkan(Context* context)
{
    VulkanContext* vulkan_context = context->vulkanContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;

    createInstance(*context);
    setupDebugMessenger(*context);
    createSurface(*context);
    pickPhysicalDevice(*context);
    createLogicalDevice(*context);
    createSwapChain(*context);
    createImageViews(*context);
    createCommandPool(*context);

    vulkan_context->boxRenderPass = createRenderPass(
        vulkan_context->device, vulkan_context->swapChainImageFormat, vulkan_context->msaaSamples,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vulkan_context->fontRenderPass =
        createRenderPass(vulkan_context->device, vulkan_context->swapChainImageFormat,
                         vulkan_context->msaaSamples, VK_ATTACHMENT_LOAD_OP_LOAD,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vulkan_context->resolutionInfo.offset = 0;
    vulkan_context->resolutionInfo.size = sizeof(float) * 2;

    auto rectangleObjects = createGraphicsPipeline(
        vulkan_context->device, vulkan_context->swapChainExtent, vulkan_context->fontRenderPass,
        VK_NULL_HANDLE, vulkan_context->msaaSamples, Vulkan_BoxInstance::getBindingDescription(),
        Vulkan_BoxInstance::getAttributeDescriptions(), vulkan_context->resolutionInfo,
        "shaders/vert.spv", "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    boxContext->pipelineLayout = std::get<0>(rectangleObjects);
    boxContext->graphicsPipeline = std::get<1>(rectangleObjects);

    vulkan_context->colorImageView = createColorResources(
        vulkan_context->physicalDevice, vulkan_context->device,
        vulkan_context->swapChainImageFormat, vulkan_context->swapChainExtent,
        vulkan_context->msaaSamples, vulkan_context->colorImage, vulkan_context->colorImageMemory);
    vulkan_context->swapChainFramebuffers = createFramebuffers(
        vulkan_context->device, vulkan_context->colorImageView, vulkan_context->fontRenderPass,
        vulkan_context->swapChainExtent, vulkan_context->swapChainImageViews);
    BoxIndexBufferCreate(context->boxContext, vulkan_context->physicalDevice,
                         vulkan_context->device, vulkan_context->commandPool,
                         vulkan_context->graphicsQueue, vulkan_context->indices);

    {
        u32 test_fontSizes[] = {30, 50};
        u32 numFonts = sizeof(test_fontSizes) / sizeof(u32);

        u32 totalDescSet = vulkan_context->MAX_FRAMES_IN_FLIGHT * numFonts;
        createFontDescriptorSetLayout(vulkan_context->device, glyphAtlas->descriptorSetLayout);
        createFontDescriptorPool(vulkan_context->device, totalDescSet, glyphAtlas->descriptorPool);

        for (u32 fontIndex = 0; fontIndex < numFonts; fontIndex++)
        {
            Font* font = PushStruct(glyphAtlas->fontArena, Font);
            StackPush(glyphAtlas->fontList, font);
            FontInit(glyphAtlas->fontArena, font, test_fontSizes[fontIndex], MAX_GLYPHS);

            createGlyphAtlasImage(context->threadCtx.scratchArena, font,
                                  vulkan_context->physicalDevice, vulkan_context->device,
                                  vulkan_context->commandPool, vulkan_context->graphicsQueue);
            createGlyphAtlasImageView(font, vulkan_context->device);
            createGlyphAtlasTextureSampler(font, vulkan_context->physicalDevice,
                                           vulkan_context->device);
            font->descriptorSets = ArrayAlloc<VkDescriptorSet>(
                glyphAtlas->fontArena, vulkan_context->MAX_FRAMES_IN_FLIGHT);
            createFontDescriptorSets(font, glyphAtlas->descriptorPool,
                                     glyphAtlas->descriptorSetLayout, vulkan_context->device,
                                     vulkan_context->MAX_FRAMES_IN_FLIGHT, font->descriptorSets);
        }

        auto glyphAtlasGraphicsObjects = createGraphicsPipeline(
            vulkan_context->device, vulkan_context->swapChainExtent, vulkan_context->fontRenderPass,
            glyphAtlas->descriptorSetLayout, vulkan_context->msaaSamples,
            Vulkan_GlyphInstance::getBindingDescription(),
            Vulkan_GlyphInstance::getAttributeDescriptions(), vulkan_context->resolutionInfo,
            "shaders/text_vert.spv", "shaders/text_frag.spv", VK_SHADER_STAGE_VERTEX_BIT);
        glyphAtlas->pipelineLayout = std::get<0>(glyphAtlasGraphicsObjects);
        glyphAtlas->graphicsPipeline = std::get<1>(glyphAtlasGraphicsObjects);
        createGlyphIndexBuffer(glyphAtlas, vulkan_context->physicalDevice, vulkan_context->device);
    }

    createCommandBuffers(*context);
    createSyncObjects(*context);
}

void
cleanup(Context* context)
{
    VulkanContext* vulkanContext = context->vulkanContext;
    ProfilingContext* profilingContext = context->profilingContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;

    vkDeviceWaitIdle(vulkanContext->device);

    if (vulkanContext->enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(vulkanContext->instance, vulkanContext->debugMessenger,
                                      nullptr);
    }
    cleanupSwapChain(*context);

    cleanupFontResources(glyphAtlas, vulkanContext->device);
    BoxCleanup(boxContext, vulkanContext->device);

    vkDestroyRenderPass(vulkanContext->device, vulkanContext->boxRenderPass, nullptr);
    vkDestroyRenderPass(vulkanContext->device, vulkanContext->fontRenderPass, nullptr);

    for (u32 i = 0; i < profilingContext->tracyContexts.size(); i++)
    {
        TracyVkDestroy(profilingContext->tracyContexts[i]);
    }
    vkDestroyCommandPool(vulkanContext->device, vulkanContext->commandPool, nullptr);

    vkDestroySurfaceKHR(vulkanContext->instance, vulkanContext->surface, nullptr);

    for (u32 i = 0; i < (u32)vulkanContext->MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(vulkanContext->device, vulkanContext->renderFinishedSemaphores[i],
                           nullptr);
        vkDestroySemaphore(vulkanContext->device, vulkanContext->imageAvailableSemaphores[i],
                           nullptr);
        vkDestroyFence(vulkanContext->device, vulkanContext->inFlightFences[i], nullptr);
    }

    vkDestroyDevice(vulkanContext->device, nullptr);
    vkDestroyInstance(vulkanContext->instance, nullptr);

    glfwDestroyWindow(vulkanContext->window);

    glfwTerminate();
}

void
cleanupColorResources(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    vkDestroyImageView(vulkanContext->device, vulkanContext->colorImageView, nullptr);
    vkDestroyImage(vulkanContext->device, vulkanContext->colorImage, nullptr);
    vkFreeMemory(vulkanContext->device, vulkanContext->colorImageMemory, nullptr);
}

void
cleanupSwapChain(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    cleanupColorResources(context);

    for (size_t i = 0; i < vulkanContext->swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(vulkanContext->device, vulkanContext->swapChainFramebuffers[i],
                             nullptr);
    }

    for (size_t i = 0; i < vulkanContext->swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(vulkanContext->device, vulkanContext->swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(vulkanContext->device, vulkanContext->swapChain, nullptr);
}

void
createSyncObjects(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;
    vulkanContext->imageAvailableSemaphores.resize(vulkanContext->MAX_FRAMES_IN_FLIGHT);
    vulkanContext->renderFinishedSemaphores.resize(vulkanContext->MAX_FRAMES_IN_FLIGHT);
    vulkanContext->inFlightFences.resize(vulkanContext->MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < (u32)vulkanContext->MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vulkanContext->device, &semaphoreInfo, nullptr,
                              &vulkanContext->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanContext->device, &semaphoreInfo, nullptr,
                              &vulkanContext->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vulkanContext->device, &fenceInfo, nullptr,
                          &vulkanContext->inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void
createCommandBuffers(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;
    ProfilingContext* profilingContext = context.profilingContext;

    vulkanContext->commandBuffers.resize(vulkanContext->swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanContext->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)vulkanContext->commandBuffers.size();

    if (vkAllocateCommandBuffers(vulkanContext->device, &allocInfo,
                                 vulkanContext->commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    profilingContext->tracyContexts.resize(vulkanContext->swapChainFramebuffers.size());
    for (u32 i = 0; i < vulkanContext->commandBuffers.size(); i++)
    {
        profilingContext->tracyContexts[i] =
            TracyVkContext(vulkanContext->physicalDevice, vulkanContext->device,
                           vulkanContext->graphicsQueue, vulkanContext->commandBuffers[i]);
    }
}

void
createCommandPool(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;
    QueueFamilyIndices queueFamilyIndices =
        findQueueFamilies(context, vulkanContext->physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vulkanContext->device, &poolInfo, nullptr,
                            &vulkanContext->commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void
createImageViews(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    vulkanContext->swapChainImageViews.resize(vulkanContext->swapChainImages.size());

    for (uint32_t i = 0; i < vulkanContext->swapChainImages.size(); i++)
    {
        vulkanContext->swapChainImageViews[i] =
            createImageView(vulkanContext->device, vulkanContext->swapChainImages[i],
                            vulkanContext->swapChainImageFormat);
    }
}

void
createSwapChain(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(context, vulkanContext->physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(context, swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkanContext->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(context, vulkanContext->physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
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

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    // It is possible to specify the old swap chain to be replaced by a new one
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vulkanContext->device, &createInfo, nullptr,
                             &vulkanContext->swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(vulkanContext->device, vulkanContext->swapChain, &imageCount, nullptr);
    vulkanContext->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanContext->device, vulkanContext->swapChain, &imageCount,
                            vulkanContext->swapChainImages.data());

    vulkanContext->swapChainImageFormat = surfaceFormat.format;
    vulkanContext->swapChainExtent = extent;
}

void
createSurface(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    if (glfwCreateWindowSurface(vulkanContext->instance, vulkanContext->window, nullptr,
                                &vulkanContext->surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void
createInstance(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    if (vulkanContext->enableValidationLayers && !checkValidationLayerSupport(context))
    {
        throw std::runtime_error("validation layers requested, but not available!");
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

    auto extensions = getRequiredExtensions(context);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vulkanContext->enableValidationLayers)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(vulkanContext->validationLayers.size());
        createInfo.ppEnabledLayerNames = vulkanContext->validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (vulkanContext->enableValidationLayers)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(vulkanContext->validationLayers.size());
        createInfo.ppEnabledLayerNames = vulkanContext->validationLayers.data();

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
        throw std::runtime_error("failed to create instance!");
    }
}

void
createLogicalDevice(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    QueueFamilyIndices indices = findQueueFamilies(context, vulkanContext->physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(vulkanContext->deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = vulkanContext->deviceExtensions.data();

    // NOTE: This if statement is no longer necessary on newer versions
    if (vulkanContext->enableValidationLayers)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(vulkanContext->validationLayers.size());
        createInfo.ppEnabledLayerNames = vulkanContext->validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(vulkanContext->physicalDevice, &createInfo, nullptr,
                       &vulkanContext->device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(vulkanContext->device, indices.graphicsFamily.value(), 0,
                     &vulkanContext->graphicsQueue);
    vkGetDeviceQueue(vulkanContext->device, indices.presentFamily.value(), 0,
                     &vulkanContext->presentQueue);
}

void
pickPhysicalDevice(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    vulkanContext->physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanContext->instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanContext->instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(context, device))
        {
            vulkanContext->physicalDevice = device;
            vulkanContext->msaaSamples = getMaxUsableSampleCount(device);
            break;
        }
    }

    if (vulkanContext->physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool
isDeviceSuitable(Context& context, VkPhysicalDevice device)
{
    // NOTE: This is where you would implement your own checks to see if the
    // device is suitable for your needs
    QueueFamilyIndices indices = findQueueFamilies(context, device);
    bool extensionsSupported = checkDeviceExtensionSupport(context, device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(context, device);
        swapChainAdequate =
            !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}

bool
checkValidationLayerSupport(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : vulkanContext->validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*>
getRequiredExtensions(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (vulkanContext->enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void
setupDebugMessenger(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;
    if (!vulkanContext->enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(vulkanContext->instance, &createInfo, nullptr,
                                     &vulkanContext->debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
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

// // NOTE: you could add logic to explicitly prefer a physical device that
// // supports drawing and presentation in the same queue for improved
// // performance.
QueueFamilyIndices
findQueueFamilies(Context& context, VkPhysicalDevice device)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    // Logic to find queue family indices to populate struct with
    u32 i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanContext->surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }
        i++;
    }
    return indices;
}

bool
checkDeviceExtensionSupport(Context& context, VkPhysicalDevice device)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(vulkanContext->deviceExtensions.begin(),
                                             vulkanContext->deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails
querySwapChainSupport(Context& context, VkPhysicalDevice device)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkanContext->surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanContext->surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanContext->surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanContext->surface, &presentModeCount,
                                              nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanContext->surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR
chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
chooseSwapExtent(Context& context, const VkSurfaceCapabilitiesKHR& capabilities)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(vulkanContext->window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
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
    ArenaTemp frameArena = ArenaTempBegin(context->threadCtx.scratchArena);
    for (Font* font = context->glyphAtlas->fontList; !CheckEmpty(font, &g_font); font = font->next)
    {
        font->instances = &g_GlyphInstance;
    }

    VulkanContext* vulkanContext = context->vulkanContext;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;
    BoxContext* boxContext = context->boxContext;
    ProfilingContext* profilingContext = context->profilingContext;
    (void)profilingContext;

    glyphAtlas->glyphInstanceBuffer =
        ArrayAlloc<Vulkan_GlyphInstance>(frameArena.arena, MAX_GLYPH_INSTANCES);

    boxContext->boxInstances = ArrayAlloc<Vulkan_BoxInstance>(frameArena.arena, MAX_BOX_INSTANCES);
    boxContext->boxList = &g_Box;
    Box* box = PushStructZero(frameArena.arena, Box);
    StackPush(boxContext->boxList, box);
    box->boxInstanceList = &g_BoxInstance;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    // add buttom that is only a rectangle and text at the moment
    {
        ZoneScopedN("Create Buttom");
        // temporary way of choosing font
        Font* font;
        if (!FontExists(glyphAtlas->fontList, 50, &font))
        {
            exitWithError("Font has not been loaded");
        }
        String8 name = str8(frameArena.arena, "test_name");
        F32Vec4 positions = {0.2f, 0.2f, 0.8f, 0.4f};
        F32Vec4 color = {0.0f, 0.8f, 0.8f, 0.1f};
        UI_WidgetFlags flags = UI_WidgetFlag_Clickable;
        AddButton(name, context->uiState, frameArena.arena, box, vulkanContext->swapChainExtent,
                  font, color, "Press Yes or No", 1.0f, 10.0f, 5.0f, context->io, positions, flags);

        name = str8(frameArena.arena, "test_name_2");
        positions = {0.2f, 0.6f, 0.8f, 0.8f};
        color = {0.8f, 0.8f, 0.0f, 0.2f};
        flags = 0;
        AddButton(name, context->uiState, frameArena.arena, box, vulkanContext->swapChainExtent,
                  font, color, "Press Yes or No", 1.0f, 10.0f, 5.0f, context->io, positions, flags);
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
        Text texts[] = {{"testing", 0, 0}, {"more testing", 300, 400}};
        Font* font;
        u32 fontSize = 50;
        if (!FontExists(glyphAtlas->fontList, fontSize, &font))
        {
            exitWithError("the font size specified is not loaded");
        }

        addTexts(frameArena.arena, font, texts, sizeof(texts) / sizeof(texts[0]),
                 Vec2<f32>(0.0f, 0.0f), Vec2<f32>(800.0f, 800.0f));
        glyphAtlas->numInstances =
            InstanceBufferFromFontBuffers(glyphAtlas->glyphInstanceBuffer, glyphAtlas->fontList);
        mapGlyphInstancesToBuffer(glyphAtlas, vulkanContext->physicalDevice, vulkanContext->device,
                                  vulkanContext->graphicsQueue);
    }

    if (vkBeginCommandBuffer(vulkanContext->commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    TracyVkCollect(profilingContext->tracyContexts[currentFrame],
                   vulkanContext->commandBuffers[currentFrame]);

    {
        TracyVkZoneC(profilingContext->tracyContexts[currentFrame],
                     vulkanContext->commandBuffers[currentFrame], "Rectangles GPU", 0xff0000);
        BoxRenderPassBegin(boxContext, vulkanContext, imageIndex, currentFrame);
    }
    {
        TracyVkZoneC(profilingContext->tracyContexts[currentFrame],
                     vulkanContext->commandBuffers[currentFrame], "Text GPU", 0x00FF00);
        beginGlyphAtlasRenderPass(glyphAtlas, vulkanContext, imageIndex, currentFrame);
    }

    if (vkEndCommandBuffer(vulkanContext->commandBuffers[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
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
                        &vulkanContext->inFlightFences[vulkanContext->currentFrame], VK_TRUE,
                        UINT64_MAX);
    }

    context->frameRate = CalculateFrameRate(&context->frameTickPrev, context->cpuFreq);

    uint32_t imageIndex;
    VkResult result =
        vkAcquireNextImageKHR(vulkanContext->device, vulkanContext->swapChain, UINT64_MAX,
                              vulkanContext->imageAvailableSemaphores[vulkanContext->currentFrame],
                              VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        vulkanContext->framebufferResized)
    {
        vulkanContext->framebufferResized = 0;
        recreateSwapChain(*context);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(vulkanContext->device, 1,
                  &vulkanContext->inFlightFences[vulkanContext->currentFrame]);
    vkResetCommandBuffer(vulkanContext->commandBuffers[vulkanContext->currentFrame], 0);

    recordCommandBuffer(context, imageIndex, vulkanContext->currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {
        vulkanContext->imageAvailableSemaphores[vulkanContext->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanContext->commandBuffers[vulkanContext->currentFrame];

    VkSemaphore signalSemaphores[] = {
        vulkanContext->renderFinishedSemaphores[vulkanContext->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanContext->graphicsQueue, 1, &submitInfo,
                      vulkanContext->inFlightFences[vulkanContext->currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
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
        recreateSwapChain(*context);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    vulkanContext->currentFrame =
        (vulkanContext->currentFrame + 1) % vulkanContext->MAX_FRAMES_IN_FLIGHT;
}

void
recreateSwapChain(Context& context)
{
    VulkanContext* vulkanContext = context.vulkanContext;

    int width = 0, height = 0;
    glfwGetFramebufferSize(vulkanContext->window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(vulkanContext->window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vulkanContext->device);

    cleanupSwapChain(context);

    createSwapChain(context);
    createImageViews(context);
    vulkanContext->colorImageView = createColorResources(
        vulkanContext->physicalDevice, vulkanContext->device, vulkanContext->swapChainImageFormat,
        vulkanContext->swapChainExtent, vulkanContext->msaaSamples, vulkanContext->colorImage,
        vulkanContext->colorImageMemory);
    vulkanContext->swapChainFramebuffers = createFramebuffers(
        vulkanContext->device, vulkanContext->colorImageView, vulkanContext->fontRenderPass,
        vulkanContext->swapChainExtent, vulkanContext->swapChainImageViews);
}