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

// String functions

String8
Str8(u8* str, u64 size)
{
    String8 str8 = {};
    str8.size = size;
    str8.str = str;
    return str8;
}

String8
Str8Push(Arena* arena, String8 str)
{
    String8 target = {};
    target.size = str.size;
    target.str = PushArray(arena, u8, target.size + 1); // +1 for space for null termination
    MemoryCopy(target.str, str.str, str.size);
    return target;
}

String8
Str8NullTermFromStr8(String8 str)
{
    str.str[str.size] = (u8)'\0';
    return str;
}

u8*
StrFromU128(Arena* arena, u128 num)
{
    u64 size = sizeof(u128);
    u8* str = PushArray(arena, u8, size + 1);
    for (u32 numI = 0; numI < ArrayLen(num.data); numI++)
    {
        for (u32 i = 0; i < sizeof(u64); i++)
        {
            str[numI * sizeof(u64) + i] = (u8)(num.data[numI] >> (8 * i));
        }
    }
    str[size] = '\0';

    return str;
}

String8
str8(Arena* arena, char* str)
{
    String8 result = {0};
    result.size = 0;
    for (char* c = str; (*c) != '\0'; c++)
    {
        result.size += 1;
    }

    u64 resultSizeWithNullTermation = result.size + 1;

    result.str = PushArray(arena, u8, resultSizeWithNullTermation);
    MemoryCopy(result.str, str, result.size);
    result.str[result.size] = '\0';

    return result;
}