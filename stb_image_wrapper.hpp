// Disable warnings for stb_image include
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Re-enable warnings for your code
#pragma GCC diagnostic pop