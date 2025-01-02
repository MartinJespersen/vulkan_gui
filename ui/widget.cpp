#include "widget.hpp"
#include "base/core.hpp"

bool
UI_Widget_IsNull(UI_Widget* widget)
{
    return widget == nullptr || widget == &g_UI_Widget;
}

UI_Widget*
UI_Widget_FromKey(UI_State* uiState, UI_Key key)
{
    UI_Widget* result = &g_UI_Widget;
    u64 slotKey = key.key % uiState->widgetCacheSize;
    UI_WidgetSlot* slot = &uiState->widgetSlot[slotKey];

    for (UI_Widget* widget = slot->first; !UI_Widget_IsNull(widget); widget = widget->hashNext)
    {
        if (UI_Key_IsEqual(widget->key, key))
        {
            result = widget;
            break;
        }
    }

    if (UI_Widget_IsNull(result))
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

bool
UI_WidgetSlot_IsNull(UI_WidgetSlot* slot)
{
    return slot == nullptr || slot == &g_UI_WidgetSlot;
}

UI_Widget*
UI_WidgetSlot_Push(UI_State* uiState, UI_Key key)
{
    UI_WidgetSlot* slot = &uiState->widgetSlot[key.key % uiState->widgetCacheSize];
    UI_Widget* widget = UI_Widget_Allocate(uiState);

    DLLPushBack_NPZ(slot->first, slot->last, widget, hashPrev, hashNext, UI_Widget_IsNull,
                    UI_Widget_SetNULL);
    widget->key = key;
    return widget;
}