#include "memory.hpp"

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
ArenaBeginTemp(Arena* arena)
{
    ArenaTemp arenaTemp = {arena, arena->pos};
    return arenaTemp;
}

void
ArenaTempEnd(ArenaTemp temp)
{
    temp.arena->pos = temp.pos;
}
