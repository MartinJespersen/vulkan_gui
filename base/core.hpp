#pragma once

#ifdef __linux__
#define per_thread __thread
#else
#error thread context not implemented for os
#endif

#define KILOBYTE(n) ((n) * 1024UL)
#define MEGABYTE(n) (KILOBYTE(n) * 1024UL)
#define GIGABYTE(n) (MEGABYTE(n) * 1024UL)

#define ArrayCount(a) (sizeof((a)) / sizeof((a[0])))

// linked list helpers
#define CheckEmpty(p, o) ((p) == 0 || (p) == (o))
#define CheckNull(p) ((p) == 0)
#define SetNull(p) ((p) = 0)

#define QueuePush_NZ(f, l, n, next, zchk, zset)                                                    \
    (zchk(f) ? (((f) = (l) = (n)), zset((n)->next)) : ((l)->next = (n), (l) = (n), zset((n)->next)))
#define QueuePushFront_NZ(f, l, n, next, zchk, zset)                                               \
    (zchk(f) ? (((f) = (l) = (n)), zset((n)->next)) : ((n)->next = (f)), ((f) = (n)))
#define QueuePop_NZ(f, l, next, zset) ((f) == (l) ? (zset(f), zset(l)) : ((f) = (f)->next))
#define StackPush_N(f, n, next) ((n)->next = (f), (f) = (n))
#define StackPop_NZ(f, next, zchk) (zchk(f) ? 0 : ((f) = (f)->next))

#define DLLInsert_NPZ(f, l, p, n, next, prev, zchk, zset)                                          \
    (zchk(f) ? (((f) = (l) = (n)), zset((n)->next), zset((n)->prev))                               \
     : zchk(p)                                                                                     \
         ? (zset((n)->prev), (n)->next = (f), (zchk(f) ? (0) : ((f)->prev = (n))), (f) = (n))      \
         : ((zchk((p)->next) ? (0) : (((p)->next->prev) = (n))), (n)->next = (p)->next,            \
            (n)->prev = (p), (p)->next = (n), ((p) == (l) ? (l) = (n) : (0))))
#define DLLPushBack_NPZ(f, l, n, next, prev, zchk, zset)                                           \
    DLLInsert_NPZ(f, l, l, n, next, prev, zchk, zset)
#define DLLRemove_NPZ(f, l, n, next, prev, zchk, zset)                                             \
    (((f) == (n))   ? ((f) = (f)->next, (zchk(f) ? (zset(l)) : zset((f)->prev)))                   \
     : ((l) == (n)) ? ((l) = (l)->prev, (zchk(l) ? (zset(f)) : zset((l)->next)))                   \
                    : ((zchk((n)->next) ? (0) : ((n)->next->prev = (n)->prev)),                    \
                       (zchk((n)->prev) ? (0) : ((n)->prev->next = (n)->next))))

#define QueuePush(f, l, n) QueuePush_NZ(f, l, n, next, CheckNull, SetNull)
#define QueuePushFront(f, l, n) QueuePushFront_NZ(f, l, n, next, CheckNull, SetNull)
#define QueuePop(f, l) QueuePop_NZ(f, l, next, SetNull)
#define StackPush(f, n) StackPush_N(f, n, next)
#define StackPop(f) StackPop_NZ(f, next, CheckNull)
#define DLLPushBack(f, l, n) DLLPushBack_NPZ(f, l, n, next, prev, CheckNull, SetNull)
#define DLLPushFront(f, l, n) DLLPushBack_NPZ(l, f, n, prev, next, CheckNull, SetNull)
#define DLLInsert(f, l, p, n) DLLInsert_NPZ(f, l, p, n, next, prev, CheckNull, SetNull)
#define DLLRemove(f, l, n) DLLRemove_NPZ(f, l, n, next, prev, CheckNull, SetNull)

// string manipulation helpers
#define CStrEqual(a, b) (!strcmp((a), (b)))

// bit arithmetic
#ifdef __GNUC__
#define LSBIndex(n) (__builtin_ffs((n)) - 1)
#else
#error compiler not supported
#endif

// tread context

struct ThreadCtx
{
    Arena* scratchArena;
};

ThreadCtx
ThreadContextAlloc();

void
ThreadContextDealloc(ThreadCtx* ctx);

// Strings

struct String8
{
    u64 size;
    u8* str;
};

String8
Str8(u8* str, u64 size);

String8
Str8Push(Arena* arena, String8 str);

String8
Str8NullTermFromStr8(String8 str);

u8*
StrFromU128(Arena* arena, u128 num);

String8
Str8(Arena* arena, char* str);
