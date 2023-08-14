// ========================================================
// Series of test functions written to ensure core library works fine.
// @Caio Guedes, 2023
// ========================================================

#include "./base.hpp"
#include "./debug.hpp"
#include "./memory.hpp"
#include "./string.hpp"
#include "./math.hpp"
#include "./file.hpp"
#include "./input.hpp"
#include "./time.hpp"
#include "./async.hpp"
#include "./ds.hpp"

namespace ty
{

void TestMemory()
{
    //TODO(caio): Review and retest arena allocator when I need it again
#if 0
    // Arena allocator
    mem::ArenaAllocator testArena = mem::MakeArenaAllocator(2048);
    ASSERT(testArena.region.start);
    ASSERT(testArena.region.capacity == 2048);
    mem::SetContext(&testArena);

    {
        u32* p = (u32*)mem::Alloc(sizeof(u32));
        ASSERT(p);
        ASSERT(testArena.offset == sizeof(u32));
        *p = 10;
        ASSERT(*p == 10);
        p = (u32*)mem::AllocZero(sizeof(u32));
        *p = 20;
        ASSERT(*p == 20);
        *(p - 1) = 30;
        ASSERT(*(p - 1) == 30);

        u8* p2 = (u8*)mem::AllocZero(sizeof(u8));
        ASSERT(testArena.offset == 2 * sizeof(u32) + sizeof(u8));
        ASSERT(*p2 == 0);

        mem::FreeAll();
        ASSERT(testArena.offset == 0);
    }

    ////TODO(caio): Redo these tests when aligned alloc is reimplemented
    //void* pAlign = mem::Alloc(sizeof(u32), 32);
    //ASSERT(IS_ALIGNED(pAlign, 32));
    //pAlign = testArena.Alloc(sizeof(u8));
    //pAlign = testArena.Alloc(sizeof(u32), 32);
    //ASSERT(IS_ALIGNED(pAlign, 32));

    mem::DestroyArenaAllocator(&testArena);
#endif

    // Heap allocator
    mem::HeapAllocator testHeap = mem::MakeHeapAllocator(MB(1));
    ASSERT(testHeap.region.start);
    mem::SetContext(&testHeap);


    u32* p = (u32*)mem::Alloc(sizeof(u32));
    mem::Free(p);

    u32* p1 = (u32*)mem::Alloc(sizeof(u32));
    u32* p2 = (u32*)mem::Alloc(sizeof(u32));
    u32* p3 = (u32*)mem::Alloc(sizeof(u32));
    mem::Free(p2);
    mem::Free(p3);

    p2 = (u32*)mem::AllocAlign(sizeof(u32) * 8, 16);
    p3 = (u32*)mem::AllocAlign(sizeof(u32) * 8, 16);
    mem::Free(p2);

    u32* p4 = (u32*)mem::Alloc(sizeof(u32));
    mem::Free(p4);
    mem::Free(p3);
    mem::Free(p1);

    p1 = (u32*)mem::Alloc(sizeof(u32));
    *p1 = 20;
    p2 = (u32*)mem::Alloc(sizeof(u32) * 4);
    p3 = (u32*)mem::Alloc(sizeof(u32));
    mem::Free(p2);
    p2 = (u32*)mem::Alloc(sizeof(u32));
    *p2 = 10;
    p2 = (u32*)mem::Realloc(p2, sizeof(u32) * 2);
    ASSERT(*p2 == 10);
    p1 = (u32*)mem::Realloc(p1, sizeof(u32) + 2);
    ASSERT(*p1 == 20);
    p1 = (u32*)mem::Realloc(p1, sizeof(u32) + 24);
    ASSERT(*p1 == 20);

    mem::Free(p1);
    mem::Free(p2);
    mem::Free(p3);

    List<u32> v1 = MakeList<u32>();
    List<math::v3f> v2 = MakeList<math::v3f>();
    for(i32 i = 0; i < 512; i++)
    {
        v1.Push(i);
        v2.Push({});
    }
    mem::FreeAll();

    mem::DestroyHeapAllocator(&testHeap);
}

void TestArray()
{
    //mem::AllocatorArena arrayArena;
    //arrayArena.Init(MB(1));
    mem::ArenaAllocator arrayArena = mem::MakeArenaAllocator(MB(1));
    mem::SetContext(&arrayArena);
    //Array<u32> arr = MakeArray<u32>(&arrayArena, 1024);
    Array<u32> arr = MakeArray<u32>(1024);
    ASSERT(arr.data);
    arr.Push(1);
    arr.Push(2);
    arr.Push(3);
    arr.Push(4);
    ASSERT(arr[0] == 1);
    ASSERT(arr[1] == 2);
    ASSERT(arr[2] == 3);
    ASSERT(arr[3] == 4);
    u32 a = arr.Pop();
    ASSERT(a == 4);
    a = arr.Pop();
    ASSERT(a == 3);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 1);
    ASSERT(arr[1] == 2);

