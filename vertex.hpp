#pragma once
#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

struct GlyphBox
{
  glm::float32 width;
  glm::float32 height;
  glm::float32 bufferOffset;
};

struct BoxInstance
{
  glm::vec2 pos1;
  glm::vec2 pos2;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(BoxInstance);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    return bindingDescription;
  }
  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(BoxInstance, pos1);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(BoxInstance, pos2);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(BoxInstance, color);

    return attributeDescriptions;
  }
};
