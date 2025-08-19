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
#include "canvas_ui/scaled_theme.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace {
    constexpr float PI = 3.14159265359f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    [[maybe_unused]] constexpr float RAD_TO_DEG = 180.0f / PI;
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
    // Update navigation handler with current UI scale and bind window if needed
    if (nav_handler_) {
        nav_handler_->set_ui_scale(renderer->get_content_scale());
        
        // Bind window for cursor control (only once)
        if (!nav_handler_window_bound_) {
            nav_handler_->bind_window(renderer->get_window());
            nav_handler_window_bound_ = true;
        }
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
        ScaledTheme scaled_theme = renderer->get_scaled_theme();
        std::string fps_text = "FPS: " + std::to_string(frame_count_);
        Point2D fps_pos(bounds.x + 10, bounds.y + 30);
        ColorRGBA text_color = renderer->get_theme().text_primary;
        renderer->draw_text(fps_text, fps_pos, text_color, scaled_theme.font_size_lg());
    }
    
    // Render camera info
    ScaledTheme scaled_theme = renderer->get_scaled_theme();
    Vector3D euler = camera_.get_rotation().to_euler();
    std::string camera_info = "Distance: " + std::to_string(camera_.get_distance()) + 
                             " | Rotation: " + std::to_string(static_cast<int>(euler.x)) + 
                             "°, " + std::to_string(static_cast<int>(euler.y)) + "°";
    Point2D info_pos(bounds.x + 10, bounds.bottom() - 20);
    ColorRGBA info_color = renderer->get_theme().text_secondary;
    renderer->draw_text(camera_info, info_pos, info_color, scaled_theme.font_size_md());
}

bool Viewport3DEditor::handle_event(const InputEvent& event, const Rect2D& bounds) {
    // FIRST: Block any trackpad gestures if we have a pending sphere click
    // This must happen before ANY other processing
    if (sphere_clicked_ >= 0) {
        if (event.type == EventType::TRACKPAD_ROTATE || 
            event.type == EventType::TRACKPAD_SCROLL ||
            event.type == EventType::TRACKPAD_PAN ||
            event.type == EventType::TRACKPAD_ZOOM) {
            return true; // Consume to prevent any processing
        }
    }
    
    // Check navigation widget interaction
    if (nav_widget_visible_ && nav_widget_) {
        if (event.type == EventType::MOUSE_MOVE) {
            // Update hover state
            int hovered = nav_widget_->hit_test(event.mouse_pos);
            nav_widget_->set_hover_axis(hovered);
            
            // Check if we've started dragging from a sphere click
            if (sphere_clicked_ >= 0) {
                // Calculate distance from original click position
                float dx = event.mouse_pos.x - sphere_click_pos_.x;
                float dy = event.mouse_pos.y - sphere_click_pos_.y;
                float drag_threshold = 5.0f; // pixels
                
                if (dx * dx + dy * dy > drag_threshold * drag_threshold) {
                    
                    // User is dragging - convert to rotation
                    sphere_clicked_ = -1; // Clear sphere click FIRST to prevent any callbacks
                    widget_dragging_ = true; // Set this flag to track widget dragging
                    nav_widget_->set_dragging(true); // Keep backdrop visible during drag
                    
                    // Simulate middle mouse press to start orbit
                    // The navigation handler will set is_dragging_ = true
                    InputEvent orbit_event = event;
                    orbit_event.type = EventType::MOUSE_PRESS;
                    orbit_event.mouse_button = MouseButton::MIDDLE;
                    orbit_event.mouse_pos = event.mouse_pos;
                    
                    // The handler will check if we're near the widget to avoid cursor capture
                    
                    if (nav_handler_) {
                        nav_handler_->handle_event(orbit_event);
                    }
                    
                    return true;
                }
                // While waiting to detect click vs drag on sphere, consume move events
                return true;
            }
            
            // If we're dragging from the widget, continue the rotation
            if (widget_dragging_) {
                // Simulate middle mouse drag for the navigation handler
                InputEvent orbit_event = event;
                orbit_event.mouse_button = MouseButton::MIDDLE;
                
                if (nav_handler_) {
                    nav_handler_->handle_event(orbit_event);
                }
                return true;
            }
        } else if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
            // First check if we clicked on a sphere
            int clicked_sphere = nav_widget_->hit_test(event.mouse_pos);
            if (clicked_sphere >= 0) {
                // Clicked on a sphere - store for potential drag detection
                sphere_clicked_ = clicked_sphere;
                sphere_click_pos_ = event.mouse_pos;
                return true; // Consume event
            }
            
            // Check if click is within widget backdrop (but not on sphere)
            if (nav_widget_->is_point_in_widget(event.mouse_pos)) {
                // Start widget dragging for rotation immediately
                widget_dragging_ = true;
                nav_widget_->set_dragging(true); // Keep backdrop visible during drag
                
                // Create a fake middle mouse press event to start orbit
                InputEvent orbit_event = event;
                orbit_event.mouse_button = MouseButton::MIDDLE;
                
                if (nav_handler_) {
                    nav_handler_->handle_event(orbit_event);
                }
                return true; // Consume event
            }
        } else if (event.type == EventType::MOUSE_RELEASE && event.mouse_button == MouseButton::LEFT) {
            // Check if we're releasing a sphere click (not a drag)
            if (sphere_clicked_ >= 0) {
                // This was a click on a sphere - trigger the axis view
                int axis = sphere_clicked_ / 2;  // 0,1 = X, 2,3 = Y, 4,5 = Z
                bool positive = (sphere_clicked_ % 2) == 0;
                
                
                // Trigger the axis click callback
                if (nav_widget_->get_axis_click_callback()) {
                    nav_widget_->get_axis_click_callback()(axis, positive);
                }
                
                sphere_clicked_ = -1; // Clear the click
                return true;
            }
            
            // End widget dragging
            if (widget_dragging_) {
                widget_dragging_ = false;
                nav_widget_->set_dragging(false); // Allow backdrop to hide when done
                
                // Simulate middle mouse release to end orbit
                InputEvent orbit_event = event;
                orbit_event.mouse_button = MouseButton::MIDDLE;
                
                if (nav_handler_) {
                    nav_handler_->handle_event(orbit_event);
                }
                return true;
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
            // Handle axis clicks to set standard views (keep current projection type)
            // Using right-handed coordinate system: +Y up, +X right, +Z towards viewer
            // Note: Orthographic is a separate toggle, not triggered by navigation
            
            float view_distance = 60.0f;
            
            // Store previous projection BEFORE we change anything
            // Only store if we're not already in an axis view
            if (current_view_axis_ == -1) {
                previous_projection_type_ = camera_.get_projection_type();
            }
            
            // Update view state tracking
            is_exact_side_view_ = (axis == 0 || axis == 2);  // X and Z views get vertical grids
            current_view_axis_ = axis;
            current_view_positive_ = positive;
            
            // Switch to orthographic for axis-aligned views (like Blender's auto-perspective)
            camera_.set_projection_type(Camera3D::ProjectionType::Orthographic);
            camera_.set_orthographic_size(30.0f);  // Reasonable default size
            camera_.set_near_plane(0.1f);
            camera_.set_far_plane(10000.0f);  // Large far plane to include grids
            
            if (axis == 0) { // X axis - Right/Left view
                // Looking along X axis - show YZ plane
                camera_.set_axis_view(positive ? Camera3D::AxisView::Right : Camera3D::AxisView::Left, view_distance);
                
                if (grid_3d_) {
                    grid_3d_->set_active_plane(Grid3DRenderer::PLANE_YZ);
                    grid_3d_->enable_vertical_grid(true);
                }
            } else if (axis == 1) { // Y axis - Top/Bottom view  
                // Looking along Y axis - no vertical grid
                camera_.set_axis_view(positive ? Camera3D::AxisView::Top : Camera3D::AxisView::Bottom, view_distance);
                
                if (grid_3d_) {
                    grid_3d_->enable_vertical_grid(false);
                }
            } else if (axis == 2) { // Z axis - Front/Back view
                // Looking along Z axis - show XY plane
                camera_.set_axis_view(positive ? Camera3D::AxisView::Front : Camera3D::AxisView::Back, view_distance);
                
                if (grid_3d_) {
                    grid_3d_->set_active_plane(Grid3DRenderer::PLANE_XY);
                    grid_3d_->enable_vertical_grid(true);
                }
            }
        });
    }
}

