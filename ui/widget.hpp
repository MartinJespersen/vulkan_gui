#pragma once

#ifdef PROFILING_ENABLE
BufferDec(TracyVkCtx);
#endif

struct ProfilingContext
{
#ifdef PROFILING_ENABLE
    // tracy profiling context
    TracyVkCtx_Buffer tracyContexts;
#endif
};



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

    String8 name;
    
    UI_WidgetFlags flags;

    UI_Key key;

    UI_Size semanticSize[Axis2_COUNT];
    Vec2<f32> computedRelativePosition;
    Vec2<f32> computedSize;
    F32Vec4 rect;

    bool hot_t;
    bool active_t;

    // layout
    F32Vec4 color;
    f32 softness;
    f32 borderThickness;
    f32 cornerRadius;
    u32 fontSize;
    Font* font;
    String8 text;
    Vec2<f32> textSize;
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

    // frame state
    UI_Widget* current; // current widget
    UI_Widget* parent;  // current parent node
    UI_Widget* root;    // root of tree structure

    // ui cache size
    u64 widgetCacheSize;
    UI_WidgetSlot* widgetSlot;
    UI_Widget* freeList;
    u64 freeListSize;
};

struct Context
{
    VulkanContext* vulkanContext;
    ProfilingContext* profilingContext;
    GlyphAtlas* glyphAtlas;
    BoxContext* boxContext;
    UI_IO* io;
    UI_State* uiState;
    ThreadCtx* thread_ctx;

    u64 frameTickPrev;
    f64 frameRate;
    u64 cpuFreq;
};

// UI_Key functions
root_function UI_Key
UI_Key_Calculate(String8 str);

root_function bool
UI_Key_IsEqual(UI_Key key0, UI_Key key1);

root_function bool
UI_Key_IsNull(UI_Key key);

// Widget Cache

root_function UI_Widget*
UI_Widget_FromKey(UI_State* uiState, UI_Key key);

root_function UI_Widget*
UI_Widget_Allocate(UI_State* uiState);

root_function UI_Widget*
UI_WidgetSlot_Push(UI_State* uiState, UI_Key key);

// Widget functions
root_function f32 
ResizeChildren(UI_Widget* widget, Axis2 axis, f32 AxChildSizeCum, UI_Size parentSemanticSizeInfo, b32 useStrictness);

root_function void 
ReassignRelativePositionsOfChildren(UI_Widget* widget, Axis2 axis);

inline_function b32
UI_Widget_IsEmpty(UI_Widget* widget);

inline_function void
UI_Widget_SetEmpty(UI_Widget** widget);

root_function void
UI_Widget_Add(String8 widgetName, Context* context, const F32Vec4 color, String8 text,
              f32 softness, f32 borderThickness, f32 cornerRadius, UI_WidgetFlags flags,
              u32 fontSize, UI_Size semanticSizeX, UI_Size semanticSizeY);

root_function void
UI_Widget_SizeAndRelativePositionCalculate(GlyphAtlas* glyphAtlas, UI_State* uiState);

root_function UI_Widget*
UI_Widget_DepthFirstPreOrder(UI_Widget* widget);

inline_function UI_Widget*
UI_Widget_TreeLeftMostFind(UI_Widget* root);

root_function void
UI_Widget_AbsolutePositionCalculate(UI_State* uiState, F32Vec4 posAbs);

root_function void
UI_Widget_DrawPrepare(Arena* arena, UI_State* uiState, BoxContext* boxContext);

// Layout functions
inline_function void
UI_PushLayout(UI_State* uiState);

inline_function void
UI_PopLayout(UI_State* uiState);

inline_function void
UI_State_FrameReset(UI_State* uiState);