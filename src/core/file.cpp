#include "core/file.hpp"

namespace Sol
{

// This uses OS specific file handling functions.
FilePath MakePath(String str)
{
    ASSERT(IsCStr(str));
    return {str};
}

FilePath GetAbsolutePath(FilePath relPath, u8* buffer, u64 size)
{
    ASSERT(PathExists(relPath));
    DWORD ret = GetFullPathName(
            ToCStr(relPath.str),
            size, (char*)buffer, NULL);
    ASSERT(ret);
    return MakePath(Str(buffer, ret));
}

FilePath GetAbsolutePath(MemArena* arena, FilePath relPath)
{
    ASSERT(PathExists(relPath));
    u8* pathBuffer = (u8*)MemAlloc(arena, MAX_PATH);
    FilePath result = GetAbsolutePath(relPath, pathBuffer, MAX_PATH);
    i64 remainingBytes = MAX_PATH - (result.str.len + 1);   // +1 for null-terminator.
    MemFree(arena, CLAMP_FLOOR(remainingBytes, 0));
    return result;
}

bool PathExists(FilePath path)
{
    DWORD fileAttributes = GetFileAttributes(ToCStr(path.str));
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool IsDirectory(FilePath path)
{
    DWORD fileAttributes = GetFileAttributes(ToCStr(path.str));
    return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

String GetExtension(FilePath path)
{
    ASSERT(PathExists(path));
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
    ASSERT(PathExists(path));
    ASSERT(!IsDirectory(path));
    i64 dirEnd = StrFindR(path.str, Str("\\"));
    if(dirEnd == STR_INVALID)
    {
        dirEnd = StrFindR(path.str, Str("/"));
        if(dirEnd == STR_INVALID)
        {
            dirEnd = 0;     // No directory before file in path.
        }
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
    ASSERT(PathExists(path));
    ASSERT(!IsDirectory(path));
    i64 dirEnd = StrFindR(path.str, Str("\\"));
    if(dirEnd == STR_INVALID)
    {
        dirEnd = StrFindR(path.str, Str("/"));
        if(dirEnd == STR_INVALID)
        {
            return Str(".");    // The path has no directories, so it points to CWD.
        }
    }

    String result;
    result.data = path.str.data;
    result.len = dirEnd + 1;
    return result;
}

u64 GetFileSize(FilePath path)
{
    ASSERT(PathExists(path));
    ASSERT(!IsDirectory(path));

    HANDLE hFile = CreateFile(
            ToCStr(path.str),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    DWORD fSize = ::GetFileSize(hFile, NULL);
    ASSERT(fSize != INVALID_FILE_SIZE);
    CloseHandle(hFile);
    return (u64)fSize;
}

u64 ReadFile(FilePath path, u8* buffer)
{
    ASSERT(PathExists(path));
    ASSERT(!IsDirectory(path));

    HANDLE hFile = CreateFile(
            ToCStr(path.str),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    DWORD fSize = ::GetFileSize(hFile, NULL);
    ASSERT(fSize != INVALID_FILE_SIZE);
    DWORD bytesRead = 0;
    BOOL ret = ::ReadFile(
            hFile,
            buffer,
            fSize,
            &bytesRead,
            NULL);
    ASSERT(ret);

    CloseHandle(hFile);
    return (u64)bytesRead;
}

Array<u8> ReadFile(MemArena* arena, FilePath path)
{
    u64 fSize = GetFileSize(path);
    Array<u8> result = ArrayAlloc<u8>(arena, fSize);
    u64 bytesRead = ReadFile(path, result.data);
    ASSERT(bytesRead == fSize);
    result.count = bytesRead;
    return result;
}

Array<FilePath> GetFilesAtDir(MemArena* arena, FilePath dir)
{
    ASSERT(PathExists(dir));
    ASSERT(IsDirectory(dir));

    // Create a temp buffer to store query string
    u8 queryBuffer[dir.str.len + 2];
    String queryStr = Strf(queryBuffer, "%s\\*", ToCStr(dir.str));

    // Make query for files
    HANDLE fileHandle;
    WIN32_FIND_DATAA fileData;
    fileHandle = FindFirstFile(ToCStr(queryStr), &fileData);
    ASSERT(fileHandle != INVALID_HANDLE_VALUE);

    Array<FilePath> result = ArrayAlloc<FilePath>(arena, 256);
    do
    {
        FilePath foundFile = MakePath(StrfAlloc(arena, "%s\\%s", ToCStr(dir.str), fileData.cFileName));
        if(!IsDirectory(foundFile))
        {
            result.Push(foundFile);
        }
    } while(FindNextFile(fileHandle, &fileData));

    return result;
}

}   // namespace Sol
