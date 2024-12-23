#pragma once
#include "core.hpp"
#include "memory.hpp"

struct ThreadCtx
{
    Arena* scratchArena;
};

ThreadCtx
AllocThreadContext();

void
DeallocThreadContext();

void
SetThreadContext(ThreadCtx* ctx);

ThreadCtx*
GetThreadContext();

ArenaTemp
ScratchArenaBegin();

void
ScratchArenaEnd(ArenaTemp temp);
