#include "./memory.hpp"
#include "./base.hpp"
#include "./debug.hpp"
#include <vcruntime_string.h>

namespace ty
{
namespace mem
{

Arena* MakeArena(u64 size)
{
    void* arenaMemory = malloc(size + sizeof(Arena));
    Arena* arena = (Arena*)arenaMemory;
    arena->start = (byte*)((u64)arenaMemory + sizeof(Arena));
    arena->offset = 0;
    arena->capacity = size;
    return arena;
}

void DestroyArena(Arena* arena)
{
    free(arena);
    arena = NULL;
}

void* ArenaPush(Arena* arena, u64 size)
{
    ASSERT(size > 0);
    ASSERT(arena->offset + size <= arena->capacity);
    byte* result = arena->start + arena->offset;
    arena->offset += size;
    return result;
}

void* ArenaPush(Arena* arena, u64 size, u64 alignment)
{
    ASSERT(size > 0);
    byte* arenaTop = arena->start + arena->offset;
    byte* arenaTopAligned = (byte*)ALIGN_TO((u64)arenaTop, alignment);
    u64 newOffset = arena->offset + (arenaTopAligned - arenaTop) + size;
    ASSERT(newOffset <= arena->capacity);
    arena->offset = newOffset;
    return arenaTopAligned;
}

void* ArenaPushZero(Arena* arena, u64 size)
{
    void* result = ArenaPush(arena, size);
    memset(result, 0, size);
    return result;
}

void* ArenaPushZero(Arena* arena, u64 size, u64 alignment)
{
    void* result = ArenaPush(arena, size, alignment);
    memset(result, 0, size);
    return result;
}

void* ArenaPushCopy(Arena* arena, u64 size, void* srcData, u64 srcSize)
{
    void* result = ArenaPush(arena, size);
    if(srcData)
    {
        memcpy(result, srcData, srcSize);
    }
    return result;
}

void ArenaPop(Arena* arena, u64 size)
{
    ASSERT(arena->offset >= size);
    arena->offset -= size;
}

void ArenaClear(Arena* arena)
{
    arena->offset = 0;
}

void* ArenaGetTop(Arena* arena)
{
    return arena->start + arena->offset;
}

void ArenaFallback(Arena* arena, u64 newOffset)
{
    ASSERT(newOffset <= arena->offset);
    arena->offset = newOffset;
}

Arena* GetScratchArena()
{
    static Arena* threadScratchArena = MakeArena(MB(256));
    return threadScratchArena;
}

};
};
