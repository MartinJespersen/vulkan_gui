#ifdef PROFILING_ENABLE
BufferImpl(TracyVkCtx);
#endif

inline_function void UI_Widget_TreeStateReset(UI_Widget* widget) {
    widget->first = 0;
    widget->last = 0;
    widget->next = 0;
    widget->prev = 0;
    widget->parent = 0;
}

root_function UI_Widget*
UI_Widget_FromKey(UI_State* uiState, UI_Key key)
{
    UI_Widget* result = {0};
    u64 slotKey = key.key % uiState->widgetCacheSize;
    UI_WidgetSlot* slot = &uiState->widgetSlot[slotKey];

    for (UI_Widget* widget = slot->first; !IsNull(widget); widget = widget->hashNext)
    {
        if (UI_Key_IsEqual(widget->key, key))
        {
            result = widget;
            break;
        }
    }

    if (IsNull(result))
    {
        result = UI_WidgetSlot_Push(uiState, key);
    }
    return result;
}

root_function UI_Widget*
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
root_function UI_Widget*
UI_WidgetSlot_Push(UI_State* uiState, UI_Key key)
{
    UI_WidgetSlot* slot = &uiState->widgetSlot[key.key % uiState->widgetCacheSize];
    UI_Widget* widget = UI_Widget_Allocate(uiState);

    DLLPushBack_NPZ(slot->first, slot->last, widget, hashNext, hashPrev, IsNull, SetNull);
    widget->key = key;
    return widget;
}

// UI_Key
root_function UI_Key
UI_Key_Calculate(String8 str)
{
    u128 hash = HashFromStr8(str);
    UI_Key key = {hash.data[1]};
    return key;
}

root_function bool
UI_Key_IsEqual(UI_Key key0, UI_Key key1)
{
    return key0.key == key1.key;
}

root_function bool
UI_Key_IsNull(UI_Key key)
{
    return key.key == 0;
}

// Widget implementations
inline_function b32
UI_Widget_IsEmpty(UI_Widget* widget)
{
    return widget == 0;
}

inline_function void
UI_Widget_SetEmpty(UI_Widget** widget)
{
    *widget = 0;
}

root_function UI_Widget*
UI_Widget_DepthFirstPreOrder(UI_Widget* widget)
{
    UI_Widget* next = {0};
    if (widget->first)
    {
        next = widget->first;
    }
    else
        for (UI_Widget* parent = widget; !IsNull(parent); parent = parent->parent)
        {
            if (parent->next)
            {
                next = parent->next;
                break;
            }
        }
    return next;
}

inline_function UI_Widget*
UI_Widget_TreeLeftMostFind(UI_Widget* root)
{
    UI_Widget* child = root;
    for (; !IsNull(child) && !IsNull(child->first); child = child->first)
    {
    };
    return child;
}

root_function UI_Widget*
UI_Widget_DepthFirstPostOrder(UI_Widget* widget)
{
    UI_Widget* next = {0};
    if (IsNull(widget->next))
    {
        next = widget->parent;
    }
    else
        for (next = widget->next; !IsNull(next->first); next = next->first)
        {
        }
    return next;
}

typedef u32 IterState;
enum
{
    UI_IterState_PRE,
    UI_IterState_POST,
    UI_IterState_Count
};

struct UI_WidgetDF
{
    UI_Widget* next;
    IterState iterState;
};

