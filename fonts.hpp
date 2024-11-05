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
cleanupFontResources(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkDevice device);

unsigned char*
initGlyphs(GlyphAtlas* glyphAtlas, int* width, int* height);
void
createGlyphIndexBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                       VkPhysicalDevice physicalDevice, VkDevice device);
void
mapGlyphInstancesToBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                          VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue);

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

Vec2<float>
calculateTextDimensions(Context& context, std::string text);