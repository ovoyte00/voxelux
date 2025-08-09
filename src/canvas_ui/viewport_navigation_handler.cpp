/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Viewport Navigation Handler Implementation
 */

#include "canvas_ui/viewport_navigation_handler.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// Constants for professional 3D navigation (Voxelux optimized values)
namespace {
    // Math constants first
    constexpr float PI = 3.14159265359f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    
    // Professional viewport navigation sensitivity values
    // Calibrated to match industry-standard 3D applications
    constexpr float ORBIT_SENSITIVITY_DEGREES = 0.4f;  // degrees per pixel
    constexpr float ORBIT_SENSITIVITY_MOUSE = ORBIT_SENSITIVITY_DEGREES * DEG_TO_RAD;  // ~0.007 radians
    constexpr float ORBIT_SENSITIVITY_TRACKPAD = ORBIT_SENSITIVITY_MOUSE * 0.5f;  // Half for trackpad
    constexpr float PAN_SENSITIVITY_MOUSE = 0.002f;  // Viewport units per pixel (industry standard)
    constexpr float PAN_SENSITIVITY_TRACKPAD = PAN_SENSITIVITY_MOUSE * 0.5f;  // Half for trackpad
    constexpr float ZOOM_SENSITIVITY_MOUSE = 0.1f;  // Zoom factor per wheel click (10% per click)
    constexpr float ZOOM_SENSITIVITY_TRACKPAD = 0.05f;  // Half for smooth trackpad
    constexpr float DOLLY_SENSITIVITY = 0.01f;
    
    constexpr float MIN_ZOOM_DISTANCE = 0.1f;
    constexpr float MAX_ZOOM_DISTANCE = 100000.0f;
    constexpr float PITCH_CLAMP_MIN = -89.0f;
    constexpr float PITCH_CLAMP_MAX = 89.0f;
}

ViewportNavigationHandler::ViewportNavigationHandler()
    : ContextualInputHandler(InputContext::EDITOR_NAVIGATION, 250)
    , camera_(nullptr)
    , current_mode_(NavigationMode::None)
    , input_device_(NavigationUtils::detect_input_device())
    , is_dragging_(false)
    , consecutive_clicks_(0)
    , momentum_enabled_(true)
    , trackpad_zoom_active_(false)
    , trackpad_rotate_active_(false)
    , needs_camera_update_(false)
{
    // Set default preferences based on detected input device
    prefs_.orbit_sensitivity = NavigationUtils::calculate_orbit_sensitivity(input_device_);
    prefs_.pan_sensitivity = NavigationUtils::calculate_pan_sensitivity(input_device_, 10.0f);
    prefs_.zoom_sensitivity = NavigationUtils::calculate_zoom_sensitivity(input_device_);
}

ViewportNavigationHandler::~ViewportNavigationHandler() = default;

bool ViewportNavigationHandler::can_handle(const InputEvent& event) const {
    // Handle all mouse and trackpad events for navigation
    return event.is_mouse_event() || 
           event.type == EventType::MOUSE_WHEEL ||
           event.type == EventType::TRACKPAD_SCROLL ||
           event.type == EventType::TRACKPAD_PAN ||
           event.type == EventType::TRACKPAD_ZOOM ||
           event.type == EventType::TRACKPAD_ROTATE ||
           event.is_keyboard_event();
}

EventResult ViewportNavigationHandler::handle_event(const InputEvent& event) {
    // Check if we should handle this event
    if (!can_handle(event)) {
        return EventResult::IGNORED;
    }
    
    // Handle different input types
    if (event.is_mouse_event() || 
        event.type == EventType::MOUSE_WHEEL || 
        event.type == EventType::TRACKPAD_SCROLL ||
        event.type == EventType::TRACKPAD_PAN ||
        event.type == EventType::TRACKPAD_ZOOM ||
        event.type == EventType::TRACKPAD_ROTATE) {
        return handle_mouse_event(event) ? EventResult::HANDLED : EventResult::IGNORED;
    }
    
    if (event.is_keyboard_event()) {
        return handle_keyboard_navigation(event) ? EventResult::HANDLED : EventResult::IGNORED;
    }
    
    return EventResult::IGNORED;
}

