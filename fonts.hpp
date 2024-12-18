#pragma once
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
}

#include "entrypoint.hpp"
#include <glm/glm.hpp>

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
addTexts(Arena* arena, Font* font, Text* texts, size_t len, u32 fontSize);
void
addText(Arena* arena, Font* font, std::string text, float x, float y);
void
beginGlyphAtlasRenderPass(GlyphAtlas& glyphAtlas, VkCommandBuffer commandBuffer,
                          VkFramebuffer swapChainFramebuffer, VkExtent2D swapChainExtent,
                          VkRenderPass renderPass, Vulkan_Resolution resolution);

void
createGlyphAtlasImage(Font* font, VkPhysicalDevice physicalDevice, VkDevice device,
                      VkCommandPool commandPool, VkQueue graphicsQueue, u32 fontSize);
void
createGlyphAtlasImageView(Font* font, VkDevice device);

void
createFontDescriptorSets(Font*, VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout, VkDevice device,
                         const u32 MAX_FRAMES_IN_FLIGHT,
                         StaticArray<VkDescriptorSet>& descriptorSets);

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
FontExists(LinkedList<Font>* fonts, u32 fontSize, Font** font);

u64
InstanceBufferFromFontBuffers(Array<GlyphInstance> outBuffer, LinkedList<Font>* fonts);