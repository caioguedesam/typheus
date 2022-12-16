#pragma once
#include "core/base.hpp"

namespace Ty
{
// FilePath type is currently just a wrapper to cstr.
struct FilePath
{
    String str;
};
FilePath MakePath(String str);
FilePath MakePath(const char* cstr);

FilePath GetAbsolutePath(FilePath relPath, u8* buffer, u64 size);
FilePath GetAbsolutePath(MemArena* arena, FilePath relPath);

bool PathExists(FilePath path);
bool IsDirectory(FilePath path);

String GetExtension(FilePath path);
String GetFilename(FilePath path, bool withExtension = true);
String GetFileDir(FilePath path);

u64 GetFileSize(FilePath path);

u64 ReadFile(FilePath path, u8* buffer);
Array<u8> ReadFileToArray(MemArena* arena, FilePath path);
String ReadFileToStr(MemArena* arena, FilePath path);
//u64 WriteFile(FilePath path, u8* data);   // TODO(caio)#FILE: I don't need this for now, implement whenever needed.

Array<FilePath> GetFilesAtDir(MemArena* arena, FilePath dir);

}   // namespace Ty
