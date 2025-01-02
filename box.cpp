#include "box.hpp"
#include <cstring>

void
createRectangleIndexBuffer(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
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
    MemoryCopy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, boxContext->indexBuffer,
                 boxContext->indexMemoryBuffer);

    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, boxContext->indexBuffer,
               bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void
beginRectangleRenderPass(BoxContext* boxContext, VkCommandBuffer commandBuffer,
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boxContext->graphicsPipeline);

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

    VkBuffer vertexBuffers[] = {boxContext->instBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, boxContext->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(commandBuffer, boxContext->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       resolution.bufferInfo.offset, resolution.size(), resolution.data);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(rectangleArraySize), 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void
cleanupRectangle(BoxContext* boxContext, VkDevice device)
{
    vkDestroyBuffer(device, boxContext->indexBuffer, nullptr);
    vkFreeMemory(device, boxContext->indexMemoryBuffer, nullptr);

    vkDestroyBuffer(device, boxContext->instBuffer, nullptr);
    vkFreeMemory(device, boxContext->instMemoryBuffer, nullptr);

    vkDestroyPipeline(device, boxContext->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, boxContext->pipelineLayout, nullptr);
}

void
mapRectanglesToBuffer(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(BoxInstance) * boxContext->numInstances;
    if (bufferSize > boxContext->instBufferSize)
    {
        vkDestroyBuffer(device, boxContext->instBuffer, nullptr);
        vkFreeMemory(device, boxContext->instMemoryBuffer, nullptr);

        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     boxContext->instBuffer, boxContext->instMemoryBuffer);
    }

    // TODO: Consider using vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges instead of
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for performance.

    void* data;
    vkMapMemory(device, boxContext->instMemoryBuffer, 0, bufferSize, 0, &data);
    MemoryCopy(data, boxContext->boxInstances.data, (size_t)bufferSize);
    vkUnmapMemory(device, boxContext->instMemoryBuffer);

    boxContext->instBufferSize = bufferSize;
}

u64
InstanceBufferFromBoxes(LinkedList<Box>* boxList, Array<BoxInstance> outBuffer)
{
    u64 numInstances = 0;
    for (LLItem<Box>* boxItem = boxList->start; boxItem != nullptr; boxItem = boxItem->next)
    {
        boxItem->item.instanceOffset = numInstances;
        LinkedList<BoxInstance>* boxInstanceList = boxItem->item.boxInstanceList;
        for (LLItem<BoxInstance>* instance = boxInstanceList->start; instance != nullptr;
             instance = instance->next)
        {
            outBuffer[numInstances] = instance->item;
            numInstances++;
        }
        boxItem->item.instanceCount = numInstances - boxItem->item.instanceOffset;
    }
    return numInstances;
}