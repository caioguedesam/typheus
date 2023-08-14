// ========================================================
// FILE
// File system utilites, such as reading/writing and path manipulation.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"
#include "./string.hpp"
#include "./ds.hpp"

namespace ty
{
namespace file
{

struct Path
{
    String str = {};

    char* CStr();
    bool Exists();
    bool IsDir();
    
    String Extension();
    String WithoutExtension();
    String FileName(bool extension = false);
    String FileDir();
};

Path MakePath(String s);

u64 GetFileSize(Path path);
List<Path> GetFilesInDir(Path dir);

u64     ReadFile(Path path, u8* output);
String  ReadFileToString(Path path);
u8*     ReadFileToBuffer(Path path, u64* size = NULL);

// TODO(caio): Implement file writing

};
};
