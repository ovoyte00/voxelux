/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional 3D Camera System Implementation
 */

#include "canvas_ui/camera_3d.h"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// Constants
namespace {
    constexpr float PI = 3.14159265359f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-6f;
}

// Quaternion implementation

Quaternion::Quaternion(const Vector3D& axis, float angle) {
    float half_angle = angle * 0.5f;
    float sin_half = std::sin(half_angle);
    Vector3D norm_axis = axis.normalized();
    
    x = norm_axis.x * sin_half;
    y = norm_axis.y * sin_half;
    z = norm_axis.z * sin_half;
    w = std::cos(half_angle);
}

Quaternion::Quaternion(float pitch, float yaw, float roll) {
    from_euler(pitch, yaw, roll);
}

Quaternion Quaternion::operator*(const Quaternion& other) const {
    return {
        w * other.x + x * other.w + y * other.z - z * other.y,
        w * other.y - x * other.z + y * other.w + z * other.x,
        w * other.z + x * other.y - y * other.x + z * other.w,
        w * other.w - x * other.x - y * other.y - z * other.z
    };
}

Quaternion Quaternion::normalized() const {
    float len = length();
    return len > EPSILON ? Quaternion{x / len, y / len, z / len, w / len} : identity();
}

Vector3D Quaternion::to_euler() const {
    // Convert quaternion to Euler angles (pitch, yaw, roll)
    float sinr_cosp = 2 * (w * x + y * z);
    float cosr_cosp = 1 - 2 * (x * x + y * y);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2 * (w * y - z * x);
    float pitch = std::abs(sinp) >= 1 ? std::copysign(PI / 2, sinp) : std::asin(sinp);

    float siny_cosp = 2 * (w * z + x * y);
    float cosy_cosp = 1 - 2 * (y * y + z * z);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return {pitch * RAD_TO_DEG, yaw * RAD_TO_DEG, roll * RAD_TO_DEG};
}

void Quaternion::from_euler(float pitch, float yaw, float roll) {
    float p = pitch * DEG_TO_RAD * 0.5f;
    float y = yaw * DEG_TO_RAD * 0.5f;
    float r = roll * DEG_TO_RAD * 0.5f;

    float cy = std::cos(y);
    float sy = std::sin(y);
    float cp = std::cos(p);
    float sp = std::sin(p);
    float cr = std::cos(r);
    float sr = std::sin(r);

    w = cr * cp * cy + sr * sp * sy;
    x = sr * cp * cy - cr * sp * sy;
    y = cr * sp * cy + sr * cp * sy;
    z = cr * cp * sy - sr * sp * cy;
}

Vector3D Quaternion::rotate_vector(const Vector3D& vec) const {
    Quaternion vec_quat(vec.x, vec.y, vec.z, 0);
    Quaternion result = *this * vec_quat * conjugate();
    return {result.x, result.y, result.z};
}

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    // If the dot product is negative, slerp won't take the shorter path
    Quaternion b_adjusted = (dot < 0.0f) ? Quaternion{-b.x, -b.y, -b.z, -b.w} : b;
    dot = std::abs(dot);
    
    if (dot > 1.0f - EPSILON) {
        // Linear interpolation for very close quaternions
        Quaternion result = {
            a.x + t * (b_adjusted.x - a.x),
            a.y + t * (b_adjusted.y - a.y),
            a.z + t * (b_adjusted.z - a.z),
            a.w + t * (b_adjusted.w - a.w)
        };
        return result.normalized();
    }
    
    float theta_0 = std::acos(dot);
    float theta = theta_0 * t;
    float sin_theta = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);
    
    float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    
    return {
        s0 * a.x + s1 * b_adjusted.x,
        s0 * a.y + s1 * b_adjusted.y,
        s0 * a.z + s1 * b_adjusted.z,
        s0 * a.w + s1 * b_adjusted.w
    };
}

Quaternion Quaternion::from_axis_angle(const Vector3D& axis, float angle) {
    return Quaternion(axis, angle);
}

