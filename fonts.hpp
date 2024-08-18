#pragma once
#include <glm/glm.hpp>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdexcept>
#include <algorithm>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "buffer.hpp"

struct Glyph
{
    glm::float32 width;
    glm::float32 height;
    glm::float32 bufferOffset;
};

VkBuffer glyphInstBuffer;
VkDeviceMemory glyphMemoryBuffer;
VkBuffer glyphIndexBuffer;
VkDeviceMemory glyphIndexMemoryBuffer;

std::vector<Glyph> glyphInstances;

void createGlyphInstBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
    // Create the buffer for the text instances
    VkDeviceSize bufferSize = sizeof(Glyph) * glyphInstances.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        physicalDevice,
        device,
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, glyphInstances.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(
        physicalDevice,
        device,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, glyphInstBuffer, glyphMemoryBuffer);

    copyBuffer(device, commandPool, queue, stagingBuffer, glyphInstBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

struct Character
{
    glm::ivec2 size;      // Size of glyph
    glm::ivec2 bearing;   // Offset from baseline to left/top of glyph
    unsigned int advance; // Offset to advance to next glyph
    unsigned int bufferOffset;
};

// Things to do:
// 1. create the buffer in 2D (so the with and height should be found ) this is to use the buffer as a texture
unsigned char *initGlyphs(int *width, int *height, std::map<char, Character> *characters)
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

    FT_Set_Pixel_Sizes(face, 0, 12);

    *width = 0;
    *height = 0;
    unsigned char *glyphBuffer;
    for (int i = 0; i < 126; i++)
    {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER))
        {
            throw std::runtime_error("failed to load glyph!");
        }
        *width += face->glyph->bitmap.width;
        *height = std::max(*height, (int)face->glyph->bitmap.rows);

        Character character = {
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)};
        characters->insert(std::pair<char, Character>(i, character));
    }

    glyphBuffer = new unsigned char[(*width) * (*height)];
    int bufferOffset = 0;
    for (int i = 0; i < 126; i++)
    {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER))
        {
            throw std::runtime_error("failed to load glyph!");
        }
        for (int j = 0; j < face->glyph->bitmap.rows; j++)
        {
            for (int k = 0; k < face->glyph->bitmap.width; k++)
            {
                glyphBuffer[j * (*width) + k + bufferOffset] = face->glyph->bitmap.buffer[k + j * face->glyph->bitmap.width];
            }
        }
        bufferOffset += face->glyph->bitmap.width;
        characters->at(i).bufferOffset = bufferOffset;
    }
    FT_Done_FreeType(ft);
    return glyphBuffer;
}
