/*
 * Copyright (C) 2024 Voxelux
 * 
 * Icon Loading and Rendering System
 * Handles loading and rendering of UI icons from SVG or PNG files
 */

#pragma once

#include "canvas_core.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

// Forward declarations for NanoSVG
struct NSVGimage;
struct NSVGrasterizer;

// OpenGL types
typedef unsigned int GLuint;

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;

// Icon sizes are now handled by ScaledTheme
// Use theme.icon_size_small(), theme.icon_size_medium(), etc.

/**
 * Represents a single icon asset that can be rendered at different sizes
 */
class IconAsset {
public:
    IconAsset(const std::string& name);
    ~IconAsset();
    
    // Load icon from file
    bool load_from_file(const std::string& filepath);
    bool load_from_svg(const std::string& svg_path);
    bool load_from_png(const std::string& png_path);
    
    // Load icon from memory
    bool load_from_memory(const uint8_t* data, size_t size, bool is_svg = false);
    
    // Render the icon - size must be pre-scaled from ScaledTheme
    void render(CanvasRenderer* renderer, const Point2D& position, float scaled_size, const ColorRGBA& tint = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Pre-render at specific size for faster display
    void prerender_at_size(CanvasRenderer* renderer, float size);
    
    // Clear all cached sizes (useful when DPI changes)
    void clear_cache();
    
    // Get icon properties
    const std::string& get_name() const { return name_; }
    bool is_loaded() const { return texture_id_ != 0; }
    Point2D get_size() const { return Point2D(width_, height_); }
    
private:
    std::string name_;
    unsigned int texture_id_ = 0;  // Current active texture
    float width_ = 0;
    float height_ = 0;
    bool is_svg_ = false;
    
    // Multi-size texture cache for optimal rendering
    struct SizeCache {
        GLuint texture_id = 0;
        int size = 0;
        std::chrono::steady_clock::time_point last_used;
    };
    std::unordered_map<int, SizeCache> size_cache_;  // Key: pixel size
    static constexpr size_t MAX_CACHED_SIZES = 8;  // Limit cache size
    
    // SVG data for re-rasterization at different sizes
    std::vector<uint8_t> svg_data_;
    NSVGimage* svg_image_ = nullptr;
    NSVGrasterizer* svg_rasterizer_ = nullptr;
    
    // Original SVG dimensions
    float original_width_ = 0;
    float original_height_ = 0;
    
    // Last rendered size for cache invalidation
    int last_rendered_size_ = 0;
    
    // Rasterize SVG at specific size
    bool rasterize_svg(int target_size);
    
    // Create OpenGL texture from pixel data
    bool create_texture(const uint8_t* pixels, int width, int height, int channels);
    
    // Get or create texture for specific size
    GLuint get_texture_for_size(int size);
    
    // Clean up old cached sizes
    void cleanup_old_caches();
};

/**
 * Texture atlas for efficient icon rendering
 */
class IconAtlas {
public:
    struct AtlasEntry {
        Rect2D texture_coords;  // UV coordinates in atlas
        float width;            // Width in pixels
        float height;           // Height in pixels
        uint32_t last_used;     // Frame number for LRU
    };
    
    IconAtlas(int width = 2048, int height = 2048);
    ~IconAtlas();
    
    // Add icon to atlas, returns UV coordinates
    bool add_icon(const std::string& key, const uint8_t* pixels, int width, int height);
    
    // Get UV coordinates for icon
    const AtlasEntry* get_entry(const std::string& key) const;
    
    // Get OpenGL texture ID
    unsigned int get_texture_id() const { return texture_id_; }
    
    // Clear atlas
    void clear();
    
    // Evict least recently used entries if needed
    void evict_lru(size_t target_count);
    
private:
    unsigned int texture_id_ = 0;
    int atlas_width_;
    int atlas_height_;
    std::unordered_map<std::string, AtlasEntry> entries_;
    
    // Rectangle packing for atlas
    int next_x_ = 0;
    int next_y_ = 0;
    int row_height_ = 0;
};

/**
 * Icon cache entry for size-specific rasterizations
 */
struct CachedIcon {
    unsigned int texture_id = 0;
    int width = 0;
    int height = 0;
    std::chrono::steady_clock::time_point last_used;
};

/**
 * Icon manager that handles loading and caching of icons
 */
class IconSystem {
public:
    IconSystem();
    ~IconSystem();
    
    // Initialize the icon system
    bool initialize(const std::string& icon_directory = "assets/icons/");
    void shutdown();
    
    // Load icons
    bool load_icon(const std::string& name, const std::string& filepath);
    bool load_icon_set(const std::string& set_name, const std::string& directory);
    
    // Get icon by name
    IconAsset* get_icon(const std::string& name);
    const IconAsset* get_icon(const std::string& name) const;
    
    // Check if icon exists
    bool has_icon(const std::string& name) const;
    
    // Preload common UI icons
    void load_default_icons();
    