    arr.Clear();
    arr.Push(10);
    ASSERT(arr.count == 1);
    ASSERT(arr[0] == 10);

    mem::DestroyArenaAllocator(&arrayArena);
}

void TestList()
{
    mem::HeapAllocator arrayHeap = mem::MakeHeapAllocator(MB(1));
    mem::SetContext(&arrayHeap);

    List<u32> arr = MakeList<u32>();
    ASSERT(arr.data);
    ASSERT(arr.capacity == 16);
    ASSERT(arr.count == 0);
    for(i32 i = 0; i < 32; i++)
    {
        arr.Push(i);
    }
    ASSERT(arr.capacity == 32);
    ASSERT(arr.count == 32);
    for(i32 i = 0; i < 10; i++)
    {
        arr.Pop();
    }
    ASSERT(arr.capacity == 32);
    ASSERT(arr.count == 22);

    mem::DestroyHeapAllocator(&arrayHeap);
}

void TestString()
{
    mem::ArenaAllocator stringArena = mem::MakeArenaAllocator(MB(1));
    mem::SetContext(&stringArena);

    const char* test1_cstr = "test 1";
    const char* test2_cstr = "test 22";
    //String a = MStr(1024, test1_cstr);
    MStr(a, 1024);
    str::Format(a, "%s", test1_cstr);
    ASSERT(a.len == strlen(test1_cstr));
    ASSERT(strcmp(a.CStr(), test1_cstr) == 0);
    String b = IStr(a);
    ASSERT(b.len == strlen(test1_cstr));
    ASSERT(strcmp(b.CStr(), test1_cstr) == 0);
    ASSERT(a == b);

    String c = IStr(test2_cstr);
    ASSERT(a != c);

    ASSERT(str::Find(a, 's') == 2);
    ASSERT(str::Find(c, IStr("t 2")) == 3);
    ASSERT(str::RFind(a, 't') == 3);
    ASSERT(str::RFind(c, IStr("2")) == 6);
    ASSERT(str::Find(c, IStr("2")) == 5);

    String sa = str::Substr(a, 2);
    ASSERT(sa == IStr("st 1"));
    String sc = str::Substr(c, 2, 4);
    ASSERT(sc == IStr("st 2"));
    ASSERT(IStr("") == str::Substr(a, 0,0));

    str::Clear(a);
    ASSERT(IStr("") == a);
    str::Append(a, "a");
    ASSERT(IStr("a") == a);
    str::Append(a, c);
    ASSERT(IStr("atest 22") == a);

    mem::DestroyArenaAllocator(&stringArena);
}

void TestFile()
{
    mem::ArenaAllocator fileArena = mem::MakeArenaAllocator(MB(32));
    mem::SetContext(&fileArena);

    const char* filePath = "./temp/test.txt";

    file::Path path1 = file::MakePath(IStr(filePath));
    ASSERT(path1.str == IStr(filePath));
    ASSERT(path1.Exists());
    ASSERT(!path1.IsDir());
    file::Path path1Dir = file::MakePath(path1.FileDir());
    ASSERT(path1Dir.Exists());
    ASSERT(path1Dir.IsDir());
    ASSERT(path1Dir.str == IStr("./temp/"));
    String path1Name = path1.FileName();
    ASSERT(path1Name == IStr("test"));
    path1Name = path1.FileName(true);
    ASSERT(path1Name == IStr("test.txt"));
    String path1Ext = path1.Extension();
    ASSERT(path1Ext == IStr(".txt"));

    u64 fSize = file::GetFileSize(path1);
    ASSERT(fSize);
    String fileStr = file::ReadFileToString(path1);
    ASSERT(fileStr == IStr("This is a test file!"));
    u64 fSize2 = 0;
    u8* fData = file::ReadFileToBuffer(path1, &fSize2);
    ASSERT(fSize == fSize2);

    mem::DestroyArenaAllocator(&fileArena);
}

