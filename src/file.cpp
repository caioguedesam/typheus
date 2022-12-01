// TYPHEUS ENGINE - FILE HANDLING
#include "file.hpp"
// This uses OS specific file handling functions.
FilePath MakePath(String str)
{
    ASSERT(IsCStr(str));
    return {str};
}

FilePath GetAbsolutePath(FilePath relPath, u8* buffer, u64 size)
{
    DWORD fileAttributes = GetFileAttributes((char*)relPath.str.data);
    ASSERT(fileAttributes != INVALID_FILE_ATTRIBUTES);
    DWORD ret = GetFullPathName(
            (char*)relPath.str.data,
            size, (char*)buffer, NULL);
    ASSERT(ret);
    // Let's add a trailing slash to directory paths
    if(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        buffer[ret] = '\\';
        ret += 1;
    }
    return MakePath(Str(buffer, ret));
}

FilePath GetAbsolutePath(MemArena* arena, FilePath relPath)
{
    u8* pathBuffer = (u8*)MemAlloc(arena, MAX_PATH);
    FilePath result = GetAbsolutePath(relPath, pathBuffer, MAX_PATH);
    i64 remainingBytes = MAX_PATH - (result.str.len + 1);   // +1 for null-terminator.
    MemFree(arena, CLAMP_FLOOR(remainingBytes, 0));
    return result;
}

bool IsDirectory(FilePath path)
{
    // Simply verifies if the last char in string is a slash.
    return path.str.data[path.str.len - 1] == '\\'
    ||     path.str.data[path.str.len - 1] == '/';
}

String GetExtension(FilePath path)
{
    ASSERT(!IsDirectory(path));
    i64 extStart = StrFindR(path.str, Str("."));
    ASSERT(extStart != STR_INVALID);
    String result;
    result.data = path.str.data + extStart + 1;
    result.len = path.str.len - extStart - 1;
    return result;
}

String GetFilename(FilePath path, bool withExtension)
{
    ASSERT(!IsDirectory(path));
    i64 dirEnd = StrFindR(path.str, Str("\\"));
    if(dirEnd == STR_INVALID)
    {
        dirEnd = StrFindR(path.str, Str("/"));
        ASSERT(dirEnd != STR_INVALID);
    }

    String result;
    result.data = path.str.data + dirEnd + 1;
    result.len = path.str.len - dirEnd - 1;

    if(!withExtension)
    {
        String ext = GetExtension(MakePath(result));
        result.len -= ext.len + 1;  // Remove the dot as well.
    }

    return result;
}

String GetFileDir(FilePath path)
{
    ASSERT(!IsDirectory(path));
    i64 dirEnd = StrFindR(path.str, Str("\\"));
    if(dirEnd == STR_INVALID)
    {
        dirEnd = StrFindR(path.str, Str("/"));
        ASSERT(dirEnd != STR_INVALID);
    }

    String result;
    result.data = path.str.data;
    result.len = dirEnd + 1;
    return result;
}

//u64 ReadFile(FilePath path, u8* buffer);
//u8* ReadFile(MemArena* arena, FilePath path);
//u64 WriteFile(FilePath path, u8* data);

//Array GetFilesAtDir(MemArena* arena, FilePath dir);
