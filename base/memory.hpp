#pragma once

// os memory
void*
memAlloc(u64 size);

void
memFree(void* ptr, u64 freeSize);

// Arena
struct Arena
{
    void* ptr;
    u64 pos;
    u64 max;
    u64 align;
};

struct ArenaTemp
{
    Arena* arena;
    u64 pos;
};

Arena*
ArenaAlloc(u64 size);

void
ArenaDealloc(Arena* arena);

void
ArenaClear(Arena* arena);

void*
ArenaPush(Arena* arena, u64 size);

void*
ArenaPushZero(Arena* arena, u64 size);

void
ArenaPop(Arena* arena, u64 pos);

ArenaTemp
ArenaTempBegin(Arena* arena);

void
ArenaTempEnd(ArenaTemp temp);

// scratch arena
ArenaTemp
ArenaScratchBegin();

#define MemoryCopy(dst, src, size) memcpy((dst), (src), (size))
#define MemorySet(dst, byte, size) memset((dst), (byte), (size))

#define MemoryZero(ptr, size) MemorySet((ptr), 0, (size))
#define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(arr) MemoryZero((arr), sizeof(arr))

#define PushArray(arena, type, count) ((type*)ArenaPush((arena), sizeof(type) * (count)))
#define PushStruct(arena, type) PushArray((arena), type, 1)
#define PushArrayZero(arena, type, count) ((type*)ArenaPushZero((arena), sizeof(type) * (count)))
#define PushStructZero(arena, type) PushArrayZero((arena), type, 1)

// Buffer impl and macros
#define BufferDec(type)                                                                            \
    struct type##_Buffer                                                                           \
    {                                                                                              \
        type* data;                                                                                \
        u64 size;                                                                                  \
    };                                                                                             \
    type##_Buffer type##_Buffer_Alloc(Arena* arena, u64 count)

#define BufferImpl(type)                                                                           \
    type##_Buffer type##_Buffer_Alloc(Arena* arena, u64 count)                                     \
    {                                                                                              \
        type##_Buffer buffer = {0};                                                                \
        buffer.data = PushArray(arena, type, count);                                               \
        buffer.size = count;                                                                       \
        return buffer;                                                                             \
    }

struct Buffer
{
    u8* data;
    u64 size;
};