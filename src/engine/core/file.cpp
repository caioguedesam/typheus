#include "engine/core/file.hpp"

namespace ty
{
namespace file
{

void Path::CStr(char* output) { return str.CStr(output); }

Path MakePath(String s)
{
    return { s };
}

Path MakePath(const char* value)
{
    return { IStr(value) };
}

Path MakePathAlloc(const char* value)
{
    return
    {
        MStr(MAX_PATH, value)
    };
}

Path MakePathAlloc(String s)
{
    return
    {
        MStr(MAX_PATH, s)
    };
}

Path GetAbsolute(Path path)
{
    ASSERT(path.Exists());
    PathToCStr(path, cstr);
    Path result = {};
    char resultBuffer[MAX_PATH];
    DWORD ret = GetFullPathName(
            cstr,
            MAX_PATH, resultBuffer, NULL);
    ASSERT(ret);
    result.str = MStr(MAX_PATH, resultBuffer);
    return result;
}

bool Path::Exists()
{
    PathToCStr(*this, cstr);
    DWORD fileAttributes = GetFileAttributes(cstr);
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool Path::IsDir()
{
    PathToCStr(*this, cstr);
    DWORD fileAttributes = GetFileAttributes(cstr);
    return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

Path Path::GetExtension()
{
    ASSERT(Exists());
    ASSERT(!IsDir());
    u64 extStart = str.RFind('.');
    ASSERT(extStart != -1);
    return MakePath(str.Substr(extStart));
}

Path Path::RemoveExtension()
{
    ASSERT(Exists());
    ASSERT(!IsDir());
    u64 extStart = str.RFind('.');
    ASSERT(extStart != -1);
    return MakePath(str.Substr(0, extStart));
}

Path Path::GetFileName(bool extension)
{   
    ASSERT(Exists());
    ASSERT(!IsDir());

    u64 lastSlash = str.RFind('\\');
    if(lastSlash == -1) lastSlash = str.RFind('/');

    String result;
    if(lastSlash == -1)
        result = str.Substr(0);
    else
        result = str.Substr(lastSlash + 1);

    if(!extension)
        result = result.Substr(0, result.len - GetExtension().str.len);

    return MakePath(result);
}

Path Path::GetFileDir()
{   
    ASSERT(Exists());
    ASSERT(!IsDir());

    u64 lastSlash = str.RFind('\\');
    if(lastSlash == -1) lastSlash = str.RFind('/');
    ASSERT(lastSlash != -1);

    return MakePath(str.Substr(0, lastSlash + 1));
}

u64 GetFileSize(Path path)
{
    ASSERT(path.Exists());
    ASSERT(!path.IsDir());

    PathToCStr(path, cstr);
    HANDLE hFile = CreateFile(
            cstr,
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

//std::vector<Path> GetFilesInDir(mem::AllocatorArena* arena, Path dir)
List<Path> GetFilesInDir(Path dir)
{
    ASSERT(dir.Exists());
    ASSERT(dir.IsDir());

    // Create a temp buffer to store query string
    char queryBuffer[MAX_PATH];
    PathToCStr(dir, cstr);
    sprintf(queryBuffer, "%s\\*", cstr);

    // Make query for files
    HANDLE fileHandle;
    WIN32_FIND_DATAA fileData;
    List<Path> result = MakeList<Path>();

    fileHandle = FindFirstFile(queryBuffer, &fileData);
    ASSERT(fileHandle != INVALID_HANDLE_VALUE);
    do
    {
        char fBuffer[MAX_PATH];
        sprintf(fBuffer, "%s\\%s", cstr, fileData.cFileName);
        Path fPath = MakePathAlloc(fBuffer);
        if(!fPath.IsDir())
        {
            result.Push(fPath);
        }
    } while(FindNextFile(fileHandle, &fileData));

    return result;
}

u64 ReadFile(Path path, u8* output)
{
    PathToCStr(path, cstr);
    HANDLE hFile = CreateFile(
            cstr,
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
            output,
            fSize,
            &bytesRead,
            NULL);
    ASSERT(ret);

    CloseHandle(hFile);
    return (u64)bytesRead;
}

String ReadFileToString(Path path)
{
    u64 fSize = GetFileSize(path);
    String result = MStr(fSize + 1); // +1 for null terminator
    u64 bytesRead = ReadFile(path, result.data);
    ASSERT(bytesRead == fSize);
    result.len = bytesRead;
    result.data[fSize] = 0; // Null terminator
    return result;
}

u8* ReadFileToBuffer(Path path, u64* size)
{
    u64 fSize = GetFileSize(path);
    u8* result = (u8*)mem::Alloc(fSize);
    u64 bytesRead = ReadFile(path, result);
    ASSERT(bytesRead == fSize);
    if(size) *size = fSize;
    return result;
}

};
};
