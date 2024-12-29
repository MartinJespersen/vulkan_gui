#include "fonts.hpp"
#include "entrypoint.hpp"
#include "vulkan_helpers.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_core.h>

void
beginGlyphAtlasRenderPass(GlyphAtlas* glyphAtlas, std::vector<VkCommandBuffer> commandBuffer,
                          VkFramebuffer swapChainFramebuffer, VkExtent2D swapChainExtent,
                          VkRenderPass renderPass, Vulkan_Resolution resolution, u32 currentFrame)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      glyphAtlas->graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer[currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer[currentFrame], 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {glyphAtlas->glyphInstBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer[currentFrame], 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer[currentFrame], glyphAtlas->glyphIndexBuffer, 0,
                         VK_INDEX_TYPE_UINT16);

    for (LLItem<Font>* fontLI = glyphAtlas->fonts->start; fontLI != nullptr; fontLI = fontLI->next)
    {
        vkCmdBindDescriptorSets(commandBuffer[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                glyphAtlas->pipelineLayout, 0, 1,
                                &fontLI->item.descriptorSets[currentFrame], 0, nullptr);

        vkCmdPushConstants(commandBuffer[currentFrame], glyphAtlas->pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, resolution.bufferInfo.offset,
                           resolution.size(), resolution.data);

        vkCmdDrawIndexed(commandBuffer[currentFrame], 6, (u32)(fontLI->item.instanceCount), 0, 0,
                         (u32)(fontLI->item.instanceOffset));
    }

    vkCmdEndRenderPass(commandBuffer[currentFrame]);
}

bool
FontExists(LinkedList<Font>* fonts, u32 fontSize, Font** font)
{
    for (LLItem<Font>* fontItem = fonts->start; fontItem != nullptr; fontItem = fontItem->next)
    {
        if (fontItem->item.fontSize == fontSize)
        {
            (*font) = &fontItem->item;
            return true;
        }
    }
    (*font) = nullptr;
    return false;
}

Vec2<float>
calculateTextDimensions(Font* font, std::string text)
{
    Vec2<float> dimensions = {0.0, 0.0};
    u64 len = (u64)text.size();

    for (u32 i = 0; i < len; i++)
    {
        if ((u32)text[i] >= MAX_GLYPHS)
        {
            exitWithError("Character not supported!");
        }

        Character* ch = &font->characters[(u64)text[i]];
        dimensions.x += (f32)(ch->advance >> 6);
        dimensions.y = std::max((float)dimensions.y, ch->height);
    }

    return dimensions;
}

void
addText(Arena* arena, Font* font, std::string text, Vec2<f32> offset, Vec2<f32> pos0,
        Vec2<f32> pos1, f32 textHeight)
{
    // find largest bearing to find origin
    f32 largestBearingY = 0;
    for (u32 textIndex = 0; textIndex < text.size(); textIndex++)
    {
        Character& ch = font->characters[((u64)text[textIndex])];
        if (ch.bearingY > largestBearingY)
        {
            largestBearingY = ch.bearingY;
        }
    }

    f32 xOrigin = offset.x;
    f32 yOrigin = offset.y + largestBearingY;

    for (u32 i = 0; i < text.size(); i++)
    {
        if ((u32)text[i] >= MAX_GLYPHS)
        {
            exitWithError("Character not supported!");
        }

        Character& ch = font->characters[((u64)text[i])];

        f32 xGlyphPos0 = xOrigin + ch.bearingX;
        f32 yGlyphPos0 = yOrigin - ch.bearingY;

        f32 xPosOffset0 = std::max(pos0.x - xGlyphPos0, 0.0f);

        f32 xpos0 = std::clamp(xGlyphPos0, pos0.x, pos1.x);
        f32 ypos0 = std::clamp(yGlyphPos0, pos0.y, pos1.y);
        f32 xpos1 = std::clamp(xGlyphPos0 + ch.width, pos0.x, pos1.x);
        f32 ypos1 = std::clamp(yGlyphPos0 + ch.height, pos0.y, pos1.y);

        f32 yPosOffset0 =
            std::max(-(largestBearingY - ch.bearingY) + ((textHeight - (ypos1 - ypos0)) / 2), 0.0f);

        GlyphInstance* glyphInstance = LinkedListPushItem(arena, font->instances);
        glyphInstance->pos0 = {xpos0, ypos0};
        glyphInstance->pos1 = {xpos1, ypos1};
        glyphInstance->glyphOffset = {(f32)ch.glyphOffset + xPosOffset0, yPosOffset0};

        xOrigin += f32(ch.advance >> 6);
    }
}

void
addTexts(Arena* arena, Font* font, Text* texts, size_t len, Vec2<f32> min, Vec2<f32> max)
{
    for (size_t i = 0; i < len; i++)
    {
        Vec2<f32> textHeight = calculateTextDimensions(font, texts[i].text);
        addText(arena, font, texts[i].text, Vec2<f32>(texts[i].x, texts[i].y), min, max,
                textHeight.y);
    }
}

u64
InstanceBufferFromFontBuffers(Array<GlyphInstance> outBuffer, LinkedList<Font>* fonts)
{
    u64 numInstances = 0;
    for (LLItem<Font>* fontItem = fonts->start; fontItem != nullptr; fontItem = fontItem->next)
    {
        fontItem->item.instanceOffset = numInstances;
        LinkedList<GlyphInstance>* glyphInstanceList = fontItem->item.instances;
        for (LLItem<GlyphInstance>* instance = glyphInstanceList->start; instance != nullptr;
             instance = instance->next)
        {
            outBuffer[numInstances] = instance->item;
            numInstances++;
        }
        fontItem->item.instanceCount = numInstances - fontItem->item.instanceOffset;
    }
    return numInstances;
}

void
mapGlyphInstancesToBuffer(GlyphAtlas* glyphAtlas, VkPhysicalDevice physicalDevice, VkDevice device,
                          VkQueue queue)
{
    // Create the buffer for the text instances
    VkDeviceSize bufferSize = sizeof(GlyphInstance) * glyphAtlas->numInstances;
    if (bufferSize > glyphAtlas->glyphInstBufferSize)
    {
        vkQueueWaitIdle(queue);
        vkFreeMemory(device, glyphAtlas->glyphMemoryBuffer, nullptr);
        vkDestroyBuffer(device, glyphAtlas->glyphInstBuffer, nullptr);

        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     glyphAtlas->glyphInstBuffer, glyphAtlas->glyphMemoryBuffer);
    }

    // TODO: Consider using vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges instead
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for performance.

    void* data;
    vkMapMemory(device, glyphAtlas->glyphMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, &glyphAtlas->glyphInstanceBuffer[0], (size_t)bufferSize);
    vkUnmapMemory(device, glyphAtlas->glyphMemoryBuffer);

    glyphAtlas->glyphInstBufferSize = bufferSize;
}

