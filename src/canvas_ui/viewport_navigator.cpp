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
    
    // Debug output
    static int depth_debug_counter = 0;
    if (++depth_debug_counter % 30 == 0) {
        std::cout << "[DEPTH] Point: (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
        std::cout << "[DEPTH] Camera pos: (" << state_.camera_position.x << ", " 
                  << state_.camera_position.y << ", " << state_.camera_position.z << ")" << std::endl;
        std::cout << "[DEPTH] Distance: " << distance << std::endl;
    }
    
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

glm::vec3 ViewportNavigator::screen_to_world_delta(const glm::vec2& screen_delta, float z_depth) const {
    // Get the inverse view matrix to transform from view space to world space
    glm::mat4 view = get_view_matrix();
    glm::mat4 view_inv = glm::inverse(view);
    
    // Extract the view's basis vectors from the inverse matrix
    // In GLM (column-major), mat[i] gives you column i
    // For inverse view matrix:
    // - Column 0 = camera right in world space
    // - Column 1 = camera up in world space  
    // - Column 2 = camera backward in world space (-forward)
    glm::vec3 view_right = glm::vec3(view_inv[0]);  // First column
    glm::vec3 view_up = glm::vec3(view_inv[1]);     // Second column
    
    // Calculate how much one pixel of screen movement translates to world movement
    float aspect_ratio = static_cast<float>(viewport_width_) / viewport_height_;
    float movement_scale;
    
    if (state_.is_orthographic) {
        // In orthographic projection, movement is independent of depth
        // Scale based on the orthographic view bounds
        movement_scale = (state_.ortho_scale * 2.0f) / viewport_height_;
    } else {
        // In perspective projection, movement scales with distance from target
        // Use field of view to determine the view cone size at the target distance
        float fov_radians = glm::radians(state_.fov);
        float view_height_at_target = 2.0f * z_depth * std::tan(fov_radians * 0.5f);
        movement_scale = view_height_at_target / viewport_height_;
    }
    
    // Apply sensitivity multiplier to make pan movement more noticeable
    // This compensates for the small scale values we're getting
    const float pan_sensitivity_multiplier = 50.0f;  // Increase for more sensitivity
    movement_scale *= pan_sensitivity_multiplier;
    
    // Convert screen pixel deltas to world space distances
    float horizontal_distance = screen_delta.x * movement_scale * aspect_ratio;
    float vertical_distance = -screen_delta.y * movement_scale;  // Negative because screen Y is inverted
    
    // Combine the movements along the view's basis vectors
    glm::vec3 world_movement = view_right * horizontal_distance + view_up * vertical_distance;
    
    // Debug output to verify the calculation
    static int debug_counter = 0;
    if (++debug_counter % 5 == 0) {  // More frequent debug output
        // Extract forward for verification
        glm::vec3 view_forward = -glm::vec3(view_inv[2]);  // Third column (negative because OpenGL looks down -Z)
        
        std::cout << "\n[PAN DEBUG] ===== Frame " << debug_counter << " =====" << std::endl;
        std::cout << "  Camera position: (" << state_.camera_position.x << ", " 
                  << state_.camera_position.y << ", " << state_.camera_position.z << ")" << std::endl;
        std::cout << "  Camera target: (" << state_.camera_target.x << ", " 
                  << state_.camera_target.y << ", " << state_.camera_target.z << ")" << std::endl;
        std::cout << "  Screen delta: (" << screen_delta.x << ", " << screen_delta.y << ") pixels" << std::endl;
        std::cout << "  Movement scale: " << movement_scale << " world units per pixel" << std::endl;
        
        std::cout << "\n  Basis vectors from inverse view matrix:" << std::endl;
        std::cout << "    Right:   (" << view_right.x << ", " << view_right.y << ", " << view_right.z << ")" << std::endl;
        std::cout << "    Up:      (" << view_up.x << ", " << view_up.y << ", " << view_up.z << ")" << std::endl;
        std::cout << "    Forward: (" << view_forward.x << ", " << view_forward.y << ", " << view_forward.z << ")" << std::endl;
        
        // Verify orthogonality
        float dot_right_up = glm::dot(view_right, view_up);
        float dot_right_forward = glm::dot(view_right, view_forward);
        float dot_up_forward = glm::dot(view_up, view_forward);
        std::cout << "\n  Orthogonality check:" << std::endl;
        std::cout << "    right·up = " << dot_right_up << " (should be ~0)" << std::endl;
        std::cout << "    right·forward = " << dot_right_forward << " (should be ~0)" << std::endl;
        std::cout << "    up·forward = " << dot_up_forward << " (should be ~0)" << std::endl;
        
        // Verify unit vectors
        std::cout << "\n  Vector magnitudes:" << std::endl;
        std::cout << "    |right| = " << glm::length(view_right) << " (should be ~1)" << std::endl;
        std::cout << "    |up| = " << glm::length(view_up) << " (should be ~1)" << std::endl;
        std::cout << "    |forward| = " << glm::length(view_forward) << " (should be ~1)" << std::endl;
        
        std::cout << "\n  World movement: (" << world_movement.x << ", " 
                  << world_movement.y << ", " << world_movement.z << ")" << std::endl;
        std::cout << "  Movement magnitude: " << glm::length(world_movement) << " world units" << std::endl;
        std::cout << "[PAN DEBUG] =====================\n" << std::endl;
    }
    
    return world_movement;
}


