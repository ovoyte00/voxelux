/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - OpenGL renderer implementation
 */

#include "glad/gl.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/canvas_window.h"
#include "canvas_ui/font_system.h"
#include "canvas_ui/polyline_shader.h"
#include "canvas_ui/scaled_theme.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace voxel_canvas {

// OpenGL error checking helper for debugging
static void check_gl_error(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << operation << ": " << error;
        switch (error) {
            case GL_INVALID_ENUM: std::cerr << " (GL_INVALID_ENUM)"; break;
            case GL_INVALID_VALUE: std::cerr << " (GL_INVALID_VALUE)"; break;
            case GL_INVALID_OPERATION: std::cerr << " (GL_INVALID_OPERATION)"; break;
            case GL_OUT_OF_MEMORY: std::cerr << " (GL_OUT_OF_MEMORY)"; break;
            default: std::cerr << " (Unknown)"; break;
        }
        std::cerr << std::endl;
    }
}

// RenderBatch implementation

void RenderBatch::add_quad(const Rect2D& rect, const ColorRGBA& color) {
    uint32_t base_index = static_cast<uint32_t>(vertices.size());
    
    // Add four vertices for the quad
    vertices.emplace_back(rect.x, rect.y, 0, 0, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x + rect.width, rect.y, 1, 0, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x + rect.width, rect.y + rect.height, 1, 1, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x, rect.y + rect.height, 0, 1, color.r, color.g, color.b, color.a);
    
    // Add two triangles
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 1);
    indices.push_back(base_index + 2);
    
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 2);
    indices.push_back(base_index + 3);
}

void RenderBatch::add_textured_quad(const Rect2D& rect, const Rect2D& uv_rect, const ColorRGBA& color) {
    uint32_t base_index = static_cast<uint32_t>(vertices.size());
    
    // Add four vertices with texture coordinates
    vertices.emplace_back(rect.x, rect.y, uv_rect.x, uv_rect.y, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x + rect.width, rect.y, uv_rect.x + uv_rect.width, uv_rect.y, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x + rect.width, rect.y + rect.height, uv_rect.x + uv_rect.width, uv_rect.y + uv_rect.height, color.r, color.g, color.b, color.a);
    vertices.emplace_back(rect.x, rect.y + rect.height, uv_rect.x, uv_rect.y + uv_rect.height, color.r, color.g, color.b, color.a);
    
    // Add two triangles
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 1);
    indices.push_back(base_index + 2);
    
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 2);
    indices.push_back(base_index + 3);
}

void RenderBatch::add_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float thickness) {
    // For now, render lines as thin rectangles
    Point2D dir(end.x - start.x, end.y - start.y);
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length == 0) return;
    
    dir.x /= length;
    dir.y /= length;
    
    Point2D normal(-dir.y * thickness * 0.5f, dir.x * thickness * 0.5f);
    
    uint32_t base_index = static_cast<uint32_t>(vertices.size());
    
    vertices.emplace_back(start.x + normal.x, start.y + normal.y, 0, 0, color.r, color.g, color.b, color.a);
    vertices.emplace_back(start.x - normal.x, start.y - normal.y, 0, 0, color.r, color.g, color.b, color.a);
    vertices.emplace_back(end.x - normal.x, end.y - normal.y, 0, 0, color.r, color.g, color.b, color.a);
    vertices.emplace_back(end.x + normal.x, end.y + normal.y, 0, 0, color.r, color.g, color.b, color.a);
    
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 1);
    indices.push_back(base_index + 2);
    
    indices.push_back(base_index + 0);
    indices.push_back(base_index + 2);
    indices.push_back(base_index + 3);
}

// CanvasRenderer implementation

CanvasRenderer::CanvasRenderer(CanvasWindow* window)
    : window_(window)
    , ui_vao_(0)
    , ui_vbo_(0)
    , ui_ebo_(0)
    , default_fbo_(0)
    , current_fbo_(0)
    , viewport_x_(0)
    , viewport_y_(0)
    , viewport_width_(0)
    , viewport_height_(0)
    , global_opacity_(1.0f)
    , path_started_(false)
    , white_texture_(0)
    , checker_texture_(0)
    , scissor_enabled_(false) {
}

CanvasRenderer::~CanvasRenderer() {
    shutdown();
}

bool CanvasRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    // Get default framebuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&default_fbo_));
    current_fbo_ = default_fbo_;
    
    // Initialize transform matrix to identity
    identity_matrix(current_transform_.matrix);

    // Enhanced OpenGL state setup for professional rendering
    
    // Critical pixel store setup for texture alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    check_gl_error("pixel store setup");
    
    // Disable problematic features
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    check_gl_error("disable features");
    
    // Setup blending for UI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    check_gl_error("blend setup");
    
    // Verify blend is actually enabled
    GLboolean blend_enabled;
    glGetBooleanv(GL_BLEND, &blend_enabled);

    // Setup shaders
    setup_shaders();
    
    // Setup buffers
    setup_buffers();
    
    // Setup default textures
    setup_default_textures();

    // Initialize font system
    font_system_ = std::make_unique<FontSystem>();
    if (!font_system_->initialize()) {
        std::cerr << "Failed to initialize font system" << std::endl;
        return false;
    }
    
    // Initialize polyline shader for thick line rendering
    polyline_shader_ = std::make_unique<PolylineShader>();
    if (!polyline_shader_->initialize()) {
        std::cerr << "Failed to initialize polyline shader" << std::endl;
        // Non-fatal, fall back to regular line rendering
    }
    
    // Load default fonts (Inter)
    if (!font_system_->load_default_fonts()) {
        std::cerr << "Warning: Failed to load default fonts" << std::endl;
    }

    // Set initial viewport
    Point2D fb_size = window_->get_framebuffer_size();
    set_viewport(0, 0, static_cast<int>(fb_size.x), static_cast<int>(fb_size.y));

    initialized_ = true;
    return true;
}

void CanvasRenderer::shutdown() {
    if (!initialized_) {
        return;
    }

    font_system_.reset();

    if (white_texture_) {
        glDeleteTextures(1, &white_texture_);
        white_texture_ = 0;
    }
    
    if (checker_texture_) {
        glDeleteTextures(1, &checker_texture_);
        checker_texture_ = 0;
    }

    ui_shader_.reset();
    text_shader_.reset();

    if (ui_vao_) {
        glDeleteVertexArrays(1, &ui_vao_);
        ui_vao_ = 0;
    }
    
    if (ui_vbo_) {
        glDeleteBuffers(1, &ui_vbo_);
        ui_vbo_ = 0;
    }
    
    if (ui_ebo_) {
        glDeleteBuffers(1, &ui_ebo_);
        ui_ebo_ = 0;
    }

    initialized_ = false;
}

void CanvasRenderer::begin_frame() {
    // Frame begins
    
    draw_calls_this_frame_ = 0;
    vertices_this_frame_ = 0;
    
    // Clear render batches
    render_batches_.clear();
    batch_render_order_.clear();
    
    // Start occlusion tracking for this frame
    occlusion_tracker_.begin_frame();
    
    // Force flush any pending OpenGL commands first
    glFlush();
    
    // Ensure we're rendering to default framebuffer (screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl_error("bind default framebuffer");
    
    
    // **Canvas UI Professional Background System** - Now using proper theme colors
    // UI rendering is confirmed working, switch to production colors
    ColorRGBA bg = theme_.background_primary; // Professional dark background
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    check_gl_error("clear color");
    
    glClear(GL_COLOR_BUFFER_BIT);
    check_gl_error("clear");
    
}

void CanvasRenderer::end_frame() {
    // Flush any pending batched draw calls
    flush_current_batch();
    
    // Render all batches sorted by state
    render_sorted_batches();
    
    // Draw occlusion debug visualization if enabled
    if (occlusion_tracker_.is_debug_mode()) {
        draw_occlusion_debug();
    }
    
    // End occlusion tracking for this frame
    occlusion_tracker_.end_frame();
    
    // CRITICAL: Reset OpenGL state after UI rendering
    glDisable(GL_SCISSOR_TEST);
    check_gl_error("disable scissor test");
    
    // Reset blend state to default 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    check_gl_error("reset blend function");
    
    // Reset depth state
    glDepthMask(GL_TRUE);
    check_gl_error("reset depth mask");
    
}

void CanvasRenderer::present_frame() {
    // Swap buffers handled by CanvasWindow
}

void CanvasRenderer::set_viewport(int x, int y, int width, int height) {
    viewport_x_ = x;
    viewport_y_ = y;
    viewport_width_ = width;
    viewport_height_ = height;
    
    
    glViewport(x, y, width, height);
    check_gl_error("glViewport");
    
    // Standard orthographic projection for UI rendering
    // Maps screen coordinates: (0,0) = top-left, (width,height) = bottom-right
    float projection[16] = {
        2.0f / width,  0,             0, 0,
        0,            -2.0f / height, 0, 0,  // Negative Y to flip coordinates (top-left origin)
        0,             0,            -1, 0,
        -1,            1,             0, 1   // Translate to NDC space
    };
    
    if (ui_shader_) {
        ui_shader_->use();
        ui_shader_->set_uniform("u_projection", projection, 16);
        check_gl_error("set projection matrix");
        
        
        ui_shader_->unuse();
    } else {
        std::cerr << "WARNING: ui_shader_ is null when setting viewport!" << std::endl;
    }
}

Point2D CanvasRenderer::get_viewport_size() const {
    return Point2D(static_cast<float>(viewport_width_), static_cast<float>(viewport_height_));
}

void CanvasRenderer::get_projection_matrix(float* matrix) const {
    if (!matrix) return;
    
    // Create orthographic projection matrix for 2D rendering
    // Maps screen coordinates to NDC (-1 to 1)
    float width = static_cast<float>(viewport_width_);
    float height = static_cast<float>(viewport_height_);
    
    // Prevent division by zero
    if (width <= 0) width = 1.0f;
    if (height <= 0) height = 1.0f;
    
    // Column-major order for OpenGL
    matrix[0] = 2.0f / width;   matrix[4] = 0.0f;           matrix[8] = 0.0f;  matrix[12] = -1.0f;
    matrix[1] = 0.0f;            matrix[5] = -2.0f / height; matrix[9] = 0.0f;  matrix[13] = 1.0f;
    matrix[2] = 0.0f;            matrix[6] = 0.0f;           matrix[10] = -1.0f; matrix[14] = 0.0f;
    matrix[3] = 0.0f;            matrix[7] = 0.0f;           matrix[11] = 0.0f;  matrix[15] = 1.0f;
}

float CanvasRenderer::get_content_scale() const {
    if (window_) {
        return window_->get_content_scale();
    }
    return 1.0f;
}

ScaledTheme CanvasRenderer::get_scaled_theme() const {
    return ScaledTheme(theme_, get_content_scale());
}

void CanvasRenderer::draw_rect(const Rect2D& rect, const ColorRGBA& color) {
    // **Canvas UI Professional Batched Rendering**
    // Accumulate draw calls into batches for massive performance improvement
    
    // Validate shader state first
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        std::cerr << "ERROR: UI shader not valid in draw_rect!" << std::endl;
        return;
    }
    
    // Ensure white texture exists
    if (white_texture_ == 0 || !glIsTexture(white_texture_)) {
        GLubyte white_pixel[4] = {255, 255, 255, 255};
        glGenTextures(1, &white_texture_);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    // Check if we can batch with current batch
    GLuint shader_id = ui_shader_->get_id();
    GLenum blend_mode = GL_SRC_ALPHA; // Use proper alpha blending for UI elements
    
    if (!current_batch_.can_batch_with(white_texture_, blend_mode, shader_id)) {
        // Flush current batch before starting new one
        flush_current_batch();
        current_batch_.texture_id = white_texture_;
        current_batch_.blend_mode = blend_mode;
        current_batch_.shader_id = shader_id;
    }
    
    // Add vertices to batch
    uint32_t base_index = current_batch_.vertices.size();
    
    current_batch_.vertices.push_back(UIVertex(rect.x, rect.y, 0, 0, color.r, color.g, color.b, color.a));
    current_batch_.vertices.push_back(UIVertex(rect.x + rect.width, rect.y, 1, 0, color.r, color.g, color.b, color.a));
    current_batch_.vertices.push_back(UIVertex(rect.x + rect.width, rect.y + rect.height, 1, 1, color.r, color.g, color.b, color.a));
    current_batch_.vertices.push_back(UIVertex(rect.x, rect.y + rect.height, 0, 1, color.r, color.g, color.b, color.a));
    
    // Add indices
    current_batch_.indices.push_back(base_index + 0);
    current_batch_.indices.push_back(base_index + 1);
    current_batch_.indices.push_back(base_index + 2);
    current_batch_.indices.push_back(base_index + 0);
    current_batch_.indices.push_back(base_index + 2);
    current_batch_.indices.push_back(base_index + 3);
    
    // Update statistics
    vertices_this_frame_ += 4;
}

