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

bool StrRead(String* str, u64 offset, u8* out, u64 n)
{
    // This trusts the output buffer to have enough size for the string.
    ASSERT(out);
    bool result = false;
    if(offset + n <= str->len)
    {
        result = true;
        memcpy(out, (str->data + offset), n);
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

bool StrCompare(String str1, String str2)
{
    if(str1.len != str2.len) return false;
    return memcmp(str1.data, str2.data, str1.len) == 0;
}

bool StrCompare(String str1, String str2, u64 n)
{
    if(str1.len < n || str2.len < n) return false;
    return memcmp(str1.data, str2.data, str1.len) == 0;
}

String StrPrefix(String str, u64 n)
{
    ASSERT(str.len >= n);
    return
    {
        str.data,
        n
    };
}

String StrSuffix(String str, u64 n)
{
    ASSERT(str.len >= n);
    return
    {
        str.data + str.len - n,
        n
    };
}

String StrSubstr(String str, u64 start, u64 n)
{
    ASSERT(str.len >= start + n);
    return
    {
        str.data + start,
        n
    };
}

i64 StrFind(String haystack, String needle)
{
    if(needle.len > haystack.len) return -1;
    // TODO(caio)#STRING: This is naive string search. Improve only if needed.
    i64 cursor = 0;
    i64 match = -1;
    while(cursor < haystack.len)
    {
        if(haystack.data[cursor] == needle.data[0])
        {
            match = cursor;
            for(i64 needleCursor = 1; needleCursor < needle.len; needleCursor++)
            {
                if(haystack.data[cursor + needleCursor] != needle.data[needleCursor])
                {
                    cursor = match;
                    match = -1;
                    break;
                }
            }
            if(match != -1) break;
        }
        cursor++;
    }

    return match;
}

// TODO(caio)#CONTINUE: Continue from here. Implement and test this. StrFind might need more tests aswell.
i64 StrFindR(String haystack, String needle)
{
    if(needle.len > haystack.len) return -1;
    i64 cursor = haystack.len - (needle.len - 1);
    i64 match = -1;
    while(cursor >= 0)
    {
        if(haystack.data[cursor] == needle.data[0])
        {
            match = cursor;
            for(i64 needleCursor = 1; needleCursor < needle.len; needleCursor++)
            {
                if(haystack.data[cursor + needleCursor] != needle.data[needleCursor])
                {
                    cursor = match;
                    match = -1;
                    break;
                }
            }
        }
        if(match != -1) break;
        cursor--;
    }
    return match;  // IMPLEMENT ME
}

Array StrSplit(MemArena* arena, String str, char delim)
{
    // Iterate once to find array size to allocate.
    i64 splitCount = 0;
    for(i64 i = 0; i < str.len; i++)
    {
        if(str.data[i] == delim) splitCount++;
    }

    Array result = ArrayInit(arena, String, splitCount + 1);
    // Didn't find delimiter char, so just return original string.
    if(splitCount == 0)
    {
        ArrayPush(result, String, str);
        return result;
    }

    // Iterate string to find delimiter, split when found.
    i64 lastSplit = 0;
    for(i64 i = 0; i < str.len; i++)
    {
        if(str.data[i] == delim)
        {
            u64 splitLen = (u64)(i - lastSplit);
            String split = { str.data + lastSplit, splitLen };
            ArrayPush(result, String, split);
            lastSplit = i + 1;
        }
    }
    String last = { str.data + lastSplit, str.len - lastSplit };
    ArrayPush(result, String, last);

    return result;
}











