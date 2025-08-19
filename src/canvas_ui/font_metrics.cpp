/*
 * Copyright (C) 2024 Voxelux
 * 
 * Font Metrics System Implementation
 */

#include "canvas_ui/font_metrics.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <cmath>

namespace voxel_canvas {

// FontMetrics implementation

FontMetrics::FontMetrics() {
}

FontMetrics::~FontMetrics() {
}

bool FontMetrics::initialize(FT_Face face) {
    if (!face) return false;
    
    face_ = face;
    
    // Extract font metrics
    metrics_.units_per_em = face->units_per_EM;
    metrics_.ascender = face->ascender;
    metrics_.descender = face->descender;
    metrics_.line_gap = face->height - (face->ascender - face->descender);
    
    // Get cap height and x-height from specific glyphs if available
    FT_UInt glyph_index = FT_Get_Char_Index(face, 'H');
    if (glyph_index && !FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE)) {
        metrics_.cap_height = face->glyph->metrics.height;
    } else {
        metrics_.cap_height = metrics_.ascender * 0.7f;  // Approximation
    }
    
    glyph_index = FT_Get_Char_Index(face, 'x');
    if (glyph_index && !FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE)) {
        metrics_.x_height = face->glyph->metrics.height;
    } else {
        metrics_.x_height = metrics_.cap_height * 0.5f;  // Approximation
    }
    
    // Load glyph metrics and kerning
    load_glyph_metrics();
    build_kerning_table();
    
    return true;
}

void FontMetrics::load_glyph_metrics() {
    if (!face_) return;
    
    // Load common ASCII characters and their advance widths
    for (uint32_t codepoint = 32; codepoint < 127; ++codepoint) {
        FT_UInt glyph_index = FT_Get_Char_Index(face_, codepoint);
        if (glyph_index) {
            if (!FT_Load_Glyph(face_, glyph_index, FT_LOAD_NO_SCALE)) {
                glyph_advances_[codepoint] = face_->glyph->metrics.horiAdvance;
            }
        }
    }
    
    // Also load common Unicode ranges if needed
    // TODO: Add more Unicode ranges as needed
}

void FontMetrics::build_kerning_table() {
    if (!face_ || !FT_HAS_KERNING(face_)) return;
    
    // Build kerning table for common character pairs
    for (uint32_t left = 32; left < 127; ++left) {
        FT_UInt left_glyph = FT_Get_Char_Index(face_, left);
        if (!left_glyph) continue;
        
        for (uint32_t right = 32; right < 127; ++right) {
            FT_UInt right_glyph = FT_Get_Char_Index(face_, right);
            if (!right_glyph) continue;
            
            FT_Vector delta;
            FT_Get_Kerning(face_, left_glyph, right_glyph, 
                          FT_KERNING_UNSCALED, &delta);
            
            if (delta.x != 0) {
                uint64_t key = (static_cast<uint64_t>(left_glyph) << 32) | right_glyph;
                kerning_table_[key] = delta.x;
            }
        }
    }
}

TextMeasurement FontMetrics::measure_text(const std::string& text, float font_size_px) const {
    TextMeasurement result;
    result.width = 0;
    result.ascender = get_ascender(font_size_px);
    result.descender = get_descender(font_size_px);
    result.height = result.ascender - result.descender;  // descender is negative
    result.line_gap = metrics_.to_pixels(metrics_.line_gap, font_size_px);
    
    // Shape the text and calculate total width
    result.glyphs = shape_text(text, font_size_px);
    
    for (const auto& glyph : result.glyphs) {
        result.width += glyph.x_advance;
    }
    
    return result;
}

