#include "./memory.hpp"
#include "./base.hpp"
#include "./debug.hpp"
#include <vcruntime_string.h>

namespace ty
{
namespace mem
{

Region AllocateRegion(i64 capacity)
{
    Region result = {};
    result.start = (u8*)malloc(capacity);
    result.capacity = capacity;
    return result;
}

void FreeRegion(Region* region)
{
    free(region->start);
    *region = {};
}

void* InvalidAlloc(i64 size)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidAllocAlign(i64 size, i64 alignment)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidAllocZero(i64 size)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidAllocAlignZero(i64 size, i64 alignment)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidRealloc(void* data, i64 size)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidReallocAlign(void* data, i64 size, i64 alignment)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void InvalidFree(void* data)
{
    ASSERT(0);  // Context is invalid for this operation
}

void InvalidFreeAll()
{
    ASSERT(0);  // Context is invalid for this operation
}

void ResetContext()
{
    ctxAllocator = nullptr;
    Alloc = InvalidAlloc;
    AllocZero = InvalidAllocZero;
    Realloc = InvalidRealloc;
    Free = InvalidFree;
    FreeAll = InvalidFreeAll;
}

ArenaAllocator MakeArenaAllocator(i64 capacity)
{
    ArenaAllocator result = {};
    result.region = AllocateRegion(capacity);
    result.offset = 0;
    return result;
}

void DestroyArenaAllocator(ArenaAllocator* arena)
{
    if(arena == ctxAllocator) ResetContext();
    FreeRegion(&arena->region);
    *arena = {};
}

void SetContext(ArenaAllocator* arena)
{
    ctxAllocator = arena;
    Alloc = ArenaAlloc;
    AllocZero = ArenaAllocZero;
    AllocAlign = InvalidAllocAlign;
    AllocAlignZero = InvalidAllocAlignZero;
    Realloc = InvalidRealloc;
    ReallocAlign = InvalidReallocAlign;
    Free = InvalidFree;
    FreeAll = ArenaFreeAll;
}

void* ArenaAlloc(i64 size)
{
    ASSERT(size > 0);
    ArenaAllocator* arena = (ArenaAllocator*)ctxAllocator;
    ASSERT(arena);
    ASSERT(arena->offset + size <= arena->region.capacity);
    u8* result = arena->region.start + arena->offset;
    arena->offset += size;
    return result;
}

void* ArenaAllocZero(i64 size)
{
    void* result = ArenaAlloc(size);
    memset(result, 0, size);
    return result;
}

void ArenaFreeAll()
{
    ArenaAllocator* arena = (ArenaAllocator*)ctxAllocator;
    ASSERT(arena);
    arena->offset = 0;
}

bool HeapAllocator_CanCoalesce(HeapAllocator::FreeHeader* lhs, HeapAllocator::FreeHeader* rhs)
{
    if(!lhs || !rhs) return false;
    return (i64)lhs + sizeof(HeapAllocator::FreeHeader) + lhs->size == (i64)rhs;
}

void HeapAllocator::AddFreeBlock(FreeHeader *block, FreeHeader *prev)
{
    ASSERT(block);
    if(!prev)
    {
        block->next = head;
        head = block;
    }
    else
    {
        FreeHeader* prevNext = prev->next;
        prev->next = block;
        block->next = prevNext;
    }
}

void HeapAllocator::RemoveFreeBlock(FreeHeader *block, FreeHeader *prev)
{
    ASSERT(block);
    if(block == head)
    {
        head = block->next;
    }
    else
    {
        prev->next = block->next;
    }
}

HeapAllocator::FreeHeader* HeapAllocator::Coalesce(FreeHeader *lhs, FreeHeader *rhs)
{
    ASSERT(HeapAllocator_CanCoalesce(lhs, rhs));
    lhs->size += rhs->size + sizeof(FreeHeader);
    RemoveFreeBlock(rhs, lhs);
    return lhs;
}

void HeapAllocator::Free(void* data)
{
    if(!data) return;

    // Create free block where allocation was
    HeapAllocator::AllocationHeader* allocHeader = (HeapAllocator::AllocationHeader*)((i64)data - sizeof(HeapAllocator::AllocationHeader));

    //LOGLF("HEAP FREE", "%p(%d)", data, allocHeader->size);

    i64 blockSize = allocHeader->size + allocHeader->offset + sizeof(HeapAllocator::AllocationHeader);
    HeapAllocator::FreeHeader* freeHeader = (HeapAllocator::FreeHeader*)((i64)allocHeader - allocHeader->offset);
    freeHeader->size = blockSize - sizeof(HeapAllocator::FreeHeader);
    freeHeader->next = NULL;

    // Insert new free block on free list at appropriate position
    // (address-ordered)
    HeapAllocator::FreeHeader* current = head;
    HeapAllocator::FreeHeader* prev = NULL;
    while(current)
    {
        if(current > freeHeader)
        {
            AddFreeBlock(freeHeader, prev);
            break;
        }
        prev = current;
        current = current->next;
    }

    // Coalesce (merge two free blocks together)
    if(HeapAllocator_CanCoalesce(prev, freeHeader))
    {
        freeHeader = Coalesce(prev, freeHeader);
    }
    if(HeapAllocator_CanCoalesce(freeHeader, freeHeader->next))
    {
        freeHeader = Coalesce(freeHeader, freeHeader->next);
    }

    // Update heap allocator internals
    i64 oldUsed = used;
    used -= blockSize;
    ASSERT(oldUsed >= used);  // Guard from underflow
}

HeapAllocator::FreeHeader* HeapAllocator::FindFreeBlock(i64 size, i64 alignment, HeapAllocator::FreeHeader** prev)
{
    ASSERT(size > 0);
    ASSERT(IS_POW2(alignment));
    ASSERT(IS_ALIGNED(size, alignment));
    // TODO(caio): Add find best strategy

    FreeHeader* currentBlock = head;
    FreeHeader* prevBlock = NULL;

    while(currentBlock)
    {
        // First, calculate the offset for the allocation header, for alignment purposes.
        i64 currentBlockStart = ALIGN_TO((i64)currentBlock, alignment);
        i64 currentBlockOffset = currentBlockStart - (i64)currentBlock;

        // Free block always has at least allocation header size. But we still need
        // to see if it supports the offset and the requested memory size.
        if(currentBlock->size >= size + currentBlockOffset)
        {
            break;
        }

        prevBlock = currentBlock;
        currentBlock = currentBlock->next;
    }
    ASSERT(currentBlock);    // If 0, couldn't find a free node.
    if(prev) *prev = prevBlock;
    return currentBlock;
}

void HeapAllocator::FreeAll()
{
    used = 0;
    head = (HeapAllocator::FreeHeader*)region.start;
    *head = {};
    head->size = region.capacity - sizeof(HeapAllocator::FreeHeader);
    head->next = NULL;
}

void* HeapAllocator::Alloc(i64 size, i64 alignment)
{
    ASSERT(size > 0);
    ASSERT(IS_POW2(alignment));
    ASSERT(IS_ALIGNED(size, alignment));

    // Allocation requires given size and space for the header, plus additional padding
    // (which is at most alignment - 1 | TODO(caio): Verify)
    ASSERT(used + size + sizeof(HeapAllocator::AllocationHeader) + alignment - 1 <= region.capacity);

    HeapAllocator::FreeHeader* prevBlock = NULL;
    HeapAllocator::FreeHeader* freeBlock = FindFreeBlock(size, alignment, &prevBlock); 
    HeapAllocator::FreeHeader* nextFreeBlock = freeBlock->next;

    // Check memory between allocation and next block.
    // If there's space for a free block, make a new one.
    // If there isn't, add the remaining space to the size of the allocation.
    i64 allocAddr = ALIGN_TO((i64)freeBlock + sizeof(HeapAllocator::AllocationHeader), alignment);
    i64 allocHeaderOffset = ((i64)allocAddr - sizeof(HeapAllocator::AllocationHeader)) - (i64)freeBlock;

    i64 allocBlockSize = size + allocHeaderOffset + sizeof(HeapAllocator::AllocationHeader);
    i64 freeBlockSize = freeBlock->size + sizeof(HeapAllocator::FreeHeader);
    i64 remainder = freeBlockSize - allocBlockSize;
    if(remainder > (i64)sizeof(HeapAllocator::FreeHeader))
    {
        HeapAllocator::FreeHeader* remainderFreeBlock = (HeapAllocator::FreeHeader*)((i64)freeBlock + sizeof(HeapAllocator::FreeHeader) + freeBlock->size - remainder);
        *remainderFreeBlock = {};
        remainderFreeBlock->size = remainder - sizeof(HeapAllocator::FreeHeader);
        AddFreeBlock(remainderFreeBlock, freeBlock);
    }
    else
    {
        size += remainder;
    }

    // Remove selected free block from list
    RemoveFreeBlock(freeBlock, prevBlock);

    // Setup allocation and return
    HeapAllocator::AllocationHeader* header = (HeapAllocator::AllocationHeader*)((i64)freeBlock + allocHeaderOffset);
    *header = {};
    header->size = size;
    header->offset = allocHeaderOffset;

    i64 oldUsed = used;
    used += size + sizeof(HeapAllocator::AllocationHeader) + allocHeaderOffset;
    ASSERT(oldUsed <= used);  // Guard from overflow


    ASSERT(IS_ALIGNED((void*)((i64)header + sizeof(HeapAllocator::AllocationHeader)), alignment));
    void* result = (void*)((i64)header + sizeof(HeapAllocator::AllocationHeader));
    //LOGLF("HEAP ALLOC", "%p(%d)", result, size);
    return result;
}

void* HeapAllocator::Realloc(void* data, i64 size, i64 alignment)
{
    ASSERT(size > 0);
    ASSERT(IS_POW2(alignment));
    ASSERT(IS_ALIGNED(size, alignment));
    if(!data)
    {
        return Alloc(size, alignment);
    }

    HeapAllocator::AllocationHeader* header = (HeapAllocator::AllocationHeader*)((i64)data - sizeof(HeapAllocator::AllocationHeader));

    // Best case: there's enough space in an adjacent free block to data
    // for the realloc.
    // First, check if there's an adjacent free block that is in the free list.
    i64 neededSize = size - header->size;
    if(neededSize == 0)
    {
        return data;
    }
    else if(neededSize < 0)
    {
        //  TODO(caio): Realloc reduce (split current memory block).
        //  This currently just goes to worst case, which works but is less memory efficient.
        //  Note: should take into account if the new free block from the reduced alloc can
        //  fit the header size.
    }
    HeapAllocator::FreeHeader* adjacentFreeBlock = (HeapAllocator::FreeHeader*)((i64)data + header->size);

    HeapAllocator::FreeHeader* prevFreeBlock = NULL;
    HeapAllocator::FreeHeader* currFreeBlock = head;
    while(currFreeBlock)
    {
        if(currFreeBlock == adjacentFreeBlock)
        {
            break;
        }
        prevFreeBlock = currFreeBlock;
        currFreeBlock = currFreeBlock->next;
    }

    if(currFreeBlock)
    {
        i64 freeBlockSize = adjacentFreeBlock->size + sizeof(HeapAllocator::FreeHeader);
        i64 freeBlockSizeAfterRealloc = (i64)freeBlockSize - (i64)neededSize;
        if(freeBlockSizeAfterRealloc > (i64)sizeof(HeapAllocator::FreeHeader))
        {
            // Create new free block with reduced size after realloc
            HeapAllocator::FreeHeader* nextFreeBlock = adjacentFreeBlock->next;
            RemoveFreeBlock(adjacentFreeBlock, prevFreeBlock);
            HeapAllocator::FreeHeader* newFreeBlock = (HeapAllocator::FreeHeader*)((i64)adjacentFreeBlock + freeBlockSize - freeBlockSizeAfterRealloc);
            *newFreeBlock = {};
            newFreeBlock->size = freeBlockSizeAfterRealloc - sizeof(HeapAllocator::FreeHeader);
            newFreeBlock->next = nextFreeBlock;
            AddFreeBlock(newFreeBlock, prevFreeBlock);

            header->size += neededSize;
            used += neededSize;
            return data;
        }
        else if(freeBlockSizeAfterRealloc > 0)
        {
            // Realloc consumes free block entirely, to avoid untracked regions
            RemoveFreeBlock(adjacentFreeBlock, prevFreeBlock);
            header->size += freeBlockSize;
            used += freeBlockSize;
            return data;
        }
        // If the free block size is negative, it's not enough to contain the realloc,
        // so move to worst case.
    }

    // Worst case: allocate new block and move data
    void* result = Alloc(size, alignment);
    // Copy the minimum memory that fits in the new block
    memcpy(result, data, header->size < size ? header->size : size);
    Free(data);
    return result;
}

void SetContext(HeapAllocator* heap)
{
    ctxAllocator = heap;
    Alloc = HeapAlloc;
    AllocAlign = HeapAllocAlign;
    AllocZero = HeapAllocZero;
    AllocAlignZero = HeapAllocAlignZero;
    Realloc = HeapRealloc;
    ReallocAlign = HeapReallocAlign;
    Free = HeapFree;
    FreeAll = HeapFreeAll;
}

void* HeapAlloc(i64 size)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    return heap->Alloc(size, 1);
}

void* HeapAllocAlign(i64 size, i64 alignment)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    return heap->Alloc(size, alignment);
}

