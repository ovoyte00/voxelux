/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional 3D vector mathematics library.
 * Independent implementation of vector operations and utilities.
 */

#pragma once

#include <cmath>
#include <functional>

namespace voxelux::core {

template<typename T>
struct Vector3 {
    T x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x_val, T y_val, T z_val) : x(x_val), y(y_val), z(z_val) {}
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator*(T scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    Vector3& operator+=(const Vector3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    
    Vector3& operator-=(const Vector3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    
    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    bool operator!=(const Vector3& other) const {
        return !(*this == other);
    }
    
    T length_squared() const {
        return x * x + y * y + z * z;
    }
    
    T length() const {
        return std::sqrt(length_squared());
    }
    
    Vector3 normalized() const {
        T len = length();
        if (len > 0) {
            return *this * (T(1) / len);
        }
        return Vector3();
    }
};

using Vector3i = Vector3<int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

}

namespace std {
    template<typename T>
    struct hash<voxelux::core::Vector3<T>> {
        size_t operator()(const voxelux::core::Vector3<T>& v) const {
            return hash<T>()(v.x) ^ (hash<T>()(v.y) << 1) ^ (hash<T>()(v.z) << 2);
        }
    };
}