void TestMath()
{
    using namespace math;
    // v2i
    {
        v2i a = {0, 1};
        v2i b = {0, 1};
        ASSERT(a == b);
        a = {1, 1};
        ASSERT(a != b);
        v2i c = {1, 2};
        ASSERT(a + b == c);
        c = {1, 0};
        ASSERT(a - b == c);
    }
    // v2f
    {
        v2f a = {0, 1};
        v2f b = {0, 1};
        ASSERT(a == b);
        a = {1, 0};
        ASSERT(a != b);
        v2f c = {1, 1};
        ASSERT(a + b == c);
        c = {1, -1};
        ASSERT(a - b == c);
        c = {0, 0};
        ASSERT(a * b == c);
        c = {2, 0};
        ASSERT(2 * a == c);
        ASSERT(a * 2 == c);

        ASSERT(Dot(a, b) == 0);
        ASSERT(Cross(a, b) == 1);
        f32 angle = AngleBetween(a, b);
        f32 halfPi = PI/2.f;
        ASSERT(angle == halfPi);

        c = {3, 0};
        ASSERT(Len2(c) == 9);
        ASSERT(Len(c) == 3);
        ASSERT(Len(Normalize(c)) == 1);
    }
    // v3f
    {
        v3f a = {0, 1, 0};
        v3f b = {0, 1, 0};
        ASSERT(a == b);
        a = {1, 0, 0};
        ASSERT(a != b);
        v3f c = {1, 1, 0};
        ASSERT(a + b == c);
        c = {1, -1, 0};
        ASSERT(a - b == c);
        c = {0, 0, 0};
        ASSERT(a * b == c);
        c = {2, 0, 0};
        ASSERT(2 * a == c);
        ASSERT(a * 2 == c);

        ASSERT(Dot(a, b) == 0);
        c = {0, 0, 1};
        ASSERT(Cross(a, b) == c);

        c = {3, 0, 0};
        ASSERT(Len2(c) == 9);
        ASSERT(Len(c) == 3);
        ASSERT(Len(Normalize(c)) == 1);

        v4f d = {1, 0, 0, 0};
        ASSERT(a.AsDirection() == d);
        d = {1, 0, 0, 1};
        ASSERT(a.AsPosition() == d);
    }
    // v4f
    {
        v4f a = {0, 1, 0, 0};
        v4f b = {0, 1, 0, 0};
        ASSERT(a == b);
        a = {1, 0, 0, 0};
        ASSERT(a != b);
        v4f c = {1, 1, 0, 0};
        ASSERT(a + b == c);
        c = {1, -1, 0, 0};
        ASSERT(a - b == c);
        c = {0, 0, 0, 0};
        ASSERT(a * b == c);
        c = {2, 0, 0, 0};
        ASSERT(2 * a == c);
        ASSERT(a * 2 == c);

        ASSERT(Dot(a, b) == 0);

        c = {3, 0, 0, 0};
        ASSERT(Len2(c) == 9);
        ASSERT(Len(c) == 3);
        ASSERT(Len(Normalize(c)) == 1);

        v3f d = {1, 0, 0};
        ASSERT(a.AsXYZ() == d);
    }
    // m4f
    {
        m4f a = Identity();
        m4f i = 
        {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1,
        };
        ASSERT(a == i);
        i.m01 = 1;
        ASSERT(a != i);
        m4f b =
        {
            1,0,0,1,
            0,1,0,1,
            0,0,1,1,
            0,0,0,1,
        };
        m4f c =
        {
            2,0,0,1,
            0,2,0,1,
            0,0,2,1,
            0,0,0,2,
        };
        ASSERT(a + b == c);
        c =
        {
            0,0,0,-1,
            0,0,0,-1,
            0,0,0,-1,
            0,0,0,0,
        };
        ASSERT(a - b == c);

        c =
        {
            2,0,0,0,
            0,2,0,0,
            0,0,2,0,
            0,0,0,2,
        };
        ASSERT(2 * a == c);
        ASSERT(a * b == b);

        v4f d = {1,2,3,4};
        ASSERT(a * d == d);
        v4f e = {2,4,6,8};
        ASSERT(c * d == e);
        c =
        {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            1,1,1,1,
        };
        ASSERT(Transpose(b) == c);
        ASSERT(Transpose(a) == a);
        c =
        {
            1,0,0,-1,
            0,1,0,-1,
            0,0,1,-1,
            0,0,0,1,
        };
        ASSERT(Inverse(b) == c);
        ASSERT(Inverse(a) == a);
        
    }
    // Easing
    {
        ASSERT(Lerp(0, 2, 0.5f) == 1);
        v2f r2 = {1, 0};
        ASSERT(Lerp(v2f{0,0}, v2f{2,0}, 0.5f) == r2);
        v3f r3 = {1, 0, 0.5f};
        ASSERT(Lerp(v3f{0,0,0}, v3f{2,0,1}, 0.5f) == r3);
    }
}

// Run just this on main
void TestCore()
{
    TestMemory();
    TestArray();
    TestString();
    TestMath();
    TestFile();
}

};
