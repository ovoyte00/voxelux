/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * 3D Navigation Widget Implementation
 */

#include "canvas_ui/navigation_widget.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/font_system.h"
#include "glad/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace voxel_canvas {

NavigationWidget::NavigationWidget() {
    // Initialize axis sphere data
    axis_spheres_[0] = {0, true,  0.0f, { axis_distance_, 0.0f, 0.0f}, x_axis_color_, 1.0f}; // +X
    axis_spheres_[1] = {0, false, 0.0f, {-axis_distance_, 0.0f, 0.0f}, x_axis_color_, 1.0f}; // -X
    axis_spheres_[2] = {1, true,  0.0f, {0.0f,  axis_distance_, 0.0f}, y_axis_color_, 1.0f}; // +Y
    axis_spheres_[3] = {1, false, 0.0f, {0.0f, -axis_distance_, 0.0f}, y_axis_color_, 1.0f}; // -Y
    axis_spheres_[4] = {2, true,  0.0f, {0.0f, 0.0f,  axis_distance_}, z_axis_color_, 1.0f}; // +Z
    axis_spheres_[5] = {2, false, 0.0f, {0.0f, 0.0f, -axis_distance_}, z_axis_color_, 1.0f}; // -Z
    
    // Professional line thickness for widget visualization
    // This will be multiplied by UI scale factor when rendering
    line_thickness_ = 3.0f;
}

NavigationWidget::~NavigationWidget() {
    cleanup();
}

bool NavigationWidget::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Create simple shader for widget rendering
    const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 FragColor;

uniform mat4 projection;
uniform vec2 offset;
uniform float scale;

void main() {
    vec2 scaled_pos = aPos * scale + offset;
    gl_Position = projection * vec4(scaled_pos, 0.0, 1.0);
    FragColor = aColor;
}
)";
    
    const char* fragment_shader_src = R"(
#version 330 core
in vec4 FragColor;
out vec4 outColor;

void main() {
    outColor = FragColor;
}
)";
    
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);
    
    // Check compilation
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        std::cerr << "Navigation widget vertex shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);
    
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        std::cerr << "Navigation widget fragment shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Link program
    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader);
    glAttachShader(shader_program_, fragment_shader);
    glLinkProgram(shader_program_);
    
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program_, 512, nullptr, info_log);
        std::cerr << "Navigation widget shader linking failed: " << info_log << std::endl;
        return false;
    }
    
    // Clean up shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    // Create VAO and VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Setup vertex attributes: position (2 floats) + color (4 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    initialized_ = true;
    return true;
}

void NavigationWidget::cleanup() {
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (shader_program_) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
    initialized_ = false;
}