bool ViewportNavigationHandler::handle_mouse_event(const InputEvent& event) {
    // Detect device type based on event rather than initialization
    // This allows dynamic switching between mouse and smart mouse
    bool is_smart_event = (event.type == EventType::TRACKPAD_SCROLL ||
                          event.type == EventType::TRACKPAD_PAN ||
                          event.type == EventType::TRACKPAD_ZOOM ||
                          event.type == EventType::TRACKPAD_ROTATE);
    
    // Update device type dynamically if we detect smart mouse events
    if (is_smart_event) {
        input_device_ = InputDevice::SmartMouse;
    }
    
    // Always use standard navigation handler which properly handles all event types
    handle_standard_mouse_navigation(event);
    return true;
}

void ViewportNavigationHandler::handle_standard_mouse_navigation(const InputEvent& event) {
    switch (event.type) {
        case EventType::MOUSE_PRESS:
            if (event.mouse_button == MouseButton::MIDDLE) {
                // Professional 3D navigation: Middle mouse button
                NavigationMode mode = detect_navigation_mode(event);
                if (mode != NavigationMode::None) {
                    current_mode_ = mode;
                    navigation_start_pos_ = event.mouse_pos;
                    last_mouse_pos_ = event.mouse_pos;
                    is_dragging_ = false;
                    
                    // Capture mouse for this operation
                    // Note: Would need EventRouter reference to actually capture
                }
            }
            break;
            
        case EventType::MOUSE_RELEASE:
            if (event.mouse_button == MouseButton::MIDDLE) {
                end_navigation();
            }
            break;
            
        case EventType::MOUSE_MOVE:
            if (current_mode_ != NavigationMode::None) {
                Point2D delta = event.mouse_pos - last_mouse_pos_;
                
                // Apply minimum movement threshold for performance
                if (std::abs(delta.x) < min_movement_threshold_ && 
                    std::abs(delta.y) < min_movement_threshold_) {
                    return;
                }
                
                is_dragging_ = true;
                
                switch (current_mode_) {
                    case NavigationMode::Orbit:
                        handle_orbit(event.mouse_pos, delta);
                        break;
                        
                    case NavigationMode::Pan:
                        handle_pan(event.mouse_pos, delta);
                        break;
                        
                    case NavigationMode::Dolly:
                        handle_dolly(event.mouse_pos, delta);
                        break;
                        
                    default:
                        break;
                }
                
                last_mouse_pos_ = event.mouse_pos;
                
                // Add momentum for smooth navigation
                if (momentum_enabled_) {
                    add_momentum(delta);
                }
            }
            break;
            
        case EventType::MOUSE_WHEEL:
            {
                Point2D mouse_pos = event.mouse_pos;
                if (prefs_.zoom_to_mouse) {
                    handle_zoom(event.wheel_delta, mouse_pos);
                } else {
                    handle_zoom(event.wheel_delta);
                }
            }
            break;
            
        case EventType::TRACKPAD_SCROLL:
            {
                Point2D mouse_pos = event.mouse_pos;
                if (prefs_.zoom_to_mouse) {
                    handle_zoom(event.wheel_delta, mouse_pos);
                } else {
                    handle_zoom(event.wheel_delta);
                }
            }
            break;
            
        case EventType::TRACKPAD_ROTATE:
            {
                // Smart mouse surface trackpad = orbit camera (like Blender MOUSEROTATE)
                Point2D gesture_delta = event.mouse_delta;
                handle_orbit(event.mouse_pos, gesture_delta);
            }
            break;
            
        case EventType::TRACKPAD_ZOOM:
            {
                // Smart mouse surface + Cmd = zoom (like Blender MOUSEZOOM)
                Point2D mouse_pos = event.mouse_pos;
                float zoom_delta = event.mouse_delta.y; // Use Y delta for zoom
                if (prefs_.zoom_to_mouse) {
                    handle_zoom(zoom_delta, mouse_pos);
                } else {
                    handle_zoom(zoom_delta);
                }
            }
            break;
            
        case EventType::TRACKPAD_PAN:
            {
                // Smart mouse surface + Shift = pan (like Blender MOUSEPAN)
                Point2D gesture_delta = event.mouse_delta;
                handle_pan(event.mouse_pos, gesture_delta);
            }
            break;
            
        default:
            break;
    }
}

