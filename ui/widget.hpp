#pragma once

// Widget Configuration -------------------------------------------



// UI_Key functions
root_function UI_Key
UI_Key_Calculate(String8 str);

root_function bool
UI_Key_IsEqual(UI_Key key0, UI_Key key1);

root_function bool
UI_Key_IsNull(UI_Key key);

// Widget Cache

root_function UI_Widget*
UI_Widget_FromKey(UI_State* ui_state, UI_Key key);

root_function UI_Widget*
UI_Widget_Allocate(UI_State* ui_state);

root_function UI_Widget*
UI_WidgetSlot_Push(UI_State* ui_state, UI_Key key);

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
UI_Widget_Add(String8 widgetName, const F32Vec4 color, f32 softness, 
            f32 borderThickness, f32 cornerRadius, UI_WidgetFlags flags,
            UI_Size semanticSizeX, UI_Size semanticSizeY);

root_function void
UI_Widget_SizeAndRelativePositionCalculate(GlyphAtlas* glyphAtlas, UI_State* ui_state);

root_function UI_Widget*
UI_Widget_DepthFirstPreOrder(UI_Widget* widget);

inline_function UI_Widget*
UI_Widget_TreeLeftMostFind(UI_Widget* root);

root_function void
UI_Widget_AbsolutePositionCalculate(UI_State* ui_state, F32Vec4 posAbs);

root_function void
UI_Widget_DrawPrepare(Arena* arena, UI_State* ui_state, BoxContext* boxContext);

// Layout functions
inline_function void
UI_PushLayout(UI_State* ui_state);

inline_function void
UI_PopLayout(UI_State* ui_state);

inline_function void
UI_State_FrameReset(UI_State* ui_state);

root_function Vec2<f32>
UI_TextExtSizeCalc(UI_Widget* widget);

root_function void
UI_TextExtDraw(UI_Widget* widget);

root_function void 
UI_Widget_TextExtAdd(UI_Widget* widget, UI_TextExtSizeCalcFuncType* size_calc_func, UI_TextExtDrawFuncType* draw_func);
