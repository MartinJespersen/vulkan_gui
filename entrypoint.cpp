#include <set>
#include <vulkan/vulkan_core.h>
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

#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "fonts.cpp"
#include "fonts.hpp"
#include "rectangle.cpp"
#include "rectangle.hpp"

#include "vulkan_helpers.cpp"
#include "vulkan_helpers.hpp"

#include "entrypoint.hpp"

__attribute__((constructor(101))) void
before_main()
{
    printf("Before main\n");
}

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
initVulkan(Context* context)
{
    createInstance(*context);
    setupDebugMessenger(*context);
    createSurface(*context);
    pickPhysicalDevice(*context);
    createLogicalDevice(*context);
    createSwapChain(*context);
    createImageViews(*context);
    context->firstRenderPass = createRenderPass(
        context->device, context->swapChainImageFormat, context->msaaSamples,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    context->renderPass =
        createRenderPass(context->device, context->swapChainImageFormat, context->msaaSamples,
                         VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    createCommandPool(*context);

    context->pushConstantInfo.offset = 0;
    context->pushConstantInfo.size = sizeof(float) * 2;

    auto rectangleObjects = createGraphicsPipeline(
        context->device, context->swapChainExtent, context->renderPass, VK_NULL_HANDLE,
        context->msaaSamples, RectangleInstance::getBindingDescription(),
        RectangleInstance::getAttributeDescriptions(), context->pushConstantInfo,
        "shaders/vert.spv", "shaders/frag.spv");

    context->vulkanRectangle.pipelineLayout = std::get<0>(rectangleObjects);
    context->vulkanRectangle.graphicsPipeline = std::get<1>(rectangleObjects);

    context->colorImageView =
        createColorResources(context->physicalDevice, context->device,
                             context->swapChainImageFormat, context->swapChainExtent,
                             context->msaaSamples, context->colorImage, context->colorImageMemory);
    context->swapChainFramebuffers =
        createFramebuffers(context->device, context->colorImageView, context->renderPass,
                           context->swapChainExtent, context->swapChainImageViews);
    createRectangleIndexBuffer(context->vulkanRectangle, context->physicalDevice, context->device,
                               context->commandPool, context->graphicsQueue, context->indices);

    {
        createGlyphAtlasImage(context->vulkanGlyphAtlas, context->glyphAtlas,
                              context->physicalDevice, context->device, context->commandPool,
                              context->graphicsQueue);
        createGlyphAtlasImageView(context->vulkanGlyphAtlas, context->device);
        createGlyphAtlasTextureSampler(context->vulkanGlyphAtlas, context->physicalDevice,
                                       context->device);

        context->vulkanGlyphAtlas.descriptorSets.setSize(context->MAX_FRAMES_IN_FLIGHT);
        createFontDescriptorSetLayout(context->device,
                                      context->vulkanGlyphAtlas.descriptorSetLayout);
        createFontDescriptorPool(context->device, context->MAX_FRAMES_IN_FLIGHT,
                                 context->vulkanGlyphAtlas.descriptorPool);
        createFontDescriptorSets(
            context->vulkanGlyphAtlas.textureImageView, context->vulkanGlyphAtlas.textureSampler,
            context->vulkanGlyphAtlas.descriptorPool, context->vulkanGlyphAtlas.descriptorSetLayout,
            context->device, context->MAX_FRAMES_IN_FLIGHT,
            context->vulkanGlyphAtlas.descriptorSets);
        auto glyphAtlasGraphicsObjects = createGraphicsPipeline(
            context->device, context->swapChainExtent, context->renderPass,
            context->vulkanGlyphAtlas.descriptorSetLayout, context->msaaSamples,
            GlyphBuffer::getBindingDescription(), GlyphBuffer::getAttributeDescriptions(),
            context->pushConstantInfo, "shaders/text_vert.spv", "shaders/text_frag.spv");
        context->vulkanGlyphAtlas.pipelineLayout = std::get<0>(glyphAtlasGraphicsObjects);
        context->vulkanGlyphAtlas.graphicsPipeline = std::get<1>(glyphAtlasGraphicsObjects);

        createGlyphIndexBuffer(context->vulkanGlyphAtlas, context->glyphAtlas,
                               context->physicalDevice, context->device, context->commandPool,
                               context->graphicsQueue);
    }

    createCommandBuffers(*context);
    createSyncObjects(*context);
}

void
cleanup(Context* context)
{
    if (context->enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(context->instance, context->debugMessenger, nullptr);
    }
    cleanupSwapChain(*context);

    cleanupFontResources(context->vulkanGlyphAtlas, context->device);
    cleanupRectangle(context->vulkanRectangle, context->device);

    vkDestroyRenderPass(context->device, context->firstRenderPass, nullptr);
    vkDestroyRenderPass(context->device, context->renderPass, nullptr);

    for (u32 i = 0; i < context->tracyContexts.size(); i++)
    {
        TracyVkDestroy(context->tracyContexts[i]);
    }
    vkDestroyCommandPool(context->device, context->commandPool, nullptr);

    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);

    for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(context->device, context->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->device, context->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context->device, context->inFlightFences[i], nullptr);
    }

    vkDestroyDevice(context->device, nullptr);
    vkDestroyInstance(context->instance, nullptr);

    glfwDestroyWindow(context->window);

    glfwTerminate();
}

