// TYPHEUS ENGINE - BASE LAYER
#pragma once
#include <windows.h>
#include <stdint.h>
#include <stdarg.h>

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

#define STMT(S) do { S; } while(0)
#define STRINGIFY(S) #S     // For macro arg expansion
#define ALIGN_TO(SIZE, BOUND) (((SIZE) + (BOUND) - 1) & ~((BOUND) - 1))   // Aligns to powers of 2 only
#define IS_ALIGNED(SIZE, BOUND) ((u64)(SIZE) % (BOUND) == 0)

#define ArrayCount(arr) (sizeof(arr)/sizeof(*(arr)))
#define ArraySize(arr) (sizeof(arr))

// [ASSERT]
#if _NO_ASSERT
#define ASSERT(EXPR)
#else
#define ASSERT(EXPR) STMT(\
    if(!(EXPR)) {\
        MessageBoxExA(\
                NULL,\
                STRINGIFY(EXPR),\
                "FAILED ASSERT",\
                MB_OK,\
                0);\
        DebugBreak();\
        ExitProcess(-1);\
    }\
    )
#endif

// [PROFILING]
#if _PROFILE
#include "tracy/Tracy.hpp"
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

// Arena macros
// Helpers to allocate arrays in arenas
#define PushArray(arena, type, count) (type*)MemAlloc((arena), sizeof(type) * (count))
#define PushArrayZero(arena, type, count) (type*)MemAllocZero((arena), sizeof(type) * (count))
#define PushArrayAlign(arena, type, count, alignment) (type*)MemAllocAlign((arena), sizeof(type) * (count), (alignment))
#define PushArrayAlignZero(arena, type, count, alignment) (type*)MemAllocAlignZero((arena), sizeof(type) * (count), (alignment))

// Helpers to allocate structs in arenas
#define PushStruct(arena, type) PushArray((arena), (type), 1)
#define PushStructZero(arena, type) PushArrayZero((arena), (type), 1)
#define PushStructAlign(arena, type) PushArrayAlign((arena), (type), 1)
#define PushStructZeroAlign(arena, type) PushArrayAlignZero((arena), (type), 1)

struct Array
{
    u8* data = 0;
    u64 count = 0;
    u64 capacity = 0;
};

Array ArrayInit_(MemArena* arena, u64 capacity, u64 alignment = 0);
Array ArrayInitZero_(MemArena* arena, u64 capacity, u64 alignment = 0);
#define ArrayInit(arena, type, capacity) (ArrayInit_((arena), sizeof(type) * (capacity)))
#define ArrayInitAlign(arena, type, capacity, alignment) (ArrayInit_((arena), sizeof(type) * (capacity), (alignment)))
#define ArrayInitZero(arena, type, capacity) (ArrayInitZero_((arena), sizeof(type) * (capacity)))
#define ArrayInitZeroAlign(arena, type, capacity, alignment) (ArrayInitZero_((arena), sizeof(type) * (capacity), (alignment)))

#define ArrayGet(array, type, index, out) STMT(\
            ASSERT((index) < (array).count);\
            ASSERT((out));\
            *(out) = ((type*)(array).data)[(index)];\
        )
#define ArraySet(array, type, index, value) STMT(\
            ASSERT((index) < (array).count);\
            ((type*)(array).data)[(index)] = (value);\
        )
#define ArrayPush(array, type, value) STMT(\
            ASSERT((array).count + 1 <= (array).capacity);\
            ((type*)(array).data)[(array).count] = (value);\
            (array).count += 1;\
        )

// [RANDOM]
u64 RandomU64();
f32 RandomUniform();

i32 RandomRange(i32 start, i32 end);
f32 RandomRange(f32 start, f32 end);

// [STRING]
// A string object is not responsible for managing it's data's lifetime.
// This could be some memory in an arena, a buffer on the stack or a C string literal somewhere.
struct String
{
    u8* data = 0;
    u64 len = 0;
};
String Str(u8* data, u64 len);
String Str(const char* cstr);

bool StrRead(String* str, u64 offset, u8* out, u64 size);
bool StrRead(String* str, u8* out);
// TODO(caio)#STRING: Make more StrRead overloads

String Strf(u8* buffer, const char* fmt, ...);

String StrAlloc(MemArena* arena, u8* data, u64 len);
String StrAlloc(MemArena* arena, const char* cstr);
String StrfAlloc(MemArena* arena, const char* fmt, ...);

// TODO(caio)#STRING: String comparison
// TODO(caio)#STRING: Finding substrings
// TODO(caio)#STRING: Split string by delimiter
