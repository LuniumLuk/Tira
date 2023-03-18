//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef MATRIX_H
#define MATRIX_H

// A relatively simple implementation of 3D matrix math.
// Matrix is implemented as column major.

// Note: do not include this file anywhere before utils.h or without utils.h

#include <math/vector.h>
#include <cassert>
#include <cmath>

namespace tira {

    template<typename T>
    struct Matrix3 {
        using dataType = T;
        Vector3<T> col[3];

        Matrix3() = default;
        Matrix3(
            T m00, T m01, T m02,
            T m10, T m11, T m12,
            T m20, T m21, T m22)
            : col{ {m00, m10, m20},
                  {m01, m11, m21},
                  {m02, m12, m22} } {}

        Matrix3(
            Vector3<T> const& c0,
            Vector3<T> const& c1,
            Vector3<T> const& c2)
            : col{ c0, c1, c2 } {}

        Matrix3(Matrix3 const& other)
            : col{ other.col[0], other.col[1], other.col[2] } {}

        Matrix3& operator=(Matrix3 const& other) {
            col[0] = other.col[0];
            col[1] = other.col[1];
            col[2] = other.col[2];
            return (*this);
        }

        Vector3<T>& operator[] (size_t c) {
            return col[c];
        }

        Vector3<T> const operator[] (size_t c) const {
            return col[c];
        }

        Vector3<T> row(size_t r) const {
            assert(r < 3);
            return Vector3<T>{
                col[0][r],
                    col[1][r],
                    col[2][r],
            };
        }

        T det() const {
            return col[0][0] * (col[1][1] * col[2][2] - col[2][1] * col[1][2])
                - col[1][0] * (col[0][1] * col[2][2] - col[2][1] * col[0][2])
                + col[2][0] * (col[0][1] * col[1][2] - col[1][1] * col[0][2]);
        }

        Matrix3 inversed() const {
            float d = this->det();
            assert(d);

            float f = 1 / d;
            float a1 = col[1][1] * col[2][2] - col[2][1] * col[1][2];
            float a2 = col[0][1] * col[2][2] - col[2][1] * col[0][2];
            float a3 = col[0][1] * col[1][2] - col[1][1] * col[0][2];
            float a4 = col[1][0] * col[2][2] - col[2][0] * col[1][2];
            float a5 = col[0][0] * col[2][2] - col[2][0] * col[0][2];
            float a6 = col[0][0] * col[1][2] - col[1][0] * col[0][2];
            float a7 = col[1][0] * col[2][1] - col[2][0] * col[1][1];
            float a8 = col[0][0] * col[2][1] - col[2][0] * col[0][1];
            float a9 = col[0][0] * col[1][1] - col[1][0] * col[0][1];

            return Matrix3{
                 a1 * f, -a4 * f,  a7 * f,
                -a2 * f,  a5 * f, -a8 * f,
                 a3 * f, -a6 * f,  a9 * f
            };
        }

        static Matrix3 identity() {
            return Matrix3{
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
            };
        }
    };

    template<typename T>
    Matrix3<T> operator*(Matrix3<T> const& first, Matrix3<T> const& second) {
        Matrix3<T> m;
        m.col[0] = first * second.col[0];
        m.col[1] = first * second.col[1];
        m.col[2] = first * second.col[2];
        return m;
    }

