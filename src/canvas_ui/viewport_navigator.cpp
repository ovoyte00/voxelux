#include "canvas_ui/viewport_navigator.h"
#include "canvas_ui/camera_3d.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace voxel_canvas;

namespace voxelux::canvas_ui {

ViewportNavigator::ViewportNavigator() 
    : camera_(nullptr)
    , viewport_width_(800)
    , viewport_height_(600) {
}

ViewportNavigator::~ViewportNavigator() = default;

void ViewportNavigator::initialize(Camera3D* camera, int viewport_width, int viewport_height) {
    camera_ = camera;
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;
    
    if (camera_) {
        // Initialize state from camera
        Vector3D pos = camera_->get_position();
        Vector3D target = camera_->get_target();
        Vector3D orbit = camera_->get_orbit_target();
        
        state_.camera_position = glm::vec3(pos.x, pos.y, pos.z);
        state_.camera_target = glm::vec3(target.x, target.y, target.z);
        state_.orbit_center = glm::vec3(orbit.x, orbit.y, orbit.z);
        state_.fov = camera_->get_field_of_view();
        
        // Calculate initial rotation from view direction
        glm::vec3 forward = glm::normalize(state_.camera_target - state_.camera_position);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::cross(right, forward);
        glm::mat3 rotation_matrix(right, up, -forward);
        state_.camera_rotation = glm::quat_cast(rotation_matrix);
    }
}

void ViewportNavigator::set_viewport_size(int width, int height) {
    viewport_width_ = width;
    viewport_height_ = height;
}

void ViewportNavigator::set_ui_scale(float scale) {
    ui_scale_ = scale;
}

float ViewportNavigator::calculate_depth_factor(const glm::vec3& point) const {
    // Project point through view-projection matrix to get depth
    glm::mat4 view = get_view_matrix();
    glm::mat4 proj = get_projection_matrix();
    glm::mat4 view_proj = proj * view;
    
    glm::vec4 projected = view_proj * glm::vec4(point, 1.0f);
    
    // Get depth in view space
    float z_depth = -projected.z;
    
    // Clamp to avoid near-zero values
    if (std::abs(z_depth) < 0.001f) {
        z_depth = (z_depth < 0) ? -0.001f : 0.001f;
    }
    
    // For perspective, depth factor is the distance
    // For orthographic, use a constant factor based on ortho scale
    if (state_.is_orthographic) {
        return state_.ortho_scale * 2.0f;
    } else {
        return std::abs(z_depth);
    }
}

glm::vec3 ViewportNavigator::screen_to_world_delta(const glm::vec2& screen_delta, float z_depth) const {
    // Industry-standard approach: Transform screen delta through inverse matrices
    // Step 1: Normalize screen coordinates to [-1, 1] range (NDC)
    float normalized_x = 2.0f * screen_delta.x * z_depth / viewport_width_;
    float normalized_y = 2.0f * screen_delta.y * z_depth / viewport_height_;
    
    // Step 2: Get projection and view matrices
    glm::mat4 projection = get_projection_matrix();
    glm::mat4 view = get_view_matrix();
    
    // Step 3: Compute inverse of projection matrix for unprojection
    glm::mat4 proj_inv = glm::inverse(projection);
    
    // Step 4: Create a vector in clip space (using normalized coords)
    // We use w=0 because we're transforming a direction, not a position
    glm::vec4 clip_delta(normalized_x, normalized_y, 0.0f, 0.0f);
    
    // Step 5: Transform from clip space to view space
    glm::vec4 view_delta = proj_inv * clip_delta;
    
    // Step 6: Transform from view space to world space
    glm::mat4 view_inv = glm::inverse(view);
    glm::vec4 world_delta_4 = view_inv * view_delta;
    
    // Step 7: Extract the 3D delta
    glm::vec3 world_delta(world_delta_4.x, world_delta_4.y, world_delta_4.z);
    
    // Step 8: Negate for drag behavior (drag right = world moves left)
    return -world_delta;
}

void ViewportNavigator::start_pan(const glm::vec2& mouse_pos) {
    state_.mode = NavigationMode::PAN;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
    
    // Clear orbit momentum when starting pan
    state_.orbit_momentum = glm::vec2(0.0f);
    state_.zoom_momentum = 0.0f;
    // Reset smoothing for new gesture
    state_.pan_smooth = glm::vec2(0.0f);
}

void ViewportNavigator::update_pan(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * pan_sensitivity_ / ui_scale_;
    
    // For very small movements, apply directly without momentum
    if (glm::length(scaled_delta) < 2.0f) {
        // Direct application for fine control
        apply_pan(scaled_delta);
    } else {
        // Update momentum for larger movements (smooth out movement)
        state_.pan_momentum = state_.pan_momentum * 0.7f + scaled_delta * 0.3f;
        apply_pan(state_.pan_momentum);
    }
}

void ViewportNavigator::update_pan_delta(const glm::vec2& delta, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * pan_sensitivity_ / ui_scale_;
    
    // Hybrid smoothing approach (same as orbit)
    static glm::vec2 target_velocity(0.0f);
    static bool was_panning = false;
    
    if (!was_panning) {
        target_velocity = glm::vec2(0.0f);
        was_panning = true;
    }
    
    // Accumulate with decay
    target_velocity = target_velocity * 0.4f + scaled_delta * 0.6f;
    
    // Smooth interpolation
    const float smoothing_factor = 0.6f;
    state_.pan_smooth = glm::mix(state_.pan_smooth, target_velocity, smoothing_factor);
    
    // Apply the smoothed delta
    apply_pan(state_.pan_smooth);
    
    if (state_.mode != NavigationMode::PAN) {
        was_panning = false;
        target_velocity = glm::vec2(0.0f);
    }
}

void ViewportNavigator::end_pan() {
    if (state_.mode == NavigationMode::PAN) {
        state_.mode = NavigationMode::NONE;
        // Disable momentum for pan to prevent drift
        state_.pan_momentum = glm::vec2(0.0f);
        state_.has_momentum = false;
        // Clear smoothing when ending gesture
        state_.pan_smooth = glm::vec2(0.0f);
    }
}

void ViewportNavigator::apply_pan(const glm::vec2& delta) {
    // Calculate depth factor at orbit center
    float depth = calculate_depth_factor(state_.orbit_center);
    
    // Convert screen delta to world space movement
    glm::vec3 world_delta = screen_to_world_delta(delta, depth);
    
    // Update both camera position and targets
    // We SUBTRACT because when we drag right, we want the scene to move left (camera moves right relative to scene)
    // This creates the "drag" metaphor where you're dragging the world
    state_.camera_position -= world_delta;
    state_.camera_target -= world_delta;
    state_.orbit_center -= world_delta;
    
    // Sync with camera
    sync_camera_state();
}

void ViewportNavigator::start_orbit(const glm::vec2& mouse_pos) {
    state_.mode = NavigationMode::ORBIT;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
    
    // Clear pan momentum when starting orbit
    state_.pan_momentum = glm::vec2(0.0f);
    state_.zoom_momentum = 0.0f;
    // Reset smoothing for new gesture
    state_.orbit_smooth = glm::vec2(0.0f);
}

void ViewportNavigator::update_orbit(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::ORBIT) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    delta *= orbit_sensitivity_mouse_ / ui_scale_;
    
