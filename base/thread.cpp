per_thread ThreadCtx* threadCtx;

ThreadCtx
AllocThreadContext()
{
    ThreadCtx ctx = {};
    return ctx;
}

void
DeallocThreadContext()
{
    if (threadCtx)
    {
        DeallocArena(threadCtx->scratchArena);
    }
}

void
SetThreadContext(ThreadCtx* ctx)
{
    threadCtx = ctx;
}

ThreadCtx*
GetThreadContext()
{
    return threadCtx;
}

ArenaTemp
ScratchArenaBegin()
{
    ThreadCtx* ctx = GetThreadContext();
    ArenaTemp temp = {ctx->scratchArena, ctx->scratchArena->pos};
    return temp;
}

void
ScratchArenaEnd(ArenaTemp temp)
{
    ArenaPop(temp.arena, temp.pos);
}