

// allocations

#ifdef __linux__
#include <sys/mman.h>

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

// scratch arena

per_thread ThreadCtx g_ThreadCtx;

__attribute__((constructor)) void
thread_init()
{
    for (u32 tctx_i = 0; tctx_i < ArrayCount(g_ThreadCtx.scratchArenas); tctx_i++)
    {
        g_ThreadCtx.scratchArenas[tctx_i] = ArenaAlloc(GIGABYTE(4));
    }
}

__attribute__((destructor)) void
thread_exit()
{
    for (u32 tctx_i = 0; tctx_i < ArrayCount(g_ThreadCtx.scratchArenas); tctx_i++)
    {
        ArenaDealloc(g_ThreadCtx.scratchArenas[tctx_i]);
    }
}

ArenaTemp
ArenaScratchBegin()
{
    ArenaTemp temp = {};
    temp.pos = g_ThreadCtx.scratchArenas[0]->pos;
    temp.arena = g_ThreadCtx.scratchArenas[0];
    return temp;
}

ArenaTemp
ArenaScratchGet(Arena** conflicts, u64 conflict_count)
{
    ArenaTemp scratch = {0};
    ThreadCtx* tctx = &g_ThreadCtx;
    for (u64 tctx_idx = 0; tctx_idx < ArrayCount(tctx->scratchArenas); tctx_idx += 1)
    {
        b32 is_conflicting = 0;
        for (Arena** conflict = conflicts; conflict < conflicts + conflict_count; conflict += 1)
        {
            if (*conflict == tctx->scratchArenas[tctx_idx])
            {
                is_conflicting = 1;
                break;
            }
        }
        if (is_conflicting == 0)
        {
            scratch.arena = tctx->scratchArenas[tctx_idx];
            scratch.pos = scratch.arena->pos;
            break;
        }
    }
    return scratch;
}

// Buffers
BufferImpl(u16);
BufferImpl(String8);
