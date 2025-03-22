#pragma once

// tread context
#ifdef __linux__
#define per_thread __thread
#elif defined(_MSC_VER)
#define per_thread __declspec(thread)
#else
#error thread context not implemented for os
#endif

// Widget Configuration --------------------------------

#define X(name, type) \
    inline_function type name##_Get();
WidgetCfg
#undef X

#define X(name, type) \
    inline_function void name##_Push(type v);
WidgetCfg
#undef X

#define X(name, type) \
    inline_function void name##_Pop();
WidgetCfg
#undef X

// Threading Context
struct ThreadCtx
{
    Arena* scratchArenas[2];
};

no_name_mangle void
ThreadContextInit();

no_name_mangle void
ThreadContextExit();

no_name_mangle void
ThreadCxtSet(ThreadCtx* ctx);

root_function ThreadCtx* 
ThreadCtxGet();

root_function ThreadCtx
ThreadContextAlloc();

root_function void
ThreadContextDealloc(ThreadCtx* ctx);

// globals context
no_name_mangle void GlobalContextSet(Context* ctx);
inline_function Context* GlobalContextGet();

