#pragma once

// standard lib
#include <cerrno>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN64
    #include <windows.h>
    #include <immintrin.h>
#elif defined(__linux__) 
    #include <sys/time.h>
    #include <x86intrin.h>
#else
# error OS not supported
#endif

#include <stdint.h>
#include <cstring>
#include <cassert>
#include <cstdarg>

// user defined
#include "error.hpp"
#include "third_party/third_party_wrapper.hpp"
#include "core.hpp"
#ifdef _GNUC_ //TODO: create implementation for windows as well  
#include "time.hpp"
#endif
#include "algos.hpp"