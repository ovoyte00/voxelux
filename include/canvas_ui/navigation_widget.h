/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Navigation Widget - Professional viewport orientation control
 */

#pragma once

#include "canvas_ui/canvas_core.h"
#include "canvas_ui/camera_3d.h"
#include <array>
#include <vector>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;
class FontSystem;

// Simple 3D point for widget use
struct Point3D {
    float x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

/**
 * 3D Navigation Widget - Professional axis orientation control
 * Renders in screen space with depth-sorted axis indicators
 */
class NavigationWidget {
public:
    NavigationWidget();
    ~NavigationWidget();
    
    // Initialize/cleanup
    bool initialize();
    void cleanup();
    
    // Rendering
    void render(CanvasRenderer* renderer, const Camera3D& camera, const Rect2D& viewport);
    
    // Interaction
    int hit_test(const Point2D& mouse_pos) const;
    void handle_click(int axis_id);
    void set_hover_axis(int axis_id);
    
    // Configuration
    void set_position(const Point2D& pos) { position_ = pos; }
    void set_size(float size) { widget_size_ = size; }
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    // Callbacks
    using AxisClickCallback = std::function<void(int axis, bool positive)>;
    void set_axis_click_callback(AxisClickCallback callback) { axis_click_callback_ = callback; }
    
private:
    // Axis sphere data
    struct AxisSphere {
        int axis;           // 0=X, 1=Y, 2=Z
        bool positive;      // true for positive axis, false for negative
        float depth;        // Camera depth for sorting
        Point3D position;   // 3D position relative to widget center
        ColorRGBA color;    // Axis color
        float alpha;        // Alpha based on depth
    };
    
    // Line data for axis indicators
    struct AxisLine {
        Point3D start;
        Point3D end;
        ColorRGBA color;
        float width;
        float depth;
    };
    
    // Rendering pipeline stages
    void calculate_view_ordering(const Camera3D& camera);
    void draw_widget_backdrop(CanvasRenderer* renderer);
    void draw_axis_connectors(CanvasRenderer* renderer);
    void draw_orientation_indicators(CanvasRenderer* renderer);
    void draw_axis_text(CanvasRenderer* renderer);
    
    // Visual effect calculations
    ColorRGBA calculate_depth_fade(const ColorRGBA& base_color, float depth);
    float calculate_size_from_depth(float base_size, float depth);
    
    // Custom rendering utilities
    void render_filled_circle(CanvasRenderer* renderer, const Point2D& center, 
                             float radius, const ColorRGBA& color);
    void render_ring_shape(CanvasRenderer* renderer, const Point2D& center,
                          float radius, const ColorRGBA& color, float thickness);
    void render_tapered_connector(CanvasRenderer* renderer, const Point2D& origin, 
                                 const Point2D& endpoint, const ColorRGBA& start_tint, 
                                 const ColorRGBA& end_tint, float thickness);
    
    // Screen space transformation
    Point2D project_to_screen(const Point3D& pos, const Camera3D& camera) const;
    
private:
    // Widget state
    bool visible_ = true;
    bool initialized_ = false;
    Point2D position_;      // Screen position (top-right by default)
    float widget_base_size_ = 40.0f;  // Base widget radius (will be scaled by UI scale factor)
    float widget_size_ = 40.0f;       // Actual widget radius after scaling
    float margin_ = 10.0f;             // Margin from screen edges
    
    // Interaction state
    int hovered_axis_ = -1;
    bool is_active_ = false;
    
    // Axis configuration
    static constexpr int NUM_AXES = 6;  // +X, -X, +Y, -Y, +Z, -Z
    std::array<AxisSphere, NUM_AXES> axis_spheres_;
    std::vector<AxisLine> axis_lines_;
    int camera_aligned_axis_ = -1;  // Track which axis aligns with view direction
    
    // Sorted indices for depth rendering
    std::array<int, NUM_AXES> sorted_sphere_indices_;
    std::array<int, 3> sorted_line_indices_;
    
    // Visual theme colors
    ColorRGBA x_axis_color_ = {0.871f, 0.314f, 0.314f, 1.0f};  // Red theme
    ColorRGBA y_axis_color_ = {0.494f, 0.753f, 0.251f, 1.0f};  // Green theme
    ColorRGBA z_axis_color_ = {0.357f, 0.518f, 0.949f, 1.0f};  // Blue theme
    ColorRGBA backdrop_color_ = {0.2f, 0.2f, 0.2f, 0.5f};
    ColorRGBA highlight_backdrop_ = {0.3f, 0.3f, 0.3f, 0.7f};
    ColorRGBA viewport_bg_ = {0.169f, 0.169f, 0.169f, 1.0f}; // Viewport background
    
    // Rendering parameters (professional viewport orientation indicator)
    float axis_distance_ = 0.80f;      // Distance from center to axis indicators
    float sphere_radius_ = 0.20f;      // Size of axis indicators (20% of widget)
    float line_thickness_ = 2.0f;      // Axis connector thickness in pixels
    float label_size_ = 14.0f;         // Axis label font size
    float alignment_threshold_ = 0.01f; // Camera-axis alignment detection threshold
    
    // Shader and buffers
    unsigned int shader_program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    
    // Vertex data
    std::vector<float> vertex_buffer_;
    
    // Callback
    AxisClickCallback axis_click_callback_;
    
    // Current viewport for screen space calculations
    Rect2D current_viewport_;
};

} // namespace voxel_canvas