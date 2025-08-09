/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional 3D Camera System
 * Advanced camera system for 3D viewport navigation with matrix transformations
 */

#pragma once

#include "canvas_core.h"
#include <array>
#include <cmath>

namespace voxel_canvas {

// Forward declarations
struct Quaternion;
struct Vector3D;
struct Matrix4x4;

/**
 * 3D Vector for camera calculations
 */
struct Vector3D {
    float x, y, z;
    
    Vector3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    // Vector operations
    Vector3D operator+(const Vector3D& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3D operator-(const Vector3D& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3D operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vector3D operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
    Vector3D& operator+=(const Vector3D& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3D& operator-=(const Vector3D& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3D& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    
    // Vector math
    float dot(const Vector3D& other) const { return x * other.x + y * other.y + z * other.z; }
    Vector3D cross(const Vector3D& other) const { 
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x}; 
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length_squared() const { return x * x + y * y + z * z; }
    Vector3D normalized() const { 
        float len = length(); 
        return len > 0 ? *this / len : Vector3D(0, 0, 0); 
    }
    void normalize() { *this = normalized(); }
    
    // Utility
    bool is_zero() const { return std::abs(x) < 1e-6f && std::abs(y) < 1e-6f && std::abs(z) < 1e-6f; }
    
    // Common vectors
    static Vector3D zero() { return {0, 0, 0}; }
    static Vector3D up() { return {0, 1, 0}; }
    static Vector3D right() { return {1, 0, 0}; }
    static Vector3D forward() { return {0, 0, -1}; }  // Negative Z is forward in OpenGL
};

/**
 * Quaternion for smooth rotations
 */
struct Quaternion {
    float x, y, z, w;
    
    Quaternion(float x = 0, float y = 0, float z = 0, float w = 1) : x(x), y(y), z(z), w(w) {}
    Quaternion(const Vector3D& axis, float angle);  // From axis-angle
    Quaternion(float pitch, float yaw, float roll); // From Euler angles
    
    // Quaternion operations
    Quaternion operator*(const Quaternion& other) const;
    Quaternion& operator*=(const Quaternion& other) { *this = *this * other; return *this; }
    Quaternion conjugate() const { return {-x, -y, -z, w}; }
    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    Quaternion normalized() const;
    void normalize() { *this = normalized(); }
    
    // Conversions
    Vector3D to_euler() const; // Returns (pitch, yaw, roll)
    void from_euler(float pitch, float yaw, float roll);
    Vector3D rotate_vector(const Vector3D& vec) const;
    
    // Interpolation
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t);
    
    // Common quaternions
    static Quaternion identity() { return {0, 0, 0, 1}; }
    static Quaternion from_axis_angle(const Vector3D& axis, float angle);
    static Quaternion look_rotation(const Vector3D& forward, const Vector3D& up = Vector3D::up());
};

/**
 * 4x4 Matrix for transformations
 */
struct Matrix4x4 {
    std::array<std::array<float, 4>, 4> m;
    
    Matrix4x4();
    Matrix4x4(float diagonal);
    
    // Matrix operations
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Matrix4x4& operator*=(const Matrix4x4& other) { *this = *this * other; return *this; }
    Vector3D transform_point(const Vector3D& point) const;
    Vector3D transform_direction(const Vector3D& direction) const;
    
    // Transformations
    static Matrix4x4 identity();
    static Matrix4x4 translation(const Vector3D& offset);
    static Matrix4x4 rotation(const Quaternion& rotation);
    static Matrix4x4 scale(const Vector3D& scale);
    static Matrix4x4 scale(float uniform_scale);
    
    // View and projection matrices
    static Matrix4x4 look_at(const Vector3D& eye, const Vector3D& target, const Vector3D& up);
    static Matrix4x4 perspective(float fovy, float aspect, float near_plane, float far_plane);
    static Matrix4x4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
    
    // Utility
    Matrix4x4 inverse() const;
    Matrix4x4 transpose() const;
    float determinant() const;
    
    // Data access
    const float* data() const { return &m[0][0]; }
    float* data() { return &m[0][0]; }
    float& operator()(int row, int col) { return m[row][col]; }
    const float& operator()(int row, int col) const { return m[row][col]; }
};

/**
 * Professional 3D Camera System
 * Based on modern 3D application architecture with full matrix transformations
 */
class Camera3D {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };
    
    enum class NavigationMode {
        Orbit,    // Orbit around target point
        Fly,      // Free-flying camera
        Walk      // Ground-constrained movement
    };
    
    Camera3D();
    ~Camera3D() = default;
    
    // Camera positioning and orientation
    void set_position(const Vector3D& position);
    void set_target(const Vector3D& target);
    void set_up_vector(const Vector3D& up);
    void set_rotation(const Quaternion& rotation);
    
    const Vector3D& get_position() const { return position_; }
    const Vector3D& get_target() const { return target_; }
    const Vector3D& get_up_vector() const { return up_vector_; }
    const Quaternion& get_rotation() const { return rotation_; }
    
    // Camera directions
    Vector3D get_forward_vector() const;
    Vector3D get_right_vector() const;
    Vector3D get_actual_up_vector() const;
    
    // Distance and orbit controls
    void set_distance(float distance);
    float get_distance() const { return distance_; }
    void set_orbit_target(const Vector3D& target);
    const Vector3D& get_orbit_target() const { return orbit_target_; }
    
    // Projection settings
    void set_projection_type(ProjectionType type);
    ProjectionType get_projection_type() const { return projection_type_; }
    
    void set_field_of_view(float fov_degrees);
    float get_field_of_view() const { return fov_degrees_; }
    
    void set_orthographic_size(float size);
    float get_orthographic_size() const { return ortho_size_; }
    
    void set_near_plane(float near_plane);
    float get_near_plane() const { return near_plane_; }
    
    void set_far_plane(float far_plane);
    float get_far_plane() const { return far_plane_; }
    
    void set_aspect_ratio(float aspect);
    float get_aspect_ratio() const { return aspect_ratio_; }
    
    // Navigation mode
    void set_navigation_mode(NavigationMode mode);
    NavigationMode get_navigation_mode() const { return navigation_mode_; }
    
    // Matrix calculations
    Matrix4x4 get_view_matrix() const;
    Matrix4x4 get_projection_matrix() const;
    Matrix4x4 get_view_projection_matrix() const;
    Matrix4x4 get_inverse_view_matrix() const;
    Matrix4x4 get_inverse_projection_matrix() const;
    
    // Navigation operations
    void orbit_horizontal(float angle_radians);
    void orbit_vertical(float angle_radians);
    void pan(const Vector3D& delta);
    void zoom(float factor);
    void dolly(float distance);
    void rotate_around_target(float horizontal_angle, float vertical_angle);
    
    // Advanced navigation
    void move_forward(float distance);
    void move_right(float distance);
    void move_up(float distance);
    void look_at(const Vector3D& target, const Vector3D& up = Vector3D::up());
    void frame_bounds(const Vector3D& min_bounds, const Vector3D& max_bounds);
    
    // Screen space calculations
    Vector3D world_to_screen(const Vector3D& world_pos, const Rect2D& viewport) const;
    Vector3D screen_to_world(const Point2D& screen_pos, float depth, const Rect2D& viewport) const;
    Vector3D screen_to_world_direction(const Point2D& screen_pos, const Rect2D& viewport) const;
    
    // Utility functions
    void reset_to_default();
    void copy_from(const Camera3D& other);
    void interpolate_to(const Camera3D& target, float t);
    
    // Constraints
    void set_orbit_constraints(float min_distance, float max_distance, float min_vertical_angle = -89.0f, float max_vertical_angle = 89.0f);
    void enable_orbit_constraints(bool enable) { orbit_constraints_enabled_ = enable; }
    bool are_orbit_constraints_enabled() const { return orbit_constraints_enabled_; }
    
    // Smooth movement
    void set_smooth_factor(float factor) { smooth_factor_ = factor; }
    float get_smooth_factor() const { return smooth_factor_; }
    void enable_smooth_movement(bool enable) { smooth_movement_enabled_ = enable; }
    void update_smooth_movement(float delta_time);
    
    // No legacy compatibility needed - pure Camera3D system
    
private:
    // Core camera state (defaults for voxel editing)
    // Initialize to safe defaults to avoid undefined behavior
    Vector3D position_{0, 0, 0};
    Vector3D target_{0, 0, 0};
    Vector3D up_vector_{0, 1, 0};
    Quaternion rotation_;
    
    // Blender-style view quaternion for turntable mode
    Quaternion view_quat_ = Quaternion::identity();  // Start with identity
    bool use_turntable_ = true;  // Use Blender's turntable rotation
    
    // Orbit camera state
    Vector3D orbit_target_{0, 0, 0};
    float distance_ = 60.0f;  // Default distance to see ~20x20x20 voxel area
    float horizontal_angle_ = -45.0f * 3.14159f / 180.0f;  // -45° yaw (matches reset_to_default)
    float vertical_angle_ = 30.0f * 3.14159f / 180.0f;    // 30° pitch
    
    // Projection settings
    ProjectionType projection_type_ = ProjectionType::Perspective;
    float fov_degrees_ = 45.0f;  // Standard FOV for editing
    float ortho_size_ = 30.0f;   // Orthographic size for ~20 voxel view
    float near_plane_ = 0.1f;    // Close enough for detailed work
    float far_plane_ = 10000.0f; // Far enough for large worlds
    float aspect_ratio_ = 16.0f / 9.0f;
    
    // Navigation settings
    NavigationMode navigation_mode_ = NavigationMode::Orbit;
    
    // Constraints (voxel-friendly limits)
    bool orbit_constraints_enabled_ = true;
    float min_distance_ = 1.0f;      // Minimum 1 voxel distance
    float max_distance_ = 100000.0f; // Allow very far zoom for large worlds
    float min_vertical_angle_ = -89.0f;
    float max_vertical_angle_ = 89.0f;
    
    // Smooth movement
    bool smooth_movement_enabled_ = false;
    float smooth_factor_ = 0.1f;
    Vector3D target_position_;
    Quaternion target_rotation_;
    
    // Internal state
    mutable bool view_matrix_dirty_ = true;
    mutable bool projection_matrix_dirty_ = true;
    mutable Matrix4x4 cached_view_matrix_;
    mutable Matrix4x4 cached_projection_matrix_;
    
    // Helper methods
    void update_position_from_orbit();
    void update_orbit_from_position();
    void update_position_from_view_quat();  // Blender-style position update
    void apply_orbit_constraints();
    void mark_view_dirty() const { view_matrix_dirty_ = true; }
    void mark_projection_dirty() const { projection_matrix_dirty_ = true; }
    void update_cached_matrices() const;
};

/**
 * Camera utilities and helper functions
 */
class CameraUtils {
public:
    // Coordinate system conversions
    static Vector3D spherical_to_cartesian(float radius, float theta, float phi);
    static void cartesian_to_spherical(const Vector3D& cartesian, float& radius, float& theta, float& phi);
    
    // Camera calculations
    static float calculate_distance_to_fit_bounds(const Vector3D& bounds_size, float fov_degrees);
    static Vector3D calculate_orbit_position(const Vector3D& target, float distance, float horizontal_angle, float vertical_angle);
    static void calculate_orbit_angles(const Vector3D& position, const Vector3D& target, float& horizontal_angle, float& vertical_angle);
    
    // Frustum calculations
    static std::array<Vector3D, 8> get_frustum_corners(const Matrix4x4& inverse_view_projection);
    static bool is_point_in_frustum(const Vector3D& point, const Matrix4x4& view_projection);
    static bool is_sphere_in_frustum(const Vector3D& center, float radius, const Matrix4x4& view_projection);
    
    // Screen space utilities
    static Point2D world_to_screen_point(const Vector3D& world_pos, const Matrix4x4& view_projection, const Rect2D& viewport);
    static Vector3D screen_to_world_ray(const Point2D& screen_pos, const Matrix4x4& inverse_view_projection, const Rect2D& viewport);
    
    // Animation and interpolation
    static Camera3D interpolate_cameras(const Camera3D& from, const Camera3D& to, float t);
    static Quaternion shortest_arc_rotation(const Vector3D& from, const Vector3D& to);
    
private:
    CameraUtils() = default;
};

} // namespace voxel_canvas