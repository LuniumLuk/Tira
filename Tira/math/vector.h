//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef VECTOR_H
#define VECTOR_H

// A relatively simple implementation of 3D vector math.
// Vectors are column vectors.

// Note: do not include this file anywhere before utils.h or without utils.h

#ifdef ENABLE_SIMD
#include "simd.h"
#endif

#include <cmath>
#include <cassert>
#include <algorithm>
#include <ostream>

namespace tira {

    template<typename T>
    struct Vector2;
    template<typename T>
    struct Vector3;
    template<typename T>
    struct Vector4;

    template<typename T, typename U, typename V>
    inline T saturate(T x, U min, V max) {
        return std::min(std::max(x, min), max);
    }

    template<typename T>
    inline T clamp(T x, T min, T max) {
        x = x < min ? min : x;
        x = x > max ? max : x;
        return x;
    }

    template<typename T>
    struct Vector2 {
        using dataType = T;
        union { dataType x, u; };
        union { dataType y, v; };

        Vector2()
            : x(static_cast<T>(0))
            , y(static_cast<T>(0)) {}
        Vector2(T s)
            : x(s)
            , y(s) {}
        Vector2(T _x, T _y)
            : x(_x)
            , y(_y) {}
        Vector2(Vector3<T> const& v)
            : x(v.x)
            , y(v.y) {}

        T& operator[] (size_t i) {
            assert(i < 2);
            return (&x)[i];
        }

        T operator[] (size_t i) const {
            assert(i < 2);
            return (&x)[i];
        }

        Vector2 operator- () const {
            return Vector2{
                -x,
                -y,
            };
        }

        Vector2 operator+ (Vector2 const& other) const {
            return Vector2{
                x + other.x,
                y + other.y,
            };
        }
        Vector2 operator- (Vector2 const& other) const {
            return Vector2{
                x - other.x,
                y - other.y,
            };
        }
        Vector2 operator* (Vector2 const& other) const {
            return Vector2{
                x * other.x,
                y * other.y,
            };
        }
        Vector2 operator/ (Vector2 const& other) const {
            return Vector2{
                x / other.x,
                y / other.y,
            };
        }
        Vector2 operator* (T scaler) const {
            return Vector2{
                x * scaler,
                y * scaler,
            };
        }
        Vector2 operator/ (T scaler) const {
            return Vector2{
                x / scaler,
                y / scaler,
            };
        }

        Vector2& operator+= (Vector2 const& other) {
            x += other.x;
            y += other.y;

            return *this;
        }
        Vector2& operator-= (Vector2 const& other) {
            x -= other.x;
            y -= other.y;

            return *this;
        }

        static Vector2<T> min(Vector2<T> const& first, Vector2<T> const& second) {
            return Vector2<T>{
                std::min(first.x, second.x),
                    std::min(first.y, second.y),
            };
        }

        static Vector2<T> max(Vector2<T> const& first, Vector2<T> const& second) {
            return Vector2<T>{
                std::max(first.x, second.x),
                    std::max(first.y, second.y),
            };
        }

        T dot(Vector2 const& other) {
            return x * other.x + y * other.y;
        }

        T cross(Vector2 const& other) {
            return x * other.y - y * other.x;
        }

        Vector2 saturated() const {
            return Vector2{
                saturate(x, static_cast<T>(0), static_cast<T>(1)),
                saturate(y, static_cast<T>(0), static_cast<T>(1)),
            };
        }

        Vector2 clamped(T min, T max) const {
            return Vector2{
                clamp(x, static_cast<T>(min), static_cast<T>(max)),
                clamp(y, static_cast<T>(min), static_cast<T>(max)),
            };
        }

        T norm() const {
            return std::sqrt(x * x + y * y);
        }

        T norm2() const {
            return x * x + y * y;
        }

        T mean() const {
            return (x + y) / static_cast<T>(2);
        }

        T max_component() const {
            return std::max(x, y);
        }

        static Vector2 min() {
            return Vector2{ std::numeric_limits<T>::min() };
        }

        static Vector2 max() {
            return Vector2{ std::numeric_limits<T>::max() };
        }

        static Vector2 one() {
            return Vector2{ 1 };
        }