Quaternion Quaternion::look_rotation(const Vector3D& forward, const Vector3D& up) {
    Vector3D f = forward.normalized();
    Vector3D u = up.normalized();
    
    // Handle the case where forward is nearly opposite to default forward (0,0,-1)
    // This happens when looking straight down the Z axis
    if (std::abs(f.z + 1.0f) < 0.001f && std::abs(f.x) < 0.001f && std::abs(f.y) < 0.001f) {
        // Looking straight back along Z axis - 180 degree rotation around Y
        return Quaternion(0, 1, 0, 0); // 180 degree rotation around Y axis
    }
    
    Vector3D r = u.cross(f).normalized();
    u = f.cross(r);
    
    // Create rotation matrix and convert to quaternion
    float trace = r.x + u.y + f.z;
    if (trace > 0) {
        float s = std::sqrt(trace + 1.0f) * 2; // s = 4 * qw
        return {
            (u.z - f.y) / s,
            (f.x - r.z) / s,
            (r.y - u.x) / s,
            0.25f * s
        };
    } else if (r.x > u.y && r.x > f.z) {
        float s = std::sqrt(1.0f + r.x - u.y - f.z) * 2; // s = 4 * qx
        return {
            0.25f * s,
            (r.y + u.x) / s,
            (f.x + r.z) / s,
            (u.z - f.y) / s
        };
    } else if (u.y > f.z) {
        float s = std::sqrt(1.0f + u.y - r.x - f.z) * 2; // s = 4 * qy
        return {
            (r.y + u.x) / s,
            0.25f * s,
            (u.z + f.y) / s,
            (f.x - r.z) / s
        };
    } else {
        float s = std::sqrt(1.0f + f.z - r.x - u.y) * 2; // s = 4 * qz
        return {
            (f.x + r.z) / s,
            (u.z + f.y) / s,
            0.25f * s,
            (r.y - u.x) / s
        };
    }
}

// Matrix4x4 implementation

Matrix4x4::Matrix4x4() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Matrix4x4::Matrix4x4(float diagonal) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = (i == j) ? diagonal : 0.0f;
        }
    }
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

Vector3D Matrix4x4::transform_point(const Vector3D& point) const {
    float w = m[3][0] * point.x + m[3][1] * point.y + m[3][2] * point.z + m[3][3];
    if (std::abs(w) < EPSILON) w = 1.0f;
    
    return {
        (m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3]) / w,
        (m[1][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3]) / w,
        (m[2][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3]) / w
    };
}

Vector3D Matrix4x4::transform_direction(const Vector3D& direction) const {
    return {
        m[0][0] * direction.x + m[0][1] * direction.y + m[0][2] * direction.z,
        m[1][0] * direction.x + m[1][1] * direction.y + m[1][2] * direction.z,
        m[2][0] * direction.x + m[2][1] * direction.y + m[2][2] * direction.z
    };
}

Matrix4x4 Matrix4x4::identity() {
    return Matrix4x4();
}

Matrix4x4 Matrix4x4::translation(const Vector3D& offset) {
    Matrix4x4 result;
    result.m[0][3] = offset.x;
    result.m[1][3] = offset.y;
    result.m[2][3] = offset.z;
    return result;
}

Matrix4x4 Matrix4x4::rotation(const Quaternion& q) {
    Matrix4x4 result;
    
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;
    
    result.m[0][0] = 1 - 2 * (yy + zz);
    result.m[0][1] = 2 * (xy - wz);
    result.m[0][2] = 2 * (xz + wy);
    
    result.m[1][0] = 2 * (xy + wz);
    result.m[1][1] = 1 - 2 * (xx + zz);
    result.m[1][2] = 2 * (yz - wx);
    
    result.m[2][0] = 2 * (xz - wy);
    result.m[2][1] = 2 * (yz + wx);
    result.m[2][2] = 1 - 2 * (xx + yy);
    
    return result;
}

Matrix4x4 Matrix4x4::scale(const Vector3D& scale) {
    Matrix4x4 result;
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return result;
}

