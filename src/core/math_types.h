#pragma once
/**
 * @file math_types.h
 * @brief Complete 3D linear algebra and spatial geometry library for Split/Second: Velocity.
 *
 * Designed to match the left-handed DirectX 9 coordinate system used by the original engine:
 *  - X: Right, Y: Up, Z: Forward
 */

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace SSV {
namespace Math {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float _x, float _y) : x(_x), y(_y) {}

    Vector2 operator+(const Vector2& o) const { return {x + o.x, y + o.y}; }
    Vector2 operator-(const Vector2& o) const { return {x - o.x, y - o.y}; }
    Vector2 operator*(float s) const { return {x * s, y * s}; }
    float Dot(const Vector2& o) const { return x * o.x + y * o.y; }
    float LengthSq() const { return x * x + y * y; }
    float Length() const { return std::sqrt(LengthSq()); }
};

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3& operator-=(const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }

    Vector3 Cross(const Vector3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }

    Vector3 Normalized() const {
        float len = Length();
        return (len > 1e-6f) ? (*this / len) : Vector3(0, 0, 0);
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    static float Distance(const Vector3& a, const Vector3& b) {
        return (b - a).Length();
    }
};

struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    Vector4() = default;
    Vector4(float _x, float _y, float _z, float _w = 1.0f) : x(_x), y(_y), z(_z), w(_w) {}
    Vector4(const Vector3& v, float _w = 1.0f) : x(v.x), y(v.y), z(v.z), w(_w) {}
};

struct Matrix4x4 {
    float m[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    static Matrix4x4 Identity() {
        return Matrix4x4();
    }

    static Matrix4x4 Translation(const Vector3& t) {
        Matrix4x4 r;
        r.m[3][0] = t.x;
        r.m[3][1] = t.y;
        r.m[3][2] = t.z;
        return r;
    }

    static Matrix4x4 Scaling(const Vector3& s) {
        Matrix4x4 r;
        r.m[0][0] = s.x;
        r.m[1][1] = s.y;
        r.m[2][2] = s.z;
        return r;
    }

    static Matrix4x4 RotationY(float radians) {
        Matrix4x4 r;
        float c = std::cos(radians);
        float s = std::sin(radians);
        r.m[0][0] = c;  r.m[0][2] = -s;
        r.m[2][0] = s;  r.m[2][2] = c;
        return r;
    }

    Matrix4x4 operator*(const Matrix4x4& o) const {
        Matrix4x4 r = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                r.m[i][j] = m[i][0] * o.m[0][j] +
                            m[i][1] * o.m[1][j] +
                            m[i][2] * o.m[2][j] +
                            m[i][3] * o.m[3][j];
            }
        }
        return r;
    }

    Vector3 TransformPoint(const Vector3& p) const {
        float x = p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + m[3][0];
        float y = p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + m[3][1];
        float z = p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + m[3][2];
        float w = p.x * m[0][3] + p.y * m[1][3] + p.z * m[2][3] + m[3][3];
        if (std::abs(w) > 1e-6f && std::abs(w - 1.0f) > 1e-6f) {
            x /= w; y /= w; z /= w;
        }
        return {x, y, z};
    }

    static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 zaxis = (target - eye).Normalized();
        Vector3 xaxis = up.Cross(zaxis).Normalized();
        Vector3 yaxis = zaxis.Cross(xaxis);

        Matrix4x4 r;
        r.m[0][0] = xaxis.x; r.m[0][1] = yaxis.x; r.m[0][2] = zaxis.x; r.m[0][3] = 0.0f;
        r.m[1][0] = xaxis.y; r.m[1][1] = yaxis.y; r.m[1][2] = zaxis.y; r.m[1][3] = 0.0f;
        r.m[2][0] = xaxis.z; r.m[2][1] = yaxis.z; r.m[2][2] = zaxis.z; r.m[2][3] = 0.0f;
        r.m[3][0] = -xaxis.Dot(eye);
        r.m[3][1] = -yaxis.Dot(eye);
        r.m[3][2] = -zaxis.Dot(eye);
        r.m[3][3] = 1.0f;
        return r;
    }

    static Matrix4x4 PerspectiveFovLH(float fovY, float aspect, float zNear, float zFar) {
        float yScale = 1.0f / std::tan(fovY * 0.5f);
        float xScale = yScale / aspect;
        Matrix4x4 r = {};
        r.m[0][0] = xScale;
        r.m[1][1] = yScale;
        r.m[2][2] = zFar / (zFar - zNear);
        r.m[2][3] = 1.0f;
        r.m[3][2] = (-zNear * zFar) / (zFar - zNear);
        return r;
    }
};

struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    Quaternion() = default;
    Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

    static Quaternion Identity() { return {0, 0, 0, 1}; }

    static Quaternion FromAxisAngle(const Vector3& axis, float angleRad) {
        float half = angleRad * 0.5f;
        float s = std::sin(half);
        Vector3 n = axis.Normalized();
        return {n.x * s, n.y * s, n.z * s, std::cos(half)};
    }

    Vector3 Rotate(const Vector3& v) const {
        Vector3 u(x, y, z);
        float s = w;
        return u * (2.0f * u.Dot(v)) +
               v * (s * s - u.Dot(u)) +
               u.Cross(v) * (2.0f * s);
    }
};

struct Ray {
    Vector3 origin;
    Vector3 direction;

    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.Normalized()) {}
    Vector3 GetPoint(float distance) const { return origin + direction * distance; }
};

struct AABB {
    Vector3 min;
    Vector3 max;

    AABB() : min(1e9f, 1e9f, 1e9f), max(-1e9f, -1e9f, -1e9f) {}
    AABB(const Vector3& _min, const Vector3& _max) : min(_min), max(_max) {}

    bool Contains(const Vector3& p) const {
        return (p.x >= min.x && p.x <= max.x &&
                p.y >= min.y && p.y <= max.y &&
                p.z >= min.z && p.z <= max.z);
    }

    bool IntersectsRay(const Ray& ray, float& outT) const {
        float tmin = (min.x - ray.origin.x) / (ray.direction.x != 0.0f ? ray.direction.x : 1e-6f);
        float tmax = (max.x - ray.origin.x) / (ray.direction.x != 0.0f ? ray.direction.x : 1e-6f);
        if (tmin > tmax) std::swap(tmin, tmax);

        float tymin = (min.y - ray.origin.y) / (ray.direction.y != 0.0f ? ray.direction.y : 1e-6f);
        float tymax = (max.y - ray.origin.y) / (ray.direction.y != 0.0f ? ray.direction.y : 1e-6f);
        if (tymin > tymax) std::swap(tymin, tymax);

        if ((tmin > tymax) || (tymin > tmax)) return false;
        if (tymin > tmin) tmin = tymin;
        if (tymax < tmax) tmax = tymax;

        float tzmin = (min.z - ray.origin.z) / (ray.direction.z != 0.0f ? ray.direction.z : 1e-6f);
        float tzmax = (max.z - ray.origin.z) / (ray.direction.z != 0.0f ? ray.direction.z : 1e-6f);
        if (tzmin > tzmax) std::swap(tzmin, tzmax);

        if ((tmin > tzmax) || (tzmin > tmax)) return false;
        if (tzmin > tmin) tmin = tzmin;
        if (tzmax < tmax) tmax = tzmax;

        outT = tmin;
        return outT >= 0.0f;
    }
};

} // namespace Math
} // namespace SSV
