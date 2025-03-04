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
ArenaPushAlign(Arena* arena, u64 size, u64 align)
{
    if ((arena->pos + size + align) > arena->max)
    {
        return nullptr;
    }

    arena->pos += (align - 1);
    arena->pos -= arena->pos % align;
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

void*
ArenaPushZeroAlign(Arena* arena, u64 size, u64 align)
{
    void* box = ArenaPushAlign(arena, size, align);
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

static inline String8
Str8V(Arena* arena, const char* str, va_list args)
{
    String8 result = {0};
    va_list args2;
    va_copy(args2, args);
    u64 sizeWithNullTermination = (u64)stbsp_vsnprintf(0, 0, str, args) + 1UL;
    result.size = sizeWithNullTermination - 1;

    result.str = PushArray(arena, u8, sizeWithNullTermination);
    stbsp_vsnprintf((char*)result.str, (i32)sizeWithNullTermination, str, args2);

    return result;
}

String8
Str8(Arena* arena, const char* str, ...)
{
    va_list args;
    va_start(args, str);
    String8 result = Str8V(arena, str, args);
    va_end(args);
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