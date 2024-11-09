
#include "fonts.hpp"
#include "entrypoint.hpp"
#include "vulkan_helpers.hpp"
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan_core.h>

void
beginGlyphAtlasRenderPass(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                          VkCommandBuffer commandBuffer, VkFramebuffer swapChainFramebuffer,
                          VkExtent2D swapChainExtent, VkDescriptorSet descriptorSet,
                          VkRenderPass renderPass, Vulkan_Resolution resolution)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vulkanGlyphAtlas.graphicsPipeline);

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

    VkBuffer vertexBuffers[] = {vulkanGlyphAtlas.glyphInstBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, vulkanGlyphAtlas.glyphIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkanGlyphAtlas.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdPushConstants(commandBuffer, vulkanGlyphAtlas.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       resolution.bufferInfo.offset, resolution.size(), resolution.data);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(glyphAtlas.glyphInstances.size), 0, 0,
                     0);

    vkCmdEndRenderPass(commandBuffer);
}

Vec2<float>
calculateTextDimensions(Context& context, std::string text)
{
    GlyphAtlas& glyphAtlas = *context.glyphAtlas;

    Vec2<float> dimensions = {0.0, 0.0};
    u64 len = (u64)text.size();

    for (u32 i = 0; i < len; i++)
    {
        if ((u32)text[i] >= MAX_GLYPHS)
        {
            throw std::runtime_error("Character not supported!");
        }

        Character& ch = glyphAtlas.characters.at((u64)text[i]);
        dimensions.x += (f32)(ch.advance >> 6);
        dimensions.y = std::max((float)dimensions.y, ch.height);
    }

    return dimensions;
}

void
addText(GlyphAtlas& glyphAtlas, std::string text, float x, float y)
{
    // find largest bearing to find origin
    f32 largestBearingY = 0;
    for (u32 textIndex = 0; textIndex < text.size(); textIndex++)
    {
        Character& ch = glyphAtlas.characters.at((u64)text[textIndex]);
        if (ch.bearingY > largestBearingY)
        {
            largestBearingY = ch.bearingY;
        }
    }
    f32 xOrigin = x;
    f32 yOrigin = y + largestBearingY;

    for (u32 i = 0; i < text.size(); i++)
    {
        if ((u32)text[i] >= MAX_GLYPHS)
        {
            throw std::runtime_error("Character not supported!");
        }

        Character& ch = glyphAtlas.characters.at((u64)text[i]);
        float xpos = xOrigin + ch.bearingX;
        float ypos = yOrigin - ch.bearingY;

        GlyphBuffer tempGlyph = {
            {xpos, ypos}, {ch.width + xpos, ch.height + ypos}, (f32)ch.glyphOffset};

        if (glyphAtlas.glyphInstances.pushBack(tempGlyph))
        {
            throw std::runtime_error("Failed to add glyph to buffer!");
        }
        xOrigin +=
            f32(ch.advance >> 6); // TODO: Consider what will happen x grows outside the screen
    }
}

void
addTexts(GlyphAtlas& glyphAtlas, Text* texts, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        addText(glyphAtlas, texts[i].text, texts[i].x, texts[i].y);
    }
}

void
mapGlyphInstancesToBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                          VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue)
{
    // Create the buffer for the text instances
    VkDeviceSize bufferSize = sizeof(GlyphBuffer) * glyphAtlas.glyphInstances.size;
    if (bufferSize > vulkanGlyphAtlas.glyphInstBufferSize)
    {
        vkQueueWaitIdle(queue);
        vkFreeMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer, nullptr);
        vkDestroyBuffer(device, vulkanGlyphAtlas.glyphInstBuffer, nullptr);

        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vulkanGlyphAtlas.glyphInstBuffer, vulkanGlyphAtlas.glyphMemoryBuffer);
    }

    // TODO: Consider using vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges instead
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for performance.

    void* data;
    vkMapMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, glyphAtlas.glyphInstances.data, (size_t)bufferSize);
    vkUnmapMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer);

    vulkanGlyphAtlas.glyphInstBufferSize = bufferSize;
}

void
createGlyphIndexBuffer(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                       VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(glyphAtlas.indices[0]) * glyphAtlas.indices.size();

    createBuffer(
        physicalDevice, device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // these are not the most performant flags
        vulkanGlyphAtlas.glyphIndexBuffer, vulkanGlyphAtlas.glyphIndexMemoryBuffer);
    // TODO: Consider using staging buffer that copies to the glyphIndexBuffer with
    // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

    void* data;
    vkMapMemory(device, vulkanGlyphAtlas.glyphIndexMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, glyphAtlas.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, vulkanGlyphAtlas.glyphIndexMemoryBuffer);
}

