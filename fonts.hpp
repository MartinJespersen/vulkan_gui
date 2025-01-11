#pragma once

#include "vulkan_helpers.hpp"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

struct Character
{
    float width; // Size of glyph
    float height;
    float bearingX; // Offset from baseline to left/top of glyph
    float bearingY;
    unsigned int advance; // Offset to advance to next glyph
    u32 glyphOffset;
    char character;
};

struct Text
{
    std::string text;
    float x;
    float y;
};

struct Vulkan_GlyphInstance
{
    glm::vec2 pos0;
    glm::vec2 pos1;
    glm::vec2 glyphOffset;

    static VkVertexInputBindingDescription
    getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vulkan_GlyphInstance);
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
        attributeDescriptions[0].offset = offsetof(Vulkan_GlyphInstance, pos0);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vulkan_GlyphInstance, pos1);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vulkan_GlyphInstance, glyphOffset);

        return attributeDescriptions;
    }
};

struct GlyphInstance
{
    Vec2<f32> pos0;
    Vec2<f32> pos1;
    Vec2<f32> glyphOffset;
    GlyphInstance* next;
};

struct Font
{
    Font* next;
    Font* prev;
    u32 fontSize;
    u64 instanceOffset;
    u64 instanceCount;
    GlyphInstance* instances;
    Array<Character> characters;

    // vulkan
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    Array<VkDescriptorSet> descriptorSets;
};

struct GlyphAtlas
{
    Arena* fontArena;
    Font* fontList;
    Array<Vulkan_GlyphInstance> glyphInstanceBuffer;
    u64 numInstances;
    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

    // Vulkan part
    VkBuffer glyphInstBuffer;
    VkDeviceMemory glyphMemoryBuffer;
    VkDeviceSize glyphInstBufferSize;
    VkBuffer glyphIndexBuffer;
    VkDeviceMemory glyphIndexMemoryBuffer;

    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout descriptorSetLayout;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
};

void
cleanupFontResources(GlyphAtlas* glyphAtlas, VkDevice device);

unsigned char*
initGlyphs(Arena* arena, Font* font, u32* width, u32* height);
void
createGlyphIndexBuffer(GlyphAtlas* glyphAtlas, VkPhysicalDevice physicalDevice, VkDevice device);

void
mapGlyphInstancesToBuffer(GlyphAtlas* glyphAtlas, VkPhysicalDevice physicalDevice, VkDevice device,
                          VkQueue queue);

void
addTexts(Arena* arena, Font* font, Text* texts, size_t len, u32 fontSize, Vec2<f32> min,
         Vec2<f32> max);
void
addText(Arena* arena, Font* font, std::string text, Vec2<f32> offset, Vec2<f32> pos0,
        Vec2<f32> pos1, f32 textHeight);
void
beginGlyphAtlasRenderPass(GlyphAtlas* glyphAtlas, VulkanContext* vulkanContext, u32 imageIndex,
                          u32 currentFrame);

void
createGlyphAtlasImage(Arena* arena, Font* font, VkPhysicalDevice physicalDevice, VkDevice device,
                      VkCommandPool commandPool, VkQueue graphicsQueue, u32 fontSize);
void
createGlyphAtlasImageView(Font* font, VkDevice device);

void
createFontDescriptorSets(Font* font, VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout, VkDevice device,
                         const u32 MAX_FRAMES_IN_FLIGHT, Array<VkDescriptorSet> descriptorSets);

void
createFontDescriptorPool(VkDevice device, const u32 MAX_FRAMES_IN_FLIGHT,
                         VkDescriptorPool& descriptorPool);

void
createFontDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout);

Vec2<float>
calculateTextDimensions(Font* font, std::string text);

void
FontInit(Arena* arena, Font* outFont, u32 fontSize, u32 numCharacter);

bool
FontExists(Font* fonts, u32 fontSize, Font** font);

u64
InstanceBufferFromFontBuffers(Array<Vulkan_GlyphInstance> outBuffer, Font* fonts);