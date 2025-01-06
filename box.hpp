#pragma once
#include "entrypoint.hpp"
#include "vulkan_helpers.hpp"
#include <vector>
#include <vulkan/vulkan.h>

void
createRectangleIndexBuffer(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           std::vector<uint16_t> indices);

void
beginRectangleRenderPass(BoxContext* boxContext, VulkanContext* vulkanContext, u32 imageIndex,
                         u32 currentFrame);

void
cleanupRectangle(BoxContext* boxContext, VkDevice device);

void
mapRectanglesToBuffer(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device);

u64
InstanceBufferFromBoxes(LinkedList<Box>* boxList, Array<BoxInstance> outBuffer);