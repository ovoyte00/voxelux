/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Viewport Navigation Handler - Professional navigation controls
 * Professional 3D viewport navigation with mouse, trackpad, and smart mouse support
 */

#pragma once

#include "event_router.h"
#include "canvas_core.h"
#include "voxelux/core/events.h"
#include "camera_3d.h"
#include <memory>
#include <chrono>

// Forward declaration
namespace voxelux::canvas_ui {
    class ViewportNavigator;
}

namespace voxel_canvas {

// Professional Camera3D system - no legacy forward declarations

// Navigation mode types for professional 3D viewport
enum class NavigationMode {
    Orbit,      // Middle mouse drag - rotate around target
    Pan,        // Shift + middle mouse drag - pan view
    Zoom,       // Mouse wheel - zoom in/out
    Dolly,      // Ctrl + middle mouse drag - dolly camera forward/back
    None        // No active navigation
};

// Input device types for different handling
enum class InputDevice {
    Mouse,          // Standard mouse
    Trackpad,       // MacBook trackpad
    SmartMouse,     // Magic Mouse, etc.
    Tablet,         // Wacom tablet
    Unknown
};

// Navigation preferences structure for professional viewport
struct NavigationPreferences {
    // Mouse settings
    bool orbit_selection = true;        // Orbit around selection vs. viewport center
    bool depth_navigate = true;         // Use depth for navigation
    bool zoom_to_mouse = true;          // Zoom towards mouse cursor
    bool auto_perspective = true;       // Auto-switch to perspective on navigation
    
    // Sensitivity settings
    float orbit_sensitivity = 1.0f;
    float pan_sensitivity = 1.0f;
    float zoom_sensitivity = 1.0f;
    float dolly_sensitivity = 1.0f;
    
    // Trackpad settings
    bool invert_trackpad_zoom = false;
    bool trackpad_natural_scroll = true;
    bool trackpad_zoom_gesture = true;
    bool trackpad_rotate_gesture = true;
    
    // Smart mouse settings
    bool smart_zoom_enabled = true;
    bool gesture_navigation = true;
    
    // Timing settings
    int double_click_time = 400;        // ms
    int gesture_timeout = 200;          // ms
    float momentum_decay = 0.95f;       // Momentum decay factor
};

/**
 * Professional 3D Viewport Navigation Handler
 * Implements professional-grade navigation controls with support for:
 * - Standard mouse (3-button navigation)
 * - MacBook trackpad (gestures and natural scrolling)
 * - Smart mice (Magic Mouse gestures)
 * - Tablets and styluses
 */
class ViewportNavigationHandler : public ContextualInputHandler {
public:
    ViewportNavigationHandler();
    ~ViewportNavigationHandler();
    
    // ContextualInputHandler interface
    bool can_handle(const InputEvent& event) const override;
    EventResult handle_event(const InputEvent& event) override;
    
    // Camera binding
    void bind_camera(Camera3D* camera);
    // Professional Camera3D only - no legacy support
    
    // Navigation preferences
    void set_preferences(const NavigationPreferences& prefs) { prefs_ = prefs; }
    const NavigationPreferences& get_preferences() const { return prefs_; }
    
    // Input device detection and configuration
    void set_input_device(InputDevice device) { input_device_ = device; }
    InputDevice get_input_device() const { return input_device_; }
    
    // Navigation state
    NavigationMode get_current_mode() const { return current_mode_; }
    bool is_navigating() const { return current_mode_ != NavigationMode::None; }
    
    // Viewport bounds for mouse position calculations
    void set_viewport_bounds(const Rect2D& bounds);
    
    // UI scale for sensitivity adjustments  
    void set_ui_scale(float scale);
    float get_ui_scale() const { return ui_scale_; }
    
    // Navigation controls (programmatic access)
    void start_orbit(const Point2D& start_pos);
    void start_pan(const Point2D& start_pos);
    void start_zoom(const Point2D& start_pos);
    void start_dolly(const Point2D& start_pos);
    void end_navigation();
    
    // Momentum and smooth navigation
    void enable_momentum(bool enable) { momentum_enabled_ = enable; }
    void update_momentum(float delta_time);
    
private:
    // Core navigation implementations
    void handle_orbit(const Point2D& current_pos, const Point2D& delta);
    void handle_pan(const Point2D& current_pos, const Point2D& delta);
    void handle_pan_with_source(const Point2D& current_pos, const Point2D& delta, EventType source);
    void handle_pan_simple(const Point2D& current_pos, const Point2D& delta);
    void handle_zoom(float zoom_delta, const Point2D& mouse_pos = Point2D(0,0));
    void handle_dolly(const Point2D& current_pos, const Point2D& delta);
    void reset_pan_state();
    
    // Input event processing
    bool handle_mouse_event(const InputEvent& event);
    bool handle_trackpad_event(const voxelux::core::events::TrackpadEvent& event);
    bool handle_smart_mouse_event(const voxelux::core::events::SmartMouseEvent& event);
    bool handle_keyboard_navigation(const InputEvent& event);
    
    // Device-specific implementations
    void handle_standard_mouse_navigation(const InputEvent& event);
    void handle_trackpad_gestures(const voxelux::core::events::TrackpadEvent& event);
    void handle_smart_mouse_gestures(const voxelux::core::events::SmartMouseEvent& event);
    
