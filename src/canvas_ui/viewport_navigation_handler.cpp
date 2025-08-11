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
#include "canvas_ui/viewport_navigator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

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
    , navigator_(std::make_unique<voxelux::canvas_ui::ViewportNavigator>())
{
    // Set default preferences based on detected input device
    prefs_.orbit_sensitivity = NavigationUtils::calculate_orbit_sensitivity(input_device_);
    prefs_.pan_sensitivity = NavigationUtils::calculate_pan_sensitivity(input_device_, 10.0f);
    prefs_.zoom_sensitivity = NavigationUtils::calculate_zoom_sensitivity(input_device_);
}

ViewportNavigationHandler::~ViewportNavigationHandler() = default;

void ViewportNavigationHandler::bind_camera(Camera3D* camera) {
    camera_ = camera;
    if (navigator_ && camera_) {
        // Initialize navigator with camera and viewport
        navigator_->initialize(camera_, viewport_bounds_.width, viewport_bounds_.height);
    }
}

void ViewportNavigationHandler::set_viewport_bounds(const Rect2D& bounds) {
    viewport_bounds_ = bounds;
    if (navigator_) {
        navigator_->set_viewport_size(bounds.width, bounds.height);
    }
}

void ViewportNavigationHandler::set_ui_scale(float scale) {
    ui_scale_ = scale;
}

bool ViewportNavigationHandler::can_handle(const InputEvent& event) const {
    // Handle all mouse and trackpad events for navigation
    return event.is_mouse_event() || 
           event.type == EventType::MOUSE_WHEEL ||
           event.type == EventType::TRACKPAD_SCROLL ||
           event.type == EventType::TRACKPAD_PAN ||
           event.type == EventType::TRACKPAD_ZOOM ||
           event.type == EventType::TRACKPAD_ROTATE ||
           event.type == EventType::MODIFIER_CHANGE ||
           event.is_keyboard_event();
}

