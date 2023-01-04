// ========================================================
// COMMON
// Code shared between my C++ projects. Currently is OS-specific to windows.
// @Caio Guedes, 2022
// ========================================================
#pragma once
#include <windows.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <float.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <algorithm>

#if _PROFILE
#include "tracy/Tracy.hpp"
#endif

namespace Ty
{

// ========================================================
// [BASE TYPES]
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

// [GENERAL DEFINES AND MACROS]

#define MAX_U8  (0xFF)
#define MAX_U16 (0xFFFF)
#define MAX_U32 (0xFFFFFFFFUL)
#define MAX_U64 (0xFFFFFFFFFFFFFFFFULL)

#define EPSILON_F32 FLT_EPSILON
#define EPSILON_F64 DBL_EPSILON

#undef KB
#undef MB
#undef GB
#define KB(V) ((V)   * 1024)
#define MB(V) (KB(V) * 1024)
#define GB(V) (MB(V) * 1024)

#undef MIN
#undef MAX
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, A, B) (MAX((A), MIN((V), (B))))
#define CLAMP_CEIL(V, A) MIN(V, A)
#define CLAMP_FLOOR(V, A) MAX(V, A)
#define ABS(V) ((V) < 0 ? -(V) : (V))

#define STMT(S) do { S; } while(0)
#define STRINGIFY(S) #S     // For macro arg expansion
#define ALIGN_TO(SIZE, BOUND) (((SIZE) + (BOUND) - 1) & ~((BOUND) - 1))   // Aligns to powers of 2 only
#define IS_ALIGNED(SIZE, BOUND) ((u64)(SIZE) % (BOUND) == 0)

#define ArrayCount(arr) (sizeof(arr)/sizeof(*(arr)))
#define ArraySize(arr) (sizeof(arr))

#define StructOffset(type, member) ((u64)&((type*)0)->member)

// ========================================================
// [ASSERT]
#if _NO_ASSERT
#define ASSERT(EXPR)
#define ASSERTF(EXPR, ...)
#else
void Assert(u64 expr, const char* msg);
void AssertFormat(u64 expr, const char* fmt, ...);

#define ASSERT(EXPR) STMT(Assert((u64)(EXPR), STRINGIFY(EXPR)))
#define ASSERTF(EXPR, FMT, ...) STMT(AssertFormat((i32)(EXPR), FMT, __VA_ARGS__))
#endif

// ========================================================
// [LOGGING]
#if _NO_LOG
#define LOG(MSG)
#define LOGL(LABEL, MSG)
#define LOGF(FMT, ...)
#define LOGLF(LABEL, FMT, ...)
#else
void LogFormat(const char* label, const char* fmt, ...);
#define LOG(MSG) STMT(LogFormat("LOG", "%s", MSG))
#define LOGL(LABEL, MSG) STMT(LogFormat(LABEL, "%s", MSG))
#define LOGF(FMT, ...) STMT(LogFormat("LOG", FMT, __VA_ARGS__))
#define LOGLF(LABEL, FMT, ...) STMT(LogFormat(LABEL, FMT, __VA_ARGS__))
#endif

// ========================================================
// [PROFILING]
#if _PROFILE
#define PROFILE_FRAME FrameMark
#define PROFILE_SCOPED ZoneScoped
//TODO(caio)#PROFILING: Add support for named zones and colors
#else
#define PROFILE_FRAME
#define PROFILE_SCOPED
#endif

// ========================================================
// [MEMORY]
// Arena Allocator
struct MemArena
{
    u64 capacity = 0;
    u8* start = 0;
    u64 offset = 0;
};

void  MemArena_Init(MemArena* arena, u64 capacity);
void  MemArena_Destroy(MemArena* arena);

void* MemArena_Alloc(MemArena* arena, u64 size);
void* MemArena_AllocAlign(MemArena* arena, u64 size, u64 alignment);
void* MemArena_AllocZero(MemArena* arena, u64 size);
void* MemArena_AllocAlignZero(MemArena* arena, u64 size, u64 alignment);
void  MemArena_Free(MemArena* arena, u64 size);
void  MemArena_Clear(MemArena* arena);

// ========================================================
// [RANDOM]
u64 RandomU64();
f32 RandomUniform();

i32 RandomRange(i32 start, i32 end);
f32 RandomRange(f32 start, f32 end);

// ========================================================
// [TIME]

#define TIMER_INVALID MAX_U64