    // Update momentum
    state_.orbit_momentum = state_.orbit_momentum * 0.8f + delta * 0.2f;
    
    // Apply orbit
    apply_orbit(state_.orbit_momentum);
}

void ViewportNavigator::update_orbit_delta(const glm::vec2& delta, float delta_time, bool is_smart_mouse) {
    if (state_.mode != NavigationMode::ORBIT) return;
    
    // Choose sensitivity based on device type
    float sensitivity = is_smart_mouse ? orbit_sensitivity_smartmouse_ : orbit_sensitivity_trackpad_;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * sensitivity / ui_scale_;
    
    // Hybrid approach: accumulate input for continuous movement, then interpolate
    // This gives us both responsiveness and smoothness
    static glm::vec2 target_velocity(0.0f);
    static bool was_orbiting = false;
    
    // Reset target if just started orbiting
    if (!was_orbiting) {
        target_velocity = glm::vec2(0.0f);
        was_orbiting = true;
    }
    
    // Accumulate the new delta into our target with higher weight for new input
    target_velocity = target_velocity * 0.4f + scaled_delta * 0.6f;  // More weight on new input
    
    // Smoothly interpolate current velocity toward target
    // Higher factor = more responsive, lower = smoother
    const float smoothing_factor = 0.4f;  // More smoothing for smoother feel
    state_.orbit_smooth = glm::mix(state_.orbit_smooth, target_velocity, smoothing_factor);
    
    // Apply the smoothed delta
    apply_orbit(state_.orbit_smooth);
    
    // Track state for next frame
    if (state_.mode != NavigationMode::ORBIT) {
        was_orbiting = false;
        target_velocity = glm::vec2(0.0f);
    }
}

