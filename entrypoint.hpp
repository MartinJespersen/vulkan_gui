#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>

// profiler
#include "profiler/tracy/TracyVulkan.hpp"

#include "fonts.hpp"
#include "rectangle.hpp"
#include "vulkan_helpers.hpp"

#define EXPORT __attribute__((visibility("default")))

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool
    isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Input
{
    i32 keyS;
    i32 keyCtrl;
};

struct Context
{
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    bool framebufferResized = false;

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    // TODO: add descriptor set layout and descriptor set to glyph atlas and rectangle

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkRenderPass firstRenderPass;
    VkRenderPass renderPass;

    Vulkan_PushConstantInfo pushConstantInfo;

    // tracy profiling context
    std::vector<TracyVkCtx> tracyContexts;

    GlyphAtlas glyphAtlas;
    Vulkan_Rectangle vulkanRectangle;
    Vulkan_GlyphAtlas vulkanGlyphAtlas;

    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
};

extern "C"
{
    EXPORT void
    initWindow(Context* context);
    EXPORT void
    initVulkan(Context* context);
    EXPORT void
    cleanup(Context* context);
    EXPORT void
    drawFrame(Context* context);
}

void
recordCommandBuffer(Context& context, u32 imageIndex, u32 currentFrame);

void
recreateSwapChain(Context& context);

void
createSyncObjects(Context& context);

void
createCommandBuffers(Context& context);

void
createImageViews(Context& context);
void
createCommandPool(Context& context);

static void
framebufferResizeCallback(GLFWwindow* window, int width, int height);

void
cleanupColorResources(Context& context);
void
cleanupSwapChain(Context& context);
void
createInstance(Context& context);
void
setupDebugMessenger(Context& context);
void
createSurface(Context& context);
void
pickPhysicalDevice(Context& context);
void
createLogicalDevice(Context& context);
void
createSwapChain(Context& context);

QueueFamilyIndices
findQueueFamilies(Context& context, VkPhysicalDevice device);

SwapChainSupportDetails
querySwapChainSupport(Context& context, VkPhysicalDevice device);

VkExtent2D
chooseSwapExtent(Context& context, const VkSurfaceCapabilitiesKHR& capabilities);

std::vector<const char*>
getRequiredExtensions(Context& context);

void
populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

bool
isDeviceSuitable(Context& context, VkPhysicalDevice device);

VkSampleCountFlagBits
getMaxUsableSampleCount(VkPhysicalDevice device);

bool
checkDeviceExtensionSupport(Context& context, VkPhysicalDevice device);

bool
checkValidationLayerSupport(Context& context);

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

VkPresentModeKHR
chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
