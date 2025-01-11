#include "base/base.hpp"
#include "fonts.hpp"
#include "ui/ui.hpp"
#include "vulkan_helpers.hpp"

void
AddButton(String8 widgetName, UI_State* uiState, Arena* arena, Box* box, VkExtent2D swapChainExtent,
          Font* font, const Vec3 color, const std::string text, f32 softness, f32 borderThickness,
          f32 cornerRadius, UI_IO* io, F32Vec4 positions, UI_WidgetFlags flags);