void ViewportNavigator::end_orbit() {
    if (state_.mode == NavigationMode::ORBIT) {
        state_.mode = NavigationMode::NONE;
        state_.has_momentum = true;
        // Clear smoothing when ending gesture
        state_.orbit_smooth = glm::vec2(0.0f);
    }
}

void ViewportNavigator::apply_orbit(const glm::vec2& delta) {
    // Industry-standard arcball implementation with original approach
    
    // Step 1: Convert mouse movement to spherical rotation
    // Sensitivity is already applied before calling this function
    float horizontal_angle = -delta.x * 0.01f;  // Convert to radians
    float vertical_angle = -delta.y * 0.01f;    // Convert to radians
    
    // Step 2: Get current camera orbit radius
    glm::vec3 cam_offset = state_.camera_position - state_.orbit_center;
    float orbit_radius = glm::length(cam_offset);
    
    if (orbit_radius < 0.0001f) return; // Prevent division by zero
    
    // Step 3: Convert to spherical coordinates for manipulation
    // Using standard spherical coordinate system: (r, theta, phi)
    glm::vec3 normalized_offset = cam_offset / orbit_radius;
    
    // Calculate current spherical angles
    float current_phi = std::acos(glm::clamp(normalized_offset.y, -1.0f, 1.0f));
    float current_theta = std::atan2(normalized_offset.z, normalized_offset.x);
    
    if (use_trackball_) {
        // Arcball mode: Full 3D rotation using quaternions
        // Both axes are applied together for diagonal movement
        
        // Calculate view-aligned axes for rotation
        glm::vec3 view_forward = glm::normalize(cam_offset);
        glm::vec3 world_up = glm::vec3(0, 1, 0);
        glm::vec3 view_right = glm::normalize(glm::cross(world_up, view_forward));
        
        // Handle special case when looking straight up/down
        if (glm::length(view_right) < 0.001f) {
            view_right = glm::vec3(1, 0, 0);
        }
        
        // Create combined rotation from both axes simultaneously
        // Horizontal rotation around world Y axis
        glm::quat horizontal_rotation = glm::angleAxis(horizontal_angle, world_up);
        
        // Vertical rotation around view-aligned right axis
        glm::quat vertical_rotation = glm::angleAxis(vertical_angle, view_right);
        
        // Combine rotations - order matters for intuitive control
        glm::quat combined_rotation = horizontal_rotation * vertical_rotation;
        
        // Apply combined rotation to camera position
        glm::vec3 new_offset = combined_rotation * cam_offset;
        state_.camera_position = state_.orbit_center + new_offset;
        
        // Store rotation for other uses
        state_.camera_rotation = combined_rotation * state_.camera_rotation;
    } else {
        // Turntable mode: Constrained rotation to prevent disorientation
        
        // Update spherical coordinates
        float new_theta = current_theta + horizontal_angle;
        float new_phi = current_phi + vertical_angle;
        
        // Clamp vertical angle to prevent flipping (5 to 175 degrees)
        const float min_phi = glm::radians(5.0f);
        const float max_phi = glm::radians(175.0f);
        new_phi = glm::clamp(new_phi, min_phi, max_phi);
        
        // Convert back to Cartesian coordinates
        glm::vec3 new_position;
        new_position.x = orbit_radius * std::sin(new_phi) * std::cos(new_theta);
        new_position.y = orbit_radius * std::cos(new_phi);
        new_position.z = orbit_radius * std::sin(new_phi) * std::sin(new_theta);
        
        state_.camera_position = state_.orbit_center + new_position;
    }
    
    // Always look at orbit center
    state_.camera_target = state_.orbit_center;
    
    // Update the actual camera
    sync_camera_state();
}

