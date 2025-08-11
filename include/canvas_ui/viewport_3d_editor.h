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
#include "viewport_navigation_handler.h"
#include "camera_3d.h"
#include "navigation_widget.h"
#include <memory>

namespace voxel_canvas {

class ViewportGrid;
class ViewportNavWidget;

// Legacy CameraState removed - using professional Camera3D system

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
    
    bool handle_camera_navigation(const InputEvent& event, const Rect2D& bounds);
    
    // 3D scene components
    std::unique_ptr<Grid3DRenderer> grid_3d_;
    std::unique_ptr<ViewportNavigationHandler> nav_handler_;
    std::unique_ptr<NavigationWidget> nav_widget_;
    
    // Professional 3D camera system
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

// Legacy ViewportGrid removed - using professional Grid3DRenderer with OpenGL shaders

// Legacy ViewportNavWidget removed - navigation handled by ViewportNavigationHandler with Camera3D directly

} // namespace voxel_canvas