#pragma once

#include <array>
#include <initializer_list>
#include <stdexcept>
#include <string>
extern "C"
{
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
}
#include <optional>
#include <vector>

#include <glm/glm.hpp>
// profiler
#include "profiler/tracy/TracyVulkan.hpp"

const int MAX_GLYPHS = 126;

template <typename T> struct StaticArray
{
    T* data;
    u32 size;

    StaticArray()
    {
        this->data = nullptr;
        this->size = 0;
    }

    StaticArray(u32 size)
    {
        this->data = new T[size];
        this->size = size;
    }

    inline void
    setSize(u32 size)
    {
        if (data)
        {
            throw std::runtime_error("Cannot set size of StaticArray after initialization!");
        }
        this->data = new T[size];
        this->size = size;
    }

    ~StaticArray()
    {
        if (data)
        {
            delete[] data;
        }
    }
};

template <typename T> struct InstanceArray
{
    T* data;
    u64 size;
    u64 capacity;

    InstanceArray()
    {
        capacity = 1024 * 1024; // NOTE: This might not be enough at some point
        data = new T[capacity];
        size = 0;
    }

    void
    add(std::initializer_list<T> list)
    {
        size = 0;
        for (auto& elem : list)
        {
            pushBack(elem);
        }
    }

    inline int
    pushBack(T glyph)
    {
        if (size >= capacity)
        {
            error("Not enough assigned memory for glyphs buffer");
        }
        data[size] = glyph;
        size++;

        return 0;
    }

    inline void
    reset()
    {
        size = 0;
    }

    ~InstanceArray()
    {
        if (data)
        {
            delete[] data;
        }
    }
};

extern "C"
{
    struct Vulkan_PushConstantInfo
    {
        uint32_t offset;
        uint32_t size;
    };

    struct Vulkan_Resolution
    {
        static const uint32_t SIZE = 2;
        f32 data[SIZE];
        Vulkan_PushConstantInfo bufferInfo;

        Vulkan_Resolution(VkExtent2D extent, Vulkan_PushConstantInfo bufferInfo)
        {
            data[0] = (f32)extent.width;
            data[1] = (f32)extent.height;
            this->bufferInfo = bufferInfo;
        }

        inline uint32_t
        size()
        {
            return sizeof(data);
        }
    };

    struct GlyphBuffer
    {
        glm::vec2 pos;
        glm::vec2 size;
        glm::float32 glyphOffset;

        static VkVertexInputBindingDescription
        getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(GlyphBuffer);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            return bindingDescription;
        }
        static std::vector<VkVertexInputAttributeDescription>
        getAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
            attributeDescriptions.resize(3);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(GlyphBuffer, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(GlyphBuffer, size);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(GlyphBuffer, glyphOffset);

            return attributeDescriptions;
        }
    };

    struct RectangleInstance
    {
        glm::vec2 pos1;
        glm::vec2 pos2;
        glm::vec3 color;
        glm::f32 softness;
        glm::f32 borderThickness;
        glm::f32 cornerRadius;

        static VkVertexInputBindingDescription
        getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(RectangleInstance);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            return bindingDescription;
        }
        static std::vector<VkVertexInputAttributeDescription>
        getAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
            attributeDescriptions.resize(6);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(RectangleInstance, pos1);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(RectangleInstance, pos2);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(RectangleInstance, color);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(RectangleInstance, softness);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(RectangleInstance, borderThickness);

            attributeDescriptions[5].binding = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[5].offset = offsetof(RectangleInstance, cornerRadius);

            return attributeDescriptions;
        }
    };

    struct Character
    {
        char character;
        float width; // Size of glyph
        float height;
        float bearingX; // Offset from baseline to left/top of glyph
        float bearingY;
        unsigned int advance; // Offset to advance to next glyph
        u32 glyphOffset;
    };

    struct Text
    {
        std::string text;
        float x;
        float y;
    };
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

    struct VulkanContext
    {
        const u32 WIDTH = 800;
        const u32 HEIGHT = 600;
        const u32 MAX_FRAMES_IN_FLIGHT = 2;

        const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
        const u8 enableValidationLayers = 0;
#else
        const u8 enableValidationLayers = 1;
#endif
        u8 framebufferResized = 0;

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

        Vulkan_PushConstantInfo resolutionInfo;
        const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
    };

    struct Vulkan_Rectangle
    {
        VkBuffer instBuffer;
        VkDeviceMemory instMemoryBuffer;
        VkDeviceSize instBufferSize;
        VkBuffer indexBuffer;
        VkDeviceMemory indexMemoryBuffer;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
    };

    struct ProfilingContext
    {
        // tracy profiling context
        std::vector<TracyVkCtx> tracyContexts;
    };

    struct GlyphAtlas
    {
        std::array<Character, MAX_GLYPHS> characters;
        InstanceArray<GlyphBuffer> glyphInstances;
        const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
    };

    struct GUI_Rectangle
    {
        InstanceArray<RectangleInstance> rectangleInstances;
    };

    struct Vulkan_GlyphAtlas
    {
        VkBuffer glyphInstBuffer;
        VkDeviceMemory glyphMemoryBuffer;
        VkDeviceSize glyphInstBufferSize;
        VkBuffer glyphIndexBuffer;
        VkDeviceMemory glyphIndexMemoryBuffer;

        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;

        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        VkDescriptorPool descriptorPool;
        StaticArray<VkDescriptorSet> descriptorSets;
        VkDescriptorSetLayout descriptorSetLayout;
    };

    typedef struct Context
    {
        VulkanContext* vulkanContext;
        ProfilingContext* profilingContext;
        GlyphAtlas* glyphAtlas;
        GUI_Rectangle* rect;
        Vulkan_Rectangle* vulkanRectangle;
        Vulkan_GlyphAtlas* vulkanGlyphAtlas;
    } Context;
}

extern "C"
{
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