void CanvasRenderer::draw_rect_outline(const Rect2D& rect, const ColorRGBA& color, float line_width) {
    draw_line(Point2D(rect.x, rect.y), Point2D(rect.x + rect.width, rect.y), color, line_width);
    draw_line(Point2D(rect.x + rect.width, rect.y), Point2D(rect.x + rect.width, rect.y + rect.height), color, line_width);
    draw_line(Point2D(rect.x + rect.width, rect.y + rect.height), Point2D(rect.x, rect.y + rect.height), color, line_width);
    draw_line(Point2D(rect.x, rect.y + rect.height), Point2D(rect.x, rect.y), color, line_width);
}

void CanvasRenderer::draw_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float width) {
    // Use polyline shader if available and line is thick
    if (polyline_shader_ && polyline_shader_->is_valid() && width > 1.0f) {
        polyline_shader_->use();
        
        // Set uniforms
        float projection[16];
        get_projection_matrix(projection);
        polyline_shader_->set_projection(projection);
        polyline_shader_->set_viewport_size(viewport_width_, viewport_height_);
        polyline_shader_->set_line_width(width);
        polyline_shader_->set_line_smooth(true);  // Enable anti-aliasing
        
        // Enable blending for smooth edges
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Draw the line
        polyline_shader_->draw_line(start.x, start.y, end.x, end.y, 
                                   color.r, color.g, color.b, color.a);
        
        polyline_shader_->unuse();
        
        draw_calls_this_frame_++;
        vertices_this_frame_ += 2;
        return;
    }
    
    // Fallback to quad-based approach if polyline shader not available
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        return;
    }
    
    // Calculate line as a rotated rectangle
    Point2D dir(end.x - start.x, end.y - start.y);
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length == 0) return;
    
    dir.x /= length;
    dir.y /= length;
    
    Point2D normal(-dir.y * width * 0.5f, dir.x * width * 0.5f);
    
    // Create quad vertices for the line - using proper winding order
    UIVertex vertices[4] = {
        UIVertex(start.x - normal.x, start.y - normal.y, 0, 0, color.r, color.g, color.b, color.a),
        UIVertex(end.x - normal.x, end.y - normal.y, 1, 0, color.r, color.g, color.b, color.a),
        UIVertex(end.x + normal.x, end.y + normal.y, 1, 1, color.r, color.g, color.b, color.a),
        UIVertex(start.x + normal.x, start.y + normal.y, 0, 1, color.r, color.g, color.b, color.a)
    };
    
    uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
    
    // Use shader and set uniforms
    ui_shader_->use();
    
    // Set projection matrix
    float vp_width = std::max(1.0f, (float)viewport_width_);
    float vp_height = std::max(1.0f, (float)viewport_height_);
    float projection[16] = {
        2.0f / vp_width,  0,                  0, 0,
        0,               -2.0f / vp_height,    0, 0,
        0,                0,                  -1, 0,
        -1,               1,                   0, 1
    };
    ui_shader_->set_uniform("u_projection", projection, 16);
    
    // Set SDF uniforms for solid line (no rounding)
    ui_shader_->set_uniform("u_widget_rect", ColorRGBA(0, 0, 1, 1));
    ui_shader_->set_uniform("u_corner_radius", ColorRGBA(0, 0, 0, 0));
    ui_shader_->set_uniform("u_border_width", 0.0f);
    ui_shader_->set_uniform("u_border_color", ColorRGBA(0, 0, 0, 0));
    ui_shader_->set_uniform("u_aa_radius", 1.0f);
    ui_shader_->set_uniform("u_texture", 0);
    
    // Ensure texture is valid before binding
    if (white_texture_ == 0 || !glIsTexture(white_texture_)) {
        // Recreate texture if invalid
        GLubyte white_pixel[4] = {255, 255, 255, 255};
        glGenTextures(1, &white_texture_);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glBindTexture(GL_TEXTURE_2D, white_texture_);
    }
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable face culling for lines
    glDisable(GL_CULL_FACE);
    
    glBindVertexArray(ui_vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    
    ui_shader_->unuse();
    glBindVertexArray(0);
    
    // Force GPU to execute commands
    glFlush();
    
    draw_calls_this_frame_++;
    vertices_this_frame_ += 4;
}

void CanvasRenderer::draw_circle(const Point2D& center, float radius, const ColorRGBA& color, int segments) {
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        std::cerr << "ERROR: UI shader not valid for circle!" << std::endl;
        return;
    }
    
    
    // Generate circle vertices using triangle fan
    // Format: position(2) + texcoord(2) + color(4) = 8 floats per vertex
    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 8); // 8 floats per vertex
    
    // Center vertex
    vertices.push_back(center.x);      // position x
    vertices.push_back(center.y);      // position y
    vertices.push_back(0.5f);          // texcoord u (center)
    vertices.push_back(0.5f);          // texcoord v (center)
    vertices.push_back(color.r);       // color r
    vertices.push_back(color.g);       // color g
    vertices.push_back(color.b);       // color b
    vertices.push_back(color.a);       // color a
    
    // Generate perimeter vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = center.x + radius * std::cos(angle);
        float y = center.y + radius * std::sin(angle);
        float u = 0.5f + 0.5f * std::cos(angle);
        float v = 0.5f + 0.5f * std::sin(angle);
        
        vertices.push_back(x);          // position x
        vertices.push_back(y);          // position y
        vertices.push_back(u);          // texcoord u
        vertices.push_back(v);          // texcoord v
        vertices.push_back(color.r);    // color r
        vertices.push_back(color.g);    // color g
        vertices.push_back(color.b);    // color b
        vertices.push_back(color.a);    // color a
    }
    
    // Use shader
    ui_shader_->use();
    
    // Set uniforms
    GLint proj_loc = glGetUniformLocation(ui_shader_->get_id(), "u_projection");
    if (proj_loc >= 0) {
        float proj_matrix[16];
        get_projection_matrix(proj_matrix);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj_matrix);
    }
    
    // Bind white texture (shader expects a texture even if we're using vertex colors)
    GLint tex_loc = glGetUniformLocation(ui_shader_->get_id(), "u_texture");
    if (tex_loc >= 0) {
        glUniform1i(tex_loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
    }
    
    // Set widget rect uniform (disable rounded corners for circles)
    GLint rect_loc = glGetUniformLocation(ui_shader_->get_id(), "u_widget_rect");
    if (rect_loc >= 0) {
        glUniform4f(rect_loc, 0, 0, 10000, 10000); // Large rect to disable corner rounding
    }
    
    // Set corner radius to 0 (we're drawing a circle, not a rounded rect)
    GLint corner_loc = glGetUniformLocation(ui_shader_->get_id(), "u_corner_radius");
    if (corner_loc >= 0) {
        glUniform4f(corner_loc, 0, 0, 0, 0);
    }
    
    // Set border width to 0
    GLint border_loc = glGetUniformLocation(ui_shader_->get_id(), "u_border_width");
    if (border_loc >= 0) {
        glUniform1f(border_loc, 0);
    }
    
    // Create temporary VAO/VBO for circle
    GLuint circle_vao, circle_vbo;
    glGenVertexArrays(1, &circle_vao);
    glGenBuffers(1, &circle_vbo);
    
    glBindVertexArray(circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    
    // Setup vertex attributes matching the shader layout
    // Layout: position(2) + texcoord(2) + color(4) = 8 floats per vertex
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2); // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw circle as triangle fan
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &circle_vao);
    glDeleteBuffers(1, &circle_vbo);
    
    glUseProgram(0);
}

void CanvasRenderer::draw_circle_ring(const Point2D& center, float radius, const ColorRGBA& color, float thickness, int segments) {
    // Draw ring (hollow circle) using triangle strip
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        return;
    }
    
    // Format: position(2) + texcoord(2) + color(4) = 8 floats per vertex
    std::vector<float> vertices;
    vertices.reserve((segments + 1) * 2 * 8); // Two vertices per segment (inner and outer)
    
    float inner_radius = radius - thickness;
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        // Outer vertex
        float outer_x = center.x + radius * cos_a;
        float outer_y = center.y + radius * sin_a;
        vertices.push_back(outer_x);        // position x
        vertices.push_back(outer_y);        // position y
        vertices.push_back(0.5f + 0.5f * cos_a);  // texcoord u
        vertices.push_back(0.5f + 0.5f * sin_a);  // texcoord v
        vertices.push_back(color.r);        // color r
        vertices.push_back(color.g);        // color g
        vertices.push_back(color.b);        // color b
        vertices.push_back(color.a);        // color a
        
        // Inner vertex
        float inner_x = center.x + inner_radius * cos_a;
        float inner_y = center.y + inner_radius * sin_a;
        vertices.push_back(inner_x);        // position x
        vertices.push_back(inner_y);        // position y
        vertices.push_back(0.5f + 0.4f * cos_a);  // texcoord u (slightly inset)
        vertices.push_back(0.5f + 0.4f * sin_a);  // texcoord v (slightly inset)
        vertices.push_back(color.r);        // color r
        vertices.push_back(color.g);        // color g
        vertices.push_back(color.b);        // color b
        vertices.push_back(color.a);        // color a
    }
    
    // Create temporary VAO and VBO for the ring
    GLuint ring_vao, ring_vbo;
    glGenVertexArrays(1, &ring_vao);
    glGenBuffers(1, &ring_vbo);
    
    glBindVertexArray(ring_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ring_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    // Set up shader
    ui_shader_->use();
    
    // Set projection matrix
    float vp_width = std::max(1.0f, (float)viewport_width_);
    float vp_height = std::max(1.0f, (float)viewport_height_);
    float projection[16] = {
        2.0f / vp_width,  0,                  0, 0,
        0,               -2.0f / vp_height,    0, 0,
        0,                0,                  -1, 0,
        -1,               1,                   0, 1
    };
    ui_shader_->set_uniform("u_projection", projection, 16);
    
    // Set texture uniform
    ui_shader_->set_uniform("u_texture", 0);
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    
    // Setup vertex attributes matching the shader layout
    // Layout: position(2) + texcoord(2) + color(4) = 8 floats per vertex
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2); // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw ring as triangle strip
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (segments + 1) * 2);
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &ring_vao);
    glDeleteBuffers(1, &ring_vbo);
    
    glUseProgram(0);
}

void CanvasRenderer::draw_text(const std::string& text, const Point2D& position, const ColorRGBA& color, 
                              float size, TextAlign align, TextBaseline baseline) {
    // CRITICAL: Flush any pending rectangle batches before text rendering
    // Text uses a different shader and immediate mode rendering
    flush_current_batch();
    render_sorted_batches();
    
    if (font_system_) {
        Point2D adjusted_pos = position;
        
        // Get actual font metrics from the font system
        FontFace* font = font_system_->get_font("Inter");
        if (!font) {
            // Fallback if font not found
            font_system_->render_text(this, text, adjusted_pos, "Inter", static_cast<int>(size), color);
            return;
        }
        
        // Get real font metrics
        float ascender = font->get_ascender(static_cast<int>(size));
        float descender = font->get_descender(static_cast<int>(size));
        float text_height = ascender - descender; // descender is negative
        
        // Measure text width for horizontal alignment
        Point2D text_size = font_system_->measure_text(text, "Inter", static_cast<int>(size));
        float text_width = text_size.x;
        
        // Adjust X position based on horizontal alignment
        switch (align) {
            case TextAlign::LEFT:
                // Position is already at left edge
                break;
            case TextAlign::CENTER:
                adjusted_pos.x -= text_width / 2.0f;
                break;
            case TextAlign::RIGHT:
                adjusted_pos.x -= text_width;
                break;
        }
        
        // Adjust Y position based on baseline
        // Note: In font metrics, Y increases downward
        switch (baseline) {
            case TextBaseline::TOP:
                // Position is at top, no adjustment needed
                break;
            case TextBaseline::MIDDLE:
                // For visual centering of UI text (capitals and mixed case)
                // The optical center is slightly above geometric center
                // because lowercase letters and descenders pull visual weight down
                // 
                // Formula: We want to center based on "optical height"
                // For typical UI text, optical center is at ~65% of ascender height
                // This accounts for capitals being ~70% of ascender
                adjusted_pos.y -= ascender * 0.65f;
                break;
            case TextBaseline::BOTTOM:
                // Position is at bottom, move up by full text height
                adjusted_pos.y -= text_height;
                break;
        }
        
        font_system_->render_text(this, text, adjusted_pos, "Inter", static_cast<int>(size), color);
        
        // After text rendering, reset batch state since text uses its own shader
        // This ensures subsequent rectangle batches start fresh
        current_batch_.reset();
    }
}

