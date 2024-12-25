#pragma once
#include "types.hpp"
#include <cstdio>
#include <cstdlib>

#define ASSERT(condition, msg) assert(((void)msg, condition))

inline void
exitWithError(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}