struct Timer
{
    u64 start   = TIMER_INVALID;
    u64 end     = TIMER_INVALID;
};

void Time_Init();

void StartTimer(Timer* timer);
void EndTimer(Timer* timer);

u64 GetTicks(const Timer& timer);
f64 GetTime_NS(const Timer& timer);
f64 GetTime_MS(const Timer& timer);
f64 GetTime_S(const Timer& timer);

// ========================================================
// [FILE]
bool PathExists(std::string_view path);
bool IsDirectory(std::string_view path);
std::string GetAbsolutePath(std::string_view path);
std::string_view GetExtension(std::string_view path);
std::string_view GetFilename(std::string_view path, bool withExtension = false);
std::string_view GetFileDir(std::string_view path);
u64 GetFileSize(std::string_view path);
std::vector<std::string> GetFilesAtDir(std::string_view dirPath);

u64 ReadFile(std::string_view path, u8* buffer);
std::string ReadFile_Str(std::string_view path);

// ========================================================
// [MATH]
// Math defines
#define PI 3.14159265358979323846

#define TO_DEG(X) (X * (180.0 / PI))
#define TO_RAD(X) (X * (PI / 180.0))
// Vectors
// Vector2f (f32: x,y | u,v)
struct v2f
{
    union
    {
        struct
        {
            union { f32 x; f32 u; };
            union { f32 y; f32 v; };
        };
        f32 data[2];
    };
};
bool operator==(const v2f& a, const v2f& b);
v2f operator+(const v2f& a, const v2f& b);
v2f operator-(const v2f& a, const v2f& b);
v2f operator*(const v2f& a, const v2f& b);
v2f operator*(const v2f& a, const f32& b);
v2f operator*(const f32& a, const v2f& b);

f32 Dot(const v2f& a, const v2f& b);
f32 Cross(const v2f& a, const v2f& b);
f32 Len2(const v2f& v);
f32 Len(const v2f& v);
v2f Normalize(const v2f& v);
f32 AngleBetween(const v2f& a, const v2f& b);

// Vector2i (i32: x,y)
struct v2i
{
    union
    {
        struct
        {
            i32 x; i32 y;
        };
        i32 data[2];
    };
};
bool operator==(const v2i& a, const v2i& b);
v2i operator+(const v2i& a, const v2i& b);
v2i operator-(const v2i& a, const v2i& b);

// Vector3f (f32: x,y,z | r,g,b)
struct v3f
{
    union
    {
        struct
        {
            union { f32 x; f32 r; };
            union { f32 y; f32 g; };
            union { f32 z; f32 b; };
        };
        f32 data[3];
    };
};
bool operator==(const v3f& a, const v3f& b);
v3f operator+(const v3f& a, const v3f& b);
v3f operator-(const v3f& a, const v3f& b);
v3f operator*(const v3f& a, const v3f& b);
v3f operator*(const v3f& a, const f32& b);
v3f operator*(const f32& a, const v3f& b);
f32 Dot(const v3f& a, const v3f& b);
v3f Cross(const v3f& a, const v3f& b);
f32 Len2(const v3f& v);
f32 Len(const v3f& v);
v3f Normalize(const v3f& v);

// Vector4f (f32: x,y,z,w | r,g,b,a)
struct v4f
{
    union
    {
        struct
        {
            union { f32 x; f32 r; };
            union { f32 y; f32 g; };
            union { f32 z; f32 b; };
            union { f32 w; f32 a; };
        };
        f32 data[4];
    };
};
bool operator==(const v4f& a, const v4f& b);
v4f operator+(const v4f& a, const v4f& b);
v4f operator-(const v4f& a, const v4f& b);
v4f operator*(const v4f& a, const v4f& b);
v4f operator*(const v4f& a, const f32& b);
v4f operator*(const f32& a, const v4f& b);

f32 Dot(const v4f& a, const v4f& b);
f32 Len2(const v4f& v);
f32 Len(const v4f& v);
v4f Normalize(const v4f& v);

// TODO(caio)#MATH: Implement quaternion type

// Matrix4f (f32, row-major)    // TODO(caio)#MATH: Test and profile row/column major perf
struct m4f
{
    union
    {
        struct
        {
            f32 m00; f32 m01; f32 m02; f32 m03;
            f32 m10; f32 m11; f32 m12; f32 m13;
            f32 m20; f32 m21; f32 m22; f32 m23;
            f32 m30; f32 m31; f32 m32; f32 m33;
        };
        f32 data[16];
    };
};
bool operator==(const m4f& a, const m4f& b);
m4f operator+(const m4f& a, const m4f& b);
m4f operator-(const m4f& a, const m4f& b);
m4f operator*(const m4f& a, const m4f& b);
m4f operator*(const f32& a, const m4f& b);
v4f operator*(const m4f& a, const v4f& v);

