#pragma once

#include "core.hpp"
#include "error.hpp"
#include "memory.hpp"
#include "third_party/third_party_wrapper.hpp"
#include <algorithm>
#include <initializer_list>

// TODO: Create a array/list that is a mix of a fixed array and a linked list to improve cache
// utility for some workloads.

// Fixed array

template <typename T> struct Array
{
    T* data;
    u64 capacity;

    T&
    operator[](u64 index)
    {
        if (index >= capacity)
        {
            exitWithError("Indexing is out of bounds");
        }
        return data[index];
    }
};

template <typename T>
Array<T>
ArrayAlloc(Arena* arena, u64 capacity)
{
    Array<T>* arr = PushStruct(arena, Array<T>);
    arr->capacity = capacity;
    arr->data = PushArray(arena, T, arr->capacity);
    return *arr;
}

// Array Bound
template <typename T> struct ArrayBound
{
    T* data;
    u64 capacity;
    u64 size;

    T&
    operator[](u64 index)
    {
        if (index >= size)
        {
            exitWithError("Indexing is out of bounds");
        }
        return data[index];
    }
};

template <typename T>
ArrayBound<T>*
ArrayBoundAlloc(Arena* arena, u64 capacity)
{
    ArrayBound<T>* arr = PushStruct(arena, ArrayBound<T>);
    arr->capacity = capacity;
    arr->data = PushArray(arena, T, arr->capacity);
    arr->size = 0;
    return arr;
}

template <typename T>
T*
ArrayBoundPush(ArrayBound<T>* arr)
{
    arr->size++;
    if (arr->size >= arr->capacity)
    {
        exitWithError("Indexing Out of Bounds");
    }

    return arr[arr->size - 1];
}

// Linked List

template <typename T> struct LLItem
{
    LLItem<T>* next;
    T item;
};

template <typename T> struct LinkedList
{
    LLItem<T>* start;
    LLItem<T>* end;
};

template <typename T>
LinkedList<T>*
LinkedListAlloc(Arena* arena)
{
    LinkedList<T>* list = PushStruct(arena, LinkedList<T>);
    list->end = 0;
    list->start = 0;
    return list;
}

template <typename T>
T*
LinkedListPushItem(Arena* arena, LinkedList<T>* list)
{
    LLItem<T>* item = PushStruct(arena, LLItem<T>);
    item->next = 0;

    if (!list->start)
    {
        list->start = item;
    }
    else
    {
        list->end->next = item;
    }

    list->end = item;

    return &item->item;
}

// MeowHash wrapper

u128
HashFromString(String8 string)
{
    u128 hash = {0};
    {
        meow_u128 meow_hash = MeowHash(MeowDefaultSeed, string.size, string.str);
        MemoryCopy(&hash, &meow_hash, std::min(sizeof(meow_hash), sizeof(hash)));
    }
    return hash;
}