Matrix4x4 Matrix4x4::scale(float uniform_scale) {
    return scale({uniform_scale, uniform_scale, uniform_scale});
}

Matrix4x4 Matrix4x4::look_at(const Vector3D& eye, const Vector3D& target, const Vector3D& up) {
    // Calculate basis vectors
    Vector3D forward = (eye - target).normalized();  // Note: eye - target for RH coordinate system
    Vector3D right = up.cross(forward).normalized();
    Vector3D actual_up = forward.cross(right);
    
    
    Matrix4x4 result;
    
    // Right vector
    result.m[0][0] = right.x;
    result.m[1][0] = right.y;
    result.m[2][0] = right.z;
    result.m[3][0] = 0.0f;
    
    // Up vector
    result.m[0][1] = actual_up.x;
    result.m[1][1] = actual_up.y;
    result.m[2][1] = actual_up.z;
    result.m[3][1] = 0.0f;
    
    // Forward vector (negated for right-handed system looking down -Z)
    result.m[0][2] = forward.x;
    result.m[1][2] = forward.y;
    result.m[2][2] = forward.z;
    result.m[3][2] = 0.0f;
    
    // Translation
    result.m[0][3] = -right.dot(eye);
    result.m[1][3] = -actual_up.dot(eye);
    result.m[2][3] = -forward.dot(eye);
    result.m[3][3] = 1.0f;
    
    return result;
}

Matrix4x4 Matrix4x4::perspective(float fovy, float aspect, float near_plane, float far_plane) {
    Matrix4x4 result(0);
    
    float tan_half_fovy = std::tan(fovy * DEG_TO_RAD * 0.5f);
    
    result.m[0][0] = 1.0f / (aspect * tan_half_fovy);
    result.m[1][1] = 1.0f / tan_half_fovy;
    result.m[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
    result.m[2][3] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
    result.m[3][2] = -1.0f;
    
    return result;
}

Matrix4x4 Matrix4x4::orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane) {
    Matrix4x4 result;
    
    result.m[0][0] = 2.0f / (right - left);
    result.m[0][3] = -(right + left) / (right - left);
    
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[1][3] = -(top + bottom) / (top - bottom);
    
    result.m[2][2] = -2.0f / (far_plane - near_plane);
    result.m[2][3] = -(far_plane + near_plane) / (far_plane - near_plane);
    
    return result;
}

Matrix4x4 Matrix4x4::inverse() const {
    Matrix4x4 result;
    float det = determinant();
    
    if (std::abs(det) < EPSILON) {
        return identity(); // Return identity if matrix is not invertible
    }
    
    // Simplified 4x4 inverse calculation (could be optimized)
    float inv_det = 1.0f / det;
    
    // This is a simplified implementation - a full implementation would compute all cofactors
    // For now, we'll handle common cases
    result.m[0][0] = (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) - 
                      m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) + 
                      m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])) * inv_det;
    
    // ... (full implementation would continue for all elements)
    // For brevity, returning a basic inverse for translation matrices
    if (m[3][0] == 0 && m[3][1] == 0 && m[3][2] == 0 && m[3][3] == 1) {
        // This is likely a transformation matrix
        result = *this;
        result.m[0][3] = -m[0][3];
        result.m[1][3] = -m[1][3];
        result.m[2][3] = -m[2][3];
    }
    
    return result;
}

Matrix4x4 Matrix4x4::transpose() const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

