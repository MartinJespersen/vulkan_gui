#pragma once

struct UI_IO
{
    Vec2<f64> mousePosition;
    bool leftClicked;
};

std::vector<char>
ReadFile(const std::string& filename);