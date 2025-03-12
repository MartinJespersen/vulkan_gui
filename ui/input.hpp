#pragma once

struct UI_IO
{
    Vec2<f64> mousePosition;
    bool leftClicked;
};

root_function Buffer
UI_ReadFile(Arena* arena, const std::string& filename);