root_function void
UI_Widget_Add(String8 widgetName, Context* context, const F32Vec4 color, String8 text,
              f32 softness, f32 borderThickness, f32 cornerRadius, UI_WidgetFlags flags,
              u32 fontSize, UI_Size semanticSizeX, UI_Size semanticSizeY)
{
    UI_State* uiState = context->uiState;
    UI_IO* io = context->io;
    GlyphAtlas* glyphAtlas = context->glyphAtlas;

    UI_Key key = UI_Key_Calculate(widgetName);
    UI_Widget* widget = UI_Widget_FromKey(uiState, key);
    
    UI_Widget_TreeStateReset(widget);

    widget->name = widgetName;
    widget->flags = flags;
    widget->color = color;
    widget->softness = softness;
    widget->borderThickness = borderThickness;
    widget->cornerRadius = cornerRadius;
    widget->fontSize = fontSize;
    widget->active_t = false;
    widget->hot_t = false;
    widget->text = text;
    widget->semanticSize[Axis2_X] = semanticSizeX;
    widget->semanticSize[Axis2_Y] = semanticSizeY;

    if (flags & UI_WidgetFlag_DrawText) {
        widget->font = FontFindOrCreate(glyphAtlas, widget->fontSize);
        widget->textSize = TextDimensionsCalculate(widget->font, widget->text);
    }

    if (io->mousePosition >= widget->rect.point.p0 && io->mousePosition <= widget->rect.point.p1)
    {
        widget->hot_t = true;
        if (io->leftClicked)
        {
            widget->active_t = true;
        }
    }
    if (uiState->parent)
    {
        // add to widget tree
        DLLPushBack(uiState->parent->first, uiState->parent->last, widget);
    }

    ASSERT((IsNull(uiState->parent) && IsNull(uiState->current)) || !IsNull(uiState->parent),
           "Tree can only have one widget root");

    widget->parent = uiState->parent;
    uiState->current = widget;
}

root_function void
UI_Widget_SizeAndRelativePositionCalculate(GlyphAtlas* glyphAtlas, UI_State* uiState)
{
    UI_Widget* leftMostWidget = UI_Widget_TreeLeftMostFind(uiState->root);
    // size and relative position calculation
    for (UI_Widget* widget = leftMostWidget; !IsNull(widget);
         widget = UI_Widget_DepthFirstPostOrder(widget))
    {
        widget->computedSize = {0.0f, 0.0f};
        widget->computedRelativePosition = {0.0f, 0.0f};

        if (widget->semanticSize[Axis2_X].kind == UI_SizeKind_TextContent ||
            widget->semanticSize[Axis2_Y].kind == UI_SizeKind_TextContent)
        {
            widget->computedSize = widget->textSize + 2.0f * widget->borderThickness;
        }

        for (u32 axis = 0; axis < Axis2_COUNT; axis++)
        {
            switch(widget->semanticSize[axis].kind) {

                case UI_SizeKind_ChildrenSum:
                {
                    for (UI_Widget* child = widget->first; !IsNull(child); child = child->next)
                    {
                        child->computedRelativePosition[axis] = widget->computedSize[axis];
                        widget->computedSize[axis] += child->computedSize[axis];
                    }

                } break;
                case UI_SizeKind_Pixels: 
                {
                    widget->computedSize[axis] = widget->semanticSize[axis].value;
                } break;
                case UI_SizeKind_Null:
                {
                    for (UI_Widget* child = widget->first; !IsNull(child); child = child->next)
                    {
                        widget->computedSize[axis] =
                            Max(widget->computedSize[axis], child->computedSize[axis]);
                    }
                } break;
            }

        }
    }
}

root_function f32 ResizeChildren(UI_Widget* widget, Axis2 axis, f32 AxChildSizeCum, UI_Size parentSemanticSizeInfo, b32 useStrictness) {
    b32 resizingDone = 0;
    f32 strictness = 0;
    f32 sizeCum = AxChildSizeCum;
    for (UI_Widget* child = widget->last; !IsNull(child); child = child->prev) {
        UI_Size childSemanticSizeInfo = child->semanticSize[axis];
        if (useStrictness) {
            strictness = childSemanticSizeInfo.strictness;
        }
        sizeCum -= Max(child->computedSize[axis] - strictness, 0);
        child->computedSize[axis] = strictness;
        f32 sizeDiff = parentSemanticSizeInfo.value - sizeCum;
        if (sizeDiff >= 0) {
            sizeCum += sizeDiff;
            child->computedSize[axis] += sizeDiff;
            resizingDone=1;
            break;
        }
    }
    return sizeCum;
}

root_function void ReassignRelativePositionsOfChildren(UI_Widget* widget, Axis2 axis) {
    f32 sizeCum = 0;
    for (UI_Widget* child = widget->first; !IsNull(child); child = child->next) {
        child->computedRelativePosition[axis] = sizeCum;
        sizeCum += child->computedSize[axis];
    }
}

