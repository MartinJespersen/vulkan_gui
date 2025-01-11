#include "assert.h"
#include "base/error.hpp"
#include "entrypoint.hpp"
#include "fonts.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

void
AddButton(String8 widgetName, UI_State* uiState, Arena* arena, Box* box, VkExtent2D swapChainExtent,
          Font* font, const F32Vec4 color, const std::string text, f32 softness,
          f32 borderThickness, f32 cornerRadius, UI_IO* io, F32Vec4 positions, UI_WidgetFlags flags)
{
    UI_Key key = UI_Key_Calculate(widgetName);
    UI_Widget* widget = UI_Widget_FromKey(uiState, key);
    widget->flags = flags;
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

    BoxInstance* boxInstance = PushStruct(arena, BoxInstance);
    StackPush(box->boxInstanceList, boxInstance);

    // reacting to last frame input
    boxInstance->pos0 = pos0Norm;
    boxInstance->pos1 = pos1Norm;
    boxInstance->color = color;
    boxInstance->softness = softness;
    boxInstance->borderThickness = borderThickness;
    boxInstance->cornerRadius = cornerRadius;
    boxInstance->attributes = 0;
    if (widget->flags & UI_WidgetFlag_Clickable)
    {
        if (widget->hot_t)
        {
            boxInstance->attributes |= BoxAttributes::HOT;
            if (widget->active_t)
            {
                boxInstance->attributes |= BoxAttributes::ACTIVE;
            }
        }
        // Setting action for next render pass

        widget->active_t = false;
        widget->hot_t = false;
        if (io->mousePosition >= pos0PxActual && io->mousePosition <= pos1PxActual)
        {
            widget->hot_t = true;
            if (io->leftClicked)
            {
                widget->active_t = true;
            }
        }
    }
}