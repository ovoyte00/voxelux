/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Core system architecture
 * Original OpenGL-based UI system inspired by professional 3D applications
 * but completely independently implemented for voxel editing workflows.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasWindow;
class CanvasRegion;
class EditorSpace;
class EventRouter;
class CanvasRenderer;

// Core types
using RegionID = uint32_t;
using EditorID = uint32_t;

// Geometric primitives for UI layout
struct Point2D {
    float x, y;
    Point2D(float x = 0, float y = 0) : x(x), y(y) {}
    Point2D operator+(const Point2D& other) const { return {x + other.x, y + other.y}; }
    Point2D operator-(const Point2D& other) const { return {x - other.x, y - other.y}; }
    Point2D operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Point2D operator/(float scalar) const { return {x / scalar, y / scalar}; }
    Point2D& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Point2D& operator+=(const Point2D& other) { x += other.x; y += other.y; return *this; }
};

struct Rect2D {
    float x, y, width, height;
    
    Rect2D(float x = 0, float y = 0, float w = 0, float h = 0) 
        : x(x), y(y), width(w), height(h) {}
    
    float right() const { return x + width; }
    float bottom() const { return y + height; }
    bool contains(const Point2D& point) const;
    bool intersects(const Rect2D& other) const;
    Rect2D intersection(const Rect2D& other) const;
};

// Color system for professional theming
struct ColorRGBA {
    float r, g, b, a;
    
    ColorRGBA(float r = 0, float g = 0, float b = 0, float a = 1) 
        : r(r), g(g), b(b), a(a) {}
    
    // Hex constructor: ColorRGBA("#2b2b2b")
    explicit ColorRGBA(const std::string& hex);
    
    // Common colors
    static ColorRGBA white() { return {1, 1, 1, 1}; }
    static ColorRGBA black() { return {0, 0, 0, 1}; }
    static ColorRGBA transparent() { return {0, 0, 0, 0}; }
    
    // Comparison operators
    bool operator==(const ColorRGBA& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const ColorRGBA& other) const {
        return !(*this == other);
    }
};

// Professional theme for voxel editing (matches voxelux_complete_styles.md)
struct CanvasTheme {
    // Base Color Palette - Gray scale foundation
    ColorRGBA gray_1{"#F2F2F2"};  // Lightest - primary text, active icons
    ColorRGBA gray_2{"#B4B4B4"};  // Light - secondary text, inactive elements
    ColorRGBA gray_3{"#575757"};  // Medium - panel backgrounds, active states
    ColorRGBA gray_4{"#393939"};  // Dark - main application background
    ColorRGBA gray_5{"#303030"};  // Darkest - borders, headers, footers
    
    // Axis Colors (3D Navigation)
    ColorRGBA axis_x{"#BC4252"};  // Red - X axis
    ColorRGBA axis_y{"#6CAC34"};  // Green - Y axis
    ColorRGBA axis_z{"#3B83BE"};  // Blue - Z axis
    
    // Semantic Colors
    ColorRGBA accent_primary{"#3B83BE"};   // Primary actions, selections, playhead
    ColorRGBA accent_secondary{"#6CAC34"}; // Secondary actions, success states
    ColorRGBA accent_warning{"#BC4252"};   // Warnings, errors, unsaved indicators
    
    // Legacy mappings for compatibility
    ColorRGBA background_primary = gray_4;     // Main background
    ColorRGBA background_secondary = gray_5;   // Headers, panels
    ColorRGBA accent_selection = accent_primary;
    ColorRGBA text_primary = gray_1;
    ColorRGBA text_secondary = gray_2;
    ColorRGBA border_normal = gray_5;
    
    // Grid colors (loaded from assets/styles/colors.json)
    ColorRGBA grid_major_lines{"#5a5a5a"};     // Major grid lines - clearly visible (90, 90, 90)
    ColorRGBA grid_minor_lines{"#4a4a4a"};     // Minor grid lines - subtle but visible (74, 74, 74)
    ColorRGBA grid_origin{"#FFFFFF"};          // Origin highlight
    ColorRGBA grid_subtle{"#3c3c3c"};          // Grid lines
    
    // Widget states
    ColorRGBA widget_normal = gray_5;
    ColorRGBA widget_hover = gray_4;
    ColorRGBA widget_active = accent_primary;
    ColorRGBA widget_disabled{"#57575733"};  // gray_3 with 0.2 opacity
    
    // Layout Dimensions (from style guide)
    float menu_height = 25.0f;
    float footer_height = 25.0f;
    float file_bar_height = 25.0f;
    float editor_header_height = 25.0f;
    float dock_header_height = 14.0f;
    float panel_tab_height = 25.0f;
    float timeline_ruler_height = 25.0f;
    float timeline_track_height = 30.0f;
    
