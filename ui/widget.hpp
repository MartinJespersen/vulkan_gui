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

typedef f32 UI_WidgetFlags;
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

struct UI_Widget
{
    UI_Widget* first;
    UI_Widget* next;
    UI_Widget* prev;
    UI_Widget* last;
    UI_Widget* parent;

    UI_WidgetFlags flags;

    UI_Size semantic_size[Axis2_COUNT];
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