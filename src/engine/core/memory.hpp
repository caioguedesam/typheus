// ========================================================
// MEMORY
// Memory allocators and allocation utilities.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"

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
    u64 capacity = 0;
};

Region  AllocateRegion(u64 capacity);
void    FreeRegion(Region* region);

// ========================================================
// [CONTEXT]
// Memory allocation is handled through a set of callbacks (Alloc, Free, etc.)
// that operate in regions. The callbacks are all switched whenever the allocation
// context changes, when the user changes the active allocator. Each allocator type
// implements each callback differently.

// These are standard calls for each callback, with an assert for invalid operation.
void* InvalidAlloc(u64 size);
void* InvalidAllocZero(u64 size);
void* InvalidRealloc(void* data, u64 size);
void  InvalidFree(void* data);
void  InvalidFreeAll();

inline void* ctxAllocator = NULL;

//TODO(caio): Implement aligned allocation functions
inline void* (*Alloc)(u64 size) = InvalidAlloc;
inline void* (*AllocZero)(u64 size) = InvalidAllocZero;
inline void* (*Realloc)(void* data, u64 size) = InvalidRealloc;
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
    u64 offset = 0;
};
ArenaAllocator  InitArenaAllocator(u64 capacity);
void            DestroyArenaAllocator(ArenaAllocator* arena); 

void  SetContext(ArenaAllocator* arena);
void* ArenaAlloc(u64 size);
void* ArenaAllocZero(u64 size);
void  ArenaFreeAll();

// ========================================================
// [HEAP]
// General purpose free list allocator.
enum HeapAllocatorStrategy
{
    FIND_FIRST,
    FIND_BEST,
};

struct HeapAllocationHeader
{
    u64 blockSize = 0;
    //TODO(caio): Alignment and padding
    u64 padding = 0;    // AllocationHeader MUST have same size as FreeListNode
                        // This member will be used when implementing aligned allocs
};

struct HeapAllocatorFreeListNode
{
    HeapAllocatorFreeListNode* next = nullptr;
    u64 blockSize = 0;
};

struct HeapAllocator
{
    Region region = {};
    u64 used = 0;

    HeapAllocatorFreeListNode* head = nullptr;
    HeapAllocatorStrategy strategy = FIND_FIRST;

    void InsertNode(HeapAllocatorFreeListNode* prev, HeapAllocatorFreeListNode* node);
    void RemoveNode(HeapAllocatorFreeListNode* prev, HeapAllocatorFreeListNode* node);
    void Coalesce(HeapAllocatorFreeListNode* prev, HeapAllocatorFreeListNode* node);

    HeapAllocatorFreeListNode* FindFirst(u64 size, HeapAllocatorFreeListNode** prev);
    HeapAllocatorFreeListNode* FindBest(u64 size, HeapAllocatorFreeListNode** prev);
};
HeapAllocator   InitHeapAllocator(u64 capacity);
void            DestroyHeapAllocator(HeapAllocator* heap);

void  SetContext(HeapAllocator* heap);
void* HeapAlloc(u64 size);
void* HeapAllocZero(u64 size);
void* HeapRealloc(void* data, u64 size);
void  HeapFree(void* data);
void  HeapFreeAll();

};
};
