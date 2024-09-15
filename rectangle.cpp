#include "rectangle.hpp"
#include "utils.hpp"
#include "vertex.hpp"

void
createRectangleIndexBuffer(Vulkan_Rectangle& vulkanRectangle, VkPhysicalDevice physicalDevice,
                           VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           std::vector<uint16_t> indices)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanRectangle.indexBuffer,
                 vulkanRectangle.indexMemoryBuffer);

    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vulkanRectangle.indexBuffer,
               bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void
beginRectangleRenderPass(Vulkan_Rectangle& vulkanRectangle, VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass, VkFramebuffer swapchainFramebuffer,
                         VkExtent2D swapChainExtent, u32 rectangleArraySize,
                         Vulkan_Resolution resolution)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vulkanRectangle.graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vulkanRectangle.instBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, vulkanRectangle.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(commandBuffer, vulkanRectangle.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       resolution.bufferInfo.offset, resolution.size(), resolution.data);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(rectangleArraySize), 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void
cleanupRectangle(Vulkan_Rectangle& vulkanRectangle, VkDevice device)
{
    vkDestroyBuffer(device, vulkanRectangle.indexBuffer, nullptr);
    vkFreeMemory(device, vulkanRectangle.indexMemoryBuffer, nullptr);

    vkDestroyBuffer(device, vulkanRectangle.instBuffer, nullptr);
    vkFreeMemory(device, vulkanRectangle.instMemoryBuffer, nullptr);

    vkDestroyPipeline(device, vulkanRectangle.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, vulkanRectangle.pipelineLayout, nullptr);
}

void
mapRectanglesToBuffer(Vulkan_Rectangle& vulkanRectangle,
                      GrowthVector<RectangleInstance>& rectangles, VkPhysicalDevice physicalDevice,
                      VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
{
    VkDeviceSize bufferSize = sizeof(RectangleInstance) * rectangles.size;
    if (bufferSize > vulkanRectangle.instBufferSize)
    {
        vkDestroyBuffer(device, vulkanRectangle.instBuffer, nullptr);
        vkFreeMemory(device, vulkanRectangle.instMemoryBuffer, nullptr);

        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vulkanRectangle.instBuffer, vulkanRectangle.instMemoryBuffer);
    }

    // TODO: Consider using vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges instead of
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for performance.

    void* data;
    vkMapMemory(device, vulkanRectangle.instMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, rectangles.data, (size_t)bufferSize);
    vkUnmapMemory(device, vulkanRectangle.instMemoryBuffer);

    vulkanRectangle.instBufferSize = bufferSize;
}