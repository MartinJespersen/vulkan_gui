// Disable warnings for stb_image include
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-function"

#include "meow_hash_x64_aesni.h"
#include "stb_sprintf.h"
#include "stb_image.h"
// Re-enable warnings for your code
#pragma GCC diagnostic pop