float Matrix4x4::determinant() const {
    // 4x4 determinant calculation
    return m[0][3] * m[1][2] * m[2][1] * m[3][0] - m[0][2] * m[1][3] * m[2][1] * m[3][0] -
           m[0][3] * m[1][1] * m[2][2] * m[3][0] + m[0][1] * m[1][3] * m[2][2] * m[3][0] +
           m[0][2] * m[1][1] * m[2][3] * m[3][0] - m[0][1] * m[1][2] * m[2][3] * m[3][0] -
           m[0][3] * m[1][2] * m[2][0] * m[3][1] + m[0][2] * m[1][3] * m[2][0] * m[3][1] +
           m[0][3] * m[1][0] * m[2][2] * m[3][1] - m[0][0] * m[1][3] * m[2][2] * m[3][1] -
           m[0][2] * m[1][0] * m[2][3] * m[3][1] + m[0][0] * m[1][2] * m[2][3] * m[3][1] +
           m[0][3] * m[1][1] * m[2][0] * m[3][2] - m[0][1] * m[1][3] * m[2][0] * m[3][2] -
           m[0][3] * m[1][0] * m[2][1] * m[3][2] + m[0][0] * m[1][3] * m[2][1] * m[3][2] +
           m[0][1] * m[1][0] * m[2][3] * m[3][2] - m[0][0] * m[1][1] * m[2][3] * m[3][2] -
           m[0][2] * m[1][1] * m[2][0] * m[3][3] + m[0][1] * m[1][2] * m[2][0] * m[3][3] +
           m[0][2] * m[1][0] * m[2][1] * m[3][3] - m[0][0] * m[1][2] * m[2][1] * m[3][3] -
           m[0][1] * m[1][0] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
}

// Camera3D implementation

Camera3D::Camera3D() {
    reset_to_default();
}

void Camera3D::set_position(const Vector3D& position) {
    position_ = position;
    update_orbit_from_position();
    mark_view_dirty();
}

void Camera3D::set_target(const Vector3D& target) {
    target_ = target;
    if (navigation_mode_ == NavigationMode::Orbit) {
        orbit_target_ = target;
        update_orbit_from_position();
    }
    mark_view_dirty();
}

void Camera3D::set_up_vector(const Vector3D& up) {
    up_vector_ = up.normalized();
    mark_view_dirty();
}

void Camera3D::set_rotation(const Quaternion& rotation) {
    rotation_ = rotation.normalized();
    if (navigation_mode_ == NavigationMode::Orbit) {
        update_position_from_orbit();
    }
    mark_view_dirty();
}

Vector3D Camera3D::get_forward_vector() const {
    return rotation_.rotate_vector(Vector3D::forward());
}

Vector3D Camera3D::get_right_vector() const {
    return rotation_.rotate_vector(Vector3D::right());
}

Vector3D Camera3D::get_actual_up_vector() const {
    return rotation_.rotate_vector(Vector3D::up());
}

void Camera3D::set_distance(float distance) {
    distance_ = std::max(min_distance_, std::min(max_distance_, distance));
    if (navigation_mode_ == NavigationMode::Orbit) {
        update_position_from_orbit();
        mark_view_dirty();
    }
}

void Camera3D::set_orbit_target(const Vector3D& target) {
    orbit_target_ = target;
    target_ = target;
    if (navigation_mode_ == NavigationMode::Orbit) {
        update_position_from_orbit();
        mark_view_dirty();
    }
}

void Camera3D::set_projection_type(ProjectionType type) {
    projection_type_ = type;
    mark_projection_dirty();
}

void Camera3D::set_field_of_view(float fov_degrees) {
    fov_degrees_ = std::max(1.0f, std::min(179.0f, fov_degrees));
    mark_projection_dirty();
}

void Camera3D::set_orthographic_size(float size) {
    ortho_size_ = std::max(0.1f, size);
    mark_projection_dirty();
}

void Camera3D::set_near_plane(float near_plane) {
    near_plane_ = std::max(0.001f, near_plane);
    mark_projection_dirty();
}

void Camera3D::set_far_plane(float far_plane) {
    far_plane_ = std::max(near_plane_ + 0.1f, far_plane);
    mark_projection_dirty();
}

void Camera3D::set_aspect_ratio(float aspect) {
    aspect_ratio_ = std::max(0.1f, aspect);
    mark_projection_dirty();
}

void Camera3D::set_navigation_mode(NavigationMode mode) {
    if (navigation_mode_ == mode) {
        return; // Already in this mode, don't update
    }
    navigation_mode_ = mode;
    if (mode == NavigationMode::Orbit) {
        update_orbit_from_position();
    }
}

