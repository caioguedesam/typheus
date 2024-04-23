#include "./file.hpp"

namespace ty
{
namespace file
{

bool PathExists(String path)
{
    DWORD fileAttributes = GetFileAttributes(path.CStr());
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool PathIsDir(String path)
{
    DWORD fileAttributes = GetFileAttributes(path.CStr());
    return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

String PathExt(String path)
{
    ASSERT(PathExists(path));
    ASSERT(!PathIsDir(path));
    u64 extStart = StrRFind(path, '.');
    ASSERT(extStart != -1);
    return Substr(path, extStart);
}

String PathNoExt(String path)
{
    ASSERT(PathExists(path));
    ASSERT(!PathIsDir(path));
    u64 extStart = StrRFind(path, '.');
    ASSERT(extStart != -1);
    return Substr(path, 0, extStart);
}

String PathFileName(String path, bool extension)
{   
    ASSERT(PathExists(path));
    ASSERT(!PathIsDir(path));

    u64 lastSlash = StrRFind(path, '\\');
    if(lastSlash == -1) lastSlash = StrRFind(path, '/');

    String result;
    if(lastSlash == -1)
        result = Substr(path, 0);
    else
        result = Substr(path, lastSlash + 1);

    if(!extension)
        result = Substr(result, 0, result.len - PathExt(path).len);

    return result;
}

String PathFileDir(String path)
{   
    ASSERT(PathExists(path));
    ASSERT(!PathIsDir(path));

    u64 lastSlash = StrRFind(path, '\\');
    if(lastSlash == -1) lastSlash = StrRFind(path, '/');
    ASSERT(lastSlash != -1);

    return Substr(path, 0, lastSlash + 1);
}

u64 GetFileSize(String path)
{
    ASSERT(PathExists(path));
    ASSERT(!PathIsDir(path));

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

SArray<String> GetFilesInDir(mem::Arena* arena, String dirPath)
{
    // TODO(caio): Implement me!
    ASSERT(0);
    return {};
    //ASSERT(dir.Exists());
    //ASSERT(dir.IsDir());

    //// Create a temp buffer to store query string
    //char queryBuffer[MAX_PATH];
    //sprintf(queryBuffer, "%s\\*", dir.CStr());

    //// Loop to find file count for array size
    //WIN32_FIND_DATAA fData;
    //HANDLE fHandle = FindFirstFile(queryBuffer, &fData);
    //u32 fCount = 0;
    //if(fHandle != INVALID_HANDLE_VALUE)
    //{
        //do
        //{
            //fCount++;
        //} while(FindNextFile(fHandle, &fData));
    //}

    //// Make query for files
    //HANDLE fileHandle;
    //WIN32_FIND_DATAA fileData;
    //DArray<Path> result = MakeDArray<Path>(arena);

    //fileHandle = FindFirstFile(queryBuffer, &fileData);
    //ASSERT(fileHandle != INVALID_HANDLE_VALUE);
    //do
    //{
        //char fBuffer[MAX_PATH];
        //sprintf(fBuffer, "%s\\%s", dir.CStr(), fileData.cFileName);

        //MStr(fPathStr, MAX_PATH);
        //Path fPath = MakePath(fPathStr);
        //if(!fPath.IsDir())
        //{
            //result.Push(fPath);
        //}
    //} while(FindNextFile(fileHandle, &fileData));

    //return result;
}

u64 ReadFile(String path, byte* output)
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

String ReadFileToString(mem::Arena* arena, String path)
{
    u64 fSize = GetFileSize(path);
    byte* buf = (byte*)mem::ArenaPush(arena, fSize + 1);
    u64 len = ReadFile(path, buf);
    ASSERT(len == fSize);
    buf[len] = 0;   // Null terminator for c-string compatibility.
    return Str(buf, len);
}

byte* ReadFileToBuffer(mem::Arena* arena, String path, u64* size)
{
    u64 fSize = GetFileSize(path);
    byte* result = (byte*)mem::ArenaPush(arena, fSize);
    u64 bytesRead = ReadFile(path, result);
    ASSERT(bytesRead == fSize);
    if(size) *size = fSize;
    return result;
}

};
};