void CanvasRenderer::draw_widget(const Rect2D& rect, const ColorRGBA& color,
                                float corner_radius, float border_width,
                                const ColorRGBA& border_color) {
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        return;
    }
    
    // Create quad vertices for the widget
    UIVertex vertices[4] = {
        UIVertex(rect.x, rect.y, 0, 0, color.r, color.g, color.b, color.a),
        UIVertex(rect.x + rect.width, rect.y, 1, 0, color.r, color.g, color.b, color.a),
        UIVertex(rect.x + rect.width, rect.y + rect.height, 1, 1, color.r, color.g, color.b, color.a),
        UIVertex(rect.x, rect.y + rect.height, 0, 1, color.r, color.g, color.b, color.a)
    };
    
    uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
    
    // Set up shader and projection
    ui_shader_->use();
    
    // Set projection matrix (critical for proper rendering)
    float vp_width = std::max(1.0f, (float)viewport_width_);
    float vp_height = std::max(1.0f, (float)viewport_height_);
    float projection[16] = {
        2.0f / vp_width,  0,                  0, 0,
        0,               -2.0f / vp_height,    0, 0,
        0,                0,                  -1, 0,
        -1,               1,                   0, 1
    };
    ui_shader_->set_uniform("u_projection", projection, 16);
    
    // Set up SDF uniforms
    ui_shader_->set_uniform("u_widget_rect", ColorRGBA(rect.x, rect.y, rect.width, rect.height));
    ui_shader_->set_uniform("u_corner_radius", ColorRGBA(corner_radius, corner_radius, corner_radius, corner_radius));
    ui_shader_->set_uniform("u_border_width", border_width);
    ui_shader_->set_uniform("u_border_color", border_color);
    ui_shader_->set_uniform("u_aa_radius", 1.0f); // Anti-aliasing radius in pixels
    ui_shader_->set_uniform("u_texture", 0);
    
    // Ensure texture is valid before binding
    if (white_texture_ == 0 || !glIsTexture(white_texture_)) {
        // Recreate texture if invalid
        GLubyte white_pixel[4] = {255, 255, 255, 255};
        glGenTextures(1, &white_texture_);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glBindTexture(GL_TEXTURE_2D, white_texture_);
    }
    
    glBindVertexArray(ui_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    // DON'T flush here - let OpenGL batch commands properly
    
    ui_shader_->unuse();
    glBindVertexArray(0);
}

void CanvasRenderer::draw_rounded_rect(const Rect2D& rect, const ColorRGBA& color, float corner_radius) {
    draw_widget(rect, color, corner_radius, 0.0f, ColorRGBA::transparent());
}

void CanvasRenderer::draw_rounded_rect_varied(const Rect2D& rect, const ColorRGBA& color,
                                              float tl_radius, float tr_radius, 
                                              float br_radius, float bl_radius) {
    // TODO: Implement varied corner radius rendering
    // For now, use average radius
    float avg_radius = (tl_radius + tr_radius + br_radius + bl_radius) / 4.0f;
    draw_rounded_rect(rect, color, avg_radius);
}

// Transform matrix operations
void CanvasRenderer::push_transform() {
    transform_stack_.push_back(current_transform_);
}

void CanvasRenderer::pop_transform() {
    if (!transform_stack_.empty()) {
        current_transform_ = transform_stack_.back();
        transform_stack_.pop_back();
        update_projection_matrix();
    }
}

