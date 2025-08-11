/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Viewport Editor implementation
 */

#include "canvas_ui/viewport_3d_editor.h"
#include "canvas_ui/canvas_renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace {
    constexpr float PI = 3.14159265359f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
}

namespace voxel_canvas {

// Viewport3DEditor implementation

Viewport3DEditor::Viewport3DEditor()
    : EditorSpace(EditorType::VIEWPORT_3D)
    , is_orbiting_(false)
    , is_panning_(false)
    , is_zooming_(false)
    , grid_visible_(true)
    , nav_widget_visible_(true)
    , show_fps_(true)
    , frame_time_(0.0f)
    , frame_count_(0) {
    
    // Camera is already initialized with good defaults in Camera3D constructor
    // which calls reset_to_default() to set up proper voxel editing view
    // Just ensure we're in orbit mode
    camera_.set_navigation_mode(Camera3D::NavigationMode::Orbit);
    
}

Viewport3DEditor::~Viewport3DEditor() {
    shutdown();
}

void Viewport3DEditor::initialize() {
    
    setup_camera();
    setup_grid();
    setup_nav_widget();
}

void Viewport3DEditor::shutdown() {
    if (grid_3d_) {
        grid_3d_->shutdown();
    }
}

void Viewport3DEditor::update(float delta_time) {
    frame_time_ += delta_time;
    frame_count_++;
    
    // Update FPS every second
    if (frame_time_ >= 1.0f) {
        frame_time_ = 0.0f;
        frame_count_ = 0;
    }
}

void Viewport3DEditor::render(CanvasRenderer* renderer, const Rect2D& bounds) {
    // Update navigation handler with current UI scale
    if (nav_handler_) {
        nav_handler_->set_ui_scale(renderer->get_content_scale());
    }
    
    // Clear the viewport with theme background color
    ColorRGBA bg_color = renderer->get_theme().background_primary;
    renderer->draw_rect(bounds, bg_color);
    
    // Render 3D scene content
    render_3d_scene(renderer, bounds);
    
    // Render grid if visible
    if (grid_visible_ && grid_3d_) {
        render_grid(renderer, bounds);
    }
}

void Viewport3DEditor::render_overlay(CanvasRenderer* renderer, const Rect2D& bounds) {
    // Professional overlay rendering - UI elements on top of 3D content
    
    // Render navigation widget if visible
    if (nav_widget_visible_ && nav_widget_) {
        nav_widget_->render(renderer, camera_, bounds);
    }
    
    // Render debug info if enabled
    if (show_fps_) {
        std::string fps_text = "FPS: " + std::to_string(frame_count_);
        Point2D fps_pos(bounds.x + 10, bounds.y + 30);
        ColorRGBA text_color = renderer->get_theme().text_primary;
        renderer->draw_text(fps_text, fps_pos, text_color, 14.0f);
    }
    
    // Render camera info
    Vector3D euler = camera_.get_rotation().to_euler();
    std::string camera_info = "Distance: " + std::to_string(camera_.get_distance()) + 
                             " | Rotation: " + std::to_string((int)euler.x) + 
                             "°, " + std::to_string((int)euler.y) + "°";
    Point2D info_pos(bounds.x + 10, bounds.bottom() - 20);
    ColorRGBA info_color = renderer->get_theme().text_secondary;
    renderer->draw_text(camera_info, info_pos, info_color, 12.0f);
}

bool Viewport3DEditor::handle_event(const InputEvent& event, const Rect2D& bounds) {
    // Check navigation widget interaction first
    if (nav_widget_visible_ && nav_widget_) {
        if (event.type == EventType::MOUSE_MOVE) {
            // Update hover state
            int hovered = nav_widget_->hit_test(event.mouse_pos);
            nav_widget_->set_hover_axis(hovered);
        } else if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
            // Check for widget click
            int clicked = nav_widget_->hit_test(event.mouse_pos);
            if (clicked >= 0) {
                nav_widget_->handle_click(clicked);
                return true; // Consume event
            }
        }
    }
    
    // Handle professional camera navigation directly
    return handle_camera_navigation(event, bounds);
}

void Viewport3DEditor::reset_camera_view() {
    // Use the camera's built-in reset which properly sets up for voxel editing
    camera_.reset_to_default();
}

void Viewport3DEditor::frame_all_objects() {
    // For now, just reset to a reasonable view
    // Later this will calculate bounds of all visible objects
    reset_camera_view();
}

void Viewport3DEditor::set_orthographic_view(int view) {
    camera_.set_projection_type(Camera3D::ProjectionType::Orthographic);
    camera_.set_orthographic_size(10.0f);
    
    switch (view) {
        case 1: // Front view
            camera_.rotate_around_target(0.0f, 0.0f);
            break;
        case 2: // Right view
            camera_.rotate_around_target(90.0f * DEG_TO_RAD, 0.0f);
            break;
        case 3: // Top view
            camera_.rotate_around_target(0.0f, 90.0f * DEG_TO_RAD);
            break;
        case 4: // Back view
            camera_.rotate_around_target(180.0f * DEG_TO_RAD, 0.0f);
            break;
        case 5: // Left view
            camera_.rotate_around_target(-90.0f * DEG_TO_RAD, 0.0f);
            break;
        case 6: // Bottom view
            camera_.rotate_around_target(0.0f, -90.0f * DEG_TO_RAD);
            break;
        default:
            camera_.set_projection_type(Camera3D::ProjectionType::Perspective);
            break;
    }
}

