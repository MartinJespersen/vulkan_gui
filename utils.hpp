#pragma once
#include <stdexcept>

struct Vector2
{
    float x;
    float y;
};

template <typename T> struct GrowthVector
{
    T* data;
    u32 size;
    u32 capacity;

    GrowthVector()
    {
        this->data = nullptr;
        this->size = 0;
        this->capacity = 0;
    }

    GrowthVector(T* data, u32 size)
    {
        this->data = data;
        this->size = size;
        this->capacity = this->size;
    }

    inline int
    pushBack(T glyph)
    {
        if (size >= capacity)
        {
            return -1;
        }
        data[size] = glyph;
        size++;

        return 0;
    }

    inline void
    reset(u32 newSize)
    {
        if (newSize > capacity)
        {
            if (data)
            {
                delete[] data;
            }
            this->capacity = newSize;
            data = new T[this->capacity];
        }
        size = 0;
    }
};

template <typename T> struct StaticArray
{
    T* data;
    u32 size;

    StaticArray()
    {
        this->data = nullptr;
        this->size = 0;
    }

    StaticArray(u32 size)
    {
        this->data = new T[size];
        this->size = size;
    }

    inline void
    setSize(u32 size)
    {
        if (data)
        {
            throw std::runtime_error("Cannot set size of StaticArray after initialization!");
        }
        this->data = new T[size];
        this->size = size;
    }

    ~StaticArray()
    {
        if (data)
        {
            delete[] data;
        }
    }
};