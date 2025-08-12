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
    
    // Try to add to atlas if glyph fits
    if (g->bitmap.width > 0 && g->bitmap.rows > 0) {
        // Check if we should use atlas (for smaller glyphs)
        bool use_atlas = g_font_system && g_font_system->try_add_to_atlas(glyph, g->bitmap.buffer, 
                                                                         g->bitmap.width, g->bitmap.rows);
        
        if (!use_atlas) {
            // Create individual texture for large glyphs
            glGenTextures(1, &glyph.texture_id);
            glBindTexture(GL_TEXTURE_2D, glyph.texture_id);
            
            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            // Upload glyph bitmap
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                        g->bitmap.width, g->bitmap.rows, 
                        0, GL_RED, GL_UNSIGNED_BYTE,
                        g->bitmap.buffer);
            
            glBindTexture(GL_TEXTURE_2D, 0);
            glyph.in_atlas = false;
        }
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
    
    // Setup VAO/VBO for batch rendering
    setup_render_buffers();
    
    // Create texture atlas
    create_texture_atlas();
    
    // Reserve space for batch vertices
    vertex_batch_.reserve(MAX_BATCH_SIZE * 6); // 6 vertices per glyph (2 triangles)
    
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
    
    // Destroy VAO/VBO
    if (text_vao_) {
        glDeleteVertexArrays(1, &text_vao_);
        text_vao_ = 0;
    }
    if (text_vbo_) {
        glDeleteBuffers(1, &text_vbo_);
        text_vbo_ = 0;
    }
    
    // Destroy shader
    if (text_shader_program_) {
        glDeleteProgram(text_shader_program_);
        text_shader_program_ = 0;
    }
    
    // Destroy atlas texture if it exists
    if (atlas_ && atlas_->texture_id) {
        glDeleteTextures(1, &atlas_->texture_id);
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

// UTF-8 decoding helper
static unsigned int decode_utf8(const std::string& text, size_t& index) {
    unsigned char c = text[index];
    unsigned int codepoint = 0;
    
    if ((c & 0x80) == 0) {
        // ASCII (1 byte)
        codepoint = c;
        index += 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (index + 1 < text.length()) {
            codepoint = ((c & 0x1F) << 6) | (text[index + 1] & 0x3F);
            index += 2;
        } else {
            index += 1; // Invalid sequence
        }
    } else if ((c & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (index + 2 < text.length()) {
            codepoint = ((c & 0x0F) << 12) | 
                       ((text[index + 1] & 0x3F) << 6) | 
                       (text[index + 2] & 0x3F);
            index += 3;
        } else {
            index += 1; // Invalid sequence
        }
    } else if ((c & 0xF8) == 0xF0) {
        // 4-byte sequence
        if (index + 3 < text.length()) {
            codepoint = ((c & 0x07) << 18) | 
                       ((text[index + 1] & 0x3F) << 12) | 
                       ((text[index + 2] & 0x3F) << 6) | 
                       (text[index + 3] & 0x3F);
            index += 4;
        } else {
            index += 1; // Invalid sequence
        }
    } else {
        // Invalid UTF-8 start byte
        index += 1;
    }
    
    return codepoint;
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
        unsigned int codepoint = decode_utf8(text, i);
        
        GlyphInfo* glyph = font->get_glyph(codepoint, size);
        if (glyph) {
            width += glyph->advance;
        }
    }
    
    return Point2D(width, height);
}

void FontSystem::render_text(CanvasRenderer* renderer, const std::string& text, 
                            const Point2D& position, const std::string& font_name, 
                            int size, const ColorRGBA& color) {
    FontFace* font = get_font(font_name);
    if (!font || text.empty()) {
        return;
    }
    
    // Save OpenGL state
    GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    GLint blend_src, blend_dst;
    glGetIntegerv(GL_BLEND_SRC, &blend_src);
    glGetIntegerv(GL_BLEND_DST, &blend_dst);
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear batch
    vertex_batch_.clear();
    
    float x = position.x;
    float y = position.y;
    float baseline_y = y + font->get_ascender(size);
    
    // Group glyphs by texture for efficient batching
    struct TextureBatch {
        unsigned int texture_id;
        std::vector<std::pair<GlyphInfo*, Point2D>> glyphs;
    };
    std::vector<TextureBatch> texture_batches;
    
    // Collect all glyphs and group by texture
    for (size_t i = 0; i < text.length(); ) {
        unsigned int codepoint = decode_utf8(text, i);
        
        GlyphInfo* glyph = font->get_glyph(codepoint, size);
        if (glyph && glyph->texture_id) {
            // Calculate glyph position
            Point2D glyph_pos(
                x + glyph->bearing.x,
                baseline_y - glyph->bearing.y
            );
            
            // Find or create batch for this texture
            bool found = false;
            for (auto& batch : texture_batches) {
                if (batch.texture_id == glyph->texture_id) {
                    batch.glyphs.push_back({glyph, glyph_pos});
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                TextureBatch new_batch;
                new_batch.texture_id = glyph->texture_id;
                new_batch.glyphs.push_back({glyph, glyph_pos});
                texture_batches.push_back(new_batch);
            }
        }
        
        if (glyph) {
            x += glyph->advance;
        }
    }
    
    // Render each texture batch
    for (const auto& batch : texture_batches) {
        vertex_batch_.clear();
        
        // Add all glyphs for this texture to the vertex batch
        for (const auto& [glyph, pos] : batch.glyphs) {
            add_glyph_to_batch(*glyph, pos);
        }
        
        // Flush this texture's batch
        if (!vertex_batch_.empty()) {
            flush_batch(renderer, batch.texture_id, color);
        }
    }
    
    // Restore OpenGL state
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(blend_src, blend_dst);
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
    // Text rendering shader with proper projection
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
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

void main() {
    float alpha = texture(text, TexCoords).r;
    FragColor = vec4(textColor.rgb, textColor.a * alpha);
}
)";
    
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);
    
    // Check vertex shader compilation
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        std::cerr << "Text vertex shader compilation failed: " << info_log << std::endl;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        std::cerr << "Text fragment shader compilation failed: " << info_log << std::endl;
    }
    
    // Link program
    text_shader_program_ = glCreateProgram();
    glAttachShader(text_shader_program_, vertex_shader);
    glAttachShader(text_shader_program_, fragment_shader);
    glLinkProgram(text_shader_program_);
    
    // Check program linking
    glGetProgramiv(text_shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(text_shader_program_, 512, nullptr, info_log);
        std::cerr << "Text shader program linking failed: " << info_log << std::endl;
    }
    
    // Get uniform locations
    projection_uniform_ = glGetUniformLocation(text_shader_program_, "projection");
    text_color_uniform_ = glGetUniformLocation(text_shader_program_, "textColor");
    texture_uniform_ = glGetUniformLocation(text_shader_program_, "text");
    
    // Clean up
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void FontSystem::setup_render_buffers() {
    // Create VAO and VBO for text rendering
    glGenVertexArrays(1, &text_vao_);
    glGenBuffers(1, &text_vbo_);
    
    glBindVertexArray(text_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
    
    // Allocate buffer for batch rendering (dynamic draw)
    glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphVertex) * MAX_BATCH_SIZE * 6, nullptr, GL_DYNAMIC_DRAW);
    
    // Setup vertex attributes (position + tex coords)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphVertex), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void FontSystem::add_glyph_to_batch(const GlyphInfo& glyph, const Point2D& position) {
    float x = position.x;
    float y = position.y;
    float w = glyph.size.x;
    float h = glyph.size.y;
    
    // Calculate texture coordinates
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 1.0f, v1 = 1.0f;
    
    // If using atlas, adjust UV coordinates
    if (glyph.in_atlas) {
        u0 = glyph.uv_rect.x;
        v0 = glyph.uv_rect.y;
        u1 = glyph.uv_rect.x + glyph.uv_rect.width;
        v1 = glyph.uv_rect.y + glyph.uv_rect.height;
    }
    
    // Add two triangles for the glyph quad
    // Triangle 1
    vertex_batch_.push_back({x,     y + h, u0, v1});
    vertex_batch_.push_back({x,     y,     u0, v0});
    vertex_batch_.push_back({x + w, y,     u1, v0});
    
    // Triangle 2
    vertex_batch_.push_back({x,     y + h, u0, v1});
    vertex_batch_.push_back({x + w, y,     u1, v0});
    vertex_batch_.push_back({x + w, y + h, u1, v1});
}

