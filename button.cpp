#include "assert.h"
#include "base/error.hpp"
#include "entrypoint.hpp"
#include "fonts.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

UI_Comm
AddButton(String8 widgetName, UI_State* uiState, Arena* arena, Box* box, VkExtent2D swapChainExtent,
          Font* font, const Vec3 color, const std::string text, f32 softness, f32 borderThickness,
          f32 cornerRadius, UI_IO* io, F32Vec4 positions)
{
    UI_Key key = UI_Key_Calculate(widgetName);
    UI_Widget* widget = UI_Widget_FromKey(uiState, key);
    widget->rect = positions; // TODO: remove this in favor of a layout algorithm
    Vec2 pos0Norm = widget->rect.point.p0;
    Vec2 pos1Norm = widget->rect.point.p1;

    Vec2<u32> screenDimensions = {swapChainExtent.width, swapChainExtent.height};
    Vec2 pos0Px = pos0Norm * screenDimensions;
    Vec2 pos1Px = pos1Norm * screenDimensions;
    Vec2 pos0PxActual = pos0Px + borderThickness;
    Vec2 pos1PxActual = pos1Px - borderThickness;

    // Reaction to previous render pass

    Vec2<f32> textDimPx = calculateTextDimensions(font, text);
    Vec2 diffDim = pos1Px - pos0Px - textDimPx;
    Vec2 glyphPos = pos0Px + diffDim / 2.0f;

    addText(arena, font, text, glyphPos, pos0PxActual, pos1PxActual, textDimPx.y);

    BoxInstance* boxInstance = LinkedListPushItem<BoxInstance>(arena, box->boxInstanceList);

    boxInstance->pos1 = glm::vec2(pos0Norm.x, pos0Norm.y);
    boxInstance->pos2 = glm::vec2(pos1Norm.x, pos1Norm.y);
    boxInstance->color = glm::vec3(color.x, color.y, color.z);
    boxInstance->softness = softness;
    boxInstance->borderThickness = borderThickness;
    boxInstance->cornerRadius = cornerRadius;

    UI_Comm flags = 0;
    widget->active_t = false;
    widget->hot_t = false;
    if (io->mousePosition >= pos0PxActual && io->mousePosition <= pos1PxActual)
    {
        widget->active_t = true;
        if (io->leftClicked)
        {
            widget->hot_t = true;
        }
    }

    // Setting action for next render pass
    if (widget->active_t)
    {
        flags |= UI_Comm_Hovered;
        if (widget->hot_t)
        {
            flags |= UI_Comm_Clicked;
        }
    }

    return flags;
}