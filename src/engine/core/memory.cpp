#include "engine/core/memory.hpp"
#include "engine/core/debug.hpp"
#include <vcruntime_string.h>

namespace ty
{
namespace mem
{

Region AllocateRegion(u64 capacity)
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

void* InvalidAlloc(u64 size)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidAllocZero(u64 size)
{
    ASSERT(0);  // Context is invalid for this operation
    return NULL;
}

void* InvalidRealloc(void* data, u64 size)
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

ArenaAllocator MakeArenaAllocator(u64 capacity)
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
    Realloc = InvalidRealloc;
    Free = InvalidFree;
    FreeAll = ArenaFreeAll;
}

void* ArenaAlloc(u64 size)
{
    ArenaAllocator* arena = (ArenaAllocator*)ctxAllocator;
    ASSERT(arena);
    ASSERT(arena->offset + size <= arena->region.capacity);
    u8* result = arena->region.start + arena->offset;
    arena->offset += size;
    return result;
}

void* ArenaAllocZero(u64 size)
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

HeapAllocator MakeHeapAllocator(u64 capacity)
{
    HeapAllocator result = {};
    result.region = AllocateRegion(capacity);
    result.used = 0;
    HeapAllocatorFreeListNode* firstNode = (HeapAllocatorFreeListNode*)result.region.start;
    firstNode->blockSize = capacity;
    firstNode->next = NULL;
    result.head = firstNode;
    return result;
}

void DestroyHeapAllocator(HeapAllocator* heap)
{
    if(heap == ctxAllocator) ResetContext();
    FreeRegion(&heap->region);
    *heap = {};
}

void SetContext(HeapAllocator* heap)
{
    ctxAllocator = heap;
    Alloc = HeapAlloc;
    AllocZero = HeapAllocZero;
    Realloc = HeapRealloc;
    Free = HeapFree;
    FreeAll = HeapFreeAll;
}

void HeapFreeAll()
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    heap->used = 0;
    HeapAllocatorFreeListNode* firstNode = (HeapAllocatorFreeListNode*)heap->region.start;
    firstNode->blockSize = heap->region.capacity;
    firstNode->next = NULL;
    heap->head = firstNode;
}

void HeapAllocator::InsertNode(HeapAllocatorFreeListNode *prev, HeapAllocatorFreeListNode *node)
{
    if(!prev)   // No previous node means inserting at head position
    {
        if(head)
        {
            node->next = head;
        }
        head = node;
    }
    else
    {
        if(!prev->next)
        {
            prev->next = node;
            node->next = NULL;
        }
        else
        {
            node->next = prev->next;
            prev->next = node;
        }
    }
}

void HeapAllocator::RemoveNode(HeapAllocatorFreeListNode *prev, HeapAllocatorFreeListNode *node)
{
    if(!prev)   // No previous node means removing at head position
    {
        head = node->next;
    }
    else
    {
        prev->next = node->next;
    }
}

void HeapAllocator::Coalesce(HeapAllocatorFreeListNode *prev, HeapAllocatorFreeListNode *node)
{
    // Coalesce to the front
    if(node->next && (void*)((u8*)node + node->blockSize) == node->next)
    {
        node->blockSize += node->next->blockSize;
        RemoveNode(node, node->next);
    }
    // Coalesce to the back
    if(prev->next && (void*)((u8*)prev + prev->blockSize) == node)
    {
        prev->blockSize += node->blockSize;
        RemoveNode(prev, node);
    }
}

HeapAllocatorFreeListNode* HeapAllocator::FindFirst(u64 size, HeapAllocatorFreeListNode** prev)
{
    // Starts at head, iterates through the list and returns first free block matching
    // size requirements (required size + header size)
    HeapAllocatorFreeListNode* node = head;
    HeapAllocatorFreeListNode* prevNode = NULL;

    while(node)
    {
        u64 requiredSpace = sizeof(HeapAllocationHeader) + size; //TODO(caio): Alignment/padding
        if(node->blockSize >= requiredSpace)
        {
            break;
        }
        prevNode = node;
        node = node->next;
    }

    if(prev) *prev = prevNode;
    return node;
}

HeapAllocatorFreeListNode* HeapAllocator::FindBest(u64 size, HeapAllocatorFreeListNode** prev)
{
    // Starts at head, iterates through the list and returns smallest free block matching
    // size requirements (required size + header size)
    HeapAllocatorFreeListNode* node = head;
    HeapAllocatorFreeListNode* prevNode = NULL;

    HeapAllocatorFreeListNode* bestNode = NULL;
    HeapAllocatorFreeListNode* bestPrevNode = NULL;
    u64 smallestDiff = MAX_U64;
    while(node)
    {
        u64 requiredSpace = sizeof(HeapAllocationHeader) + size; //TODO(caio): Alignment/padding
        if(node->blockSize >= requiredSpace && (node->blockSize - requiredSpace < smallestDiff))
        {
            bestPrevNode = prevNode;
            bestNode = node;
            smallestDiff = node->blockSize - requiredSpace;
        }
        prevNode = node;
        node = node->next;
    }

    if(prev) *prev = bestPrevNode;
    return bestNode;
}