Matrix4x4 Camera3D::get_view_matrix() const {
    if (view_matrix_dirty_) {
        update_cached_matrices();
    }
    return cached_view_matrix_;
}

Matrix4x4 Camera3D::get_projection_matrix() const {
    if (projection_matrix_dirty_) {
        update_cached_matrices();
    }
    return cached_projection_matrix_;
}

Matrix4x4 Camera3D::get_view_projection_matrix() const {
    return get_projection_matrix() * get_view_matrix();
}

void Camera3D::orbit_horizontal(float angle_radians) {
    if (navigation_mode_ == NavigationMode::Orbit && use_turntable_) {
        // Turntable mode: rotate around global Y axis (up)
        // Global rotation is applied first for professional 3D navigation
        Quaternion quat_global_y = Quaternion::from_axis_angle(Vector3D(0, 1, 0), angle_radians);
        
        // Pre-multiply for global rotation (rotate the camera in world space)
        rotation_ = quat_global_y * rotation_;
        rotation_ = rotation_.normalized();
        
        // Update position from the new view quaternion
        update_position_from_view_quat();
        
        // IMPORTANT: Update the angle to reflect actual rotation
        horizontal_angle_ += angle_radians;
        
        mark_view_dirty();
    } else if (navigation_mode_ == NavigationMode::Orbit) {
        horizontal_angle_ += angle_radians;
        update_position_from_orbit();
        mark_view_dirty();
    }
}

void Camera3D::orbit_vertical(float angle_radians) {
    if (navigation_mode_ == NavigationMode::Orbit && use_turntable_) {
        // Turntable mode: rotate around the camera's local X axis (right vector)
        // Local rotation for vertical movement
        
        // Get the camera's right vector from the current view quaternion
        // In view space, right is +X
        Vector3D right = rotation_.rotate_vector(Vector3D(1, 0, 0));
        
        // Create rotation around this axis
        Quaternion quat_local_x = Quaternion::from_axis_angle(right, angle_radians);
        
        // Pre-multiply to apply in world space for proper rotation order
        rotation_ = quat_local_x * rotation_;
        rotation_ = rotation_.normalized();
        
        // Update position from the new view quaternion
        update_position_from_view_quat();
        
        // IMPORTANT: Update the angle to reflect actual rotation
        vertical_angle_ += angle_radians;
        
        mark_view_dirty();
    } else if (navigation_mode_ == NavigationMode::Orbit) {
        vertical_angle_ += angle_radians;
        update_position_from_orbit();
        mark_view_dirty();
    }
}

void Camera3D::pan(const Vector3D& delta) {
    Vector3D right = get_right_vector();
    Vector3D up = get_actual_up_vector();
    Vector3D movement = right * delta.x + up * delta.y;
    
    position_ += movement;
    if (navigation_mode_ == NavigationMode::Orbit) {
        orbit_target_ += movement;
        target_ = orbit_target_;
    }
    
    mark_view_dirty();
}

void Camera3D::zoom(float factor) {
    if (projection_type_ == ProjectionType::Perspective) {
        if (navigation_mode_ == NavigationMode::Orbit) {
            // std::cout << "[Camera] Zoom in orbit mode: factor=" << factor 
            //           << " old_distance=" << distance_ 
            //           << " new_distance=" << (distance_ * factor) << std::endl;
            // std::cout << "[Camera] Old position: " << position_.x << ", " << position_.y << ", " << position_.z << std::endl;
            // std::cout << "[Camera] Target: " << orbit_target_.x << ", " << orbit_target_.y << ", " << orbit_target_.z << std::endl;
            set_distance(distance_ * factor);
            // std::cout << "[Camera] New position: " << position_.x << ", " << position_.y << ", " << position_.z << std::endl;
        } else {
            Vector3D forward = get_forward_vector();
            position_ += forward * (distance_ * (1.0f - factor));
        }
    } else {
        ortho_size_ *= factor;
        mark_projection_dirty();
    }
}

