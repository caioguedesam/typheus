#include "./math.hpp"

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

v2f operator-(v2f a)
{
    return
    {
        -a.x,
        -a.y,
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

v3f operator-(v3f a)
{
    return
    {
        -a.x,
        -a.y,
        -a.z,
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

v3f v4f::AsXYZ() { return { x, y, z }; }

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

quat GetQuat(f32 angle, v3f axis)
{
    f32 s = sin(angle / 2.f);
    f32 c = cos(angle / 2.f);
    quat q =
    {
        s * axis.x,
        s * axis.y,
        s * axis.z,
        c
    };
    return math::Normalize(q);
}

quat GetInvQuat(quat q)
{
    return
    {
        -q.x, -q.y, -q.z, q.w
    };
}

quat QuatMul(quat a, quat b)
{
    return
    {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,  // i
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,  // j
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,  // k
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z   // 1
    };
}

v3f Rotate(v3f p, quat q)
{
    quat pure = {p.x, p.y, p.z, 0};
    quat result = QuatMul(QuatMul(q, pure), GetInvQuat(q));
    return result.AsXYZ();
}

v3f Rotate(v3f p, f32 angle, v3f axis)
{
    return Rotate(p, GetQuat(angle, axis));
}

plane GetPlane(v3f point, v3f normal)
{
    plane P;
    normal = math::Normalize(normal);
    P.x = normal.x;
    P.y = normal.y;
    P.z = normal.z;
    P.w = -(math::Dot(normal, point));
    return P;
}

plane GetPlane(v3f p0, v3f A, v3f B)
{
    A = math::Normalize(A);
    B = math::Normalize(B);
    v3f N = math::Normalize(math::Cross(A, B));
    return GetPlane(p0, N);
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

m4f RotationMatrix(quat q)
{
    // https://www.songho.ca/opengl/gl_quaternion.html
    m4f result = math::Identity();
    result.m00 = 1 - (2 * q.y * q.y) - (2 * q.z * q.z);
    result.m01 = (2 * q.x * q.y) - (2 * q.w * q.z);
    result.m02 = (2 * q.x * q.z) + (2 * q.w * q.y);
    result.m10 = (2 * q.x * q.y) + (2 * q.w * q.z);
    result.m11 = 1 - (2 * q.x * q.x) - (2 * q.z * q.z);
    result.m12 = (2 * q.y * q.z) - (2 * q.w * q.x);
    result.m20 = (2 * q.x * q.z) - (2 * q.w * q.y);
    result.m21 = (2 * q.y * q.z) + (2 * q.w * q.x);
    result.m22 = 1 - (2 * q.x * q.x) - (2 * q.y * q.y);

    return result;
}

m4f RotationMatrix(f32 angle, v3f axis)
{
    quat q = GetQuat(angle, axis);
    return RotationMatrix(q);
    //f32 angSin = sinf(angle); f32 angCos = cosf(angle); f32 invCos = 1.f - angCos;
    //return
    //{
    //    axis.x * axis.x * invCos + angCos,          axis.y * axis.x * invCos - axis.z * angSin, axis.z * axis.x * invCos + axis.y * angSin, 0.f,
    //    axis.x * axis.y * invCos + axis.z * angSin, axis.y * axis.y * invCos + angCos,          axis.z * axis.y * invCos - axis.x * angSin, 0.f,
    //    axis.x * axis.z * invCos - axis.y * angSin, axis.y * axis.z * invCos + axis.x * angSin, axis.z * axis.z * invCos + angCos,          0.f,
    //    0.f, 0.f, 0.f, 1.f,
    //};
}

void GetAngleAxis(m4f rotation, f32* angle, v3f* axis)
{
    //  Note: rotation matrix must be a pure rotation.
    //  https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToAngle/derivation/index.htm
    // Edge cases:
    f32 e = 0.001f;
    if(rotation.m01 - rotation.m10 < e
       && rotation.m02 - rotation.m20 < e
       && rotation.m12 - rotation.m21 < e)
    {
        // Edge case angle = 0
        *angle = 0;
        *axis = {0,0,1};
        return;

        // Edge case angle = 180 (TODO(caio))
    }

    // No edge cases
    *angle = acosf((rotation.m00 + rotation.m11 + rotation.m22 - 1) / 2.f);
    f32 term1 = rotation.m21 - rotation.m12;
    f32 term2 = rotation.m02 - rotation.m20;
    f32 term3 = rotation.m10 - rotation.m01;
    f32 denom = sqrtf(term1 * term1 + term2 * term2 + term3 * term3);
    axis->x = term1 / denom;
    axis->y = term2 / denom;
    axis->z = term3 / denom;
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

m4f ViewRH(v3f axisX, v3f axisY, v3f axisZ, v3f position)
{
    //https://learnopengl.com/Getting-started/Camera
    // View matrix is inverse of composed camera coordinate space matrix and translation matrix, set directly here.
    m4f result = math::Identity();
    result.m00 = axisX.x;
    result.m01 = axisX.y;
    result.m02 = axisX.z;
    result.m10 = axisY.x;
    result.m11 = axisY.y;
    result.m12 = axisY.z;
    result.m20 = axisZ.x;
    result.m21 = axisZ.y;
    result.m22 = axisZ.z;
    result.m03 = -Dot(axisX, position);
    result.m13 = -Dot(axisY, position);
    result.m23 = -Dot(axisZ, position);
    return result;
}

m4f PerspectiveRH(f32 fov, f32 aspect, f32 zNear, f32 zFar)
{
    // https://www.vincentparizet.com/blog/posts/vulkan_perspective_matrix/
    f32 y = 1.f / tan(fov/2.f);
    f32 x = y / aspect;
    m4f result = {};
    result.m00 = x;
    result.m11 = -y;
    result.m22 = zFar / (zNear - zFar);
    result.m23 = (zNear * zFar) / (zNear - zFar);
    result.m32 = -1;
    
    return result;
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

v3f RandomUniformV3F()
{
    return
    {
        RandomUniformF32(),
        RandomUniformF32(),
        RandomUniformF32()
    };
}

v3f RandomUniformV3F(f32 start, f32 end)
{
    return
    {
        RandomUniformF32(start, end),
        RandomUniformF32(start, end),
        RandomUniformF32(start, end)
    };
}

AABB TransformAABB(AABB aabb, m4f transform)
{
    // TODO(caio): This performs a lot of calculations that could be optimized to less
    // transforming, and no idea if this works for non uniform scaling.
    // Verify if this works

    // Compute all 8 vertices of AABB
    v4f aabbVertices[8] =
    {
        {aabb.min.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.max.z, 1.f},
    };

    // Transform all vertices
    v4f aabbTransformedVertices[8] =
    {
        transform * aabbVertices[0],
        transform * aabbVertices[1],
        transform * aabbVertices[2],
        transform * aabbVertices[3],
        transform * aabbVertices[4],
        transform * aabbVertices[5],
        transform * aabbVertices[6],
        transform * aabbVertices[7],
    };

    // Find min/max of new vertices and create new AABB
    AABB result = {};
    result.min = aabbTransformedVertices[0].AsXYZ();
    result.max = aabbTransformedVertices[0].AsXYZ();
    for(u32 i = 1; i < 8; i++)
    {
        result.min.x = MIN(result.min.x, aabbTransformedVertices[i].x);
        result.min.y = MIN(result.min.y, aabbTransformedVertices[i].y);
        result.min.z = MIN(result.min.z, aabbTransformedVertices[i].z);

        result.max.x = MAX(result.max.x, aabbTransformedVertices[i].x);
        result.max.y = MAX(result.max.y, aabbTransformedVertices[i].y);
        result.max.z = MAX(result.max.z, aabbTransformedVertices[i].z);
    }
    return result;
}

v3f GetAABBCenter(AABB aabb)
{
    return aabb.min + 0.5f * GetAABBSize(aabb);
}

v3f GetAABBSize(AABB aabb)
{
    return (aabb.max - aabb.min);
}

v3f ClipToWorldSpace(v3f p, m4f invView, m4f invProj)
{
    v4f P = {p.x, p.y, p.z, 1};
    // To view space (multiply by inverse projection then revert perspective divide).
    P = invProj * P;
    P = P * (1.f / P.w);
    // To world space (multiply by inverse view).
    P = invView * P;
    return P.AsXYZ();
}

Frustum GetFrustum(m4f view, m4f proj)
{
    // Derives frustum points in world space out of view and projection matrices.
    Frustum result = {};
    for(i32 i = 0; i < 8; i++)
    {
        result.points[i] = {0,0,0};
    }

    m4f invProj = math::Inverse(proj);
    m4f invView = math::Inverse(view);

    result.points[0] = ClipToWorldSpace({-1, -1, 0}, invView, invProj);
    result.points[1] = ClipToWorldSpace({ 1, -1, 0}, invView, invProj);
    result.points[2] = ClipToWorldSpace({ 1,  1, 0}, invView, invProj);
    result.points[3] = ClipToWorldSpace({-1,  1, 0}, invView, invProj);
    result.points[4] = ClipToWorldSpace({-1, -1, 1}, invView, invProj);
    result.points[5] = ClipToWorldSpace({ 1, -1, 1}, invView, invProj);
    result.points[6] = ClipToWorldSpace({ 1,  1, 1}, invView, invProj);
    result.points[7] = ClipToWorldSpace({-1,  1, 1}, invView, invProj);

    // Deriving frustum planes out of frustum points
    // Left (F_03 x F_04)
    result.planes[0] = GetPlane(result.points[0], result.points[3] - result.points[0], result.points[4] - result.points[0]);
    // Right (F_51 x F_21)
    result.planes[1] = GetPlane(result.points[1], result.points[1] - result.points[5], result.points[1] - result.points[2]);
    // Bottom (F_26 x F_27)
    result.planes[2] = GetPlane(result.points[2], result.points[6] - result.points[2], result.points[7] - result.points[2]);
    // Top (F_14 x F15)
    result.planes[3] = GetPlane(result.points[1], result.points[4] - result.points[1], result.points[5] - result.points[1]);
    // Near (F_01 x F_03)
    result.planes[4] = GetPlane(result.points[0], result.points[1] - result.points[0], result.points[3] - result.points[0]);
    // Far (F_47 x F_45)
    result.planes[5] = GetPlane(result.points[4], result.points[7] - result.points[4], result.points[5] - result.points[4]);

    return result;
}

bool IsInFrustum(v3f p, Frustum f)
{
    // Point is in frustum if it's inside half-space of all planes (using SDF).
    for(i32 i = 0; i < 6; i++)
    {
        f32 sdf = math::Dot(f.planes[i].AsXYZ(), p) + f.planes[i].w;
        if(sdf < 0.f)
        {
            return false;
        }
    }
    return true;
}

bool IsInFrustum(AABB aabb, Frustum f)
{
    // AABB is in frustum if, for each plane, the point furthest along the plane's normal
    // is inside its half-space.
    for(i32 i = 0; i < 6; i++)
    {
        v3f p;
        p.x = f.planes[i].x < 0.f ? aabb.min.x : aabb.max.x;
        p.y = f.planes[i].y < 0.f ? aabb.min.y : aabb.max.y;
        p.z = f.planes[i].z < 0.f ? aabb.min.z : aabb.max.z;
        f32 sdf = math::Dot(f.planes[i].AsXYZ(), p) + f.planes[i].w;
        if(sdf < 0.f)
        {
            return false;
        }
    }
    return true;
}

};
};
