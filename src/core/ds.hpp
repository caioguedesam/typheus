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
// [ARRAY]
// Fixed capacity only, no dynamic resize
template <typename T>
struct Array
{
    void* allocator = NULL;
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

    //T& operator[](Handle<T> handle)
    //{
        //ASSERT(handle.IsValid());
        //ASSERT(handle.value < count);
        //return data[handle.value];
    //}

    //const T& operator[](Handle<T> handle) const
    //{
        //ASSERT(handle.IsValid());
        //ASSERT(handle.value < count);
        //return data[handle.value];
    //}

    void Push(const T& value)
    {
        ASSERT(count + 1 <= capacity);
        memcpy(data + count, &value, sizeof(T));
        count++;
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
Array<T> MakeArray(u64 capacity)
{
    Array<T> result;
    result.allocator = mem::ctxAllocator;
    result.count = 0;
    result.data = (T*)mem::Alloc(capacity * sizeof(T));
    result.capacity = capacity;
    return result;
}

template<typename T>
Array<T> MakeArray(u64 capacity, u64 initialCount, T initialValue)
{
    Array<T> result = MakeArray<T>(capacity);
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

template<typename T>
Array<T> MakeArrayAlign(u64 capacity, i64 alignment)
{
    Array<T> result;
    result.allocator = mem::ctxAllocator;
    result.count = 0;
    result.data = (T*)mem::AllocAlign(capacity * sizeof(T), alignment);
    result.capacity = capacity;
    return result;
}

template<typename T>
Array<T> MakeArrayAlign(u64 capacity, u64 initialCount, T initialValue, i64 alignment)
{
    Array<T> result = MakeArrayAlign<T>(capacity, alignment);
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

template<typename T>
void DestroyArray(T* arr)
{
    ASSERT(arr);
    ASSERT(arr->data);
    ASSERT(mem::ctxAllocator == arr->allocator);
    mem::Free(arr->data);
    *arr = {};
}

// ========================================================
// [LIST]
// Dynamic array, resizes using next pow-2. Starts at size 16
template <typename T>
struct List
{
    void* allocator = NULL;
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

    //T& operator[](Handle<T> index)
    //{
        //ASSERT(index.value < count);
        //return data[index.value];
    //}

    //const T& operator[](Handle<T> index) const
    //{
        //ASSERT(index.value < count);
        //return data[index.value];
    //}

    void Push(const T& value)
    {
        if(count + 1 > capacity)
        {
            // Capacity not enough, resize to next pow-2
            ASSERT(mem::ctxAllocator == allocator);
            data = (T*)mem::Realloc(data, capacity * 2 * sizeof(T));
            capacity *= 2;
        }
        memcpy(data + count, &value, sizeof(T));
        count++;
    }

    T Pop()
    {
        //TODO(caio): Reducing memory dynamically?
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
List<T> MakeList(u64 initialCapacity = 1) //TODO(caio): Should I start this at 0?
{
    List<T> result;
    result.allocator = mem::ctxAllocator;
    result.count = 0;
    result.data = (T*)mem::Alloc(initialCapacity * sizeof(T));
    result.capacity = initialCapacity;
    return result;
}

template<typename T>
List<T> MakeList(u64 initialCount, T initialValue)
{
    List<T> result = MakeList<T>();
    for(u64 i = 0; i < initialCount; i++)
    {
        result.Push(initialValue);
    }
    return result;
}

template<typename T>
void DestroyList(T* list)
{
    ASSERT(list);
    ASSERT(list->data);
    ASSERT(mem::ctxAllocator == list->allocator);
    mem::Free(list->data);
    *list = {};
}

// ========================================================
// [HARRAY]
// Same as array, but operates with handle types.
// When elements are deleted, they open up slots for other elements
// by bumping up the handle metadata.
template <typename T>
struct HArray
{
    Array<T> elements;
    Array<HandleMetadata> metadata;

    T& operator[](Handle<T> handle)
    {
        ASSERT(handle.IsValid());
        ASSERT(handle.GetIndex() < elements.count);
        HandleMetadata md = metadata[handle.GetIndex()];
        ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
        return elements[handle.GetIndex()];
    }

    const T& operator[](Handle<T> handle) const
    {
        ASSERT(handle.IsValid());
        ASSERT(handle.GetIndex() < elements.count);
        HandleMetadata md = metadata[handle.index];
        ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
        return elements[handle.GetIndex()];
    }

    Handle<T> At(const i32& index)
    {
        ASSERT(index >= 0 && index < elements.count);
        return Handle<T>(metadata[index], index);
    }

    Handle<T> Insert(const T& value)
    {
        // Only perform linear search for free handles if capacity
        // is full.
        if(elements.count + 1 > elements.capacity)
        {
            // TODO(caio): Profile this
            // First, try to add in any already freed array slot
            for(i32 i = 0; i < metadata.count; i++)
            {
                if(!metadata[i].IsValid())
                {
                    elements[i] = value;
                    metadata[i].valid = HANDLE_VALID_METADATA;
                    metadata[i].gen += 1;
                    return Handle<T>(metadata[i], i);
                }
            }

            ASSERT(0);  // Can't insert new elements into array.
        }

        // When capacity isn't full, push to array regularly
        elements.Push(value);
        metadata.Push(
                {
                    .valid = HANDLE_VALID_METADATA,
                    .gen = 0,
                });

        return Handle<T>(metadata[(i32)elements.count - 1], (i32)elements.count - 1);
    }

    T Remove(Handle<T> handle)
    {
        T value = (*this)[handle];
        metadata[handle.GetIndex()].valid = HANDLE_INVALID_METADATA;
        return value;
    }

    void Clear()
    {
        elements.Clear();
        metadata.Clear();
    }

    // Iterator (for performing operations in HArray ignoring invalid elements).
    // Can't use count for this, since an element here can be freed
    Handle<T> Start()
    {
        for(i32 i = 0; i < metadata.count; i++)
        {
            if(metadata[i].IsValid())
            {
                return Handle<T>(metadata[i], i);
            }
        }

        return Handle<T>(HANDLE_INVALID_VALUE);
    }

    Handle<T> Next(Handle<T> handle)
    {
        ASSERT(handle.IsValid());
        if(handle.GetIndex() + 1 >= metadata.count)
        {
            return Handle<T>(HANDLE_INVALID_VALUE);
        }

        for(i32 i = handle.GetIndex() + 1; i < metadata.count; i++)
        {
            if(metadata[i].IsValid())
            {
                return Handle<T>(metadata[i], i);
            }
        }

        return Handle<T>(HANDLE_INVALID_VALUE);
    }

#define ForHArray(arr, handle) for(auto handle = arr.Start(); handle.GetData() != HANDLE_INVALID_VALUE; handle = arr.Next(handle))
};

template<typename T>
HArray<T> MakeHArray(u64 capacity)
{
    HArray<T> result;
    result.elements = MakeArray<T>(capacity);
    result.metadata = MakeArray<HandleMetadata>(capacity);
    return result;
}

template<typename T>
HArray<T> MakeHArray(u64 capacity, u64 initialCount, T initialValue)
{
    HArray<T> result;
    result.elements = MakeArray<T>(capacity, initialCount, initialValue);
    result.metadata = MakeArray<HandleMetadata>(capacity, initialCount, {});
    return result;
}

template<typename T>
HArray<T> MakeHArrayAlign(u64 capacity, i64 alignment)
{
    HArray<T> result;
    result.elements = MakeArrayAlign<T>(capacity);
    result.metadata = MakeArrayAlign<HandleMetadata>(capacity);
    return result;
}

template<typename T>
HArray<T> MakeHArrayAlign(u64 capacity, u64 initialCount, T initialValue, i64 alignment)
{
    HArray<T> result;
    result.elements = MakeArrayAlign<T>(capacity, initialCount, initialValue);
    result.metadata = MakeArrayAlign<HandleMetadata>(capacity, initialCount, {});
    return result;
}

template<typename T>
void DestroyHArray(T* arr)
{
    DestroyArray(&arr->elements);
    DestroyArray(&arr->metadata);
    *arr = {};
}

// ========================================================
// [HLIST]
// Same as dynamic array, but operates with handle types.
// When elements are deleted, they open up slots for other elements
// by bumping up the handle metadata.
template <typename T>
struct HList
{
    List<T> elements;
    List<HandleMetadata> metadata;

    T& operator[](Handle<T> handle)
    {
        ASSERT(handle.IsValid());
        ASSERT(handle.GetIndex() < elements.count);
        HandleMetadata md = metadata[handle.GetIndex()];
        ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
        return elements[handle.GetIndex()];
    }

    const T& operator[](Handle<T> handle) const
    {
        ASSERT(handle.IsValid());
        ASSERT(handle.GetIndex() < elements.count);
        HandleMetadata md = metadata[handle.GetIndex()];
        ASSERT(md == handle.GetMetadata());   // This handle is an invalid reference, either slot is free or generation doesn't match.
        return elements[handle.GetIndex()];
    }

    Handle<T> At(const i32& index)
    {
        ASSERT(index >= 0 && index < elements.count);
        return Handle<T>(metadata[index], index);
    }

    Handle<T> Insert(const T& value)
    {
        // Only perform linear search for free handles if capacity
        // is full.
        if(elements.count + 1 > elements.capacity)
        {
            // TODO(caio): Profile this
            // First, try to add in any already freed array slot
            for(i32 i = 0; i < metadata.count; i++)
            {
                if(!metadata[i].IsValid())
                {
                    elements[i] = value;
                    metadata[i].valid = HANDLE_VALID_METADATA;
                    metadata[i].gen += 1;
                    return Handle<T>(metadata[i], i);
                }
            }
        }

        // When capacity isn't full, push to array regularly
        elements.Push(value);
        metadata.Push(
                {
                    .valid = HANDLE_VALID_METADATA,
                    .gen = 0,
                });

        return Handle<T>(metadata[(i32)elements.count - 1], (i32)elements.count - 1);
    }

    T Remove(Handle<T> handle)
    {
        T value = (*this)[handle];
        metadata[handle.GetIndex()].valid = HANDLE_INVALID_METADATA;
        return value;
    }

    void Clear()
    {
        elements.Clear();
        metadata.Clear();
    }

    // Iterator (for performing operations in HArray ignoring invalid elements).
    // Can't use count for this, since an element here can be freed
    Handle<T> Start()
    {
        for(i32 i = 0; i < metadata.count; i++)
        {
            if(metadata[i].IsValid())
            {
                return Handle<T>(metadata[i], i);
            }
        }

        return Handle<T>(HANDLE_INVALID_VALUE);
    }

    Handle<T> Next(Handle<T> handle)
    {
        ASSERT(handle.IsValid());
        if(handle.GetIndex() + 1 >= metadata.count)
        {
            return Handle<T>(HANDLE_INVALID_VALUE);
        }

        for(i32 i = handle.GetIndex() + 1; i < metadata.count; i++)
        {
            if(metadata[i].IsValid())
            {
                return Handle<T>(metadata[i], i);
            }
        }

        return Handle<T>(HANDLE_INVALID_VALUE);
    }
#define ForHList(arr, handle) for(auto handle = arr.Start(); handle.u.data != HANDLE_INVALID_VALUE; handle = arr.Next(handle))
};

template<typename T>
HList<T> MakeHList(u64 initialCapacity = 16)
{
    HList<T> result;
    result.elements = MakeList<T>(initialCapacity);
    result.metadata = MakeList<HandleMetadata>(initialCapacity);
    return result;
}

template<typename T>
HList<T> MakeHList(u64 initialCount, T initialValue)
{
    HList<T> result;
    result.elements = MakeList<T>(initialCount, initialValue);
    result.metadata = MakeList<HandleMetadata>(initialCount, {});
    return result;
}

template<typename T>
void DestroyHList(T* list)
{
    DestroyList(&list->elements);
    DestroyList(&list->metadata);
    *list = {};
}



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
    Array<Bucket> buckets;

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
HashMap<Tk, Tv> MakeMap(u64 capacity)
{
    HashMap<Tk, Tv> result;
    result.buckets = MakeArray<typename HashMap<Tk, Tv>::Bucket>(capacity);
    typename HashMap<Tk, Tv>::Bucket empty = {};
    for(u64 i = 0; i < capacity; i++)
    {
        result.buckets.Push(empty);
    }
    return result;
}

template<typename Tk, typename Tv>
void DestroyMap(HashMap<Tk, Tv>* map)
{
    ASSERT(map);
    DestroyArray(&map->buckets);
    *map = {};
}

};