        static Vector2 zero() {
            return Vector2{ 0 };
        }
    };

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, Vector2<T> const& v) {
        os << v.x << ", " << v.y;
        return os;
    }

    template<typename T>
    struct Vector3 {
        using dataType = T;

#ifdef ENABLE_SIMD
        union {
            struct {
                union { dataType x, r; };
                union { dataType y, g; };
                union { dataType z, b; };
            };
            __m128 m;
        };

        Vector3(__m128 _m)
            : m(_m) {}
#else
        union { dataType x, r; };
        union { dataType y, g; };
        union { dataType z, b; };
#endif

        Vector3()
            : x(static_cast<T>(0))
            , y(static_cast<T>(0))
            , z(static_cast<T>(0)) {}
        Vector3(T s)
            : x(s)
            , y(s)
            , z(s) {}
        Vector3(T _x, T _y, T _z)
            : x(_x)
            , y(_y)
            , z(_z) {}
        Vector3(Vector4<T> const& v)
            : x(v.x)
            , y(v.y)
            , z(v.z) {}

        T& operator[] (size_t i) {
            assert(i < 3);
            return (&x)[i];
        }

        T operator[] (size_t i) const {
            assert(i < 3);
            return (&x)[i];
        }

        Vector3 operator- () const {
            return Vector3{
                -x,
                -y,
                -z,
            };
        }

        Vector3 operator+ (Vector3 const& other) const {
#ifdef ENABLE_SIMD
            return { _mm_add_ps(m, other.m) };
#else
            return Vector3{
                x + other.x,
                y + other.y,
                z + other.z,
            };
#endif
        }
        Vector3 operator- (Vector3 const& other) const {
#ifdef ENABLE_SIMD
            return { _mm_sub_ps(m, other.m) };
#else
            return Vector3{
                x - other.x,
                y - other.y,
                z - other.z,
            };
#endif
        }
        Vector3 operator* (Vector3 const& other) const {
#ifdef ENABLE_SIMD
            return { _mm_mul_ps(m, other.m) };
#else
            return Vector3{
                 x * other.x,
                 y * other.y,
                 z * other.z,
            };
#endif
        }
        Vector3 operator/ (Vector3 const& other) const {
#ifdef ENABLE_SIMD
            return { _mm_div_ps(m, other.m) };
#else
            return Vector3{
                x / other.x,
                y / other.y,
                z / other.z,
            };
#endif
        }
        Vector3 operator* (T scaler) const {
            return Vector3{
                x * scaler,
                y * scaler,
                z * scaler,
            };
        }
        Vector3 operator/ (T scaler) const {
            return Vector3{
                x / scaler,
                y / scaler,
                z / scaler,
            };
        }

        Vector3& operator+= (Vector3 const& other) {
            x += other.x;
            y += other.y;
            z += other.z;

            return *this;
        }
        Vector3& operator-= (Vector3 const& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;

            return *this;
        }

        static Vector3<T> min(Vector3<T> const& first, Vector3<T> const& second) {
            return Vector3<T>{
                std::min(first.x, second.x),
                    std::min(first.y, second.y),
                    std::min(first.z, second.z),
            };
        }

        static Vector3<T> max(Vector3<T> const& first, Vector3<T> const& second) {
            return Vector3<T>{
                std::max(first.x, second.x),
                    std::max(first.y, second.y),
                    std::max(first.z, second.z),
            };
        }

        T dot(Vector3 const& other) const {
#ifdef ENABLE_SIMD
            __m128 s, r;
            s = _mm_mul_ps(m, other.m);
            r = _mm_add_ss(s, _mm_movehl_ps(s, s));
            r = _mm_add_ss(r, _mm_shuffle_ps(r, r, 1));
            return _mm_cvtss_f32(r);
#else
            return x * other.x + y * other.y + z * other.z;
#endif
        }

        T mean() const {
            return (x + y + z) / static_cast<T>(3);
        }

        Vector3 cross(Vector3 const& other) const {
#ifdef ENABLE_SIMD
            __m128 a = m;
            __m128 b = other.m;
            __m128 ea, eb;
            ea = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
            eb = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
            __m128 xa = _mm_mul_ps(ea, eb);
            a = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
            b = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
            __m128 xb = _mm_mul_ps(a, b);
            return Vector3{ _mm_sub_ps(xa, xb) };
#else
            return Vector3{
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x,
            };
#endif
        }

        Vector3 normalized() const {
            T magnitude = (*this).dot(*this);
            if (magnitude == 0) return (*this);
            T f = 1.0f / std::sqrt(magnitude);
            return Vector3{
                x * f,
                y * f,
                z * f,
            };
        }

        Vector3 saturated() const {
            return Vector3{
                saturate(x, static_cast<T>(0), static_cast<T>(1)),
                saturate(y, static_cast<T>(0), static_cast<T>(1)),
                saturate(z, static_cast<T>(0), static_cast<T>(1)),
            };
        }

        Vector3 clamped(T min, T max) const {
            return Vector3{
                clamp(x, static_cast<T>(min), static_cast<T>(max)),
                clamp(y, static_cast<T>(min), static_cast<T>(max)),
                clamp(z, static_cast<T>(min), static_cast<T>(max)),
            };
        }

        T norm() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        T norm2() const {
            return x * x + y * y + z * z;
        }

        T max_component() const {
            return std::max(std::max(x, y), z);
        }

        static Vector3 min() {
            return Vector3{ std::numeric_limits<T>::min() };
        }

        static Vector3 max() {
            return Vector3{ std::numeric_limits<T>::max() };
        }

        static Vector3 one() {
            return Vector3{ 1 };
        }

        static Vector3 zero() {
            return Vector3{ 0 };
        }
    };

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, Vector3<T> const& v) {
        os << v.x << ", " << v.y << ", " << v.z;
        return os;
    }

    template<typename T>
    struct Vector4 {
        using dataType = T;
        union { dataType x, r; };
        union { dataType y, g; };
        union { dataType z, b; };
        union { dataType w, a; };

        Vector4()
            : x(static_cast<T>(0))
            , y(static_cast<T>(0))
            , z(static_cast<T>(0))
            , w(static_cast<T>(0)) {}
        Vector4(T s)
            : x(s)
            , y(s)
            , z(s)
            , w(s) {}
        Vector4(T _x, T _y, T _z, T _w)
            : x(_x)
            , y(_y)
            , z(_z)
            , w(_w) {}
        Vector4(Vector3<T> const& v, T _w)
            : x(v.x)
            , y(v.y)
            , z(v.z)
            , w(_w) {}

        Vector4(Vector4 const& other)
            : x(other.x)
            , y(other.y)
            , z(other.z)
            , w(other.w) {}

        Vector4& operator=(Vector4 const& other) {
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
            return (*this);
        }

        T& operator[] (size_t i) {
            assert(i < 4);
            return (&x)[i];
        }

        T operator[] (size_t i) const {
            assert(i < 4);
            return (&x)[i];
        }

        Vector4 operator- () const {
            return Vector4{
                -x,
                -y,
                -z,
                -w,
            };
        }

        Vector4 operator+ (Vector4 const& other) const {
            return Vector4{
                x + other.x,
                y + other.y,
                z + other.z,
                w + other.w,
            };
        }
        Vector4 operator- (Vector4 const& other) const {
            return Vector4{
                x - other.x,
                y - other.y,
                z - other.z,
                w - other.w,
            };
        }
        Vector4 operator* (Vector4 const& other) const {
            return Vector4{
                x * other.x,
                y * other.y,
                z * other.z,
                w * other.w,
            };
        }
        Vector4 operator/ (Vector4 const& other) const {
            return Vector4{
                x / other.x,
                y / other.y,
                z / other.z,
                w / other.w,
            };
        }
        Vector4 operator* (T scaler) const {
            return Vector4{
                x * scaler,
                y * scaler,
                z * scaler,
                w * scaler,
            };
        }
        Vector4 operator/ (T scaler) const {
            return Vector4{
                x / scaler,
                y / scaler,
                z / scaler,
                w / scaler,
            };
        }

        Vector4& operator+= (Vector4 const& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;

            return *this;
        }
        Vector4& operator-= (Vector4 const& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;

            return *this;
        }

        static Vector4<T> min(Vector4<T> const& first, Vector4<T> const& second) {
            return Vector4<T>{
                std::min(first.x, second.x),
                    std::min(first.y, second.y),
                    std::min(first.z, second.z),
                    std::min(first.w, second.w),
            };
        }

        static Vector4<T> max(Vector4<T> const& first, Vector4<T> const& second) {
            return Vector4<T>{
                std::max(first.x, second.x),
                    std::max(first.y, second.y),
                    std::max(first.z, second.z),
                    std::max(first.w, second.w),
            };
        }

        T dot(Vector4 const& other) {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        Vector4 saturated() const {
            return Vector4{
                saturate(x, static_cast<T>(0), static_cast<T>(1)),
                saturate(y, static_cast<T>(0), static_cast<T>(1)),
                saturate(z, static_cast<T>(0), static_cast<T>(1)),
                saturate(w, static_cast<T>(0), static_cast<T>(1)),
            };
        }

        Vector4 clamped(T min, T max) const {
            return Vector4{
                clamp(x, static_cast<T>(min), static_cast<T>(max)),
                clamp(y, static_cast<T>(min), static_cast<T>(max)),
                clamp(z, static_cast<T>(min), static_cast<T>(max)),
                clamp(w, static_cast<T>(min), static_cast<T>(max)),
            };
        }

        T norm() const {
            return std::sqrt(x * x + y * y + z * z + w * w);
        }

        T norm2() const {
            return x * x + y * y + z * z + w * w;
        }

        T mean() const {
            return (x + y + z + w) / static_cast<T>(4);
        }

        T max_component() const {
            return std::max(std::max(x, y), std::max(z, w));
        }

        static Vector4 min() {
            return Vector4{ std::numeric_limits<T>::min() };
        }

        static Vector4 max() {
            return Vector4{ std::numeric_limits<T>::max() };
        }

        static Vector4 one() {
            return Vector4{ 1 };
        }

        static Vector4 zero() {
            return Vector4{ 0 };
        }
    };

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, Vector4<T> const& v) {
        os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
        return os;
    }

    using float2 = Vector2<float>;
    using int2 = Vector2<int32_t>;
    using float3 = Vector3<float>;
    using colorf = Vector3<float>;
    using int3 = Vector3<int32_t>;
    using float4 = Vector4<float>;
    using int4 = Vector4<int32_t>;

    // For compatibility

    template<typename T>
    inline Vector3<T> cross(Vector3<T> const& x, Vector3<T> const& y) {
        return x.cross(y);
    }

    template<typename T>
    inline T dot(Vector3<T> const& x, Vector3<T> const& y) {
        return x.dot(y);
    }

    template<typename T>
    inline Vector3<T> normalize(Vector3<T> const& x) {
        return x.normalized();
    }

    template<typename T>
    inline T length(Vector3<T> const& x) {
        return x.norm();
    }

    template<typename T>
    inline Vector2<T> saturate(Vector2<T> const& x) {
        return x.saturated();
    }

    template<typename T>
    inline Vector3<T> saturate(Vector3<T> const& x) {
        return x.saturated();
    }

    template<typename T>
    inline Vector4<T> saturate(Vector4<T> const& x) {
        return x.saturated();
    }

    template<typename T>
    inline Vector2<T> clamp(Vector2<T> const& x, T min, T max) {
        return x.clamped(min, max);
    }

    template<typename T>
    inline Vector3<T> clamp(Vector3<T> const& x, T min, T max) {
        return x.clamped(min, max);
    }

    template<typename T>
    inline Vector4<T> clamp(Vector4<T> const& x, T min, T max) {
        return x.clamped(min, max);
    }

    namespace transform {

        inline float3 reflect(float3 const& vec, float3 const& N) {
            return (vec - N * vec.dot(N) * 2).normalized();
        }

        inline float3 refract(float3 const& vec, float3 const& N, float eta) {
            float cos_theta = dot(-vec, N);
            float3 r_out_perp = (vec + N * cos_theta) * eta;
            float3 r_out_parallel = N * -std::sqrt(1 - dot(r_out_perp, r_out_perp));
            return r_out_perp + r_out_parallel;
        }

        inline bool refract(float3 const& vec, float3 const& N, float eta, float3& refracted) {
            auto dt = vec.dot(N);
            auto discriminant = 1 - eta * eta * (1 - dt * dt);
            if (discriminant > 0) {
                refracted = ((vec - N * dt) * eta - N * std::sqrt(discriminant)).normalized();
                return true;
            }
            else {
                return false;
            }
        }

    } // namespace transform

} // namespace tira

#endif
