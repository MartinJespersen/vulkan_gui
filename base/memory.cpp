#include "memory.hpp"

// Arenas
Arena*
AllocArena(u64 size)
{
    void* block = memAlloc(size);
    Arena* arena = (Arena*)block;
    arena->max = size;
    arena->pos = sizeof(Arena);
    arena->ptr = (void*)((u8*)arena + arena->pos);
    arena->align = 8;
    return arena;
}

void
ArenaClear(Arena* arena)
{
    arena->pos = 0;
}

void*
ArenaPush(Arena* arena, u64 size)
{
    if ((arena->pos + size + arena->align) > arena->max)
    {
        return nullptr;
    }

    arena->pos += (arena->align - 1);
    arena->pos -= arena->pos % arena->align;
    void* result = (void*)((u8*)arena->ptr + arena->pos);
    arena->pos += size;
    return result;
}

void*
ArenaPushZero(Arena* arena, u64 size)
{
    void* box = ArenaPush(arena, size);
    MemoryZero(box, size);
    return box;
}

void
ArenaPop(Arena* arena, u64 pos)
{
    arena->pos = pos;
}

void
DeallocArena(Arena* arena)
{
    memFree(arena, arena->max);
}

ArenaTemp
ArenaTempBegin(Arena* arena)
{
    ArenaTemp arenaTemp = {arena, arena->pos};
    return arenaTemp;
}

void
ArenaTempEnd(ArenaTemp temp)
{
    temp.arena->pos = temp.pos;
}

// allocations

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