bool ViewportNavigationHandler::handle_trackpad_event(const voxelux::core::events::TrackpadEvent& event) {
    switch (event.type()) {
        case voxelux::core::events::TrackpadEvent::Type::Scroll:
            // Trackpad scroll for zoom
            if (prefs_.trackpad_natural_scroll) {
                handle_zoom(-event.delta_y() * prefs_.zoom_sensitivity);
            } else {
                handle_zoom(event.delta_y() * prefs_.zoom_sensitivity);
            }
            return true;
            
        case voxelux::core::events::TrackpadEvent::Type::Pan:
            // Two-finger pan
            start_pan(Point2D(event.x(), event.y()));
            handle_pan(Point2D(event.x(), event.y()), Point2D(event.delta_x(), event.delta_y()));
            return true;
            
        case voxelux::core::events::TrackpadEvent::Type::Zoom:
            // Pinch to zoom
            if (prefs_.trackpad_zoom_gesture) {
                float zoom_factor = event.scale_factor();
                handle_zoom(zoom_factor > 1.0f ? 1.0f : -1.0f, Point2D(event.x(), event.y()));
            }
            return true;
            
        case voxelux::core::events::TrackpadEvent::Type::Rotate:
            // Two-finger rotate
            if (prefs_.trackpad_rotate_gesture) {
                // Convert rotation to orbit motion
                float rotation_delta = event.rotation_angle();
                Point2D orbit_delta(rotation_delta * 2.0f, 0);
                handle_orbit(Point2D(event.x(), event.y()), orbit_delta);
            }
            return true;
            
        case voxelux::core::events::TrackpadEvent::Type::SmartZoom:
            // Two-finger tap for smart zoom
            if (prefs_.smart_zoom_enabled) {
                // Frame all or zoom to fit
                // This would need access to viewport bounds calculation
                handle_zoom(0.5f, Point2D(event.x(), event.y()));
            }
            return true;
    }
    
    return false;
}

bool ViewportNavigationHandler::handle_smart_mouse_event(const voxelux::core::events::SmartMouseEvent& event) {
    if (!prefs_.gesture_navigation) {
        return false;
    }
    
    switch (event.gesture()) {
        case voxelux::core::events::SmartMouseEvent::Gesture::SwipeLeft:
            // Orbit left
            handle_orbit(Point2D(event.x(), event.y()), Point2D(-50, 0));
            return true;
            
        case voxelux::core::events::SmartMouseEvent::Gesture::SwipeRight:
            // Orbit right
            handle_orbit(Point2D(event.x(), event.y()), Point2D(50, 0));
            return true;
            
        case voxelux::core::events::SmartMouseEvent::Gesture::SwipeUp:
            // Orbit up
            handle_orbit(Point2D(event.x(), event.y()), Point2D(0, -50));
            return true;
            
        case voxelux::core::events::SmartMouseEvent::Gesture::SwipeDown:
            // Orbit down
            handle_orbit(Point2D(event.x(), event.y()), Point2D(0, 50));
            return true;
            
        case voxelux::core::events::SmartMouseEvent::Gesture::ForceClick:
            // Frame selected or reset view
            return true;
            
        default:
            break;
    }
    
    return false;
}

bool ViewportNavigationHandler::handle_keyboard_navigation(const InputEvent& event) {
    if (event.type != EventType::KEY_PRESS) {
        return false;
    }
    
    // Professional numpad navigation shortcuts
    switch (event.key_code) {
        case 49: // Numpad 1 - Front view
            if (camera_) {
                camera_->set_projection_type(Camera3D::ProjectionType::Orthographic);
                camera_->rotate_around_target(0.0f, 0.0f); // Front view
            }
            return true;
            
        case 51: // Numpad 3 - Right view
            if (camera_) {
                camera_->set_projection_type(Camera3D::ProjectionType::Orthographic);
                camera_->rotate_around_target(90.0f * DEG_TO_RAD, 0.0f); // Right view
            }
            return true;
            
        case 55: // Numpad 7 - Top view
            if (camera_) {
                camera_->set_projection_type(Camera3D::ProjectionType::Orthographic);
                camera_->rotate_around_target(0.0f, 90.0f * DEG_TO_RAD); // Top view
            }
            return true;
            
        case 46: // Period/Home - Frame all
            // Reset to default view
            if (camera_) {
                camera_->reset_to_default();
            }
            return true;
    }
    
    return false;
}

