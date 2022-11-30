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

    Array splitResult = StrSplit(&arena, Str("ababa ababa abab"));

    String s;
    ArrayGet(splitResult, String, 0, &s);
    ArrayGet(splitResult, String, 1, &s);
    ArrayGet(splitResult, String, 2, &s);

    i32 a = 10;

    return 0;
}
