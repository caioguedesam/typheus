// TYPHEUS ENGINE
// Project headers
#include "base.hpp"
#include "file.hpp"
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
#include "file.cpp"

int main()
{
    // Testing file handling functions
    MemArena arena;
    MemArenaInit(&arena, MB(1));

    Array filesAtDir = GetFilesAtDir(&arena, MakePath(Str(".")));
    for(i32 i = 0; i < filesAtDir.count; i++)
    {
        FilePath path;
        ArrayGet(filesAtDir, FilePath, i, &path);
        i32 a = 0;
    }
    return 0;
}
