// ========================================================
// MEMORY
// Memory allocators and allocation utilities.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"
#include "./debug.hpp"

namespace ty
{
namespace mem
{

struct Arena
{
    byte* start = NULL;
    u64 offset = 0;
    u64 capacity = 0;
};

Arena*  MakeArena(u64 size);
void    DestroyArena(Arena* arena);

void*   ArenaPush(Arena* arena, u64 size);
void*   ArenaPush(Arena* arena, u64 size, u64 alignment);
void*   ArenaPushZero(Arena* arena, u64 size);
void*   ArenaPushZero(Arena* arena, u64 size, u64 alignment);
void*   ArenaPushCopy(Arena* arena, u64 size, void* srcData, u64 srcSize);
void    ArenaPop(Arena* arena, u64 size);
void    ArenaClear(Arena* arena);
void*   ArenaGetTop(Arena* arena);
void    ArenaFallback(Arena* arena, u64 newOffset);

#define MEM_ARENA_CHECKPOINT_SET(ARENA, NAME) u64 CONCATENATE(NAME, __fallback) = (ARENA)->offset
#define MEM_ARENA_CHECKPOINT_RESET(ARENA, NAME) ty::mem::ArenaFallback((ARENA), CONCATENATE(NAME, __fallback))

};
};
