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
    
    // Initialize camera to match archived implementation - 45 degree angle looking down
    camera_.rotation = Point2D(25.0f, 45.0f); // 45 degree angle looking down at grid
    camera_.distance = 15.0f; // Better distance for grid visibility
    camera_.target = Point2D(0, 0);
    camera_.orthographic = false;
    camera_.ortho_scale = 1.0f;
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
    if (nav_widget_) {
        nav_widget_->shutdown();
    }
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
    // Clear the viewport background - temporarily disabled to test rendering
    // ColorRGBA bg_color = renderer->get_theme().background_primary;
    // renderer->draw_rect(bounds, bg_color);
    
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
        render_nav_widget(renderer, bounds);
    }
    
    // Render debug info if enabled
    if (show_fps_) {
        std::string fps_text = "FPS: " + std::to_string(frame_count_);
        Point2D fps_pos(bounds.x + 10, bounds.y + 30);
        ColorRGBA text_color = renderer->get_theme().text_primary;
        renderer->draw_text(fps_text, fps_pos, text_color, 14.0f);
    }
    
    // Render camera info
    std::string camera_info = "Distance: " + std::to_string(camera_.distance) + 
                             " | Rotation: " + std::to_string((int)camera_.rotation.x) + 
                             "°, " + std::to_string((int)camera_.rotation.y) + "°";
    Point2D info_pos(bounds.x + 10, bounds.bottom() - 20);
    ColorRGBA info_color = renderer->get_theme().text_secondary;
    renderer->draw_text(camera_info, info_pos, info_color, 12.0f);
}

bool Viewport3DEditor::handle_event(const InputEvent& event, const Rect2D& bounds) {
    // Handle navigation widget first if visible
    if (nav_widget_visible_ && nav_widget_) {
        // Convert Camera3D to CameraState for nav widget compatibility
        CameraState legacy_camera;
        legacy_camera.rotation = camera_.rotation;
        legacy_camera.distance = camera_.distance;
        legacy_camera.target = camera_.target;
        legacy_camera.orthographic = camera_.orthographic;
        legacy_camera.ortho_scale = camera_.ortho_scale;
        
        if (nav_widget_->handle_event(event, bounds, legacy_camera)) {
            // Copy changes back to Camera3D
            camera_.rotation = legacy_camera.rotation;
            camera_.distance = legacy_camera.distance;
            camera_.target = legacy_camera.target;
            camera_.orthographic = legacy_camera.orthographic;
            camera_.ortho_scale = legacy_camera.ortho_scale;
            return true;
        }
    }
    
    // Handle camera navigation
    return handle_camera_navigation(event, bounds);
}

void Viewport3DEditor::reset_camera_view() {
    camera_.rotation = Point2D(25.0f, 45.0f);
    camera_.distance = 15.0f;
    camera_.target = Point2D(0, 0);
}

void Viewport3DEditor::frame_all_objects() {
    // For now, just reset to a reasonable view
    // Later this will calculate bounds of all visible objects
    reset_camera_view();
}

void Viewport3DEditor::set_orthographic_view(int view) {
    camera_.orthographic = true;
    camera_.ortho_scale = 1.0f;
    
    switch (view) {
        case 1: // Front view
            camera_.rotation = Point2D(0.0f, 0.0f);
            break;
        case 2: // Right view
            camera_.rotation = Point2D(0.0f, 90.0f);
            break;
        case 3: // Top view
            camera_.rotation = Point2D(90.0f, 0.0f);
            break;
        case 4: // Back view
            camera_.rotation = Point2D(0.0f, 180.0f);
            break;
        case 5: // Left view
            camera_.rotation = Point2D(0.0f, -90.0f);
            break;
        case 6: // Bottom view
            camera_.rotation = Point2D(-90.0f, 0.0f);
            break;
        default:
            camera_.orthographic = false;
            break;
    }
}

void Viewport3DEditor::setup_camera() {
    // Camera setup is mostly data initialization, done in constructor
}

void Viewport3DEditor::setup_grid() {
    grid_3d_ = std::make_unique<Grid3DRenderer>();
    if (!grid_3d_->initialize()) {
        std::cerr << "Failed to initialize 3D grid renderer" << std::endl;
        grid_3d_.reset();
    }
}

void Viewport3DEditor::setup_nav_widget() {
    nav_widget_ = std::make_unique<ViewportNavWidget>();
    nav_widget_->initialize();
    
    // Position in bottom-right corner by default
    nav_widget_->set_position(Point2D(0.95f, 0.95f));
}