void CanvasRenderer::translate(float x, float y) {
    float translation[16];
    identity_matrix(translation);
    translate_matrix(translation, x, y);
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, translation);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::rotate(float degrees) {
    float rotation[16];
    identity_matrix(rotation);
    rotate_matrix(rotation, degrees);
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, rotation);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::scale(float x, float y) {
    float scaling[16];
    identity_matrix(scaling);
    scale_matrix(scaling, x, y);
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, scaling);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::set_transform(const float* matrix4x4) {
    std::copy(matrix4x4, matrix4x4 + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::translate3d(float x, float y, float z) {
    float translation[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    };
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, translation);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::rotate3d(float angle_x, float angle_y, float angle_z) {
    // Convert degrees to radians
    float rx = angle_x * M_PI / 180.0f;
    float ry = angle_y * M_PI / 180.0f;
    float rz = angle_z * M_PI / 180.0f;
    
    // Rotation matrix X
    float rot_x[16] = {
        1, 0, 0, 0,
        0, std::cos(rx), std::sin(rx), 0,
        0, -std::sin(rx), std::cos(rx), 0,
        0, 0, 0, 1
    };
    
    // Rotation matrix Y
    float rot_y[16] = {
        std::cos(ry), 0, -std::sin(ry), 0,
        0, 1, 0, 0,
        std::sin(ry), 0, std::cos(ry), 0,
        0, 0, 0, 1
    };
    
    // Rotation matrix Z
    float rot_z[16] = {
        std::cos(rz), std::sin(rz), 0, 0,
        -std::sin(rz), std::cos(rz), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    // Combine rotations: Z * Y * X
    float temp[16], result[16];
    multiply_matrix(temp, rot_z, rot_y);
    multiply_matrix(result, temp, rot_x);
    
    float final[16];
    multiply_matrix(final, current_transform_.matrix, result);
    std::copy(final, final + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::rotate_axis(float angle, float axis_x, float axis_y, float axis_z) {
    // Normalize axis
    float length = std::sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
    if (length == 0) return;
    
    axis_x /= length;
    axis_y /= length;
    axis_z /= length;
    
    float rad = angle * M_PI / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);
    float t = 1.0f - c;
    
    // Rodrigues' rotation formula as matrix
    float rotation[16] = {
        t * axis_x * axis_x + c,
        t * axis_x * axis_y + s * axis_z,
        t * axis_x * axis_z - s * axis_y,
        0,
        
        t * axis_x * axis_y - s * axis_z,
        t * axis_y * axis_y + c,
        t * axis_y * axis_z + s * axis_x,
        0,
        
        t * axis_x * axis_z + s * axis_y,
        t * axis_y * axis_z - s * axis_x,
        t * axis_z * axis_z + c,
        0,
        
        0, 0, 0, 1
    };
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, rotation);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::scale3d(float x, float y, float z) {
    float scaling[16] = {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    };
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, scaling);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::apply_transform(const float* matrix4x4) {
    float result[16];
    multiply_matrix(result, current_transform_.matrix, matrix4x4);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

void CanvasRenderer::set_perspective(float fov_degrees, float near_plane, float far_plane) {
    float aspect = float(viewport_width_) / float(viewport_height_);
    float fov_rad = fov_degrees * M_PI / 180.0f;
    float f = 1.0f / std::tan(fov_rad / 2.0f);
    
    float perspective[16] = {
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far_plane + near_plane) / (near_plane - far_plane), -1,
        0, 0, (2 * far_plane * near_plane) / (near_plane - far_plane), 0
    };
    
    std::copy(perspective, perspective + 16, projection_matrix_);
    update_projection_matrix();
}

void CanvasRenderer::set_ortho(float left, float right, float bottom, float top, float near, float far) {
    float ortho[16] = {
        2.0f / (right - left), 0, 0, 0,
        0, 2.0f / (top - bottom), 0, 0,
        0, 0, -2.0f / (far - near), 0,
        -(right + left) / (right - left),
        -(top + bottom) / (top - bottom),
        -(far + near) / (far - near),
        1
    };
    
    std::copy(ortho, ortho + 16, projection_matrix_);
    update_projection_matrix();
}

void CanvasRenderer::skew(float angle_x, float angle_y) {
    float skew_x = std::tan(angle_x * M_PI / 180.0f);
    float skew_y = std::tan(angle_y * M_PI / 180.0f);
    
    float skew_matrix[16] = {
        1, skew_y, 0, 0,
        skew_x, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    float result[16];
    multiply_matrix(result, current_transform_.matrix, skew_matrix);
    std::copy(result, result + 16, current_transform_.matrix);
    update_projection_matrix();
}

// Clipping stack
void CanvasRenderer::push_clip(const Rect2D& rect) {
    // State change - flush pending batch
    flush_current_batch();
    
    // If there's already a clip, intersect with it
    if (!clip_stack_.empty()) {
        Rect2D current_clip = clip_stack_.back();
        
        // Calculate intersection
        float x1 = std::max(rect.x, current_clip.x);
        float y1 = std::max(rect.y, current_clip.y);
        float x2 = std::min(rect.x + rect.width, current_clip.x + current_clip.width);
        float y2 = std::min(rect.y + rect.height, current_clip.y + current_clip.height);
        
        Rect2D intersected(x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1));
        clip_stack_.push_back(intersected);
    } else {
        clip_stack_.push_back(rect);
    }
    
    enable_scissor(clip_stack_.back());
}

void CanvasRenderer::pop_clip() {
    if (!clip_stack_.empty()) {
        // State change - flush pending batch
        flush_current_batch();
        
        clip_stack_.pop_back();
        
        if (!clip_stack_.empty()) {
            enable_scissor(clip_stack_.back());
        } else {
            disable_scissor();
        }
    }
}

// Opacity management
void CanvasRenderer::set_global_opacity(float opacity) {
    global_opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

void CanvasRenderer::push_opacity(float opacity) {
    opacity_stack_.push_back(global_opacity_);
    global_opacity_ *= opacity;
}

void CanvasRenderer::pop_opacity() {
    if (!opacity_stack_.empty()) {
        global_opacity_ = opacity_stack_.back();
        opacity_stack_.pop_back();
    }
}

// Advanced rendering methods
void CanvasRenderer::draw_linear_gradient(const Rect2D& rect, float angle_degrees,
                                         const std::vector<std::pair<ColorRGBA, float>>& stops) {
    if (stops.size() < 2) return;
    
    // Convert angle to radians and calculate direction
    float radians = angle_degrees * (M_PI / 180.0f);
    float dx = std::sin(radians);
    float dy = -std::cos(radians);
    
    // For simplicity, we'll approximate with multiple rectangles
    // In a real implementation, this would use a shader
    const int segments = 20;
    
    for (int i = 0; i < segments; i++) {
        float t = static_cast<float>(i) / segments;
        float next_t = static_cast<float>(i + 1) / segments;
        
        // Interpolate color based on stops
        ColorRGBA color = interpolate_gradient_color(stops, t);
        ColorRGBA next_color = interpolate_gradient_color(stops, next_t);
        
        // Calculate segment bounds
        Rect2D segment_rect;
        if (std::abs(dx) > std::abs(dy)) {
            // More horizontal
            segment_rect.x = rect.x + rect.width * t;
            segment_rect.y = rect.y;
            segment_rect.width = rect.width / segments;
            segment_rect.height = rect.height;
        } else {
            // More vertical
            segment_rect.x = rect.x;
            segment_rect.y = rect.y + rect.height * t;
            segment_rect.width = rect.width;
            segment_rect.height = rect.height / segments;
        }
        
        // Draw gradient segment
        draw_gradient_rect(segment_rect, color, next_color, std::abs(dx) > std::abs(dy));
    }
}

void CanvasRenderer::draw_radial_gradient(const Rect2D& rect, const Point2D& center,
                                         float radius_x, float radius_y,
                                         const std::vector<std::pair<ColorRGBA, float>>& stops) {
    if (stops.size() < 2) return;
    
    // Approximate with concentric ellipses
    const int rings = 20;
    
    for (int i = rings - 1; i >= 0; i--) {
        float t = static_cast<float>(i) / rings;
        ColorRGBA color = interpolate_gradient_color(stops, t);
        
        float current_rx = radius_x * t;
        float current_ry = radius_y * t;
        
        // Draw filled ellipse (approximated with circle for now)
        float avg_radius = (current_rx + current_ry) / 2;
        draw_circle(center, avg_radius, color, 32);
    }
}

void CanvasRenderer::draw_conic_gradient(const Rect2D& rect, const Point2D& center,
                                        float start_angle,
                                        const std::vector<std::pair<ColorRGBA, float>>& stops) {
    if (stops.size() < 2) return;
    
    // Approximate with triangular segments
    const int segments = 36;  // 10-degree segments
    
    for (int i = 0; i < segments; i++) {
        float t = static_cast<float>(i) / segments;
        float angle = start_angle + t * 360.0f;
        float next_angle = start_angle + (t + 1.0f / segments) * 360.0f;
        
        ColorRGBA color = interpolate_gradient_color(stops, t);
        
        // Draw triangular segment from center
        float rad1 = angle * (M_PI / 180.0f);
        float rad2 = next_angle * (M_PI / 180.0f);
        
        float radius = std::max(rect.width, rect.height);
        Point2D p1 = center;
        Point2D p2(center.x + radius * std::cos(rad1), center.y + radius * std::sin(rad1));
        Point2D p3(center.x + radius * std::cos(rad2), center.y + radius * std::sin(rad2));
        
        // Draw triangle (simplified - would need proper triangle rendering)
        draw_line(p1, p2, color, 1);
        draw_line(p2, p3, color, 1);
        draw_line(p3, p1, color, 1);
    }
}

// Helper function to interpolate gradient colors
ColorRGBA CanvasRenderer::interpolate_gradient_color(
    const std::vector<std::pair<ColorRGBA, float>>& stops, float position) {
    
    if (stops.empty()) return ColorRGBA(0, 0, 0, 0);
    if (stops.size() == 1) return stops[0].first;
    
    // Find the two stops to interpolate between
    size_t lower_idx = 0;
    size_t upper_idx = stops.size() - 1;
    
    for (size_t i = 0; i < stops.size() - 1; i++) {
        if (position >= stops[i].second && position <= stops[i + 1].second) {
            lower_idx = i;
            upper_idx = i + 1;
            break;
        }
    }
    
    // Interpolate between the two stops
    float range = stops[upper_idx].second - stops[lower_idx].second;
    float t = (range > 0) ? (position - stops[lower_idx].second) / range : 0;
    
    const ColorRGBA& c1 = stops[lower_idx].first;
    const ColorRGBA& c2 = stops[upper_idx].first;
    
    return ColorRGBA(
        c1.r + (c2.r - c1.r) * t,
        c1.g + (c2.g - c1.g) * t,
        c1.b + (c2.b - c1.b) * t,
        c1.a + (c2.a - c1.a) * t
    );
}

void CanvasRenderer::draw_gradient_rect(const Rect2D& rect, const ColorRGBA& top_color, 
                                       const ColorRGBA& bottom_color, bool horizontal) {
    RenderBatch batch;
    batch.layer = RenderLayer::UI_CONTENT;
    batch.texture_id = white_texture_;
    
    uint32_t base_index = 0;
    
    if (horizontal) {
        // Horizontal gradient (left to right)
        batch.vertices.emplace_back(rect.x, rect.y, 0, 0, 
                                   top_color.r, top_color.g, top_color.b, top_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x + rect.width, rect.y, 1, 0,
                                   bottom_color.r, bottom_color.g, bottom_color.b, bottom_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x + rect.width, rect.y + rect.height, 1, 1,
                                   bottom_color.r, bottom_color.g, bottom_color.b, bottom_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x, rect.y + rect.height, 0, 1,
                                   top_color.r, top_color.g, top_color.b, top_color.a * global_opacity_);
    } else {
        // Vertical gradient (top to bottom)
        batch.vertices.emplace_back(rect.x, rect.y, 0, 0,
                                   top_color.r, top_color.g, top_color.b, top_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x + rect.width, rect.y, 1, 0,
                                   top_color.r, top_color.g, top_color.b, top_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x + rect.width, rect.y + rect.height, 1, 1,
                                   bottom_color.r, bottom_color.g, bottom_color.b, bottom_color.a * global_opacity_);
        batch.vertices.emplace_back(rect.x, rect.y + rect.height, 0, 1,
                                   bottom_color.r, bottom_color.g, bottom_color.b, bottom_color.a * global_opacity_);
    }
    
    // Add indices for the quad
    batch.indices.push_back(base_index + 0);
    batch.indices.push_back(base_index + 1);
    batch.indices.push_back(base_index + 2);
    batch.indices.push_back(base_index + 0);
    batch.indices.push_back(base_index + 2);
    batch.indices.push_back(base_index + 3);
    
    submit_batch(batch);
}

// Shadow cache implementation
void CanvasRenderer::ShadowCache::create_shadow_texture(NinePatchTexture& texture, 
                                                       float blur_radius, 
                                                       float spread, 
                                                       const ColorRGBA& color) {
    // Calculate texture size - we only need to store one corner + edges
    int border = std::max(16, int(blur_radius * 2 + spread));
    int tex_size = border * 3;  // 3x3 grid of border size
    
    texture.texture_width = tex_size;
    texture.texture_height = tex_size;
    texture.border_size = border;
    
    // Create pixel data for shadow
    std::vector<uint8_t> pixels(tex_size * tex_size * 4, 0);
    
    // Generate shadow using gaussian blur approximation
    float sigma = blur_radius / 3.0f;
    float two_sigma_sq = 2.0f * sigma * sigma;
    
    for (int y = 0; y < tex_size; y++) {
        for (int x = 0; x < tex_size; x++) {
            // Calculate distance from edge
            float dist_x = 0, dist_y = 0;
            
            if (x < border) dist_x = border - x;
            else if (x >= tex_size - border) dist_x = x - (tex_size - border - 1);
            
            if (y < border) dist_y = border - y;
            else if (y >= tex_size - border) dist_y = y - (tex_size - border - 1);
            
            float dist = std::sqrt(dist_x * dist_x + dist_y * dist_y);
            
            // Apply gaussian falloff
            float alpha = 0;
            if (dist <= spread) {
                alpha = 1.0f;
            } else if (blur_radius > 0) {
                float blur_dist = dist - spread;
                alpha = std::exp(-(blur_dist * blur_dist) / two_sigma_sq);
            }
            
            // Set pixel
            int idx = (y * tex_size + x) * 4;
            pixels[idx + 0] = uint8_t(color.r * 255);
            pixels[idx + 1] = uint8_t(color.g * 255);
            pixels[idx + 2] = uint8_t(color.b * 255);
            pixels[idx + 3] = uint8_t(color.a * alpha * 255);
        }
    }
    
    // Create OpenGL texture
    glGenTextures(1, &texture.texture_id);
    glBindTexture(GL_TEXTURE_2D, texture.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_size, tex_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Calculate UV coordinates for 9-patch
    texture.left_u = float(border) / tex_size;
    texture.center_u = float(tex_size - border) / tex_size;
    texture.right_u = 1.0f;
    texture.top_v = float(border) / tex_size;
    texture.middle_v = float(tex_size - border) / tex_size;
    texture.bottom_v = 1.0f;
}

void CanvasRenderer::draw_shadow_cached(const Rect2D& rect, float blur_radius, float spread,
                                       float offset_x, float offset_y, const ColorRGBA& color) {
    // Get or create cached shadow texture
    auto* shadow = shadow_cache_.get_or_create_shadow(blur_radius, spread, color);
    if (!shadow || !shadow->texture_id) {
        // Fallback to old method if cache fails
        draw_shadow(rect, blur_radius, spread, offset_x, offset_y, color);
        return;
    }
    
    // Calculate shadow bounds
    Rect2D shadow_rect(
        rect.x + offset_x - blur_radius - spread,
        rect.y + offset_y - blur_radius - spread,
        rect.width + (blur_radius + spread) * 2,
        rect.height + (blur_radius + spread) * 2
    );
    
    // Draw 9-patch shadow
    float border = shadow->border_size;
    
    // Define the 9 patches
    struct Patch {
        Rect2D dest;
        float u1, v1, u2, v2;
    };
    
    Patch patches[9] = {
        // Top-left corner
        {{shadow_rect.x, shadow_rect.y, border, border},
         0, 0, shadow->left_u, shadow->top_v},
        
        // Top edge
        {{shadow_rect.x + border, shadow_rect.y, shadow_rect.width - border * 2, border},
         shadow->left_u, 0, shadow->center_u, shadow->top_v},
        
        // Top-right corner
        {{shadow_rect.x + shadow_rect.width - border, shadow_rect.y, border, border},
         shadow->center_u, 0, shadow->right_u, shadow->top_v},
        
        // Left edge
        {{shadow_rect.x, shadow_rect.y + border, border, shadow_rect.height - border * 2},
         0, shadow->top_v, shadow->left_u, shadow->middle_v},
        
        // Center (usually transparent for shadows)
        {{shadow_rect.x + border, shadow_rect.y + border, 
          shadow_rect.width - border * 2, shadow_rect.height - border * 2},
         shadow->left_u, shadow->top_v, shadow->center_u, shadow->middle_v},
        
        // Right edge
        {{shadow_rect.x + shadow_rect.width - border, shadow_rect.y + border, 
          border, shadow_rect.height - border * 2},
         shadow->center_u, shadow->top_v, shadow->right_u, shadow->middle_v},
        
        // Bottom-left corner
        {{shadow_rect.x, shadow_rect.y + shadow_rect.height - border, border, border},
         0, shadow->middle_v, shadow->left_u, shadow->bottom_v},
        
        // Bottom edge
        {{shadow_rect.x + border, shadow_rect.y + shadow_rect.height - border, 
          shadow_rect.width - border * 2, border},
         shadow->left_u, shadow->middle_v, shadow->center_u, shadow->bottom_v},
        
        // Bottom-right corner
        {{shadow_rect.x + shadow_rect.width - border, 
          shadow_rect.y + shadow_rect.height - border, border, border},
         shadow->center_u, shadow->middle_v, shadow->right_u, shadow->bottom_v}
    };
    
    // Check if we can batch with current batch
    GLuint shader_id = ui_shader_->get_id();
    GLenum blend_mode = GL_ONE; // TODO: Get actual blend mode
    
    if (!current_batch_.can_batch_with(shadow->texture_id, blend_mode, shader_id)) {
        flush_current_batch();
        current_batch_.texture_id = shadow->texture_id;
        current_batch_.blend_mode = blend_mode;
        current_batch_.shader_id = shader_id;
    }
    
    // Add all 9 patches to batch
    for (const auto& patch : patches) {
        uint32_t base_index = current_batch_.vertices.size();
        
        // Add vertices
        current_batch_.vertices.push_back(UIVertex(
            patch.dest.x, patch.dest.y, 
            patch.u1, patch.v1, 1, 1, 1, 1));
        current_batch_.vertices.push_back(UIVertex(
            patch.dest.x + patch.dest.width, patch.dest.y, 
            patch.u2, patch.v1, 1, 1, 1, 1));
        current_batch_.vertices.push_back(UIVertex(
            patch.dest.x + patch.dest.width, patch.dest.y + patch.dest.height, 
            patch.u2, patch.v2, 1, 1, 1, 1));
        current_batch_.vertices.push_back(UIVertex(
            patch.dest.x, patch.dest.y + patch.dest.height, 
            patch.u1, patch.v2, 1, 1, 1, 1));
        
        // Add indices
        current_batch_.indices.push_back(base_index + 0);
        current_batch_.indices.push_back(base_index + 1);
        current_batch_.indices.push_back(base_index + 2);
        current_batch_.indices.push_back(base_index + 0);
        current_batch_.indices.push_back(base_index + 2);
        current_batch_.indices.push_back(base_index + 3);
    }
    
    vertices_this_frame_ += 36;  // 9 patches * 4 vertices
}

void CanvasRenderer::draw_shadow(const Rect2D& rect, float blur_radius, float spread,
                                float offset_x, float offset_y, const ColorRGBA& color) {
    // Try cached version first
    draw_shadow_cached(rect, blur_radius, spread, offset_x, offset_y, color);
    return;
    
    // Original implementation as fallback (now unused)
    // Simplified shadow rendering using multiple alpha-blended rectangles
    if (blur_radius <= 0 && spread == 0) {
        // Simple offset shadow
        Rect2D shadow_rect(rect.x + offset_x, rect.y + offset_y, rect.width, rect.height);
        draw_rect(shadow_rect, color);
        return;
    }
    
    // Multi-pass blur approximation
    int passes = std::min(10, static_cast<int>(blur_radius / 2) + 1);
    float alpha_per_pass = color.a / passes;
    
    for (int i = passes - 1; i >= 0; i--) {
        float factor = static_cast<float>(i) / passes;
        float current_spread = spread + blur_radius * (1.0f - factor);
        
        Rect2D shadow_rect(
            rect.x + offset_x - current_spread,
            rect.y + offset_y - current_spread,
            rect.width + current_spread * 2,
            rect.height + current_spread * 2
        );
        
        ColorRGBA blur_color = color;
        blur_color.a = alpha_per_pass * (1.0f - factor * 0.5f);
        draw_rect(shadow_rect, blur_color);
    }
}

void CanvasRenderer::begin_blur_region(const Rect2D& region, float blur_radius) {
    // Ensure blur texture is large enough
    int width = static_cast<int>(region.width);
    int height = static_cast<int>(region.height);
    
    if (blur_texture_width_ < width || blur_texture_height_ < height) {
        // Recreate blur framebuffer and texture
        if (blur_fbo_) {
            glDeleteFramebuffers(1, &blur_fbo_);
            glDeleteTextures(1, &blur_texture_);
        }
        
        glGenFramebuffers(1, &blur_fbo_);
        glGenTextures(1, &blur_texture_);
        
        glBindTexture(GL_TEXTURE_2D, blur_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_texture_, 0);
        
        blur_texture_width_ = width;
        blur_texture_height_ = height;
    }
    
    // Start rendering to blur texture
    glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo_);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
}

void CanvasRenderer::end_blur_region() {
    // Return to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport_x_, viewport_y_, viewport_width_, viewport_height_);
}

void CanvasRenderer::apply_gaussian_blur(GLuint texture, const Rect2D& src_rect, 
                                        const Rect2D& dst_rect, float blur_radius) {
    if (!blur_shader_ || !blur_shader_->is_valid()) {
        // Fallback: just draw the texture without blur
        draw_texture(texture, dst_rect);
        return;
    }
    
    // Create temp texture for two-pass blur
    GLuint temp_texture;
    glGenTextures(1, &temp_texture);
    glBindTexture(GL_TEXTURE_2D, temp_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                static_cast<GLsizei>(src_rect.width), 
                static_cast<GLsizei>(src_rect.height), 
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create FBO for blur passes
    GLuint blur_fbo;
    glGenFramebuffers(1, &blur_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo);
    
    // Setup quad vertices
    float quad_vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    
    GLuint quad_vbo;
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    blur_shader_->use();
    
    // Pass 1: Horizontal blur
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_texture, 0);
    glViewport(0, 0, static_cast<GLsizei>(src_rect.width), static_cast<GLsizei>(src_rect.height));
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, texture);
    blur_shader_->set_uniform("u_texture", 0);
    blur_shader_->set_uniform("u_direction", Point2D(1.0f, 0.0f));
    blur_shader_->set_uniform("u_radius", blur_radius);
    blur_shader_->set_uniform("u_resolution", Point2D(src_rect.width, src_rect.height));
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Pass 2: Vertical blur back to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport_x_, viewport_y_, viewport_width_, viewport_height_);
    
    glBindTexture(GL_TEXTURE_2D, temp_texture);
    blur_shader_->set_uniform("u_direction", Point2D(0.0f, 1.0f));
    
    // Draw to destination rect
    float dst_vertices[] = {
        dst_rect.x, dst_rect.y, 0.0f, 0.0f,
        dst_rect.x + dst_rect.width, dst_rect.y, 1.0f, 0.0f,
        dst_rect.x + dst_rect.width, dst_rect.y + dst_rect.height, 1.0f, 1.0f,
        dst_rect.x, dst_rect.y + dst_rect.height, 0.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(dst_vertices), dst_vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    blur_shader_->unuse();
    
    // Cleanup
    glDeleteBuffers(1, &quad_vbo);
    glDeleteFramebuffers(1, &blur_fbo);
    glDeleteTextures(1, &temp_texture);
}

void CanvasRenderer::begin_backdrop_filter(const Rect2D& region) {
    // Store the region for later use
    backdrop_region_ = region;
    
    // Ensure backdrop texture is large enough
    int width = static_cast<int>(region.width);
    int height = static_cast<int>(region.height);
    
    if (backdrop_texture_width_ < width || backdrop_texture_height_ < height) {
        // Recreate backdrop framebuffer and texture
        if (backdrop_fbo_) {
            glDeleteFramebuffers(1, &backdrop_fbo_);
            glDeleteTextures(1, &backdrop_texture_);
        }
        
        glGenFramebuffers(1, &backdrop_fbo_);
        glGenTextures(1, &backdrop_texture_);
        
        glBindTexture(GL_TEXTURE_2D, backdrop_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        backdrop_texture_width_ = width;
        backdrop_texture_height_ = height;
    }
    
    // Copy current framebuffer content to backdrop texture
    glBindTexture(GL_TEXTURE_2D, backdrop_texture_);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                        static_cast<GLint>(region.x), 
                        static_cast<GLint>(viewport_height_ - region.y - region.height),
                        width, height);
}

void CanvasRenderer::end_backdrop_filter() {
    // Nothing to do here for now
}

void CanvasRenderer::apply_backdrop_blur(const Rect2D& region, float blur_radius, float opacity) {
    if (!backdrop_texture_) return;
    
    // Apply blur to the backdrop texture
    Rect2D src_rect(0, 0, region.width, region.height);
    
    // Create a temporary blurred texture
    GLuint blurred_texture;
    glGenTextures(1, &blurred_texture);
    glBindTexture(GL_TEXTURE_2D, blurred_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                static_cast<GLsizei>(region.width),
                static_cast<GLsizei>(region.height),
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Use the blur shader to blur the backdrop
    if (blur_shader_ && blur_shader_->is_valid()) {
        GLuint temp_fbo;
        glGenFramebuffers(1, &temp_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurred_texture, 0);
        
        // Apply gaussian blur
        apply_gaussian_blur(backdrop_texture_, src_rect, src_rect, blur_radius);
        
        glDeleteFramebuffers(1, &temp_fbo);
    }
    
    // Draw the blurred backdrop with specified opacity
    ColorRGBA tint(1, 1, 1, opacity);
    draw_texture(blurred_texture, region, tint);
    
    // Cleanup
    glDeleteTextures(1, &blurred_texture);
}

void CanvasRenderer::apply_backdrop_saturate(const Rect2D& region, float saturation) {
    if (!backdrop_texture_) return;
    
    // TODO: Implement saturation shader
    // For now, just draw the backdrop as-is
    draw_texture(backdrop_texture_, region);
}

void CanvasRenderer::apply_backdrop_brightness(const Rect2D& region, float brightness) {
    if (!backdrop_texture_) return;
    
    // Apply brightness by modulating the tint color
    ColorRGBA tint(brightness, brightness, brightness, 1.0f);
    draw_texture(backdrop_texture_, region, tint);
}

void CanvasRenderer::draw_occlusion_debug() {
    const auto& occluded_regions = occlusion_tracker_.get_occluded_regions();
    
    // Draw occluded regions with semi-transparent overlay
    for (const auto& region : occluded_regions) {
        // Draw filled region with transparency
        ColorRGBA fill_color(1.0f, 0.0f, 0.0f, 0.2f);  // Red with 20% opacity
        draw_rect(region, fill_color);
        
        // Draw border
        ColorRGBA border_color(1.0f, 0.0f, 0.0f, 0.8f);  // Red with 80% opacity
        draw_rect_outline(region, border_color, 2.0f);
    }
    
    // Draw stats text
    if (font_system_) {
        std::string stats = "Occlusion Culling: " + 
                          std::to_string(occlusion_tracker_.get_culled_count()) + " culled / " +
                          std::to_string(occlusion_tracker_.get_tested_count()) + " tested";
        
        Point2D text_pos(10, viewport_height_ - 30);
        ColorRGBA text_color(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
        font_system_->render_text(this, stats, text_pos, "default", 14, text_color);
    }
}

void CanvasRenderer::draw_dashed_line(const Point2D& start, const Point2D& end, 
                                     const ColorRGBA& color, float width,
                                     float dash_length, float gap_length) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length == 0) return;
    
    float pattern_length = dash_length + gap_length;
    int num_patterns = static_cast<int>(length / pattern_length);
    
    dx /= length;
    dy /= length;
    
    for (int i = 0; i <= num_patterns; i++) {
        float dash_start = i * pattern_length;
        float dash_end = std::min(dash_start + dash_length, length);
        
        Point2D segment_start(start.x + dx * dash_start, start.y + dy * dash_start);
        Point2D segment_end(start.x + dx * dash_end, start.y + dy * dash_end);
        
        draw_line(segment_start, segment_end, color, width);
    }
}

void CanvasRenderer::draw_dotted_line(const Point2D& start, const Point2D& end,
                                     const ColorRGBA& color, float width,
                                     float dot_spacing) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length == 0) return;
    
    int num_dots = static_cast<int>(length / dot_spacing) + 1;
    dx /= length;
    dy /= length;
    
    for (int i = 0; i < num_dots; i++) {
        float t = i * dot_spacing;
        Point2D dot_center(start.x + dx * t, start.y + dy * t);
        draw_circle(dot_center, width / 2, color, 8);
    }
}

// Text rendering with ellipsis
void CanvasRenderer::draw_text_with_ellipsis(const std::string& text, const Rect2D& bounds,
                                            const ColorRGBA& color, float size,
                                            TextAlign align) {
    if (!font_system_) return;
    
    // Measure text width
    float text_width = font_system_->measure_text(text, "default", static_cast<int>(size)).x;
    
    if (text_width <= bounds.width) {
        // Text fits, draw normally
        Point2D pos(bounds.x, bounds.y);
        if (align == TextAlign::CENTER) {
            pos.x = bounds.x + (bounds.width - text_width) / 2;
        } else if (align == TextAlign::RIGHT) {
            pos.x = bounds.x + bounds.width - text_width;
        }
        draw_text(text, pos, color, size, align, TextBaseline::TOP);
        return;
    }
    
    // Text doesn't fit, add ellipsis
    std::string ellipsis = "...";
    float ellipsis_width = font_system_->measure_text(ellipsis, "default", static_cast<int>(size)).x;
    float available_width = bounds.width - ellipsis_width;
    
    // Binary search for the longest substring that fits
    size_t left = 0, right = text.length();
    size_t best_fit = 0;
    
    while (left <= right) {
        size_t mid = (left + right) / 2;
        std::string substr = text.substr(0, mid);
        float substr_width = font_system_->measure_text(substr, "default", static_cast<int>(size)).x;
        
        if (substr_width <= available_width) {
            best_fit = mid;
            left = mid + 1;
        } else {
            if (mid == 0) break;
            right = mid - 1;
        }
    }
    
    std::string truncated = text.substr(0, best_fit) + ellipsis;
    Point2D pos(bounds.x, bounds.y);
    
    if (align == TextAlign::CENTER) {
        float truncated_width = font_system_->measure_text(truncated, "default", static_cast<int>(size)).x;
        pos.x = bounds.x + (bounds.width - truncated_width) / 2;
    } else if (align == TextAlign::RIGHT) {
        float truncated_width = font_system_->measure_text(truncated, "default", static_cast<int>(size)).x;
        pos.x = bounds.x + bounds.width - truncated_width;
    }
    
    draw_text(truncated, pos, color, size, align, TextBaseline::TOP);
}

void CanvasRenderer::draw_text_with_shadow(const std::string& text, const Point2D& position,
                                          const ColorRGBA& color, const ColorRGBA& shadow_color,
                                          float size, float shadow_offset_x,
                                          float shadow_offset_y, float shadow_blur) {
    // Draw shadow first
    if (shadow_blur > 0) {
        // Multi-pass blur for shadow
        int passes = std::min(3, static_cast<int>(shadow_blur) + 1);
        for (int i = 0; i < passes; i++) {
            float offset_factor = static_cast<float>(i) / passes;
            Point2D shadow_pos(
                position.x + shadow_offset_x * (1.0f - offset_factor * 0.5f),
                position.y + shadow_offset_y * (1.0f - offset_factor * 0.5f)
            );
            ColorRGBA blur_color = shadow_color;
            blur_color.a *= (1.0f / passes) * (1.0f - offset_factor * 0.3f);
            draw_text(text, shadow_pos, blur_color, size);
        }
    } else {
        // Simple shadow
        Point2D shadow_pos(position.x + shadow_offset_x, position.y + shadow_offset_y);
        draw_text(text, shadow_pos, shadow_color, size);
    }
    
    // Draw main text
    draw_text(text, position, color, size);
}

// Image rendering with object-fit
void CanvasRenderer::draw_image(GLuint texture_id, const Rect2D& bounds, ObjectFit fit,
                               const ColorRGBA& tint) {
    // Get texture dimensions (simplified - in real implementation, store texture dimensions)
    int tex_width, tex_height;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_height);
    
    Rect2D draw_rect = bounds;
    
    switch (fit) {
        case ObjectFit::Fill:
            // Default - stretch to fill
            break;
            
        case ObjectFit::Contain: {
            // Scale to fit within bounds while maintaining aspect ratio
            float scale_x = bounds.width / tex_width;
            float scale_y = bounds.height / tex_height;
            float scale = std::min(scale_x, scale_y);
            
            draw_rect.width = tex_width * scale;
            draw_rect.height = tex_height * scale;
            draw_rect.x = bounds.x + (bounds.width - draw_rect.width) / 2;
            draw_rect.y = bounds.y + (bounds.height - draw_rect.height) / 2;
            break;
        }
        
        case ObjectFit::Cover: {
            // Scale to cover bounds while maintaining aspect ratio
            float scale_x = bounds.width / tex_width;
            float scale_y = bounds.height / tex_height;
            float scale = std::max(scale_x, scale_y);
            
            draw_rect.width = tex_width * scale;
            draw_rect.height = tex_height * scale;
            draw_rect.x = bounds.x + (bounds.width - draw_rect.width) / 2;
            draw_rect.y = bounds.y + (bounds.height - draw_rect.height) / 2;
            break;
        }
        
        case ObjectFit::None:
            // No scaling, center in bounds
            draw_rect.width = static_cast<float>(tex_width);
            draw_rect.height = static_cast<float>(tex_height);
            draw_rect.x = bounds.x + (bounds.width - draw_rect.width) / 2;
            draw_rect.y = bounds.y + (bounds.height - draw_rect.height) / 2;
            break;
            
        case ObjectFit::ScaleDown: {
            // Scale down if larger than bounds, otherwise no scaling
            if (tex_width > bounds.width || tex_height > bounds.height) {
                float scale_x = bounds.width / tex_width;
                float scale_y = bounds.height / tex_height;
                float scale = std::min(scale_x, scale_y);
                
                draw_rect.width = tex_width * scale;
                draw_rect.height = tex_height * scale;
            } else {
                draw_rect.width = static_cast<float>(tex_width);
                draw_rect.height = static_cast<float>(tex_height);
            }
            draw_rect.x = bounds.x + (bounds.width - draw_rect.width) / 2;
            draw_rect.y = bounds.y + (bounds.height - draw_rect.height) / 2;
            break;
        }
    }
    
    draw_texture(texture_id, draw_rect, tint);
}

// Matrix helper functions
void CanvasRenderer::identity_matrix(float* matrix) {
    std::fill(matrix, matrix + 16, 0.0f);
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void CanvasRenderer::multiply_matrix(float* result, const float* a, const float* b) {
    float temp[16];
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            temp[row * 4 + col] = 0;
            for (int k = 0; k < 4; k++) {
                temp[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
            }
        }
    }
    
    std::copy(temp, temp + 16, result);
}

void CanvasRenderer::translate_matrix(float* matrix, float x, float y) {
    matrix[12] = x;
    matrix[13] = y;
}

void CanvasRenderer::rotate_matrix(float* matrix, float degrees) {
    float radians = degrees * (M_PI / 180.0f);
    float c = std::cos(radians);
    float s = std::sin(radians);
    
    matrix[0] = c;
    matrix[1] = s;
    matrix[4] = -s;
    matrix[5] = c;
}

void CanvasRenderer::scale_matrix(float* matrix, float x, float y) {
    matrix[0] = x;
    matrix[5] = y;
}

void CanvasRenderer::update_projection_matrix() {
    // Update the projection matrix with current transform
    // This would update shader uniforms
    if (ui_shader_) {
        ui_shader_->use();
        ui_shader_->set_uniform("u_transform", current_transform_.matrix, 16);
        ui_shader_->unuse();
    }
}

void CanvasRenderer::draw_button(const Rect2D& rect, const ColorRGBA& color, bool pressed, bool hovered) {
    ColorRGBA button_color = color;
    
    if (pressed) {
        button_color = ColorRGBA(color.r * 0.8f, color.g * 0.8f, color.b * 0.8f, color.a);
    } else if (hovered) {
        button_color = ColorRGBA(color.r * 1.1f, color.g * 1.1f, color.b * 1.1f, color.a);
    }
    
    float corner_radius = 4.0f;
    float border_width = pressed ? 0.0f : 1.0f;
    ColorRGBA border_color = pressed ? ColorRGBA::transparent() : ColorRGBA(0.6f, 0.6f, 0.6f, 1.0f);
    
    draw_widget(rect, button_color, corner_radius, border_width, border_color);
}

void CanvasRenderer::flush_current_batch() {
    if (current_batch_.vertices.empty()) {
        return;  // Nothing to flush
    }
    
    // Add to completed batches for sorting
    completed_batches_.emplace_back(current_batch_);
    
    // Reset current batch
    current_batch_.reset();
}

void CanvasRenderer::render_sorted_batches() {
    if (completed_batches_.empty()) {
        return;
    }
    
    // Sort batches by state to minimize state changes
    std::sort(completed_batches_.begin(), completed_batches_.end(),
              [](const CompletedBatch& a, const CompletedBatch& b) {
                  return a.sort_key < b.sort_key;
              });
    
    // Track current state to avoid redundant state changes
    GLuint current_texture = 0;
    GLuint current_shader = 0;
    GLenum current_blend = 0;
    
    // Set up common state once
    glBindVertexArray(ui_vao_);
    
    // Render each batch
    for (const auto& batch : completed_batches_) {
        // Only change texture if different
        if (batch.texture_id != current_texture) {
            glActiveTexture(GL_TEXTURE0);  // Make sure we're using texture unit 0
            glBindTexture(GL_TEXTURE_2D, batch.texture_id);
            current_texture = batch.texture_id;
        }
        
        // Only change shader if different
        if (batch.shader_id != current_shader) {
            // TODO: Support multiple shaders
            ui_shader_->use();
            
            // Set projection matrix for new shader
            float vp_width = std::max(1.0f, (float)viewport_width_);
            float vp_height = std::max(1.0f, (float)viewport_height_);
            
            float projection[16] = {
                2.0f / vp_width,  0,                  0, 0,
                0,               -2.0f / vp_height,    0, 0,
                0,                0,                  -1, 0,
                -1,               1,                   0, 1
            };
            ui_shader_->set_uniform("u_projection", projection, 16);
            
            // Set default uniforms
            // CRITICAL: Set u_has_texture to 1 for textured rendering (rectangles use white texture)
            ui_shader_->set_uniform("u_has_texture", 1);  // Enable textured path
            ui_shader_->set_uniform("u_texture", 0);  // Use texture unit 0
            
            // These are still needed even though we're using textured path
            ui_shader_->set_uniform("u_widget_rect", ColorRGBA(0, 0, 10000, 10000));  // Large rect disables SDF clipping
            ui_shader_->set_uniform("u_corner_radius", ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f)); 
            ui_shader_->set_uniform("u_border_width", 0.0f);
            ui_shader_->set_uniform("u_border_color", ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f));
            ui_shader_->set_uniform("u_aa_radius", 1.0f);
            ui_shader_->set_uniform("u_global_opacity", global_opacity_);
            
            current_shader = batch.shader_id;
        }
        
        // Only change blend mode if different
        if (batch.blend_mode != current_blend) {
            // TODO: Set blend mode based on batch.blend_mode
            current_blend = batch.blend_mode;
        }
        
        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
        glBufferData(GL_ARRAY_BUFFER, 
                     batch.vertices.size() * sizeof(UIVertex), 
                     batch.vertices.data(), 
                     GL_STREAM_DRAW);
        
        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                     batch.indices.size() * sizeof(uint32_t), 
                     batch.indices.data(), 
                     GL_STREAM_DRAW);
        
        // Draw!
        glDrawElements(GL_TRIANGLES, batch.indices.size(), GL_UNSIGNED_INT, nullptr);
        
        // Update statistics
        draw_calls_this_frame_++;
    }
    
    // Clear completed batches
    completed_batches_.clear();
}