void FontSystem::flush_batch(CanvasRenderer* renderer, unsigned int texture_id, const ColorRGBA& color) {
    if (vertex_batch_.empty()) {
        return;
    }
    
    // Use text shader
    glUseProgram(text_shader_program_);
    
    // Set uniforms
    if (renderer) {
        // Get projection matrix from renderer
        float projection[16];
        renderer->get_projection_matrix(projection);
        glUniformMatrix4fv(projection_uniform_, 1, GL_FALSE, projection);
    } else {
        // Create orthographic projection for screen space
        float width = 1920.0f;  // Default screen width
        float height = 1080.0f; // Default screen height
        float projection[16] = {
            2.0f/width,  0.0f,         0.0f, 0.0f,
            0.0f,        -2.0f/height, 0.0f, 0.0f,
            0.0f,        0.0f,         -1.0f, 0.0f,
            -1.0f,       1.0f,         0.0f, 1.0f
        };
        glUniformMatrix4fv(projection_uniform_, 1, GL_FALSE, projection);
    }
    
    glUniform4f(text_color_uniform_, color.r, color.g, color.b, color.a);
    glUniform1i(texture_uniform_, 0);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // Upload vertex data
    glBindVertexArray(text_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphVertex) * vertex_batch_.size(), vertex_batch_.data());
    
    // Draw
    glDrawArrays(GL_TRIANGLES, 0, vertex_batch_.size());
    
    // Cleanup
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    // Clear batch
    vertex_batch_.clear();
}