void NavigationWidget::calculate_view_ordering(const Camera3D& camera) {
    // Get camera view matrix (rotation only for widget)
    Matrix4x4 view_matrix = camera.get_view_matrix();
    
    // Convert to GLM for easier manipulation
    glm::mat4 view(1.0f);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            view[i][j] = view_matrix.m[j][i]; // Transpose for column-major
        }
    }
    
    // Extract rotation component only
    glm::mat4 rotation_only = glm::mat4(1.0f);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rotation_only[i][j] = view[i][j];
        }
    }
    
    // Check for axis alignment with camera
    camera_aligned_axis_ = -1;
    for (int axis = 0; axis < 3; axis++) {
        float alignment = std::abs(rotation_only[2][axis]);
        if (alignment > (1.0f - alignment_threshold_)) {
            camera_aligned_axis_ = axis;
            break;
        }
    }
    
    // Transform sphere positions and calculate depth
    float min_depth = 1e10f, max_depth = -1e10f;
    
    // Store original positions
    Point3D original_positions[NUM_AXES] = {
        { axis_distance_, 0.0f, 0.0f}, // +X
        {-axis_distance_, 0.0f, 0.0f}, // -X
        {0.0f,  axis_distance_, 0.0f}, // +Y
        {0.0f, -axis_distance_, 0.0f}, // -Y
        {0.0f, 0.0f,  axis_distance_}, // +Z
        {0.0f, 0.0f, -axis_distance_}  // -Z
    };
    
    for (int i = 0; i < NUM_AXES; i++) {
        // Transform sphere position by camera rotation
        glm::vec4 pos(original_positions[i].x, 
                     original_positions[i].y,
                     original_positions[i].z, 1.0f);
        glm::vec4 transformed = rotation_only * pos;
        
        // Update sphere position to view space
        axis_spheres_[i].position = {transformed.x, transformed.y, transformed.z};
        axis_spheres_[i].depth = transformed.z;
        
        min_depth = std::min(min_depth, axis_spheres_[i].depth);
        max_depth = std::max(max_depth, axis_spheres_[i].depth);
    }
    
    // Set colors without depth fading - just use solid colors
    for (int i = 0; i < NUM_AXES; i++) {
        // Use full opacity for all spheres
        axis_spheres_[i].alpha = 1.0f;
        
        // Store original axis color
        axis_spheres_[i].color = (axis_spheres_[i].axis == 0) ? x_axis_color_ :
                                (axis_spheres_[i].axis == 1) ? y_axis_color_ : z_axis_color_;
    }
    
    // Sort sphere indices by depth (back to front for proper blending)
    for (int i = 0; i < NUM_AXES; i++) {
        sorted_sphere_indices_[i] = i;
    }
    std::sort(sorted_sphere_indices_.begin(), sorted_sphere_indices_.end(),
             [this](int a, int b) {
                 return axis_spheres_[a].depth < axis_spheres_[b].depth;
             });
    
    // Update axis connectors (only positive axes have lines)
    axis_lines_.clear();
    
    // Line goes from center to just before the sphere
    // axis_distance_ is 0.8 in normalized space, sphere_radius_ is 0.2
    float line_length = axis_distance_ * 0.7f;    // Stop well before sphere
    
    // Define line endpoints in world space - all start from center
    glm::vec3 line_starts[3] = {
        glm::vec3(0, 0, 0),  // +X start at center
        glm::vec3(0, 0, 0),  // +Y start at center
        glm::vec3(0, 0, 0)   // +Z start at center
    };
    
    glm::vec3 line_ends[3] = {
        glm::vec3(line_length, 0, 0),  // +X end
        glm::vec3(0, line_length, 0),  // +Y end
        glm::vec3(0, 0, line_length)   // +Z end
    };
    
    AxisLine lines[3];
    
    // Transform lines by camera rotation
    for (int i = 0; i < 3; i++) {
        glm::vec4 start_world(line_starts[i], 1.0f);
        glm::vec4 end_world(line_ends[i], 1.0f);
        
        // Transform to view space
        glm::vec4 start_view = rotation_only * start_world;
        glm::vec4 end_view = rotation_only * end_world;
        
        // Store transformed positions
        lines[i].start = {start_view.x, start_view.y, start_view.z};
        lines[i].end = {end_view.x, end_view.y, end_view.z};
        lines[i].color = (i == 0) ? x_axis_color_ : (i == 1) ? y_axis_color_ : z_axis_color_;
        lines[i].width = line_thickness_;
        lines[i].depth = end_view.z;
        sorted_line_indices_[i] = i;
    }
    
    // Sort by depth (back to front)
    std::sort(sorted_line_indices_.begin(), sorted_line_indices_.end(),
             [&lines](int a, int b) {
                 return lines[a].depth < lines[b].depth;
             });
    
    // Add sorted lines to render list
    for (int i = 0; i < 3; i++) {
        axis_lines_.push_back(lines[sorted_line_indices_[i]]);
    }
}

