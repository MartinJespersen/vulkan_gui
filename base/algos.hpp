#pragma once

#include "memory.hpp"
#include <initializer_list>

template <typename T> struct Vector
{
    T* Data;
    u64 Size;
    u64 Capacity;

    Vector()
    {
        Capacity = 0;
        Size = 0;
        Data = NULL;
    }

    void
    resize(u64 newSize)
    {
        if (newSize > Capacity)
        {
            T* oldData = Data;
            Capacity = newSize + Capacity / 2;
            Data = memAlloc(Capacity * sizeof(T));
            memcpy(Data, oldData, Size);
            memFree(oldData, Size * sizeof(T));
        }
        Size = newSize;
    }

    void
    add(std::initializer_list<T> list)
    {
        u64 listSize = list.size();
        u64 listSizeInBytes = listSize * sizeof(T);
        u64 minimumCapacity = listSize + Size;
        u64 oldSize = Size;

        resize(minimumCapacity);
        memcpy(Data + oldSize, list.begin(), listSizeInBytes);
    }

    int
    pushBack(const T& elem)
    {
        u64 oldSize = Size;
        u64 requiredCapacity = Size + 1;

        resize(requiredCapacity);
        Data[oldSize] = elem;

        return 0;
    }

    void
    reset()
    {
        Size = 0;
    }

    ~Vector()
    {
        if (Data)
        {
            delete[] Data;
        }
    }
};

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
Array<T>*
AllocArray(Arena* arena, u64 capacity)
{
    Array<T>* arr = PushStruct(arena, Array<T>);
    arr->capacity = capacity;
    arr->data = PushArray(arena, T, arr->capacity);
    return arr;
}