void ViewportNavigator::zoom(float delta, const glm::vec2& mouse_pos) {
    // Apply sensitivity based on input device characteristics with UI scale compensation
    float sensitivity = zoom_sensitivity_ / ui_scale_;
    
    // The accumulated deltas from trackpad/smart mouse are typically 5-20
    // Mouse wheel sends larger discrete values (> 20)
    // We're now accumulating events, so deltas will be larger
    
    if (std::abs(delta) < 5.0f) {
        // Small accumulated values = slow gesture
        sensitivity *= 0.02f;
    } else if (std::abs(delta) <= 20.0f) {
        // Medium accumulated values = normal gesture speed
        sensitivity *= 0.01f;
    } else {
        // Large values = fast gesture or mouse wheel
        sensitivity *= 0.02f;  // Scale appropriately for larger accumulated values
    }
    
    float scaled_delta = delta * sensitivity;
    
    // Debug what's happening
    static int zoom_debug = 0;
    if (++zoom_debug % 10 == 0) {
        std::cout << "[Navigator] Zoom delta: " << delta << ", scaled: " << scaled_delta 
                  << ", mouse: (" << mouse_pos.x << ", " << mouse_pos.y << ")" << std::endl;
    }
    
    // Use simple zoom behavior - just move camera closer/farther from orbit center
    apply_zoom(scaled_delta);
}

void ViewportNavigator::apply_zoom(float delta) {
    // Industry-standard exponential zoom implementation
    
    if (state_.is_orthographic) {
        // Orthographic zoom: Scale the view bounds
        // Use exponential scaling for smooth, consistent feel
        float scale_factor = std::exp(-delta);
        state_.ortho_scale *= scale_factor;
        
        // Apply reasonable limits
        const float min_scale = 0.01f;
        const float max_scale = 1000.0f;
        state_.ortho_scale = glm::clamp(state_.ortho_scale, min_scale, max_scale);
    } else {
        // Perspective zoom: Dolly camera along view vector
        
        // Step 1: Calculate current distance from orbit center
        glm::vec3 view_vector = state_.camera_position - state_.orbit_center;
        float current_distance = glm::length(view_vector);
        
        // Prevent issues with zero distance
        if (current_distance < 0.001f) {
            current_distance = 0.001f;
            view_vector = glm::vec3(0, 0, 1);
        } else {
            view_vector = view_vector / current_distance; // Normalize
        }
        
        // Step 2: Apply exponential zoom for smooth feel
        // Exponential provides consistent zoom speed regardless of distance
        float zoom_factor = std::exp(-delta);
        float new_distance = current_distance * zoom_factor;
        
        // Step 3: Apply constraints to prevent extreme values
        const float min_distance = 0.1f;
        const float max_distance = 1000.0f;
        new_distance = glm::clamp(new_distance, min_distance, max_distance);
        
        // Step 4: Update camera position
        state_.camera_position = state_.orbit_center + view_vector * new_distance;
    }
    
    // Update the actual camera
    sync_camera_state();
}