void FontSystem::render_glyph(CanvasRenderer* renderer, const GlyphInfo& glyph, 
                             const Point2D& position, const ColorRGBA& color) {
    if (!glyph.texture_id || glyph.size.x == 0 || glyph.size.y == 0) {
        return; // Skip empty glyphs (like spaces)
    }
    
    // Add to batch
    add_glyph_to_batch(glyph, position);
    
    // For now, flush immediately (later we can batch multiple glyphs)
    flush_batch(renderer, glyph.texture_id, color);
}

void FontSystem::create_texture_atlas() {
    // Create texture atlas for glyph packing
    atlas_ = std::make_unique<TextureAtlas>();
    
    // Create the atlas texture
    glGenTextures(1, &atlas_->texture_id);
    glBindTexture(GL_TEXTURE_2D, atlas_->texture_id);
    
    // Set texture parameters for crisp text rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Allocate empty texture (single channel for glyphs)
    std::vector<unsigned char> empty_data(atlas_->width * atlas_->height, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 
                atlas_->width, atlas_->height, 
                0, GL_RED, GL_UNSIGNED_BYTE, 
                empty_data.data());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Reset packing position
    atlas_->current_x = 1; // Leave 1 pixel border
    atlas_->current_y = 1;
    atlas_->row_height = 0;
}

bool FontSystem::try_add_to_atlas(GlyphInfo& glyph, const unsigned char* bitmap_data, 
                                  int width, int height) {
    if (!atlas_ || !atlas_->texture_id) {
        return false;
    }
    
    // Don't use atlas for very large glyphs
    const int MAX_GLYPH_SIZE = 128;
    if (width > MAX_GLYPH_SIZE || height > MAX_GLYPH_SIZE) {
        return false;
    }
    
    // Add padding between glyphs to prevent bleeding
    const int padding = 2;
    int padded_width = width + padding;
    int padded_height = height + padding;
    
    // Check if we need to move to next row
    if (atlas_->current_x + padded_width > atlas_->width) {
        atlas_->current_x = 1;
        atlas_->current_y += atlas_->row_height + padding;
        atlas_->row_height = 0;
    }
    
    // Check if we have space in the atlas
    if (atlas_->current_y + padded_height > atlas_->height) {
        // Atlas is full - could implement atlas resizing or multiple atlases here
        std::cerr << "Texture atlas full, using individual texture for glyph" << std::endl;
        return false;
    }
    
    // Upload glyph to atlas
    glBindTexture(GL_TEXTURE_2D, atlas_->texture_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 
                   atlas_->current_x, atlas_->current_y,
                   width, height,
                   GL_RED, GL_UNSIGNED_BYTE,
                   bitmap_data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Set glyph UV coordinates (normalized)
    glyph.uv_rect.x = static_cast<float>(atlas_->current_x) / atlas_->width;
    glyph.uv_rect.y = static_cast<float>(atlas_->current_y) / atlas_->height;
    glyph.uv_rect.width = static_cast<float>(width) / atlas_->width;
    glyph.uv_rect.height = static_cast<float>(height) / atlas_->height;
    
    // Mark as in atlas and set texture ID
    glyph.in_atlas = true;
    glyph.texture_id = atlas_->texture_id;
    
    // Update packing position
    atlas_->current_x += padded_width;
    atlas_->row_height = std::max(atlas_->row_height, padded_height);
    
    return true;
}

} // namespace voxel_canvas