    // Navigation mode detection
    NavigationMode detect_navigation_mode(const InputEvent& event);
    NavigationMode detect_navigation_mode_from_modifiers(uint32_t modifiers);
    bool should_start_navigation(const InputEvent& event);
    bool should_end_navigation(const InputEvent& event);
    
    // Camera calculations
    void apply_orbit_rotation(float pitch_delta, float yaw_delta);
    void apply_pan_translation(float x_delta, float y_delta);
    void apply_zoom_scale(float zoom_factor, const Point2D& focus_point);
    void apply_dolly_movement(float dolly_delta);
    
    // Coordinate system helpers
    Point2D screen_to_viewport(const Point2D& screen_pos) const;
    Point2D viewport_to_normalized(const Point2D& viewport_pos) const;
    float calculate_zoom_factor(float wheel_delta) const;
    
    // Gesture recognition
    bool is_double_click(const InputEvent& event);
    bool is_gesture_complete(const InputEvent& event);
    void reset_gesture_state();
    
    // Momentum system
    void add_momentum(const Point2D& velocity);
    void apply_momentum_to_camera();
    void decay_momentum(float decay_factor);
    
    // Professional Camera3D system only
    Camera3D* camera_ = nullptr;
    
    // Navigation state
    NavigationMode current_mode_ = NavigationMode::None;
    InputDevice input_device_ = InputDevice::Unknown;
    NavigationPreferences prefs_;
    
    // Interaction tracking
    Point2D navigation_start_pos_{0, 0};
    Point2D last_mouse_pos_{0, 0};
    Point2D accumulated_delta_{0, 0};
    bool is_dragging_ = false;
    
    // Viewport information
    Rect2D viewport_bounds_{0, 0, 800, 600};
    
    // Timing and gestures
    std::chrono::steady_clock::time_point last_click_time_;
    std::chrono::steady_clock::time_point gesture_start_time_;
    int consecutive_clicks_ = 0;
    
    // Momentum system
    bool momentum_enabled_ = true;
    Point2D momentum_velocity_{0, 0};
    float momentum_decay_rate_ = 0.95f;
    
    // Smart mouse and trackpad state
    bool trackpad_zoom_active_ = false;
    bool trackpad_rotate_active_ = false;
    float trackpad_zoom_start_scale_ = 1.0f;
    float trackpad_rotate_start_angle_ = 0.0f;
    
    // Performance optimization
    bool needs_camera_update_ = false;
    bool needs_pan_reset_ = false;
    float min_movement_threshold_ = 0.1f;
    
    // Trackpad event tracking
    EventType last_trackpad_event_ = EventType::MOUSE_MOVE;
    EventType previous_trackpad_event_ = EventType::MOUSE_MOVE;
    
    // Trackpad gesture state tracking
    Point2D trackpad_accumulated_scroll_{0, 0};  // Accumulated scroll position
    Point2D trackpad_last_scroll_pos_{0, 0};     // Last scroll position for delta calculation
    EventType trackpad_last_mode_ = EventType::MOUSE_MOVE;  // Last trackpad gesture mode
    bool trackpad_gesture_active_ = false;        // Whether a trackpad gesture is active
    
    // Pan smoothing state
    Point2D pan_smoothed_delta_{0, 0};           // Smoothed pan delta
    Point2D pan_accumulated_micro_{0, 0};        // Accumulated micro movements
    
    // Pan state tracking to prevent jumps
    Point2D pan_start_position_{0, 0};           // Camera position when pan started
    Point2D pan_start_target_{0, 0};             // Camera target when pan started
    Point2D pan_accumulated_delta_{0, 0};        // Total accumulated pan delta
    bool pan_session_active_ = false;            // Whether we're in an active pan session
    
    // UI scale for proper sensitivity adjustment
    float ui_scale_ = 1.0f;  // Default to 1.0, should be set from window
    
    // New clean viewport navigator
    std::unique_ptr<voxelux::canvas_ui::ViewportNavigator> navigator_;
};

/**
 * Navigation Helper Utilities
 */
class NavigationUtils {
public:
    // Input device detection
    static InputDevice detect_input_device();
    static bool is_trackpad_available();
    static bool is_smart_mouse_available();
    
    // Coordinate transformations
    static Point2D screen_to_ndc(const Point2D& screen_pos, const Rect2D& viewport);
    static float calculate_orbit_sensitivity(InputDevice device);
    static float calculate_pan_sensitivity(InputDevice device, float camera_distance);
    static float calculate_zoom_sensitivity(InputDevice device);
    
    // Industry-standard navigation calculations
    static void orbit_camera_around_target(Camera3D& camera, float pitch_delta, float yaw_delta, const Point2D& target);
    static void pan_camera_in_view_plane(Camera3D& camera, float x_delta, float y_delta);
    static void zoom_camera_towards_point(Camera3D& camera, float zoom_factor, const Point2D& screen_point, const Rect2D& viewport);
    
    // Gesture recognition utilities
    static bool is_pan_gesture(const Point2D& start, const Point2D& current, float threshold = 10.0f);
    static bool is_zoom_gesture(float scale_delta, float threshold = 0.1f);
    static bool is_rotate_gesture(float angle_delta, float threshold = 5.0f);
    
private:
    NavigationUtils() = default;
};

} // namespace voxel_canvas