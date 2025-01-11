#pragma once

#include "entrypoint.hpp"
#include <string>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>

struct Vulkan_PushConstantInfo
{
    uint32_t offset;
    uint32_t size;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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

    VkRenderPass boxRenderPass;
    VkRenderPass fontRenderPass;

    Vulkan_PushConstantInfo resolutionInfo;
    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
};

VkCommandBuffer
beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

void
endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                      VkCommandBuffer commandBuffer);

void
createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size,
             VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
             VkDeviceMemory& bufferMemory);
void
copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer,
           VkBuffer dstBuffer, VkDeviceSize size);

void
transitionImageLayout(VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue,
                      VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

void
copyBufferToImage(VkCommandPool commandPool, VkDevice device, VkQueue queue, VkBuffer buffer,
                  VkImage image, uint32_t width, uint32_t height);
void
createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height,
            VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
            VkDeviceMemory& imageMemory);

VkImageView
createImageView(VkDevice device, VkImage image, VkFormat format);

VkImageView
createColorResources(VkPhysicalDevice physicalDevice, VkDevice device,
                     VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
                     VkSampleCountFlagBits msaaSamples, VkImage& colorImage,
                     VkDeviceMemory& colorImageMemory);

std::vector<VkFramebuffer>
createFramebuffers(VkDevice device, VkImageView colorImageView, VkRenderPass renderPass,
                   VkExtent2D swapChainExtent, std::vector<VkImageView> swapChainImageViews);

std::tuple<VkPipelineLayout, VkPipeline>
createGraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass,
                       VkDescriptorSetLayout descriptorSetLayout, VkSampleCountFlagBits msaaSamples,
                       VkVertexInputBindingDescription bindingDescription,
                       std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
                       Vulkan_PushConstantInfo pushConstInfo, std::string vertShaderPath,
                       std::string fragShaderPath, VkShaderStageFlagBits pushConstantStage);

VkRenderPass
createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples,
                 VkAttachmentLoadOp loadOp, VkImageLayout initialLayout, VkImageLayout finalLayout);