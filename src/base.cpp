// TYPHEUS ENGINE - BASE LAYER
#include "base.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>

void MemArenaInit(MemArena* arena, u64 capacity)
{
    //TODO(caio)#MEMORY: Using malloc for arena initializing, maybe change to win32 VirtualAlloc
    arena->capacity = capacity;
    arena->start = (u8*)malloc(capacity);
    arena->offset = 0;
}

void MemArenaDestroy(MemArena* arena)
{
    //TODO(caio)#MEMORY: Using free for arena destruction, maybe change to win32 VirtualFree
    arena->capacity = 0;
    arena->offset = 0;
    free(arena->start);
    arena->start = 0;
}

void* MemAlloc(MemArena* arena, u64 size)
{
    ASSERT(arena->offset + size <= arena->capacity);
    u8* result = arena->start + arena->offset;
    arena->offset += size;
    return result;
}

void* MemAllocAlign(MemArena* arena, u64 size, u64 alignment)
{
    ASSERT(ALIGN_TO(arena->offset, alignment) + size <= arena->capacity);
    arena->offset = ALIGN_TO(arena->offset, alignment);
    ASSERT(IS_ALIGNED(arena->offset, alignment));
    u8* result = arena->start + arena->offset;
    arena->offset += size;
    return result;
}

void* MemAllocZero(MemArena* arena, u64 size)
{
    ASSERT(arena->offset + size <= arena->capacity);
    u8* result = arena->start + arena->offset;
    arena->offset += size;
    memset(result, 0, size);
    return result;
}

void* MemAllocAlignZero(MemArena* arena, u64 size, u64 alignment)
{
    ASSERT(ALIGN_TO(arena->offset, alignment) + size <= arena->capacity);
    arena->offset = ALIGN_TO(arena->offset, alignment);
    u8* result = arena->start + arena->offset;
    arena->offset += size;
    memset(result, 0, size);
    return result;
}

void MemFree(MemArena* arena, u64 size)
{
    arena->offset = size > arena->offset ? 0 : arena->offset - size;
}

void MemClear(MemArena* arena)
{
    arena->offset = 0;
}

Array ArrayInit_(MemArena* arena, u64 capacity, u64 alignment)
{
    Array result;
    result.capacity = capacity;
    result.count = 0;
    result.data = alignment ?
        (u8*)MemAllocAlign(arena, capacity, alignment) :
        (u8*)MemAlloc(arena, capacity);

    return result;
}

Array ArrayInitZero_(MemArena* arena, u64 capacity, u64 alignment)
{
    Array result;
    result.capacity = capacity;
    result.count = 0;
    result.data = alignment ?
        (u8*)MemAllocAlignZero(arena, capacity, alignment) :
        (u8*)MemAllocZero(arena, capacity);

    return result;
}

u64 RandomU64()
{
    // Xorshift*64
    static u64 x = __rdtsc();   // TODO(caio)#THREAD: This shouldn't be static when doing multithreading!
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
}

f32 RandomUniform()
{
    return (f32)RandomU64()/(f32)MAX_U64;
}

i32 RandomRange(i32 start, i32 end)
{
    // Random int between start (inclusive) and end (inclusive)
    return start + RandomUniform() * (end + 1 - start);
}

f32 RandomRange(f32 start, f32 end)
{
    return start + RandomUniform() * (end - start);
}

String Str(u8* data, u64 len)
{
    return
    {
        data, len
    };
}

String Str(const char* cstr)
{
    u64 len = 0;
    for(; cstr[len] != 0; len++);
    return
    {
        (u8*)cstr, len
    };
}

bool StrRead(String* str, u64 offset, u8* out, u64 size)
{
    // This trusts the output buffer to have enough size for the string.
    ASSERT(out);
    bool result = false;
    if(offset + size <= str->len)
    {
        result = true;
        memcpy(out, (str->data + offset), size);
    }
    return result;
};

bool StrRead(String* str, u8* out)
{
    return StrRead(str, 0, out, str->len);
};

String Strf(u8* buffer, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    String result;
    u64 len = vsprintf((char*)buffer, fmt, args);    // This assumes buffer is big enough to store the string.
    result.data = buffer;
    result.len = len;

    va_end(args);
    return result;
}

String StrAlloc(MemArena* arena, u8* data, u64 len)
{
    String result;
    result.data = (u8*)MemAlloc(arena, len);
    memcpy(result.data, data, len);
    result.len = len;
    return result;
}

String StrAlloc(MemArena* arena, const char* cstr)
{
    String result;
    u64 len = 0;
    for(; cstr[len] != 0; len++);
    result.data = (u8*)MemAlloc(arena, len + 1);    // +1 for null terminator
    memcpy(result.data, cstr, len + 1);             // +1 for null terminator
    result.len = len;
    return result;
}

String StrfAlloc(MemArena* arena, const char* fmt, ...)
{
    va_list args;
    va_list args2;
    va_start(args, fmt);
    va_copy(args2, args);

    String result;
    // First, try to allocate string on 128-byte buffer.
    u64 allocSize = 128;
    u8* allocBuffer = (u8*)MemAlloc(arena, allocSize);
    u64 strfSize = vsnprintf((char*)allocBuffer, allocSize, fmt, args);
    if(strfSize < allocSize)
    {
        MemFree(arena, allocSize - strfSize - 1);   // -1 for null terminator.
        result = {allocBuffer, strfSize};
    }
    // If it doesn't work, try again with the proper formatted size.
    else
    {
        MemFree(arena, allocSize);
        allocBuffer = (u8*)MemAlloc(arena, strfSize + 1);    // +1 for null terminator.
        strfSize = vsnprintf((char*)allocBuffer, strfSize + 1, fmt, args2);
        result = {allocBuffer, strfSize};
    }

    va_end(args);
    va_end(args2);
    return result;
}