void NavigationWidget::render(CanvasRenderer* renderer, const Camera3D& camera, const Rect2D& viewport) {
    if (!visible_ || !initialized_) {
        return;
    }
    
    current_viewport_ = viewport;
    
    // Get the actual viewport size from the renderer
    // This gives us the size that the projection matrix is set up for
    Point2D viewport_size = renderer->get_viewport_size();
    float ui_scale = renderer->get_content_scale();
    
    // Scale the widget size by UI scale factor
    // On retina displays (scale=2), widget should be 2x larger in pixels
    // Increase base size to 60 for better visibility
    widget_base_size_ = 60.0f;
    widget_size_ = widget_base_size_ * ui_scale;
    
    // Position widget in window coordinates (top-right corner)
    // The viewport size is already in logical/window coordinates
    float total_size = widget_size_ * 2.0f;
    position_.x = viewport_size.x - (margin_ * ui_scale) - total_size;
    position_.y = margin_ * ui_scale;
    
    // Calculate depth ordering for proper layering
    calculate_view_ordering(camera);
    
    // Save OpenGL state
    GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
    GLint blend_src, blend_dst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    // Clear vertex buffer
    vertex_buffer_.clear();
    
    // Draw components in depth order (back to front)
    draw_widget_backdrop(renderer);
    draw_orientation_indicators(renderer);  // Draw circles first
    draw_axis_connectors(renderer);        // Draw lines on top of circles
    draw_axis_text(renderer);
    
    // Restore OpenGL state
    if (!blend_enabled) glDisable(GL_BLEND);
    if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
    glBlendFunc(blend_src, blend_dst);
}

void NavigationWidget::draw_widget_backdrop(CanvasRenderer* renderer) {
    // Draw circular backdrop when widget is interactive
    if (is_active_ || hovered_axis_ >= 0) {
        // Widget center is at position + radius (since position is top-left)
        Point2D center(position_.x + widget_size_, position_.y + widget_size_);
        
        // Use highlight color when hovering, standard backdrop otherwise
        ColorRGBA backdrop = (hovered_axis_ >= 0) ? highlight_backdrop_ : backdrop_color_;
        
        // Draw smooth circular backdrop (full widget radius)
        render_filled_circle(renderer, center, widget_size_, backdrop);
    }
}

void NavigationWidget::draw_axis_connectors(CanvasRenderer* renderer) {
    // Widget center is at position + radius
    Point2D widget_center(position_.x + widget_size_, position_.y + widget_size_);
    float ui_scale = renderer->get_content_scale();
    
    // Draw lines from center to positive axis spheres
    for (const auto& line : axis_lines_) {
        // Draw all lines regardless of depth (no culling)
        
        // Transform endpoints to screen space
        Point2D start(widget_center.x + line.start.x * widget_size_,
                     widget_center.y - line.start.y * widget_size_);
        Point2D end(widget_center.x + line.end.x * widget_size_,
                   widget_center.y - line.end.y * widget_size_);
        
        // Use axis color with full opacity
        ColorRGBA line_color = line.color;
        line_color.a = 1.0f;
        
        // Calculate line thickness proportional to widget size
        // Use widget_size / 20 for good visibility
        float line_thickness = (widget_base_size_ / 20.0f) * ui_scale;
        renderer->draw_line(start, end, line_color, line_thickness);
    }
}

