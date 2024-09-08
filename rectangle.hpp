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
    VkRenderPass renderPass;
};

Vulkan_Rectangle vulkanRectangle;

void createBoxIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<uint16_t> indices);
void createBoxInstBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<BoxInstance> boxInstances);
void BoxRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkFramebuffer swapchainFramebuffer, VkExtent2D swapChainExtent, std::vector<BoxInstance> boxInstances, VkDescriptorSet descriptorSet);
void cleanupRectangle(VkDevice device);