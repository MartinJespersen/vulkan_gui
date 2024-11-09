#pragma once
#include "entrypoint.hpp"
#include "vulkan_helpers.hpp"
#include <vector>
#include <vulkan/vulkan.h>

void
createRectangleIndexBuffer(Vulkan_Rectangle& vulkanRectangle, VkPhysicalDevice physicalDevice,
                           VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           std::vector<uint16_t> indices);

void
beginRectangleRenderPass(Vulkan_Rectangle& vulkanRectangle, VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass, VkFramebuffer swapchainFramebuffer,
                         VkExtent2D swapChainExtent, std::vector<RectangleInstance> boxInstances,
                         Vulkan_Resolution resolution);
void
cleanupRectangle(Vulkan_Rectangle& vulkanRectangle, VkDevice device);

void
mapRectanglesToBuffer(Vulkan_Rectangle& vulkanRectangle,
                      InstanceArray<RectangleInstance>& rectangles, VkPhysicalDevice physicalDevice,
                      VkDevice device);