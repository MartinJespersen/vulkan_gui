#include "assert.h"
#include "entrypoint.hpp"
#include "fonts.hpp"
#include "vertex.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

void
AddButton(Context& context, const Vec2<f32> pos, const Vec2<f32> dim, const Vec3 color,
          const std::string text)
{
    assert(dim > 0 && dim < 1);
    assert(pos > 0 && pos < 1);

    GlyphAtlas& glyphAtlas = *context.glyphAtlas;
    GUI_Rectangle& rect = *context.rect;
    VkExtent2D swapChainExtent = context.vulkanContext->swapChainExtent;

    Vec2<u32> screenDimensions = {swapChainExtent.width, swapChainExtent.height};

    Vec2<f32> textDimInPixel = calculateTextDimensions(context, text);
    Vec2<f32> textDimNorm = textDimInPixel / (Vec2<f32>)screenDimensions;

    Vec2 diffDim = dim - textDimNorm;

    // TODO: border of the rectangle should be taken into account
    ASSERT(diffDim.x > 0 && diffDim.y > 0, "Text cannot be contained in the button");

    Vec2 posOffset = pos + diffDim / 2.0f;

    Vec2 posOffsetInPixel = posOffset * (Vec2<f32>)screenDimensions;
    addText(glyphAtlas, text, posOffsetInPixel.x, posOffsetInPixel.y);

    Vec2 bottomCorner = pos + dim;
    rect.rectangleInstances.pushBack(
        RectangleInstance{.pos1 = glm::vec2(pos.x, pos.y),
                          .pos2 = glm::vec2(bottomCorner.x, bottomCorner.y),
                          .color = glm::vec3(color.x, color.y, color.z)});
}