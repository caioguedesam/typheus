// ========================================================
// STRING
// String type and functionality
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"
#include "./memory.hpp"
#include "./ds.hpp"
#include "./debug.hpp"

namespace ty
{

// [STRING]
// Strings in typheus are immutable. They can be a view to a literal, or to an arena allocated buffer.
struct String
{
    //u64 capacity    = 0;
    u64 len         = 0;
    byte* data        = NULL;

    char& operator[](u64 index);

    char* CStr();

    String();
    String(const char* value);  // Implicit conversion from const char*
};
bool operator==(String s1, String s2);
bool operator!=(String s1, String s2);
bool operator==(String s1, const char* s2);
bool operator==(const char* s1, String s2);
bool operator!=(String s1, const char* s2);
bool operator!=(const char* s1, String s2);
u32 Hash(String s);
u32 Hash(const char* value);

String Str(byte* data, u64 len);
String Str(const char* literal);
String Str(mem::Arena* arena, const char* value);
String Str(mem::Arena* arena, String value);

// String operations can return strings that simply view subsets of operands.
// Whenever a string operation creates a new string, it requires an arena to be specified.
i64 StrFind(String s, char target);
i64 StrFind(String s, String target);
i64 StrRFind(String s, char target);
i64 StrRFind(String s, String target);
String Substr(String s, u64 start);
String Substr(String s, u64 start, u64 length);

SArray<String> StrSplit(mem::Arena* arena, String s, char delimiter);
String StrConcat(mem::Arena* arena, String s1, String s2);
String StrFmt(mem::Arena* arena, const char* fmt, ...);

};