void Viewport3DEditor::setup_camera() {
    // Camera setup is mostly data initialization, done in constructor
}

void Viewport3DEditor::setup_grid() {
    std::cout << "Setting up 3D grid renderer..." << std::endl;
    grid_3d_ = std::make_unique<Grid3DRenderer>();
    if (!grid_3d_->initialize()) {
        std::cerr << "ERROR: Failed to initialize 3D grid renderer" << std::endl;
        grid_3d_.reset();
    } else {
        std::cout << "3D grid renderer initialized successfully" << std::endl;
    }
}

void Viewport3DEditor::setup_nav_widget() {
    // Set up professional navigation handler
    nav_handler_ = std::make_unique<ViewportNavigationHandler>();
    
    // Bind the professional Camera3D to the navigation handler
    nav_handler_->bind_camera(&camera_);
    
    // Configure navigation preferences for professional use
    NavigationPreferences prefs;
    prefs.orbit_selection = true;     // Orbit around selection
    prefs.depth_navigate = true;      // Use depth information
    prefs.zoom_to_mouse = true;       // Zoom towards mouse cursor
    prefs.auto_perspective = true;    // Auto-switch to perspective
    prefs.trackpad_natural_scroll = true;  // Natural trackpad scrolling
    prefs.trackpad_zoom_gesture = true;    // Pinch to zoom
    prefs.trackpad_rotate_gesture = true;  // Two-finger rotate
    nav_handler_->set_preferences(prefs);
    
    // Create navigation widget for viewport orientation
    nav_widget_ = std::make_unique<NavigationWidget>();
    if (!nav_widget_->initialize()) {
        std::cerr << "Failed to initialize navigation widget" << std::endl;
        nav_widget_.reset();
    } else {
        // Set up axis click callback
        nav_widget_->set_axis_click_callback([this](int axis, bool positive) {
            // Handle axis clicks to set standard orthographic views
            // Using right-handed coordinate system: +Y up, +X right, +Z towards viewer
            camera_.set_projection_type(Camera3D::ProjectionType::Orthographic);
            float view_distance = 60.0f;
            
            if (axis == 0) { // X axis - Right/Left view
                // Looking along X axis
                camera_.set_position(Vector3D(positive ? view_distance : -view_distance, 0, 0));
                camera_.look_at(Vector3D(0, 0, 0), Vector3D(0, 1, 0)); // Y is up
            } else if (axis == 1) { // Y axis - Top/Bottom view  
                // Looking along Y axis
                camera_.set_position(Vector3D(0, positive ? view_distance : -view_distance, 0));
                camera_.look_at(Vector3D(0, 0, 0), Vector3D(0, 0, positive ? -1 : 1)); // Z forward when looking down
            } else if (axis == 2) { // Z axis - Front/Back view
                // Looking along Z axis
                camera_.set_position(Vector3D(0, 0, positive ? view_distance : -view_distance));
                camera_.look_at(Vector3D(0, 0, 0), Vector3D(0, 1, 0)); // Y is up
            }
        });
    }
}

void Viewport3DEditor::render_3d_scene(CanvasRenderer* renderer, const Rect2D& bounds) {
    // Set up OpenGL 3D rendering context for professional viewport
    
    // Clear only the depth buffer for this viewport
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Set up OpenGL viewport to this region only
    glViewport(bounds.x, bounds.y, bounds.width, bounds.height);
}

void Viewport3DEditor::render_grid(CanvasRenderer* renderer, const Rect2D& bounds) {
    if (grid_3d_) {
        // Use proper 3D shader-based grid rendering with theme colors
        grid_3d_->render(camera_, bounds, renderer->get_theme());
    }
}


bool Viewport3DEditor::handle_camera_navigation(const InputEvent& event, const Rect2D& bounds) {
    if (!nav_handler_) {
        return false;
    }
    
    // Debug: Only log important events
    if (event.type == EventType::MOUSE_PRESS || event.type == EventType::MOUSE_WHEEL) {
        std::cout << "3D Viewport received " << (event.type == EventType::MOUSE_PRESS ? "MOUSE_PRESS" : "MOUSE_WHEEL") << std::endl;
    }
    
    // Update navigation handler with current viewport bounds
    nav_handler_->set_viewport_bounds(bounds);
    
    // Let the navigation handler process the event using the Camera3D directly
    EventResult result = nav_handler_->handle_event(event);
    
    if (result == EventResult::HANDLED) {
        if (event.type == EventType::MOUSE_PRESS || event.type == EventType::MOUSE_WHEEL) {
            std::cout << "Navigation processed " << (event.type == EventType::MOUSE_PRESS ? "click" : "wheel") << std::endl;
        }
        return true;
    }
    
    return false;
}


// Note: ViewportGrid removed - now using shader-based Grid3DRenderer

// Legacy ViewportNavWidget removed - replaced by professional Camera3D navigation system
// Navigation is now handled directly by ViewportNavigationHandler with Camera3D

} // namespace voxel_canvas