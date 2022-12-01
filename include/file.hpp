// TYPHEUS ENGINE - FILE HANDLING
#pragma once
#include "base.hpp"

// FilePath type is currently just a wrapper to cstr.
struct FilePath
{
    String str;
};
FilePath MakePath(String str);

FilePath GetAbsolutePath(FilePath relPath, u8* buffer, u64 size);
FilePath GetAbsolutePath(MemArena* arena, FilePath relPath);

bool IsDirectory(FilePath path);

String GetExtension(FilePath path);
String GetFilename(FilePath path, bool withExtension = true);
String GetFileDir(FilePath path);

u64 ReadFile(FilePath path, u8* buffer);
u8* ReadFile(MemArena* arena, FilePath path);
u64 WriteFile(FilePath path, u8* data);

Array GetFilesAtDir(MemArena* arena, FilePath dir);
