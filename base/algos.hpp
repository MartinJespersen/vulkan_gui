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
ArrayAlloc(Arena* arena, u64 capacity);

// hashing
u128
HashFromStr8(String8 string);
