#include "engine/core/string.hpp"
#include "string.h"

namespace ty
{

String MStr(u64 capacity)
{
    String result;
    result.data = (u8*)mem::AllocZero(capacity);
    result.len = 0;
    result.capacity = capacity;
    return result;
}

String MStr(u64 capacity, const char* value)
{
    u64 len = strlen(value);
    ASSERT(len + 1 <= capacity);

    String result = MStr(capacity);
    result.len = strlen(value);
    result.capacity = capacity;

    strcpy((char*)result.data, value);
    return result;
}

String MStr(u64 capacity, String value)
{
    ASSERT(value.len + 1 <= capacity);

    String result = MStr(capacity);
    result.len = value.len;
    result.capacity = capacity;

    memcpy(result.data, value.data, value.len);
    return result;
}

String MStr(const char* value)
{
    return MStr(strlen(value) + 1, value);
}

String MStr(String value)
{
    return MStr(value.len + 1, value);
}

void DestroyMStr(String* str)
{
    ASSERT(str->capacity);
    mem::Free(str->data);
    *str = {};
}

String IStr(const char* value)
{
    String result;
    result.data = (u8*)value;
    result.len = strlen(value);
    result.capacity = 0;
    return result;
}

String IStr(String value)
{
    String result;
    result.data = value.data;
    result.len = value.len;
    result.capacity = 0;
    return result;
}

String IStr(u8* data, u64 len)
{
    String result;
    result.data = data;
    result.len = len;
    result.capacity = 0;
    return result;
}

//const char* String::CStr()
//{
    //ASSERT(data);
    //return (const char*)data;
//}

void String::CStr(char* output)
{
    ASSERT(data);
    memcpy(output, data, len);
    output[len] = 0;    // Null term
}

u8 String::At(u64 i)
{
    return *(data + i);
}

bool StrEquals(String a, String b)
{
    return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

bool operator==(String a, String b) { return StrEquals(a, b); }
bool operator!=(String a, String b) { return !StrEquals(a, b); }

i64 String::Find(char c)
{
    for(i64 i = 0; i < len; i++)
    {
        if((char)At(i) == c)
        {
            return i;
        }
    }
    return -1;
}

i64 String::Find(String s)
{
    ASSERT(s.len);
    if(s.len > len) return -1;

    // Naive string search. Can definitely be optimized.
    for(i64 i = 0; i < len - s.len + 1; i++)
    {
        if(At(i) == s.At(0))
        {
            bool match = true;
            for(i64 j = 1; j < s.len; j++)
            {
                if(At(i + j) != s.At(j))
                {
                    match = false;
                    break;
                }
            }
            if(match) return i;
        }
    }
    return -1;
}

i64 String::RFind(char c)
{
    for(i64 i = len - 1; i >= 0; i--)
    {
        if((char)At(i) == c)
        {
            return i;
        }
    }
    return -1;
}

i64 String::RFind(String s)
{
    ASSERT(s.len);
    if(s.len > len) return -1;

    // Naive string search. Can definitely be optimized.
    for(i64 i = len - 1; i >= s.len - 1; i--)
    {
        if(At(i) == s.At(s.len - 1))
        {
            bool match = true;
            for(i64 j = s.len - 2; j >= 0; j--)
            {
                if(At(i - j) != s.At(j))
                {
                    match = false;
                    break;
                }
            }
            if(match) return i;
        }
    }
    return -1;
}

String String::Substr(u64 start)
{
    ASSERT(start < len);
    String result;
    result.capacity = 0;
    result.data = data + start;
    result.len = len - start;
    return result;
}

String String::Substr(u64 start, u64 size)
{
    ASSERT(start + size <= len);
    String result;
    result.capacity = 0;
    result.len = size;
    result.data = data + start;
    return result;
}

void String::Clear()
{
    ASSERT(capacity);
    memset(data, 0, capacity);
    len = 0;
}

void String::Append(const char* other)
{
    u64 otherLen = strlen(other);
    ASSERT(len + otherLen < capacity);
    strcpy((char*)(data + len), other);
    len = len + otherLen;
}

void String::Append(String other)
{
    ASSERT(len + other.len < capacity);
    memcpy((char*)(data + len), other.data, other.len);
    len = len + other.len;
}

// djb2 string hash
u32 Hash(const char* cstr)
{
    u32 result = 5381;
    u32 slen = strlen(cstr);
    for(u32 i = 0; i < slen; i++)
    {
        result = ((result << 5) + result) + cstr[i];
    }
    return result;
}

// djb2 string hash
u32 Hash(String str)
{
    u32 result = 5381;
    for(u32 i = 0; i < str.len; i++)
    {
        result = ((result << 5) + result) + str.At(i);
    }
    return result;
}

};