NavigationMode ViewportNavigationHandler::detect_navigation_mode(const InputEvent& event) {
    if (event.mouse_button != MouseButton::MIDDLE) {
        return NavigationMode::None;
    }
    
    // Professional 3D navigation modifier detection
    if (event.has_modifier(KeyModifier::SHIFT)) {
        return NavigationMode::Pan;       // Shift + MMB = Pan
    } else if (event.has_modifier(KeyModifier::CTRL)) {
        return NavigationMode::Dolly;     // Ctrl + MMB = Dolly
    } else {
        return NavigationMode::Orbit;     // MMB = Orbit
    }
}

void ViewportNavigationHandler::handle_orbit(const Point2D& current_pos, const Point2D& delta) {
    // Use the appropriate sensitivity based on input device
    float sensitivity = (input_device_ == InputDevice::Trackpad || input_device_ == InputDevice::SmartMouse) ?
                       ORBIT_SENSITIVITY_TRACKPAD : ORBIT_SENSITIVITY_MOUSE;
    
    float pitch_delta = -delta.y * sensitivity;
    float yaw_delta = delta.x * sensitivity;
    
    if (camera_) {
        NavigationUtils::orbit_camera_around_target(*camera_, pitch_delta, yaw_delta, Point2D(0, 0));
    }
    
    needs_camera_update_ = true;
}

void ViewportNavigationHandler::handle_pan(const Point2D& current_pos, const Point2D& delta) {
    // Use the appropriate sensitivity based on input device
    float sensitivity = (input_device_ == InputDevice::Trackpad || input_device_ == InputDevice::SmartMouse) ?
                       PAN_SENSITIVITY_TRACKPAD : PAN_SENSITIVITY_MOUSE;
    
    float x_delta = -delta.x * sensitivity;
    float y_delta = delta.y * sensitivity;
    
    // Scale pan by camera distance for consistent feel
    if (camera_) {
        float distance_scale = camera_->get_distance();
        x_delta *= distance_scale;
        y_delta *= distance_scale;
        NavigationUtils::pan_camera_in_view_plane(*camera_, x_delta, y_delta);
    }
    
    needs_camera_update_ = true;
}

void ViewportNavigationHandler::handle_zoom(float zoom_delta, const Point2D& mouse_pos) {
    float zoom_factor = calculate_zoom_factor(zoom_delta);
    
    
    if (camera_) {
        NavigationUtils::zoom_camera_towards_point(*camera_, zoom_factor, mouse_pos, viewport_bounds_);
    }
    
    needs_camera_update_ = true;
}

void ViewportNavigationHandler::handle_dolly(const Point2D& current_pos, const Point2D& delta) {
    float dolly_delta = delta.y * DOLLY_SENSITIVITY;
    
    if (camera_) {
        camera_->dolly(dolly_delta);
    }
    
    needs_camera_update_ = true;
}

float ViewportNavigationHandler::calculate_zoom_factor(float wheel_delta) const {
    // Use different sensitivity for trackpad vs mouse wheel
    float sensitivity = (input_device_ == InputDevice::Trackpad || input_device_ == InputDevice::SmartMouse) ? 
                       ZOOM_SENSITIVITY_TRACKPAD : ZOOM_SENSITIVITY_MOUSE;
    
    // Convert wheel delta to zoom factor
    // Positive delta = zoom in (decrease distance), negative = zoom out (increase distance)
    if (wheel_delta > 0) {
        return 1.0f - sensitivity;  // Zoom in (e.g., 0.92)
    } else {
        return 1.0f + sensitivity;  // Zoom out (e.g., 1.08)
    }
}

void ViewportNavigationHandler::end_navigation() {
    current_mode_ = NavigationMode::None;
    is_dragging_ = false;
    accumulated_delta_ = Point2D(0, 0);
    
    // Release mouse capture if we had it
    // Note: Would need EventRouter reference to actually release
}

void ViewportNavigationHandler::add_momentum(const Point2D& velocity) {
    momentum_velocity_ = momentum_velocity_ * 0.8f + velocity * 0.2f;
}

