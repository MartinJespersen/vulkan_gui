#pragma once

// os memory
void*
memAlloc(u64 size);

void
memFree(void* ptr, u64 freeSize);

// scratch arena
ArenaTemp
ArenaScratchBegin();
ArenaTemp
ArenaScratchGet(Arena** conflicts, u64 conflict_count);

#define MemoryCopy(dst, src, size) memcpy((dst), (src), (size))
#define MemorySet(dst, byte, size) memset((dst), (byte), (size))

#define MemoryZero(ptr, size) MemorySet((ptr), 0, (size))
#define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(arr) MemoryZero((arr), sizeof(arr))

#define PushArray(arena, type, count) ((type*)ArenaPush((arena), sizeof(type) * (count)))
#define PushStruct(arena, type) PushArray((arena), type, 1)
#define PushArrayZero(arena, type, count) ((type*)ArenaPushZero((arena), sizeof(type) * (count)))
#define PushStructZero(arena, type) PushArrayZero((arena), type, 1)

// align
#define PushArrayAlign(arena, type, count)                                                         \
    ((type*)ArenaPushAlign((arena), sizeof(type) * (count), (Max(8, AlignOf(type)))))
#define PushStructAlign(arena, type) PushArrayAlign((arena), type, 1)
#define PushArrayZeroAlign(arena, type, count)                                                     \
    ((type*)ArenaPushZeroAlign((arena), sizeof(type) * (count), (Max(8, AlignOf(type)))))
#define PushStructZeroAlign(arena, type) PushArrayZeroAlign((arena), type, 1)

// Buffers
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

BufferDec(u16);
BufferDec(String8);