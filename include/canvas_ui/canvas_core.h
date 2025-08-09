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
};

// Professional theme for voxel editing
struct CanvasTheme {
    // Base colors (Blender-inspired but original)
    ColorRGBA background_primary{"#2b2b2b"};   // Main background
    ColorRGBA background_secondary{"#323232"}; // Headers, panels
    ColorRGBA accent_selection{"#5680c2"};     // Selection highlight  
    ColorRGBA text_primary{"#cccccc"};         // Main text
    ColorRGBA text_secondary{"#999999"};       // Secondary text
    ColorRGBA grid_subtle{"#3c3c3c"};          // Grid lines
    ColorRGBA border_normal{"#555555"};        // Panel borders
    
    // Grid colors (loaded from assets/styles/colors.json)
    ColorRGBA grid_major_lines{"#5a5a5a"};     // Major grid lines - clearly visible (90, 90, 90)
    ColorRGBA grid_minor_lines{"#3a3a3a"};     // Minor grid lines - subtle but visible (58, 58, 58)
    ColorRGBA grid_origin{"#FFFFFF"};          // Origin highlight
    ColorRGBA axis_x_color{"#BC4252"};         // X-axis red
    ColorRGBA axis_y_color{"#6CAC34"};         // Y-axis green
    ColorRGBA axis_z_color{"#3B83BE"};         // Z-axis blue
    
    // Widget states
    ColorRGBA widget_normal{"#404040"};
    ColorRGBA widget_hover{"#4a4a4a"};
    ColorRGBA widget_active{"#5680c2"};
    ColorRGBA widget_disabled{"#2a2a2a"};
    
    // UI dimensions
    float panel_header_height = 24.0f;
    float splitter_width = 4.0f;
    float button_height = 20.0f;
    float padding_small = 4.0f;
    float padding_normal = 8.0f;
    float padding_large = 16.0f;
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
    SMART_MOUSE_GESTURE
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