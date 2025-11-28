#pragma once

#include "types.h"

#include <array>

#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

template <typename T, size_t Rows, size_t Columns>
struct MatrixBase {
    union {
        T m[Rows][Columns];
        std::array<T, Rows * Columns> a;
    };
};

using Matrix33f = MatrixBase<f32, 3, 3>;
using Matrix34f = MatrixBase<f32, 3, 4>;
using Matrix44f = MatrixBase<f32, 4, 4>;

template <typename T>
struct Vector2 {
    union {
        struct {
            T x, y;
        };
        std::array<T, 2> e;
    };
};

template <typename T>
struct Vector3 {
    union {
        struct {
            T x, y, z;
        };
        std::array<T, 3> e;
    };
};

template <typename T>
struct Vector4 {
    union {
        struct {
            T x, y, z, w;
        };
        std::array<T, 4> e;
    };
};

using Vector2f = Vector2<f32>;
using Vector3f = Vector3<f32>;
using Vector4f = Vector4<f32>;

template <typename T>
struct Quat {
    union {
        struct {
            T x, y, z, w;
        };
        std::array<T, 4> e;
    };
};

using Quatf = Quat<f32>;

#ifdef _MSC_VER
#pragma warning(default : 4201)
#endif