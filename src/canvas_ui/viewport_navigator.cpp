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
    // Build view matrix and extract camera vectors
    glm::mat4 view = get_view_matrix();
    
    // The view matrix transforms from world to camera space
    // Its transpose (or inverse for orthonormal matrices) gives us camera to world
    // The columns of the inverse are the camera's basis vectors in world space
    glm::mat4 view_inv = glm::inverse(view);
    
    // Extract camera basis vectors
    // Column 0: Right vector in world space
    // Column 1: Up vector in world space
    // Column 2: Back vector in world space (-forward)
    glm::vec3 cam_right = glm::vec3(view_inv[0]);
    glm::vec3 cam_up = glm::vec3(view_inv[1]);
    
    // Debug output
    static int debug_counter = 0;
    if (debug_counter++ % 30 == 0 && glm::length(screen_delta) > 0.1f) {
        glm::vec3 cam_back = glm::vec3(view_inv[2]);
        glm::vec3 forward = glm::normalize(state_.camera_target - state_.camera_position);
        std::cout << "\n[screen_to_world] Screen delta: (" << screen_delta.x << ", " << screen_delta.y << ")" << std::endl;
        std::cout << "  Camera pos: (" << state_.camera_position.x << ", " << state_.camera_position.y << ", " << state_.camera_position.z << ")" << std::endl;
        std::cout << "  Camera target: (" << state_.camera_target.x << ", " << state_.camera_target.y << ", " << state_.camera_target.z << ")" << std::endl;
        std::cout << "  Forward (from pos->target): (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;
        std::cout << "  Camera right (from matrix): (" << cam_right.x << ", " << cam_right.y << ", " << cam_right.z << ")" << std::endl;
        std::cout << "  Camera up (from matrix): (" << cam_up.x << ", " << cam_up.y << ", " << cam_up.z << ")" << std::endl;
        std::cout << "  Camera back (from matrix): (" << cam_back.x << ", " << cam_back.y << ", " << cam_back.z << ")" << std::endl;
    }
    
    // Calculate the scale factor based on FOV and depth
    float aspect = static_cast<float>(viewport_width_) / viewport_height_;
    float fov_rad = glm::radians(state_.fov);
    float scale_factor = 2.0f * z_depth * std::tan(fov_rad * 0.5f);
    
    // Convert screen pixels to world units
    // Trackpad gives us: +X = right, +Y = up (already in correct orientation)
    // We want: pan right = move right, pan up = move up
    float world_dx = (screen_delta.x / viewport_width_) * scale_factor * aspect;
    float world_dy = (screen_delta.y / viewport_height_) * scale_factor;  // Don't negate - trackpad Y is already correct
    
    // Apply movement along camera axes
    glm::vec3 world_delta = cam_right * world_dx + cam_up * world_dy;
    
    return world_delta;
}

void ViewportNavigator::start_pan(const glm::vec2& mouse_pos) {
    state_.mode = NavigationMode::PAN;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
    
    // Clear orbit momentum when starting pan
    state_.orbit_momentum = glm::vec2(0.0f);
    state_.zoom_momentum = 0.0f;
}

void ViewportNavigator::update_pan(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity
    delta *= pan_sensitivity_;
    
    // For very small movements, apply directly without momentum
    if (glm::length(delta) < 2.0f) {
        // Direct application for fine control
        apply_pan(delta);
    } else {
        // Update momentum for larger movements (smooth out movement)
        state_.pan_momentum = state_.pan_momentum * 0.7f + delta * 0.3f;
        apply_pan(state_.pan_momentum);
    }
}

void ViewportNavigator::update_pan_delta(const glm::vec2& delta, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Apply sensitivity directly to delta
    glm::vec2 scaled_delta = delta * pan_sensitivity_;
    
    // Debug output
    static int debug_counter = 0;
    if (debug_counter++ % 30 == 0 && glm::length(delta) > 0.1f) {
        std::cout << "[Pan Delta] Input: (" << delta.x << ", " << delta.y 
                  << ") Scaled: (" << scaled_delta.x << ", " << scaled_delta.y << ")" << std::endl;
    }
    
    // For trackpad, we get the delta directly, so just apply it
    apply_pan(scaled_delta);
}

void ViewportNavigator::end_pan() {
    if (state_.mode == NavigationMode::PAN) {
        state_.mode = NavigationMode::NONE;
        state_.has_momentum = true;
    }
}

