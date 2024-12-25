#include "entrypoint.hpp"

void
AddButton(Arena* arena, Box* box, VkExtent2D swapChainExtent, Font* font, const Vec2<f32> pos,
          const Vec2<f32> dim, const Vec3 color, const std::string text, f32 softness,
          f32 borderThickness, f32 cornerRadius);