void CanvasRenderer::submit_batch(const RenderBatch& batch) {
    render_batches_.push_back(batch);
    
    // Reduce debug spam
}

void CanvasRenderer::flush_batches() {
    std::cout << "Flushing " << render_batches_.size() << " batches" << std::endl;
    
    if (render_batches_.empty()) {
        std::cout << "No batches to flush!" << std::endl;
        return;
    }
    
    // Sort batches by layer
    sort_batches_by_layer();
    
    std::cout << "Sorted batches, rendering " << batch_render_order_.size() << " in order" << std::endl;
    
    // Render batches in order
    for (size_t batch_idx : batch_render_order_) {
        const auto& batch = render_batches_[batch_idx];
        
        if (batch.vertices.empty() || batch.indices.empty()) {
            continue;
        }
        
        // Bind texture
        bind_texture(batch.texture_id);
        
        // Upload vertex data
        glBindVertexArray(ui_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
        glBufferData(GL_ARRAY_BUFFER, 
                     batch.vertices.size() * sizeof(UIVertex), 
                     batch.vertices.data(), GL_STREAM_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                     batch.indices.size() * sizeof(uint32_t), 
                     batch.indices.data(), GL_STREAM_DRAW);
        
        // Draw
        if (ui_shader_) {
            ui_shader_->use();
            // Reduced debug spam
            glDrawElements(batch.primitive_type, 
                          static_cast<GLsizei>(batch.indices.size()), 
                          GL_UNSIGNED_INT, nullptr);
            ui_shader_->unuse();
        } else {
            std::cout << "ERROR: No UI shader available!" << std::endl;
        }
        
        draw_calls_this_frame_++;
        vertices_this_frame_ += static_cast<int>(batch.vertices.size());
    }
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CanvasRenderer::set_theme(const CanvasTheme& theme) {
    theme_ = theme;
}

void CanvasRenderer::enable_scissor(const Rect2D& rect) {
    scissor_enabled_ = true;
    scissor_rect_ = rect;
    
    glEnable(GL_SCISSOR_TEST);
    glScissor(static_cast<GLint>(rect.x), 
              static_cast<GLint>(viewport_height_ - rect.y - rect.height),
              static_cast<GLsizei>(rect.width), 
              static_cast<GLsizei>(rect.height));
}

void CanvasRenderer::disable_scissor() {
    scissor_enabled_ = false;
    glDisable(GL_SCISSOR_TEST);
}

void CanvasRenderer::bind_texture(GLuint texture_id, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

void CanvasRenderer::draw_texture(GLuint texture_id, const Rect2D& rect, const ColorRGBA& tint) {
    // Save current state
    GLboolean blend_enabled;
    glGetBooleanv(GL_BLEND, &blend_enabled);
    
    // Enable blending for texture rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use the UI shader
    ui_shader_->use();
    
    // Set projection matrix - standard orthographic projection
    // The viewport dimensions already account for the physical pixel size
    float projection[16] = {
        2.0f / viewport_width_,  0,                        0, 0,
        0,                      -2.0f / viewport_height_,  0, 0,
        0,                       0,                       -1, 0,
        -1,                      1,                        0, 1
    };
    ui_shader_->set_uniform("u_projection", projection, 16);
    ui_shader_->set_uniform("u_has_texture", 1);
    
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    ui_shader_->set_uniform("u_texture", 0);
    
    // Setup vertex data with texture coordinates
    struct TexturedVertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };
    
    TexturedVertex vertices[4] = {
        // Bottom-left
        {rect.x, rect.y + rect.height, 
         0.0f, 1.0f,
         tint.r, tint.g, tint.b, tint.a},
        // Bottom-right
        {rect.x + rect.width, rect.y + rect.height,
         1.0f, 1.0f,
         tint.r, tint.g, tint.b, tint.a},
        // Top-right
        {rect.x + rect.width, rect.y,
         1.0f, 0.0f,
         tint.r, tint.g, tint.b, tint.a},
        // Top-left
        {rect.x, rect.y,
         0.0f, 0.0f,
         tint.r, tint.g, tint.b, tint.a}
    };
    
    unsigned int indices[6] = {
        0, 1, 2,
        2, 3, 0
    };
    
    // Upload vertex data
    glBindVertexArray(ui_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    // Setup vertex attributes
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Upload indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    
    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableVertexAttribArray(1); // Disable texture coord attribute
    ui_shader_->set_uniform("u_has_texture", 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    
    // Restore blend state
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }
}

void CanvasRenderer::draw_sdf_texture(GLuint sdf_texture_id, const Rect2D& rect, const ColorRGBA& tint,
                                      float threshold, float smoothness) {
    // Check if SDF shader is available
    if (sdf_shader_program_ == 0) {
        // Fallback to regular texture rendering if SDF shader not available
        draw_texture(sdf_texture_id, rect, tint);
        return;
    }
    
    // Save current state
    GLboolean blend_enabled;
    glGetBooleanv(GL_BLEND, &blend_enabled);
    
    // Enable blending for SDF rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use the SDF shader
    glUseProgram(sdf_shader_program_);
    
    // Set projection matrix
    float projection[16] = {
        2.0f / viewport_width_,  0,                        0, 0,
        0,                      -2.0f / viewport_height_,  0, 0,
        0,                       0,                       -1, 0,
        -1,                      1,                        0, 1
    };
    glUniformMatrix4fv(sdf_u_projection_, 1, GL_FALSE, projection);
    
    // Set SDF parameters
    glUniform4f(sdf_u_color_, tint.r, tint.g, tint.b, tint.a);
    glUniform1f(sdf_u_threshold_, threshold);
    
    // Dynamic smoothness based on physical pixel size
    // Use pixelsize factor for proper DPI-aware scaling
    float logical_size = std::min(rect.width, rect.height);
    float pixelsize = window_ ? window_->get_pixelsize() : get_content_scale();
    float physical_size = logical_size * pixelsize;  // Use DPI-adjusted physical pixels
    float adjusted_smoothness;
    
    if (physical_size <= 32.0f) {
        // Very small icons (16 logical @ 2x = 32 physical): minimal smoothing
        adjusted_smoothness = 0.001f;
    } else if (physical_size <= 48.0f) {
        // Small icons (24 logical @ 2x = 48 physical): very light smoothing
        adjusted_smoothness = 0.002f;
    } else if (physical_size <= 64.0f) {
        // Medium icons (32 logical @ 2x = 64 physical): light smoothing
        adjusted_smoothness = 0.005f;
    } else if (physical_size <= 96.0f) {
        // Large icons (48 logical @ 2x = 96 physical): moderate smoothing
        adjusted_smoothness = 0.01f;
    } else {
        // Very large icons: full smoothing for best anti-aliasing
        adjusted_smoothness = 0.02f;
    }
    
    glUniform1f(sdf_u_smoothness_, adjusted_smoothness);
    
    // Bind the SDF texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sdf_texture_id);
    glUniform1i(sdf_u_texture_, 0);
    
    // Debug: Verify texture is bound
    GLint bound_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
    if (bound_texture != sdf_texture_id) {
        std::cerr << "ERROR: SDF texture not bound! Expected: " << sdf_texture_id 
                  << ", Got: " << bound_texture << std::endl;
    }
    
    // Setup vertex data
    struct TexturedVertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };
    
    TexturedVertex vertices[4] = {
        // Bottom-left
        {rect.x, rect.y + rect.height, 
         0.0f, 1.0f,
         1.0f, 1.0f, 1.0f, 1.0f},
        // Bottom-right
        {rect.x + rect.width, rect.y + rect.height,
         1.0f, 1.0f,
         1.0f, 1.0f, 1.0f, 1.0f},
        // Top-right
        {rect.x + rect.width, rect.y,
         1.0f, 0.0f,
         1.0f, 1.0f, 1.0f, 1.0f},
        // Top-left
        {rect.x, rect.y,
         0.0f, 0.0f,
         1.0f, 1.0f, 1.0f, 1.0f}
    };
    
    GLuint indices[6] = {0, 1, 2, 2, 3, 0};
    
    // Use VAO for rendering
    glBindVertexArray(ui_vao_);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    // Setup vertex attributes for SDF shader
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    
    // Texture coord attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)(2 * sizeof(float)));
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    
    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    
    // Restore blend state
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }
}

void CanvasRenderer::setup_shaders() {
    create_ui_shader();
    create_text_shader();
    create_sdf_shader();
    create_blur_shader();
    create_gradient_shader();
}

void CanvasRenderer::setup_buffers() {
    
    // Create VAO
    glGenVertexArrays(1, &ui_vao_);
    glBindVertexArray(ui_vao_);
    check_gl_error("create and bind VAO");
    
    // Create VBO
    glGenBuffers(1, &ui_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    check_gl_error("create and bind VBO");
    
    // Create EBO
    glGenBuffers(1, &ui_ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    check_gl_error("create and bind EBO");
    
    // Setup vertex attributes with detailed logging
    
    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)0);
    check_gl_error("setup position attribute");
    
    // Texture coordinates (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)(2 * sizeof(float)));
    check_gl_error("setup texture coordinate attribute");
    
    // Color (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)(4 * sizeof(float)));
    check_gl_error("setup color attribute");
    
    
    glBindVertexArray(0);
}

void CanvasRenderer::setup_default_textures() {
    // Create white texture
    GLubyte white_pixel[4] = {255, 255, 255, 255};
    glGenTextures(1, &white_texture_);
    std::cout << "Created white_texture_ with ID: " << white_texture_ << std::endl;
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Create checker texture for debugging
    GLubyte checker_data[64]; // 4x4 RGBA
    for (int i = 0; i < 16; i++) {
        GLubyte color = ((i / 4) + (i % 4)) % 2 ? 255 : 128;
        checker_data[i * 4 + 0] = color;
        checker_data[i * 4 + 1] = color;
        checker_data[i * 4 + 2] = color;
        checker_data[i * 4 + 3] = 255;
    }
    
    glGenTextures(1, &checker_texture_);
    glBindTexture(GL_TEXTURE_2D, checker_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CanvasRenderer::create_ui_shader() {
    std::string vertex_source = R"(
#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec4 a_color;

uniform mat4 u_projection;

out vec2 v_texcoord;
out vec4 v_color;
out vec2 v_position;

void main() {
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
    v_texcoord = a_texcoord;
    v_color = a_color;
    v_position = a_position;
}
)";

    std::string fragment_source = R"(
#version 330 core

in vec2 v_texcoord;
in vec4 v_color;
in vec2 v_position;

uniform sampler2D u_texture;
uniform int u_has_texture;
uniform vec4 u_widget_rect;
uniform vec4 u_corner_radius;
uniform float u_border_width;
uniform vec4 u_border_color;
uniform float u_aa_radius;

out vec4 fragColor;

float sdf_rounded_rect(vec2 p, vec2 b, vec4 r) {
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x  = (p.y > 0.0) ? r.x  : r.y;
    vec2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

void main() {
    // Simple path for textured rendering
    if (u_has_texture == 1) {
        vec4 tex_color = texture(u_texture, v_texcoord);
        fragColor = tex_color * v_color;  // Tint the texture with vertex color
        return;
    }
    
    // Original SDF path for UI widgets
    vec2 widget_size = u_widget_rect.zw;
    vec2 widget_center = widget_size * 0.5;
    vec2 local_pos = v_position - u_widget_rect.xy;
    vec2 sdf_pos = local_pos - widget_center;
    
    float sdf = sdf_rounded_rect(sdf_pos, widget_size * 0.5, u_corner_radius);
    
    float inner_sdf = sdf + u_border_width;
    float outer_sdf = sdf;
    
    float inner_alpha = 1.0 - smoothstep(-u_aa_radius, u_aa_radius, inner_sdf);
    float outer_alpha = 1.0 - smoothstep(-u_aa_radius, u_aa_radius, outer_sdf);
    float border_alpha = outer_alpha - inner_alpha;
    
    vec4 fill_color = v_color;
    vec4 border_color = u_border_color;
    
    vec4 final_color = mix(fill_color, border_color, border_alpha);
    final_color.a *= outer_alpha;
    
    if (final_color.a < 0.01) {
        discard;
    }
    
    fragColor = final_color;
}
)";

    ui_shader_ = std::make_unique<ShaderProgram>();
    if (!ui_shader_->load_from_strings(vertex_source, fragment_source)) {
        std::cerr << "CRITICAL ERROR: Failed to create UI shader!" << std::endl;
    } else {
        }
}

void CanvasRenderer::create_text_shader() {
    // Text shader similar to UI shader for now
    text_shader_ = std::make_unique<ShaderProgram>();
    // TODO: Implement proper text rendering shader
}

void CanvasRenderer::create_gradient_shader() {
    const char* vertex_source = R"(
        #version 330 core
        
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texcoord;
        
        uniform mat4 u_projection;
        
        out vec2 v_position;
        out vec2 v_texcoord;
        
        void main() {
            gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
            v_position = a_position;
            v_texcoord = a_texcoord;
        }
    )";
    
    const char* fragment_source = R"(
        #version 330 core
        
        in vec2 v_position;
        in vec2 v_texcoord;
        
        out vec4 fragColor;
        
        // Gradient uniforms
        uniform int u_gradient_type;  // 0 = linear, 1 = radial, 2 = conic
        uniform vec2 u_gradient_start;
        uniform vec2 u_gradient_end;
        uniform vec2 u_gradient_center;
        uniform vec2 u_gradient_radius;
        uniform float u_gradient_angle;
        
        // Color stops (up to 8)
        uniform int u_stop_count;
        uniform vec4 u_stop_colors[8];
        uniform float u_stop_positions[8];
        
        vec4 interpolate_gradient(float t) {
            if (t <= u_stop_positions[0]) return u_stop_colors[0];
            if (t >= u_stop_positions[u_stop_count - 1]) return u_stop_colors[u_stop_count - 1];
            
            for (int i = 0; i < u_stop_count - 1; i++) {
                if (t >= u_stop_positions[i] && t <= u_stop_positions[i + 1]) {
                    float local_t = (t - u_stop_positions[i]) / 
                                  (u_stop_positions[i + 1] - u_stop_positions[i]);
                    return mix(u_stop_colors[i], u_stop_colors[i + 1], local_t);
                }
            }
            
            return u_stop_colors[u_stop_count - 1];
        }
        
        void main() {
            float t = 0.0;
            
            if (u_gradient_type == 0) {  // Linear gradient
                vec2 grad_vec = u_gradient_end - u_gradient_start;
                float grad_len = length(grad_vec);
                if (grad_len > 0.0) {
                    vec2 grad_dir = grad_vec / grad_len;
                    vec2 pos_vec = v_position - u_gradient_start;
                    t = dot(pos_vec, grad_dir) / grad_len;
                    t = clamp(t, 0.0, 1.0);
                }
            }
            else if (u_gradient_type == 1) {  // Radial gradient
                vec2 pos_vec = v_position - u_gradient_center;
                float dist = length(pos_vec / u_gradient_radius);
                t = clamp(dist, 0.0, 1.0);
            }
            else if (u_gradient_type == 2) {  // Conic gradient
                vec2 pos_vec = v_position - u_gradient_center;
                float angle = atan(pos_vec.y, pos_vec.x);
                angle = angle - u_gradient_angle;
                if (angle < 0.0) angle += 6.28318530718;  // 2*PI
                t = angle / 6.28318530718;
            }
            
            fragColor = interpolate_gradient(t);
        }
    )";
    
    gradient_shader_ = std::make_unique<ShaderProgram>();
    if (!gradient_shader_->load_from_strings(vertex_source, fragment_source)) {
        std::cerr << "Failed to create gradient shader" << std::endl;
        gradient_shader_.reset();
    }
}