EventResult ViewportNavigationHandler::handle_event(const InputEvent& event) {
    // Check if we should handle this event
    if (!can_handle(event) || !navigator_ || !camera_) {
        if (!navigator_) std::cout << "[NavHandler] No navigator!" << std::endl;
        if (!camera_) std::cout << "[NavHandler] No camera!" << std::endl;
        return EventResult::IGNORED;
    }
    
    
    // Handle modifier change events
    if (event.type == EventType::MODIFIER_CHANGE) {
        // Check if shift was released during pan
        bool shift_held = event.has_modifier(KeyModifier::SHIFT);
        if (!shift_held && navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
            navigator_->end_pan();
            current_mode_ = NavigationMode::None;
        }
        // Check if shift was pressed during orbit - switch to pan
        if (shift_held && navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
            navigator_->end_orbit();
            navigator_->start_pan(glm::vec2(last_mouse_pos_.x, last_mouse_pos_.y));
            current_mode_ = NavigationMode::Pan;
        }
        return EventResult::HANDLED;
    }
    
    // Convert mouse position to glm::vec2
    glm::vec2 mouse_pos(event.mouse_pos.x, event.mouse_pos.y);
    glm::vec2 mouse_delta(event.mouse_delta.x, event.mouse_delta.y);
    
    // Handle different event types
    switch (event.type) {
        case EventType::TRACKPAD_PAN:
            {
                
                // Trackpad pan (Shift + two-finger scroll)
                // Start pan if not already panning
                if (navigator_->get_mode() != voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                    navigator_->start_pan(mouse_pos);
                    current_mode_ = NavigationMode::Pan;
                }
                
                // Update pan with delta (trackpad gives us deltas, not positions)
                navigator_->update_pan_delta(mouse_delta, 0.016f);  // ~60 FPS delta time
                return EventResult::HANDLED;
            }
            
        case EventType::TRACKPAD_ROTATE:
            {
                // Trackpad rotate (two-finger rotate without shift)
                // Start orbit if not already orbiting
                if (navigator_->get_mode() != voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                    navigator_->start_orbit(mouse_pos);
                    current_mode_ = NavigationMode::Orbit;
                    last_mouse_pos_ = event.mouse_pos;  // Reset tracking position
                }
                
                // For orbit, accumulate position for this gesture
                last_mouse_pos_ = last_mouse_pos_ + event.mouse_delta;
                navigator_->update_orbit(glm::vec2(last_mouse_pos_.x, last_mouse_pos_.y), 0.016f);
                return EventResult::HANDLED;
            }
            
        case EventType::TRACKPAD_ZOOM:
            {
                // Trackpad zoom (pinch gesture)
                float zoom_delta = event.wheel_delta * 0.05f;  // Trackpad zoom sensitivity
                navigator_->zoom(zoom_delta, mouse_pos);
                current_mode_ = NavigationMode::Zoom;
                return EventResult::HANDLED;
            }
            
        case EventType::TRACKPAD_SCROLL:
            {
                // Regular trackpad scroll without modifiers - also zoom
                float zoom_delta = -event.wheel_delta * 0.05f;  // Invert for natural scrolling
                navigator_->zoom(zoom_delta, mouse_pos);
                current_mode_ = NavigationMode::Zoom;
                return EventResult::HANDLED;
            }
            
        case EventType::MOUSE_WHEEL:
            {
                // Mouse wheel zoom
                float zoom_delta = event.wheel_delta * 0.1f;  // Mouse wheel sensitivity
                navigator_->zoom(zoom_delta, mouse_pos);
                current_mode_ = NavigationMode::Zoom;
                return EventResult::HANDLED;
            }
            
        case EventType::MOUSE_PRESS:
            {
                // Check for middle mouse button
                if (event.mouse_button == MouseButton::MIDDLE) {
                    bool shift_held = event.has_modifier(KeyModifier::SHIFT);
                    
                    if (shift_held) {
                        // Shift + Middle Mouse = Pan
                        navigator_->start_pan(mouse_pos);
                        current_mode_ = NavigationMode::Pan;
                        is_dragging_ = true;
                    } else {
                        // Middle Mouse = Orbit
                        navigator_->start_orbit(mouse_pos);
                        current_mode_ = NavigationMode::Orbit;
                        is_dragging_ = true;
                    }
                    return EventResult::HANDLED;
                }
                break;
            }
            
        case EventType::MOUSE_RELEASE:
            {
                // End navigation on mouse release
                if (event.mouse_button == MouseButton::MIDDLE && is_dragging_) {
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                        navigator_->end_pan();
                    } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        navigator_->end_orbit();
                    }
                    current_mode_ = NavigationMode::None;
                    is_dragging_ = false;
                    return EventResult::HANDLED;
                }
                break;
            }
            
        case EventType::MOUSE_MOVE:
            {
                // Update navigation if dragging
                if (is_dragging_) {
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                        navigator_->update_pan(mouse_pos, 0.016f);
                    } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        navigator_->update_orbit(mouse_pos, 0.016f);
                    }
                    return EventResult::HANDLED;
                }
                break;
            }
            
        default:
            break;
    }
    
    // Handle keyboard navigation if implemented
    if (event.is_keyboard_event()) {
        return handle_keyboard_navigation(event) ? EventResult::HANDLED : EventResult::IGNORED;
    }
    
    return EventResult::IGNORED;
}

bool ViewportNavigationHandler::handle_mouse_event(const InputEvent& event) {
    // This is now handled in handle_event directly
    return false;
}

bool ViewportNavigationHandler::handle_trackpad_event(const voxelux::core::events::TrackpadEvent& event) {
    // This is now handled in handle_event directly
    return false;
}

bool ViewportNavigationHandler::handle_smart_mouse_event(const voxelux::core::events::SmartMouseEvent& event) {
    // Not implemented yet
    return false;
}