std::vector<ShapedGlyph> FontMetrics::shape_text(const std::string& text, float font_size_px) const {
    std::vector<ShapedGlyph> shaped;
    if (!face_) return shaped;
    
    float scale = font_size_px / metrics_.units_per_em;
    uint32_t prev_glyph = 0;
    [[maybe_unused]] float x_pos = 0;
    
    for (size_t i = 0; i < text.length(); ++i) {
        // Simple UTF-8 to codepoint (handles ASCII for now)
        // TODO: Proper UTF-8 decoding for international text
        uint32_t codepoint = static_cast<unsigned char>(text[i]);
        uint32_t glyph_id = codepoint_to_glyph(codepoint);
        
        ShapedGlyph glyph;
        glyph.glyph_id = glyph_id;
        glyph.codepoint = codepoint;
        glyph.x_offset = 0;
        glyph.y_offset = 0;
        glyph.y_advance = 0;
        
        // Apply kerning if available
        if (prev_glyph != 0) {
            float kern = get_kerning(prev_glyph, glyph_id);
            if (kern != 0) {
                x_pos += kern * scale;
                glyph.x_offset = kern * scale;
            }
        }
        
        // Get advance width
        auto it = glyph_advances_.find(codepoint);
        if (it != glyph_advances_.end()) {
            glyph.x_advance = it->second * scale;
        } else {
            // Fallback for unknown glyphs
            glyph.x_advance = font_size_px * 0.5f;
        }
        
        shaped.push_back(glyph);
        x_pos += glyph.x_advance;
        prev_glyph = glyph_id;
    }
    
    return shaped;
}

float FontMetrics::get_char_advance(uint32_t codepoint, float font_size_px, 
                                   uint32_t prev_codepoint) const {
    float scale = font_size_px / metrics_.units_per_em;
    float advance = 0;
    
    // Get base advance
    auto it = glyph_advances_.find(codepoint);
    if (it != glyph_advances_.end()) {
        advance = it->second * scale;
    } else {
        advance = font_size_px * 0.5f;  // Fallback
    }
    
    // Add kerning if previous character provided
    if (prev_codepoint != 0) {
        uint32_t prev_glyph = codepoint_to_glyph(prev_codepoint);
        uint32_t curr_glyph = codepoint_to_glyph(codepoint);
        float kern = get_kerning(prev_glyph, curr_glyph);
        advance += kern * scale;
    }
    
    return advance;
}

float FontMetrics::get_kerning(uint32_t left_glyph, uint32_t right_glyph) const {
    uint64_t key = (static_cast<uint64_t>(left_glyph) << 32) | right_glyph;
    auto it = kerning_table_.find(key);
    return (it != kerning_table_.end()) ? it->second : 0;
}

uint32_t FontMetrics::codepoint_to_glyph(uint32_t codepoint) const {
    if (!face_) return 0;
    return FT_Get_Char_Index(face_, codepoint);
}

float FontMetrics::get_ascender(float font_size_px) const {
    return metrics_.to_pixels(metrics_.ascender, font_size_px);
}

float FontMetrics::get_descender(float font_size_px) const {
    return metrics_.to_pixels(metrics_.descender, font_size_px);
}

float FontMetrics::get_line_height(float font_size_px) const {
    return metrics_.to_pixels(metrics_.ascender - metrics_.descender + metrics_.line_gap, 
                              font_size_px);
}

float FontMetrics::get_cap_height(float font_size_px) const {
    return metrics_.to_pixels(metrics_.cap_height, font_size_px);
}

float FontMetrics::get_x_height(float font_size_px) const {
    return metrics_.to_pixels(metrics_.x_height, font_size_px);
}

// TextMeasurementCache implementation

TextMeasurement TextMeasurementCache::measure(const CacheKey& key,
                                             const std::function<TextMeasurement()>& compute) {
    // Check cache
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    
    // Compute and cache
    TextMeasurement result = compute();
    
    // Evict if cache is too large
    if (cache_.size() >= max_cache_size_) {
        evict_oldest();
    }
    
    cache_[key] = result;
    return result;
}

void TextMeasurementCache::clear() {
    cache_.clear();
}

void TextMeasurementCache::evict_oldest() {
    // Simple eviction: remove first element (not truly LRU)
    // For production, implement proper LRU with timestamp or access order
    if (!cache_.empty()) {
        cache_.erase(cache_.begin());
    }
}

} // namespace voxel_canvas