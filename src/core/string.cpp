#include "./string.hpp"
#include "src/core/memory.hpp"
#include "string.h"
#include <vcruntime_string.h>

namespace ty
{

String Str(byte* data, u64 len)
{
    String result = {};
    result.data = data;
    result.len = len;
    return result;
}

String Str(const char* literal)
{
    return Str((byte*)literal, strlen(literal));
}

String Str(mem::Arena* arena, const char* value)
{
    u64 len = strlen(value);
    byte* buf = (byte*)mem::ArenaPush(arena, len + 1);
    memcpy(buf, value, len);
    buf[len] = 0;   // Null terminator for c-string compatibility.
    return Str(buf, len);
}

String Str(mem::Arena* arena, String value)
{
    u64 len = value.len;
    byte* buf = (byte*)mem::ArenaPush(arena, len + 1);
    memcpy(buf, value.data, len);
    buf[len] = 0;   // Null terminator for c-string compatibility.
    return Str(buf, len);
}

String::String()
{}

String::String(const char* value)
{
    //*this = IStr(value);
    *this = Str(value);
}

char& String::operator[](u64 index)
{
    ASSERT(index < len);
    return (char&)data[index];
};

char* String::CStr()
{
    return (char*)data;
}

//String IStr(const char* value)
//{
    //String result = {};
    //result.capacity = 0;
    //result.len = strlen(value);
    //result.data = (u8*)value;
    //return result;
//};

//String IStr(String value)
//{
    //String result = {};
    //result.capacity = 0;
    //result.len = value.len;
    //result.data = value.data;
    //return result;
//};

//void FreeMStr(String* s)
//{
    //ASSERT(IS_MUTABLE(*s));
    //mem::Free(s->data);
    //*s = {};
//}

bool operator==(String s1, String s2)
{
    return s1.len == s2.len && memcmp(s1.data, s2.data, s1.len) == 0;
};

bool operator!=(String s1, String s2)
{
    return s1.len != s2.len || memcmp(s1.data, s2.data, s1.len) != 0;
};

bool operator==(String s1, const char* s2)
{
    return s1 == Str(s2);
}

bool operator==(const char* s1, String s2)
{
    return Str(s1) == s2;
}

bool operator!=(String s1, const char* s2)
{
    return s1 != Str(s2);
}

bool operator!=(const char* s1, String s2)
{
    return Str(s1) != s2;
}

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
    return Hash(Str(value));
}

i64 StrFind(String s, char target)
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

i64 StrFind(String s, String target)
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

//i64 Find(String s, const char* target)
//{
    //return Find(s, IStr(target));
//}

i64 StrRFind(String s, char target)
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

i64 StrRFind(String s, String target)
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

//i64 RFind(String s, const char* target)
//{
    //return RFind(s, IStr(target));
//}

String Substr(String s, u64 start)
{
    ASSERT(start < s.len);
    String result;
    //result.capacity = 0;
    result.data = s.data + start;
    result.len = s.len - start;
    return result;
}

String Substr(String s, u64 start, u64 length)
{
    ASSERT(start + length <= s.len);
    String result;
    //result.capacity = 0;
    result.len = length;
    result.data = s.data + start;
    return result;
}

SArray<String> StrSplit(mem::Arena* arena, String s, char delimiter)
{
    //TODO(caio): Implement me!
    ASSERT(0);
    return {};
}

String StrConcat(mem::Arena* arena, String s1, String s2)
{
    String result = {};
    result.len = s1.len + s2.len;
    byte* buf = (byte*)mem::ArenaPush(arena, result.len + 1);
    memcpy(buf, s1.data, s1.len);
    memcpy(buf + s1.len, s2.data, s2.len);
    buf[result.len] = 0;   // Null terminator for c-string compatibility.
    result.data = buf;
    return result;
}

//void Clear(String& s)
//{
    //ASSERT(IS_MUTABLE(s));
    //memset(s.data, 0, s.capacity);
    //s.len = 0;
//}

//void Append(String& s, String other)
//{
    //ASSERT(IS_MUTABLE(s));
    //ASSERT(s.len + other.len < s.capacity);
    //memcpy((char*)(s.data + s.len), other.data, other.len);
    //s.len = s.len + other.len;
    //// Null term so string is always CStr compatible
    //*(s.data + s.len) = 0;
//}

//void Append(String& s, const char* other)
//{
    //return Append(s, IStr(other));
//}

//void Format(String& s, const char* fmt, ...)
//{
    //ASSERT(IS_MUTABLE(s));
    //va_list args;
    //va_start(args, fmt);

    //Clear(s);
    //i64 length = vsnprintf((char*)s.data, s.capacity, fmt, args);
    //ASSERT(length > 0);
    //s.len = length;

    //va_end(args);
//}

String StrFmt(mem::Arena* arena, const char* fmt, ...)
{
    ASSERT(0);  // TODO(caio): This is broken with String, fix it before using
    va_list args;
    va_start(args, fmt);

    i64 len = vsnprintf(0, 0, fmt, args);
    byte* buf = (byte*)mem::ArenaPush(arena, len + 1);
    vsnprintf((char*)buf, len, fmt, args);
    buf[len] = 0;   // Null terminator for c-string compatibility

    va_end(args);
    return Str(buf, len);
}

};