    // Icon rendering helpers - size must be pre-scaled from ScaledTheme
    void render_icon(CanvasRenderer* renderer, const std::string& icon_name, 
                     const Point2D& position, float scaled_size, 
                     const ColorRGBA& tint = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Get singleton instance
    static IconSystem& get_instance();
    
    // Get all icons for batch operations
    const std::unordered_map<std::string, std::unique_ptr<IconAsset>>& get_all_icons() const { return icons_; }
    
    // Clear all icon caches (useful when DPI changes)
    void clear_all_caches();
    
private:
    std::unordered_map<std::string, std::unique_ptr<IconAsset>> icons_;
    std::string icon_directory_;
    bool initialized_ = false;
    
    // Cache for different icon sizes
    // Key: icon_name + "_" + size
    std::unordered_map<std::string, CachedIcon> size_cache_;
    
    // Maximum cache size in MB
    static constexpr size_t MAX_CACHE_SIZE_MB = 32;
    
    // Load built-in icons from embedded data
    void load_builtin_icons();
    
    // Helper to determine file type
    bool is_svg_file(const std::string& filepath) const;
    bool is_png_file(const std::string& filepath) const;
    
    // Cache management
    void evict_old_cache_entries();
    std::string make_cache_key(const std::string& icon_name, int size) const;
};

/**
 * Icon button - a button that displays an icon
 */
class IconButton {
public:
    // Icon size must be pre-scaled from ScaledTheme
    IconButton(const std::string& icon_name, const Rect2D& bounds, float scaled_icon_size = 24.0f);
    
    void render(CanvasRenderer* renderer);
    bool handle_event(const InputEvent& event);
    
    // State
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_selected(bool selected) { selected_ = selected; }
    void set_tooltip(const std::string& tooltip) { tooltip_ = tooltip; }
    
    // Appearance
    void set_icon(const std::string& icon_name) { icon_name_ = icon_name; }
    void set_icon_size(float scaled_size) { scaled_icon_size_ = scaled_size; }
    void set_tint(const ColorRGBA& tint) { icon_tint_ = tint; }
    
    // Callbacks
    using ClickCallback = std::function<void()>;
    void set_click_callback(ClickCallback callback) { click_callback_ = callback; }
    
    // Bounds
    void set_bounds(const Rect2D& bounds) { bounds_ = bounds; }
    Rect2D get_bounds() const { return bounds_; }
    
private:
    std::string icon_name_;
    Rect2D bounds_;
    float scaled_icon_size_;
    ColorRGBA icon_tint_;
    
    bool enabled_ = true;
    bool selected_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    
    std::string tooltip_;
    ClickCallback click_callback_;
};

/**
 * Common UI icon names
 */
namespace Icons {
    // File operations
    constexpr const char* NEW_FILE = "new_file";
    constexpr const char* OPEN_FILE = "open_file";
    constexpr const char* SAVE_FILE = "save_file";
    constexpr const char* CLOSE_FILE = "close_file";
    
    // Edit operations
    constexpr const char* UNDO = "undo";
    constexpr const char* REDO = "redo";
    constexpr const char* CUT = "cut";
    constexpr const char* COPY = "copy";
    constexpr const char* PASTE = "paste";
    constexpr const char* DELETE = "delete";
    
    // View operations
    constexpr const char* ZOOM_IN = "zoom_in";
    constexpr const char* ZOOM_OUT = "zoom_out";
    constexpr const char* ZOOM_FIT = "zoom_fit";
    constexpr const char* FULLSCREEN = "fullscreen";
    
    // Tools
    constexpr const char* SELECT = "select";
    constexpr const char* PAINT = "paint";
    constexpr const char* ERASE = "erase";
    constexpr const char* FILL = "fill";
    constexpr const char* PICK = "pick";
    constexpr const char* MOVE = "move";
    constexpr const char* ROTATE = "rotate";
    constexpr const char* SCALE = "scale";
    
    // Navigation
    constexpr const char* ARROW_LEFT = "arrow_left";
    constexpr const char* ARROW_RIGHT = "arrow_right";
    constexpr const char* ARROW_UP = "arrow_up";
    constexpr const char* ARROW_DOWN = "arrow_down";
    
    // UI elements
    constexpr const char* MENU = "menu";
    constexpr const char* SETTINGS = "settings";
    constexpr const char* HELP = "help";
    constexpr const char* INFO = "info";
    constexpr const char* WARNING = "warning";
    constexpr const char* ERROR = "error";
    
    // Dock/Panel
    constexpr const char* DOCK_LEFT = "dock_left";
    constexpr const char* DOCK_RIGHT = "dock_right";
    constexpr const char* DOCK_BOTTOM = "dock_bottom";
    constexpr const char* EXPAND = "expand";
    constexpr const char* COLLAPSE = "collapse";
    
    // Playback
    constexpr const char* PLAY = "play";
    constexpr const char* PAUSE = "pause";
    constexpr const char* STOP = "stop";
    constexpr const char* FORWARD = "forward";
    constexpr const char* BACKWARD = "backward";
}

} // namespace voxel_canvas