void CanvasRenderer::create_blur_shader() {
    const char* vertex_source = R"(
        #version 330 core
        
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texcoord;
        
        out vec2 v_texcoord;
        
        void main() {
            gl_Position = vec4(a_position * 2.0 - 1.0, 0.0, 1.0);
            v_texcoord = a_texcoord;
        }
    )";
    
    const char* fragment_source = R"(
        #version 330 core
        
        in vec2 v_texcoord;
        out vec4 fragColor;
        
        uniform sampler2D u_texture;
        uniform vec2 u_direction;  // (1,0) for horizontal, (0,1) for vertical
        uniform float u_radius;
        uniform vec2 u_resolution;
        
        // Gaussian weights for up to 15 samples
        const float weights[15] = float[](
            0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216,
            0.003240, 0.000540, 0.000081, 0.000012, 0.000002,
            0.0000003, 0.00000004, 0.000000005, 0.0000000006, 0.00000000007
        );
        
        void main() {
            vec2 tex_offset = 1.0 / u_resolution;
            vec3 result = texture(u_texture, v_texcoord).rgb * weights[0];
            
            // Two-pass separable filter
            int sample_count = min(int(u_radius * 2.0), 14);
            
            for(int i = 1; i <= sample_count; ++i) {
                vec2 offset = u_direction * tex_offset * float(i);
                result += texture(u_texture, v_texcoord + offset).rgb * weights[i];
                result += texture(u_texture, v_texcoord - offset).rgb * weights[i];
            }
            
            fragColor = vec4(result, 1.0);
        }
    )";
    
    blur_shader_ = std::make_unique<ShaderProgram>();
    if (!blur_shader_->load_from_strings(vertex_source, fragment_source)) {
        std::cerr << "Failed to create blur shader" << std::endl;
        blur_shader_.reset();
    }
}

