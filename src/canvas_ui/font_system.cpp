/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Font system implementation using FreeType2
 */

#include "canvas_ui/font_system.h"
#include "canvas_ui/canvas_renderer.h"
#include "glad/gl.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <fstream>
#include <algorithm>

namespace voxel_canvas {

// Global font system instance
FontSystem* g_font_system = nullptr;

// FontFace implementation

FontFace::FontFace(const std::string& path, int size)
    : font_path_(path)
    , default_size_(size) {
    // Extract font name from path
    size_t last_slash = path.find_last_of("/\\");
    size_t last_dot = path.find_last_of(".");
    if (last_slash != std::string::npos && last_dot != std::string::npos) {
        font_name_ = path.substr(last_slash + 1, last_dot - last_slash - 1);
    } else {
        font_name_ = "Unknown";
    }
}

FontFace::~FontFace() {
    unload();
}

bool FontFace::load() {
    if (face_) {
        return true; // Already loaded
    }
    
    FT_Library ft = g_font_system->get_ft_library();
    if (!ft) {
        std::cerr << "FreeType library not initialized" << std::endl;
        return false;
    }
    
    // Load font face
    FT_Error error = FT_New_Face(ft, font_path_.c_str(), 0, &face_);
    if (error) {
        std::cerr << "Failed to load font: " << font_path_ << " (error: " << error << ")" << std::endl;
        return false;
    }
    
    // Set default size
    FT_Set_Pixel_Sizes(face_, 0, default_size_);
    
    return true;
}

void FontFace::unload() {
    if (face_) {
        FT_Done_Face(face_);
        face_ = nullptr;
    }
    
    // Clear all cached glyphs
    for (auto& [size, cache] : size_caches_) {
        for (auto& [codepoint, glyph] : cache.glyphs) {
            if (glyph.texture_id && !glyph.in_atlas) {
                glDeleteTextures(1, &glyph.texture_id);
            }
        }
    }
    size_caches_.clear();
}

GlyphInfo* FontFace::get_glyph(unsigned int codepoint, int size) {
    // Check if we have this size cached
    auto size_it = size_caches_.find(size);
    if (size_it == size_caches_.end()) {
        // Create new size cache
        SizeCache cache;
        cache.pixel_size = size;
        
        // Set font size and get metrics
        FT_Set_Pixel_Sizes(face_, 0, size);
        cache.line_height = face_->size->metrics.height >> 6;
        cache.ascender = face_->size->metrics.ascender >> 6;
        cache.descender = face_->size->metrics.descender >> 6;
        
        size_caches_[size] = cache;
        size_it = size_caches_.find(size);
    }
    
    // Check if glyph is cached
    auto& cache = size_it->second;
    auto glyph_it = cache.glyphs.find(codepoint);
    if (glyph_it != cache.glyphs.end()) {
        return &glyph_it->second;
    }
    
    // Load the glyph
    return load_glyph(codepoint, size);
}

GlyphInfo* FontFace::load_glyph(unsigned int codepoint, int size) {
    if (!face_) {
        return nullptr;
    }
    
    // Set size
    FT_Set_Pixel_Sizes(face_, 0, size);
    
    // Load glyph
    if (FT_Load_Char(face_, codepoint, FT_LOAD_RENDER)) {
        std::cerr << "Failed to load glyph for codepoint: " << codepoint << std::endl;
        return nullptr;
    }
    
    FT_GlyphSlot g = face_->glyph;
    
    // Create glyph info
    GlyphInfo glyph;
    glyph.size = Point2D(g->bitmap.width, g->bitmap.rows);
    glyph.bearing = Point2D(g->bitmap_left, g->bitmap_top);
    glyph.advance = g->advance.x >> 6; // Convert from 26.6 fixed point
    
    // Create texture for glyph
    if (g->bitmap.width > 0 && g->bitmap.rows > 0) {
        glGenTextures(1, &glyph.texture_id);
        glBindTexture(GL_TEXTURE_2D, glyph.texture_id);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, 0x2802, 0x812F); // GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, 0x2803, 0x812F); // GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE  
        glTexParameteri(GL_TEXTURE_2D, 0x2801, 0x2601); // GL_TEXTURE_MIN_FILTER, GL_LINEAR
        glTexParameteri(GL_TEXTURE_2D, 0x2800, 0x2601); // GL_TEXTURE_MAG_FILTER, GL_LINEAR
        
        // Upload glyph bitmap
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, 0x1903,  // GL_RED
                    g->bitmap.width, g->bitmap.rows, 
                    0, 0x1903, GL_UNSIGNED_BYTE,   // GL_RED
                    g->bitmap.buffer);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    // Store in cache
    size_caches_[size].glyphs[codepoint] = glyph;
    return &size_caches_[size].glyphs[codepoint];
}

float FontFace::get_line_height(int size) const {
    auto it = size_caches_.find(size);
    if (it != size_caches_.end()) {
        return it->second.line_height;
    }
    
    // Calculate from face metrics
    if (face_) {
        FT_Set_Pixel_Sizes(const_cast<FT_Face>(face_), 0, size);
        return face_->size->metrics.height >> 6;
    }
    
    return size * 1.2f; // Fallback
}

float FontFace::get_ascender(int size) const {
    auto it = size_caches_.find(size);
    if (it != size_caches_.end()) {
        return it->second.ascender;
    }
    
    if (face_) {
        FT_Set_Pixel_Sizes(const_cast<FT_Face>(face_), 0, size);
        return face_->size->metrics.ascender >> 6;
    }
    
    return size * 0.8f; // Fallback
}

