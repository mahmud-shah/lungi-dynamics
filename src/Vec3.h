#pragma once

#include <cmath>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    constexpr Vec3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

inline Vec3 normalized(const Vec3& v)
{
    const float len = length(v);
    return len > 1e-8f ? v / len : Vec3(0.0f, 1.0f, 0.0f);
}

inline Vec3 mul(const Vec3& a, const Vec3& b) { return Vec3(a.x * b.x, a.y * b.y, a.z * b.z); }

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline bool isFinite(const Vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
