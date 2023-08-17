#include "./string.hpp"
#include "src/core/memory.hpp"
#include "string.h"

namespace ty
{

char& String::operator[](u64 index)
{
    ASSERT(index < len);
    return (char&)data[index];
};

char* String::CStr()
{
    return (char*)data;
}

String IStr(const char* value)
{
    String result = {};
    result.capacity = 0;
    result.len = strlen(value);
    result.data = (u8*)value;
    return result;
};

String IStr(String value)
{
    String result = {};
    result.capacity = 0;
    result.len = value.len;
    result.data = value.data;
    return result;
};

void FreeMStr(String* s)
{
    ASSERT(IS_MUTABLE(*s));
    mem::Free(s->data);
    *s = {};
}

bool operator==(String s1, String s2)
{
    return s1.len == s2.len && memcmp(s1.data, s2.data, s1.len) == 0;
};

bool operator!=(String s1, String s2)
{
    return s1.len != s2.len || memcmp(s1.data, s2.data, s1.len) != 0;
};

// djb2 string hash
u32 Hash(String str)
{
    u32 result = 5381;
    for(u32 i = 0; i < str.len; i++)
    {
        result = ((result << 5) + result) + (u8)str[i];
    }
    return result;
}

u32 Hash(const char* value)
{
    return Hash(IStr(value));
}

namespace str
{

i64 Find(String s, char target)
{
    for(i64 i = 0; i < s.len; i++)
    {
        if(s[i] == target)
        {
            return i;
        }
    }
    return -1;
}

i64 Find(String s, String target)
{
    ASSERT(target.len);
    if(target.len > s.len) return -1;

    // Naive string search. Can definitely be optimized.
    for(i64 i = 0; i < s.len - target.len + 1; i++)
    {
        if(s[i] == target[0])
        {
            bool match = true;
            for(i64 j = 1; j < target.len; j++)
            {
                if(s[i + j] != target[j])
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

i64 Find(String s, const char* target)
{
    return Find(s, IStr(target));
}

i64 RFind(String s, char target)
{
    for(i64 i = s.len - 1; i >= 0; i--)
    {
        if(s[i] == target)
        {
            return i;
        }
    }
    return -1;
}

i64 RFind(String s, String target)
{
    ASSERT(target.len);
    if(target.len > s.len) return -1;

    // Naive string search. Can definitely be optimized.
    for(i64 i = s.len - 1; i >= target.len - 1; i--)
    {
        if(s[i] == target[target.len - 1])
        {
            bool match = true;
            for(i64 j = target.len - 2; j >= 0; j--)
            {
                if(s[i - j] != target[j])
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

i64 RFind(String s, const char* target)
{
    return RFind(s, IStr(target));
}

String Substr(String s, u64 start)
{
    ASSERT(start < s.len);
    String result;
    result.capacity = 0;
    result.data = s.data + start;
    result.len = s.len - start;
    return result;
}

String Substr(String s, u64 start, u64 length)
{
    ASSERT(start + length <= s.len);
    String result;
    result.capacity = 0;
    result.len = length;
    result.data = s.data + start;
    return result;
}

Array<String> Split(String s, char delimiter)
{
    //TODO(caio): Implement me!
    ASSERT(0);
    return {};
}

String Concat(String s1, String s2)
{
    //TODO(caio): Implement me!
    ASSERT(0);
    return {};
}

void Clear(String& s)
{
    ASSERT(IS_MUTABLE(s));
    memset(s.data, 0, s.capacity);
    s.len = 0;
}

void Append(String& s, String other)
{
    ASSERT(IS_MUTABLE(s));
    ASSERT(s.len + other.len < s.capacity);
    memcpy((char*)(s.data + s.len), other.data, other.len);
    s.len = s.len + other.len;
}

void Append(String& s, const char* other)
{
    return Append(s, IStr(other));
}

void Format(String& s, const char* fmt, ...)
{
    ASSERT(IS_MUTABLE(s));
    va_list args;
    va_start(args, fmt);

    Clear(s);
    i64 length = vsnprintf((char*)s.data, s.capacity, fmt, args);
    ASSERT(length > 0);
    s.len = length;

    va_end(args);
}

};
};