void* HeapAlloc(u64 size)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;

    u64 headerSize = sizeof(HeapAllocationHeader);
    u64 allocationSize = headerSize + size; //TODO(caio): Alignment/padding
    ASSERT(heap->used + allocationSize <= heap->region.capacity);

    //u64 headerSize = sizeof(HeapAllocationHeader);
    //if(size < headerSize) size = headerSize + size;

    // Find free node to use for allocation
    HeapAllocatorFreeListNode* node = NULL;
    HeapAllocatorFreeListNode* prevNode = NULL;
    switch(heap->strategy)
    {
        case FIND_FIRST:
        {
            node = heap->FindFirst(size, &prevNode);
        } break;
        case FIND_BEST:
        {
            node = heap->FindBest(size, &prevNode);
        } break;
        default: ASSERT(0);
    }
    ASSERT(node);

    // Allocate space and adjust free list accordingly
    u64 remainingSize = node->blockSize - allocationSize;
    if(remainingSize)
    {
        HeapAllocatorFreeListNode* newNode = (HeapAllocatorFreeListNode*)((u8*)node + allocationSize);
        newNode->blockSize = remainingSize;
        heap->InsertNode(node, newNode);
    }
    heap->RemoveNode(prevNode, node);

    // Set allocation header and return allocated memory address
    HeapAllocationHeader* header = (HeapAllocationHeader*)(node);
    header->blockSize = allocationSize;
    heap->used += allocationSize;
    return (void*)((u8*)header + headerSize);
}

void* HeapAllocZero(u64 size)
{
    void* result = HeapAlloc(size);
    memset(result, 0, size);
    return result;
}

void* HeapRealloc(void* data, u64 size)
{
    if(!data) return HeapAlloc(size);

    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;

    // First, check the memory right after the data to realloc.
    // If there's a free block that has enough size for the realloc,
    // just use and adjust that.
    u64 headerSize = sizeof(HeapAllocationHeader);
    u64 allocationSize = headerSize + size;
    // Find what would be a free node right after the already allocated data in the free list
    HeapAllocationHeader* dataHeader = (HeapAllocationHeader*)((u8*)data - headerSize);
    HeapAllocatorFreeListNode* freeNodeAfterData = (HeapAllocatorFreeListNode*)((u8*)data - headerSize + dataHeader->blockSize);
    // Then search for it in the free list to see if it's actually an available free node.
    HeapAllocatorFreeListNode* node = heap->head;
    HeapAllocatorFreeListNode* prevNode = NULL;
    while(node)
    {
        if(node == freeNodeAfterData) break;
        prevNode = node;
        node = node->next;
    }
    if(node && node->blockSize + dataHeader->blockSize >= allocationSize)
    {
        // Found available space, adjust existing alloc and free list accordingly.
        u64 oldDataSize = dataHeader->blockSize;
        i64 dataSizeDelta = allocationSize - oldDataSize; //TODO(caio): Test this with reallocs that reduce instead of enlarge
        dataHeader->blockSize = allocationSize;

        u64 newFreeNodeBlockSize = node->blockSize - dataSizeDelta;  //TODO(caio)CONTINUE: bug when this is 0, use ASSERT on bottom and active remedy session
        HeapAllocatorFreeListNode* newFreeNodeNext = node->next;
        if(newFreeNodeBlockSize == 0)
        {
            // Consumed all of the node's free memory, just delete the node.
            if(!prevNode)
            {
                heap->head = newFreeNodeNext;
            }
            else
            {
                prevNode->next = newFreeNodeNext;
            }
        }
        else
        {
            // Some of the node's free memory is still left, replace it with new smaller node
            if(!prevNode)
            {
                heap->head = (HeapAllocatorFreeListNode*)((u8*)node + dataSizeDelta);
                heap->head->blockSize = newFreeNodeBlockSize;
                heap->head->next = newFreeNodeNext;
            }
            else
            {
                HeapAllocatorFreeListNode* newNode = (HeapAllocatorFreeListNode*)((u8*)node + dataSizeDelta);
                ASSERT((u64)newNode != (u64)newFreeNodeNext);
                prevNode->next = newNode;
                newNode->blockSize = newFreeNodeBlockSize;
                newNode->next = newFreeNodeNext;
            }
        }

        return data;
    }

    // If there's no free block, or if the next free block is not
    // big enough for the realloc, then just find another space in
    // the heap and copy the old data.
    void* result = HeapAllocZero(size);
    memcpy(result, data, dataHeader->blockSize - headerSize);
    HeapFree(data);
    return result;
}

void HeapFree(void* data)
{
    HeapAllocator* heap = (HeapAllocator*)ctxAllocator;
    if(!data) return;

    // Create new free list node header where allocation was
    HeapAllocationHeader* header = (HeapAllocationHeader*)((u8*)data - sizeof(HeapAllocationHeader));
    HeapAllocatorFreeListNode* newNode = (HeapAllocatorFreeListNode*)header;
    newNode->blockSize = header->blockSize; //TODO(caio): Alignment/padding
    newNode->next = NULL;

    // Insert the node at the appropriate position in the free list
    HeapAllocatorFreeListNode* node = heap->head;
    HeapAllocatorFreeListNode* prevNode = NULL;
    while(node)
    {
        if(node > data)
        {
            heap->InsertNode(prevNode, newNode);
        }
        prevNode = node;
        node = node->next;
    }

    // Coalesce newly free node to avoid breaks between contiguous free blocks
    heap->used -= newNode->blockSize;
    heap->Coalesce(prevNode, newNode);
}

};
};
