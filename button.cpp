#include "assert.h"
#include "base/error.hpp"
#include "entrypoint.hpp"
#include "fonts.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

void
AddButton(Arena* arena, Box* box, VkExtent2D swapChainExtent, Font* font, const Vec2<f32> pos,
          const Vec2<f32> dim, const Vec3 color, const std::string text, f32 softness,
          f32 borderThickness, f32 cornerRadius)
{
    assert(dim > 0 && dim < 1);
    assert(pos > 0 && pos < 1);

    Vec2<u32> screenDimensions = {swapChainExtent.width, swapChainExtent.height};

    Vec2<f32> textDimInPixel = calculateTextDimensions(font, text);
    Vec2<f32> textDimNorm = textDimInPixel / (Vec2<f32>)screenDimensions;

    Vec2 diffDim = dim - textDimNorm;

    // TODO: border of the rectangle should be taken into account
    ASSERT(diffDim.x > 0 && diffDim.y > 0, "Text cannot be contained in the button");

    Vec2 posOffset = pos + diffDim / 2.0f;

    Vec2 posOffsetInPixel = posOffset * (Vec2<f32>)screenDimensions;
    addText(arena, font, text, posOffsetInPixel.x, posOffsetInPixel.y);

    Vec2 bottomCorner = pos + dim;
    BoxInstance* boxInstance = LinkedListPushItem<BoxInstance>(arena, box->boxInstanceList);

    boxInstance->pos1 = glm::vec2(pos.x, pos.y);
    boxInstance->pos2 = glm::vec2(bottomCorner.x, bottomCorner.y);
    boxInstance->color = glm::vec3(color.x, color.y, color.z);
    boxInstance->softness = softness;
    boxInstance->borderThickness = borderThickness;
    boxInstance->cornerRadius = cornerRadius;
}