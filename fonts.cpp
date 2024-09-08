#include "fonts.hpp"
#include "vulkan_helpers.hpp"
#include <iostream>

void TextRenderPass(VkCommandBuffer commandBuffer,
                    VkFramebuffer swapChainFramebuffer,
                    VkExtent2D swapChainExtent,
                    VkDescriptorSet descriptorSet)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanGlyphAtlas.renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

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

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGlyphAtlas.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(glyphAtlas.glyphInstances.size), 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void addText(std::string text, float x, float y)
{
    for (int i = 0; i < text.size(); i++)
    {
        if (text[i] >= MAX_GLYPHS)
        {
            throw std::runtime_error("Character not supported!");
        }
        if (glyphAtlas.characters.find(text[i]) == glyphAtlas.characters.end())
        {
            throw std::runtime_error("Character not supported!");
        }
        Character &ch = glyphAtlas.characters.at(text[i]);
        float xpos = x + ch.bearingX;
        float ypos = y - ch.bearingY;

        GlyphBuffer tempGlyph = {{xpos, ypos}, {ch.width + xpos, ch.height + ypos}, ch.glyphOffset};

        if (glyphAtlas.glyphInstances.pushBack(tempGlyph))
        {
            throw std::runtime_error("Failed to add glyph to buffer!");
        }
        x += (ch.advance >> 6); // TODO: Consider what will happen x grows outside the screen
    }
}

void addTexts(Text *texts, size_t len)
{

    size_t bufferSize = 0;

    for (size_t i = 0; i < len; i++)
    {
        bufferSize += texts[i].text.size();
    }

    glyphAtlas.glyphInstances.reset(bufferSize);

    for (size_t i = 0; i < len; i++)
    {
        addText(texts[i].text, texts[i].x, texts[i].y);
    }
}

void mapGlyphInstancesToBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
    // Create the buffer for the text instances
    VkDeviceSize bufferSize = sizeof(GlyphBuffer) * glyphAtlas.glyphInstances.size;
    if (bufferSize > vulkanGlyphAtlas.glyphInstBufferSize)
    {

        vkDestroyBuffer(device, vulkanGlyphAtlas.glyphInstBuffer, nullptr);
        vkFreeMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer, nullptr);

        createBuffer(
            physicalDevice,
            device,
            bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vulkanGlyphAtlas.glyphInstBuffer, vulkanGlyphAtlas.glyphMemoryBuffer);
    }

    // TODO: Consider using vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges instead of VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for performance.

    void *data;
    vkMapMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, glyphAtlas.glyphInstances.data, (size_t)bufferSize);
    vkUnmapMemory(device, vulkanGlyphAtlas.glyphMemoryBuffer);

    vulkanGlyphAtlas.glyphInstBufferSize = bufferSize;
}

void createGlyphIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
{
    VkDeviceSize bufferSize = sizeof(glyphAtlas.indices[0]) * glyphAtlas.indices.size();

    createBuffer(physicalDevice,
                 device,
                 bufferSize,
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // these are not the most performant flags
                 vulkanGlyphAtlas.glyphIndexBuffer, vulkanGlyphAtlas.glyphIndexMemoryBuffer);
    // TODO: Consider using staging buffer that copies to the glyphIndexBuffer with VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

    void *data;
    vkMapMemory(device, vulkanGlyphAtlas.glyphIndexMemoryBuffer, 0, bufferSize, 0, &data);
    memcpy(data, glyphAtlas.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, vulkanGlyphAtlas.glyphIndexMemoryBuffer);
}

