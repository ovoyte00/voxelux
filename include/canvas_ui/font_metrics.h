/*
 * Copyright (C) 2024 Voxelux
 * 
 * Font Metrics System - Accurate text measurement with kerning and shaping
 */

#pragma once

#include "canvas_core.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Forward declarations
typedef struct FT_FaceRec_* FT_Face;

namespace voxel_canvas {

/**
 * Shaped glyph with positioning information
 */
struct ShapedGlyph {
    uint32_t glyph_id;      // Font's internal glyph ID
    uint32_t codepoint;     // Original Unicode codepoint
    float x_advance;        // How far to advance horizontally
    float y_advance;        // For vertical text (usually 0)
    float x_offset;         // Kerning/positioning adjustment
    float y_offset;         // Vertical adjustment
};

/**
 * Result of text shaping and measurement
 */
struct TextMeasurement {
    float width;            // Total width of the text
    float height;           // Total height (ascender + descender)
    float ascender;         // Height above baseline
    float descender;        // Depth below baseline
    float line_gap;         // Recommended gap between lines
    std::vector<ShapedGlyph> glyphs;  // Positioned glyphs
};

/**
 * Font metrics at a specific size
 */
struct FontMetricsData {
    float units_per_em;     // Base unit size of font
    float ascender;         // Max height above baseline (in font units)
    float descender;        // Max depth below baseline (in font units)
    float line_gap;         // Recommended line spacing (in font units)
    float cap_height;       // Height of capital letters
    float x_height;         // Height of lowercase 'x'
    
    // Convert font units to pixels for a given size
    float to_pixels(float units, float font_size_px) const {
        return (units * font_size_px) / units_per_em;
    }
};

/**
 * Kerning pair information
 */
struct KerningPair {
    uint32_t left;          // Left glyph ID
    uint32_t right;         // Right glyph ID
    float adjustment;       // Kerning adjustment in font units
};

/**
 * Font metrics and text measurement system
 */
class FontMetrics {
public:
    FontMetrics();
    ~FontMetrics();
    
    // Initialize from FreeType face
    bool initialize(FT_Face face);
    
    // Measure text accurately with kerning
    TextMeasurement measure_text(const std::string& text, float font_size_px) const;
    
    // Get character advance width (includes kerning if prev_codepoint provided)
    float get_char_advance(uint32_t codepoint, float font_size_px, 
                          uint32_t prev_codepoint = 0) const;
    
    // Get font metrics at specific size
    float get_ascender(float font_size_px) const;
    float get_descender(float font_size_px) const;
    float get_line_height(float font_size_px) const;
    float get_cap_height(float font_size_px) const;
    float get_x_height(float font_size_px) const;
    
    // Shape text (handles ligatures, kerning, etc.)
    std::vector<ShapedGlyph> shape_text(const std::string& text, float font_size_px) const;
    
    // Get metrics data
    const FontMetricsData& get_metrics() const { return metrics_; }
    
private:
    FT_Face face_ = nullptr;
    FontMetricsData metrics_;
    
    // Glyph advance widths (in font units)
    std::unordered_map<uint32_t, float> glyph_advances_;
    
    // Kerning table (key is (left << 32) | right)
    std::unordered_map<uint64_t, float> kerning_table_;
    
    // Helper to get kerning between two glyphs
    float get_kerning(uint32_t left_glyph, uint32_t right_glyph) const;
    
    // Convert codepoint to glyph ID
    uint32_t codepoint_to_glyph(uint32_t codepoint) const;
    
    // Build kerning table from font
    void build_kerning_table();
    
    // Load glyph metrics
    void load_glyph_metrics();
};

/**
 * Text measurement cache for performance
 */
class TextMeasurementCache {
public:
    struct CacheKey {
        std::string text;
        std::string font_name;
        float font_size;
        
        bool operator==(const CacheKey& other) const {
            return text == other.text && 
                   font_name == other.font_name && 
                   font_size == other.font_size;
        }
    };
    
    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const {
            size_t h1 = std::hash<std::string>{}(key.text);
            size_t h2 = std::hash<std::string>{}(key.font_name);
            size_t h3 = std::hash<float>{}(key.font_size);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
    
    // Get or compute measurement
    TextMeasurement measure(const CacheKey& key, 
                           const std::function<TextMeasurement()>& compute);
    
    // Clear cache
    void clear();
    
    // Set max cache size
    void set_max_size(size_t max_size) { max_cache_size_ = max_size; }
    
private:
    std::unordered_map<CacheKey, TextMeasurement, CacheKeyHash> cache_;
    size_t max_cache_size_ = 1000;
    
    // LRU eviction
    void evict_oldest();
};

} // namespace voxel_canvas