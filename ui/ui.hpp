#pragma once

// third party libs
#include <vulkan/vulkan_core.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm> // Necessary for std::clamp
#include <glm/glm.hpp>
#include <fstream>

// user defined headers
#include "vulkan_helpers.hpp"
#include "box.hpp"
#include "fonts.hpp"
#include "input.hpp"
#include "widget.hpp"