void ViewportNavigator::update_momentum(float delta_time) {
    if (!state_.has_momentum) return;
    
    // Decay momentum
    state_.pan_momentum *= momentum_decay_;
    state_.orbit_momentum *= momentum_decay_;
    state_.zoom_momentum *= momentum_decay_;
    
    // Apply remaining momentum based on last mode
    float momentum_threshold = 0.001f;
    
    if (glm::length(state_.pan_momentum) > momentum_threshold) {
        apply_pan(state_.pan_momentum);
    } else {
        state_.pan_momentum = glm::vec2(0.0f);
    }
    
    if (glm::length(state_.orbit_momentum) > momentum_threshold) {
        apply_orbit(state_.orbit_momentum);
    } else {
        state_.orbit_momentum = glm::vec2(0.0f);
    }
    
    if (std::abs(state_.zoom_momentum) > momentum_threshold) {
        apply_zoom(state_.zoom_momentum);
    } else {
        state_.zoom_momentum = 0.0f;
    }
    
    // Stop momentum if all values are below threshold
    if (glm::length(state_.pan_momentum) <= momentum_threshold &&
        glm::length(state_.orbit_momentum) <= momentum_threshold &&
        std::abs(state_.zoom_momentum) <= momentum_threshold) {
        state_.has_momentum = false;
        clear_momentum();
    }
}

void ViewportNavigator::clear_momentum() {
    state_.pan_momentum = glm::vec2(0.0f);
    state_.orbit_momentum = glm::vec2(0.0f);
    state_.zoom_momentum = 0.0f;
    state_.has_momentum = false;
}

void ViewportNavigator::sync_camera_state() {
    if (!camera_) return;
    
    // Debug camera state changes
    static glm::vec3 last_pos = state_.camera_position;
    static glm::vec3 last_target = state_.camera_target;
    static int sync_count = 0;
    
    if (++sync_count % 100 == 0 || glm::distance(last_pos, state_.camera_position) > 1.0f) {
        glm::vec3 view_dir = glm::normalize(state_.camera_target - state_.camera_position);
        std::cout << "[Navigator] Camera sync - pos: (" << state_.camera_position.x << ", " 
                  << state_.camera_position.y << ", " << state_.camera_position.z 
                  << "), view_dir: (" << view_dir.x << ", " << view_dir.y << ", " << view_dir.z << ")"
                  << ", mode: " << static_cast<int>(state_.mode) << std::endl;
        last_pos = state_.camera_position;
        last_target = state_.camera_target;
    }
    
    // Update camera with new state - convert glm::vec3 to Vector3D
    camera_->set_position(Vector3D(state_.camera_position.x, state_.camera_position.y, state_.camera_position.z));
    camera_->set_target(Vector3D(state_.camera_target.x, state_.camera_target.y, state_.camera_target.z));
    camera_->set_orbit_target(Vector3D(state_.orbit_center.x, state_.orbit_center.y, state_.orbit_center.z));
    
    // Camera matrices are updated automatically in Camera3D
}

glm::mat4 ViewportNavigator::get_view_matrix() const {
    return glm::lookAt(state_.camera_position, state_.camera_target, glm::vec3(0, 1, 0));
}

glm::mat4 ViewportNavigator::get_projection_matrix() const {
    float aspect = static_cast<float>(viewport_width_) / viewport_height_;
    
    if (state_.is_orthographic) {
        float half_width = state_.ortho_scale * aspect;
        float half_height = state_.ortho_scale;
        return glm::ortho(-half_width, half_width, -half_height, half_height, 0.1f, 1000.0f);
    } else {
        return glm::perspective(glm::radians(state_.fov), aspect, 0.1f, 1000.0f);
    }
}

} // namespace voxelux::canvas_ui