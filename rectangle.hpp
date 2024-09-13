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

void createRectangleIndexBuffer(Vulkan_Rectangle &vulkanRectangle, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<uint16_t> indices);
void createRectangleInstBuffer(Vulkan_Rectangle &vulkanRectangle, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<RectangleInstance> boxInstances);
void beginRectangleRenderPass(Vulkan_Rectangle &vulkanRectangle,
                              VkCommandBuffer commandBuffer,
                              VkRenderPass renderPass,
                              VkFramebuffer swapchainFramebuffer,
                              VkExtent2D swapChainExtent,
                              VkDescriptorSet descriptorSet,
                              std::vector<RectangleInstance> boxInstances,
                              Vulkan_Resolution resolution);
void cleanupRectangle(Vulkan_Rectangle &vulkanRectangle, VkDevice device);