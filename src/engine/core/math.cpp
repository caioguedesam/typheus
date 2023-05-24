#include "engine/core/math.hpp"

namespace ty
{
namespace math
{

bool operator==(v2f a, v2f b)
{
    return a.x == b.x && a.y == b.y;
}

bool operator!=(v2f a, v2f b)
{
    return a.x != b.x || a.y != b.y;
}

v2f operator+(v2f a, v2f b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
    };
}

v2f operator-(v2f a, v2f b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
    };
}

v2f operator*(v2f a, v2f b)
{
    return
    {
        a.x * b.x,
        a.y * b.y,
    };
}

v2f operator*(v2f a, f32 b)
{
    return
    {
        a.x * b,
        a.y * b,
    };
}

v2f operator*(f32 a, v2f b)
{
    return
    {
        b.x * a,
        b.y * a,
    };
}

f32 Dot(v2f a, v2f b)
{
    return a.x * b.x + a.y * b.y;
}

f32 Cross(v2f a, v2f b)
{
    // Cross-product is only defined in 3D space. 2D version only returns Z coordinate.
    // This can be useful for calculating winding order between points, for example.
    return a.x * b.y - a.y * b.x;
}

f32 Len2(v2f v)
{
    return Dot(v, v);
}

f32 Len(v2f v)
{
    return sqrt(Len2(v));
}

v2f Normalize(v2f v)
{
    f32 l = Len(v);
    if(l < EPSILON_F32) return {0.f, 0.f};
    return v * (1.f/l);
}

f32 AngleBetween(v2f a, v2f b)
{
    // Angle in radians between 2 vectors, from 0 to PI
    return acos(Dot(Normalize(a), Normalize(b)));
}

bool operator==(v2i a, v2i b)
{
    return a.x == b.x && a.y == b.y;
}

bool operator!=(v2i a, v2i b)
{
    return a.x != b.x || a.y != b.y;
}

v2i operator+(v2i a, v2i b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
    };
}

v2i operator-(v2i a, v2i b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
    };
}

v4f v3f::AsDirection() { return { x, y, z, 0 }; }
v4f v3f::AsPosition() { return { x, y, z, 1 }; }

