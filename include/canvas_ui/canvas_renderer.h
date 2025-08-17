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
#include "scaled_theme.h"
#include "glad/gl.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

// Forward declare to avoid circular dependency
namespace voxel_canvas { 
    class FontSystem;
    class PolylineShader;
}

namespace voxel_canvas {

// Forward declarations
class CanvasWindow;
class ShaderProgram;
class TextRenderer;
struct RenderBatch;

// UIVertex definition - moved here from bottom of file for CurrentBatch
struct UIVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
    
    UIVertex(float x = 0, float y = 0, float u = 0, float v = 0, 
             float r = 1, float g = 1, float b = 1, float a = 1)
        : x(x), y(y), u(u), v(v), r(r), g(g), b(b), a(a) {}
};

// Widget instance data for GPU instancing with professional architecture
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

// Text alignment options
enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

enum class TextBaseline {
    TOP,
    MIDDLE,
    BOTTOM
};

/**
 * Modern SDF-based renderer for Canvas UI
 * Professional GPU-accelerated widget rendering system
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
    
    // Basic drawing primitives (UI - Batched rendering)
    void draw_rect(const Rect2D& rect, const ColorRGBA& color);
    void draw_rect_outline(const Rect2D& rect, const ColorRGBA& color, float line_width = 1.0f);
    void draw_text(const std::string& text, const Point2D& position, const ColorRGBA& color, 
                   float size = 14.0f, TextAlign align = TextAlign::LEFT, 
                   TextBaseline baseline = TextBaseline::TOP);
    
    // UI line drawing using batched rectangles
    void draw_line_batched(const Point2D& start, const Point2D& end, const ColorRGBA& color, float width = 1.0f);
    
    // Viewport immediate rendering (for 3D overlays like navigation widget)
    // These use immediate mode OpenGL and should only be called by viewport components
    void draw_viewport_line(const Point2D& start, const Point2D& end, const ColorRGBA& color, float width = 1.0f);
    void draw_viewport_circle(const Point2D& center, float radius, const ColorRGBA& color, int segments = 32);
    void draw_viewport_circle_ring(const Point2D& center, float radius, const ColorRGBA& color, float thickness = 3.0f, int segments = 32);
    
    // SDF-based widget rendering (future implementation)
    void draw_widget(const Rect2D& rect, const ColorRGBA& color,
                    float corner_radius = 0.0f, float border_width = 0.0f,
                    const ColorRGBA& border_color = ColorRGBA(0, 0, 0, 0));
    
    void draw_rounded_rect(const Rect2D& rect, const ColorRGBA& color,
                          float corner_radius = 4.0f);
    
    void draw_rounded_rect_varied(const Rect2D& rect, const ColorRGBA& color,
                                  float tl_radius, float tr_radius, 
                                  float br_radius, float bl_radius);
    
    void draw_button(const Rect2D& rect, const ColorRGBA& color,
                    bool pressed = false, bool hovered = false);
    
    // Advanced rendering
    void draw_gradient_rect(const Rect2D& rect, const ColorRGBA& top_color, 
                           const ColorRGBA& bottom_color, bool horizontal = false);
    
    // More gradient support
    void draw_linear_gradient(const Rect2D& rect, float angle_degrees,
                             const std::vector<std::pair<ColorRGBA, float>>& stops);
    
    // Gaussian blur effects
    void begin_blur_region(const Rect2D& region, float blur_radius);
    void end_blur_region();
    void apply_gaussian_blur(GLuint texture, const Rect2D& src_rect, const Rect2D& dst_rect, float blur_radius);
    
    // Backdrop filter effects (frosted glass, etc)
    void begin_backdrop_filter(const Rect2D& region);
    void end_backdrop_filter();
    void apply_backdrop_blur(const Rect2D& region, float blur_radius, float opacity = 0.8f);
    void apply_backdrop_saturate(const Rect2D& region, float saturation);
    void apply_backdrop_brightness(const Rect2D& region, float brightness);
    
    void draw_radial_gradient(const Rect2D& rect, const Point2D& center,
                             float radius_x, float radius_y,
                             const std::vector<std::pair<ColorRGBA, float>>& stops);
    
    void draw_conic_gradient(const Rect2D& rect, const Point2D& center,
                            float start_angle,
                            const std::vector<std::pair<ColorRGBA, float>>& stops);
    
    void draw_shadow(const Rect2D& rect, float blur_radius, float spread,
                    float offset_x, float offset_y, const ColorRGBA& color);
    
    // Optimized shadow rendering using cached 9-patch textures
    void draw_shadow_cached(const Rect2D& rect, float blur_radius, float spread,
                           float offset_x, float offset_y, const ColorRGBA& color);
    
    void draw_dashed_line(const Point2D& start, const Point2D& end, 
                         const ColorRGBA& color, float width = 1.0f,
                         float dash_length = 5.0f, float gap_length = 3.0f);
    
    void draw_dotted_line(const Point2D& start, const Point2D& end,
                         const ColorRGBA& color, float width = 1.0f,
                         float dot_spacing = 4.0f);
    
    // Text with advanced features
    void draw_text_with_ellipsis(const std::string& text, const Rect2D& bounds,
                                 const ColorRGBA& color, float size = 14.0f,
                                 TextAlign align = TextAlign::LEFT);
    
    void draw_text_with_shadow(const std::string& text, const Point2D& position,
                               const ColorRGBA& color, const ColorRGBA& shadow_color,
                               float size = 14.0f, float shadow_offset_x = 1.0f,
                               float shadow_offset_y = 1.0f, float shadow_blur = 0.0f);
    
    // Image rendering with fit modes
    enum class ObjectFit {
        Fill,      // Stretch to fill
        Contain,   // Scale to fit within bounds
        Cover,     // Scale to cover bounds
        None,      // No scaling
        ScaleDown  // Scale down if larger than bounds
    };
    
    void draw_image(GLuint texture_id, const Rect2D& bounds, ObjectFit fit = ObjectFit::Fill,
                   const ColorRGBA& tint = ColorRGBA(1, 1, 1, 1));
    
    // Path rendering (simplified)
    void begin_path();
    void move_to(float x, float y);
    void line_to(float x, float y);
    void curve_to(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void close_path();
    void stroke_path(const ColorRGBA& color, float width = 1.0f);
    void fill_path(const ColorRGBA& color);
    
    // Filter effects
    void push_blur(float radius);
    void pop_blur();
    void push_brightness(float factor);
    void pop_brightness();
    void push_contrast(float factor);
    void pop_contrast();
    
    // Batch management 
    void submit_batch(const RenderBatch& batch);
    void flush_batches();
    void flush_current_batch();  // Flush the current batch being built
    void render_sorted_batches(); // Render all batches sorted by state
    
    // Instance rendering
    void begin_instance_batch();
    void add_widget_instance(const Rect2D& rect, const ColorRGBA& color, 
                            float corner_radius = 0.0f, float border_width = 0.0f,
                            const ColorRGBA& border_color = ColorRGBA(0,0,0,0),
                            float outline_width = 0.0f, float outline_offset = 0.0f,
                            const ColorRGBA& outline_color = ColorRGBA(0,0,0,0));
    void flush_instances();  // Render all accumulated instances in one draw call
    
    // Viewport management
    void set_viewport(int x, int y, int width, int height);
    Point2D get_viewport_size() const;
    
    // Theme
    void set_theme(const CanvasTheme& theme);
    const CanvasTheme& get_theme() const { return theme_; }
    
    // Clipping stack
    void push_clip(const Rect2D& rect);
    void pop_clip();
    void enable_scissor(const Rect2D& rect);
    void disable_scissor();
    
    // Transform matrix stack
    void push_transform();
    void pop_transform();
    void translate(float x, float y);
    void translate3d(float x, float y, float z);
    void rotate(float degrees);  // Rotation in degrees around Z
    void rotate3d(float angle_x, float angle_y, float angle_z);  // 3D rotation in degrees
    void rotate_axis(float angle, float axis_x, float axis_y, float axis_z);  // Rotation around arbitrary axis
    void scale(float x, float y);
    void scale3d(float x, float y, float z);
    void set_transform(const float* matrix4x4);  // Set custom 4x4 matrix
    void apply_transform(const float* matrix4x4);  // Multiply current transform by matrix
    void set_perspective(float fov_degrees, float near_plane, float far_plane);
    void set_ortho(float left, float right, float bottom, float top, float near, float far);
    void skew(float angle_x, float angle_y);  // Skew transformation
    
    // Opacity/Alpha blending
    void set_global_opacity(float opacity);
    float get_global_opacity() const { return global_opacity_; }
    void push_opacity(float opacity);
    void pop_opacity();
    
    // Texture binding and rendering
    void bind_texture(GLuint texture_id, int unit = 0);
    void draw_texture(GLuint texture_id, const Rect2D& rect, const ColorRGBA& tint = ColorRGBA(1, 1, 1, 1));
    
    // SDF shader support (used internally by UI shader)
    GLuint get_sdf_shader() const { return sdf_shader_program_; }
    bool has_sdf_support() const { return sdf_shader_program_ != 0; }
    
    // Statistics
    int get_draw_calls_this_frame() const { return draw_calls_this_frame_; }
    int get_vertices_this_frame() const { return vertices_this_frame_; }
    int get_frame_id() const { return frame_id_; }  // Get current frame ID for duplicate detection
    
    // Dirty rectangle management
    void mark_dirty(const Rect2D& rect) { dirty_tracker_.mark_dirty(rect); }
    void mark_full_redraw() { dirty_tracker_.mark_full_redraw(); }
    bool needs_redraw() const { return dirty_tracker_.needs_redraw(); }
    void clear_dirty_regions() { dirty_tracker_.clear(); }
    
    // Occlusion culling management
    struct OcclusionFlags {
        bool allows_occlusion = false;     // Widget can be occluded
        bool is_opaque = false;            // Widget is fully opaque
        bool has_transparency = false;     // Widget uses transparency
        bool has_shadow = false;           // Widget has shadow effects
        bool is_3d_element = false;        // Widget needs depth testing
        bool force_render = false;         // Always render (debug, etc)
    };
    
    bool should_cull_widget(const Rect2D& bounds, const OcclusionFlags& flags) {
        return occlusion_tracker_.should_cull(bounds, flags);
    }
    void add_occluder(const Rect2D& bounds, const OcclusionFlags& flags) {
        occlusion_tracker_.add_occluder(bounds, flags);
    }
    void begin_occlusion_frame() { occlusion_tracker_.begin_frame(); }
    void end_occlusion_frame() { occlusion_tracker_.end_frame(); }
    void set_occlusion_debug(bool enabled) { occlusion_tracker_.set_debug_mode(enabled); }
    bool is_occlusion_debug() const { return occlusion_tracker_.is_debug_mode(); }
    int get_culled_widget_count() const { return occlusion_tracker_.get_culled_count(); }
    int get_tested_widget_count() const { return occlusion_tracker_.get_tested_count(); }
    
    // Matrix access for text rendering
    void get_projection_matrix(float* matrix) const;
    
    // Window properties access
    float get_content_scale() const;
    CanvasWindow* get_window() const { return window_; }
    
    // ScaledTheme access - primary way to get theme values
    class ScaledTheme get_scaled_theme() const;

private:
    // Shader and resource management
    void setup_shaders();
    void setup_buffers();
    void setup_default_textures();
    
    void create_ui_shader();
    void create_instance_shader();
    void create_text_shader();
    void create_sdf_shader();
    void create_blur_shader();
    void create_gradient_shader();
    
    // Batch management
    void sort_batches_by_layer();
    
    // Debug visualization
    void draw_occlusion_debug();
    
    // Text batching
    void batch_text(const std::string& text, const Point2D& position, 
                    const std::string& font_name, int size, const ColorRGBA& color);
    
private:
    CanvasWindow* window_;
    bool initialized_ = false;
    
    // Rendering resources
    std::unique_ptr<ShaderProgram> ui_shader_;
    std::unique_ptr<ShaderProgram> instance_shader_;  // New instanced rendering shader
    std::unique_ptr<ShaderProgram> text_shader_;
    std::unique_ptr<FontSystem> font_system_;
    std::unique_ptr<PolylineShader> polyline_shader_;
    
    GLuint ui_vao_ = 0;
    GLuint ui_vbo_ = 0; 
    GLuint ui_ebo_ = 0;
    
    // Static widget geometry (industry standard approach)
    GLuint widget_vao_ = 0;
    GLuint widget_vbo_ = 0;
    GLuint widget_ebo_ = 0;
    
    // Instance rendering system - Single-pass widget rendering
    struct WidgetInstanceData {
        float transform[4];      // x, y, width, height
        float color[4];          // RGBA (background/fill color)
        float uv_rect[4];        // texture atlas coords (u0, v0, u1, v1)
        float params[4];         // corner_radius (tl, tr, br, bl)
        float border[4];         // border_width, border_color RGB
        float extra[4];          // outline_width, outline_offset, outline_color RG
        // Note: extra[2] = outline_color B, extra[3] = outline_alpha
    };
    
    GLuint instance_vbo_ = 0;  // Instance data buffer
    std::vector<WidgetInstanceData> instance_data_;
    size_t max_instances_ = 10000;  // Maximum instances per batch
    
    // Performance optimization flags (cross-platform)
    bool use_buffer_orphaning_ = true;   // Use buffer orphaning for updates
    bool use_mapped_buffers_ = false;    // Use mapped buffers (requires GL 4.4+)
    
    // Advanced features (only used if available)
    void* instance_buffer_ptr_ = nullptr;  // For persistent mapping (GL 4.4+)
    GLsync fence_sync_ = 0;                // For sync operations (GL 4.4+)
    
    // SDF shader
    GLuint sdf_shader_program_ = 0;
    GLint sdf_u_projection_ = -1;
    GLint sdf_u_texture_ = -1;
    GLint sdf_u_color_ = -1;
    GLint sdf_u_threshold_ = -1;
    GLint sdf_u_smoothness_ = -1;
    
    // Gaussian blur shader
    std::unique_ptr<ShaderProgram> blur_shader_;
    GLuint blur_fbo_ = 0;
    GLuint blur_texture_ = 0;
    int blur_texture_width_ = 0;
    int blur_texture_height_ = 0;
    
    // Backdrop filter resources
    GLuint backdrop_fbo_ = 0;
    GLuint backdrop_texture_ = 0;
    int backdrop_texture_width_ = 0;
    int backdrop_texture_height_ = 0;
    Rect2D backdrop_region_;
    
    // Gradient shader
    std::unique_ptr<ShaderProgram> gradient_shader_;
    
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
    
    // Current batch being built
    struct CurrentBatch {
        std::vector<UIVertex> vertices;
        std::vector<uint32_t> indices;
        GLuint texture_id = 0;
        GLenum blend_mode = GL_ONE;  // Current blend mode
        GLuint shader_id = 0;         // Current shader
        bool needs_flush = false;
        int layer = 0;                // Render layer for sorting
        bool is_text = false;         // Whether this batch contains text glyphs
        
        void reset() {
            vertices.clear();
            indices.clear();
            needs_flush = false;
            is_text = false;
        }
        
        bool can_batch_with(GLuint tex, GLenum blend, GLuint shader) const {
            return !needs_flush && 
                   texture_id == tex && 
                   blend_mode == blend && 
                   shader_id == shader &&
                   vertices.size() < 10000;  // Max vertices per batch
        }
        
        // Generate sort key for state sorting
        uint64_t get_sort_key() const {
            // Sort by: layer (8 bits) | shader (16 bits) | texture (24 bits) | blend (16 bits)
            return (uint64_t(layer & 0xFF) << 56) |
                   (uint64_t(shader_id & 0xFFFF) << 40) |
                   (uint64_t(texture_id & 0xFFFFFF) << 16) |
                   (uint64_t(blend_mode & 0xFFFF));
        }
    };
    CurrentBatch current_batch_;
    
    // Completed batches waiting to be rendered
    struct CompletedBatch {
        std::vector<UIVertex> vertices;
        std::vector<uint32_t> indices;
        GLuint texture_id;
        GLenum blend_mode;
        GLuint shader_id;
        int layer;
        uint64_t sort_key;
        bool is_text;
        
        CompletedBatch(const CurrentBatch& current) 
            : vertices(current.vertices)
            , indices(current.indices)
            , texture_id(current.texture_id)
            , blend_mode(current.blend_mode)
            , shader_id(current.shader_id)
            , layer(current.layer)
            , is_text(current.is_text)
            , sort_key(current.get_sort_key()) {}
    };
    std::vector<CompletedBatch> completed_batches_;
    
    // Theme
    CanvasTheme theme_;
    
    // Statistics
    int draw_calls_this_frame_ = 0;
    int vertices_this_frame_ = 0;
    int frame_id_ = 0;  // Frame counter for duplicate detection
    
    // Transform matrix stack
    struct TransformState {
        float matrix[16];  // 4x4 matrix
    };
    std::vector<TransformState> transform_stack_;
    TransformState current_transform_;
    float projection_matrix_[16];  // Projection matrix for 3D transforms
    
    // Clipping stack
    std::vector<Rect2D> clip_stack_;
    
    // Opacity stack
    float global_opacity_ = 1.0f;
    std::vector<float> opacity_stack_;
    
    // Path rendering state
    struct PathPoint {
        float x, y;
        enum Type { MoveTo, LineTo, CurveTo } type;
        float cp1x, cp1y, cp2x, cp2y;  // Control points for curves
    };
    std::vector<PathPoint> current_path_;
    bool path_started_ = false;
    
    // Filter effect stack
    struct FilterState {
        enum Type { None, Blur, Brightness, Contrast } type;
        float value;
    };
    std::vector<FilterState> filter_stack_;
    
    // Shadow texture cache for 9-patch shadows
    class ShadowCache {
    public:
        struct ShadowKey {
            float blur_radius;
            float spread;
            uint32_t color_rgba;  // Packed RGBA
            
            bool operator<(const ShadowKey& other) const {
                if (blur_radius != other.blur_radius) return blur_radius < other.blur_radius;
                if (spread != other.spread) return spread < other.spread;
                return color_rgba < other.color_rgba;
            }
        };
        
        struct NinePatchTexture {
            GLuint texture_id = 0;
            int texture_width = 0;
            int texture_height = 0;
            int border_size = 0;  // Size of the border region
            
            // UV coordinates for 9-patch regions
            float left_u, center_u, right_u;
            float top_v, middle_v, bottom_v;
        };
        
        NinePatchTexture* get_or_create_shadow(float blur_radius, float spread, const ColorRGBA& color) {
            ShadowKey key{blur_radius, spread, pack_color(color)};
            
            auto it = cache_.find(key);
            if (it != cache_.end()) {
                return &it->second;
            }
            
            // Create new shadow texture
            NinePatchTexture& texture = cache_[key];
            create_shadow_texture(texture, blur_radius, spread, color);
            return &texture;
        }
        
        void clear() {
            for (auto& [key, texture] : cache_) {
                if (texture.texture_id) {
                    glDeleteTextures(1, &texture.texture_id);
                }
            }
            cache_.clear();
        }
        
    private:
        std::map<ShadowKey, NinePatchTexture> cache_;
        
        uint32_t pack_color(const ColorRGBA& color) const {
            return (uint32_t(color.r * 255) << 24) |
                   (uint32_t(color.g * 255) << 16) |
                   (uint32_t(color.b * 255) << 8) |
                   uint32_t(color.a * 255);
        }
        
        void create_shadow_texture(NinePatchTexture& texture, float blur_radius, float spread, const ColorRGBA& color);
    };
    ShadowCache shadow_cache_;
    
    // Dirty rectangle tracking for partial redraws
    class DirtyRectTracker {
    public:
        void mark_dirty(const Rect2D& rect) {
            if (needs_full_redraw_) return;  // Already doing full redraw
            
            if (dirty_regions_.empty()) {
                dirty_regions_.push_back(rect);
            } else {
                // Try to merge with existing rectangles
                bool merged = false;
                for (auto& existing : dirty_regions_) {
                    if (should_merge(existing, rect)) {
                        existing = merge_rects(existing, rect);
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    dirty_regions_.push_back(rect);
                }
                
                // If too many regions, just do full redraw
                if (dirty_regions_.size() > 8) {
                    mark_full_redraw();
                }
            }
        }
        
        void mark_full_redraw() {
            needs_full_redraw_ = true;
            dirty_regions_.clear();
        }
        
        bool needs_redraw() const {
            return needs_full_redraw_ || !dirty_regions_.empty();
        }
        
        bool is_full_redraw() const { return needs_full_redraw_; }
        
        const std::vector<Rect2D>& get_dirty_regions() const { return dirty_regions_; }
        
        void clear() {
            dirty_regions_.clear();
            needs_full_redraw_ = false;
        }
        
        // Check if point/rect intersects any dirty region
        bool is_dirty(const Rect2D& rect) const {
            if (needs_full_redraw_) return true;
            for (const auto& dirty : dirty_regions_) {
                if (rects_intersect(dirty, rect)) return true;
            }
            return false;
        }
        
    private:
        std::vector<Rect2D> dirty_regions_;
        bool needs_full_redraw_ = true;  // Start with full redraw
        
        bool should_merge(const Rect2D& a, const Rect2D& b) const {
            // Merge if rectangles overlap or are very close
            float expand = 10.0f;  // Merge if within 10 pixels
            Rect2D expanded_a(a.x - expand, a.y - expand, 
                            a.width + expand * 2, a.height + expand * 2);
            return rects_intersect(expanded_a, b);
        }
        
        Rect2D merge_rects(const Rect2D& a, const Rect2D& b) const {
            float min_x = std::min(a.x, b.x);
            float min_y = std::min(a.y, b.y);
            float max_x = std::max(a.x + a.width, b.x + b.width);
            float max_y = std::max(a.y + a.height, b.y + b.height);
            return Rect2D(min_x, min_y, max_x - min_x, max_y - min_y);
        }
        
        bool rects_intersect(const Rect2D& a, const Rect2D& b) const {
            return !(a.x + a.width < b.x || b.x + b.width < a.x ||
                    a.y + a.height < b.y || b.y + b.height < a.y);
        }
    };
    DirtyRectTracker dirty_tracker_;
    
    // Occlusion culling system for reducing overdraw
    class OcclusionTracker {
    public:
        using OcclusionFlags = CanvasRenderer::OcclusionFlags;
        
        void begin_frame() {
            occluded_regions_.clear();
            culled_count_ = 0;
            tested_count_ = 0;
            debug_mode_ = false;
        }
        
        void end_frame() {
            // Stats available for debugging
        }
        
        // Test if a widget should be culled
        bool should_cull(const Rect2D& bounds, const OcclusionFlags& flags) {
            tested_count_++;
            
            // Never cull if not allowed or has special requirements
            if (!flags.allows_occlusion || flags.force_render || 
                flags.is_3d_element || flags.has_transparency || flags.has_shadow) {
                return false;
            }
            
            // Check if completely covered by opaque regions
            for (const auto& occluder : occluded_regions_) {
                if (is_completely_covered(bounds, occluder)) {
                    culled_count_++;
                    return true;
                }
            }
            
            return false;
        }
        
        // Mark region as occluding (for opaque widgets)
        void add_occluder(const Rect2D& bounds, const OcclusionFlags& flags) {
            // Only add if widget is opaque and can occlude others
            if (!flags.is_opaque || flags.has_transparency) {
                return;
            }
            
            // Try to merge with existing occluders
            bool merged = false;
            for (auto& existing : occluded_regions_) {
                if (should_merge_occluders(existing, bounds)) {
                    existing = merge_rects(existing, bounds);
                    merged = true;
                    break;
                }
            }
            
            if (!merged) {
                occluded_regions_.push_back(bounds);
            }
        }
        
        // Debug visualization
        void set_debug_mode(bool enabled) { debug_mode_ = enabled; }
        bool is_debug_mode() const { return debug_mode_; }
        
        const std::vector<Rect2D>& get_occluded_regions() const { 
            return occluded_regions_; 
        }
        
        int get_culled_count() const { return culled_count_; }
        int get_tested_count() const { return tested_count_; }
        
    private:
        std::vector<Rect2D> occluded_regions_;
        int culled_count_ = 0;
        int tested_count_ = 0;
        bool debug_mode_ = false;
        
        bool is_completely_covered(const Rect2D& rect, const Rect2D& occluder) const {
            return rect.x >= occluder.x && 
                   rect.y >= occluder.y &&
                   rect.x + rect.width <= occluder.x + occluder.width &&
                   rect.y + rect.height <= occluder.y + occluder.height;
        }
        
        bool should_merge_occluders(const Rect2D& a, const Rect2D& b) const {
            // Merge if overlapping or adjacent
            return !(a.x > b.x + b.width || b.x > a.x + a.width ||
                    a.y > b.y + b.height || b.y > a.y + a.height);
        }
        
        Rect2D merge_rects(const Rect2D& a, const Rect2D& b) const {
            float min_x = std::min(a.x, b.x);
            float min_y = std::min(a.y, b.y);
            float max_x = std::max(a.x + a.width, b.x + b.width);
            float max_y = std::max(a.y + a.height, b.y + b.height);
            return Rect2D(min_x, min_y, max_x - min_x, max_y - min_y);
        }
    };
    OcclusionTracker occlusion_tracker_;
    
    // Helper methods for matrix operations
    void multiply_matrix(float* result, const float* a, const float* b);
    void identity_matrix(float* matrix);
    void translate_matrix(float* matrix, float x, float y);
    void rotate_matrix(float* matrix, float degrees);
    void scale_matrix(float* matrix, float x, float y);
    void update_projection_matrix();
    
    // Helper for gradient color interpolation
    static ColorRGBA interpolate_gradient_color(
        const std::vector<std::pair<ColorRGBA, float>>& stops, float position);
};

// Utility functions for shader compilation
GLuint compile_shader(const std::string& source, GLenum type);
GLuint create_shader_program(const std::string& vertex_source, 
                            const std::string& fragment_source);

// Utility classes for rendering system

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

// Implementation of get_scaled_theme from scaled_theme.h
// Must be here to avoid circular dependency
inline ScaledTheme get_scaled_theme(CanvasRenderer* renderer) {
    return ScaledTheme(renderer->get_theme(), renderer->get_content_scale());
}

} // namespace voxel_canvas