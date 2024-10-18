#pragma once
#include "utils.hpp"
#include <ft2build.h>
#include <glm/glm.hpp>
#include <map>
#include FT_FREETYPE_H
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "vulkan_helpers.hpp"

const int MAX_GLYPHS = 126;
struct GlyphBuffer
{
    glm::vec2 pos;
    glm::vec2 size;
    glm::float32 glyphOffset;

    static VkVertexInputBindingDescription
    getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GlyphBuffer);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        return bindingDescription;
    }
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(GlyphBuffer, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(GlyphBuffer, size);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(GlyphBuffer, glyphOffset);

        return attributeDescriptions;
    }
};

struct ArrayGlyphInstance
{
    GlyphBuffer* data;
    size_t size;
    size_t capacity;

    ArrayGlyphInstance() : data(nullptr), size(0), capacity(0)
    {
    }

    inline int
    pushBack(GlyphBuffer glyph)
    {
        if (size >= capacity)
        {
            return -1;
        }
        data[size] = glyph;
        size++;

        return 0;
    }

    inline void
    reset(size_t newSize)
    {
        if (newSize > capacity)
        {
            if (data)
            {
                delete[] data;
            }
            this->capacity = newSize;
            data = new GlyphBuffer[this->capacity];
        }
        size = 0;
    }
};

struct Character
{
    float width; // Size of glyph
    float height;
    float bearingX; // Offset from baseline to left/top of glyph
    float bearingY;
    unsigned int advance; // Offset to advance to next glyph
    float glyphOffset;
};

struct Text
{
    std::string text;
    float x;
    float y;
};

struct GlyphAtlas
{
    std::map<char, Character> characters;
    ArrayGlyphInstance glyphInstances;
    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
};

struct Vulkan_GlyphAtlas
{
    VkBuffer glyphInstBuffer;
    VkDeviceMemory glyphMemoryBuffer;
    VkDeviceSize glyphInstBufferSize;
    VkBuffer glyphIndexBuffer;
    VkDeviceMemory glyphIndexMemoryBuffer;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkDescriptorPool descriptorPool;
    StaticArray<VkDescriptorSet> descriptorSets;
    VkDescriptorSetLayout descriptorSetLayout;
};

void
cleanupFontResources(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkDevice device);

unsigned char*
initGlyphs(GlyphAtlas& glyphAtlas, int* width, int* height);
void
createGlyphIndexBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                       VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool,
                       VkQueue graphicsQueue);
void
mapGlyphInstancesToBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                          VkPhysicalDevice physicalDevice, VkDevice device,
                          VkCommandPool commandPool, VkQueue queue);

void
addTexts(GlyphAtlas& glyphAtlas, Text* texts, size_t len);
void
addText(GlyphAtlas& glyphAtlas, std::string text, float x, float y);
void
beginGlyphAtlasRenderPass(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                          VkCommandBuffer commandBuffer, VkFramebuffer swapChainFramebuffer,
                          VkExtent2D swapChainExtent, VkDescriptorSet descriptorSet,
                          VkRenderPass renderPass, Vulkan_Resolution resolution);

void
createGlyphAtlasImage(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                      VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool,
                      VkQueue graphicsQueue);
void
createGlyphAtlasImageView(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkDevice device);

void
createFontDescriptorSets(VkImageView imageView, VkSampler sampler, VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout, VkDevice device,
                         const u32 MAX_FRAMES_IN_FLIGHT,
                         StaticArray<VkDescriptorSet>& descriptorSets);

void
createFontDescriptorPool(VkDevice device, const u32 MAX_FRAMES_IN_FLIGHT,
                         VkDescriptorPool& descriptorPool);

void
createFontDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout);