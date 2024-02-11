#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdlib.h>

namespace cglm {

    constexpr float PI_M = 3.14159265358979323846f;

    static float to_radians(float degrees) { return degrees * PI_M / 180.0f; }

    template <size_t N, typename T> struct Vec {
        std::array<T, N> data;

        // Constructors
        Vec() = default;

        explicit Vec(T e0) {
            for (size_t i = 0; i < N; i++) data[i] = e0;
        }

        Vec(const std::initializer_list<T>& list) {
            size_t i = 0;
            for (const auto& e : list) {
                data[i] = e;
                if (++i == N) break;
            }
        }


        T operator[](size_t i) const { return data[i]; }
        T& operator[](size_t i) { return data[i]; }
    };

    /// function dot
    template<size_t N, typename T>
    inline T dot(const Vec<N, T>& v1, const Vec<N, T>& v2) {
        T t(0);
        for (size_t i = 0; i < N; ++i) {
            t += v1[i] * v2[i];
        }
        return t;
    }

    /// Return the geometric squared length of the vector
    template <size_t N, typename T> T length2(const Vec<N, T>& v) { return dot(v, v); }
    /// Return the geometric length of the vector
    template <size_t N, typename T> T length(const Vec<N, T>& v) { return std::sqrt(length2(v)); }
    /// Return a unit-length copy of the vector
    template <size_t N, typename T> Vec<N, T> normalize(const Vec<N, T>& v) { return v / length(v); }
    /// component-wise vector-vector assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator+=(Vec<N, T>& v1, const Vec<N, S>& v2) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] += v2[i];
        }
        return v1;
    }
    /// component-wise vector-vector assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator-=(Vec<N, T>& v1, const Vec<N, S>& v2) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] -= v2[i];
        }
        return v1;
    }
    /// component-wise vector-vector assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator*=(Vec<N, T>& v1, const Vec<N, S>& v2) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] *= v2[i];
        }
        return v1;
    }
    /// component-wise vector-vector assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator/=(Vec<N, T>& v1, const Vec<N, S>& v2) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] /= v2[i];
        }
        return v1;
    }

    /// component-wise vector-scalar assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator+=(Vec<N, T>& v1, S s) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] += s;
        }
        return v1;
    }
    /// component-wise vector-scalar assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator-=(Vec<N, T>& v1, S s) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] -= s;
        }
        return v1;
    }
    /// component-wise vector-scalar assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator*=(Vec<N, T>& v1, S s) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] *= s;
        }
        return v1;
    }
    /// component-wise vector-scalar assignment-arithmetic
    template <size_t N, typename T, typename S> Vec<N, T>& operator/=(Vec<N, T>& v1, S s) {
        for (size_t i = 0; i < N; ++i) {
            v1[i] /= s;
        }
        return v1;
    }

    // positive and negative
    template <size_t N, typename T> inline const Vec<N, T>& operator+(const Vec<N, T>& v) { return v; }

    template <size_t N, typename T> Vec<N, T> operator-(const Vec<N, T>& v) {
        Vec<N, T> v2;
        for (size_t i = 0; i < N; ++i) {
            v2[i] = -v[i];
        }
        return v2;
    }

    /// component-wise vector-vector addition
    template <size_t N, typename T> Vec<N, T> operator+(const Vec<N, T>& v1, const Vec<N, T>& v2) {
        Vec<N, T> v3;
        for (size_t i = 0; i < N; ++i) {
            v3[i] = v1[i] + v2[i];
        }
        return v3;
    }
    /// component-wise vector-vector subtraction
    template <size_t N, typename T> Vec<N, T> operator-(const Vec<N, T>& v1, const Vec<N, T>& v2) {
        Vec<N, T> v3;
        for (size_t i = 0; i < N; ++i) {
            v3[i] = v1[i] - v2[i];
        }
        return v3;
    }
    /// component-wise vector-vector multiplication
    template <size_t N, typename T> Vec<N, T> operator*(const Vec<N, T>& v1, const Vec<N, T>& v2) {
        Vec<N, T> v3;
        for (size_t i = 0; i < N; ++i) {
            v3[i] = v1[i] * v2[i];
        }
        return v3;
    }
    /// component-wise vector-vector division
    template <size_t N, typename T> Vec<N, T> operator/(const Vec<N, T>& v1, const Vec<N, T>& v2) {
        Vec<N, T> v3;
        for (size_t i = 0; i < N; ++i) {
            v3[i] = v1[i] / v2[i];
        }
        return v3;
    }
    /// component-wise vector-scalar division 
    template <size_t N, typename T, typename S> Vec<N, T> operator/(const Vec<N, T>& v, S s) {
        Vec<N, T> v2;
        for (size_t i = 0; i < N; ++i) {
            v2[i] = v[i] / s;
        }
        return v2;
    }
    /// component-wise vector-scalar multiplication
    template <size_t N, typename T, typename S> Vec<N, T> operator*(const Vec<N, T>& v, S s) {
        Vec<N, T> v2;
        for (size_t i = 0; i < N; ++i) {
            v2[i] = v[i] * s;
        }
        return v2;
    }


    /// A mathematical 3-vector
    template <typename T> struct Vec<3, T> {
        union {
            std::array<T, 3> e;
            struct {
                T x, y, z;
            };
            struct {
                T r, g, b;
            };
            Vec<2, T> xy;
        };

        constexpr Vec() = default;
        constexpr explicit Vec(T e0) : x(e0), y(e0), z(e0) {}
        constexpr Vec(T e0, T e1, T e2) : x(e0), y(e1), z(e2) {}
        constexpr Vec(const Vec<2, T> xy, T _z) : x(xy.x), y(xy.y), z(_z) {}

        T operator[](size_t i) const { return e[i]; }
        T& operator[](size_t i) { return e[i]; }

        static inline Vec Zero() { return Vec(T(0)); }
        static inline Vec UnitX() { return Vec(T(1), T(0), T(0)); }
        static inline Vec UnitY() { return Vec(T(0), T(1), T(0)); }
        static inline Vec UnitZ() { return Vec(T(0), T(0), T(1)); }
    };

    /// cross is specific to 3-vectors
    template <typename T> inline Vec<3, T> cross(const Vec<3, T>& v1, const Vec<3, T>& v2) {
        return Vec<3, T>((v1.y * v2.z - v1.z * v2.y), -(v1.x * v2.z - v1.z * v2.x), (v1.x * v2.y - v1.y * v2.x));
    }

    /// A 4-vector
    template <typename T> struct Vec<4, T> {
        union {
            std::array<T, 4> e;
            struct {
                T x, y, z, w;
            };
            struct {
                T r, g, b, a;
            };
            Vec<3, T> xyz;
            Vec<3, T> rgb;
            Vec<2, T> xy;
        };

        constexpr Vec() = default;
        constexpr explicit Vec(T e0) : x(e0), y(e0), z(e0), w(e0) {}
        constexpr Vec(T e0, T e1, T e2, T e3) : x(e0), y(e1), z(e2), w(e3) {}
        constexpr Vec(const Vec<3, T> xyz, T _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}

        T operator[](size_t i) const { return e[i]; }
        T& operator[](size_t i) { return e[i]; }

        static inline Vec Zero() { return Vec(T(0)); }
        static inline Vec UnitX() { return Vec(T(1), T(0), T(0), T(0)); }
        static inline Vec UnitY() { return Vec(T(0), T(1), T(0), T(0)); }
        static inline Vec UnitZ() { return Vec(T(0), T(0), T(1), T(0)); }
        static inline Vec UnitW() { return Vec(T(0), T(0), T(0), T(1)); }
    };

    template <size_t N, typename T> std::ostream& operator<<(std::ostream& os, const Vec<N, T>& v) {
        os << "(";
        for (size_t i = 0; i < N; ++i) {
            os << v[i];
            if (i < N - 1) os << ", ";
        }
        os << ")";

        return os;
    }

    // =================================================================================================
    //                                       2D, 3D, 4D vector types
    // =================================================================================================
    template <typename T> using Vec2 = Vec<2, T>;
    template <typename T> using Vec3 = Vec<3, T>;
    template <typename T> using Vec4 = Vec<4, T>;

    template <typename T> using Color3 = Vec<3, T>;
    template <typename T> using Color4 = Vec<4, T>;

    using Vec2f = Vec2<float>;
    using Vec2d = Vec2<double>;
    using Vec2i = Vec2<std::int32_t>;
    using Vec2u = Vec2<std::uint32_t>;
    using Vec2c = Vec2<std::uint8_t>;

    using Vec3f = Vec3<float>;
    using Vec3d = Vec3<double>;
    using Vec3i = Vec3<std::int32_t>;
    using Vec3u = Vec3<std::uint32_t>;
    using Vec3c = Vec3<std::uint8_t>;

    using Vec4f = Vec4<float>;
    using Vec4d = Vec4<double>;
    using Vec4i = Vec4<std::int32_t>;
    using Vec4u = Vec4<std::uint32_t>;
    using Vec4c = Vec4<std::uint8_t>;




