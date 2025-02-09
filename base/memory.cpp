

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
    g_ThreadCtx.scratchArena = ArenaAlloc(GIGABYTE(4));
}

__attribute__((destructor)) void
thread_exit()
{
    ArenaDealloc(g_ThreadCtx.scratchArena);
}

ArenaTemp
ArenaScratchBegin()
{
    ArenaTemp temp = {};
    temp.pos = g_ThreadCtx.scratchArena->pos;
    temp.arena = g_ThreadCtx.scratchArena;
    return temp;
}

// Buffers
BufferImpl(u16);
BufferImpl(String8);