    // Dock System Dimensions
    float dock_collapsed_width = 36.0f;
    float dock_collapsed_max = 225.0f;
    float dock_expanded_width = 250.0f;
    float dock_expanded_min = 250.0f;
    float dock_expanded_max = 500.0f;
    float tool_dock_expanded = 72.0f;
    
    // Component Dimensions
    float button_height = 22.0f;
    float button_width = 22.0f;
    float button_border_radius = 3.0f;
    float tool_icon_size = 32.0f;
    float tool_icon_inner = 18.0f;
    float tab_close_size = 16.0f;
    float tab_min_width = 120.0f;
    float tab_max_width = 200.0f;
    float dropdown_height = 19.0f;
    float input_height = 19.0f;
    
    // Spacing System
    float spacing_xs = 2.0f;   // Tight spacing
    float spacing_sm = 4.0f;   // Small spacing
    float spacing_md = 8.0f;   // Medium spacing
    float spacing_lg = 10.0f;  // Large spacing
    float spacing_xl = 20.0f;  // Extra large
    
    // Split View System
    float resize_handle_width = 2.0f;
    float resize_handle_hit_area = 4.0f;
    float split_min_size = 100.0f;
    
    // Timeline Component
    float timeline_default_height = 200.0f;
    float timeline_min_height = 100.0f;
    float timeline_playhead_width = 2.0f;
    float keyframe_size = 10.0f;
    float track_header_width = 120.0f;
    
    // Navigation Widget
    float nav_widget_size = 80.0f;
    float nav_cube_face_size = 60.0f;
    
    // Scrollbars
    float scrollbar_width = 8.0f;
    float scrollbar_thumb_radius = 4.0f;
    
    // Transitions (in seconds)
    float transition_fast = 0.15f;
    float transition_medium = 0.2f;
    float transition_slow = 0.3f;
    
    // Legacy compatibility
    float panel_header_height = panel_tab_height;
    float splitter_width = resize_handle_width;
    float padding_small = spacing_sm;
    float padding_normal = spacing_md;
    float padding_large = spacing_lg;
};

// Input event system
enum class EventType {
    MOUSE_PRESS,
    MOUSE_RELEASE, 
    MOUSE_MOVE,
    MOUSE_WHEEL,
    KEY_PRESS,
    KEY_RELEASE,
    WINDOW_RESIZE,
    WINDOW_CLOSE,
    TRACKPAD_SCROLL,
    TRACKPAD_PAN,
    TRACKPAD_ZOOM,
    TRACKPAD_ROTATE,
    SMART_MOUSE_GESTURE,
    MODIFIER_CHANGE  // Special event for when modifier keys change
};

enum class MouseButton {
    LEFT = 0,
    RIGHT = 1, 
    MIDDLE = 2
};

enum class KeyModifier {
    NONE = 0,
    SHIFT = 1,
    CTRL = 2,
    ALT = 4,
    CMD = 8  // macOS Command key
};

struct InputEvent {
    EventType type;
    Point2D mouse_pos;
    Point2D mouse_delta;
    MouseButton mouse_button;
    int key_code;
    uint32_t modifiers; // Bitfield of KeyModifier values
    float wheel_delta;
    double timestamp;
    
    // Extended data for trackpad and smart mouse events
    struct TrackpadData {
        float scale_factor = 1.0f;
        float rotation_angle = 0.0f;
        bool direction_inverted = false;
        bool is_smart_mouse = false;  // true for smart mouse, false for regular trackpad
    } trackpad;
    
    struct SmartMouseData {
        float pressure = 0.0f;
        int gesture_type = 0; // Maps to SmartMouseEvent::Gesture
    } smart_mouse;
    
    // Convenience methods
    bool has_modifier(KeyModifier mod) const { return (modifiers & static_cast<uint32_t>(mod)) != 0; }
    bool is_mouse_event() const;
    bool is_keyboard_event() const;
    bool is_trackpad_event() const;
    bool is_smart_mouse_event() const;
};

// Event handling result
enum class EventResult {
    HANDLED,     // Event was processed, stop propagation
    IGNORED,     // Event was ignored, continue propagation  
    PASS_THROUGH // Event partially handled, continue propagation
};

// Input handler interface
class InputHandler {
public:
    virtual ~InputHandler() = default;
    
    // Check if this handler can process the event
    virtual bool can_handle(const InputEvent& event) const = 0;
    
    // Process the event
    virtual EventResult handle_event(const InputEvent& event) = 0;
    
    // Handler priority (higher = processed first)
    virtual int get_priority() const { return 0; }
};

} // namespace voxel_canvas