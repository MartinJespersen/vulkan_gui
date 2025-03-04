#pragma once

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
struct u128
{
    u64 data[2];
};

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

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
Str8(Arena* arena, const char* str, ...);

char*
CharFromStr8(String8 str);

// geometry
enum Axis2
{
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT
};

template <typename T> struct Vec2
{
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
    operator+(const f32 adder) const
    {
        return {x + adder, y + adder};
    }

    inline Vec2
    operator-(const Vec2& other) const
    {
        return {x - other.x, y - other.y};
    }

    inline Vec2
    operator-(const f32 substractor) const
    {
        return {x - substractor, y - substractor};
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
        return (x < comparator && y < comparator);
    }

    inline bool
    operator>(const T& comparator) const
    {
        return (x > comparator && y > comparator);
    }

    inline bool
    operator<=(const Vec2<T>& comparator) const
    {
        return (x <= comparator.x && y <= comparator.y);
    }

    inline bool
    operator>=(const Vec2<T>& comparator) const
    {
        return (x >= comparator.x && y >= comparator.y);
    }

    template <typename To> operator Vec2<To>() const
    {
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