    template<typename T>
    Vector3<T> operator*(Matrix3<T> const& first, Vector3<T> const& second) {
        return Vector3<T>{
            first.row(0).dot(second),
                first.row(1).dot(second),
                first.row(2).dot(second),
        };
    }

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, Matrix3<T> const& m) {
        os << m[0][0] << ", " << m[0][1] << ", " << m[0][2] << "\n"
            << m[1][0] << ", " << m[1][1] << ", " << m[1][2] << "\n"
            << m[2][0] << ", " << m[2][1] << ", " << m[2][2];
        return os;
    }

    template<typename T>
    struct Matrix4 {
        using dataType = T;
        Vector4<T> col[4];

        Matrix4() = default;
        Matrix4(
            T m00, T m01, T m02, T m03,
            T m10, T m11, T m12, T m13,
            T m20, T m21, T m22, T m23,
            T m30, T m31, T m32, T m33)
            : col{ {m00, m10, m20, m30},
                  {m01, m11, m21, m31},
                  {m02, m12, m22, m32},
                  {m03, m13, m23, m33} } {}

        Matrix4(
            Vector4<T> const& c0,
            Vector4<T> const& c1,
            Vector4<T> const& c2,
            Vector4<T> const& c3)
            : col{ c0, c1, c2, c3 } {}

        Matrix4(Matrix4 const& other)
            : col{ other.col[0], other.col[1], other.col[2], other.col[3] } {}

        Matrix4& operator=(Matrix4 const& other) {
            col[0] = other.col[0];
            col[1] = other.col[1];
            col[2] = other.col[2];
            col[3] = other.col[3];
            return (*this);
        }

        Vector4<T>& operator[] (size_t c) {
            return col[c];
        }

        Vector4<T> const operator[] (size_t c) const {
            return col[c];
        }

        Vector4<T> row(size_t r) const {
            assert(r < 4);
            return Vector4<T>{
                col[0][r],
                    col[1][r],
                    col[2][r],
                    col[3][r],
            };
        }

        static Matrix4 identity() {
            return Matrix4{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
        }
    };

    template<typename T>
    Matrix4<T> operator*(Matrix4<T> const& first, Matrix4<T> const& second) {
        Matrix4<T> m;
        m.col[0] = first * second.col[0];
        m.col[1] = first * second.col[1];
        m.col[2] = first * second.col[2];
        m.col[3] = first * second.col[3];
        return m;
    }

    template<typename T>
    Vector4<T> operator*(Matrix4<T> const& first, Vector4<T> const& second) {
        return Vector4<T>{
            first.row(0).dot(second),
                first.row(1).dot(second),
                first.row(2).dot(second),
                first.row(3).dot(second),
        };
    }

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, Matrix4<T> const& m) {
        os << m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", " << m[0][3] << "\n"
            << m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", " << m[1][3] << "\n"
            << m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", " << m[2][3] << "\n"
            << m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", " << m[3][3];
        return os;
    }

    using float3x3 = Matrix3<float>;
    using int3x3 = Matrix3<int>;
    using float4x4 = Matrix4<float>;
    using int4x4 = Matrix4<int>;

    namespace transform {

        inline float4x4 perspective(float fov = 1.570796325f, float aspect = 1.0, float near = 0.1, float far = 100.0) {
            float4x4 m;
            m[0][0] = 1 / (aspect * std::tan(fov / 2));
            m[1][1] = 1 / (aspect * std::tan(fov / 2));
            m[2][2] = -(far + near) / (far - near);
            m[3][2] = -(2 * far * near) / (far - near);
            m[2][3] = -1;
            return m;
        }

        inline float4x4 ortho(float left, float right, float bottom, float top, float near, float far) {
            float4x4 m;
            m[0][0] = 2 / (right - left);
            m[1][1] = 2 / (top - bottom);
            m[2][2] = -2 / (far - near);
            m[3][0] = -(right + left) / (right - left);
            m[3][1] = -(top + bottom) / (top - bottom);
            m[3][2] = -(far + near) / (far - near);
            m[3][3] = 1;
            return m;
        }

        inline float4x4 translate(float x, float y, float z) {
            float4x4 m;
            m[0][0] = 1;
            m[1][1] = 1;
            m[2][2] = 1;
            m[3][3] = 1;
            m[3][0] = x;
            m[3][1] = y;
            m[3][2] = z;
            return m;
        }

        inline float4x4 rotateX(float angle) {
            float sin = std::sin(angle);
            float cos = std::cos(angle);
            float4x4 m;
            m[0][0] = 1;
            m[1][1] = cos;
            m[2][1] = sin;
            m[1][2] = -sin;
            m[2][2] = cos;
            m[3][3] = 1;
            return m;
        }

        inline float4x4 rotateY(float angle) {
            float sin = std::sin(angle);
            float cos = std::cos(angle);
            float4x4 m;
            m[0][0] = cos;
            m[2][0] = sin;
            m[1][1] = 1;
            m[0][2] = -sin;
            m[2][2] = cos;
            m[3][3] = 1;
            return m;
        }

        inline float4x4 rotate(float3 const& axis, float angle) {
            float sin = std::sin(angle);
            float cos = std::cos(angle);
            float xx = axis.x * axis.x;
            float xy = axis.x * axis.y;
            float xz = axis.x * axis.z;
            float yy = axis.y * axis.y;
            float yz = axis.y * axis.z;
            float zz = axis.z * axis.z;

            float4x4 m = {
                cos + xx * (1.0f - cos), xy * (1.0f - cos) - axis.z * sin, xz * (1.0f - cos) + axis.y * sin, 0.0f,
                xy * (1.0f - cos) + axis.z * sin, cos + yy * (1.0f - cos), yz * (1.0f - cos) - axis.x * sin, 0.0f,
                xz * (1.0f - cos) - axis.y * sin, yz * (1.0f - cos) + axis.x * sin, cos + zz * (1.0f - cos), 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            return m;
        }

        inline float4x4 lookAt(float3 const& eye, float3 const& at, float3 const& up) {
            float3 forward = (eye - at).normalized();
            float3 left = up.cross(forward).normalized();
            float3 upward = forward.cross(left);
            float4x4 m(
                left.x, left.y, left.z, -(left.x * eye.x + left.y * eye.y + left.z * eye.z),
                upward.x, upward.y, upward.z, -(upward.x * eye.x + upward.y * eye.y + upward.z * eye.z),
                forward.x, forward.y, forward.z, -(forward.x * eye.x + forward.y * eye.y + forward.z * eye.z),
                0.0f, 0.0f, 0.0f, 1.0f
            );
            return m;
        }

    } // namespace transform

} // namespace tira

#endif
