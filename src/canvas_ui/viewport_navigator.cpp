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
    // For pan, we need the distance from camera to the point
    float distance = glm::length(point - state_.camera_position);
    
    
    // Clamp to avoid near-zero values
    if (distance < 1.0f) {
        distance = 1.0f;
    }
    
    // For perspective, depth factor is the distance
    // For orthographic, use a constant factor based on ortho scale
    if (state_.is_orthographic) {
        return state_.ortho_scale * 2.0f;
    } else {
        return distance;
    }
}



void ViewportNavigator::start_pan(const glm::vec2& mouse_pos) {
    // Clear all previous navigation state
    clear_momentum();
    
    state_.mode = NavigationMode::PAN;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
    state_.pan_smooth = glm::vec2(0.0f);  // Reset smooth state for fresh start
}

void ViewportNavigator::update_pan(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * pan_sensitivity_ / ui_scale_;
    
    // Direct 1:1 application - no smoothing for immediate response
    apply_pan(scaled_delta);
}

void ViewportNavigator::update_pan_delta(const glm::vec2& delta, float delta_time) {
    if (state_.mode != NavigationMode::PAN) return;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * pan_sensitivity_ / ui_scale_;
    
    // Direct application for trackpad too - let the OS handle smoothing
    apply_pan(scaled_delta);
}

void ViewportNavigator::end_pan() {
    if (state_.mode == NavigationMode::PAN) {
        state_.mode = NavigationMode::NONE;
        // Clear all pan-related state to prevent drift
        state_.pan_momentum = glm::vec2(0.0f);
        state_.pan_smooth = glm::vec2(0.0f);  // Important: reset smooth state
        state_.has_momentum = false;
    }
}

void ViewportNavigator::apply_pan(const glm::vec2& delta) {
    if (!camera_) return;
    
    // Clear any orbit momentum when panning to prevent interference
    state_.orbit_momentum = glm::vec2(0.0f);
    
    
    // Calculate depth factor at current target (not orbit center)
    float depth = calculate_depth_factor(state_.camera_target);
    
    
    // Store old position for comparison
    Vector3D old_position = camera_->get_position();
    Vector3D old_target = camera_->get_target();
    
    // Camera3D::pan expects a delta where:
    // x = amount to move along camera's right vector
    // y = amount to move along camera's up vector
    // We need to scale the screen delta by the appropriate factor
    
    // Calculate movement scale based on projection type and depth
    float movement_scale;
    float aspect_ratio = static_cast<float>(viewport_width_) / viewport_height_;
    
    if (state_.is_orthographic) {
        // In orthographic, scale is based on view size
        movement_scale = state_.ortho_scale * 2.0f / viewport_height_;
    } else {
        // In perspective, scale depends on distance to target
        float fov_rad = glm::radians(state_.fov);
        float view_height = 2.0f * depth * tan(fov_rad * 0.5f);
        movement_scale = view_height / viewport_height_;
    }
    
    // Scale the screen delta to camera-space units
    // For scrolling: positive delta.x should pan camera right (world moves left)
    // For scrolling: positive delta.y should pan camera up (world moves down)
    Vector3D camera_delta(delta.x * movement_scale, -delta.y * movement_scale, 0);
    
    
    // Use Camera3D's built-in pan method which handles orbit mode correctly
    camera_->pan(camera_delta);
    
    // Update our internal state to match the camera
    Vector3D new_position = camera_->get_position();
    Vector3D new_target = camera_->get_target();
    Vector3D new_orbit_target = camera_->get_orbit_target();
    
    state_.camera_position = glm::vec3(new_position.x, new_position.y, new_position.z);
    state_.camera_target = glm::vec3(new_target.x, new_target.y, new_target.z);
    // Keep orbit center in sync when panning
    state_.orbit_center = glm::vec3(new_orbit_target.x, new_orbit_target.y, new_orbit_target.z);
    
}

void ViewportNavigator::start_orbit(const glm::vec2& mouse_pos) {
    // Start orbit at position
    
    // Clear all previous navigation state
    clear_momentum();
    
    state_.mode = NavigationMode::ORBIT;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
    
    // Orbit started
}

void ViewportNavigator::update_orbit(const glm::vec2& mouse_pos, float delta_time) {
    if (state_.mode != NavigationMode::ORBIT) return;
    
    // Calculate mouse delta
    glm::vec2 delta = mouse_pos - state_.last_mouse_pos;
    
    // Handle cursor warp: If we get a huge jump (cursor captured and warped to center),
    // reset the reference point without applying rotation
    const float WARP_THRESHOLD = 200.0f;  // Pixels - any jump larger than this is likely a warp
    if (glm::length(delta) > WARP_THRESHOLD) {
        state_.last_mouse_pos = mouse_pos;
        return;  // Skip this update
    }
    
    
    state_.last_mouse_pos = mouse_pos;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    delta *= orbit_sensitivity_mouse_ / ui_scale_;
    
    
    // Update momentum
    state_.orbit_momentum = state_.orbit_momentum * 0.8f + delta * 0.2f;
    
    // Apply orbit
    apply_orbit(state_.orbit_momentum);
}