void NavigationWidget::draw_orientation_indicators(CanvasRenderer* renderer) {
    // Widget center is at position + radius
    Point2D widget_center(position_.x + widget_size_, position_.y + widget_size_);
    
    // Draw spheres in depth order (back to front)
    for (int idx = 0; idx < NUM_AXES; idx++) {
        int i = sorted_sphere_indices_[idx];
        const auto& sphere = axis_spheres_[i];
        
        // Draw all spheres regardless of alignment (no culling)
        
        // Calculate screen position (scale by widget radius)
        Point2D sphere_pos(
            widget_center.x + sphere.position.x * widget_size_ * 0.7f,
            widget_center.y - sphere.position.y * widget_size_ * 0.7f
        );
        
            // Fixed size for all spheres, no depth scaling
        // Sphere radius should be about 15% of widget radius for good proportions
        float screen_radius = 0.15f * widget_size_;
        
        // Apply hover scaling
        if (i == hovered_axis_) {
            screen_radius *= 1.15f;
        }
        
        // No depth fading, use original color
        ColorRGBA base_color = sphere.color;
        
        if (!sphere.positive) {
            // Negative axes - hollow sphere with ring
            // Interior fill: mix between background and transparent axis color based on depth
            float depth_factor = (sphere.depth + 1.0f) * 0.5f;  // 0 = back, 1 = front
            
            // As sphere comes to front, blend from axis color with low alpha to background at full opacity
            ColorRGBA interior_color;
            if (depth_factor < 0.5f) {
                // Back half - more axis color
                interior_color = sphere.color;
                interior_color.a = 0.3f;
            } else {
                // Front half - blend towards background
                float blend = (depth_factor - 0.5f) * 2.0f;  // 0 to 1 for front half
                interior_color.r = sphere.color.r * (1.0f - blend) + viewport_bg_.r * blend;
                interior_color.g = sphere.color.g * (1.0f - blend) + viewport_bg_.g * blend;
                interior_color.b = sphere.color.b * (1.0f - blend) + viewport_bg_.b * blend;
                interior_color.a = 0.3f + blend * 0.7f;  // Alpha goes from 0.3 to 1.0
            }
            
            // Fill interior
            render_filled_circle(renderer, sphere_pos, screen_radius - 3.0f, interior_color);
            
            // Solid colored ring outline
            ColorRGBA ring_color = sphere.color;
            ring_color.a = 1.0f;  // Always full opacity for outline
            render_ring_shape(renderer, sphere_pos, screen_radius, ring_color, 3.0f);
        } else {
            // Positive axes - solid filled sphere, no depth effects
            ColorRGBA sphere_color = sphere.color;
            sphere_color.a = 1.0f;  // Always full opacity
            
            // Enhance color on hover
            if (i == hovered_axis_) {
                sphere_color.r = std::min(1.0f, sphere_color.r * 1.2f);
                sphere_color.g = std::min(1.0f, sphere_color.g * 1.2f);
                sphere_color.b = std::min(1.0f, sphere_color.b * 1.2f);
            }
            
            // Main sphere body - solid filled circle, no highlights
            render_filled_circle(renderer, sphere_pos, screen_radius, sphere_color);
        }
    }
}

void NavigationWidget::draw_axis_text(CanvasRenderer* renderer) {
    if (!g_font_system) {
        return;
    }
    
    // Widget center is at position + radius
    Point2D widget_center(position_.x + widget_size_, position_.y + widget_size_);
    
    // Draw text labels for visible axis indicators
    for (int i = 0; i < NUM_AXES; i++) {
        const auto& sphere = axis_spheres_[i];
        
        // For negative axes, only show text on hover
        if (!sphere.positive && i != hovered_axis_) {
            continue;
        }
        
        // Calculate screen position (matching sphere positioning)
        Point2D sphere_pos(
            widget_center.x + sphere.position.x * widget_size_ * 0.7f,
            widget_center.y - sphere.position.y * widget_size_ * 0.7f
        );
        
        // Create label text
        std::string label;
        if (sphere.axis == 0) label = "X";
        else if (sphere.axis == 1) label = "Y";
        else if (sphere.axis == 2) label = "Z";
        
        // Negative axes show minus
        if (!sphere.positive) {
            label = "-" + label;
        }
        
        // Calculate font size proportional to sphere size
        // Use sphere radius * 1.25 for good readability
        // Our sphere radius is 0.15f * widget_size_, so text should be proportional
        float sphere_screen_radius = 0.15f * widget_size_;
        float font_size = sphere_screen_radius * 1.25f;  // Proportional to sphere size
        
        Point2D text_size = g_font_system->measure_text(label, "Inter-Bold", font_size);
        
        // Center text position mathematically
        // text_size.y is the full line height (ascender + |descender|)
        // The text is rendered with its top at position.y
        // For centering: text middle should be at sphere center
        // text middle = position.y + text_size.y/2
        // We want: position.y + text_size.y/2 = sphere_pos.y
        // Therefore: position.y = sphere_pos.y - text_size.y/2
        Point2D text_pos(
            std::round(sphere_pos.x - text_size.x / 2),    // Horizontal center
            std::round(sphere_pos.y - text_size.y / 2)     // Vertical center
        );
        
        // Determine text color based on hover state
        ColorRGBA text_color;
        if (i == hovered_axis_) {
            // White text on hover
            text_color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            // Black text for non-hovered positive axes
            text_color = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);
        }
        
        // Main text (no shadow)
        g_font_system->render_text(renderer, label, text_pos, "Inter-Bold", font_size, text_color);
    }
}

