#pragma once

#include <cstdint>
#include <type_traits>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

enum Axis2
{
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT
};

template <typename T> struct Vec2
{
    static_assert(std::is_arithmetic<T>::value, "Divisor must be arithmetic type");

    T x;
    T y;

    Vec2() : x(0), y(0)
    {
    }

    Vec2(const T& x, const T& y) : x(x), y(y)
    {
    }

    Vec2(const T&& x, const T&& y) : x(x), y(y)
    {
    }

    inline Vec2
    operator+(const Vec2& other) const
    {
        return {x + other.x, y + other.y};
    }

    inline Vec2
    operator-(const Vec2& other) const
    {
        return {x - other.x, y - other.y};
    }

    inline Vec2
    operator/(const f32& divisor) const
    {
        return Vec2(x / divisor, y / divisor);
    }

    inline Vec2
    operator/(const Vec2<f32>& divisor) const
    {
        return Vec2(x / divisor.x, y / divisor.y);
    }

    inline Vec2
    operator*(const f32& multiplier) const
    {
        return Vec2(x * multiplier, y * multiplier);
    }

    inline Vec2
    operator*(const Vec2<T>& multiplier) const
    {
        return Vec2(x * multiplier.x, y * multiplier.y);
    }

    inline bool
    operator<(const T& comparator) const
    {
        static_assert(std::is_floating_point<T>::value, "Divisor must be arithmetic type");
        return (x < comparator && y < comparator);
    }

    inline bool
    operator>(const T& comparator) const
    {
        static_assert(std::is_floating_point<T>::value, "Divisor must be arithmetic type");
        return (x > comparator && y > comparator);
    }

    template <typename To> operator Vec2<To>() const
    {
        static_assert(std::is_floating_point<To>::value, "Divisor must be arithmetic type");
        return Vec2<To>(static_cast<To>(x), static_cast<To>(y));
    }
};

union F32Vec4
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    } axis;
    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    } color;

    struct
    {
        Vec2<f32> p0;
        Vec2<f32> p1;
    } point;

    f32 data[4];
};

struct Vec3
{
    f32 x;
    f32 y;
    f32 z;

    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z)
    {
    }

    Vec3
    operator+(const Vec3& other) const
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vec3
    operator-(const Vec3& other) const
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vec3
    operator/(const f32 divisor)
    {
        return Vec3(x / divisor, x / divisor, z / divisor);
    }
};