// =================================================================================================
//                                            Matrix
// =================================================================================================

    template <typename T> struct Mat44 {
        using Vec3 = Vec<3, T>;
        using Vec4 = Vec<4, T>;

        /// Direct access to matrix elements
        union {
            /// Access to individual matrix elements,
            /// where the first digit is the row, and
            /// second is the column
            struct {
                T m00, m10, m20, m30;
                T m01, m11, m21, m31;
                T m02, m12, m22, m32;
                T m03, m13, m23, m33;
            };
            /// Array access to matrix columns
            std::array<Vec4, 4> m;
            /// Access to columns as named 4-vectors x, y, z, w
            struct {
                Vec4 x, y, z, w;
            };
            /// Access to columns as named 4-vectors a, b, c, d
            struct {
                Vec4 a, b, c, d;
            };
        };

        ///\{ \name Constructors and assignment.
        explicit Mat44(T s);
        Mat44() : Mat44(1) {}
        Mat44(const Vec4 &A, const Vec4 &B, const Vec4 &C, const Vec4 &D);
        Mat44(const Vec3 &A, const Vec3 &B, const Vec3 &C, const Vec3 &D);
        Mat44(const T a[4][4]);
        ///\}

        ///\{ \name Element access.
        const Vec<4, T> &operator[](int col) const { return m[col]; }
        Vec<4, T> &operator[](int col) { return m[col]; }
        T &operator()(int row, int col) { return m[col][row]; }
        T operator()(int row, int col) const { return m[col][row]; }
        ///\}
    };

    template <typename T> inline Mat44<T>::Mat44(T s) {
        m00 = s;
        m01 = 0;
        m02 = 0;
        m03 = 0;
        m10 = 0;
        m11 = s;
        m12 = 0;
        m13 = 0;
        m20 = 0;
        m21 = 0;
        m22 = s;
        m23 = 0;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = s;
    }

    template <typename T> inline Mat44<T>::Mat44(const T mat[4][4]) { memcpy(m, mat, 16 * sizeof(T)); }

    template <typename T>
    inline Mat44<T>::Mat44(const Vec<4, T> &a, const Vec<4, T> &b, const Vec<4, T> &c, const Vec<4, T> &d)
        : x(a), y(b), z(c), w(d) {}

    template <typename T>
    inline Mat44<T>::Mat44(const Vec<3, T> &a, const Vec<3, T> &b, const Vec<3, T> &c, const Vec<3, T> &d)
        : Mat44({a, 0}, {b, 0}, {c, 0}, {d, 1}) {}

    template <typename T> inline Mat44<T> multiply(const Mat44<T> &a, const Mat44<T> &b) {
        return {{a.x * b.x.x + a.y * b.x.y + a.z * b.x.z + a.w * b.x.w},
                {a.x * b.y.x + a.y * b.y.y + a.z * b.y.z + a.w * b.y.w},
                {a.x * b.z.x + a.y * b.z.y + a.z * b.z.z + a.w * b.z.w},
                {a.x * b.w.x + a.y * b.w.y + a.z * b.w.z + a.w * b.w.w}};
    }

    template <typename T> inline const Mat44<T> &operator*=(Mat44<T> &lhs, const Mat44<T> &rhs) {
        return (lhs = multiply(lhs, rhs));
    }

    template <typename T> inline const Mat44<T> &operator/=(Mat44<T> &m, T a) { return (m *= T(1) / a); }

    template <typename T> inline Mat44<T> operator*(const Mat44<T> &lhs, const Mat44<T> &rhs) {
        return multiply(lhs, rhs);
    }

    template <typename T> inline Mat44<T> operator*(T a, const Mat44<T> &m) { return m * a; }

    template <typename T> inline Vec<4, T> operator*(const Mat44<T> &m, const Vec<4, T> &a) {
        return Vec<4, T>(m(0, 0) * a[0] + m(0, 1) * a[1] + m(0, 2) * a[2] + m(0, 3) * a[3],
                        m(1, 0) * a[0] + m(1, 1) * a[1] + m(1, 2) * a[2] + m(1, 3) * a[3],
                        m(2, 0) * a[0] + m(2, 1) * a[1] + m(2, 2) * a[2] + m(2, 3) * a[3],
                        m(3, 0) * a[0] + m(3, 1) * a[1] + m(3, 2) * a[2] + m(3, 3) * a[3]);
    }

    template <typename T> inline Vec<3, T> operator*(const Mat44<T> &m, const Vec<3, T> &a) {
        return Vec<3, T>(m(0, 0) * a[0] + m(0, 1) * a[1] + m(0, 2) * a[2],
                        m(1, 0) * a[0] + m(1, 1) * a[1] + m(1, 2) * a[2],
                        m(2, 0) * a[0] + m(2, 1) * a[1] + m(2, 2) * a[2]);
    }

    template <typename T> inline Mat44<T> operator*(const Mat44<T> &m, T a) {
        return Mat44<T>(m[0] * a, m[1] * a, m[2] * a, m[3] * a);
    }

    template <typename T> inline const Mat44<T> &operator*=(Mat44<T> &m, T a) {
        m[0] *= a;
        m[1] *= a;
        m[2] *= a;
        m[3] *= a;
        return m;
    }

    template <typename T> inline Mat44<T> operator/(const Mat44<T> &m, T a) { return m * (T(1) / a); }

    //----------------------------------
    // Transpose and inverse
    //----------------------------------

    template <typename T> inline Mat44<T> transpose(const Mat44<T> &m) {
        return {
            {m.m00, m.m01, m.m02, m.m03},
            {m.m10, m.m11, m.m12, m.m13},
            {m.m20, m.m21, m.m22, m.m23},
            {m.m30, m.m31, m.m32, m.m33}
        };
    }

    template <typename T> Mat44<T> adjugate(const Mat44<T> &m) {
        return {
            {m.m11 * m.m22 * m.m33 + m.m13 * m.m21 * m.m32 + m.m12 * m.m23 * m.m31 - m.m11 * m.m23 * m.m32 -
            m.m12 * m.m21 * m.m33 - m.m13 * m.m22 * m.m31,
            m.m10 * m.m23 * m.m32 + m.m12 * m.m20 * m.m33 + m.m13 * m.m22 * m.m30 - m.m13 * m.m20 * m.m32 -
            m.m12 * m.m23 * m.m30 - m.m10 * m.m22 * m.m33,
            m.m10 * m.m21 * m.m33 + m.m13 * m.m20 * m.m31 + m.m11 * m.m23 * m.m30 - m.m10 * m.m23 * m.m31 -
            m.m11 * m.m20 * m.m33 - m.m13 * m.m21 * m.m30,
            m.m10 * m.m22 * m.m31 + m.m11 * m.m20 * m.m32 + m.m12 * m.m21 * m.m30 - m.m10 * m.m21 * m.m32 -
            m.m12 * m.m20 * m.m31 - m.m11 * m.m22 * m.m30},
            {m.m21 * m.m33 * m.m02 + m.m22 * m.m31 * m.m03 + m.m23 * m.m32 * m.m01 - m.m21 * m.m32 * m.m03 -
            m.m23 * m.m31 * m.m02 - m.m22 * m.m33 * m.m01,
            m.m20 * m.m32 * m.m03 + m.m23 * m.m30 * m.m02 + m.m22 * m.m33 * m.m00 - m.m20 * m.m33 * m.m02 -
            m.m22 * m.m30 * m.m03 - m.m23 * m.m32 * m.m00,
            m.m20 * m.m33 * m.m01 + m.m21 * m.m30 * m.m03 + m.m23 * m.m31 * m.m00 - m.m20 * m.m31 * m.m03 -
            m.m23 * m.m30 * m.m01 - m.m21 * m.m33 * m.m00,
            m.m20 * m.m31 * m.m02 + m.m22 * m.m30 * m.m01 + m.m21 * m.m32 * m.m00 - m.m20 * m.m32 * m.m01 -
            m.m21 * m.m30 * m.m02 - m.m22 * m.m31 * m.m00},
            {m.m31 * m.m02 * m.m13 + m.m33 * m.m01 * m.m12 + m.m32 * m.m03 * m.m11 - m.m31 * m.m03 * m.m12 -
            m.m32 * m.m01 * m.m13 - m.m33 * m.m02 * m.m11,
            m.m30 * m.m03 * m.m12 + m.m32 * m.m00 * m.m13 + m.m33 * m.m02 * m.m10 - m.m30 * m.m02 * m.m13 -
            m.m33 * m.m00 * m.m12 - m.m32 * m.m03 * m.m10,
            m.m30 * m.m01 * m.m13 + m.m33 * m.m00 * m.m11 + m.m31 * m.m03 * m.m10 - m.m30 * m.m03 * m.m11 -
            m.m31 * m.m00 * m.m13 - m.m33 * m.m01 * m.m10,
            m.m30 * m.m02 * m.m11 + m.m31 * m.m00 * m.m12 + m.m32 * m.m01 * m.m10 - m.m30 * m.m01 * m.m12 -
            m.m32 * m.m00 * m.m11 - m.m31 * m.m02 * m.m10},
            {m.m01 * m.m13 * m.m22 + m.m02 * m.m11 * m.m23 + m.m03 * m.m12 * m.m21 - m.m01 * m.m12 * m.m23 -
            m.m03 * m.m11 * m.m22 - m.m02 * m.m13 * m.m21,
            m.m00 * m.m12 * m.m23 + m.m03 * m.m10 * m.m22 + m.m02 * m.m13 * m.m20 - m.m00 * m.m13 * m.m22 -
            m.m02 * m.m10 * m.m23 - m.m03 * m.m12 * m.m20,
            m.m00 * m.m13 * m.m21 + m.m01 * m.m10 * m.m23 + m.m03 * m.m11 * m.m20 - m.m00 * m.m11 * m.m23 -
            m.m03 * m.m10 * m.m21 - m.m01 * m.m13 * m.m20,
            m.m00 * m.m11 * m.m22 + m.m02 * m.m10 * m.m21 + m.m01 * m.m12 * m.m20 - m.m00 * m.m12 * m.m21 -
            m.m01 * m.m10 * m.m22 - m.m02 * m.m11 * m.m20}
        };
    }

    template <typename T> T determinant(const Mat44<T> &m) {
        return m.m00 * (m.m11 * m.m22 * m.m33 + m.m13 * m.m21 * m.m32 + m.m12 * m.m23 * m.m31 - m.m11 * m.m23 * m.m32 -
                        m.m12 * m.m21 * m.m33 - m.m13 * m.m22 * m.m31) +
            m.m10 * (m.m21 * m.m33 * m.m02 + m.m22 * m.m31 * m.m03 + m.m23 * m.m32 * m.m01 - m.m21 * m.m32 * m.m03 -
                        m.m23 * m.m31 * m.m02 - m.m22 * m.m33 * m.m01) +
            m.m20 * (m.m31 * m.m02 * m.m13 + m.m33 * m.m01 * m.m12 + m.m32 * m.m03 * m.m11 - m.m31 * m.m03 * m.m12 -
                        m.m32 * m.m01 * m.m13 - m.m33 * m.m02 * m.m11) +
            m.m30 * (m.m01 * m.m13 * m.m22 + m.m02 * m.m11 * m.m23 + m.m03 * m.m12 * m.m21 - m.m01 * m.m12 * m.m23 -
                        m.m03 * m.m11 * m.m22 - m.m02 * m.m13 * m.m21);
    }

    template <typename T> inline Mat44<T> inverse(const Mat44<T>& m) { return adjugate(m) / determinant(m); }
    
    // return a rotate matrix transformation based on given axis and angle
    template <typename T> Mat44<T> rotate(const Vec<3, T>& axis, T angle) {
        T c = std::cos(angle);
        T s = std::sin(angle);
        T t = T(1) - c;
        Vec<3, T> n = normalize(axis);

        return Mat44<T>(
            { t * n.x * n.x + c, t * n.x * n.y + s * n.z, t * n.x * n.z - s * n.y, 0 },
            { t * n.x * n.y - s * n.z, t * n.y * n.y + c, t * n.y * n.z + s * n.x, 0 },
            { t * n.x * n.z + s * n.y, t * n.y * n.z - s * n.x, t * n.z * n.z + c, 0 },
            { 0, 0, 0, 1 }
        );

    }

    template <typename T> Mat44<T> lookAt(const Vec<3, T>& eye, const Vec<3, T>& center, const Vec<3, T>& up) {
        Vec<3, T> f = normalize(center - eye);
        Vec<3, T> u = normalize(up);
        Vec<3, T> s = normalize(cross(f, u));
        u = cross(s, f);

        return Mat44<T>(
            {s.x, u.x, -f.x, 0},
            {s.y, u.y, -f.y, 0},
            {s.z, u.z, -f.z, 0},
            {-dot(s, eye), -dot(u, eye), dot(f, eye), 1}
        );
    }

    template <typename T> Mat44<T> perspective(T fovy, T aspect, T zNear, T zFar) {
        T f = T(1) / std::tan(fovy / T(2));
        return Mat44<T>(
            {f / aspect, 0, 0, 0},
            {0, f, 0, 0},
            {0, 0, (zFar + zNear) / (zNear - zFar), -1},
            {0, 0, (T(2) * zFar * zNear) / (zNear - zFar), 0}
        );
    }

    template <typename T> Mat44<T> translation(Vec3<T> v) {
        return Mat44<T>(
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {v.x, v.y, v.z, 1}
        );
    }

    template <typename T> Mat44<T> rotation(Vec4<T> r) {
        T x = r.x;
        T y = r.y;
        T z = r.z;
        T w = r.w;

        T x2 = x + x;
        T y2 = y + y;
        T z2 = z + z;
        T xx = x * x2;
        T xy = x * y2;
        T xz = x * z2;
        T yy = y * y2;
        T yz = y * z2;
        T zz = z * z2;
        T wx = w * x2;
        T wy = w * y2;
        T wz = w * z2;

        return Mat44<T>(
            {1 - (yy + zz), xy + wz, xz - wy, 0},
            {xy - wz, 1 - (xx + zz), yz + wx, 0},
            {xz + wy, yz - wx, 1 - (xx + yy), 0},
            {0, 0, 0, 1}
        );
    }

    template <typename T> Mat44<T> scale(Vec3<T> s) {
        return Mat44<T>(
            {s.x, 0, 0, 0},
            {0, s.y, 0, 0},
            {0, 0, s.z, 0},
            {0, 0, 0, 1}
        );
    }

    template <typename T> Vec3<T> transform_point(const Vec3<T>& p, const Mat44<T>& m) {
        Vec4<T> v = m * Vec4<T>(p, (T)1);
        return Vec3<T>(v.x / v.w, v.y / v.w, v.z / v.w);
    }

    using Mat44f = Mat44<float>;
    using Mat44d = Mat44<double>;

    
} // namespace cglm