#include "./file.hpp"

namespace ty
{
namespace file
{

char* Path::CStr()
{
    return str.CStr();
}

Path MakePath(String s)
{
    return { .str = s };
}

bool Path::Exists()
{
    DWORD fileAttributes = GetFileAttributes(CStr());
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool Path::IsDir()
{
    DWORD fileAttributes = GetFileAttributes(CStr());
    return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

String Path::Extension()
{
    ASSERT(Exists());
    ASSERT(!IsDir());
    u64 extStart = str::RFind(str, '.');
    ASSERT(extStart != -1);
    return str::Substr(str, extStart);
}

String Path::WithoutExtension()
{
    ASSERT(Exists());
    ASSERT(!IsDir());
    u64 extStart = str::RFind(str, '.');
    ASSERT(extStart != -1);
    return str::Substr(str, 0, extStart);
}

String Path::FileName(bool extension)
{   
    ASSERT(Exists());
    ASSERT(!IsDir());

    u64 lastSlash = str::RFind(str, '\\');
    if(lastSlash == -1) lastSlash = str::RFind(str, '/');

    String result;
    if(lastSlash == -1)
        result = str::Substr(str, 0);
    else
        result = str::Substr(str, lastSlash + 1);

    if(!extension)
        result = str::Substr(result, 0, result.len - Extension().len);

    return result;
}

String Path::FileDir()
{   
    ASSERT(Exists());
    ASSERT(!IsDir());

    u64 lastSlash = str::RFind(str, '\\');
    if(lastSlash == -1) lastSlash = str::RFind(str, '/');
    ASSERT(lastSlash != -1);

    return str::Substr(str, 0, lastSlash + 1);
}

u64 GetFileSize(Path path)
{
    ASSERT(path.Exists());
    ASSERT(!path.IsDir());

    HANDLE hFile = CreateFile(
            path.CStr(),
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

List<Path> GetFilesInDir(Path dir)
{
    ASSERT(dir.Exists());
    ASSERT(dir.IsDir());

    // Create a temp buffer to store query string
    char queryBuffer[MAX_PATH];
    sprintf(queryBuffer, "%s\\*", dir.CStr());

    // Make query for files
    HANDLE fileHandle;
    WIN32_FIND_DATAA fileData;
    List<Path> result = MakeList<Path>();

    fileHandle = FindFirstFile(queryBuffer, &fileData);
    ASSERT(fileHandle != INVALID_HANDLE_VALUE);
    do
    {
        char fBuffer[MAX_PATH];
        sprintf(fBuffer, "%s\\%s", dir.CStr(), fileData.cFileName);

        MStr(fPathStr, MAX_PATH);
        Path fPath = MakePath(fPathStr);
        if(!fPath.IsDir())
        {
            result.Push(fPath);
        }
    } while(FindNextFile(fileHandle, &fileData));

    return result;
}

u64 ReadFile(Path path, u8* output)
{
    HANDLE hFile = CreateFile(
            path.CStr(),
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
    MStr(result, fSize + 1);
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