bool operator==(v3f a, v3f b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!=(v3f a, v3f b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

v3f operator+(v3f a, v3f b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

v3f operator-(v3f a, v3f b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

v3f operator*(v3f a, v3f b)
{
    return
    {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
    };
}

v3f operator*(v3f a, f32 b)
{
    return
    {
        a.x * b,
        a.y * b,
        a.z * b,
    };
}

v3f operator*(f32 a, v3f b)
{
    return
    {
        b.x * a,
        b.y * a,
        b.z * a,
    };
}

f32 Dot(v3f a, v3f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

v3f Cross(v3f a, v3f b)
{
    return 
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

f32 Len2(v3f v)
{
    return Dot(v, v);
}

f32 Len(v3f v)
{
    return sqrt(Len2(v));
}

v3f Normalize(v3f v)
{
    f32 l = Len(v);
    if(l < EPSILON_F32) return {0.f, 0.f};
    return v * (1.f/l);
}

v3f v4f::AsXYZ() { return { x, y, z}; }

bool operator==(v4f a, v4f b)
{
    return a.x == b.x
        && a.y == b.y
        && a.z == b.z
        && a.w == b.w;
}

bool operator!=(v4f a, v4f b)
{
    return a.x != b.x
        || a.y != b.y
        || a.z != b.z
        || a.w != b.w;
}

v4f operator+(v4f a, v4f b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w,
    };
}

v4f operator-(v4f a, v4f b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w,
    };
}

v4f operator*(v4f a, v4f b)
{
    return
    {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w,
    };
}

v4f operator*(v4f a, f32 b)
{
    return
    {
        a.x * b,
        a.y * b,
        a.z * b,
        a.w * b,
    };
}

v4f operator*(f32 a, v4f b)
{
    return
    {
        b.x * a,
        b.y * a,
        b.z * a,
        b.w * a,
    };
}

f32 Dot(v4f a, v4f b)
{
    return a.x * b.x
         + a.y * b.y
         + a.z * b.z
         + a.w * b.w;
}

f32 Len2(v4f v)
{
    return Dot(v, v);
}

f32 Len(v4f v)
{
    return sqrt(Len2(v));
}

v4f Normalize(v4f v)
{
    f32 l = Len(v);
    if(l < EPSILON_F32) return {0.f, 0.f};
    return v * (1.f/l);
}

bool operator==(m4f a, m4f b)
{
    // Could be optimized by using SIMD. Can't use memcmp because of precision.
    bool row0 = a.m00 == b.m00 && a.m01 == b.m01 && a.m02 == b.m02 && a.m03 == b.m03;
    bool row1 = a.m10 == b.m10 && a.m11 == b.m11 && a.m12 == b.m12 && a.m13 == b.m13;
    bool row2 = a.m20 == b.m20 && a.m21 == b.m21 && a.m22 == b.m22 && a.m23 == b.m23;
    bool row3 = a.m30 == b.m30 && a.m31 == b.m31 && a.m32 == b.m32 && a.m33 == b.m33;
    return row0 && row1 && row2 && row3;
}

bool operator!=(m4f a, m4f b)
{
    // Could be optimized by using SIMD. Can't use memcmp because of precision.
    bool row0 = a.m00 != b.m00 || a.m01 != b.m01 || a.m02 != b.m02 || a.m03 != b.m03;
    bool row1 = a.m10 != b.m10 || a.m11 != b.m11 || a.m12 != b.m12 || a.m13 != b.m13;
    bool row2 = a.m20 != b.m20 || a.m21 != b.m21 || a.m22 != b.m22 || a.m23 != b.m23;
    bool row3 = a.m30 != b.m30 || a.m31 != b.m31 || a.m32 != b.m32 || a.m33 != b.m33;
    return row0 || row1 || row2 || row3;
}

// TODO(caio)#MATH: Matrix operations should be vectorized
m4f operator+(m4f a, m4f b)
{
    return
    {
        a.m00 + b.m00,
        a.m01 + b.m01,
        a.m02 + b.m02,
        a.m03 + b.m03,
        a.m10 + b.m10,
        a.m11 + b.m11,
        a.m12 + b.m12,
        a.m13 + b.m13,
        a.m20 + b.m20,
        a.m21 + b.m21,
        a.m22 + b.m22,
        a.m23 + b.m23,
        a.m30 + b.m30,
        a.m31 + b.m31,
        a.m32 + b.m32,
        a.m33 + b.m33,
    };
}

m4f operator-(m4f a, m4f b)
{
    return
    {
        a.m00 - b.m00,
        a.m01 - b.m01,
        a.m02 - b.m02,
        a.m03 - b.m03,
        a.m10 - b.m10,
        a.m11 - b.m11,
        a.m12 - b.m12,
        a.m13 - b.m13,
        a.m20 - b.m20,
        a.m21 - b.m21,
        a.m22 - b.m22,
        a.m23 - b.m23,
        a.m30 - b.m30,
        a.m31 - b.m31,
        a.m32 - b.m32,
        a.m33 - b.m33,
    };
}

m4f operator*(m4f a, m4f b)
{
    return
    {
        // Row 0
        a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30, 
        a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31, 
        a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32, 
        a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33, 

        // Row 1
        a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30, 
        a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31, 
        a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32, 
        a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33, 

        // Row 2
        a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30, 
        a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31, 
        a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32, 
        a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33, 

        // Row 3
        a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30, 
        a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31, 
        a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32, 
        a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33, 
    };
}

m4f operator*(f32 b, m4f a)
{
    return
    {
        a.m00 * b, a.m01 * b, a.m02 * b, a.m03 * b,
        a.m10 * b, a.m11 * b, a.m12 * b, a.m13 * b,
        a.m20 * b, a.m21 * b, a.m22 * b, a.m23 * b,
        a.m30 * b, a.m31 * b, a.m32 * b, a.m33 * b,
    };
}

v4f operator*(m4f a, v4f v)
{
    return
    {
        a.m00 * v.x + a.m01 * v.y + a.m02 * v.z + a.m03 * v.w,
        a.m10 * v.x + a.m11 * v.y + a.m12 * v.z + a.m13 * v.w,
        a.m20 * v.x + a.m21 * v.y + a.m22 * v.z + a.m23 * v.w,
        a.m30 * v.x + a.m31 * v.y + a.m32 * v.z + a.m33 * v.w,
    };
}

f32 Determinant(m4f m)
{
    return
        m.m03 * m.m12 * m.m21 * m.m30 - m.m02 * m.m13 * m.m21 * m.m30 -
        m.m03 * m.m11 * m.m22 * m.m30 + m.m01 * m.m13 * m.m22 * m.m30 +
        m.m02 * m.m11 * m.m23 * m.m30 - m.m01 * m.m12 * m.m23 * m.m30 -
        m.m03 * m.m12 * m.m20 * m.m31 + m.m02 * m.m13 * m.m20 * m.m31 +
        m.m03 * m.m10 * m.m22 * m.m31 - m.m00 * m.m13 * m.m22 * m.m31 -
        m.m02 * m.m10 * m.m23 * m.m31 + m.m00 * m.m12 * m.m23 * m.m31 +
        m.m03 * m.m11 * m.m20 * m.m32 - m.m01 * m.m13 * m.m20 * m.m32 -
        m.m03 * m.m10 * m.m21 * m.m32 + m.m00 * m.m13 * m.m21 * m.m32 +
        m.m01 * m.m10 * m.m23 * m.m32 - m.m00 * m.m11 * m.m23 * m.m32 -
        m.m02 * m.m11 * m.m20 * m.m33 + m.m01 * m.m12 * m.m20 * m.m33 +
        m.m02 * m.m10 * m.m21 * m.m33 - m.m00 * m.m12 * m.m21 * m.m33 -
        m.m01 * m.m10 * m.m22 * m.m33 + m.m00 * m.m11 * m.m22 * m.m33;
}

m4f Transpose(m4f m)
{
    return
    {
        m.m00, m.m10, m.m20, m.m30,
        m.m01, m.m11, m.m21, m.m31,
        m.m02, m.m12, m.m22, m.m32,
        m.m03, m.m13, m.m23, m.m33,
    };
}

m4f Inverse(m4f m)
{
    f32 A2323 = m.m22 * m.m33 - m.m23 * m.m32;
    f32 A1323 = m.m21 * m.m33 - m.m23 * m.m31;
    f32 A1223 = m.m21 * m.m32 - m.m22 * m.m31;
    f32 A0323 = m.m20 * m.m33 - m.m23 * m.m30;
    f32 A0223 = m.m20 * m.m32 - m.m22 * m.m30;
    f32 A0123 = m.m20 * m.m31 - m.m21 * m.m30;
    f32 A2313 = m.m12 * m.m33 - m.m13 * m.m32;
    f32 A1313 = m.m11 * m.m33 - m.m13 * m.m31;
    f32 A1213 = m.m11 * m.m32 - m.m12 * m.m31;
    f32 A2312 = m.m12 * m.m23 - m.m13 * m.m22;
    f32 A1312 = m.m11 * m.m23 - m.m13 * m.m21;
    f32 A1212 = m.m11 * m.m22 - m.m12 * m.m21;
    f32 A0313 = m.m10 * m.m33 - m.m13 * m.m30;
    f32 A0213 = m.m10 * m.m32 - m.m12 * m.m30;
    f32 A0312 = m.m10 * m.m23 - m.m13 * m.m20;
    f32 A0212 = m.m10 * m.m22 - m.m12 * m.m20;
    f32 A0113 = m.m10 * m.m31 - m.m11 * m.m30;
    f32 A0112 = m.m10 * m.m21 - m.m11 * m.m20;

    f32 det = m.m00 * (m.m11 * A2323 - m.m12 * A1323 + m.m13 * A1223)
    - m.m01 * (m.m10 * A2323 - m.m12 * A0323 + m.m13 * A0223)
    + m.m02 * (m.m10 * A1323 - m.m11 * A0323 + m.m13 * A0123)
    - m.m03 * (m.m10 * A1223 - m.m11 * A0223 + m.m12 * A0123);
    det = 1 / det;

    return
    {
        det *  (m.m11 * A2323 - m.m12 * A1323 + m.m13 * A1223),
        det * -(m.m01 * A2323 - m.m02 * A1323 + m.m03 * A1223),
        det *  (m.m01 * A2313 - m.m02 * A1313 + m.m03 * A1213),
        det * -(m.m01 * A2312 - m.m02 * A1312 + m.m03 * A1212),
        det * -(m.m10 * A2323 - m.m12 * A0323 + m.m13 * A0223),
        det *  (m.m00 * A2323 - m.m02 * A0323 + m.m03 * A0223),
        det * -(m.m00 * A2313 - m.m02 * A0313 + m.m03 * A0213),
        det *  (m.m00 * A2312 - m.m02 * A0312 + m.m03 * A0212),
        det *  (m.m10 * A1323 - m.m11 * A0323 + m.m13 * A0123),
        det * -(m.m00 * A1323 - m.m01 * A0323 + m.m03 * A0123),
        det *  (m.m00 * A1313 - m.m01 * A0313 + m.m03 * A0113),
        det * -(m.m00 * A1312 - m.m01 * A0312 + m.m03 * A0112),
        det * -(m.m10 * A1223 - m.m11 * A0223 + m.m12 * A0123),
        det *  (m.m00 * A1223 - m.m01 * A0223 + m.m02 * A0123),
        det * -(m.m00 * A1213 - m.m01 * A0213 + m.m02 * A0113),
        det *  (m.m00 * A1212 - m.m01 * A0212 + m.m02 * A0112),
    };
}

m4f Identity()
{
    return
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
}

m4f ScaleMatrix(v3f scale)
{
    return
    {
        scale.x, 0.f, 0.f, 0.f,
        0.f, scale.y, 0.f, 0.f,
        0.f, 0.f, scale.z, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
};

m4f RotationMatrix(f32 angle, v3f axis)
{
    f32 angSin = sinf(angle); f32 angCos = cosf(angle); f32 invCos = 1.f - angCos;
    return
    {
        axis.x * axis.x * invCos + angCos,          axis.y * axis.x * invCos - axis.z * angSin, axis.z * axis.x * invCos + axis.y * angSin, 0.f,
        axis.x * axis.y * invCos + axis.z * angSin, axis.y * axis.y * invCos + angCos,          axis.z * axis.y * invCos - axis.x * angSin, 0.f,
        axis.x * axis.z * invCos - axis.y * angSin, axis.y * axis.z * invCos + axis.x * angSin, axis.z * axis.z * invCos + angCos,          0.f,
        0.f, 0.f, 0.f, 1.f,
    };
}

m4f TranslationMatrix(v3f move)
{
    return
    {
        1.f, 0.f, 0.f, move.x,
        0.f, 1.f, 0.f, move.y,
        0.f, 0.f, 1.f, move.z,
        0.f, 0.f, 0.f, 1.f,
    };
};

v3f TransformPosition(v3f position, m4f transform)
{
    v4f v = position.AsPosition();
    v = transform * v;
    return v.AsXYZ();
}

v3f TransformDirection(v3f direction, m4f transform)
{
    v4f v = direction.AsDirection();
    v = transform * v;
    return v.AsXYZ();
}

m4f LookAt(v3f center, v3f target, v3f up)
{
    v3f lookDir     = Normalize(center - target);
    v3f lookRight   = Normalize(Cross(up, lookDir));
    v3f lookUp      = Normalize(Cross(lookDir, lookRight));
    m4f lookRotation =
    {
        lookRight.x,    lookRight.y,    lookRight.z, 0,
        lookUp.x,       lookUp.y,       lookUp.z, 0,
        lookDir.x,      lookDir.y,      lookDir.z, 0,
        0, 0, 0, 1,
    };
    m4f lookTranslation = TranslationMatrix({-center.x, -center.y, -center.z});
    return lookRotation * lookTranslation;
}

m4f Perspective(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane)
{
    f32 top = tanf(fov / 2.f) * nearPlane;
    f32 bottom = -top;
    f32 right = top * aspect;
    f32 left = bottom * aspect;
    m4f result = {};
    result.m00 = (2.f * nearPlane) / (right - left);
    result.m11 = -(2.f * nearPlane) / (top - bottom);   // -1 to account for Vulkan coordinate system (this uses left hand, vk uses right hand)
    result.m32 = -(2.f * nearPlane * farPlane) / (farPlane - nearPlane);
    result.m20 = (right + left) / (right - left);
    result.m21 = (top + bottom) / (top - bottom);
    result.m22 = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result.m23 = -1;

    return Transpose(result);       // TODO(caio): Why do I transpose here? I already transpose when sending
                                    // to shaders...
}

f32 Lerp(f32 a, f32 b, f32 t)
{
    return a + (b - a) * CLAMP(t, 0, 1);
}

v2f Lerp(v2f a, v2f b, f32 t)
{
    return
    {
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t),
    };
}

v3f Lerp(v3f a, v3f b, f32 t)
{
    return
    {
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t),
        Lerp(a.z, b.z, t),
    };
}

u64 RandomU64()
{
    // Xorshift*64
    static u64 x = __rdtsc();   // TODO(caio)#THREAD: This shouldn't be static when doing multithreading!
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
}

f32 RandomUniformF32()
{
    return (f32)RandomU64()/(f32)MAX_U64;
}

f32 RandomUniformF32(f32 start, f32 end)
{
    return start + RandomUniformF32() * (end - start);
}

i32 RandomUniformI32(i32 start, i32 end)
{
    // Random int between start (inclusive) and end (inclusive)
    return start + RandomUniformF32() * (end + 1 - start);
}

};
};
