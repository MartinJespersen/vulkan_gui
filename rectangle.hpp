#pragma once
#include <vulkan/vulkan.h>
#include "vulkan_helpers.hpp"
#include "string.h"
#include <vector>
#include "vertex.hpp"

struct Vulkan_Rectangle
{
    VkBuffer boxInstBuffer;
    VkDeviceMemory boxMemoryBuffer;
    VkBuffer boxIndexBuffer;
    VkDeviceMemory boxIndexMemoryBuffer;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
};

Vulkan_Rectangle vulkanRectangle;

void createRectangleIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<uint16_t> indices);
void createRectangleInstBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<RectangleInstance> boxInstances);
void beginRectangleRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer swapchainFramebuffer, VkExtent2D swapChainExtent, std::vector<RectangleInstance> boxInstances, VkDescriptorSet descriptorSet, Vulkan_Resolution resolution);
void cleanupRectangle(VkDevice device);