// Arenas
root_function Arena*
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

root_function void
ArenaClear(Arena* arena)
{
    arena->pos = 0;
}

root_function void*
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

root_function void*
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

root_function void*
ArenaPushZero(Arena* arena, u64 size)
{
    void* box = ArenaPush(arena, size);
    MemoryZero(box, size);
    return box;
}

root_function void*
ArenaPushZeroAlign(Arena* arena, u64 size, u64 align)
{
    void* box = ArenaPushAlign(arena, size, align);
    MemoryZero(box, size);
    return box;
}

root_function void
ArenaPop(Arena* arena, u64 pos)
{
    arena->pos = pos;
}

root_function void
ArenaDealloc(Arena* arena)
{
    memFree(arena, arena->max);
}

root_function ArenaTemp
ArenaTempBegin(Arena* arena)
{
    ArenaTemp arenaTemp = {arena, arena->pos};
    return arenaTemp;
}

root_function void
ArenaTempEnd(ArenaTemp temp)
{
    temp.arena->pos = temp.pos;
}

// String functions

root_function String8
Str8(u8* str, u64 size)
{
    String8 str8 = {};
    str8.size = size;
    str8.str = str;
    return str8;
}

root_function String8
Str8Push(Arena* arena, String8 str)
{
    String8 target = {};
    target.size = str.size;
    target.str = PushArray(arena, u8, target.size + 1); // +1 for space for null termination
    MemoryCopy(target.str, str.str, str.size);
    return target;
}

root_function String8
Str8NullTermFromStr8(String8 str)
{
    str.str[str.size] = (u8)'\0';
    return str;
}

root_function u8*
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

root_function String8
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

root_function String8
Str8(Arena* arena, const char* str, ...)
{
    va_list args;
    va_start(args, str);
    String8 result = Str8V(arena, str, args);
    va_end(args);
    return result;
}

root_function char*
StrFromStr8(String8 str)
{
    return (char*)str.str;
}

root_function char**
StrArrFromStr8Buffer(Arena* arena, String8* buffer, u64 count)
{
    char** arr = PushArray(arena, char*, count);

    for (u32 i = 0; i < count; i++)
    {
        arr[i] = (char*)buffer[i].str;
    }
    return arr;
}

// allocations

#ifdef __linux__
#include <sys/mman.h>

root_function void*
memAlloc(u64 size)
{
    void* mappedMem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mappedMem == MAP_FAILED)
    {
        exitWithError(strerror(errno));
    }
    return mappedMem;
}

root_function void
memFree(void* ptr, u64 freeSize)
{
    if (munmap(ptr, freeSize) < 0)
    {
        exitWithError(strerror(errno));
    }
}

#elif defined(_MSC_VER)

// Function to allocate memory
root_function void* memAlloc(u64 size)
{
    // Allocate memory using VirtualAlloc
    void* mappedMem = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (mappedMem == NULL)
    {
        // Handle error if allocation fails
        exitWithError("Memory allocation failed");
    }
    return mappedMem;
}

// Function to free memory
root_function void memFree(void* ptr, u64 freeSize)
{
    // Free memory using VirtualFree
    if (!VirtualFree(ptr, 0, MEM_RELEASE))
    {
        // Handle error if deallocation fails
        exitWithError("Memory deallocation failed");
    }
}
#else
#error "Unsupported OS"
#endif

// scratch arena

per_thread ThreadCtx g_ThreadCtx;

void
ThreadContextInit()
{
    u64 size = GIGABYTE(4);
    for (u32 tctx_i = 0; tctx_i < ArrayCount(g_ThreadCtx.scratchArenas); tctx_i++)
    {
        g_ThreadCtx.scratchArenas[tctx_i] = ArenaAlloc(size);
    }
}

void
ThreadContextExit()
{
    for (u32 tctx_i = 0; tctx_i < ArrayCount(g_ThreadCtx.scratchArenas); tctx_i++)
    {
        ArenaDealloc(g_ThreadCtx.scratchArenas[tctx_i]);
    }
}

root_function ArenaTemp
ArenaScratchGet()
{
    ArenaTemp temp = {};
    temp.pos = g_ThreadCtx.scratchArenas[0]->pos;
    temp.arena = g_ThreadCtx.scratchArenas[0];
    return temp;
}

root_function ArenaTemp
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

// helpers
template <typename N>
root_function N
Clamp(N d, N min, N max)
{
    N temp = d > min ? d : min;
    return temp < max ? temp : max;
}