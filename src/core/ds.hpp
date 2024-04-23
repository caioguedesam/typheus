// ========================================================
// DS
// Collection of data structures (array, hash map...)
// @Caio Guedes, 2023
// ========================================================
#pragma once
#include "./base.hpp"
#include "./memory.hpp"
#include "./debug.hpp"

namespace ty
{

// ========================================================
// [RANGE]
// A range of elements for a given type
struct Range
{
    i64 start = -1;
    i64 len = 0;

    bool IsValid() { return start != -1; }
};

// ========================================================
// [STATIC ARRAY]
// Fixed capacity only, no dynamic resize
template <typename T>
struct SArray
{
    u64 capacity = 0;
    u64 count = 0;
    T* data = NULL;

    T& operator[](u64 index)
    {
        ASSERT(index < count);
        return data[index];
    }

    const T& operator[](u64 index) const
    {
        ASSERT(index < count);
        return data[index];
    }

    handle Push(const T& value)
    {
        ASSERT(count + 1 <= capacity);
        memcpy(data + count, &value, sizeof(T));
        count++;
        return count - 1;
    }

    T Pop()
    {
        ASSERT(count - 1 >= 0);
        T result = data[count - 1];
        count--;
        return result;
    }

    void Clear()
    {
        count = 0;
    }
};

template<typename T>
SArray<T> MakeSArray(mem::Arena* arena, u64 capacity)
{
    SArray<T> result;
    result.count = 0;
    result.data = (T*)mem::ArenaPush(arena, capacity * sizeof(T));
    result.capacity = capacity;
    return result;
}

template<typename T>
SArray<T> MakeSArray(mem::Arena* arena, u64 capacity, u64 initialCount, T initialValue)
{
    SArray<T> result = MakeSArray<T>(arena, capacity);
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

template<typename T>
SArray<T> MakeSArrayAlign(mem::Arena* arena, u64 capacity, i64 alignment)
{
    SArray<T> result;
    result.count = 0;
    result.data = (T*)mem::ArenaPush(arena, capacity * sizeof(T), alignment);
    result.capacity = capacity;
    return result;
}

template<typename T>
SArray<T> MakeSArrayAlign(mem::Arena* arena, u64 capacity, u64 initialCount, T initialValue, i64 alignment)
{
    SArray<T> result = MakeSArrayAlign<T>(arena, capacity, alignment);
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

//template<typename T>
//void DestroyArray(T* arr)
//{
//    ASSERT(arr);
//    ASSERT(arr->data);
//    ASSERT(mem::ctxAllocator == arr->allocator);
//    mem::Free(arr->data);
//    *arr = {};
//}

// ========================================================
// [DYNAMIC ARRAY]
// Variable length array, resizes at pow-2. At worst case, uses almost double the memory
// than a static array.
template <typename T>
struct DArray
{
    mem::Arena* arena = NULL;
    u64 capacity = 0;
    u64 count = 0;
    T* data = NULL;

    T& operator[](u64 index)
    {
        ASSERT(index < count);
        return data[index];
    }

    const T& operator[](u64 index) const
    {
        ASSERT(index < count);
        return data[index];
    }

    handle Push(const T& value)
    {
        if(count + 1 > capacity)
        {
            // First, if array is at top of arena, we can just expand existing memory.
            byte* arrayTop = (byte*)data + count;
            if(arrayTop == mem::ArenaGetTop(arena))
            {
                mem::ArenaPush(arena, capacity);
            }
            // If not, allocate new block in arena and memcpy from old one.
            else
            {
                T* newData = (T*)mem::ArenaPush(arena, capacity * 2 * sizeof(T));
                memcpy(newData, data, capacity * sizeof(T));
                data = newData;
                capacity *= 2;
            }
        }
        memcpy(data + count, &value, sizeof(T));
        count++;
        return count - 1;   // Element's index
    }

    T Pop()
    {
        ASSERT(count - 1 >= 0);
        T result = data[count - 1];
        count--;
        return result;
    }

    void Clear()
    {
        count = 0;
    }
};

template<typename T>
DArray<T> MakeDArray(mem::Arena* arena, u64 initialCapacity = 1) //TODO(caio): Should I start this at 0?
{
    DArray<T> result;
    result.count = 0;
    result.data = (T*)mem::ArenaPush(arena, initialCapacity * sizeof(T));
    result.capacity = initialCapacity;
    result.arena = arena;
    return result;
}

template<typename T>
DArray<T> MakeDArray(mem::Arena* arena, u64 initialCount, T initialValue)
{
    DArray<T> result = MakeDArray<T>(arena);
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

//template<typename T>
//void DestroyList(T* list)
//{
//    ASSERT(list);
//    ASSERT(list->data);
//    ASSERT(mem::ctxAllocator == list->allocator);
//    mem::Free(list->data);
//    *list = {};
//}

// // ========================================================
// // [HANDLE STATIC ARRAY]
// // Same as static array, but operates with handle types.
// // When elements are deleted, they open up slots for other elements
// // by bumping up the handle metadata.
// template <typename T>
// struct HSArray
// {
//     SArray<T> elements;
//     SArray<HandleMetadata> metadata;
// 
//     T& operator[](Handle<T> handle)
//     {
//         ASSERT(handle.IsValid());
//         ASSERT(handle.GetIndex() < elements.count);
//         HandleMetadata md = metadata[handle.GetIndex()];
//         ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
//         return elements[handle.GetIndex()];
//     }
// 
//     const T& operator[](Handle<T> handle) const
//     {
//         ASSERT(handle.IsValid());
//         ASSERT(handle.GetIndex() < elements.count);
//         HandleMetadata md = metadata[handle.index];
//         ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
//         return elements[handle.GetIndex()];
//     }
// 
//     Handle<T> At(const i32& index)
//     {
//         ASSERT(index >= 0 && index < elements.count);
//         return Handle<T>(metadata[index], index);
//     }
// 
//     Handle<T> Insert(const T& value)
//     {
//         // Only perform linear search for free handles if capacity
//         // is full.
//         if(elements.count + 1 > elements.capacity)
//         {
//             // TODO(caio): Profile this
//             // First, try to add in any already freed array slot
//             for(i32 i = 0; i < metadata.count; i++)
//             {
//                 if(!metadata[i].IsValid())
//                 {
//                     elements[i] = value;
//                     metadata[i].valid = HANDLE_VALID_METADATA;
//                     metadata[i].gen += 1;
//                     return Handle<T>(metadata[i], i);
//                 }
//             }
// 
//             ASSERT(0);  // Can't insert new elements into array.
//         }
// 
//         // When capacity isn't full, push to array regularly
//         elements.Push(value);
//         metadata.Push(
//                 {
//                     .valid = HANDLE_VALID_METADATA,
//                     .gen = 0,
//                 });
// 
//         return Handle<T>(metadata[(i32)elements.count - 1], (i32)elements.count - 1);
//     }
// 
//     T Remove(Handle<T> handle)
//     {
//         T value = (*this)[handle];
//         metadata[handle.GetIndex()].valid = HANDLE_INVALID_METADATA;
//         return value;
//     }
// 
//     void Clear()
//     {
//         elements.Clear();
//         metadata.Clear();
//     }
// 
//     // Iterator (for performing operations in HArray ignoring invalid elements).
//     // Can't use count for this, since an element here can be freed
//     Handle<T> Start()
//     {
//         for(i32 i = 0; i < metadata.count; i++)
//         {
//             if(metadata[i].IsValid())
//             {
//                 return Handle<T>(metadata[i], i);
//             }
//         }
// 
//         return Handle<T>(HANDLE_INVALID_VALUE);
//     }
// 
//     Handle<T> Next(Handle<T> handle)
//     {
//         ASSERT(handle.IsValid());
//         if(handle.GetIndex() + 1 >= metadata.count)
//         {
//             return Handle<T>(HANDLE_INVALID_VALUE);
//         }
// 
//         for(i32 i = handle.GetIndex() + 1; i < metadata.count; i++)
//         {
//             if(metadata[i].IsValid())
//             {
//                 return Handle<T>(metadata[i], i);
//             }
//         }
// 
//         return Handle<T>(HANDLE_INVALID_VALUE);
//     }
// 
// #define ForHArray(arr, handle) for(auto handle = arr.Start(); handle.GetData() != HANDLE_INVALID_VALUE; handle = arr.Next(handle))
// };
// 
// template<typename T>
// HSArray<T> MakeHSArray(mem::Arena* arena, u64 capacity)
// {
//     HSArray<T> result;
//     result.elements = MakeSArray<T>(arena, capacity);
//     result.metadata = MakeSArray<HandleMetadata>(arena, capacity);
//     return result;
// }
// 
// template<typename T>
// HSArray<T> MakeHSArray(mem::Arena* arena, u64 capacity, u64 initialCount, T initialValue)
// {
//     HSArray<T> result;
//     result.elements = MakeSArray<T>(arena, capacity, initialCount, initialValue);
//     result.metadata = MakeSArray<HandleMetadata>(arena, capacity, initialCount, {});
//     return result;
// }
// 
// // template<typename T>
// // HArray<T> MakeHArrayAlign(u64 capacity, i64 alignment)
// // {
// //     HArray<T> result;
// //     result.elements = MakeArrayAlign<T>(capacity);
// //     result.metadata = MakeArrayAlign<HandleMetadata>(capacity);
// //     return result;
// // }
// // 
// // template<typename T>
// // HArray<T> MakeHArrayAlign(u64 capacity, u64 initialCount, T initialValue, i64 alignment)
// // {
// //     HArray<T> result;
// //     result.elements = MakeArrayAlign<T>(capacity, initialCount, initialValue);
// //     result.metadata = MakeArrayAlign<HandleMetadata>(capacity, initialCount, {});
// //     return result;
// // }
// // 
// // template<typename T>
// // void DestroyHArray(T* arr)
// // {
// //     DestroyArray(&arr->elements);
// //     DestroyArray(&arr->metadata);
// //     *arr = {};
// // }
// 
// // ========================================================
// // [HANDLE DYNAMIC ARRAY]
// // Same as dynamic array, but operates with handle types.
// // When elements are deleted, they open up slots for other elements
// // by bumping up the handle metadata.
// template <typename T>
// struct HDArray
// {
//     DArray<T> elements;
//     DArray<HandleMetadata> metadata;
// 
//     T& operator[](Handle<T> handle)
//     {
//         ASSERT(handle.IsValid());
//         ASSERT(handle.GetIndex() < elements.count);
//         HandleMetadata md = metadata[handle.GetIndex()];
//         ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
//         return elements[handle.GetIndex()];
//     }
// 
//     const T& operator[](Handle<T> handle) const
//     {
//         ASSERT(handle.IsValid());
//         ASSERT(handle.GetIndex() < elements.count);
//         HandleMetadata md = metadata[handle.GetIndex()];
//         ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
//         return elements[handle.GetIndex()];
//     }
// 
//     Handle<T> At(const i32& index)
//     {
//         ASSERT(index >= 0 && index < elements.count);
//         return Handle<T>(metadata[index], index);
//     }
// 
//     Handle<T> Insert(const T& value)
//     {
//         // Only perform linear search for free handles if capacity
//         // is full.
//         if(elements.count + 1 > elements.capacity)
//         {
//             // TODO(caio): Profile this
//             // First, try to add in any already freed array slot
//             for(i32 i = 0; i < metadata.count; i++)
//             {
//                 if(!metadata[i].IsValid())
//                 {
//                     elements[i] = value;
//                     metadata[i].valid = HANDLE_VALID_METADATA;
//                     metadata[i].gen += 1;
//                     return Handle<T>(metadata[i], i);
//                 }
//             }
//         }
// 
//         // When capacity isn't full, push to array regularly
//         elements.Push(value);
//         metadata.Push(
//                 {
//                     .valid = HANDLE_VALID_METADATA,
//                     .gen = 0,
//                 });
// 
//         return Handle<T>(metadata[(i32)elements.count - 1], (i32)elements.count - 1);
//     }
// 
//     T Remove(Handle<T> handle)
//     {
//         T value = (*this)[handle];
//         metadata[handle.GetIndex()].valid = HANDLE_INVALID_METADATA;
//         return value;
//     }
// 
//     void Clear()
//     {
//         elements.Clear();
//         metadata.Clear();
//     }
// 
//     // Iterator (for performing operations in HArray ignoring invalid elements).
//     // Can't use count for this, since an element here can be freed
//     Handle<T> Start()
//     {
//         for(i32 i = 0; i < metadata.count; i++)
//         {
//             if(metadata[i].IsValid())
//             {
//                 return Handle<T>(metadata[i], i);
//             }
//         }
// 
//         return Handle<T>(HANDLE_INVALID_VALUE);
//     }
// 
//     Handle<T> Next(Handle<T> handle)
//     {
//         ASSERT(handle.IsValid());
//         if(handle.GetIndex() + 1 >= metadata.count)
//         {
//             return Handle<T>(HANDLE_INVALID_VALUE);
//         }
// 
//         for(i32 i = handle.GetIndex() + 1; i < metadata.count; i++)
//         {
//             if(metadata[i].IsValid())
//             {
//                 return Handle<T>(metadata[i], i);
//             }
//         }
// 
//         return Handle<T>(HANDLE_INVALID_VALUE);
//     }
// #define ForHList(arr, handle) for(auto handle = arr.Start(); handle.u.data != HANDLE_INVALID_VALUE; handle = arr.Next(handle))
// };
// 
// template<typename T>
// HDArray<T> MakeHList(mem::Arena* arena, u64 initialCapacity = 16)
// {
//     HDArray<T> result;
//     result.elements = MakeDArray<T>(arena, initialCapacity);
//     result.metadata = MakeDArray<HandleMetadata>(arena, initialCapacity);
//     return result;
// }
// 
// template<typename T>
// HDArray<T> MakeHList(mem::Arena* arena, u64 initialCount, T initialValue)
// {
//     HDArray<T> result;
//     result.elements = MakeDArray<T>(arena, initialCount, initialValue);
//     result.metadata = MakeDArray<HandleMetadata>(arena, initialCount, {});
//     return result;
// }
// 
// //template<typename T>
// //void DestroyHList(T* list)
// //{
//     //DestroyList(&list->elements);
//     //DestroyList(&list->metadata);
//     //*list = {};
// //}



// ========================================================
// [HASH MAP]
// Fixed capacity bucket array, linear probing
// Requires Key type to implement Hash() and operator==()

// This macro calls implemented hash function overloaded for the value's type.
// Will raise compile error if there's no hash implemented for the type.
#define HASH(v) Hash((v))

template<typename Tk, typename Tv>
struct HashMap
{
    struct Bucket
    {
        bool valid = false;
        Tk key;
        Tv value;
    };
    SArray<Bucket> buckets;

    Tv& operator[](Tk key)
    {
        u32 keyHash = HASH(key);
        for(u32 i = 0; i < buckets.count; i++)
        {
            u32 pos = (keyHash + i) % buckets.count;
            Bucket& bucket = buckets[pos];
            if(bucket.valid && bucket.key == key) return bucket.value;
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
        return buckets[0].value;
    };

    const Tv& operator[](Tk key) const
    {
        u32 keyHash = HASH(key);
        for(u32 i = 0; i < buckets.count; i++)
        {
            u32 pos = (keyHash + i) % buckets.count;
            Bucket& bucket = buckets[pos];
            if(bucket.valid && bucket.key == key) return bucket.value;
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
        return buckets[0].value;
    };

    bool HasKey(Tk key)
    {
        u32 keyHash = HASH(key);
        for(u32 i = 0; i < buckets.count; i++)
        {
            u32 pos = (keyHash + i) % buckets.count;
            Bucket& bucket = buckets[pos];
            if(bucket.valid && bucket.key == key) return true;
        }
        return false;
    }

    void Insert(const Tk& key, const Tv& value)
    {
        u64 keyHash = HASH(key);
        for(u32 i = 0; i < buckets.count; i++)
        {
            u32 pos = (keyHash + i) % buckets.count;
            if(!buckets[i].valid)
            {
                buckets[i] = { true, key, value };
                return;
            }
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
    }

    void Remove(const Tk& key)
    {
        u64 keyHash = HASH(key);
        for(u32 i = 0; i < buckets.count; i++)
        {
            u32 pos = (keyHash + i) % buckets.count;
            if(buckets[i].valid && buckets[i].key == key)
            {
                buckets[i].valid = false;
                return;
            }
        }
        ASSERT(0);      // Key not present in the hash map, invalid op.
    }
};

template<typename Tk, typename Tv>
HashMap<Tk, Tv> MakeMap(mem::Arena* arena, u64 capacity)
{
    HashMap<Tk, Tv> result;
    result.buckets = MakeSArray<typename HashMap<Tk, Tv>::Bucket>(arena, capacity);
    typename HashMap<Tk, Tv>::Bucket empty = {};
    for(u64 i = 0; i < capacity; i++)
    {
        result.buckets.Push(empty);
    }
    return result;
}

// template<typename Tk, typename Tv>
// void DestroyMap(HashMap<Tk, Tv>* map)
// {
//     ASSERT(map);
//     DestroyArray(&map->buckets);
//     *map = {};
// }

};