void ViewportNavigator::update_orbit_delta(const glm::vec2& delta, float delta_time, bool is_smart_mouse) {
    // Update orbit delta
    
    if (state_.mode != NavigationMode::ORBIT) {
        // Ignoring orbit delta - not in ORBIT mode
        return;
    }
    
    // Choose sensitivity based on device type
    float sensitivity = is_smart_mouse ? orbit_sensitivity_smartmouse_ : orbit_sensitivity_trackpad_;
    
    // Apply sensitivity with UI scale compensation (divide by scale for HiDPI)
    glm::vec2 scaled_delta = delta * sensitivity / ui_scale_;
    
    // Accumulate the new delta into our momentum with higher weight for new input
    state_.orbit_momentum = state_.orbit_momentum * 0.4f + scaled_delta * 0.6f;  // More weight on new input
    
    // Smoothly interpolate current velocity toward target
    // Higher factor = more responsive, lower = smoother
    const float smoothing_factor = 0.4f;  // More smoothing for smoother feel
    state_.orbit_smooth = glm::mix(state_.orbit_smooth, state_.orbit_momentum, smoothing_factor);
    
    // Apply the smoothed delta
    apply_orbit(state_.orbit_smooth);
}

void ViewportNavigator::end_orbit() {
    // End orbit
    
    if (state_.mode == NavigationMode::ORBIT) {
        state_.mode = NavigationMode::NONE;
        state_.has_momentum = true;
        // Clear all orbit-related state when ending gesture
        state_.orbit_smooth = glm::vec2(0.0f);
        state_.orbit_momentum = glm::vec2(0.0f);
        
        // Orbit ended
    } else {
        // Not in orbit mode
    }
}

void ViewportNavigator::apply_orbit(const glm::vec2& delta) {
    if (!camera_) return;
    
    // Clear any pan momentum when orbiting to prevent interference
    state_.pan_momentum = glm::vec2(0.0f);
    
    // Convert mouse movement to rotation angles
    // Sensitivity is already applied before calling this function
    float horizontal_angle = -delta.x * 0.01f;  // Convert to radians
    float vertical_angle = -delta.y * 0.01f;    // Convert to radians
    
    
    // Store old state for debugging
    Vector3D old_position = camera_->get_position();
    Vector3D old_target = camera_->get_target();
    
    // Use Camera3D's built-in orbit methods
    // These handle the camera's internal state correctly without conflicts
    camera_->rotate_around_target(horizontal_angle, vertical_angle);
    
    // Update our internal state to match the camera
    Vector3D new_position = camera_->get_position();
    Vector3D new_target = camera_->get_target();
    
    state_.camera_position = glm::vec3(new_position.x, new_position.y, new_position.z);
    state_.camera_target = glm::vec3(new_target.x, new_target.y, new_target.z);
    state_.orbit_center = state_.camera_target; // Keep orbit center at target
    
}

void ViewportNavigator::zoom(float delta, const glm::vec2& mouse_pos) {
    // Zoom called
    
    // Clear orbit state when zooming to prevent interference
    if (state_.mode == NavigationMode::ORBIT) {
        // Ending orbit before zoom
        end_orbit();
    }
    
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
        // Zoom delta and mouse position
    }
    
    // Use simple zoom behavior - just move camera closer/farther from orbit center
    apply_zoom(scaled_delta);
}

void ViewportNavigator::apply_zoom(float delta) {
    if (!camera_) return;
    
    // Clear pan and orbit momentum when zooming to prevent interference
    state_.pan_momentum = glm::vec2(0.0f);
    state_.orbit_momentum = glm::vec2(0.0f);
    
    
    // Store old state for debugging
    Vector3D old_position = camera_->get_position();
    float old_distance = camera_->get_distance();
    
    // Calculate zoom factor using exponential for smooth feel
    // Negative delta zooms in, positive zooms out
    float zoom_factor = std::exp(-delta);
    
    // Use Camera3D's built-in zoom method
    // This handles both perspective (dolly) and orthographic zoom correctly
    camera_->zoom(zoom_factor);
    
    // Update our internal state to match the camera
    Vector3D new_position = camera_->get_position();
    Vector3D new_target = camera_->get_target();
    float new_distance = camera_->get_distance();
    
    state_.camera_position = glm::vec3(new_position.x, new_position.y, new_position.z);
    state_.camera_target = glm::vec3(new_target.x, new_target.y, new_target.z);
    
    // Update orthographic scale if in ortho mode
    if (camera_->get_projection_type() == Camera3D::ProjectionType::Orthographic) {
        state_.ortho_scale = camera_->get_orthographic_size();
        state_.is_orthographic = true;
    } else {
        state_.is_orthographic = false;
    }
    
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
    state_.pan_smooth = glm::vec2(0.0f);
    state_.orbit_smooth = glm::vec2(0.0f);
    state_.has_momentum = false;
}


} // namespace voxelux::canvas_ui