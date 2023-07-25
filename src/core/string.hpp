// ========================================================
// STRING
// String type and functionality
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"
#include "./memory.hpp"
#include "./debug.hpp"

namespace ty
{

// String types are made to be compatible with c strings.
// Strings can be MUTABLE (capacity > 0) or IMMUTABLE (capacity == 0)
struct String
{
    u64 capacity = 0;   // Total amount of bytes, INCLUDING the null terminator.
    u64 len      = 0;   // Amount of valid characters, EXCLUDING the null terminator.
    u8* data     = NULL;

    void CStr(char* output);
    u8 At(u64 i);

    i64 Find(char c);
    i64 Find(String s);
    i64 RFind(char c);
    i64 RFind(String s);

    // Substrings are created immutable
    String Substr(u64 start);
    String Substr(u64 start, u64 size);

    // Only supported by MUTABLE strings
    void Clear();
    void Append(String other);
    void Append(const char* other);
    //void Prepend(String other);
    //void Prepend(const char* other);
};
bool    StrEquals(String a, String b);
bool    operator==(String a, String b);
bool    operator!=(String a, String b);

String  MStr(u64 capacity, const char* value);
String  MStr(u64 capacity, String value);
String  MStr(u64 capacity);
String  MStr(const char* value);
String  MStr(String value);
String  IStr(String value);
String  IStr(const char* value);
String  IStr(u8* data, u64 len);
void    DestroyMStr(String* str);

#define ToCStr(S, bufname)  char bufname[(S).len + 1];\
                            (S).CStr(bufname)

u32 Hash(const char* cstr);
u32 Hash(String str);

#ifdef _NOLOGGING
#define LOGSTR(S)
#else
#define LOGSTR(S) LOG((S).CStr())
#endif

};