float FontFace::get_descender(int size) const {
    auto it = size_caches_.find(size);
    if (it != size_caches_.end()) {
        return it->second.descender;
    }
    
    if (face_) {
        FT_Set_Pixel_Sizes(const_cast<FT_Face>(face_), 0, size);
        return face_->size->metrics.descender >> 6;
    }
    
    return size * 0.2f; // Fallback
}

// FontSystem implementation

FontSystem::FontSystem() {
}

FontSystem::~FontSystem() {
    shutdown();
}

bool FontSystem::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize FreeType
    FT_Error error = FT_Init_FreeType(&ft_library_);
    if (error) {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
        return false;
    }
    
    // Create text rendering shader
    create_text_shader();
    
    // Set global instance
    g_font_system = this;
    
    initialized_ = true;
    return true;
}

void FontSystem::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Clear all fonts
    fonts_.clear();
    
    // Destroy shader
    if (text_shader_program_) {
        glDeleteProgram(text_shader_program_);
        text_shader_program_ = 0;
    }
    
    // Shutdown FreeType
    if (ft_library_) {
        FT_Done_FreeType(ft_library_);
        ft_library_ = nullptr;
    }
    
    g_font_system = nullptr;
    initialized_ = false;
}

bool FontSystem::load_font(const std::string& name, const std::string& path, int default_size) {
    // Check if already loaded
    if (fonts_.find(name) != fonts_.end()) {
        return true;
    }
    
    // Create and load font face
    auto font = std::make_unique<FontFace>(path, default_size);
    if (!font->load()) {
        return false;
    }
    
    fonts_[name] = std::move(font);
    return true;
}

FontFace* FontSystem::get_font(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Point2D FontSystem::measure_text(const std::string& text, const std::string& font_name, int size) {
    FontFace* font = get_font(font_name);
    if (!font) {
        return Point2D(0, 0);
    }
    
    float width = 0;
    float height = font->get_line_height(size);
    
    // Iterate through UTF-8 text
    for (size_t i = 0; i < text.length(); ) {
        // Simple ASCII for now, proper UTF-8 decoding needed
        unsigned int codepoint = static_cast<unsigned char>(text[i]);
        
        GlyphInfo* glyph = font->get_glyph(codepoint, size);
        if (glyph) {
            width += glyph->advance;
        }
        
        i++;
    }
    
    return Point2D(width, height);
}

void FontSystem::render_text(CanvasRenderer* renderer, const std::string& text, 
                            const Point2D& position, const std::string& font_name, 
                            int size, const ColorRGBA& color) {
    FontFace* font = get_font(font_name);
    if (!font) {
        return;
    }
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float x = position.x;
    float y = position.y;
    
    // Render each character
    for (size_t i = 0; i < text.length(); ) {
        // Simple ASCII for now
        unsigned int codepoint = static_cast<unsigned char>(text[i]);
        
        GlyphInfo* glyph = font->get_glyph(codepoint, size);
        if (glyph) {
            if (glyph->texture_id) {
                // Render glyph quad
                Point2D glyph_pos(x + glyph->bearing.x, y - (glyph->size.y - glyph->bearing.y));
                render_glyph(renderer, *glyph, glyph_pos, color);
            }
            x += glyph->advance;
        }
        
        i++;
    }
}

bool FontSystem::load_default_fonts() {
    // Load Inter Variable font (supports multiple weights)
    std::string font_path = "assets/fonts/InterVariable.ttf";
    
    // Load as default font with different names for different use cases
    if (!load_font("Inter", font_path, 14)) {
        std::cerr << "Failed to load Inter Variable font at: " << font_path << std::endl;
        return false;
    }
    
    // For now, use same variable font for different weights
    // Later we can implement font variation axes for weight control
    if (!load_font("Inter-Medium", font_path, 16)) {
        std::cerr << "Failed to load Inter for medium weight" << std::endl;
    }
    
    if (!load_font("Inter-Bold", font_path, 14)) {
        std::cerr << "Failed to load Inter for bold weight" << std::endl;
    }
    
    return true;
}

void FontSystem::create_text_shader() {
    // Simple text rendering shader
    const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";
    
    const char* fragment_shader_src = R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec4 textColor;

void main() {
    float alpha = texture(text, TexCoords).r;
    color = vec4(textColor.rgb, textColor.a * alpha);
}
)";
    
    // Compile shaders
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);
    
    // Link program
    text_shader_program_ = glCreateProgram();
    glAttachShader(text_shader_program_, vertex_shader);
    glAttachShader(text_shader_program_, fragment_shader);
    glLinkProgram(text_shader_program_);
    
    // Clean up
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void FontSystem::render_glyph(CanvasRenderer* renderer, const GlyphInfo& glyph, 
                             const Point2D& position, const ColorRGBA& color) {
    if (!glyph.texture_id || glyph.size.x == 0 || glyph.size.y == 0) {
        return; // Skip empty glyphs (like spaces)
    }
    
    // For now, temporarily disable text rendering to avoid white rectangles
    // until we implement proper glyph texture rendering
    
    // TODO: Implement proper textured quad rendering for glyphs
    // This requires either:
    // 1. A dedicated text shader that samples from the glyph texture
    // 2. Or extending the UI shader to handle single-channel textures
    
    // Skip rendering until we have proper glyph shader
    return;
}

} // namespace voxel_canvas