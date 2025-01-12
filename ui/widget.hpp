#pragma once

struct Panel
{
    Panel* first;
    Panel* last;
    Panel* next;
    Panel* prev;
    Panel* parent;
    f32 pct_of_parent;
    Axis2 split_axis;
};

enum UI_SizeKind
{
    UI_SizeKind_Null,
    UI_SizeKind_Pixels,
    UI_SizeKind_TextContent,
    UI_SizeKind_PercentOfParent,
    UI_SizeKind_ChildrenSum,
};

struct UI_Size
{
    UI_SizeKind kind;
    f32 value;
    f32 strictness;
};

typedef u32 UI_WidgetFlags;
enum
{
    UI_WidgetFlag_Clickable = (1 << 0),
    UI_WidgetFlag_ViewScroll = (1 << 1),
    UI_WidgetFlag_DrawText = (1 << 2),
    UI_WidgetFlag_DrawBorder = (1 << 3),
    UI_WidgetFlag_DrawBackground = (1 << 4),
    UI_WidgetFlag_DrawDropShadow = (1 << 5),
    UI_WidgetFlag_Clip = (1 << 6),
    UI_WidgetFlag_HotAnimation = (1 << 7),
    UI_WidgetFlag_ActiveAnimation = (1 << 8),
    // potentially more flags
};

struct UI_Key
{
    u64 key;
};

struct UI_Widget
{
    UI_Widget* hashNext;
    UI_Widget* hashPrev;

    UI_Widget* first;
    UI_Widget* next;
    UI_Widget* prev;
    UI_Widget* last;
    UI_Widget* parent;

    UI_WidgetFlags flags;

    UI_Key key;

    UI_Size semanticSize[Axis2_COUNT];
    Vec2<f32> computedRelativePosition;
    Vec2<f32> computedSize;
    F32Vec4 rect;

    bool hot_t;
    bool active_t;
};

typedef u32 UI_Comm;
enum
{
    UI_Comm_Hovered = (1 << 0),
    UI_Comm_Clicked = (1 << 1)
};

struct UI_WidgetSlot
{
    UI_Widget* first;
    UI_Widget* last;
};

struct UI_State
{
    Arena* arena;
    UI_Widget* root;

    // ui cache size
    u64 widgetCacheSize;
    UI_WidgetSlot* widgetSlot;
    UI_Widget* freeList;
    u64 freeListSize;
};

// UI_Key functions
UI_Key
UI_Key_Calculate(String8 str);

bool
UI_Key_IsEqual(UI_Key key0, UI_Key key1);

bool
UI_Key_IsNull(UI_Key key);

// Widget Cache

UI_Widget g_UI_Widget = {&g_UI_Widget, &g_UI_Widget, &g_UI_Widget, &g_UI_Widget,
                         &g_UI_Widget, &g_UI_Widget, &g_UI_Widget};

#define UI_Widget_SetNULL(w) ((w) = &g_UI_Widget)

bool
UI_Widget_IsNull(UI_Widget* widget);

UI_Widget*
UI_Widget_FromKey(UI_State* uiState, UI_Key key);

UI_Widget*
UI_Widget_Allocate(UI_State* uiState);

// UI_WidgetSlot
UI_WidgetSlot g_UI_WidgetSlot = {};

bool
UI_WidgetSlot_IsNull(UI_WidgetSlot* slot);

UI_Widget*
UI_WidgetSlot_Push(UI_State* uiState, UI_Key key);

// Button function

void
AddButton(String8 widgetName, UI_State* uiState, Arena* arena, Box* box, VkExtent2D swapChainExtent,
          Font* font, const F32Vec4 color, const std::string text, f32 softness,
          f32 borderThickness, f32 cornerRadius, UI_IO* io, F32Vec4 positions,
          UI_WidgetFlags flags);