int NavigationWidget::hit_test(const Point2D& mouse_pos) const {
    if (!visible_) {
        return -1;
    }
    
    // Widget center is at position + radius
    Point2D widget_center(position_.x + widget_size_, position_.y + widget_size_);
    
    // Check if mouse is within widget bounds
    float dx = mouse_pos.x - widget_center.x;
    float dy = mouse_pos.y - widget_center.y;
    float dist_from_center = std::sqrt(dx * dx + dy * dy);
    
    if (dist_from_center > widget_size_) {
        return -1;
    }
    
    // Check each sphere for hit
    float closest_dist = std::numeric_limits<float>::max();
    int closest_sphere = -1;
    
    for (int i = 0; i < NUM_AXES; i++) {
        const auto& sphere = axis_spheres_[i];
        
        // Calculate screen position of sphere (matching draw positioning)
        Point2D sphere_pos(
            widget_center.x + sphere.position.x * widget_size_ * 0.7f,
            widget_center.y - sphere.position.y * widget_size_ * 0.7f
        );
        
        // Check distance to sphere center
        float sdx = mouse_pos.x - sphere_pos.x;
        float sdy = mouse_pos.y - sphere_pos.y;
        float dist = std::sqrt(sdx * sdx + sdy * sdy);
        
        // Hit radius is slightly larger than visual radius for easier clicking
        float hit_radius = sphere_radius_ * widget_size_ * 1.2f;
        
        if (dist < hit_radius && dist < closest_dist) {
            closest_dist = dist;
            closest_sphere = i;
        }
    }
    
    return closest_sphere;
}

void NavigationWidget::handle_click(int axis_id) {
    if (axis_id < 0 || axis_id >= NUM_AXES) {
        return;
    }
    
    const auto& sphere = axis_spheres_[axis_id];
    
    // Call the callback if set
    if (axis_click_callback_) {
        axis_click_callback_(sphere.axis, sphere.positive);
    }
}

void NavigationWidget::set_hover_axis(int axis_id) {
    hovered_axis_ = axis_id;
}

// Custom rendering utilities implementation
void NavigationWidget::render_filled_circle(CanvasRenderer* renderer, const Point2D& center,
                                           float radius, const ColorRGBA& color) {
    // Draw filled circle using Canvas renderer
    const int segments = 32;
    renderer->draw_circle(center, radius, color, segments);
}

void NavigationWidget::render_ring_shape(CanvasRenderer* renderer, const Point2D& center,
                                        float radius, const ColorRGBA& color, float thickness) {
    // Draw ring using dedicated ring drawing function
    renderer->draw_circle_ring(center, radius, color, thickness, 32);
}


// Visual effect calculation functions
ColorRGBA NavigationWidget::calculate_depth_fade(const ColorRGBA& base_color, float depth) {
    // Interpolate between viewport background and base color based on depth
    // depth: 0 = back, 1 = front
    float fade_factor = 0.5f + (depth * 0.5f);  // Range [0.5, 1.0]
    
    ColorRGBA result;
    result.r = viewport_bg_.r + (base_color.r - viewport_bg_.r) * fade_factor;
    result.g = viewport_bg_.g + (base_color.g - viewport_bg_.g) * fade_factor;
    result.b = viewport_bg_.b + (base_color.b - viewport_bg_.b) * fade_factor;
    result.a = base_color.a * (0.5f + depth * 0.5f);  // Alpha from 50% to 100%
    
    return result;
}

float NavigationWidget::calculate_size_from_depth(float base_size, float depth) {
    // No size scaling - return base size
    return base_size;
}

} // namespace voxel_canvas