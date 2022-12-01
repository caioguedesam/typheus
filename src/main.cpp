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

    FilePath path1 = MakePath(Str("path/to/file1.png"));

    FilePath path3 = GetAbsolutePath(&arena, MakePath(Str(".")));
    FilePath path4 = GetAbsolutePath(&arena, MakePath(Str("..")));

    bool isDir = IsDirectory(path3);
    isDir = IsDirectory(path1);

    String ext = GetExtension(path1);
    String fname = GetFilename(path1);
    String fnameNoExt = GetFilename(path1, false);
    String dirname = GetFileDir(path1);

    i32 a = 0;
    return 0;
}
