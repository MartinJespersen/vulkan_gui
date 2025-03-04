#pragma once

// user defined: [hpp]
#include "base/base.hpp"
#include "ui/ui.hpp"

// profiler
#include "profiler/tracy/Tracy.hpp"
#include "profiler/tracy/TracyVulkan.hpp"

const u64 FONT_ARENA_SIZE = MEGABYTE(4);
const u32 MAX_FONTS_IN_USE = 10;
const u64 MAX_GLYPH_INSTANCES = 1000;
#ifdef PROFILING_ENABLE
BufferDec(TracyVkCtx);
#endif

extern "C"
{
    struct ProfilingContext
    {
#ifdef PROFILING_ENABLE
        // tracy profiling context
        TracyVkCtx_Buffer tracyContexts;
#endif
    };

    struct Context
    {
        VulkanContext* vulkanContext;
        ProfilingContext* profilingContext;
        GlyphAtlas* glyphAtlas;
        BoxContext* boxContext;
        UI_IO* io;

        UI_State* uiState;
        u64 frameTickPrev;
        f64 frameRate;
        u64 cpuFreq;
    };
}

extern "C"
{
    void
    InitContext(Context* context);
    void
    DeleteContext(Context* context);
    void
    initWindow(Context* context);
    void
    VulkanInit(Context* context);
    void
    cleanup(Context* context);
    void
    drawFrame(Context* context);
}

void
recordCommandBuffer(Context* context, u32 imageIndex, u32 currentFrame);

void
recreateSwapChain(VulkanContext* vulkanContext);

void
createSyncObjects(VulkanContext* vulkanContext);

void
createCommandBuffers(Context* context);

void
SwapChainImageViewsCreate(VulkanContext* vulkanContext);
void
createCommandPool(VulkanContext* vulkanContext);

void
cleanupColorResources(VulkanContext* vulkanContext);
void
cleanupSwapChain(VulkanContext* vulkanContext);
void
createInstance(VulkanContext* vulkanContext);
void
setupDebugMessenger(VulkanContext* vulkanContext);
void
createSurface(VulkanContext* vulkanContext);
void
pickPhysicalDevice(VulkanContext* vulkanContext);
void
createLogicalDevice(VulkanContext* vulkanContext);

SwapChainInfo
SwapChainCreate(Arena* arena, VulkanContext* vulkanContext);
u32
SwapChainImageCountGet(VulkanContext* vulkanContext);
void
SwapChainImagesCreate(VulkanContext* vulkanContext, SwapChainInfo swapChainInfo, u32 imageCount);

VkExtent2D
chooseSwapExtent(VulkanContext* vulkanContext, const VkSurfaceCapabilitiesKHR& capabilities);

String8_Buffer
getRequiredExtensions(VulkanContext* vulkanContext);

void
populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

bool
isDeviceSuitable(VulkanContext* vulkanContext, VkPhysicalDevice device,
                 QueueFamilyIndexBits indexBits);

VkSampleCountFlagBits
getMaxUsableSampleCount(VkPhysicalDevice device);

bool
checkDeviceExtensionSupport(VulkanContext* vulkanContext, VkPhysicalDevice device);

bool
checkValidationLayerSupport(VulkanContext* vulkanContext);

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(VkSurfaceFormatKHR_Buffer availableFormats);

VkPresentModeKHR
chooseSwapPresentMode(VkPresentModeKHR_Buffer availablePresentModes);