void Viewport3DEditor::render_3d_scene(CanvasRenderer* renderer, const Rect2D& bounds) {
    // 3D scene content rendering will be implemented here
    // For now, render nothing - focusing on professional grid display
}

void Viewport3DEditor::render_grid(CanvasRenderer* renderer, const Rect2D& bounds) {
    if (grid_3d_) {
        // Use proper 3D shader-based grid rendering with theme colors
        grid_3d_->render(camera_, bounds, renderer->get_theme());
    }
}

void Viewport3DEditor::render_nav_widget(CanvasRenderer* renderer, const Rect2D& bounds) {
    if (nav_widget_) {
        // Convert Camera3D to CameraState for nav widget compatibility
        CameraState legacy_camera;
        legacy_camera.rotation = camera_.rotation;
        legacy_camera.distance = camera_.distance;
        legacy_camera.target = camera_.target;
        legacy_camera.orthographic = camera_.orthographic;
        legacy_camera.ortho_scale = camera_.ortho_scale;
        
        nav_widget_->render(renderer, bounds, legacy_camera);
    }
}

bool Viewport3DEditor::handle_camera_navigation(const InputEvent& event, const Rect2D& bounds) {
    if (event.type == EventType::MOUSE_PRESS) {
        if (event.mouse_button == MouseButton::MIDDLE) {
            if (event.has_modifier(KeyModifier::SHIFT)) {
                is_panning_ = true;
            } else {
                is_orbiting_ = true;
            }
            last_mouse_pos_ = event.mouse_pos;
            return true;
        }
    } else if (event.type == EventType::MOUSE_RELEASE) {
        if (event.mouse_button == MouseButton::MIDDLE) {
            is_orbiting_ = false;
            is_panning_ = false;
            return true;
        }
    } else if (event.type == EventType::MOUSE_MOVE) {
        if (is_orbiting_ || is_panning_) {
            Point2D delta = event.mouse_pos - last_mouse_pos_;
            
            if (is_orbiting_) {
                // Camera orbiting
                float sensitivity = 0.5f;
                camera_.rotation.x += delta.y * sensitivity;
                camera_.rotation.y += delta.x * sensitivity;
                
                // Clamp pitch to avoid gimbal lock
                camera_.rotation.x = std::clamp(camera_.rotation.x, -89.0f, 89.0f);
                
                // Wrap yaw
                while (camera_.rotation.y < 0) camera_.rotation.y += 360.0f;
                while (camera_.rotation.y >= 360.0f) camera_.rotation.y -= 360.0f;
                
            } else if (is_panning_) {
                // Camera panning
                float sensitivity = 0.01f;
                camera_.target.x -= delta.x * sensitivity * camera_.distance;
                camera_.target.y += delta.y * sensitivity * camera_.distance;
            }
            
            last_mouse_pos_ = event.mouse_pos;
            return true;
        }
    } else if (event.type == EventType::MOUSE_WHEEL) {
        // Camera zooming
        float zoom_factor = 1.1f;
        if (event.wheel_delta > 0) {
            camera_.distance /= zoom_factor;
        } else {
            camera_.distance *= zoom_factor;
        }
        
        // Clamp zoom distance
        camera_.distance = std::clamp(camera_.distance, 0.1f, 100.0f);
        return true;
    }
    
    return false;
}

bool Viewport3DEditor::handle_nav_widget_interaction(const InputEvent& event, const Rect2D& bounds) {
    if (nav_widget_) {
        // Convert Camera3D to CameraState for nav widget compatibility
        CameraState legacy_camera;
        legacy_camera.rotation = camera_.rotation;
        legacy_camera.distance = camera_.distance;
        legacy_camera.target = camera_.target;
        legacy_camera.orthographic = camera_.orthographic;
        legacy_camera.ortho_scale = camera_.ortho_scale;
        
        bool handled = nav_widget_->handle_event(event, bounds, legacy_camera);
        
        if (handled) {
            // Copy changes back to Camera3D
            camera_.rotation = legacy_camera.rotation;
            camera_.distance = legacy_camera.distance;
            camera_.target = legacy_camera.target;
            camera_.orthographic = legacy_camera.orthographic;
            camera_.ortho_scale = legacy_camera.ortho_scale;
        }
        
        return handled;
    }
    return false;
}

// Note: ViewportGrid removed - now using shader-based Grid3DRenderer

