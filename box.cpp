#include "box.hpp"
#include <cstring>

Box g_Box = {.next = &g_Box};
BoxInstance g_BoxInstance = {.next = &g_BoxInstance};

void
BoxIndexBufferCreate(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device,
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
BoxRenderPassBegin(BoxContext* boxContext, VulkanContext* vulkanContext, u32 imageIndex,
                   u32 currentFrame)
{
    VkExtent2D swapChainExtent = vulkanContext->swapChainExtent;
    VkCommandBuffer commandBuffer = vulkanContext->commandBuffers[currentFrame];
    Vulkan_PushConstantInfo pushContextInfo = vulkanContext->resolutionInfo;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanContext->boxRenderPass;
    renderPassInfo.framebuffer = vulkanContext->swapChainFramebuffers[imageIndex];
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
    f32 resolutionData[2] = {(f32)swapChainExtent.width, (f32)swapChainExtent.height};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, boxContext->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(commandBuffer, boxContext->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                       pushContextInfo.offset, pushContextInfo.size, resolutionData);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(boxContext->numInstances), 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void
BoxCleanup(BoxContext* boxContext, VkDevice device)
{
    vkDestroyBuffer(device, boxContext->indexBuffer, nullptr);
    vkFreeMemory(device, boxContext->indexMemoryBuffer, nullptr);

    vkDestroyBuffer(device, boxContext->instBuffer, nullptr);
    vkFreeMemory(device, boxContext->instMemoryBuffer, nullptr);

    vkDestroyPipeline(device, boxContext->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, boxContext->pipelineLayout, nullptr);
}

void
InstanceBufferFillFromBoxes(BoxContext* boxContext, VkPhysicalDevice physicalDevice,
                            VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(Vulkan_BoxInstance) * boxContext->numInstances;
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
InstanceBufferFromBoxes(Box* boxList, Array<Vulkan_BoxInstance> outBuffer)
{
    u64 numInstances = 0;
    for (Box* boxItem = boxList; !CheckEmpty(boxItem, &g_Box); boxItem = boxItem->next)
    {
        boxItem->instanceOffset = numInstances;
        BoxInstance* boxInstanceList = boxItem->boxInstanceList;
        for (BoxInstance* instance = boxInstanceList; !CheckEmpty(instance, &g_BoxInstance);
             instance = instance->next)
        {
            outBuffer[numInstances].pos0 = (glm::vec2)(instance->pos0);
            outBuffer[numInstances].pos1 = (glm::vec2)(instance->pos1);
            outBuffer[numInstances].color =
                glm::vec4(instance->color.data[0], instance->color.data[1], instance->color.data[2],
                          instance->color.data[3]);
            outBuffer[numInstances].borderThickness = instance->borderThickness;
            outBuffer[numInstances].cornerRadius = instance->cornerRadius;
            outBuffer[numInstances].softness = instance->softness;
            outBuffer[numInstances].attributes = instance->attributes;
            numInstances++;
        }
        boxItem->instanceCount = numInstances - boxItem->instanceOffset;
    }
    return numInstances;
}