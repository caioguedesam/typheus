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

bool PathExists(String path);
bool PathIsDir(String path);
String PathExt(String path);
String PathNoExt(String path);
String PathFileName(String path, bool extension = false);
String PathFileDir(String path);

u64 GetFileSize(String path);
SArray<String> GetFilesInDir(mem::Arena* arena, String dirPath);

u64     ReadFile(String path, byte* output);
String  ReadFileToString(mem::Arena* arena, String path);
byte*   ReadFileToBuffer(mem::Arena* arena, String path, u64* size = NULL);

// TODO(caio): Implement file writing

};
};
