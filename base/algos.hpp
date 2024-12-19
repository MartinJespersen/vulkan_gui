#pragma once

#include "error.hpp"
#include "memory.hpp"
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
