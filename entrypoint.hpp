#pragma once

#include <array>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
// profiler
#include "profiler/tracy/TracyVulkan.hpp"

// user defined: [hpp]
#include "base/base.hpp"
#include "box.hpp"
#include "button.hpp"
#include "fonts.hpp"
#include "ui/ui.hpp"
#include "vulkan_helpers.hpp"

const int MAX_GLYPHS = 126;
const u32 MAX_FONT_SIZE = 100;
const u64 FONT_ARENA_SIZE = MEGABYTE(4);
const u32 MAX_FONTS_IN_USE = 10;
const u64 MAX_GLYPH_INSTANCES = 1000;
const u64 MAX_BOX_INSTANCES = 1000;

extern "C"
{
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

    struct ProfilingContext
    {
        // tracy profiling context
        std::vector<TracyVkCtx> tracyContexts;
    };

    struct Context
    {
        VulkanContext* vulkanContext;
        ProfilingContext* profilingContext;
        GlyphAtlas* glyphAtlas;
        BoxContext* boxContext;
        UI_IO* io;
        ThreadCtx threadCtx;
        UI_State* uiState;
        u64 frameTickPrev;
        f64 frameRate;
        u64 cpuFreq;
    };
}

extern "C"
{
    ThreadCtx
    InitContext(Context* context);
    void
    DeleteContext(Context* context);

    void
    initWindow(Context* context);
    void
    initVulkan(Context* context);
    void
    cleanup(Context* context);
    void
    drawFrame(Context* context);
}

void
recordCommandBuffer(Context* context, u32 imageIndex, u32 currentFrame);

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