void ViewportNavigator::start_pan(const glm::vec2& mouse_pos) {
    // Clear all previous navigation state
    clear_momentum();
    
    state_.mode = NavigationMode::PAN;
    state_.last_mouse_pos = mouse_pos;
    state_.mouse_delta = glm::vec2(0.0f);
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
    
    // Accumulate with decay
    state_.pan_momentum = state_.pan_momentum * 0.4f + scaled_delta * 0.6f;
    
    // Smooth interpolation
    const float smoothing_factor = 0.6f;
    state_.pan_smooth = glm::mix(state_.pan_smooth, state_.pan_momentum, smoothing_factor);
    
    // Apply the smoothed delta
    apply_pan(state_.pan_smooth);
}

void ViewportNavigator::end_pan() {
    if (state_.mode == NavigationMode::PAN) {
        state_.mode = NavigationMode::NONE;
        // Clear all pan-related state to prevent drift
        state_.pan_momentum = glm::vec2(0.0f);
        state_.pan_smooth = glm::vec2(0.0f);
        state_.has_momentum = false;
    }
}

void ViewportNavigator::apply_pan(const glm::vec2& delta) {
    if (!camera_) return;
    
    // Clear any orbit momentum when panning to prevent interference
    state_.orbit_momentum = glm::vec2(0.0f);
    
    // Debug: show what we're receiving
    static int pan_apply_counter = 0;
    if (++pan_apply_counter % 10 == 0) {
        std::cout << "[PAN APPLY] Delta received: (" << delta.x << ", " << delta.y << ")" << std::endl;
    }
    
    // Calculate depth factor at current target (not orbit center)
    float depth = calculate_depth_factor(state_.camera_target);
    
    // Debug: show depth calculation
    if (pan_apply_counter % 10 == 0) {
        std::cout << "[PAN APPLY] Depth factor: " << depth << std::endl;
        std::cout << "[PAN APPLY] Camera target: (" << state_.camera_target.x << ", " 
                  << state_.camera_target.y << ", " << state_.camera_target.z << ")" << std::endl;
    }
    
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
    // Negate X because screen X goes right but we want to move the world left when dragging right
    Vector3D camera_delta(-delta.x * movement_scale, delta.y * movement_scale, 0);
    
    // Debug: show camera delta
    if (pan_apply_counter % 10 == 0) {
        std::cout << "[PAN APPLY] Camera delta: (" << camera_delta.x << ", " 
                  << camera_delta.y << ", " << camera_delta.z << ")" << std::endl;
        std::cout << "[PAN APPLY] Movement scale: " << movement_scale << std::endl;
    }
    
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
    
    // Debug: show actual change
    if (pan_apply_counter % 10 == 0) {
        std::cout << "[PAN APPLY] Camera moved from: (" << old_position.x << ", " 
                  << old_position.y << ", " << old_position.z << ")" << std::endl;
        std::cout << "[PAN APPLY] Camera moved to: (" << new_position.x << ", " 
                  << new_position.y << ", " << new_position.z << ")" << std::endl;
        std::cout << "[PAN APPLY] Actual movement: " << (new_position - old_position).length() << std::endl;
    }
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
    
    // Debug output
    static int orbit_debug = 0;
    if (++orbit_debug % 10 == 0) {
        std::cout << "[ORBIT DEBUG]" << std::endl;
        std::cout << "  Delta: (" << delta.x << ", " << delta.y << ")" << std::endl;
        std::cout << "  Horizontal angle: " << glm::degrees(horizontal_angle) << " degrees" << std::endl;
        std::cout << "  Vertical angle: " << glm::degrees(vertical_angle) << " degrees" << std::endl;
    }
    
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
    
    // Debug: show actual change
    if (orbit_debug % 10 == 0) {
        std::cout << "  Camera moved from: (" << old_position.x << ", " 
                  << old_position.y << ", " << old_position.z << ")" << std::endl;
        std::cout << "  Camera moved to: (" << new_position.x << ", " 
                  << new_position.y << ", " << new_position.z << ")" << std::endl;
        std::cout << "  Orbit center: (" << state_.orbit_center.x << ", " 
                  << state_.orbit_center.y << ", " << state_.orbit_center.z << ")" << std::endl;
        
        // Calculate the angle from camera to origin in XZ plane
        glm::vec3 to_camera = state_.camera_position - state_.orbit_center;
        float angle_rad = std::atan2(to_camera.z, to_camera.x);
        float angle_deg = glm::degrees(angle_rad);
        std::cout << "  Camera angle in XZ plane: " << angle_deg << " degrees" << std::endl;
        std::cout << "  Distance from orbit center: " << glm::length(to_camera) << std::endl;
    }
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
    state_.pan_smooth = glm::vec2(0.0f);
    state_.orbit_smooth = glm::vec2(0.0f);
    state_.has_momentum = false;
}

