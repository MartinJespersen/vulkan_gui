#pragma once
#include "vulkan_helpers.hpp"
#include <vector>
#include <vulkan/vulkan.h>

enum BoxAttributes
{
    HOT = (1 << 0),
    ACTIVE = (1 << 1)
};

struct Vulkan_BoxInstance
{
    glm::vec2 pos0;
    glm::vec2 pos1;
    glm::vec4 color;
    glm::f32 softness;
    glm::f32 borderThickness;
    glm::f32 cornerRadius;
    u32 attributes;

    static VkVertexInputBindingDescription
    getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vulkan_BoxInstance);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        return bindingDescription;
    }
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(7);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vulkan_BoxInstance, pos0);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vulkan_BoxInstance, pos1);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vulkan_BoxInstance, color);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vulkan_BoxInstance, softness);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vulkan_BoxInstance, borderThickness);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vulkan_BoxInstance, cornerRadius);

        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[6].offset = offsetof(Vulkan_BoxInstance, attributes);

        return attributeDescriptions;
    }
};

struct BoxInstance
{
    BoxInstance* next;
    Vec2<f32> pos0;
    Vec2<f32> pos1;
    F32Vec4 color;
    f32 softness;
    f32 borderThickness;
    f32 cornerRadius;
    u32 attributes;
};

struct Box
{
    Box* next;
    u64 instanceOffset;
    u64 instanceCount;
    BoxInstance* boxInstanceList;
};

struct BoxContext
{
    Array<Vulkan_BoxInstance> boxInstances;
    Box* boxList;
    u64 numInstances;

    // vulkan part
    VkBuffer instBuffer;
    VkDeviceMemory instMemoryBuffer;
    VkDeviceSize instBufferSize;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemoryBuffer;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
};

void
BoxIndexBufferCreate(BoxContext* boxContext, VkPhysicalDevice physicalDevice, VkDevice device,
                     VkCommandPool commandPool, VkQueue graphicsQueue,
                     std::vector<uint16_t> indices);

void
BoxRenderPassBegin(BoxContext* boxContext, VulkanContext* vulkanContext, u32 imageIndex,
                   u32 currentFrame);

void
BoxCleanup(BoxContext* boxContext, VkDevice device);

void
InstanceBufferFillFromBoxes(BoxContext* boxContext, VkPhysicalDevice physicalDevice,
                            VkDevice device);

u64
InstanceBufferFromBoxes(Box* boxList, Array<Vulkan_BoxInstance> outBuffer);