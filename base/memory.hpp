#pragma once
#include <cerrno>
#include <cstdlib>

#ifdef __linux__
#include <string.h>
#include <sys/mman.h>

#include "error.hpp"
#include "types.hpp"

void*
memAlloc(u64 size)
{
    void* mappedMem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mappedMem == MAP_FAILED)
    {
        exitWithError(strerror(errno));
    }
    return mappedMem;
}

void
memFree(void* ptr, u64 freeSize)
{
    if (munmap(ptr, freeSize) < 0)
    {
        exitWithError(strerror(errno));
    }
}

#else
#error "Unsupported OS"
#endif

// Arena
struct Arena
{
    void* ptr;
    u64 pos;
    u64 max;
    u64 align;
};

struct ArenaTemp
{
    Arena* arena;
    u64 pos;
};

Arena*
AllocArena(u64 size);

void
DeallocArena(Arena* arena);

void
ArenaClear(Arena* arena);

void*
ArenaPush(Arena* arena, u64 size);

void
ArenaPop(Arena* arena, u64 pos);

ArenaTemp
ArenaTempBegin(Arena* arena);

void
ArenaTempEnd(ArenaTemp temp);

#define PushArray(arena, type, count) ((type*)ArenaPush((arena), sizeof(type) * (count)))
#define PushStruct(arena, type) PushArray((arena), type, 1)