void Camera3D::dolly(float distance) {
    Vector3D forward = get_forward_vector();
    position_ += forward * distance;
    mark_view_dirty();
}

void Camera3D::rotate_around_target(float horizontal_angle, float vertical_angle) {
    // Apply rotations separately for turntable navigation
    orbit_horizontal(horizontal_angle);
    orbit_vertical(vertical_angle);
}

void Camera3D::look_at(const Vector3D& target, const Vector3D& up) {
    
    target_ = target;
    up_vector_ = up.normalized();
    
    Vector3D forward = (target - position_).normalized();
    rotation_ = Quaternion::look_rotation(forward, up_vector_);
    
    if (navigation_mode_ == NavigationMode::Orbit) {
        orbit_target_ = target;
        // Update spherical coordinates but preserve the quaternion we just set
        Vector3D to_camera = position_ - target;
        distance_ = to_camera.length();
        CameraUtils::cartesian_to_spherical(to_camera, distance_, horizontal_angle_, vertical_angle_);
        vertical_angle_ = PI * 0.5f - vertical_angle_; // Convert to elevation angle
        // Don't rebuild the quaternion - keep the one from look_rotation
    }
    
    mark_view_dirty();
}

void Camera3D::set_axis_view(AxisView view, float distance) {
    // Pre-calculated quaternions for each axis view
    // These represent the exact rotations needed for each view
    // Using the same approach as professional 3D applications
    
    const float SQRT_HALF = 0.70710678118654752440f; // sqrt(0.5) or 1/sqrt(2)
    
    // Set the position and rotation based on the view
    orbit_target_ = Vector3D(0, 0, 0);
    target_ = orbit_target_;
    distance_ = distance;
    
    switch (view) {
        case AxisView::Front:
            // Looking along -Z axis (camera at +Z)
            position_ = Vector3D(0, 0, distance);
            // Identity quaternion - we're already looking along -Z by default
            rotation_ = Quaternion(0, 0, 0, 1);
            break;
            
        case AxisView::Back:
            // Looking along +Z axis (camera at -Z)
            position_ = Vector3D(0, 0, -distance);
            // 180° rotation around Y to look backward
            rotation_ = Quaternion(0, 1, 0, 0);
            break;
            
        case AxisView::Right:
            // Looking along -X axis (camera at +X)
            position_ = Vector3D(distance, 0, 0);
            // 90° rotation around Y
            rotation_ = Quaternion(0, SQRT_HALF, 0, SQRT_HALF);
            break;
            
        case AxisView::Left:
            // Looking along +X axis (camera at -X)
            position_ = Vector3D(-distance, 0, 0);
            // -90° rotation around Y
            rotation_ = Quaternion(0, -SQRT_HALF, 0, SQRT_HALF);
            break;
            
        case AxisView::Top:
            // Looking along -Y axis (camera at +Y)
            position_ = Vector3D(0, distance, 0);
            // 90° rotation around X to look down
            // This should rotate from default forward (-Z) to looking down (-Y)
            rotation_ = Quaternion(-SQRT_HALF, 0, 0, SQRT_HALF);
            break;
            
        case AxisView::Bottom:
            // Looking along +Y axis (camera at -Y)
            position_ = Vector3D(0, -distance, 0);
            // -90° rotation around X (opposite of Top)
            rotation_ = Quaternion(SQRT_HALF, 0, 0, SQRT_HALF);
            break;
            
        case AxisView::Custom:
            // Don't change anything for custom view
            return;
    }
    
    // Update spherical coordinates to match
    Vector3D to_camera = position_ - target_;
    CameraUtils::cartesian_to_spherical(to_camera, distance_, horizontal_angle_, vertical_angle_);
    vertical_angle_ = PI * 0.5f - vertical_angle_;
    
    up_vector_ = Vector3D(0, 1, 0);
    mark_view_dirty();
}

