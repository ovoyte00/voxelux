/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Viewport Editor for Canvas UI system
 * Professional 3D editing viewport with grid, navigation controls, and voxel rendering
 */

#pragma once

#include "canvas_region.h"
#include "canvas_renderer.h"
#include "grid_3d_renderer.h"
#include <memory>

namespace voxel_canvas {

class ViewportGrid;
class ViewportNavWidget;

// Camera state structure for 3D viewport
struct CameraState {
    Point2D rotation{0, 0};         // Pitch, Yaw in degrees
    float distance = 10.0f;         // Distance from target
    Point2D target{0, 0};           // Look-at target (2D for now)
    bool orthographic = false;
    float ortho_scale = 1.0f;
};

/**
 * 3D Viewport editor for voxel editing
 * Integrates grid rendering, navigation controls, and 3D scene rendering
 */
class Viewport3DEditor : public EditorSpace {
public:
    Viewport3DEditor();
    ~Viewport3DEditor() override;
    
    // EditorSpace interface
    std::string get_name() const override { return "3D Viewport"; }
    std::string get_icon() const override { return "VIEW_3D"; }
    
    void initialize() override;
    void shutdown() override;
    void update(float delta_time) override;
    
    void render(CanvasRenderer* renderer, const Rect2D& bounds) override;
    void render_overlay(CanvasRenderer* renderer, const Rect2D& bounds) override;
    
    bool handle_event(const InputEvent& event, const Rect2D& bounds) override;
    bool wants_keyboard_focus() const override { return true; }
    bool wants_mouse_capture() const override { return true; }
    
    // Viewport-specific functionality
    void set_grid_visible(bool visible) { grid_visible_ = visible; }
    bool is_grid_visible() const { return grid_visible_; }
    
    void set_nav_widget_visible(bool visible) { nav_widget_visible_ = visible; }
    bool is_nav_widget_visible() const { return nav_widget_visible_; }
    
    // Camera controls
    void reset_camera_view();
    void frame_all_objects();
    void set_orthographic_view(int view); // 1=front, 2=right, 3=top, etc.
    
private:
    void setup_camera();
    void setup_grid();
    void setup_nav_widget();
    
    void render_3d_scene(CanvasRenderer* renderer, const Rect2D& bounds);
    void render_grid(CanvasRenderer* renderer, const Rect2D& bounds);
    void render_nav_widget(CanvasRenderer* renderer, const Rect2D& bounds);
    
    bool handle_camera_navigation(const InputEvent& event, const Rect2D& bounds);
    bool handle_nav_widget_interaction(const InputEvent& event, const Rect2D& bounds);
    
    // 3D scene components
    std::unique_ptr<Grid3DRenderer> grid_3d_;
    std::unique_ptr<ViewportNavWidget> nav_widget_;
    
    // Camera state (use 3D camera for proper matrices)
    Camera3D camera_;
    
    // Interaction state
    bool is_orbiting_ = false;
    bool is_panning_ = false;
    bool is_zooming_ = false;
    Point2D last_mouse_pos_{0, 0};
    
    // Viewport settings
    bool grid_visible_ = true;
    bool nav_widget_visible_ = true;
    bool show_fps_ = false;
    
    // Performance
    float frame_time_ = 0.0f;
    int frame_count_ = 0;
};

/**
 * Grid rendering for 3D viewport
 * Renders professional grid lines with distance-based fading
 */
class ViewportGrid {
public:
    ViewportGrid();
    ~ViewportGrid();
    
    void initialize();
    void shutdown();
    
    void render(CanvasRenderer* renderer, const Rect2D& bounds, const CameraState& camera);
    
    // Grid settings
    void set_grid_size(float size) { grid_size_ = size; }
    float get_grid_size() const { return grid_size_; }
    
    void set_subdivision_count(int count) { subdivision_count_ = count; }
    int get_subdivision_count() const { return subdivision_count_; }
    
    void set_grid_color(const ColorRGBA& color) { grid_color_ = color; }
    const ColorRGBA& get_grid_color() const { return grid_color_; }
    
private:
    void create_grid_geometry();
    void render_grid_lines(CanvasRenderer* renderer, const Rect2D& bounds, const CameraState& camera);
    void render_axis_lines(CanvasRenderer* renderer, const Rect2D& bounds, const CameraState& camera);
    
