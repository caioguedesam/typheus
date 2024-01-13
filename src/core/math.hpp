// ========================================================
// MATH
// Mathematics constants, functions and utilities.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"

namespace ty
{
namespace math
{

// ========================================================
// [CONSTANTS]
#define PI 3.14159265358979323846

// ========================================================
// [MACROS]
#define TO_DEG(X) (X * (180.0 / PI))
#define TO_RAD(X) (X * (PI / 180.0))

// ========================================================
// [VECTORS]
// All vector types have data stored in a contiguous array.

struct v2f;
struct v2i;
struct v3f;
struct v4f;

// Vector2f (f32: x,y | u,v)
struct v2f
{
    union
    {
        struct
        {
            union { f32 x = 0; f32 u; };
            union { f32 y = 0; f32 v; };
        };
        f32 data[2];
    };
};
bool operator==(v2f a, v2f b);
bool operator!=(v2f a, v2f b);
v2f  operator+ (v2f a, v2f b);
v2f  operator- (v2f a, v2f b);
v2f  operator* (v2f a, v2f b);
v2f  operator* (v2f a, f32 b);
v2f  operator* (f32 a, v2f b);

f32 Dot             (v2f a, v2f b);
f32 Cross           (v2f a, v2f b);
f32 AngleBetween    (v2f a, v2f b);
f32 Len2            (v2f v);
f32 Len             (v2f v);
v2f Normalize       (v2f v);

// Vector2i (i32: x,y)
struct v2i
{
    union
    {
        struct
        {
            i32 x = 0; i32 y = 0;
        };
        i32 data[2];
    };
};
bool operator==(v2i a, v2i b);
bool operator!=(v2i a, v2i b);
v2i  operator+ (v2i a, v2i b);
v2i  operator- (v2i a, v2i b);

// Vector3f (f32: x,y,z | r,g,b)
struct v3f
{
    union
    {
        struct
        {
            union { f32 x = 0; f32 r; };
            union { f32 y = 0; f32 g; };
            union { f32 z = 0; f32 b; };
        };
        f32 data[3];
    };

    v4f AsDirection();
    v4f AsPosition();
};
bool operator==(v3f a, v3f b);
bool operator!=(v3f a, v3f b);
v3f  operator+ (v3f a, v3f b);
v3f  operator- (v3f a, v3f b);
v3f  operator* (v3f a, v3f b);
v3f  operator* (v3f a, f32 b);
v3f  operator* (f32 a, v3f b);

f32 Dot         (v3f a, v3f b);
v3f Cross       (v3f a, v3f b);
f32 Len2        (v3f v);
f32 Len         (v3f v);
v3f Normalize   (v3f v);

// Vector4f (f32: x,y,z,w | r,g,b,a)
struct v4f
{
    union
    {
        struct
        {
            union { f32 x = 0; f32 r; };
            union { f32 y = 0; f32 g; };
            union { f32 z = 0; f32 b; };
            union { f32 w = 0; f32 a; };
        };
        f32 data[4];
    };

    v3f AsXYZ();
};
bool operator==(v4f a, v4f b);
bool operator!=(v4f a, v4f b);
v4f  operator+ (v4f a, v4f b);
v4f  operator- (v4f a, v4f b);
v4f  operator* (v4f a, v4f b);
v4f  operator* (v4f a, f32 b);
v4f  operator* (f32 a, v4f b);

f32 Dot         (v4f a, v4f b);
f32 Len2        (v4f v);
f32 Len         (v4f v);
v4f Normalize   (v4f v);

// ========================================================
// [MATRIX]
// All matrix types have data stored in a contiguous array, and are row-major in memory.

// Matrix4f (f32)
struct m4f
{
    union
    {
        struct
        {
            f32 m00 = 0; f32 m01 = 0; f32 m02 = 0; f32 m03 = 0;
            f32 m10 = 0; f32 m11 = 0; f32 m12 = 0; f32 m13 = 0;
            f32 m20 = 0; f32 m21 = 0; f32 m22 = 0; f32 m23 = 0;
            f32 m30 = 0; f32 m31 = 0; f32 m32 = 0; f32 m33 = 0;
        };
        f32 data[16];
    };
};
bool operator==(m4f a, m4f b);
bool operator!=(m4f a, m4f b);
m4f  operator+ (m4f a, m4f b);
m4f  operator- (m4f a, m4f b);
m4f  operator* (m4f a, m4f b);
m4f  operator* (f32 a, m4f b);
v4f  operator* (m4f a, v4f v);

m4f Identity();
f32 Determinant     (m4f m);
m4f Transpose       (m4f m);
m4f Inverse         (m4f m);

m4f ScaleMatrix         (v3f scale);
m4f RotationMatrix      (f32 angle, v3f axis);
m4f TranslationMatrix   (v3f move);

v3f TransformPosition   (v3f position,  m4f transform);
v3f TransformDirection  (v3f direction, m4f transform);

m4f LookAtLH        (v3f center, v3f target, v3f up);
m4f LookAtRH        (v3f center, v3f target, v3f up);
m4f PerspectiveLH   (f32 fov, f32 aspect, f32 zNear, f32 zFar);
m4f PerspectiveRH   (f32 fov, f32 aspect, f32 zNear, f32 zFar);

//TODO(caio): Implement orthogonal projection
//TODO(caio): Implement quaternions

// ========================================================
// [EASING]

f32 Lerp(f32 a, f32 b, f32 t);
v2f Lerp(v2f a, v2f b, f32 t);
v3f Lerp(v3f a, v3f b, f32 t);

//TODO(caio): More easing functions

// ========================================================
// [RANDOM]
u64 RandomU64();

f32 RandomUniformF32();
f32 RandomUniformF32(f32 start, f32 end);
i32 RandomUniformI32(i32 start, i32 end);

//TODO(caio): More random distributions if needed

};

typedef math::v2f v2f;
typedef math::v2i v2i;
typedef math::v3f v3f;
typedef math::v4f v4f;
typedef math::m4f m4f;
};