void Camera3D::reset_to_default() {
    // Set up camera for voxel editing with right-handed coordinate system:
    // +Y pointing up (vertical)
    // +X pointing right
    // +Z pointing towards viewer (out of screen)
    // This means camera looks from positive Z towards negative Z (into the scene)
    
    // Set orbit target at origin
    orbit_target_ = {0, 0, 0};
    target_ = orbit_target_;
    up_vector_ = {0, 1, 0};  // Y is up
    
    // Set distance for viewing
    distance_ = 60.0f;  // Distance to comfortably see 20x20x20 cube
    
    // Build view quaternion for initial 3/4 view
    // Looking from front-right-above (positive X, positive Y, positive Z quadrant)
    rotation_ = Quaternion::identity();
    
    // First: horizontal rotation around world Y (45 degrees to the left to see from front-right)
    // Negative angle rotates camera to the left (counter-clockwise from top view)
    Quaternion y_rot = Quaternion::from_axis_angle(Vector3D(0, 1, 0), 45.0f * DEG_TO_RAD);
    rotation_ = y_rot * rotation_;
    
    // Second: vertical rotation around local X (30 degrees looking down)
    Vector3D right = rotation_.rotate_vector(Vector3D(1, 0, 0));
    Quaternion x_rot = Quaternion::from_axis_angle(right, -30.0f * DEG_TO_RAD);
    rotation_ = x_rot * rotation_;
    
    rotation_ = rotation_.normalized();
    
    // Also set the spherical angles to match
    horizontal_angle_ = 45.0f * DEG_TO_RAD;
    vertical_angle_ = 30.0f * DEG_TO_RAD;
    
    // Derive position from view quaternion
    update_position_from_view_quat();
    
    
    fov_degrees_ = 45.0f;
    ortho_size_ = 10.0f;
    near_plane_ = 0.1f;
    far_plane_ = 100000.0f;  // Much larger far plane for large scenes
    aspect_ratio_ = 16.0f / 9.0f;
    
    mark_view_dirty();
    mark_projection_dirty();
}

// Legacy sync methods removed - using pure Camera3D system

void Camera3D::update_position_from_orbit() {
    if (use_turntable_) {
        // In turntable mode, position is derived from view quaternion
        // This preserves the exact camera orientation during zoom
        update_position_from_view_quat();
    } else {
        // Calculate position using spherical coordinates
        // This naturally handles rotation past the poles
        position_ = CameraUtils::calculate_orbit_position(orbit_target_, distance_, horizontal_angle_, vertical_angle_);
        
        // Calculate forward vector
        Vector3D forward = (orbit_target_ - position_).normalized();
        
        // For continuous rotation, we need a consistent right vector
        // Use world up crossed with forward, unless we're looking straight up/down
        Vector3D world_up = {0, 1, 0};
        Vector3D right;
        
        if (std::abs(forward.y) > 0.999f) {
            // Looking straight up or down, use a different approach
            right = Vector3D(1, 0, 0);
        } else {
            right = world_up.cross(forward).normalized();
        }
        
        // Calculate actual up from right and forward
        Vector3D up = forward.cross(right).normalized();
        
        // Build rotation matrix manually
        // This allows the view to naturally invert when going upside down
        rotation_ = Quaternion::look_rotation(forward, up);
        
        target_ = orbit_target_;
    }
}

void Camera3D::update_orbit_from_position() {
    Vector3D to_camera = position_ - target_;  // Vector from target to camera
    distance_ = to_camera.length();
    CameraUtils::cartesian_to_spherical(to_camera, distance_, horizontal_angle_, vertical_angle_);
    vertical_angle_ = PI * 0.5f - vertical_angle_; // Convert to elevation angle
    
    
    // For turntable mode, build quaternion from scratch without look_rotation
    if (use_turntable_) {
        // Build the view quaternion purely from rotations (no look_rotation)
        rotation_ = Quaternion::identity();
        
        // First apply the horizontal rotation (around world Y)
        Quaternion y_rot = Quaternion::from_axis_angle(Vector3D(0, 1, 0), horizontal_angle_);
        rotation_ = y_rot * rotation_;
        
        // Then apply the vertical rotation (around local X)
        Vector3D right = rotation_.rotate_vector(Vector3D(1, 0, 0));
        Quaternion x_rot = Quaternion::from_axis_angle(right, -vertical_angle_);
        rotation_ = x_rot * rotation_;
        
        rotation_ = rotation_.normalized();
    }
}