// // Things to do:
// // 1. create the buffer in 2D (so the with and height should be found ) this is to use the buffer
// as
// // a texture
unsigned char*
initGlyphs(GlyphAtlas& glyphAtlas, u32* width, u32* height)
{
    FT_Library ft;
    FT_Face face;
    if (FT_Init_FreeType(&ft))
    {
        throw std::runtime_error("failed to init freetype library!");
    }

    int error;
    error = FT_New_Face(ft, "fonts/Roboto-Black.ttf", 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        throw std::runtime_error("failed to load font as the file format is unknown!");
    }
    else if (error)
    {
        throw std::runtime_error("failed to load font file!");
    }

    FT_Set_Pixel_Sizes(face, 0, 40);

    unsigned char* glyphBuffer;

    *width = 0;
    *height = 0;

    for (int i = 0; i < MAX_GLYPHS; i++)
    {
        u64 c = static_cast<u64>(i);
        if (FT_Load_Char(face, c, FT_LOAD_DEFAULT))
        {
            std::cerr << "Failed to load glyph for character " << c << std::endl;
            continue; // Skip this character and continue with the next
        }

        *width += face->glyph->bitmap.width;
        *height = std::max(*height, face->glyph->bitmap.rows);

        glyphAtlas.characters[c].width = static_cast<f32>(face->glyph->bitmap.width);
        glyphAtlas.characters[c].height = static_cast<f32>(face->glyph->bitmap.rows);
        glyphAtlas.characters[c].bearingX = static_cast<f32>(face->glyph->bitmap_left);
        glyphAtlas.characters[c].bearingY = static_cast<f32>(face->glyph->bitmap_top);
        glyphAtlas.characters[c].advance = static_cast<u32>(face->glyph->advance.x);
    }

    glyphBuffer = new unsigned char[(*width) * (*height)];
    u32 glyphOffset = 0;
    for (int i = 0; i < MAX_GLYPHS; i++)
    {
        u64 curChar = static_cast<u64>(i);
        if (FT_Load_Char(face, curChar, FT_LOAD_RENDER))
        {
            throw std::runtime_error("failed to load glyph!");
        }
        for (unsigned int j = 0; j < face->glyph->bitmap.rows; j++)
        {
            for (unsigned int k = 0; k < face->glyph->bitmap.width; k++)
            {
                glyphBuffer[j * (*width) + k + glyphOffset] =
                    face->glyph->bitmap.buffer[k + j * face->glyph->bitmap.width];
            }
        }
        glyphAtlas.characters[curChar].glyphOffset = glyphOffset;
        glyphOffset += face->glyph->bitmap.width;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return glyphBuffer;
}

void
cleanupFontResources(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkDevice device)
{
    vkFreeMemory(device, vulkanGlyphAtlas.textureImageMemory, nullptr);
    vkDestroyImage(device, vulkanGlyphAtlas.textureImage, nullptr);
    vkDestroyImageView(device, vulkanGlyphAtlas.textureImageView, nullptr);
    vkDestroySampler(device, vulkanGlyphAtlas.textureSampler, nullptr);

    vkDestroyPipeline(device, vulkanGlyphAtlas.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, vulkanGlyphAtlas.pipelineLayout, nullptr);

    vkDestroyBuffer(device, vulkanGlyphAtlas.glyphInstBuffer, nullptr);
    vkFreeMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer, nullptr);
    vkDestroyBuffer(device, vulkanGlyphAtlas.glyphIndexBuffer, nullptr);
    vkFreeMemory(device, vulkanGlyphAtlas.glyphIndexMemoryBuffer, nullptr);

    vkDestroyDescriptorPool(device, vulkanGlyphAtlas.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, vulkanGlyphAtlas.descriptorSetLayout, nullptr);
}

void
createGlyphAtlasImage(Vulkan_GlyphAtlas& vulkanGlyphAtlas, GlyphAtlas& glyphAtlas,
                      VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool,
                      VkQueue graphicsQueue)
{
    u32 texWidth, texHeight;
    unsigned char* pixels = initGlyphs(glyphAtlas, &texWidth, &texHeight);
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

    delete[] pixels;

    createImage(physicalDevice, device, (u32)texWidth, (u32)texHeight, VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanGlyphAtlas.textureImage,
                vulkanGlyphAtlas.textureImageMemory);
    transitionImageLayout(commandPool, device, graphicsQueue, vulkanGlyphAtlas.textureImage,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(commandPool, device, graphicsQueue, stagingBuffer,
                      vulkanGlyphAtlas.textureImage, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    transitionImageLayout(commandPool, device, graphicsQueue, vulkanGlyphAtlas.textureImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void
createGlyphAtlasImageView(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkDevice device)
{
    vulkanGlyphAtlas.textureImageView =
        createImageView(device, vulkanGlyphAtlas.textureImage, VK_FORMAT_R8_UNORM);
}

void
createGlyphAtlasTextureSampler(Vulkan_GlyphAtlas& vulkanGlyphAtlas, VkPhysicalDevice physicalDevice,
                               VkDevice device)
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
    if (vkCreateSampler(device, &samplerInfo, nullptr, &vulkanGlyphAtlas.textureSampler) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void
createFontDescriptorSets(VkImageView imageView, VkSampler sampler, VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout descriptorSetLayout, VkDevice device,
                         const u32 MAX_FRAMES_IN_FLIGHT,
                         StaticArray<VkDescriptorSet>& descriptorSets)
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

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets.data[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
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