// ViewportNavWidget implementation

ViewportNavWidget::ViewportNavWidget()
    : position_(0.9f, 0.9f)
    , size_(80.0f)
    , hovered_axis_(-1)
    , pressed_axis_(-1)
    , is_dragging_(false) {
    
    // Initialize sphere data structure from original implementation
    // This mirrors the professional axis system
    sphere_data_[0].axis = 0; sphere_data_[0].positive = true;  sphere_data_[0].depth = 0.0f; sphere_data_[0].position = Point2D(1.0f, 0.0f); sphere_data_[0].color = axis_colors_[0];  // +X
    sphere_data_[1].axis = 0; sphere_data_[1].positive = false; sphere_data_[1].depth = 0.0f; sphere_data_[1].position = Point2D(-1.0f, 0.0f); sphere_data_[1].color = axis_colors_[0]; // -X
    sphere_data_[2].axis = 1; sphere_data_[2].positive = true;  sphere_data_[2].depth = 0.0f; sphere_data_[2].position = Point2D(0.0f, 1.0f); sphere_data_[2].color = axis_colors_[1];  // +Y
    sphere_data_[3].axis = 1; sphere_data_[3].positive = false; sphere_data_[3].depth = 0.0f; sphere_data_[3].position = Point2D(0.0f, -1.0f); sphere_data_[3].color = axis_colors_[1]; // -Y
    sphere_data_[4].axis = 2; sphere_data_[4].positive = true;  sphere_data_[4].depth = 0.0f; sphere_data_[4].position = Point2D(0.0f, 0.0f); sphere_data_[4].color = axis_colors_[2];  // +Z
    sphere_data_[5].axis = 2; sphere_data_[5].positive = false; sphere_data_[5].depth = 0.0f; sphere_data_[5].position = Point2D(0.0f, 0.0f); sphere_data_[5].color = axis_colors_[2]; // -Z
}

ViewportNavWidget::~ViewportNavWidget() {
    shutdown();
}

void ViewportNavWidget::initialize() {
    // Widget initialization
}

void ViewportNavWidget::shutdown() {
    // Widget cleanup
}

void ViewportNavWidget::render(CanvasRenderer* renderer, const Rect2D& bounds, const CameraState& camera) {
    // Calculate widget screen position
    Point2D screen_pos(
        bounds.x + bounds.width * position_.x - size_ * 0.5f,
        bounds.y + bounds.height * position_.y - size_ * 0.5f
    );
    
    Rect2D widget_bounds(screen_pos.x, screen_pos.y, size_, size_);
    
    // Draw background circle
    renderer->draw_circle(
        Point2D(widget_bounds.x + widget_bounds.width * 0.5f, 
                widget_bounds.y + widget_bounds.height * 0.5f),
        size_ * 0.5f,
        background_color_
    );
    
    render_navigation_sphere(renderer, camera);
    render_axis_labels(renderer, camera);
}

bool ViewportNavWidget::handle_event(const InputEvent& event, const Rect2D& bounds, CameraState& camera) {
    if (!is_point_in_widget(event.mouse_pos, bounds)) {
        hovered_axis_ = -1;
        return false;
    }
    
    if (event.type == EventType::MOUSE_MOVE) {
        hovered_axis_ = get_axis_at_point(event.mouse_pos, bounds, camera);
        return true;
    } else if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
        pressed_axis_ = get_axis_at_point(event.mouse_pos, bounds, camera);
        if (pressed_axis_ >= 0) {
            // Set camera to orthographic view
            switch (pressed_axis_) {
                case 0: // +X
                    camera.rotation = Point2D(0, 90);
                    break;
                case 1: // +Y
                    camera.rotation = Point2D(0, 0);
                    break;
                case 2: // +Z
                    camera.rotation = Point2D(90, 0);
                    break;
                case 3: // -X
                    camera.rotation = Point2D(0, -90);
                    break;
                case 4: // -Y
                    camera.rotation = Point2D(0, 180);
                    break;
                case 5: // -Z
                    camera.rotation = Point2D(-90, 0);
                    break;
            }
            return true;
        }
    }
    
    return false;
}