    // Grid properties
    float grid_size_ = 1.0f;
    int subdivision_count_ = 10;
    ColorRGBA grid_color_{0.24f, 0.24f, 0.24f, 1.0f}; // Blender-style grid color
    ColorRGBA axis_x_color_{0.8f, 0.2f, 0.2f, 1.0f};  // Red X axis
    ColorRGBA axis_y_color_{0.2f, 0.8f, 0.2f, 1.0f};  // Green Y axis
    ColorRGBA axis_z_color_{0.2f, 0.2f, 0.8f, 1.0f};  // Blue Z axis
    
    // Grid geometry
    struct GridLine {
        Point2D start, end;
        ColorRGBA color;
        float width;
    };
    std::vector<GridLine> grid_lines_;
    bool geometry_dirty_ = true;
};

/**
 * 3D Navigation widget for viewport
 * Professional navigation sphere like Blender's viewport gizmo
 */
class ViewportNavWidget {
public:
    ViewportNavWidget();
    ~ViewportNavWidget();
    
    void initialize();
    void shutdown();
    
    void render(CanvasRenderer* renderer, const Rect2D& bounds, const CameraState& camera);
    bool handle_event(const InputEvent& event, const Rect2D& bounds, CameraState& camera);
    
    // Widget settings
    void set_position(const Point2D& position) { position_ = position; }
    const Point2D& get_position() const { return position_; }
    
    void set_size(float size) { size_ = size; }
    float get_size() const { return size_; }
    
    // Professional navigation widget settings
    void set_axis_colors(const ColorRGBA& x_color, const ColorRGBA& y_color, const ColorRGBA& z_color) {
        axis_colors_[0] = x_color;
        axis_colors_[1] = y_color;
        axis_colors_[2] = z_color;
    }
    
    void set_background_alpha(float alpha) { background_color_.a = alpha; }
    void set_show_axis_labels(bool show) { show_labels_ = show; }
    
private:
    // Professional axis sphere data structure
    struct AxisSphere {
        int axis;          // 0=X, 1=Y, 2=Z
        bool positive;     // true for +X/+Y/+Z, false for -X/-Y/-Z  
        float depth;       // Depth value for sorting
        Point2D position;  // 3D position projected to 2D
        ColorRGBA color;   // Axis color
    };
    
    // Billboard data for screen-space rendering with depth sorting
    struct BillboardData {
        Point2D position;
        float radius;
        ColorRGBA color;
        bool positive;
        float depth;
        int sphere_index;
    };
    
    // Line data for axis lines
    struct LineData {
        Point2D start, end;
        ColorRGBA color;
        float alpha;
    };
    
    void regenerate_sorted_widget(const CameraState& camera);
    void render_navigation_sphere(CanvasRenderer* renderer, const CameraState& camera);
    void render_axis_labels(CanvasRenderer* renderer, const CameraState& camera);
    void render_screen_space_billboards(CanvasRenderer* renderer, const CameraState& camera);
    void render_axis_lines(CanvasRenderer* renderer, const CameraState& camera);
    
    bool is_point_in_widget(const Point2D& point, const Rect2D& bounds) const;
    int get_axis_at_point(const Point2D& point, const Rect2D& bounds, const CameraState& camera) const;
    Point2D project_to_screen(const Point2D& world_pos, const CameraState& camera, const Rect2D& bounds) const;
    
    // Widget properties
    Point2D position_{0, 0};  // Relative position in viewport (0-1)
    float size_ = 80.0f;      // Widget size in pixels
    
    // Professional sphere data (6 axes: +/-X, +/-Y, +/-Z)
    AxisSphere sphere_data_[6];
    std::vector<BillboardData> billboard_data_;
    std::vector<LineData> line_data_;
    
    // Interaction state
    int hovered_axis_ = -1;   // -1=none, 0-5 for the 6 axes
    int pressed_axis_ = -1;
    bool is_dragging_ = false;
    
    // Visual settings
    ColorRGBA background_color_{0.0f, 0.0f, 0.0f, 0.6f};
    ColorRGBA axis_colors_[3] = {
        {0.8f, 0.2f, 0.2f, 1.0f}, // X - Red
        {0.2f, 0.8f, 0.2f, 1.0f}, // Y - Green  
        {0.2f, 0.2f, 0.8f, 1.0f}  // Z - Blue
    };
    
    bool show_labels_ = true;
    float axis_distance_ = 1.0f;
    float sphere_radius_ = 0.15f;
    float background_alpha_ = 0.6f;
};

} // namespace voxel_canvas