// Things to do:
// 1. create the buffer in 2D (so the with and height should be found ) this is to use the buffer as a texture
unsigned char *initGlyphs(int *width, int *height)
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

    *width = 0;
    *height = 0;
    unsigned char *glyphBuffer;
    for (int i = 0; i < MAX_GLYPHS; i++)
    {
        char c = static_cast<char>(i);
        if (FT_Load_Char(face, c, FT_LOAD_DEFAULT))
        {
            std::cerr << "Failed to load glyph for character " << c << std::endl;
            continue; // Skip this character and continue with the next
        }

        *width += face->glyph->bitmap.width;
        *height = std::max(*height, static_cast<int>(face->glyph->bitmap.rows));

        glyphAtlas.characters[c] = {(float)face->glyph->bitmap.width, (float)face->glyph->bitmap.rows,
                                    (float)face->glyph->bitmap_left, (float)face->glyph->bitmap_top,
                                    static_cast<unsigned int>(face->glyph->advance.x)};
    }

    glyphBuffer = new unsigned char[(*width) * (*height)];
    int glyphOffset = 0;
    for (int i = 0; i < 126; i++)
    {
        char curChar = static_cast<char>(i);
        if (FT_Load_Char(face, curChar, FT_LOAD_RENDER))
        {
            throw std::runtime_error("failed to load glyph!");
        }
        for (int j = 0; j < face->glyph->bitmap.rows; j++)
        {
            for (int k = 0; k < face->glyph->bitmap.width; k++)
            {
                glyphBuffer[j * (*width) + k + glyphOffset] = face->glyph->bitmap.buffer[k + j * face->glyph->bitmap.width];
            }
        }
        glyphAtlas.characters.at(curChar).glyphOffset = glyphOffset;
        glyphOffset += face->glyph->bitmap.width;
    }
    FT_Done_FreeType(ft);
    return glyphBuffer;
}

void createTextGraphicsPipeline(VkDevice device,
                                VkExtent2D swapChainExtent,

                                VkDescriptorSetLayout descriptorSetLayout,
                                VkSampleCountFlagBits msaaSamples,
                                std::string vertShaderPath,
                                std::string fragShaderPath)
{
    auto vertShaderCode = readFile(vertShaderPath);
    auto fragShaderCode = readFile(fragShaderPath);

    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
    auto bindingDescription = GlyphBuffer::getBindingDescription();
    auto attributeDescriptions = GlyphBuffer::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &vulkanGlyphAtlas.pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState; // Optional
    pipelineInfo.layout = vulkanGlyphAtlas.pipelineLayout;
    pipelineInfo.renderPass = vulkanGlyphAtlas.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &vulkanGlyphAtlas.graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void createTextRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // This property tells something about how memory is stored in buffer
                                                                   // This one is optimal for presentation

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // This one is optimal for color attachment for writing colors from fragment shader

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 1;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, colorAttachmentResolve};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &vulkanGlyphAtlas.renderPass) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void cleanupFontResources(VkDevice device)
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
    vkDestroyRenderPass(device, vulkanGlyphAtlas.renderPass, nullptr);
}

void createGlyphAtlasImage(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
{
    int texWidth, texHeight;
    unsigned char *pixels = initGlyphs(&texWidth, &texHeight);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(physicalDevice,
                 device,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,           // buffer should be used as source in a memory transfer operation
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT         // making visible to host
                     | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // coherent between the host (CPU) and the device (GPU). This means that after the host writes data to this memory, the device will see the updated data without needing an explicit flush of the memory
                 stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data); // maps the buffer memory into the application address space, so cpu can access it
    memcpy(data, pixels, static_cast<size_t>(imageSize));             // copy the pixel data to the buffer
    vkUnmapMemory(device, stagingBufferMemory);                       // unmap the buffer memory so cpu no longer has access to it

    delete pixels;

    createImage(physicalDevice,
                device,
                texWidth,
                texHeight,
                VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vulkanGlyphAtlas.textureImage,
                vulkanGlyphAtlas.textureImageMemory);
    transitionImageLayout(commandPool, device, graphicsQueue, vulkanGlyphAtlas.textureImage, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(commandPool, device, graphicsQueue, stagingBuffer, vulkanGlyphAtlas.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(commandPool, device, graphicsQueue, vulkanGlyphAtlas.textureImage, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void createGlyphAtlasImageView(VkDevice device)
{
    vulkanGlyphAtlas.textureImageView = createImageView(device, vulkanGlyphAtlas.textureImage, VK_FORMAT_R8_UNORM);
}

void createGlyphAtlasTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device)
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
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // this is the maximum amount of texel samples that can be used to calculate the final color of a texture
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable = VK_FALSE; // if true texels will be compared to a value and result used in filtering
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // for mipmapping
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // The sampler is not attached to a image so it can be applied to any image
    if (vkCreateSampler(device, &samplerInfo, nullptr, &vulkanGlyphAtlas.textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