void ViewportNavWidget::regenerate_sorted_widget(const CameraState& camera) {
    // Professional depth sorting algorithm from original implementation
    float sphere_positions[6][3] = {
        { axis_distance_, 0.0f, 0.0f},  // +X
        {-axis_distance_, 0.0f, 0.0f},  // -X
        {0.0f,  axis_distance_, 0.0f},  // +Y
        {0.0f, -axis_distance_, 0.0f},  // -Y
        {0.0f, 0.0f,  axis_distance_},  // +Z
        {0.0f, 0.0f, -axis_distance_}   // -Z
    };
    
    // Transform positions to camera space and calculate depths
    float sphere_depths[6];
    float min_depth = 1000.0f, max_depth = -1000.0f;
    
    // Simple depth calculation based on camera rotation
    float pitch_rad = camera.rotation.x * M_PI / 180.0f;
    float yaw_rad = camera.rotation.y * M_PI / 180.0f;
    
    for (int i = 0; i < 6; i++) {
        // Apply camera rotation to get view-space depth
        float x = sphere_positions[i][0];
        float y = sphere_positions[i][1];
        float z = sphere_positions[i][2];
        
        // Rotate by yaw (Y-axis)
        float x_rot = x * cos(yaw_rad) - z * sin(yaw_rad);
        float z_rot = x * sin(yaw_rad) + z * cos(yaw_rad);
        
        // Rotate by pitch (X-axis)  
        float y_final = y * cos(pitch_rad) - z_rot * sin(pitch_rad);
        float z_final = y * sin(pitch_rad) + z_rot * cos(pitch_rad);
        
        sphere_depths[i] = z_final;
        min_depth = std::min(min_depth, sphere_depths[i]);
        max_depth = std::max(max_depth, sphere_depths[i]);
        
        // Update sphere data
        sphere_data_[i].depth = sphere_depths[i];
        sphere_data_[i].position = Point2D(x_rot, y_final);
    }
    
    // Normalize depths and create billboard data with depth-based scaling
    billboard_data_.clear();
    for (int i = 0; i < 6; i++) {
        float normalized_depth = (sphere_depths[i] - min_depth) / (max_depth - min_depth);
        float depth_scale = 0.7f + 0.3f * normalized_depth; // Scale from 0.7 to 1.0
        
        BillboardData billboard;
        billboard.position = sphere_data_[i].position;
        billboard.radius = sphere_radius_ * depth_scale;
        billboard.color = sphere_data_[i].color;
        billboard.positive = sphere_data_[i].positive;
        billboard.depth = sphere_depths[i];
        billboard.sphere_index = i;
        billboard_data_.push_back(billboard);
    }
    
    // Sort billboards back-to-front for proper rendering
    std::sort(billboard_data_.begin(), billboard_data_.end(), 
              [](const BillboardData& a, const BillboardData& b) {
                  return a.depth < b.depth; // Render furthest first
              });
    
    // Generate axis lines (only for positive axes)
    line_data_.clear();
    float line_end_offset = sphere_radius_ * 0.05f;
    float line_length = axis_distance_ - line_end_offset;
    
    // Add axis lines with depth-based alpha
    for (int axis = 0; axis < 3; axis++) {
        int pos_index = axis * 2;     // Positive axis index
        float line_alpha = 0.8f * (1.0f - (sphere_depths[pos_index] - min_depth) / (max_depth - min_depth) * 0.5f);
        
        LineData line;
        line.start = Point2D(0, 0);
        line.end = sphere_data_[pos_index].position;
        line.color = axis_colors_[axis];
        line.alpha = line_alpha;
        line_data_.push_back(line);
    }
}

void ViewportNavWidget::render_navigation_sphere(CanvasRenderer* renderer, const CameraState& camera) {
    // Regenerate widget data based on camera
    regenerate_sorted_widget(camera);
    
    // Render axis lines first
    render_axis_lines(renderer, camera);
    
    // Render billboards with professional depth sorting
    render_screen_space_billboards(renderer, camera);
}

void ViewportNavWidget::render_screen_space_billboards(CanvasRenderer* renderer, const CameraState& camera) {
    Point2D widget_center(
        position_.x * 800 - size_ * 0.5f + size_ * 0.5f,
        position_.y * 600 - size_ * 0.5f + size_ * 0.5f
    );
    
    float scale_factor = size_ * 0.3f; // Scale factor for screen space
    
    // Render billboards in depth-sorted order
    for (const auto& billboard : billboard_data_) {
        Point2D screen_pos = widget_center + Point2D(
            billboard.position.x * scale_factor,
            -billboard.position.y * scale_factor // Flip Y for screen space
        );
        
        float radius = billboard.radius * scale_factor;
        ColorRGBA color = billboard.color;
        
        // Highlight if hovered
        if (hovered_axis_ == billboard.sphere_index) {
            color.r = std::min(1.0f, color.r * 1.5f);
            color.g = std::min(1.0f, color.g * 1.5f);
            color.b = std::min(1.0f, color.b * 1.5f);
        }
        
        // Dim negative axes
        if (!billboard.positive) {
            color.r *= 0.6f;
            color.g *= 0.6f;
            color.b *= 0.6f;
        }
        
        renderer->draw_circle(screen_pos, radius, color);
    }
}