void
initWindow(Context* context)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    context->window = glfwCreateWindow(context->WIDTH, context->HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(context->window, &context);
    glfwSetFramebufferSizeCallback(context->window, framebufferResizeCallback);
}

static void
framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
    context->framebufferResized = true;
}

void
cleanupColorResources(Context& context)
{
    vkDestroyImageView(context.device, context.colorImageView, nullptr);
    vkDestroyImage(context.device, context.colorImage, nullptr);
    vkFreeMemory(context.device, context.colorImageMemory, nullptr);
}

void
cleanupSwapChain(Context& context)
{
    cleanupColorResources(context);

    for (size_t i = 0; i < context.swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(context.device, context.swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < context.swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(context.device, context.swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(context.device, context.swapChain, nullptr);
}

void
createSyncObjects(Context& context)
{
    context.imageAvailableSemaphores.resize(context.MAX_FRAMES_IN_FLIGHT);
    context.renderFinishedSemaphores.resize(context.MAX_FRAMES_IN_FLIGHT);
    context.inFlightFences.resize(context.MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < context.MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr,
                              &context.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.device, &semaphoreInfo, nullptr,
                              &context.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceInfo, nullptr, &context.inFlightFences[i]) !=
                VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void
createCommandBuffers(Context& context)
{
    context.commandBuffers.resize(context.swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)context.commandBuffers.size();

    if (vkAllocateCommandBuffers(context.device, &allocInfo, context.commandBuffers.data()) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    context.tracyContexts.resize(context.swapChainFramebuffers.size());
    for (u32 i = 0; i < context.commandBuffers.size(); i++)
    {
        context.tracyContexts[i] = TracyVkContext(context.physicalDevice, context.device,
                                                  context.graphicsQueue, context.commandBuffers[i]);
    }
}

void
createCommandPool(Context& context)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(context, context.physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void
createImageViews(Context& context)
{
    context.swapChainImageViews.resize(context.swapChainImages.size());

    for (uint32_t i = 0; i < context.swapChainImages.size(); i++)
    {
        context.swapChainImageViews[i] = createImageView(context.device, context.swapChainImages[i],
                                                         context.swapChainImageFormat);
    }
}

void
createSwapChain(Context& context)
{
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(context, context.physicalDevice);

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
    createInfo.surface = context.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(context, context.physicalDevice);
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

    if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapChain) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, nullptr);
    context.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount,
                            context.swapChainImages.data());

    context.swapChainImageFormat = surfaceFormat.format;
    context.swapChainExtent = extent;
}

void
createSurface(Context& context)
{
    if (glfwCreateWindowSurface(context.instance, context.window, nullptr, &context.surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void
createInstance(Context& context)
{
    if (context.enableValidationLayers && !checkValidationLayerSupport(context))
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

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto extensions = getRequiredExtensions(context);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (context.enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(context.validationLayers.size());
        createInfo.ppEnabledLayerNames = context.validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (context.enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(context.validationLayers.size());
        createInfo.ppEnabledLayerNames = context.validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }
}

void
createLogicalDevice(Context& context)
{
    QueueFamilyIndices indices = findQueueFamilies(context, context.physicalDevice);

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

    createInfo.enabledExtensionCount = static_cast<uint32_t>(context.deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = context.deviceExtensions.data();

    // NOTE: This if statement is no longer necessary on newer versions
    if (context.enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(context.validationLayers.size());
        createInfo.ppEnabledLayerNames = context.validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(context.device, indices.graphicsFamily.value(), 0, &context.graphicsQueue);
    vkGetDeviceQueue(context.device, indices.presentFamily.value(), 0, &context.presentQueue);
}

void
pickPhysicalDevice(Context& context)
{
    context.physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(context, device))
        {
            context.physicalDevice = device;
            context.msaaSamples = getMaxUsableSampleCount(device);
            break;
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
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
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : context.validationLayers)
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
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (context.enableValidationLayers)
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
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void
setupDebugMessenger(Context& context)
{
    if (!context.enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(context.instance, &createInfo, nullptr,
                                     &context.debugMessenger) != VK_SUCCESS)
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

// NOTE: you could add logic to explicitly prefer a physical device that
// supports drawing and presentation in the same queue for improved
// performance.
QueueFamilyIndices
findQueueFamilies(Context& context, VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    // Logic to find queue family indices to populate struct with
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &presentSupport);
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
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(context.deviceExtensions.begin(),
                                             context.deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails
querySwapChainSupport(Context& context, VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount,
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
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(context.window, &width, &height);

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
recordCommandBuffer(Context& context, u32 imageIndex, u32 currentFrame)
{
    ZoneScoped;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(context.commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    TracyVkCollect(context.tracyContexts[currentFrame], context.commandBuffers[currentFrame]);

    Vulkan_Resolution resolution =
        Vulkan_Resolution(context.swapChainExtent, context.pushConstantInfo);

    // recording rectangles
    {
        ZoneScopedN("Rectangle CPU");
        TracyVkZoneC(context.tracyContexts[currentFrame], context.commandBuffers[currentFrame],
                     "Rectangles GPU", 0xff0000);
        RectangleInstance boxInstances[] = {{{0.0f, 0.0f}, {0.2f, 0.2f}, {0.1f, 0.1f, 0.1f}},
                                            {{0.8f, 0.0f}, {1.0f, 0.2f}, {0.8f, 0.8f, 0.8f}},
                                            {{0.0f, 0.8f}, {0.2f, 1.0f}, {0.8f, 0.8f, 0.8f}},
                                            {{0.8f, 0.8f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}};

        GrowthVector<RectangleInstance> rectangleInstances(
            &boxInstances[0], static_cast<u32>(sizeof(boxInstances) / sizeof(RectangleInstance)));
        mapRectanglesToBuffer(context.vulkanRectangle, rectangleInstances, context.physicalDevice,
                              context.device, context.commandPool, context.graphicsQueue);
        beginRectangleRenderPass(context.vulkanRectangle, context.commandBuffers[currentFrame],
                                 context.firstRenderPass, context.swapChainFramebuffers[imageIndex],
                                 context.swapChainExtent, rectangleInstances.size, resolution);
    }
    // recording text
    {
        ZoneScopedN("Text CPU");
        TracyVkZoneC(context.tracyContexts[currentFrame], context.commandBuffers[currentFrame],
                     "Text GPU", 0x00FF00);
        Text texts[] = {{"testing", 300, 300}, {"more testing", 300, 400}};
        addTexts(context.glyphAtlas, texts, sizeof(texts) / sizeof(texts[0]));
        mapGlyphInstancesToBuffer(context.vulkanGlyphAtlas, context.glyphAtlas,
                                  context.physicalDevice, context.device, context.commandPool,
                                  context.graphicsQueue);
        beginGlyphAtlasRenderPass(
            context.vulkanGlyphAtlas, context.glyphAtlas, context.commandBuffers[currentFrame],
            context.swapChainFramebuffers[imageIndex], context.swapChainExtent,
            context.vulkanGlyphAtlas.descriptorSets.data[currentFrame], context.renderPass,
            resolution);
    }

    if (vkEndCommandBuffer(context.commandBuffers[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void
drawFrame(Context* context)
{
    ZoneScoped;
    {
        ZoneScopedN("Wait for frame");
        vkWaitForFences(context->device, 1, &context->inFlightFences[context->currentFrame],
                        VK_TRUE, UINT64_MAX);
    }
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        context->device, context->swapChain, UINT64_MAX,
        context->imageAvailableSemaphores[context->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        context->framebufferResized)
    {
        context->framebufferResized = false;
        recreateSwapChain(*context);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(context->device, 1, &context->inFlightFences[context->currentFrame]);
    vkResetCommandBuffer(context->commandBuffers[context->currentFrame], 0);

    recordCommandBuffer(*context, imageIndex, context->currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {context->imageAvailableSemaphores[context->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context->commandBuffers[context->currentFrame];

    VkSemaphore signalSemaphores[] = {context->renderFinishedSemaphores[context->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo,
                      context->inFlightFences[context->currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {context->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(context->presentQueue, &presentInfo);
    // TracyVkCollect(tracyContexts[currentFrame], commandBuffers[currentFrame]);
    FrameMark; // end of frame is assumed to be here as the
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        context->framebufferResized)
    {
        context->framebufferResized = false;
        recreateSwapChain(*context);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    context->currentFrame = (context->currentFrame + 1) % context->MAX_FRAMES_IN_FLIGHT;
}

void
recreateSwapChain(Context& context)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(context.window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(context.window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(context.device);

    cleanupSwapChain(context);

    createSwapChain(context);
    createImageViews(context);
    context.colorImageView = createColorResources(
        context.physicalDevice, context.device, context.swapChainImageFormat,
        context.swapChainExtent, context.msaaSamples, context.colorImage, context.colorImageMemory);
    context.swapChainFramebuffers =
        createFramebuffers(context.device, context.colorImageView, context.renderPass,
                           context.swapChainExtent, context.swapChainImageViews);
}