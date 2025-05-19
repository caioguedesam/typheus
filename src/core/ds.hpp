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
    SArray<T>* result = (SArray<T>*)mem::ArenaPush(arena, sizeof(SArray<T>));
    result->count = 0;
    result->data = (T*)mem::ArenaPush(arena, capacity * sizeof(T));
    result->capacity = capacity;
    return *result;
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
    SArray<T>* result = (SArray<T>*)mem::ArenaPush(arena, sizeof(SArray<T>));
    result->count = 0;
    result->data = (T*)mem::ArenaPush(arena, capacity * sizeof(T), alignment);
    result->capacity = capacity;
    return *result;
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
    DArray<T>* result = (DArray<T>*)mem::ArenaPush(arena, sizeof(DArray<T>));
    result->count = 0;
    result->data = (T*)mem::ArenaPush(arena, initialCapacity * sizeof(T));
    result->capacity = initialCapacity;
    result->arena = arena;
    return *result;
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

// ========================================================
// [HASH MAP]
// Fixed capacity bucket array, linear probing
// Requires Key type to implement Hash() and operator==()

// This macro calls implemented hash function overloaded for the value's type.
// Will raise compile error if there's no hash implemented for the type.
#define HASH(v) Hash((v))

u32 Hash(u64 v);

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
};