void* HeapAllocZero(i64 size)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    void* result = heap->Alloc(size, 1);
    memset(result, 0, size);
    return result;
}

void* HeapAllocAlignZero(i64 size, i64 alignment)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    void* result = heap->Alloc(size, alignment);
    memset(result, 0, size);
    return result;
}

void* HeapRealloc(void* data, i64 size)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    return heap->Realloc(data, size, 1);
}

void* HeapReallocAlign(void* data, i64 size, i64 alignment)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    return heap->Realloc(data, size, alignment);
}

void HeapFree(void *data)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    heap->Free(data);
}

void HeapFreeAll()
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    heap->FreeAll();
}

HeapAllocator MakeHeapAllocator(i64 capacity)
{
    HeapAllocator result = {};
    result.region = AllocateRegion(capacity);

    result.used = 0;
    result.head = (HeapAllocator::FreeHeader*)result.region.start;
    *result.head = {};
    result.head->size = result.region.capacity - sizeof(HeapAllocator::FreeHeader);
    result.head->next = NULL;
    return result;
}

void DestroyHeapAllocator(HeapAllocator* heap)
{
    if(heap == ctxAllocator) ResetContext();
    FreeRegion(&heap->region);
    *heap = {};
}

void HeapAllocator::DebugPrint()
{
    // Clear console
    system("cls");

    LOGLF("HEAP_ALLOC", "Heap allocator [%llu] (%llu bytes used of %llu)", (i64)region.start, used, region.capacity);
    FreeHeader* current = head;

    if((i64)current != (i64)region.start)
    {
        i64 distance = (i64)current - (i64)region.start;
        for(i32 i = 0; i < distance; i++)
        {
            printf("/");
        }
    }

    while(current)
    {
        printf("|");
        for(i32 i = 0; i < sizeof(FreeHeader); i++)
        {
            printf("#");
        }
        for(i32 i = 0; i < current->size; i++)
        {
            printf("-");
        }
        if(current->next)
        {
            i64 distance = (i64)current->next - ((i64)current + current->size + sizeof(FreeHeader));
            for(i32 i = 0; i < distance; i++)
            {
                printf("/");
            }
        }
        current = current->next;
    }
    printf("\n");
}

};
};