void Camera3D::update_position_from_view_quat() {
    // Turntable mode: derive position from view quaternion
    // The view quaternion represents the camera's orientation
    // The camera looks down its local -Z axis (forward in OpenGL)
    
    // Get the view direction (camera looks down -Z in its local space)
    Vector3D view_forward = rotation_.rotate_vector(Vector3D(0, 0, -1));
    
    // Position is orbit_target minus the forward direction scaled by distance
    // (camera is behind where it's looking)
    position_ = orbit_target_ - view_forward * distance_;
    
    
    // Target stays at orbit target
    target_ = orbit_target_;
}

void Camera3D::apply_orbit_constraints() {
    if (!orbit_constraints_enabled_) return;
    
    distance_ = std::max(min_distance_, std::min(max_distance_, distance_));
    // Don't constrain vertical angle - allow full rotation
}

void Camera3D::update_cached_matrices() const {
    if (view_matrix_dirty_) {
        if (use_turntable_ && navigation_mode_ == NavigationMode::Orbit) {
            // Build view matrix directly from quaternion for turntable mode
            // The view matrix transforms from world to camera space
            
            // Rotation part: conjugate of view quaternion to invert the rotation
            Matrix4x4 rot_matrix = Matrix4x4::rotation(rotation_.conjugate());
            
            // Translation part: negative position in world space
            Matrix4x4 trans_matrix = Matrix4x4::translation(Vector3D(-position_.x, -position_.y, -position_.z));
            
            // Combine: first translate, then rotate (standard view matrix order)
            cached_view_matrix_ = rot_matrix * trans_matrix;
        } else {
            cached_view_matrix_ = Matrix4x4::look_at(position_, target_, up_vector_);
        }
        view_matrix_dirty_ = false;
    }
    
    if (projection_matrix_dirty_) {
        if (projection_type_ == ProjectionType::Perspective) {
            cached_projection_matrix_ = Matrix4x4::perspective(fov_degrees_, aspect_ratio_, near_plane_, far_plane_);
        } else {
            float half_size = ortho_size_ * 0.5f;
            float left = -half_size * aspect_ratio_;
            float right = half_size * aspect_ratio_;
            float bottom = -half_size;
            float top = half_size;
            cached_projection_matrix_ = Matrix4x4::orthographic(left, right, bottom, top, near_plane_, far_plane_);
        }
        projection_matrix_dirty_ = false;
    }
}

// CameraUtils implementation

Vector3D CameraUtils::spherical_to_cartesian(float radius, float theta, float phi) {
    float sin_phi = std::sin(phi);
    return {
        radius * sin_phi * std::cos(theta),
        radius * std::cos(phi),
        radius * sin_phi * std::sin(theta)
    };
}

void CameraUtils::cartesian_to_spherical(const Vector3D& cartesian, float& radius, float& theta, float& phi) {
    radius = cartesian.length();
    if (radius < EPSILON) {
        theta = phi = 0;
        return;
    }
    
    theta = std::atan2(cartesian.z, cartesian.x);
    phi = std::acos(cartesian.y / radius);
}

Vector3D CameraUtils::calculate_orbit_position(const Vector3D& target, float distance, float horizontal_angle, float vertical_angle) {
    return target + spherical_to_cartesian(distance, horizontal_angle, PI * 0.5f - vertical_angle);
}

void CameraUtils::calculate_orbit_angles(const Vector3D& position, const Vector3D& target, float& horizontal_angle, float& vertical_angle) {
    Vector3D offset = position - target;
    float distance;
    cartesian_to_spherical(offset, distance, horizontal_angle, vertical_angle);
    vertical_angle = PI * 0.5f - vertical_angle;
}

} // namespace voxel_canvas