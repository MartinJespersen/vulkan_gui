#pragma once
#include "shader.hpp"
#include <stdexcept>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>

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
                      VkImage image, VkFormat format, VkImageLayout oldLayout,
                      VkImageLayout newLayout);

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
                       std::string fragShaderPath);

VkRenderPass
createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples,
                 VkAttachmentLoadOp loadOp, VkImageLayout initialLayout, VkImageLayout finalLayout);