// profiling ------------------------------------------
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

// Widget Configuration -------------------------------------
#define WidgetCfg \
    X(C_Parent,             UI_Widget*,     g_ui_widget) \
    X(C_FontSize,           u32,            0) \
    X(C_Text,               String8,        {0}) \
    X(C_BackgroundColor,    F32Vec4,        {0}) 

enum WidgetConfigEnum {
    #define X(name, type, default) name,
        WidgetCfg
    #undef X
    WIDGET_CONFIG_COUNT
};
struct Config {
    Config* next;
    void* ptr;
};

struct ConfigBucket {
    Config* c[WIDGET_CONFIG_COUNT];
};

// UI State --------------------------------------------
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


//format + styling extension
struct UI_Widget;
typedef Vec2<f32> UI_TextExtSizeCalcFuncType(UI_Widget* widget);
typedef void UI_TextExtDrawFuncType(UI_Widget* widget);
struct UI_TextExtData {
    u32 font_size;
    String8 text;
    Vec2<f32> text_size;
};

struct UI_TextExt {
    UI_TextExtSizeCalcFuncType* size_calc_func;
    UI_TextExtDrawFuncType* draw_func;
    void* data;
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

    // format + styling extensions
    UI_TextExt *text_ext;

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
};

struct UI_WidgetSlot
{
    UI_Widget* first;
    UI_Widget* last;
};

struct UI_State
{
    Arena* arena_permanent;

    // frame state
    Arena* arena_frame;
    UI_Widget* current; // current widget
    UI_Widget* root;    // root of tree structure

    // ui cache size
    u64 widgetCacheSize;
    UI_WidgetSlot* widgetSlot;
    UI_Widget* freeList;
    u64 freeListSize;

    // Configuration options
    ConfigBucket cfg_bucket;
};

struct UI_IO
{
    Vec2<f64> mousePosition;
    bool leftClicked;
};


// global contexts 
struct Context
{
    VulkanContext* vulkanContext;
    ProfilingContext* profilingContext;
    GlyphAtlas* glyphAtlas;
    BoxContext* boxContext;
    UI_IO* io;
    UI_State* ui_state;

    u64 frameTickPrev;
    f64 frameRate;
    u64 cpuFreq;

    // empty objects
    UI_Widget* g_ui_widget;
};


