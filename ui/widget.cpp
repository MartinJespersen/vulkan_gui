UI_Widget*
UI_Widget_FromKey(UI_State* uiState, UI_Key key)
{
    UI_Widget* result = {0};
    u64 slotKey = key.key % uiState->widgetCacheSize;
    UI_WidgetSlot* slot = &uiState->widgetSlot[slotKey];

    for (UI_Widget* widget = slot->first; !CheckNull(widget); widget = widget->hashNext)
    {
        if (UI_Key_IsEqual(widget->key, key))
        {
            result = widget;
            break;
        }
    }

    if (CheckNull(result))
    {
        result = UI_WidgetSlot_Push(uiState, key);
    }
    return result;
}

UI_Widget*
UI_Widget_Allocate(UI_State* uiState)
{
    UI_Widget* widget = uiState->freeList;
    if (uiState->freeListSize > 0)
    {
        StackPop(uiState->freeList);
        MemoryZeroStruct(widget);
        uiState->freeListSize -= 1;
    }
    else
    {
        widget = PushStruct(uiState->arena, UI_Widget);
    }
    return widget;
}

// UI_WidgetSlot
UI_Widget*
UI_WidgetSlot_Push(UI_State* uiState, UI_Key key)
{
    UI_WidgetSlot* slot = &uiState->widgetSlot[key.key % uiState->widgetCacheSize];
    UI_Widget* widget = UI_Widget_Allocate(uiState);

    DLLPushBack_NPZ(slot->first, slot->last, widget, hashNext, hashPrev, CheckNull, SetNull);
    widget->key = key;
    return widget;
}

// UI_Key
UI_Key
UI_Key_Calculate(String8 str)
{
    u128 hash = HashFromStr8(str);
    UI_Key key = {hash.data[1]};
    return key;
}

bool
UI_Key_IsEqual(UI_Key key0, UI_Key key1)
{
    return key0.key == key1.key;
}

bool
UI_Key_IsNull(UI_Key key)
{
    return key.key == 0;
}

// Button impl

void
AddButton(String8 widgetName, UI_State* uiState, GlyphAtlas* glyphAtlas, Arena* arena,
          BoxContext* boxContext, VkExtent2D swapChainExtent, const F32Vec4 color, String8 text,
          f32 softness, f32 borderThickness, f32 cornerRadius, UI_IO* io, F32Vec4 positions,
          UI_WidgetFlags flags, u32 fontSize)
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

    Font* font = FontFindOrCreate(glyphAtlas, fontSize);
    Vec2<f32> textDimPx = calculateTextDimensions(font, text);
    Vec2 diffDim = pos1Px - pos0Px - textDimPx;
    Vec2 glyphPos = pos0Px + diffDim / 2.0f;

    addText(arena, font, text, glyphPos, pos0PxActual, pos1PxActual, textDimPx.y);

    Box* box = PushStructZero(arena, Box);
    StackPush(boxContext->boxList, box);

    // reacting to last frame input
    box->pos0 = pos0Norm;
    box->pos1 = pos1Norm;
    box->color = color;
    box->softness = softness;
    box->borderThickness = borderThickness;
    box->cornerRadius = cornerRadius;
    box->attributes = 0;

    if (widget->flags & UI_WidgetFlag_Clickable)
    {
        if (widget->hot_t)
        {
            box->attributes |= BoxAttributes::HOT;
            if (widget->active_t)
            {
                box->attributes |= BoxAttributes::ACTIVE;
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