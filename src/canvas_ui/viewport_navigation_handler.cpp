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
#include "canvas_ui/canvas_window.h"
#include <GLFW/glfw3.h>
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
    if (navigator_) {
        navigator_->set_ui_scale(scale);
    }
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
    
    /* Input Scheme Summary:
     * 
     * ROTATION:
     * - Middle mouse drag: Orbit
     * - Trackpad: Two-finger rotate gesture
     * - Smart Mouse: One-finger rotate gesture
     * 
     * PANNING:
     * - Shift + Middle mouse drag: Pan
     * - Shift + Trackpad: Two-finger scroll (H & V)
     * - Shift + Smart Mouse: One-finger scroll (H & V)
     * 
     * ZOOM:
     * - Mouse wheel (no modifiers): Zoom
     * - Trackpad: Two-finger pinch gesture
     * - CMD/CTRL + Trackpad: Two-finger vertical scroll
     * - CMD/CTRL + Smart Mouse: One-finger vertical scroll
     */
    
    // Handle modifier change events
    if (event.type == EventType::MODIFIER_CHANGE) {
        bool shift_held = event.has_modifier(KeyModifier::SHIFT);
        bool cmd_held = event.has_modifier(KeyModifier::CMD);
        bool ctrl_held = event.has_modifier(KeyModifier::CTRL);
        
        std::cout << "[MOD_DEBUG] MODIFIER_CHANGE - shift=" << shift_held 
                  << ", cmd=" << cmd_held << ", ctrl=" << ctrl_held
                  << ", current_mode=" << static_cast<int>(current_mode_)
                  << ", nav_mode=" << (navigator_ ? static_cast<int>(navigator_->get_mode()) : -1)
                  << std::endl;
        
        // Check if shift was released during pan
        if (!shift_held && navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
            std::cout << "[MOD_DEBUG] Shift released - ending PAN mode" << std::endl;
            navigator_->end_pan();
            current_mode_ = NavigationMode::None;
        }
        // Check if shift was pressed during orbit - switch to pan
        if (shift_held && navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
            std::cout << "[MOD_DEBUG] Shift pressed - switching from ROTATE to PAN" << std::endl;
            navigator_->end_orbit();
            navigator_->start_pan(glm::vec2(last_mouse_pos_.x, last_mouse_pos_.y));
            current_mode_ = NavigationMode::Pan;
        }
        
        // Check if CMD/CTRL was released while we were zooming
        if (!cmd_held && !ctrl_held && current_mode_ == NavigationMode::Zoom) {
            std::cout << "[MOD_DEBUG] CMD/CTRL released - STOPPING ALL ZOOM PROCESSING" << std::endl;
            current_mode_ = NavigationMode::None;
            
            // Clear any accumulated zoom delta
            accumulated_zoom_delta_ = 0.0f;
            last_zoom_accumulation_time_ = 0.0;
            
            // Clear any momentum in the navigator
            if (navigator_) {
                navigator_->clear_momentum();
            }
            
            std::cout << "[MOD_DEBUG] Cleared all zoom state and momentum" << std::endl;
        }
        
        // Also handle shift release during pan
        if (!shift_held && current_mode_ == NavigationMode::Pan) {
            std::cout << "[MOD_DEBUG] Shift released - STOPPING ALL PAN PROCESSING" << std::endl;
            current_mode_ = NavigationMode::None;
            
            // End pan in navigator
            if (navigator_ && navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                navigator_->end_pan();
            }
            
            // Clear momentum
            if (navigator_) {
                navigator_->clear_momentum();
            }
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
                // This event should only come when shift is held
                
                // End any other navigation mode first
                if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                    std::cout << "[NavDebug] Ending ORBIT mode to start PAN (trackpad pan event)" << std::endl;
                    navigator_->end_orbit();
                } else if (current_mode_ == NavigationMode::Zoom) {
                    // Clear zoom mode
                    std::cout << "[NavDebug] Ending ZOOM mode to start PAN (trackpad pan event)" << std::endl;
                    current_mode_ = NavigationMode::None;
                }
                
                // Start pan if not already panning
                if (navigator_->get_mode() != voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                    std::cout << "[NavDebug] Starting PAN mode (trackpad two-finger pan)" << std::endl;
                    navigator_->start_pan(mouse_pos);
                    current_mode_ = NavigationMode::Pan;
                }
                
                // Update pan with delta (trackpad gives us deltas, not positions)
                // On macOS with natural scrolling, pan deltas need to be inverted
                // because natural scroll inverts the direction for scroll but pan should follow finger
                glm::vec2 pan_delta = mouse_delta;
                if (event.trackpad.direction_inverted) {
                    // With natural scrolling, invert the deltas so pan follows finger movement
                    pan_delta = -mouse_delta;
                }
                std::cout << "[NavDebug] PAN update - raw delta: (" << mouse_delta.x << ", " << mouse_delta.y 
                          << "), adjusted delta: (" << pan_delta.x << ", " << pan_delta.y << ")" << std::endl;
                navigator_->update_pan_delta(pan_delta, 0.016f);  // ~60 FPS delta time
                return EventResult::HANDLED;
            }
            
        case EventType::TRACKPAD_ROTATE:
            {
                // Rotation gestures:
                // - Trackpad: Two-finger rotate gesture
                // - Smart Mouse: One-finger rotate gesture (e.g., Apple Magic Mouse)
                // Both come through this same event type
                bool shift_held = event.has_modifier(KeyModifier::SHIFT);
                bool cmd_ctrl_held = event.has_modifier(KeyModifier::CMD) || event.has_modifier(KeyModifier::CTRL);
                
                // Trackpad rotate event received
                
                // Only handle rotation if NO modifiers are held
                // This prevents conflicts with shift+pan and CMD+zoom operations
                if (!shift_held && !cmd_ctrl_held) {
                    // End any pan operation first
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                        std::cout << "[NavDebug] Ending PAN mode to start ROTATE" << std::endl;
                        navigator_->end_pan();
                    }
                    
                    // Start orbit if not already orbiting
                    if (navigator_->get_mode() != voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        std::cout << "[NavDebug] Starting ROTATE mode (trackpad/smart mouse gesture)" << std::endl;
                        navigator_->start_orbit(mouse_pos);
                        current_mode_ = NavigationMode::Orbit;
                        last_mouse_pos_ = event.mouse_pos;  // Reset tracking position
                    }
                    
                    // For trackpad/smart mouse rotate, use delta directly with device-specific sensitivity
                    bool is_smart_mouse = event.trackpad.is_smart_mouse;
                    // Processing rotation
                    navigator_->update_orbit_delta(mouse_delta, 0.016f, is_smart_mouse);
                    return EventResult::HANDLED;
                } else {
                    // If modifiers are held during rotation gesture, end any active orbit
                    // This prevents rotation events from accumulating during zoom
                    // Rotation blocked due to modifiers
                    
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        // Force-ending orbit mode due to modifier
                        navigator_->end_orbit();
                        current_mode_ = NavigationMode::None;
                    }
                    // Consume the event to prevent accumulation
                    // Event consumed to prevent accumulation
                    return EventResult::HANDLED;
                }
            }
            
        case EventType::TRACKPAD_ZOOM:
            {
                // Trackpad zoom (two-finger pinch gesture)
                // Note: Smart Mouse doesn't support pinch gestures
                // Pinch should override other operations (most intuitive)
                
                // End any active navigation first
                if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                    std::cout << "[NavDebug] Ending PAN mode for ZOOM (pinch gesture)" << std::endl;
                    navigator_->end_pan();
                } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                    std::cout << "[NavDebug] Ending ROTATE mode for ZOOM (pinch gesture)" << std::endl;
                    navigator_->end_orbit();
                }
                
                // Pass raw delta - navigator handles sensitivity
                std::cout << "[NavDebug] ZOOM (pinch) - delta: " << event.wheel_delta << std::endl;
                navigator_->zoom(event.wheel_delta, mouse_pos);
                current_mode_ = NavigationMode::Zoom;
                return EventResult::HANDLED;
            }
            
        case EventType::TRACKPAD_SCROLL:
            {
                // Scroll gestures - check for modifiers:
                // - Trackpad: Two-finger scroll (horizontal and vertical)
                // - Smart Mouse: One-finger scroll (horizontal and vertical)
                // Both devices send events through this same event type
                bool shift_held = event.has_modifier(KeyModifier::SHIFT);
                bool cmd_ctrl_held = event.has_modifier(KeyModifier::CMD) || event.has_modifier(KeyModifier::CTRL);
                
                if (shift_held) {
                    // Shift + scroll = Pan (both horizontal and vertical)
                    // - Trackpad: Shift + two-finger scroll
                    // - Smart Mouse: Shift + one-finger scroll
                    if (navigator_->get_mode() != voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                        std::cout << "[NavDebug] Starting PAN mode (shift + trackpad/smart mouse scroll)" << std::endl;
                        navigator_->start_pan(mouse_pos);
                        current_mode_ = NavigationMode::Pan;
                    }
                    // For trackpad scroll, use mouse_delta for both X and Y
                    // mouse_delta contains the actual 2D scroll values
                    float pan_x = event.mouse_delta.x * 10.0f;
                    float pan_y = event.mouse_delta.y * 10.0f;  // Use mouse_delta.y for vertical
                    
                    // Invert for natural scrolling so pan follows finger
                    if (event.trackpad.direction_inverted) {
                        pan_x = -pan_x;
                        pan_y = -pan_y;
                    }
                    
                    glm::vec2 pan_delta(pan_x, pan_y);
                    std::cout << "[NavDebug] PAN (shift+scroll) - delta: (" << pan_x << ", " << pan_y << ")" << std::endl;
                    navigator_->update_pan_delta(pan_delta, 0.016f);
                } else if (cmd_ctrl_held) {
                    // CMD/CTRL + trackpad/smart mouse scroll = Zoom (vertical only)
                    // Always end any active orbit when CMD/CTRL is detected
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        std::cout << "[NavDebug] Ending ORBIT mode for ZOOM (CMD/CTRL detected)" << std::endl;
                        navigator_->end_orbit();
                        current_mode_ = NavigationMode::None;
                    }
                    
                    // Accumulate small deltas to avoid event spam
                    const float MIN_ZOOM_DELTA = 5.0f;  // Minimum accumulated delta before zooming
                    const double ZOOM_TIMEOUT = 0.1;    // Reset accumulator after 100ms
                    
                    double now = glfwGetTime();
                    
                    // Reset accumulator if too much time has passed
                    if (now - last_zoom_accumulation_time_ > ZOOM_TIMEOUT) {
                        accumulated_zoom_delta_ = 0.0f;
                        // End any active navigation modes when starting fresh zoom
                        if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                            std::cout << "[NavDebug] Ending PAN mode for ZOOM (CMD/CTRL + scroll)" << std::endl;
                            navigator_->end_pan();
                        }
                    }
                    
                    // Use both vertical and horizontal scroll components for zoom
                    // Users often scroll diagonally on trackpads
                    float zoom_delta = 0.0f;
                    if (event.mouse_delta.y != 0 || event.mouse_delta.x != 0) {
                        // Check if we need to invert for natural scrolling
                        // With natural scrolling: scroll up (negative y) = zoom in (positive delta)
                        // Without natural: scroll up (negative y) = zoom out (negative delta)
                        float invert = event.trackpad.direction_inverted ? 1.0f : -1.0f;
                        
                        // Combine both axes - use the larger magnitude for zoom
                        // This feels natural for diagonal scrolling
                        float vertical = invert * event.mouse_delta.y;
                        // Horizontal should be opposite - scroll right = zoom in (always)
                        float horizontal = -invert * event.mouse_delta.x;
                        
                        // Use the component with larger magnitude, or combine them if similar
                        if (std::abs(vertical) > std::abs(horizontal) * 1.5f) {
                            zoom_delta = vertical;  // Primarily vertical
                        } else if (std::abs(horizontal) > std::abs(vertical) * 1.5f) {
                            zoom_delta = horizontal;  // Primarily horizontal
                        } else {
                            // Diagonal - average both components
                            zoom_delta = (vertical + horizontal) * 0.7f;  // Scale down slightly for diagonal
                        }
                    } else {
                        // Mouse wheel doesn't have natural scrolling (it's a physical wheel)
                        zoom_delta = -event.wheel_delta;
                    }
                    
                    // Accumulate the delta
                    accumulated_zoom_delta_ += zoom_delta;
                    last_zoom_accumulation_time_ = now;
                    
                    // Debug: show accumulation
                    static int zoom_debug_count = 0;
                    if (++zoom_debug_count % 10 == 0) {  // Log every 10th event
                        std::cout << "[NavDebug] ZOOM accumulating - delta: " << zoom_delta 
                                  << ", accumulated: " << accumulated_zoom_delta_ << std::endl;
                    }
                    
                    // Only zoom if we've accumulated enough delta
                    if (std::abs(accumulated_zoom_delta_) >= MIN_ZOOM_DELTA) {
                        std::cout << "[NavDebug] ZOOM executing - accumulated delta: " << accumulated_zoom_delta_ << std::endl;
                        navigator_->zoom(accumulated_zoom_delta_, mouse_pos);
                        current_mode_ = NavigationMode::Zoom;
                        accumulated_zoom_delta_ = 0.0f;  // Reset after zooming
                    }
                } else {
                    // No modifiers with trackpad scroll
                    // For now, ignore trackpad scroll without modifiers
                    // Rotation should come through TRACKPAD_ROTATE event
                    // Zoom should use CMD+scroll or mouse wheel
                    return EventResult::IGNORED;
                }
                return EventResult::HANDLED;
            }
            
        case EventType::MOUSE_WHEEL:
            {
                // Mouse wheel (no modifiers) = standard zoom
                bool has_modifiers = event.has_modifier(KeyModifier::SHIFT) || 
                                    event.has_modifier(KeyModifier::CTRL) || 
                                    event.has_modifier(KeyModifier::CMD) ||
                                    event.has_modifier(KeyModifier::ALT);
                
                if (!has_modifiers) {
                    // No modifiers = Zoom
                    // End any active navigation first
                    if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN) {
                        std::cout << "[NavDebug] Ending PAN mode for ZOOM (mouse wheel)" << std::endl;
                        navigator_->end_pan();
                    } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        std::cout << "[NavDebug] Ending ROTATE mode for ZOOM (mouse wheel)" << std::endl;
                        navigator_->end_orbit();
                    }
                    
                    // Pass raw delta - navigator handles sensitivity
                    std::cout << "[NavDebug] ZOOM (mouse wheel) - delta: " << event.wheel_delta << std::endl;
                    navigator_->zoom(event.wheel_delta, mouse_pos);
                    current_mode_ = NavigationMode::Zoom;
                    return EventResult::HANDLED;
                }
                // Ignore mouse wheel with modifiers
                return EventResult::IGNORED;
            }
            
        case EventType::MOUSE_PRESS:
            {
                // Check for middle mouse button
                if (event.mouse_button == MouseButton::MIDDLE) {
                    bool shift_held = event.has_modifier(KeyModifier::SHIFT);
                    
                    if (shift_held) {
                        // Shift + Middle Mouse = Pan
                        std::cout << "[NavDebug] Starting PAN mode (shift + middle mouse)" << std::endl;
                        navigator_->start_pan(mouse_pos);
                        current_mode_ = NavigationMode::Pan;
                        is_dragging_ = true;
                    } else {
                        // Middle Mouse = Orbit
                        std::cout << "[NavDebug] Starting ROTATE mode (middle mouse)" << std::endl;
                        
                        // Store initial position for restoration
                        drag_start_pos_ = Point2D(mouse_pos.x, mouse_pos.y);
                        
                        navigator_->start_orbit(mouse_pos);
                        current_mode_ = NavigationMode::Orbit;
                        is_dragging_ = true;
                        
                        // Capture cursor for infinite dragging (Blender-style)
                        // Following Blender's approach: ALWAYS capture cursor for rotation
                        // The navigator will handle the initial warp properly
                        if (window_) {
                            window_->capture_cursor();
                        }
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
                        std::cout << "[NavDebug] Ending PAN mode (mouse release)" << std::endl;
                        navigator_->end_pan();
                    } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        std::cout << "[NavDebug] Ending ROTATE mode (mouse release)" << std::endl;
                        navigator_->end_orbit();
                        
                        // Release cursor and restore position
                        if (window_) {
                            window_->release_cursor();
                        }
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
                        std::cout << "[NavDebug] PAN update (mouse move) - pos: (" << mouse_pos.x << ", " << mouse_pos.y << ")" << std::endl;
                        navigator_->update_pan(mouse_pos, 0.016f);
                    } else if (navigator_->get_mode() == voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT) {
                        std::cout << "[NavDebug] ROTATE update (mouse move) - pos: (" << mouse_pos.x << ", " << mouse_pos.y << ")" << std::endl;
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

// Debug methods implementation
void ViewportNavigationHandler::print_current_state() const {
    const char* mode_str = get_mode_string();
    const char* device_str = "Unknown";
    
    switch (input_device_) {
        case InputDevice::Mouse: device_str = "Mouse"; break;
        case InputDevice::Trackpad: device_str = "Trackpad"; break;
        case InputDevice::SmartMouse: device_str = "SmartMouse"; break;
        case InputDevice::Tablet: device_str = "Tablet"; break;
        default: break;
    }
    
    std::cout << "[NavDebug] === Current State ===" << std::endl;
    std::cout << "[NavDebug] Mode: " << mode_str << std::endl;
    std::cout << "[NavDebug] Device: " << device_str << std::endl;
    std::cout << "[NavDebug] Dragging: " << (is_dragging_ ? "Yes" : "No") << std::endl;
    
    if (navigator_) {
        auto nav_mode = navigator_->get_mode();
        const char* nav_mode_str = "NONE";
        switch (nav_mode) {
            case voxelux::canvas_ui::ViewportNavigator::NavigationMode::PAN:
                nav_mode_str = "PAN";
                break;
            case voxelux::canvas_ui::ViewportNavigator::NavigationMode::ORBIT:
                nav_mode_str = "ORBIT";
                break;
            case voxelux::canvas_ui::ViewportNavigator::NavigationMode::ZOOM:
                nav_mode_str = "ZOOM";
                break;
            default:
                break;
        }
        std::cout << "[NavDebug] Navigator Mode: " << nav_mode_str << std::endl;
    }
    
    std::cout << "[NavDebug] ===================" << std::endl;
}

const char* ViewportNavigationHandler::get_mode_string() const {
    switch (current_mode_) {
        case NavigationMode::Orbit: return "ROTATE/ORBIT";
        case NavigationMode::Pan: return "PAN";
        case NavigationMode::Zoom: return "ZOOM";
        case NavigationMode::Dolly: return "DOLLY";
        case NavigationMode::None: return "NONE";
        default: return "UNKNOWN";
    }
}

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