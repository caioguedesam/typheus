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

// #define IS_MUTABLE(s) ((s).capacity > 0)
// 
// // String types:
// // - Immutable: represents an immutable string, with capacity = 0.
// //      - Underlying memory points to either a string literal, or any
// //      string memory which shouldn't be modified.
// //      - Constructed with IStr.
// //      - Cannot be mutated by operations such as Append.
// String IStr(const char* value);
// String IStr(String value);
// 
// // - Stack: represents a string backed by a char buffer in stack memory.
// //      - Constructed with SStr.
// //      - Can be mutated by operations such as Append.
// #define SStrName(NAME) CONCATENATE(NAME, _sstrbuf)
// #define SStr(NAME, CAPACITY) String NAME;\
//     u8 SStrName(NAME)[CAPACITY];\
//     NAME.capacity = CAPACITY;\
//     NAME.len = 0;\
//     NAME.data = SStrName(NAME)
// 
// // - Allocated: represents a string backed by a buffer allocated by any allocator.
// //      - Constructed with MStr.
// //      - Can be mutated by operations such as Append.
// #define MStrName(NAME) CONCATENATE(NAME, _mstrbuf)
// #define MStr(NAME, CAPACITY) String NAME;\
//     u8* MStrName(NAME) = (u8*)mem::AllocZero(CAPACITY);\
//     NAME.capacity = CAPACITY;\
//     NAME.len = 0;\
//     NAME.data = MStrName(NAME)
// void FreeMStr(String* s);
// 
// // Strings constructed with this API are made to be compatible with C-style strings.
// // The string LENGTH doesn't take into account the null terminator.
// // The string CAPACITY (for mutable strings) does take into account the null terminator.
// 
// namespace str
// {
// 
// // General string operations
// // - Performed on both immutable and mutable strings.
// // - In-place:
// i64 Find(String s, char target);
// i64 Find(String s, String target);
// i64 Find(String s, const char* target);
// i64 RFind(String s, char target);
// i64 RFind(String s, String target);
// i64 RFind(String s, const char* target);
// String Substr(String s, u64 start);
// String Substr(String s, u64 start, u64 length);
// // - Memory allocating:
// Array<String> Split(String s, char delimiter);
// String Concat(String s1, String s2);
// 
// // Mutable-only string operations
// // In-place:
// void Clear(String& s);
// void Format(String& s, const char* fmt, ...);
// void Append(String& s, String other);
// void Append(String& s, const char* other);

//};
};