void Viewport3DEditor::render_3d_scene([[maybe_unused]] CanvasRenderer* renderer, const Rect2D& bounds) {
    // Set up OpenGL 3D rendering context for professional viewport
    
    // Clear only the depth buffer for this viewport
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Set up OpenGL viewport to this region only
    glViewport(static_cast<GLint>(bounds.x), static_cast<GLint>(bounds.y), static_cast<GLsizei>(bounds.width), static_cast<GLsizei>(bounds.height));
}

void Viewport3DEditor::render_grid(CanvasRenderer* renderer, const Rect2D& bounds) {
    if (grid_3d_) {
        // Use proper 3D shader-based grid rendering with theme colors
        grid_3d_->render(camera_, bounds, renderer->get_theme(), renderer->get_content_scale());
    }
}


bool Viewport3DEditor::handle_camera_navigation(const InputEvent& event, const Rect2D& bounds) {
    if (!nav_handler_) {
        return false;
    }
    
    // Detect when orbiting starts to disable vertical grid and restore perspective
    if (event.type == EventType::TRACKPAD_ROTATE || 
        (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::MIDDLE && !event.has_modifier(KeyModifier::SHIFT))) {
        
        // User is starting to orbit/rotate the camera
        if (current_view_axis_ != -1) {  // We're in an axis view
            
            // Restore previous projection mode (usually perspective)
            camera_.set_projection_type(previous_projection_type_);
            camera_.set_far_plane(100000.0f);  // Restore large far plane for perspective
            
            
            // Disable vertical grid
            if (grid_3d_) {
                grid_3d_->enable_vertical_grid(false);
            }
            
            is_exact_side_view_ = false;
            current_view_axis_ = -1;
        }
    }
    
    // Clear any pending sphere click if user starts navigating with middle mouse or trackpad
    if ((event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::MIDDLE) ||
        event.type == EventType::TRACKPAD_ROTATE ||
        event.type == EventType::TRACKPAD_PAN ||
        event.type == EventType::TRACKPAD_ZOOM ||
        (event.type == EventType::TRACKPAD_SCROLL && 
         (event.has_modifier(KeyModifier::SHIFT) || event.has_modifier(KeyModifier::CMD) || event.has_modifier(KeyModifier::CTRL)))) {
        if (sphere_clicked_ >= 0) {
            sphere_clicked_ = -1;  // Clear pending sphere click
        }
    }
    
    
    // Update navigation handler with current viewport bounds
    nav_handler_->set_viewport_bounds(bounds);
    
    // Let the navigation handler process the event using the Camera3D directly
    EventResult result = nav_handler_->handle_event(event);
    
    if (result == EventResult::HANDLED) {
        return true;
    }
    
    return false;
}


// Note: ViewportGrid removed - now using shader-based Grid3DRenderer

// Legacy ViewportNavWidget removed - replaced by professional Camera3D navigation system
// Navigation is now handled directly by ViewportNavigationHandler with Camera3D

} // namespace voxel_canvas