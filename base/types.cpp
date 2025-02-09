// Arenas
Arena*
ArenaAlloc(u64 size)
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
ArenaDealloc(Arena* arena)
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
    for (u32 numI = 0; numI < ArrayCount(num.data); numI++)
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
Str8(Arena* arena, const char* str)
{
    String8 result = {0};
    result.size = 0;
    for (const char* c = str; (*c) != '\0'; c++)
    {
        result.size += 1;
    }

    u64 resultSizeWithNullTermation = result.size + 1;

    result.str = PushArray(arena, u8, resultSizeWithNullTermation);
    MemoryCopy(result.str, str, result.size);
    result.str[result.size] = '\0';

    return result;
}

char*
StrFromStr8(String8 str)
{
    return (char*)str.str;
}

char**
StrArrFromStr8Buffer(Arena* arena, String8* buffer, u64 count)
{
    char** arr = PushArray(arena, char*, count);

    for (u32 i = 0; i < count; i++)
    {
        arr[i] = (char*)buffer[i].str;
    }
    return arr;
}