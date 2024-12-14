#pragma once

#ifdef __linux__
#define per_thread __thread
#else
#error thread context not implemented for os
#endif

#define KILOBYTE(n) ((n) * 1024)
#define MEGABYTE(n) (KILOBYTE(n) * 1024)