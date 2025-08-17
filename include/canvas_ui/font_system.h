/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Font system for Canvas UI - Professional text rendering
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "canvas_ui/canvas_core.h"
#include "canvas_ui/font_metrics.h"

// Forward declarations
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;

// Glyph metrics and rendering data
struct GlyphInfo {
    unsigned int texture_id = 0;  // OpenGL texture ID for this glyph
    Point2D size;                  // Size of glyph in pixels
    Point2D bearing;               // Offset from baseline to left/top of glyph
    float advance;                 // Horizontal advance to next glyph
    Rect2D uv_rect;               // UV coordinates in atlas (if using atlas)
    bool in_atlas = false;        // Whether this glyph is in the texture atlas
};

// Font face with multiple sizes
class FontFace {
public:
    FontFace(const std::string& path, int size = 14);
    ~FontFace();
    
    bool load();
    void unload();
    
    // Get or create glyph for character
    GlyphInfo* get_glyph(unsigned int codepoint, int size);
    
    // Font metrics
    float get_line_height(int size) const;
    float get_ascender(int size) const;
    float get_descender(int size) const;
    
    // Accurate text measurement with kerning
    TextMeasurement measure_text(const std::string& text, float font_size_px) const;
    
    // Get font metrics object
    const FontMetrics* get_metrics() const { return metrics_.get(); }
    
    // Font properties
    const std::string& get_path() const { return font_path_; }
    const std::string& get_name() const { return font_name_; }
    bool is_loaded() const { return face_ != nullptr; }
    
private:
    std::string font_path_;
    std::string font_name_;
    FT_Face face_ = nullptr;
    int default_size_;
    
    // Font metrics for accurate measurement
    std::unique_ptr<FontMetrics> metrics_;
    
    // Cache of loaded glyphs per size
    struct SizeCache {
        int pixel_size;
        std::unordered_map<unsigned int, GlyphInfo> glyphs;
        float line_height;
        float ascender;
        float descender;
    };
    std::unordered_map<int, SizeCache> size_caches_;
    
    // Load glyph for specific size
    GlyphInfo* load_glyph(unsigned int codepoint, int size);
};

// Main font system manager
class FontSystem {
public:
    FontSystem();
    ~FontSystem();
    
    // Initialize/shutdown
    bool initialize();
    void shutdown();
    
    // Font management
    bool load_font(const std::string& name, const std::string& path, int default_size = 14);
    FontFace* get_font(const std::string& name);
    
    // Text rendering
    Point2D measure_text(const std::string& text, const std::string& font_name, int size);
    
    // Accurate text measurement with kerning
    TextMeasurement measure_text_accurate(const std::string& text, const std::string& font_name, float font_size_px);
    
    // Viewport immediate mode text rendering - for 3D overlays like navigation widget
    // For UI text, use CanvasRenderer::draw_text() which uses batched rendering
    void render_text(CanvasRenderer* renderer, const std::string& text, 
                    const Point2D& position, const std::string& font_name, 
                    int size, const ColorRGBA& color);
    
    // Default fonts
    bool load_default_fonts();
    
    // Access to FreeType library (for FontFace)
    FT_Library get_ft_library() const { return ft_library_; }
    
    // Texture atlas management
    bool try_add_to_atlas(GlyphInfo& glyph, const unsigned char* bitmap_data, 
                         int width, int height);
    
private:
    FT_Library ft_library_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<FontFace>> fonts_;
    bool initialized_ = false;
    
    // Text measurement cache
    TextMeasurementCache measurement_cache_;
    
    // Texture atlas for small glyphs (optional optimization)
    struct TextureAtlas {
        unsigned int texture_id = 0;
        int width = 2048;
        int height = 2048;
        int current_x = 0;
        int current_y = 0;
        int row_height = 0;
    };
    std::unique_ptr<TextureAtlas> atlas_;
    
    // Shader for text rendering
    unsigned int text_shader_program_ = 0;
    unsigned int projection_uniform_ = 0;
    unsigned int text_color_uniform_ = 0;
    unsigned int texture_uniform_ = 0;
    
    // VAO/VBO for batch rendering
    unsigned int text_vao_ = 0;
    unsigned int text_vbo_ = 0;
    
    // Debug flag to disable SDF rendering
    bool use_sdf_rendering_ = true;
    
    // Batch rendering data
    struct GlyphVertex {
        float x, y;      // Position
        float u, v;      // Texture coordinates
    };
    std::vector<GlyphVertex> vertex_batch_;
    static constexpr size_t MAX_BATCH_SIZE = 1000; // Max glyphs per batch
    
    void create_text_shader();
    void setup_render_buffers();
    void create_texture_atlas();
    void render_glyph(CanvasRenderer* renderer, const GlyphInfo& glyph, 
                     const Point2D& position, const ColorRGBA& color);
    void flush_batch(CanvasRenderer* renderer, unsigned int texture_id, const ColorRGBA& color);
    void add_glyph_to_batch(const GlyphInfo& glyph, const Point2D& position);
};

// Global font system instance
extern FontSystem* g_font_system;

} // namespace voxel_canvas