void
createGlyphIndexBuffer(GlyphAtlas* glyphAtlas, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(glyphAtlas->indices[0]) * glyphAtlas->indices.size();

    createBuffer(
        physicalDevice, device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // these are not the most performant flags
        glyphAtlas->glyphIndexBuffer, glyphAtlas->glyphIndexMemoryBuffer);
    // TODO: Consider using staging buffer that copies to the glyphIndexBuffer with
    // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

    void* data;
    vkMapMemory(device, glyphAtlas->glyphIndexMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, glyphAtlas->indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, glyphAtlas->glyphIndexMemoryBuffer);
}

void
FontInit(Arena* arena, Font* outFont, u32 fontSize, u32 numCharacter)
{
    outFont->fontSize = fontSize;
    outFont->characters = ArrayAlloc<Character>(arena, numCharacter);
}

unsigned char*
initGlyphs(Arena* arena, Font* font, u32* width, u32* height)
{
    Array<Character> charArr = font->characters;
    FT_Library ft;
    FT_Face face;
    if (FT_Init_FreeType(&ft))
    {
        exitWithError("failed to init freetype library!");
    }

    int error;
    error = FT_New_Face(ft, "fonts/Roboto-Black.ttf", 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        exitWithError("failed to load font as the file format is unknown!");
    }
    else if (error)
    {
        exitWithError("failed to load font file!");
    }

    FT_Set_Pixel_Sizes(face, 0, font->fontSize);

    *width = 0;
    *height = 0;

    for (u64 i = 0; i < charArr.capacity; i++)
    {
        u64 c = static_cast<u64>(i);
        if (FT_Load_Char(face, c, FT_LOAD_DEFAULT))
        {
            std::cerr << "Failed to load glyph for character " << c << std::endl;
            continue; // Skip this character and continue with the next
        }

        *width += face->glyph->bitmap.width;
        *height = std::max(*height, face->glyph->bitmap.rows);

        charArr[c].width = static_cast<f32>(face->glyph->bitmap.width);
        charArr[c].height = static_cast<f32>(face->glyph->bitmap.rows);
        charArr[c].bearingX = static_cast<f32>(face->glyph->bitmap_left);
        charArr[c].bearingY = static_cast<f32>(face->glyph->bitmap_top);
        charArr[c].advance = static_cast<u32>(face->glyph->advance.x);
    }
    unsigned char* glyphBuffer;
    glyphBuffer = PushArray(arena, u8, (*width) * (*height));
    u32 glyphOffset = 0;
    for (int i = 0; i < MAX_GLYPHS; i++)
    {
        u64 curChar = static_cast<u64>(i);
        if (FT_Load_Char(face, curChar, FT_LOAD_RENDER))
        {
            exitWithError("failed to load glyph!");
        }
        for (unsigned int j = 0; j < face->glyph->bitmap.rows; j++)
        {
            for (unsigned int k = 0; k < face->glyph->bitmap.width; k++)
            {
                glyphBuffer[j * (*width) + k + glyphOffset] =
                    face->glyph->bitmap.buffer[k + j * face->glyph->bitmap.width];
            }
        }
        charArr[curChar].glyphOffset = glyphOffset;
        glyphOffset += face->glyph->bitmap.width;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return glyphBuffer;
}

void
cleanupFontResources(GlyphAtlas* glyphAtlas, VkDevice device)
{
    for (LLItem<Font>* fontLI = glyphAtlas->fonts->start; fontLI != nullptr; fontLI = fontLI->next)
    {
        vkFreeMemory(device, fontLI->item.textureImageMemory, nullptr);
        vkDestroyImage(device, fontLI->item.textureImage, nullptr);
        vkDestroyImageView(device, fontLI->item.textureImageView, nullptr);
        vkDestroySampler(device, fontLI->item.textureSampler, nullptr);
    }

    vkDestroyPipeline(device, glyphAtlas->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, glyphAtlas->pipelineLayout, nullptr);

    vkDestroyBuffer(device, glyphAtlas->glyphInstBuffer, nullptr);
    vkFreeMemory(device, glyphAtlas->glyphMemoryBuffer, nullptr);
    vkDestroyBuffer(device, glyphAtlas->glyphIndexBuffer, nullptr);
    vkFreeMemory(device, glyphAtlas->glyphIndexMemoryBuffer, nullptr);

    vkDestroyDescriptorPool(device, glyphAtlas->descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, glyphAtlas->descriptorSetLayout, nullptr);
}

void
createGlyphAtlasImage(Arena* arena, Font* font, VkPhysicalDevice physicalDevice, VkDevice device,
                      VkCommandPool commandPool, VkQueue graphicsQueue)
{
    ArenaTemp scratchArena = ArenaTempBegin(arena);
    u32 texWidth, texHeight;
    unsigned char* pixels = initGlyphs(scratchArena.arena, font, &texWidth, &texHeight);
    VkDeviceSize imageSize = (u32)texWidth * (u32)texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(
        device, stagingBufferMemory, 0, imageSize, 0,
        &data); // maps the buffer memory into the application address space, so cpu can access
    memcpy(data, pixels, static_cast<size_t>(imageSize)); // copy the pixel data to the buffer
    vkUnmapMemory(device,
                  stagingBufferMemory); // unmap the buffer memory so cpu no longer has access to

    createImage(physicalDevice, device, (u32)texWidth, (u32)texHeight, VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, font->textureImage, font->textureImageMemory);
    transitionImageLayout(commandPool, device, graphicsQueue, font->textureImage,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(commandPool, device, graphicsQueue, stagingBuffer, font->textureImage,
                      static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(commandPool, device, graphicsQueue, font->textureImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    ArenaTempEnd(scratchArena);
}

void
createGlyphAtlasImageView(Font* font, VkDevice device)
{
    font->textureImageView = createImageView(device, font->textureImage, VK_FORMAT_R8_UNORM);
}

void
createGlyphAtlasTextureSampler(Font* font, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.unnormalizedCoordinates = VK_TRUE;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    // anisotropic filtering is a optional feature and has to be enabled in the device features
    samplerInfo.maxAnisotropy =
        properties.limits
            .maxSamplerAnisotropy; // this is the maximum amount of texel samples that can be

    // to calculate the final color of a texture
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable =
        VK_FALSE; // if true texels will be compared to a value and result used in filtering
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // for mipmapping
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // The sampler is not attached to a image so it can be applied to any image
    if (vkCreateSampler(device, &samplerInfo, nullptr, &font->textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void
createFontDescriptorSets(Font* font, VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout, VkDevice device,
                         const u32 MAX_FRAMES_IN_FLIGHT, Array<VkDescriptorSet> descriptorSets)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorImageInfo imageInfo = {};

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = font->textureImageView;
    imageInfo.sampler = font->textureSampler;

    for (size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; frameIndex++)
    {
        VkWriteDescriptorSet descriptorWrites = {};

        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.dstSet = descriptorSets.data[frameIndex];
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr);
    }
}

void
createFontDescriptorPool(VkDevice device, const u32 MAX_FRAMES_IN_FLIGHT,
                         VkDescriptorPool& descriptorPool)
{
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void
createFontDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout)
{
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}