void ViewportNavigator::apply_pan(const glm::vec2& delta) {
    // Calculate depth factor at orbit center
    float depth = calculate_depth_factor(state_.orbit_center);
    
    // Convert screen delta to world space movement
    glm::vec3 world_delta = screen_to_world_delta(delta, depth);
    
    // Debug output
    static int debug_counter = 0;
    if (debug_counter++ % 30 == 0 && glm::length(delta) > 0.1f) {
        glm::vec3 forward = glm::normalize(state_.camera_target - state_.camera_position);
        std::cout << "[Apply Pan] Screen delta: (" << delta.x << ", " << delta.y << ")" << std::endl;
        std::cout << "  Depth: " << depth << std::endl;
        std::cout << "  World delta: (" << world_delta.x << ", " << world_delta.y << ", " << world_delta.z << ")" << std::endl;
        std::cout << "  Camera forward: (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;
        std::cout << "  Camera pos before: (" << state_.camera_position.x << ", " << state_.camera_position.y << ", " << state_.camera_position.z << ")" << std::endl;
    }
    
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
}

void ViewportNavigator::update_orbit(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::ORBIT) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity
    delta *= orbit_sensitivity_;
    
    // Update momentum
    state_.orbit_momentum = state_.orbit_momentum * 0.8f + delta * 0.2f;
    
    // Apply orbit
    apply_orbit(state_.orbit_momentum);
}

void ViewportNavigator::end_orbit() {
    if (state_.mode == NavigationMode::ORBIT) {
        state_.mode = NavigationMode::NONE;
        state_.has_momentum = true;
    }
}

void ViewportNavigator::apply_orbit(const glm::vec2& delta) {
    // Convert pixel movement to rotation angles
    float yaw = -delta.x * 0.01f;   // Horizontal rotation
    float pitch = -delta.y * 0.01f; // Vertical rotation
    
    // Get camera direction relative to orbit center
    glm::vec3 offset = state_.camera_position - state_.orbit_center;
    float distance = glm::length(offset);
    
    if (distance < 0.001f) return; // Too close to orbit center
    
    if (use_trackball_) {
        // Trackball rotation - full 3D rotation
        glm::quat yaw_quat = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
        
        // Calculate right vector for pitch axis
        glm::vec3 forward = glm::normalize(offset);
        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
        glm::quat pitch_quat = glm::angleAxis(pitch, right);
        
        // Combine rotations
        glm::quat rotation = yaw_quat * pitch_quat;
        
        // Apply rotation to camera offset
        offset = rotation * offset;
        
        // Update camera position
        state_.camera_position = state_.orbit_center + offset;
        
        // Update camera rotation
        state_.camera_rotation = rotation * state_.camera_rotation;
    } else {
        // Turntable rotation - constrained to avoid gimbal lock
        
        // Yaw around world Y axis
        glm::quat yaw_quat = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
        offset = yaw_quat * offset;
        
        // Calculate right vector for pitch
        glm::vec3 forward = glm::normalize(offset);
        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
        
        // Limit pitch to avoid flipping
        glm::vec3 new_forward = glm::rotate(glm::angleAxis(pitch, right), forward);
        float dot_up = glm::dot(new_forward, glm::vec3(0, 1, 0));
        
        // Prevent looking straight up or down (gimbal lock)
        if (std::abs(dot_up) < 0.99f) {
            glm::quat pitch_quat = glm::angleAxis(pitch, right);
            offset = pitch_quat * offset;
        }
        
        // Update camera position
        state_.camera_position = state_.orbit_center + offset;
    }
    
    // Update camera target to maintain orbit center
    state_.camera_target = state_.orbit_center;
    
    // Sync with camera
    sync_camera_state();
}

void ViewportNavigator::zoom(float delta, const glm::vec2& mouse_pos) {
    state_.zoom_momentum = delta * zoom_sensitivity_;
    apply_zoom(state_.zoom_momentum);
}

void ViewportNavigator::apply_zoom(float delta) {
    if (state_.is_orthographic) {
        // For orthographic, adjust the scale
        state_.ortho_scale *= (1.0f - delta);
        state_.ortho_scale = std::max(0.1f, std::min(1000.0f, state_.ortho_scale));
    } else {
        // For perspective, move camera along view direction
        glm::vec3 direction = glm::normalize(state_.camera_position - state_.orbit_center);
        float distance = glm::length(state_.camera_position - state_.orbit_center);
        
        // Scale movement by current distance for consistent feel
        float move_amount = delta * distance;
        
        // Prevent getting too close or too far
        float new_distance = distance - move_amount;
        new_distance = std::max(0.1f, std::min(1000.0f, new_distance));
        
        // Update camera position
        state_.camera_position = state_.orbit_center + direction * new_distance;
    }
    
    // Sync with camera
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