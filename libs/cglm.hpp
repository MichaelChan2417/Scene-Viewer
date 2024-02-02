#pragma once

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdlib.h>

namespace cglm {

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





    
} // namespace cglm