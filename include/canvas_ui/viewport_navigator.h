#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace voxel_canvas {
    class Camera3D;
}

namespace voxelux::canvas_ui {

using Camera3D = voxel_canvas::Camera3D;

class ViewportNavigator {
public:
    enum class NavigationMode {
        NONE,
        PAN,
        ORBIT,
        ZOOM
    };

    struct NavigationState {
        NavigationMode mode = NavigationMode::NONE;
        
        // Camera state
        glm::vec3 camera_position;
        glm::vec3 camera_target;
        glm::vec3 orbit_center;
        glm::quat camera_rotation;
        
        // Navigation parameters
        float depth_factor = 1.0f;
        float fov = 45.0f;
        bool is_orthographic = false;
        float ortho_scale = 10.0f;
        
        // Input tracking
        glm::vec2 last_mouse_pos{0.0f};
        glm::vec2 mouse_delta{0.0f};
        
        // Momentum tracking
        glm::vec2 pan_momentum{0.0f};
        glm::vec2 orbit_momentum{0.0f};
        float zoom_momentum = 0.0f;
        
        // Smoothing for trackpad (reduces choppiness)
        glm::vec2 orbit_smooth{0.0f};
        glm::vec2 pan_smooth{0.0f};
        
        // Timing
        double last_update_time = 0.0;
        bool has_momentum = false;
    };

    ViewportNavigator();
    ~ViewportNavigator();
    
    // Initialize with camera
    void initialize(Camera3D* camera, int viewport_width, int viewport_height);
    
    // Update viewport dimensions
    void set_viewport_size(int width, int height);
    
    // Set UI scale factor (for HiDPI displays)
    void set_ui_scale(float scale);
    
    // Navigation operations
    void start_pan(const glm::vec2& mouse_pos);
    void update_pan(const glm::vec2& mouse_pos, float delta_time);
    void update_pan_delta(const glm::vec2& delta, float delta_time);  // For trackpad deltas
    void end_pan();
    
    void start_orbit(const glm::vec2& mouse_pos);
    void update_orbit(const glm::vec2& mouse_pos, float delta_time);
    void update_orbit_delta(const glm::vec2& delta, float delta_time, bool is_smart_mouse = false);  // For trackpad deltas
    void end_orbit();
    
    void zoom(float delta, const glm::vec2& mouse_pos);
    
    // Update momentum (call each frame)
    void update_momentum(float delta_time);
    
    // Reset momentum when switching modes
    void clear_momentum();
    
    // Get current navigation mode
    NavigationMode get_mode() const { return state_.mode; }
    
private:
    // Calculate depth factor for current view
    float calculate_depth_factor(const glm::vec3& point) const;
    
    
    // Apply pan transformation
    void apply_pan(const glm::vec2& delta);
    
    // Apply orbit transformation
    void apply_orbit(const glm::vec2& delta);
    
    // Apply zoom transformation
    void apply_zoom(float delta);
    
    
    
private:
    Camera3D* camera_;
    NavigationState state_;
    
    int viewport_width_;
    int viewport_height_;
    
    // Navigation parameters (base values before UI scale)
    float ui_scale_ = 1.0f;
    float pan_sensitivity_ = 3.0f;
    float orbit_sensitivity_mouse_ = 0.7f;       // Mouse orbit sensitivity (lower = less sensitive)
    float orbit_sensitivity_trackpad_ = 1.2f;    // Trackpad sensitivity (lower = less sensitive)
    float orbit_sensitivity_smartmouse_ = 0.7f;  // Smart mouse sensitivity
    float zoom_sensitivity_ = 0.1f;
    float momentum_decay_ = 0.92f;  // Momentum decay factor
    
    // Trackball parameters
    bool use_trackball_ = false;  // Use turntable for more intuitive control
    float trackball_size_ = 0.8f;
};

} // namespace voxelux::canvas_ui