void ViewportNavWidget::render_axis_lines(CanvasRenderer* renderer, const CameraState& camera) {
    Point2D widget_center(
        position_.x * 800 - size_ * 0.5f + size_ * 0.5f,
        position_.y * 600 - size_ * 0.5f + size_ * 0.5f
    );
    
    float scale_factor = size_ * 0.3f;
    
    for (const auto& line : line_data_) {
        Point2D start = widget_center + Point2D(
            line.start.x * scale_factor,
            -line.start.y * scale_factor
        );
        Point2D end = widget_center + Point2D(
            line.end.x * scale_factor,
            -line.end.y * scale_factor
        );
        
        ColorRGBA color = line.color;
        color.a = line.alpha;
        
        renderer->draw_line(start, end, color, 2.0f);
    }
}

void ViewportNavWidget::render_axis_labels(CanvasRenderer* renderer, const CameraState& camera) {
    if (!show_labels_) return;
    
    Point2D widget_center(
        position_.x * 800 - size_ * 0.5f + size_ * 0.5f,
        position_.y * 600 - size_ * 0.5f + size_ * 0.5f
    );
    
    float scale_factor = size_ * 0.3f;
    const char* labels[6] = {"X", "-X", "Y", "-Y", "Z", "-Z"};
    
    // Render labels for visible spheres
    for (const auto& billboard : billboard_data_) {
        Point2D screen_pos = widget_center + Point2D(
            billboard.position.x * scale_factor,
            -billboard.position.y * scale_factor
        );
        
        // Offset label position slightly
        Point2D label_pos = screen_pos + Point2D(-4, -6);
        
        ColorRGBA label_color = ColorRGBA::white();
        if (hovered_axis_ == billboard.sphere_index) {
            label_color = ColorRGBA(1.0f, 1.0f, 0.8f, 1.0f); // Slight yellow tint when hovered
        }
        
        renderer->draw_text(labels[billboard.sphere_index], label_pos, label_color, 10.0f);
    }
}

bool ViewportNavWidget::is_point_in_widget(const Point2D& point, const Rect2D& bounds) const {
    Point2D widget_center(
        bounds.x + bounds.width * position_.x,
        bounds.y + bounds.height * position_.y
    );
    
    float dist_sq = (point.x - widget_center.x) * (point.x - widget_center.x) + 
                    (point.y - widget_center.y) * (point.y - widget_center.y);
    float radius_sq = (size_ * 0.5f) * (size_ * 0.5f);
    
    return dist_sq <= radius_sq;
}

int ViewportNavWidget::get_axis_at_point(const Point2D& point, const Rect2D& bounds, const CameraState& camera) const {
    Point2D widget_center(
        bounds.x + bounds.width * position_.x,
        bounds.y + bounds.height * position_.y
    );
    
    float axis_radius = 12.0f;
    float spacing = 25.0f;
    
    // Check each axis position
    std::vector<Point2D> axis_positions = {
        Point2D(widget_center.x + spacing, widget_center.y),         // +X
        Point2D(widget_center.x, widget_center.y - spacing),         // +Y  
        Point2D(widget_center.x + spacing * 0.7f, widget_center.y - spacing * 0.7f), // +Z
        Point2D(widget_center.x - spacing, widget_center.y),         // -X
        Point2D(widget_center.x, widget_center.y + spacing),         // -Y
        Point2D(widget_center.x - spacing * 0.7f, widget_center.y + spacing * 0.7f)  // -Z
    };
    
    for (int i = 0; i < 6; i++) {
        Point2D axis_pos = axis_positions[i];
        float dist_sq = (point.x - axis_pos.x) * (point.x - axis_pos.x) + 
                        (point.y - axis_pos.y) * (point.y - axis_pos.y);
        
        if (dist_sq <= axis_radius * axis_radius) {
            return i;
        }
    }
    
    return -1;
}

} // namespace voxel_canvas