void ViewportNavigationHandler::update_momentum(float delta_time) {
    if (!momentum_enabled_ || current_mode_ != NavigationMode::None) {
        momentum_velocity_ = Point2D(0, 0);
        return;
    }
    
    // Apply momentum to camera if velocity is significant
    float velocity_magnitude = std::sqrt(momentum_velocity_.x * momentum_velocity_.x + 
                                        momentum_velocity_.y * momentum_velocity_.y);
    
    if (velocity_magnitude > 0.1f) {
        // Apply momentum based on last navigation mode
        // For now, just apply as orbit
        handle_orbit(Point2D(0, 0), momentum_velocity_ * delta_time * 60.0f);
        
        // Decay momentum
        decay_momentum(momentum_decay_rate_);
    } else {
        momentum_velocity_ = Point2D(0, 0);
    }
}

void ViewportNavigationHandler::decay_momentum(float decay_factor) {
    momentum_velocity_.x *= decay_factor;
    momentum_velocity_.y *= decay_factor;
}

void ViewportNavigationHandler::start_orbit(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Orbit;
    navigation_start_pos_ = start_pos;
    last_mouse_pos_ = start_pos;
}

void ViewportNavigationHandler::start_pan(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Pan;
    navigation_start_pos_ = start_pos;
    last_mouse_pos_ = start_pos;
}

void ViewportNavigationHandler::start_zoom(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Zoom;
    navigation_start_pos_ = start_pos;
    last_mouse_pos_ = start_pos;
}

void ViewportNavigationHandler::start_dolly(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Dolly;
    navigation_start_pos_ = start_pos;
    last_mouse_pos_ = start_pos;
}

// NavigationUtils implementation

InputDevice NavigationUtils::detect_input_device() {
    // Platform-specific detection would go here
    // For now, return Mouse as default
    return InputDevice::Mouse;
}

bool NavigationUtils::is_trackpad_available() {
    // Platform-specific trackpad detection
    return false;
}

bool NavigationUtils::is_smart_mouse_available() {
    // Platform-specific smart mouse detection
    return false;
}

Point2D NavigationUtils::screen_to_ndc(const Point2D& screen_pos, const Rect2D& viewport) {
    return Point2D(
        (screen_pos.x - viewport.x) / viewport.width * 2.0f - 1.0f,
        (screen_pos.y - viewport.y) / viewport.height * 2.0f - 1.0f
    );
}

float NavigationUtils::calculate_orbit_sensitivity(InputDevice device) {
    switch (device) {
        case InputDevice::Trackpad: return ORBIT_SENSITIVITY_TRACKPAD;
        case InputDevice::SmartMouse: return ORBIT_SENSITIVITY_TRACKPAD;
        default: return ORBIT_SENSITIVITY_MOUSE;
    }
}

float NavigationUtils::calculate_pan_sensitivity(InputDevice device, float camera_distance) {
    float base_sensitivity = (device == InputDevice::Trackpad) ? 
                           PAN_SENSITIVITY_TRACKPAD : PAN_SENSITIVITY_MOUSE;
    return base_sensitivity * std::max(1.0f, camera_distance * 0.1f);
}

float NavigationUtils::calculate_zoom_sensitivity(InputDevice device) {
    switch (device) {
        case InputDevice::Trackpad: return ZOOM_SENSITIVITY_TRACKPAD;
        case InputDevice::SmartMouse: return ZOOM_SENSITIVITY_TRACKPAD;
        default: return ZOOM_SENSITIVITY_MOUSE;
    }
}

// Legacy camera functions removed - using professional Camera3D system

bool NavigationUtils::is_pan_gesture(const Point2D& start, const Point2D& current, float threshold) {
    float distance = std::sqrt((current.x - start.x) * (current.x - start.x) + 
                              (current.y - start.y) * (current.y - start.y));
    return distance > threshold;
}

bool NavigationUtils::is_zoom_gesture(float scale_delta, float threshold) {
    return std::abs(scale_delta - 1.0f) > threshold;
}

bool NavigationUtils::is_rotate_gesture(float angle_delta, float threshold) {
    return std::abs(angle_delta) > threshold;
}

void NavigationUtils::orbit_camera_around_target(Camera3D& camera, float pitch_delta, float yaw_delta, const Point2D& target) {
    camera.rotate_around_target(yaw_delta, pitch_delta);
}

void NavigationUtils::pan_camera_in_view_plane(Camera3D& camera, float x_delta, float y_delta) {
    camera.pan({x_delta, y_delta, 0});
}

void NavigationUtils::zoom_camera_towards_point(Camera3D& camera, float zoom_factor, const Point2D& screen_point, const Rect2D& viewport) {
    camera.zoom(zoom_factor);
}

} // namespace voxel_canvas