root_function void
UI_Widget_AbsolutePositionCalculate(UI_State* uiState, F32Vec4 posAbs)
{
    for (UI_Widget* widget = uiState->root; !IsNull(widget);
         widget = UI_Widget_DepthFirstPreOrder(widget))
    {
        widget->rect = {0};

        F32Vec4 rectParent = posAbs;
        if (widget != uiState->root)
        {
            rectParent = widget->parent->rect;
        }

        for (u32 axis = 0; axis < Axis2_COUNT; axis++) {
            UI_Size semanticSizeInfo = widget->semanticSize[axis];
            switch (semanticSizeInfo.kind) {
                case UI_SizeKind_Pixels:{
                    f32 childAxSizeTotal = 0;
                    for (UI_Widget* child = widget->first; !IsNull(child); child = child->next) {
                        childAxSizeTotal += child->computedSize[axis];
                    }
                    if (childAxSizeTotal > semanticSizeInfo.value) {
                        b32 useStrictness = 1;
                        
                        f32 sizeAfterStrictnessResize = ResizeChildren(widget,(Axis2)axis, childAxSizeTotal, semanticSizeInfo, useStrictness); 
                        if (sizeAfterStrictnessResize > semanticSizeInfo.value) {
                            useStrictness = 0;
                            ResizeChildren(widget,(Axis2)axis, sizeAfterStrictnessResize, semanticSizeInfo, useStrictness); 
                        }
                    }
                    ReassignRelativePositionsOfChildren(widget, (Axis2)axis);
                } break;
                case UI_SizeKind_PercentOfParent: {
                    ASSERT(0.0f <= semanticSizeInfo.value && semanticSizeInfo.value <= 1.0f, "Semantic value must be a value between 0 and 1");
                    f32 parentSize = rectParent.point.p1[axis] - rectParent.point.p0[axis];
                    widget->computedSize[axis] = parentSize * semanticSizeInfo.value;
                    f32 computedRelativePosition = {0};
                    if(widget->prev) {
                        computedRelativePosition = widget->prev->computedRelativePosition[axis] + widget->prev->computedSize[axis]; 
                    }
                    widget->computedRelativePosition[axis] = computedRelativePosition;
                } break;
                case UI_SizeKind_Null:
                {
                    u32 otherAx = (axis + 1) % Axis2_COUNT;
                    if (widget->semanticSize[otherAx].kind == UI_SizeKind_PercentOfParent) {
                        widget->computedSize[axis] = rectParent.point.p1[axis] - rectParent.point.p0[axis];
                    }
                } break;
            }
        }

        widget->rect.point.p0 =
            widget->computedRelativePosition + rectParent.point.p0;
        widget->rect.point.p1 = widget->rect.point.p0 + widget->computedSize;
    }
}

root_function void
UI_Widget_DrawPrepare(Arena* arena, UI_State* uiState, BoxContext* boxContext)
{
    for (UI_Widget* widget = uiState->root; !IsNull(widget);
         widget = UI_Widget_DepthFirstPreOrder(widget))
    {
        if (widget->flags & UI_WidgetFlag_DrawBackground)
        {
            Box* box = PushStructZero(arena, Box);
            QueuePush(boxContext->boxQueue.first, boxContext->boxQueue.last, box);

            // reacting to last frame input
            box->pos0 = widget->rect.point.p0;
            box->pos1 = widget->rect.point.p1;
            box->color = widget->color;
            box->softness = widget->softness;
            box->borderThickness = widget->borderThickness;
            box->cornerRadius = widget->cornerRadius;
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
            }
        }

        if (widget->flags & UI_WidgetFlag_DrawText)
        {
            Vec2 diffDim = widget->rect.point.p1 - widget->rect.point.p0 - widget->textSize;
            Vec2 glyphPos = widget->rect.point.p0 + diffDim / 2.0f;
            Vec2 p0xB = widget->rect.point.p0 + widget->borderThickness;
            Vec2 p1xB = widget->rect.point.p1 - widget->borderThickness;
            addText(arena, widget->font, widget->text, glyphPos, p0xB, p1xB, widget->textSize.y);
        }
    }
}

// layout
inline_function void
UI_PushLayout(UI_State* uiState)
{
    uiState->parent = uiState->current;
}

inline_function void
UI_PopLayout(UI_State* uiState)
{
    uiState->root = uiState->current = uiState->parent;
    uiState->parent = uiState->current ? uiState->current->parent : 0;
}

inline_function void
UI_State_FrameReset(UI_State* uiState)
{
    uiState->parent = 0;
    uiState->current = 0;
    uiState->root = 0;
}