f32 Determinant(const m4f& m);
m4f Transpose(const m4f& m);
m4f Inverse(const m4f& m);

m4f Identity();
m4f ScaleMatrix(const v3f& scale);
m4f RotationMatrix(const f32& angle, const v3f& axis);
m4f TranslationMatrix(const v3f& move);

v3f TransformPosition(const v3f& position, const m4f& transform);
v3f TransformDirection(const v3f& direction, const m4f& transform);

m4f LookAtMatrix(const v3f& center, const v3f& target, const v3f& up);
m4f PerspectiveProjectionMatrix(const f32& fovY, const f32& aspectRatio, const f32& nearPlane, const f32& farPlane);

// Math utilities
f32 Lerp(const f32& a, const f32& b, const f32& t);
v2f Lerp(const v2f& a, const v2f& b, const f32& t);
v3f Lerp(const v3f& a, const v3f& b, const f32& t);

// ========================================================
// [INPUT]
// This uses OS specific types, since input is OS specific.
// All virtual keys supported. These map directly to Win32 virtual key codes.
enum InputKey : u32
{
    KEY_INVALID         = 0x00,
    KEY_LMB             = 0x01,
    KEY_RMB             = 0x02,
    KEY_MMB             = 0x03,
    KEY_BACKSPACE       = 0x08,
    KEY_TAB             = 0x09,
    KEY_RETURN          = 0x0D,
    KEY_SHIFT           = 0x10,
    KEY_CTRL            = 0x11,
    KEY_ALT             = 0x12,
    KEY_ESCAPE          = 0x1B,
    KEY_SPACE           = 0x20,
    KEY_ARROW_LEFT      = 0x25,
    KEY_ARROW_UP        = 0x26,
    KEY_ARROW_RIGHT     = 0x27,
    KEY_ARROW_DOWN      = 0x28,
    KEY_0               = 0x30,
    KEY_1               = 0x31,
    KEY_2               = 0x32,
    KEY_3               = 0x33,
    KEY_4               = 0x34,
    KEY_5               = 0x35,
    KEY_6               = 0x36,
    KEY_7               = 0x37,
    KEY_8               = 0x38,
    KEY_9               = 0x39,
    KEY_A               = 0x41,
    KEY_B               = 0x42,
    KEY_C               = 0x43,
    KEY_D               = 0x44,
    KEY_E               = 0x45,
    KEY_F               = 0x46,
    KEY_G               = 0x47,
    KEY_H               = 0x48,
    KEY_I               = 0x49,
    KEY_J               = 0x4A,
    KEY_K               = 0x4B,
    KEY_L               = 0x4C,
    KEY_M               = 0x4D,
    KEY_N               = 0x4E,
    KEY_O               = 0x4F,
    KEY_P               = 0x50,
    KEY_Q               = 0x51,
    KEY_R               = 0x52,
    KEY_S               = 0x53,
    KEY_T               = 0x54,
    KEY_U               = 0x55,
    KEY_V               = 0x56,
    KEY_W               = 0x57,
    KEY_X               = 0x58,
    KEY_Y               = 0x59,
    KEY_Z               = 0x5A,
    // TODO(caio)#INPUT: Add more whenever needed.
};

struct MouseState
{
    v2i pos;    // Mouse position in pixels starting from TOP-LEFT of application window.
    v2f delta;  // Current mouse delta direction from last position
    bool hidden = false;
    bool locked = false;
};

struct InputState
{
    u8 buttons[256];
    MouseState mouse;
};

void Input_UpdateState();
void Input_UpdateMouseState(MouseState* mouseState);
void Input_LockMouse(bool lock);
void Input_ToggleLockMouse();
void Input_ShowMouse(bool show);
void Input_ToggleShowMouse();

bool Input_IsKeyDown(InputKey key);
bool Input_IsKeyUp(InputKey key);
bool Input_IsKeyJustDown(InputKey key);
bool Input_IsKeyJustUp(InputKey key);

v2i Input_GetMousePosition();
v2f Input_GetMouseDelta();

}   // namespace Ty
