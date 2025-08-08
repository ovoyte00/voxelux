/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - SDF-based widget rendering system
 */

#pragma once

#include "canvas_core.h"
#include "glad/glad.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declare to avoid circular dependency
namespace voxel_canvas { class FontSystem; }

namespace voxel_canvas {

// Forward declarations
class CanvasWindow;
class ShaderProgram;
class TextRenderer;
struct RenderBatch;
struct UIVertex;

// Widget instance data for GPU instancing (Blender-inspired architecture)
struct WidgetInstance {
    // Transform: rect (x, y, width, height)
    float rect[4];
    // Round corners (top-left, top-right, bottom-right, bottom-left)
    float corners[4];
    // Colors
    float color_inner[4];
    float color_border[4];
    // Parameters
    float border_width;
    float shade_dir;  // 0 = no gradient, 1 = vertical, -1 = horizontal
    float widget_type;  // 0 = rect, 1 = circle, 2 = rounded rect
    float alpha;
};

// Text vertex for dynamic text rendering
struct TextVertex {
    float pos[2];      // Position
    float uv[2];       // Texture coordinates in atlas
    float color[4];    // RGBA color
};

// Text rendering now handled by FontSystem - see font_system.h

/**
 * Modern SDF-based renderer for Canvas UI
 * Inspired by Blender's GPU-accelerated widget rendering
 */
class CanvasRenderer {
public:
    explicit CanvasRenderer(CanvasWindow* window);
    ~CanvasRenderer();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Frame management
    void begin_frame();
    void end_frame();
    void present_frame();
    
    // Basic drawing primitives
    void draw_rect(const Rect2D& rect, const ColorRGBA& color);
    void draw_rect_outline(const Rect2D& rect, const ColorRGBA& color, float line_width = 1.0f);
    void draw_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float width = 1.0f);
    void draw_circle(const Point2D& center, float radius, const ColorRGBA& color, int segments = 32);
    void draw_text(const std::string& text, const Point2D& position, const ColorRGBA& color, float size = 14.0f);
    
    // SDF-based widget rendering (future implementation)
    void draw_widget(const Rect2D& rect, const ColorRGBA& color,
                    float corner_radius = 0.0f, float border_width = 0.0f,
                    const ColorRGBA& border_color = ColorRGBA(0, 0, 0, 0));
    
    void draw_rounded_rect(const Rect2D& rect, const ColorRGBA& color,
                          float corner_radius = 4.0f);
    
    void draw_button(const Rect2D& rect, const ColorRGBA& color,
                    bool pressed = false, bool hovered = false);
    
    // Batch management 
    void submit_batch(const RenderBatch& batch);
    void flush_batches();
    
    // Viewport management
    void set_viewport(int x, int y, int width, int height);
    Point2D get_viewport_size() const;
    
    // Theme
    void set_theme(const CanvasTheme& theme);
    const CanvasTheme& get_theme() const { return theme_; }
    
    // Clipping
    void enable_scissor(const Rect2D& rect);
    void disable_scissor();
    
    // Texture binding
    void bind_texture(GLuint texture_id, int unit = 0);
    
    // Statistics
    int get_draw_calls_this_frame() const { return draw_calls_this_frame_; }
    int get_vertices_this_frame() const { return vertices_this_frame_; }

private:
    // Shader and resource management
    void setup_shaders();
    void setup_buffers();
    void setup_default_textures();
    
    void create_ui_shader();
    void create_text_shader();
    
    // Batch management
    void sort_batches_by_layer();
    
private:
    CanvasWindow* window_;
    bool initialized_ = false;
    
    // Rendering resources
    std::unique_ptr<ShaderProgram> ui_shader_;
    std::unique_ptr<ShaderProgram> text_shader_;
    std::unique_ptr<FontSystem> font_system_;
    
    GLuint ui_vao_ = 0;
    GLuint ui_vbo_ = 0; 
    GLuint ui_ebo_ = 0;
    
    // Textures
    GLuint white_texture_ = 0;
    GLuint checker_texture_ = 0;
    
    // Framebuffer management
    GLuint default_fbo_ = 0;
    GLuint current_fbo_ = 0;
    
    // Viewport state
    int viewport_x_ = 0;
    int viewport_y_ = 0;
    int viewport_width_ = 0;
    int viewport_height_ = 0;
    
    // Scissor testing
    bool scissor_enabled_ = false;
    Rect2D scissor_rect_;
    
    // Render batching
    std::vector<RenderBatch> render_batches_;
    std::vector<size_t> batch_render_order_;
    
    // Theme
    CanvasTheme theme_;
    
    // Statistics
    int draw_calls_this_frame_ = 0;
    int vertices_this_frame_ = 0;
};

// Utility functions for shader compilation
GLuint compile_shader(const std::string& source, GLenum type);
GLuint create_shader_program(const std::string& vertex_source, 
                            const std::string& fragment_source);

// Utility classes for rendering system

struct UIVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
    
    UIVertex(float x = 0, float y = 0, float u = 0, float v = 0, 
             float r = 1, float g = 1, float b = 1, float a = 1)
        : x(x), y(y), u(u), v(v), r(r), g(g), b(b), a(a) {}
};

enum class RenderLayer {
    UI_BACKGROUND = 0,
    UI_CONTENT = 1,
    UI_OVERLAY = 2,
    UI_MODAL = 3
};

struct RenderBatch {
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    RenderLayer layer = RenderLayer::UI_CONTENT;
    GLenum primitive_type = GL_TRIANGLES;
    GLuint texture_id = 0;
    
    void add_quad(const Rect2D& rect, const ColorRGBA& color);
    void add_textured_quad(const Rect2D& rect, const Rect2D& uv_rect, const ColorRGBA& color);
    void add_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float thickness = 1.0f);
};

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();
    
    bool load_from_strings(const std::string& vertex_source, const std::string& fragment_source);
    void use() const;
    void unuse() const;
    
    bool is_valid() const { return program_id_ != 0; }
    GLuint get_id() const { return program_id_; }
    
    void set_uniform(const std::string& name, float value);
    void set_uniform(const std::string& name, int value);
    void set_uniform(const std::string& name, const Point2D& value);
    void set_uniform(const std::string& name, const ColorRGBA& value);
    void set_uniform(const std::string& name, const float* matrix, int count = 1);
    
private:
    GLuint compile_shader(const std::string& source, GLenum type);
    bool link_program();
    GLint get_uniform_location(const std::string& name) const;
    
    GLuint program_id_ = 0;
    GLuint vertex_shader_ = 0;
    GLuint fragment_shader_ = 0;
    mutable std::unordered_map<std::string, GLint> uniform_cache_;
};

class TextRenderer {
public:
    explicit TextRenderer(CanvasRenderer* renderer);
    ~TextRenderer();
    
    bool initialize();
    void shutdown();
    
    void render_text(const std::string& text, const Point2D& position, 
                    const ColorRGBA& color, float size = 14.0f);
    Point2D measure_text(const std::string& text, float size = 14.0f) const;
    
private:
    CanvasRenderer* renderer_;
    bool initialized_ = false;
    float font_size_ = 14.0f;
};

} // namespace voxel_canvas