void ViewportNavigator::sync_camera_state() {
    if (!camera_) {
        std::cout << "[SYNC ERROR] No camera pointer!" << std::endl;
        return;
    }
    
    // Debug camera state changes
    static glm::vec3 last_pos = state_.camera_position;
    static glm::vec3 last_target = state_.camera_target;
    static int sync_count = 0;
    
    // Always log position changes for debugging
    float move_distance = glm::distance(last_pos, state_.camera_position);
    float target_distance = glm::distance(last_target, state_.camera_target);
    
    std::cout << "[CAMERA SYNC] Syncing camera state:" << std::endl;
    std::cout << "  Old Position: (" << last_pos.x << ", " << last_pos.y << ", " << last_pos.z << ")" << std::endl;
    std::cout << "  New Position: (" << state_.camera_position.x << ", " 
              << state_.camera_position.y << ", " << state_.camera_position.z << ")" << std::endl;
    std::cout << "  Position change: " << move_distance << std::endl;
    std::cout << "  Old Target: (" << last_target.x << ", " << last_target.y << ", " << last_target.z << ")" << std::endl;
    std::cout << "  New Target: (" << state_.camera_target.x << ", " 
              << state_.camera_target.y << ", " << state_.camera_target.z << ")" << std::endl;
    std::cout << "  Target change: " << target_distance << std::endl;
    
    last_pos = state_.camera_position;
    last_target = state_.camera_target;
    
    // CRITICAL: Set camera to Fly mode before updating position/target
    // Otherwise Camera3D's update_orbit_from_position() will recalculate and override our position!
    camera_->set_navigation_mode(Camera3D::NavigationMode::Fly);
    
    // Update camera with new state - convert glm::vec3 to Vector3D
    camera_->set_position(Vector3D(state_.camera_position.x, state_.camera_position.y, state_.camera_position.z));
    camera_->set_target(Vector3D(state_.camera_target.x, state_.camera_target.y, state_.camera_target.z));
    camera_->set_orbit_target(Vector3D(state_.orbit_center.x, state_.orbit_center.y, state_.orbit_center.z));
    
    // Verify the camera was updated
    Vector3D cam_pos = camera_->get_position();
    Vector3D cam_target = camera_->get_target();
    
    std::cout << "[CAMERA SYNC] Camera after update:" << std::endl;
    std::cout << "  Camera->get_position(): (" << cam_pos.x << ", " << cam_pos.y << ", " << cam_pos.z << ")" << std::endl;
    std::cout << "  Camera->get_target(): (" << cam_target.x << ", " << cam_target.y << ", " << cam_target.z << ")" << std::endl;
    
    // Check if the values match what we set
    bool position_matches = (std::abs(cam_pos.x - state_.camera_position.x) < 0.001f &&
                             std::abs(cam_pos.y - state_.camera_position.y) < 0.001f &&
                             std::abs(cam_pos.z - state_.camera_position.z) < 0.001f);
    bool target_matches = (std::abs(cam_target.x - state_.camera_target.x) < 0.001f &&
                          std::abs(cam_target.y - state_.camera_target.y) < 0.001f &&
                          std::abs(cam_target.z - state_.camera_target.z) < 0.001f);
    
    if (!position_matches || !target_matches) {
        std::cout << "[CAMERA SYNC ERROR] Camera did not accept new values!" << std::endl;
    } else {
        std::cout << "[CAMERA SYNC] Camera successfully updated!" << std::endl;
    }
}

glm::mat4 ViewportNavigator::get_view_matrix() const {
    // Use camera_target instead of orbit_center for view
    // This allows pan to move the target independently
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