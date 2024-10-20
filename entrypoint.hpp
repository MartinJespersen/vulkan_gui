#pragma once

#include <array>
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

template <typename T> struct GrowthVector
{
    T* data;
    u32 size;
    u32 capacity;

    GrowthVector()
    {
        this->data = nullptr;
        this->size = 0;
        this->capacity = 0;
    }

    GrowthVector(T* data, u32 size)
    {
        this->data = data;
        this->size = size;
        this->capacity = this->size;
    }

    inline int
    pushBack(T glyph)
    {
        if (size >= capacity)
        {
            return -1;
        }
        data[size] = glyph;
        size++;

        return 0;
    }

    inline void
    reset(u32 newSize)
    {
        if (newSize > capacity)
        {
            if (data)
            {
                delete[] data;
            }
            this->capacity = newSize;
            data = new T[this->capacity];
        }
        size = 0;
    }
};

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
        float data[SIZE];
        Vulkan_PushConstantInfo bufferInfo;

        Vulkan_Resolution(VkExtent2D extent, Vulkan_PushConstantInfo bufferInfo)
        {
            data[0] = extent.width;
            data[1] = extent.height;
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

    struct ArrayGlyphInstance
    {
        GlyphBuffer* data;
        size_t size;
        size_t capacity;

        ArrayGlyphInstance() : data(nullptr), size(0), capacity(0)
        {
        }

        inline int
        pushBack(GlyphBuffer glyph)
        {
            if (size >= capacity)
            {
                return -1;
            }
            data[size] = glyph;
            size++;

            return 0;
        }

        inline void
        reset(size_t newSize)
        {
            if (newSize > capacity)
            {
                if (data)
                {
                    delete[] data;
                }
                this->capacity = newSize;
                data = new GlyphBuffer[this->capacity];
            }
            size = 0;
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
        float glyphOffset;
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
        ArrayGlyphInstance glyphInstances;
        const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
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
