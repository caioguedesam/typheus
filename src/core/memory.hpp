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

// ========================================================
// [MEMORY REGION]
// Contiguous memory allocated with c malloc/free.
struct Region
{
    u8* start = NULL;
    i64 capacity = 0;
};

Region  AllocateRegion(i64 capacity);
void    FreeRegion(Region* region);

// ========================================================
// [CONTEXT]
// Memory allocation is handled through a set of callbacks (Alloc, Free, etc.)
// that operate in regions. The callbacks are all switched whenever the allocation
// context changes, when the user changes the active allocator. Each allocator type
// implements each callback differently.

// These are standard calls for each callback, with an assert for invalid operation.
void* InvalidAlloc(i64 size);
void* InvalidAllocAlign(i64 size, i64 alignment);
void* InvalidAllocZero(i64 size);
void* InvalidAllocAlignZero(i64 size, i64 alignment);
void* InvalidRealloc(void* data, i64 size);
void* InvalidReallocAlign(void* data, i64 size, i64 alignment);
void  InvalidFree(void* data);
void  InvalidFreeAll();

inline void* ctxAllocator = NULL;

//TODO(caio): Implement aligned allocation functions
inline void* (*Alloc)(i64 size) = InvalidAlloc;
inline void* (*AllocAlign)(i64 size, i64 alignment) = InvalidAllocAlign;
inline void* (*AllocZero)(i64 size) = InvalidAllocZero;
inline void* (*AllocAlignZero)(i64 size, i64 alignment) = InvalidAllocAlignZero;
inline void* (*Realloc)(void* data, i64 size) = InvalidRealloc;
inline void* (*ReallocAlign)(void* data, i64 size, i64 alignment) = InvalidReallocAlign;
inline void  (*Free)(void* data) = InvalidFree;
inline void  (*FreeAll)() = InvalidFreeAll;

void ResetContext();

// ========================================================
// [ARENA]
// Simple stack-like allocator that only supports free all.
// Meant for fast allocations that don't require freeing or
// can be freed in one go.
struct ArenaAllocator
{
    Region region = {};
    i64 offset = 0;
};
ArenaAllocator  MakeArenaAllocator(i64 capacity);
void            DestroyArenaAllocator(ArenaAllocator* arena); 

void  SetContext(ArenaAllocator* arena);
void* ArenaAlloc(i64 size);
void* ArenaAllocZero(i64 size);
void  ArenaFreeAll();

// ========================================================
// [HEAP]
// General purpose free list allocator.
struct HeapAllocator
{
    //TODO(caio): How to reduce these sizes? 16 bytes seems overkill
    struct AllocationHeader
    {
        i64 size = 0;       // The memory usable in this block
        i64 offset = 0;     // The offset from the block start until the header (padding)
    };

    struct FreeHeader
    {
        i64 size = 0;               // The available memory for allocation from this block
        FreeHeader* next = NULL;    // The next free header in the list
    };

    STATIC_ASSERT(sizeof(AllocationHeader) == sizeof(FreeHeader));

    Region region = {};
    i64 used = 0;
    FreeHeader* head = NULL;

    FreeHeader* FindFreeBlock(i64 size, i64 alignment, FreeHeader** prev);

    void AddFreeBlock(FreeHeader* block, FreeHeader* prev);
    void RemoveFreeBlock(FreeHeader* block, FreeHeader* prev);
    FreeHeader* Coalesce(FreeHeader* lhs, FreeHeader* rhs);

    void*   Alloc(i64 size, i64 alignment = 1);
    void*   Realloc(void* data, i64 size, i64 alignment = 1);
    void    Free(void* data);
    void    FreeAll();

    void    DebugPrint();
};
HeapAllocator   MakeHeapAllocator(i64 capacity);
void            DestroyHeapAllocator(HeapAllocator* heap);

// Heap alloc interface
void  SetContext(HeapAllocator* heap);
void* HeapAlloc(i64 size);
void* HeapAllocZero(i64 size);
void* HeapAllocAlign(i64 size, i64 alignment);
void* HeapAllocAlignZero(i64 size, i64 alignment);
void* HeapRealloc(void* data, i64 size);
void* HeapReallocAlign(void* data, i64 size, i64 alignment);
void  HeapFree(void* data);
void  HeapFreeAll();

};
};
