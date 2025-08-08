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
#include <iostream>
#include <fstream>
#include <sstream>

namespace voxel_canvas {

// OpenGL error checking helper (Blender-style)
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

    // Enhanced OpenGL state setup (Blender-style)
    
    // Critical pixel store setup (from Blender)
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
    
    // Force flush any pending OpenGL commands first
    glFlush();
    
    // Ensure we're rendering to default framebuffer (screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl_error("bind default framebuffer");
    
    
    // **Canvas UI Professional Background System** - Now using proper theme colors
    // UI rendering is confirmed working, switch to production colors
    ColorRGBA bg = theme_.background_primary; // Professional dark background like Blender
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    check_gl_error("clear color");
    
    glClear(GL_COLOR_BUFFER_BIT);
    check_gl_error("clear");
    
}

void CanvasRenderer::end_frame() {
    // Using immediate mode rendering for now - more reliable than batching
    // Will implement proper batching system later based on Blender's approach
    
    // CRITICAL: Reset OpenGL state after UI rendering (Blender pattern)
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

void CanvasRenderer::draw_rect(const Rect2D& rect, const ColorRGBA& color) {
    // **Canvas UI Professional Immediate Mode Rendering** (inspired by Blender's GPU immediate mode)
    // Always render - no debug limits, no frame-based restrictions
    
    // Validate shader state first (critical for continuous rendering)
    if (!ui_shader_ || !ui_shader_->is_valid()) {
        std::cerr << "ERROR: UI shader not valid in draw_rect!" << std::endl;
        return;
    }
    
    
    // **Blender-Style Immediate Mode**: Create geometry and render immediately
    // No batching, no state tracking, just direct OpenGL calls
    
    UIVertex vertices[4] = {
        UIVertex(rect.x, rect.y, 0, 0, color.r, color.g, color.b, color.a),
        UIVertex(rect.x + rect.width, rect.y, 1, 0, color.r, color.g, color.b, color.a),
        UIVertex(rect.x + rect.width, rect.y + rect.height, 1, 1, color.r, color.g, color.b, color.a),
        UIVertex(rect.x, rect.y + rect.height, 0, 1, color.r, color.g, color.b, color.a)
    };
    
    uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
    
    // **OpenGL State Setup** - Fresh state every call like Blender
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    check_gl_error("bind texture");
    
    ui_shader_->use();
    check_gl_error("use shader");
    
    // **CRITICAL**: Set projection matrix for EVERY draw call
    // The projection might not be set if shader was recompiled or state was lost
    // Ensure viewport dimensions are valid
    float vp_width = std::max(1.0f, (float)viewport_width_);
    float vp_height = std::max(1.0f, (float)viewport_height_);
    
    float projection[16] = {
        2.0f / vp_width,  0,                  0, 0,
        0,               -2.0f / vp_height,    0, 0,
        0,                0,                  -1, 0,
        -1,               1,                   0, 1
    };
    ui_shader_->set_uniform("u_projection", projection, 16);
    check_gl_error("set projection for draw");
    
    // **SDF Shader Uniforms** - Set fresh every call
    ui_shader_->set_uniform("u_widget_rect", ColorRGBA(rect.x, rect.y, rect.width, rect.height));
    ui_shader_->set_uniform("u_corner_radius", ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f)); 
    ui_shader_->set_uniform("u_border_width", 0.0f);
    ui_shader_->set_uniform("u_border_color", ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f));
    ui_shader_->set_uniform("u_aa_radius", 1.0f);
    ui_shader_->set_uniform("u_texture", 0);
    check_gl_error("set SDF uniforms");
    
    // **Buffer Operations** - Upload fresh data every call like Blender's immediate mode
    glBindVertexArray(ui_vao_);
    check_gl_error("bind VAO");
    
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    check_gl_error("bind VBO");
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    check_gl_error("upload vertex data");
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    check_gl_error("bind EBO");
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
    check_gl_error("upload index data");
    
    // **Draw Call** - The actual rendering
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    check_gl_error("draw elements");
    
    // **CRITICAL FIX**: Force GPU to execute commands immediately
    // Without this, commands may be queued but not executed before swap
    glFlush();
    check_gl_error("flush after draw");
    
    
    // **Cleanup** - Reset state like Blender's immediate mode
    ui_shader_->unuse();
    check_gl_error("unuse shader");
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // **Statistics**
    draw_calls_this_frame_++;
    vertices_this_frame_ += 4;
}

void CanvasRenderer::draw_rect_outline(const Rect2D& rect, const ColorRGBA& color, float line_width) {
    draw_line(Point2D(rect.x, rect.y), Point2D(rect.x + rect.width, rect.y), color, line_width);
    draw_line(Point2D(rect.x + rect.width, rect.y), Point2D(rect.x + rect.width, rect.y + rect.height), color, line_width);
    draw_line(Point2D(rect.x + rect.width, rect.y + rect.height), Point2D(rect.x, rect.y + rect.height), color, line_width);
    draw_line(Point2D(rect.x, rect.y + rect.height), Point2D(rect.x, rect.y), color, line_width);
}

void CanvasRenderer::draw_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float width) {
    // Immediate mode line rendering (like draw_rect)
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
    
    // Create quad vertices for the line
    UIVertex vertices[4] = {
        UIVertex(start.x + normal.x, start.y + normal.y, 0, 0, color.r, color.g, color.b, color.a),
        UIVertex(end.x + normal.x, end.y + normal.y, 1, 0, color.r, color.g, color.b, color.a),
        UIVertex(end.x - normal.x, end.y - normal.y, 1, 1, color.r, color.g, color.b, color.a),
        UIVertex(start.x - normal.x, start.y - normal.y, 0, 1, color.r, color.g, color.b, color.a)
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
    
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    glBindVertexArray(ui_vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glFlush();
    
    ui_shader_->unuse();
    glBindVertexArray(0);
    
    draw_calls_this_frame_++;
    vertices_this_frame_ += 4;
}

void CanvasRenderer::draw_circle(const Point2D& center, float radius, const ColorRGBA& color, int segments) {
    // TODO: Implement circle rendering
    // For now, draw a square
    Rect2D rect(center.x - radius, center.y - radius, radius * 2, radius * 2);
    draw_rect(rect, color);
}

void CanvasRenderer::draw_text(const std::string& text, const Point2D& position, const ColorRGBA& color, float size) {
    if (font_system_) {
        font_system_->render_text(this, text, position, "Inter", static_cast<int>(size), color);
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
    
    glBindTexture(GL_TEXTURE_2D, white_texture_);
    
    glBindVertexArray(ui_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glFlush(); // Ensure commands are executed
    
    ui_shader_->unuse();
    glBindVertexArray(0);
}

void CanvasRenderer::draw_rounded_rect(const Rect2D& rect, const ColorRGBA& color, float corner_radius) {
    draw_widget(rect, color, corner_radius, 0.0f, ColorRGBA::transparent());
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

void CanvasRenderer::setup_shaders() {
    create_ui_shader();
    create_text_shader();
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

} // namespace voxel_canvas