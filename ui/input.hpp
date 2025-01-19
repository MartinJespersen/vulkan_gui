#pragma once

struct UI_IO
{
    Vec2<f64> mousePosition;
    bool leftClicked;
};

Buffer
ReadFile(Arena* arena, const std::string& filename);