void CanvasRenderer::create_sdf_shader() {
    // SDF vertex shader - same as UI shader but simpler
    const char* vertex_source = R"(
        #version 330 core
        
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texcoord;
        layout(location = 2) in vec4 a_color;
        
        uniform mat4 u_projection;
        
        out vec2 v_texcoord;
        out vec4 v_color;
        
        void main() {
            gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
            v_texcoord = a_texcoord;
            v_color = a_color;
        }
    )";
    
    // SDF fragment shader with distance field rendering
    const char* fragment_source = R"(
        #version 330 core
        
        in vec2 v_texcoord;
        in vec4 v_color;
        
        out vec4 fragColor;
        
        uniform sampler2D u_texture;
        uniform vec4 u_color;
        uniform float u_threshold;
        uniform float u_smoothness;
        
        void main() {
            // Sample the distance field
            float distance = texture(u_texture, v_texcoord).r;
            
            // Calculate alpha using smoothstep for anti-aliasing
            // Values > threshold are inside the shape
            float alpha = smoothstep(u_threshold - u_smoothness, 
                                    u_threshold + u_smoothness, 
                                    distance);
            
            // Apply color and alpha
            fragColor = vec4(u_color.rgb * v_color.rgb, u_color.a * v_color.a * alpha);
            
            // Discard fully transparent pixels for better performance
            if (fragColor.a < 0.01) {
                discard;
            }
        }
    )";
    
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
    glCompileShader(vertex_shader);
    
    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        std::cerr << "SDF vertex shader compilation failed: " << info_log << std::endl;
        glDeleteShader(vertex_shader);
        return;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
    glCompileShader(fragment_shader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        std::cerr << "SDF fragment shader compilation failed: " << info_log << std::endl;
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return;
    }
    
    // Create and link program
    sdf_shader_program_ = glCreateProgram();
    glAttachShader(sdf_shader_program_, vertex_shader);
    glAttachShader(sdf_shader_program_, fragment_shader);
    glLinkProgram(sdf_shader_program_);
    
    // Check linking
    glGetProgramiv(sdf_shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(sdf_shader_program_, 512, nullptr, info_log);
        std::cerr << "SDF shader linking failed: " << info_log << std::endl;
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(sdf_shader_program_);
        sdf_shader_program_ = 0;
        return;
    }
    
    // Clean up shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    // Get uniform locations
    sdf_u_projection_ = glGetUniformLocation(sdf_shader_program_, "u_projection");
    sdf_u_texture_ = glGetUniformLocation(sdf_shader_program_, "u_texture");
    sdf_u_color_ = glGetUniformLocation(sdf_shader_program_, "u_color");
    sdf_u_threshold_ = glGetUniformLocation(sdf_shader_program_, "u_threshold");
    sdf_u_smoothness_ = glGetUniformLocation(sdf_shader_program_, "u_smoothness");
    
    std::cout << "SDF shader created successfully" << std::endl;
}

