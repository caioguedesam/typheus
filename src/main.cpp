// TYPHEUS ENGINE
// Project headers
#include "base.hpp"
#include "time.hpp"
#include "math.hpp"
#include <stdio.h>

// Compiling just one file to dramatically speed up compile times
// Dependencies
#if _PROFILE
#include "TracyClient.cpp"
#endif

// Project files
#include "base.cpp"
#include "time.cpp"
#include "math.cpp"

void PrintArray(Array& arr)
{
    for(i32 i = 0; i < arr.count; i++)
    {
        f32 v;
        ArrayGet(arr, f32, i, &v);
        printf("%f |", v);
    }
    printf("\n");
}

int main()
{
    MemArena arena;
    MemArenaInit(&arena, 1024);

    char buf[128];
    sprintf(buf, "Test string 1");
    String str1 = Str((u8*)buf, strlen(buf));

    String str2 = Str("Test string 2 (cstr)");

    i32 a = 10;

    char readBuffer[1024];
    bool result = StrRead(&str2, (u8*)readBuffer);
    result = StrRead(&str2, 5, (u8*)readBuffer, 5);

    a = 10;

    String str3 = StrAlloc(&arena, (u8*)readBuffer, 20);
    String str4 = StrAlloc(&arena, "Test string 4 (cstr)");

    a = 10;

    String str5 = Strf((u8*)readBuffer, "Format test %d %f %s", 3, 4.5f, "hi!");
    String str6 = StrfAlloc(&arena, "Second format test! %d %c %.2f", 5, 'u', 10.2f);

    a = 10;

    return 0;
}
