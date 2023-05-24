// ========================================================
// FILE
// File system utilites, such as reading/writing and path manipulation.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"
#include "engine/core/string.hpp"
#include "engine/core/ds.hpp"

namespace ty
{
namespace file
{

struct Path
{
    String str = {};

    void CStr(char* output);
    bool Exists();
    bool IsDir();
    
    Path GetExtension();
    Path RemoveExtension();
    Path GetFileName(bool extension = false);
    Path GetFileDir();
};
Path MakePath(String s);
Path MakePath(const char* value);
Path MakePathAlloc(String s);
Path MakePathAlloc(const char* value);
Path GetAbsolute(Path path);

u64 GetFileSize(Path path);
List<Path> GetFilesInDir(Path dir);

u64     ReadFile(Path path, u8* output);
String  ReadFileToString(Path path);
u8*     ReadFileToBuffer(Path path, u64* size = NULL);
#define ReadFileToStackBuffer(P, bufname)   u8 bufname[GetFileSize((P))];\
                                            ReadFile((P), bufname)

// TODO(caio): Implement file writing

#define PathToCStr(P, bufname) ToCStr((P).str, bufname)

};
};