void CanvasRenderer::sort_batches_by_layer() {
    batch_render_order_.clear();
    batch_render_order_.reserve(render_batches_.size());
    
    for (size_t i = 0; i < render_batches_.size(); i++) {
        batch_render_order_.push_back(i);
    }
    
    std::sort(batch_render_order_.begin(), batch_render_order_.end(),
        [this](size_t a, size_t b) {
            return static_cast<int>(render_batches_[a].layer) < static_cast<int>(render_batches_[b].layer);
        });
}

// ShaderProgram implementation

ShaderProgram::ShaderProgram() = default;

ShaderProgram::~ShaderProgram() {
    if (program_id_) {
        glDeleteProgram(program_id_);
    }
    if (vertex_shader_) {
        glDeleteShader(vertex_shader_);
    }
    if (fragment_shader_) {
        glDeleteShader(fragment_shader_);
    }
}

bool ShaderProgram::load_from_strings(const std::string& vertex_source, const std::string& fragment_source) {
    vertex_shader_ = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (!vertex_shader_) {
        return false;
    }
    
    fragment_shader_ = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (!fragment_shader_) {
        return false;
    }
    
    return link_program();
}

void ShaderProgram::use() const {
    if (program_id_) {
        glUseProgram(program_id_);
    }
}

void ShaderProgram::unuse() const {
    glUseProgram(0);
}

void ShaderProgram::set_uniform(const std::string& name, float value) {
    GLint location = get_uniform_location(name);
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void ShaderProgram::set_uniform(const std::string& name, int value) {
    GLint location = get_uniform_location(name);
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const Point2D& value) {
    GLint location = get_uniform_location(name);
    if (location >= 0) {
        glUniform2f(location, value.x, value.y);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const ColorRGBA& value) {
    GLint location = get_uniform_location(name);
    if (location >= 0) {
        glUniform4f(location, value.r, value.g, value.b, value.a);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const float* matrix, int count) {
    GLint location = get_uniform_location(name);
    if (location >= 0) {
        glUniformMatrix4fv(location, count / 16, GL_FALSE, matrix);
    }
}

GLint ShaderProgram::get_uniform_location(const std::string& name) const {
    auto it = uniform_cache_.find(name);
    if (it != uniform_cache_.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    uniform_cache_[name] = location;
    return location;
}

GLuint ShaderProgram::compile_shader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* source_ptr = source.c_str();
    glShaderSource(shader, 1, &source_ptr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "Shader compilation failed: " << info_log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool ShaderProgram::link_program() {
    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vertex_shader_);
    glAttachShader(program_id_, fragment_shader_);
    glLinkProgram(program_id_);
    
    GLint success;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program_id_, 512, nullptr, info_log);
        std::cerr << "Shader linking failed: " << info_log << std::endl;
        glDeleteProgram(program_id_);
        program_id_ = 0;
        return false;
    }
    
    
    return true;
}

// TextRenderer implementation

TextRenderer::TextRenderer(CanvasRenderer* renderer)
    : renderer_(renderer) {
}

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::initialize() {
    // TODO: Implement proper font loading and text rendering
    initialized_ = true;
    return true;
}

void TextRenderer::shutdown() {
    // TODO: Cleanup font resources
    initialized_ = false;
}

void TextRenderer::render_text(const std::string& text, const Point2D& position, const ColorRGBA& color, float size) {
    // Debug text rendering - make text highly visible
    if (text.empty() || !renderer_) return;
    
    float char_width = size * 0.6f;
    float char_height = size;
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c == ' ') continue; // Skip spaces
        
        Rect2D char_rect(
            position.x + i * char_width,
            position.y,
            char_width * 0.8f,
            char_height
        );
        
        // Make text bright and visible for debugging
        ColorRGBA bright_color = ColorRGBA(
            std::max(0.8f, color.r), 
            std::max(0.8f, color.g), 
            std::max(0.8f, color.b), 
            1.0f
        );
        
        renderer_->draw_rect(char_rect, bright_color);
        
        // Add border for character visibility
        renderer_->draw_rect_outline(char_rect, ColorRGBA(1.0f, 1.0f, 0.0f, 0.8f), 1.0f);
    }
}

Point2D TextRenderer::measure_text(const std::string& text, float size) const {
    if (size == 0) size = font_size_;
    return Point2D(text.length() * size * 0.6f, size);
}

// Path rendering implementation
void CanvasRenderer::begin_path() {
    current_path_.clear();
    path_started_ = true;
}

void CanvasRenderer::move_to(float x, float y) {
    if (!path_started_) return;
    
    PathPoint point;
    point.type = PathPoint::MoveTo;
    point.x = x;
    point.y = y;
    current_path_.push_back(point);
}

void CanvasRenderer::line_to(float x, float y) {
    if (!path_started_ || current_path_.empty()) return;
    
    PathPoint point;
    point.type = PathPoint::LineTo;
    point.x = x;
    point.y = y;
    current_path_.push_back(point);
}

void CanvasRenderer::curve_to(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
    if (!path_started_ || current_path_.empty()) return;
    
    PathPoint point;
    point.type = PathPoint::CurveTo;
    point.x = x;
    point.y = y;
    point.cp1x = cp1x;
    point.cp1y = cp1y;
    point.cp2x = cp2x;
    point.cp2y = cp2y;
    current_path_.push_back(point);
}

void CanvasRenderer::close_path() {
    if (!path_started_ || current_path_.size() < 2) return;
    
    // Find the last MoveTo
    for (auto it = current_path_.rbegin(); it != current_path_.rend(); ++it) {
        if (it->type == PathPoint::MoveTo) {
            line_to(it->x, it->y);
            break;
        }
    }
}

void CanvasRenderer::stroke_path(const ColorRGBA& color, float width) {
    if (!path_started_ || current_path_.size() < 2) return;
    
    // Convert path to line segments and render
    Point2D current_pos;
    bool has_current = false;
    
    for (const auto& point : current_path_) {
        switch (point.type) {
            case PathPoint::MoveTo:
                current_pos = Point2D(point.x, point.y);
                has_current = true;
                break;
                
            case PathPoint::LineTo:
                if (has_current) {
                    draw_line(current_pos, Point2D(point.x, point.y), color, width);
                    current_pos = Point2D(point.x, point.y);
                }
                break;
                
            case PathPoint::CurveTo:
                if (has_current) {
                    // Approximate cubic bezier with line segments
                    const int segments = 20;
                    Point2D prev = current_pos;
                    
                    for (int i = 1; i <= segments; i++) {
                        float t = static_cast<float>(i) / segments;
                        float t2 = t * t;
                        float t3 = t2 * t;
                        float mt = 1 - t;
                        float mt2 = mt * mt;
                        float mt3 = mt2 * mt;
                        
                        float x = mt3 * current_pos.x + 
                                 3 * mt2 * t * point.cp1x + 
                                 3 * mt * t2 * point.cp2x + 
                                 t3 * point.x;
                        float y = mt3 * current_pos.y + 
                                 3 * mt2 * t * point.cp1y + 
                                 3 * mt * t2 * point.cp2y + 
                                 t3 * point.y;
                        
                        Point2D next(x, y);
                        draw_line(prev, next, color, width);
                        prev = next;
                    }
                    
                    current_pos = Point2D(point.x, point.y);
                }
                break;
        }
    }
    
    path_started_ = false;
}

void CanvasRenderer::fill_path(const ColorRGBA& color) {
    if (!path_started_ || current_path_.size() < 3) return;
    
    // Simplified fill using triangulation
    // For complex paths, a proper triangulation algorithm would be needed
    // This is a simplified version that works for convex paths
    
    std::vector<Point2D> vertices;
    Point2D current_pos;
    bool has_current = false;
    
    for (const auto& point : current_path_) {
        switch (point.type) {
            case PathPoint::MoveTo:
                current_pos = Point2D(point.x, point.y);
                vertices.push_back(current_pos);
                has_current = true;
                break;
                
            case PathPoint::LineTo:
                if (has_current) {
                    vertices.push_back(Point2D(point.x, point.y));
                    current_pos = Point2D(point.x, point.y);
                }
                break;
                
            case PathPoint::CurveTo:
                // Tessellate curve into line segments
                if (has_current) {
                    const int segments = 10;
                    for (int i = 1; i <= segments; i++) {
                        float t = static_cast<float>(i) / segments;
                        float t2 = t * t;
                        float t3 = t2 * t;
                        float mt = 1 - t;
                        float mt2 = mt * mt;
                        float mt3 = mt2 * mt;
                        
                        float x = mt3 * current_pos.x + 
                                 3 * mt2 * t * point.cp1x + 
                                 3 * mt * t2 * point.cp2x + 
                                 t3 * point.x;
                        float y = mt3 * current_pos.y + 
                                 3 * mt2 * t * point.cp1y + 
                                 3 * mt * t2 * point.cp2y + 
                                 t3 * point.y;
                        
                        vertices.push_back(Point2D(x, y));
                    }
                    current_pos = Point2D(point.x, point.y);
                }
                break;
        }
    }
    
    // Simple fan triangulation for convex shapes
    if (vertices.size() >= 3) {
        for (size_t i = 2; i < vertices.size(); i++) {
            // Draw triangle from first vertex to i-1 and i
            // This would be better done with actual triangle rendering
            // For now, approximate with very thin rectangles
            draw_line(vertices[0], vertices[i-1], color, 1);
            draw_line(vertices[i-1], vertices[i], color, 1);
            draw_line(vertices[i], vertices[0], color, 1);
        }
    }
    
    path_started_ = false;
}

// Filter effects (stubs - require shader support)
void CanvasRenderer::push_blur(float radius) {
    FilterState filter;
    filter.type = FilterState::Blur;
    filter.value = radius;
    filter_stack_.push_back(filter);
    
    // TODO: Implement blur effect with framebuffer and shader
}

void CanvasRenderer::pop_blur() {
    if (!filter_stack_.empty() && filter_stack_.back().type == FilterState::Blur) {
        filter_stack_.pop_back();
    }
}

void CanvasRenderer::push_brightness(float factor) {
    FilterState filter;
    filter.type = FilterState::Brightness;
    filter.value = factor;
    filter_stack_.push_back(filter);
    
    // TODO: Implement brightness adjustment in shader
}

void CanvasRenderer::pop_brightness() {
    if (!filter_stack_.empty() && filter_stack_.back().type == FilterState::Brightness) {
        filter_stack_.pop_back();
    }
}

void CanvasRenderer::push_contrast(float factor) {
    FilterState filter;
    filter.type = FilterState::Contrast;
    filter.value = factor;
    filter_stack_.push_back(filter);
    
    // TODO: Implement contrast adjustment in shader
}

void CanvasRenderer::pop_contrast() {
    if (!filter_stack_.empty() && filter_stack_.back().type == FilterState::Contrast) {
        filter_stack_.pop_back();
    }
}

} // namespace voxel_canvas