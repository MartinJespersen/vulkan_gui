#include "core.hpp"
ThreadCtx
ThreadContextAlloc()
{
    ThreadCtx ctx = {};
    ctx.scratchArena = (Arena*)AllocArena(MEGABYTE(64));
    return ctx;
}

void
ThreadContextDealloc(ThreadCtx* ctx)
{
    if (ctx)
    {
        DeallocArena(ctx->scratchArena);
    }
}
