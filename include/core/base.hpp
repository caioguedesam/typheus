#pragma once
#include <windows.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <float.h>

#if _PROFILE
#include "tracy/Tracy.hpp"
#endif

namespace Ty
{

// [BASE TYPES]
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

// [GENERAL DEFINES AND MACROS]

#define MAX_U8  (0xFF)
#define MAX_U16 (0xFFFF)
#define MAX_U32 (0xFFFFFFFFUL)
#define MAX_U64 (0xFFFFFFFFFFFFFFFFULL)

#define EPSILON_F32 FLT_EPSILON
#define EPSILON_F64 DBL_EPSILON

#undef KB
#undef MB
#undef GB
#define KB(V) ((V)   * 1024)
#define MB(V) (KB(V) * 1024)
#define GB(V) (MB(V) * 1024)

#undef MIN
#undef MAX
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, A, B) (MAX((A), MIN((V), (B))))
#define CLAMP_CEIL(V, A) MIN(V, A)
#define CLAMP_FLOOR(V, A) MAX(V, A)

#define STMT(S) do { S; } while(0)
#define STRINGIFY(S) #S     // For macro arg expansion
#define ALIGN_TO(SIZE, BOUND) (((SIZE) + (BOUND) - 1) & ~((BOUND) - 1))   // Aligns to powers of 2 only
#define IS_ALIGNED(SIZE, BOUND) ((u64)(SIZE) % (BOUND) == 0)

#define ArrayCount(arr) (sizeof(arr)/sizeof(*(arr)))
#define ArraySize(arr) (sizeof(arr))

#define StructOffset(type, member) ((u64)&((type*)0)->member)

// [ASSERT]
#if _NO_ASSERT
#define ASSERT(EXPR)
#define ASSERTF(EXPR, ...)
#else
void Assert(u64 expr, const char* msg);
void AssertFormat(u64 expr, const char* fmt, ...);

#define ASSERT(EXPR) STMT(Assert((u64)(EXPR), STRINGIFY(EXPR)))
#define ASSERTF(EXPR, FMT, ...) STMT(AssertFormat((i32)(EXPR), FMT, __VA_ARGS__))
#endif

// [LOGGING]

#if _NO_LOG
#define LOG(MSG)
#define LOGL(LABEL, MSG)
#define LOGF(MSG, ...)
#define LOGLF(LABEL, MSG, ...)
#else
void LogFormat(const char* label, const char* fmt, ...);
#define LOG(MSG) STMT(LogFormat("LOG", "%s", MSG))
#define LOGL(LABEL, MSG) STMT(LogFormat(LABEL, "%s", MSG))
#define LOGF(FMT, ...) STMT(LogFormat("LOG", FMT, __VA_ARGS__))
#define LOGLF(LABEL, FMT, ...) STMT(LogFormat(LABEL, FMT, __VA_ARGS__))
#endif

// [PROFILING]
#if _PROFILE
#define PROFILE_FRAME FrameMark
#define PROFILE_SCOPED ZoneScoped
//TODO(caio)#PROFILING: Add support for named zones and colors
#else
#define PROFILE_FRAME
#define PROFILE_SCOPED
#endif

// [MEMORY]
// Memory Arena
struct MemArena
{
    u64 capacity = 0;
    u8* start = 0;
    u64 offset = 0;
};

// Arena initialization
void  MemArenaInit(MemArena* arena, u64 capacity);
void  MemArenaDestroy(MemArena* arena);

// Arena allocation and free
void* MemAlloc(MemArena* arena, u64 size);
void* MemAllocAlign(MemArena* arena, u64 size, u64 alignment);
void* MemAllocZero(MemArena* arena, u64 size);
void* MemAllocAlignZero(MemArena* arena, u64 size, u64 alignment);
void  MemFree(MemArena* arena, u64 size);
void  MemClear(MemArena* arena);

// [ARRAY]
template<typename T>
struct Array
{
    T* data = 0;
    u64 count = 0;
    u64 capacity = 0;

    T& operator[](u64 i)
    {
        ASSERT(i < count);
        return data[i];
    }

    void Push(const T& value)
    {
        ASSERT(count + 1 <= capacity);
        data[count] = value;
        count++;
    }

    u64 Size()
    {
        return count * sizeof(T);
    }
};

template<typename T>
Array<T> ArrayAlloc(MemArena* arena, u64 capacity, u64 alignment = 0)
{
    Array<T> result;
    result.capacity = capacity;
    result.count = 0;
    result.data = alignment ?
        (T*)MemAllocAlign(arena, capacity * sizeof(T), alignment) :
        (T*)MemAlloc(arena, capacity * sizeof(T));
    return result;
}

template<typename T>
Array<T> ArrayAllocZero(MemArena* arena, u64 capacity, u64 alignment = 0)
{
    Array<T> result;
    result.capacity = capacity;
    result.count = 0;
    result.data = alignment ?
        (T*)MemAllocAlignZero(arena, capacity * sizeof(T), alignment) :
        (T*)MemAllocZero(arena, capacity * sizeof(T));
    return result;
}

// [RANDOM]
u64 RandomU64();
f32 RandomUniform();

i32 RandomRange(i32 start, i32 end);
f32 RandomRange(f32 start, f32 end);

// [STRING]
// A string object is not responsible for managing it's data's lifetime.
// This could be some memory in an arena, a buffer on the stack or a C string literal somewhere.
// Allocated string objects are compatible with C-strings, and don't consider null-term
// as part of their length.
#define STR_INVALID -1
struct String
{
    u8* data = 0;
    u64 len = 0;

    char* ToCStr();
};
String Str(u8* data, u64 len);
String Str(const char* cstr);

bool StrRead(String* str, u64 offset, u8* out, u64 n);
bool StrRead(String* str, u8* out);
// TODO(caio)#STRING: Make more StrRead overloads

String Strf(u8* buffer, const char* fmt, ...);

String StrAlloc(MemArena* arena, u8* data, u64 len);
String StrAllocZero(MemArena* arena, u64 len);
String StrAlloc(MemArena* arena, const char* cstr);
String StrfAlloc(MemArena* arena, const char* fmt, ...);

bool StrCompare(String str1, String str2);
bool StrCompare(String str1, String str2, u64 n);
bool IsCStr(String str);

String StrPrefix(String str, u64 n);
String StrSuffix(String str, u64 n);
String StrSubstr(String str, u64 start, u64 n);

i64 StrFind(String haystack, String needle);
i64 StrFindR(String haystack, String needle);

Array<String> StrSplit(MemArena* arena, String str, char delim = ' ');

}   // namespace Ty