bool ViewportNavigationHandler::handle_keyboard_navigation(const InputEvent& event) {
    // Keyboard shortcuts for viewport navigation
    if (event.type != EventType::KEY_PRESS) {
        return false;
    }
    
    // Numpad shortcuts for standard views (like Blender)
    // Home key to frame all
    // Period key to frame selected
    // etc.
    
    return false;
}

void ViewportNavigationHandler::update_momentum(float delta_time) {
    if (navigator_ && momentum_enabled_) {
        navigator_->update_momentum(delta_time);
    }
}

// Navigation control methods
void ViewportNavigationHandler::start_orbit(const Point2D& start_pos) {
    if (navigator_) {
        navigator_->start_orbit(glm::vec2(start_pos.x, start_pos.y));
        current_mode_ = NavigationMode::Orbit;
    }
}

void ViewportNavigationHandler::start_pan(const Point2D& start_pos) {
    if (navigator_) {
        navigator_->start_pan(glm::vec2(start_pos.x, start_pos.y));
        current_mode_ = NavigationMode::Pan;
    }
}

void ViewportNavigationHandler::start_zoom(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Zoom;
    navigation_start_pos_ = start_pos;
}

void ViewportNavigationHandler::start_dolly(const Point2D& start_pos) {
    current_mode_ = NavigationMode::Dolly;
    navigation_start_pos_ = start_pos;
}

void ViewportNavigationHandler::end_navigation() {
    if (navigator_) {
        if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
            navigator_->end_pan();
        } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
            navigator_->end_orbit();
        }
    }
    current_mode_ = NavigationMode::None;
    is_dragging_ = false;
}

// Stub implementations for removed methods (for compatibility)
void ViewportNavigationHandler::handle_orbit(const Point2D& current_pos, const Point2D& delta) {}
void ViewportNavigationHandler::handle_pan(const Point2D& current_pos, const Point2D& delta) {}
void ViewportNavigationHandler::handle_pan_with_source(const Point2D& current_pos, const Point2D& delta, EventType source) {}
void ViewportNavigationHandler::handle_pan_simple(const Point2D& current_pos, const Point2D& delta) {}
void ViewportNavigationHandler::handle_zoom(float zoom_delta, const Point2D& mouse_pos) {}
void ViewportNavigationHandler::handle_dolly(const Point2D& current_pos, const Point2D& delta) {}
void ViewportNavigationHandler::reset_pan_state() {}

// NavigationUtils implementation
InputDevice NavigationUtils::detect_input_device() {
    // Platform-specific detection would go here
    // For now, default to trackpad on macOS
#ifdef __APPLE__
    return InputDevice::Trackpad;
#else
    return InputDevice::Mouse;
#endif
}

bool NavigationUtils::is_trackpad_available() {
#ifdef __APPLE__
    return true;  // Most Macs have trackpads
#else
    return false;
#endif
}

bool NavigationUtils::is_smart_mouse_available() {
    // Would need to check for Magic Mouse or similar
    return false;
}

float NavigationUtils::calculate_orbit_sensitivity(InputDevice device) {
    switch (device) {
        case InputDevice::Trackpad:
            return 0.5f;
        case InputDevice::SmartMouse:
            return 0.6f;
        case InputDevice::Tablet:
            return 0.4f;
        default:
            return 1.0f;
    }
}

float NavigationUtils::calculate_pan_sensitivity(InputDevice device, float camera_distance) {
    float base = 1.0f;
    switch (device) {
        case InputDevice::Trackpad:
            base = 0.7f;  // Trackpads are more sensitive
            break;
        case InputDevice::SmartMouse:
            base = 0.8f;
            break;
        default:
            base = 1.0f;
    }
    
    // Scale by camera distance for consistent feel
    return base * std::log10(std::max(1.0f, camera_distance));
}

float NavigationUtils::calculate_zoom_sensitivity(InputDevice device) {
    switch (device) {
        case InputDevice::Trackpad:
            return 0.05f;  // Trackpad scroll is continuous
        case InputDevice::SmartMouse:
            return 0.07f;
        default:
            return 0.1f;   // Mouse wheel is discrete
    }
}

} // namespace voxel_canvas