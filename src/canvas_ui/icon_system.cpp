/*
 * Copyright (C) 2024 Voxelux
 * 
 * Implementation of Icon Loading and Rendering System
 */

#include "canvas_ui/icon_system.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/canvas_window.h"
#include <glad/gl.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

// For PNG loading, we'll use stb_image (header-only library)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"

// For SVG rasterization using NanoSVG
// NanoSVG - zlib License (no attribution required)
#define NANOSVG_IMPLEMENTATION
#include "../../lib/third_party/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION  
#include "../../lib/third_party/nanosvgrast.h"

// For texture atlas packing
// rectpack2D - MIT License, Copyright (c) 2016 Patryk Nadrowski
#include "../../lib/third_party/rectpack2D/finders_interface.h"

namespace voxel_canvas {

// IconAsset implementation

IconAsset::IconAsset(const std::string& name)
    : name_(name) {
}

IconAsset::~IconAsset() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
    }
    
    // Clean up all cached size textures
    for (auto& [size, cache] : size_cache_) {
        if (cache.texture_id != 0) {
            glDeleteTextures(1, &cache.texture_id);
        }
    }
    
    // Clean up NanoSVG resources
    if (svg_image_) {
        nsvgDelete(svg_image_);
    }
    if (svg_rasterizer_) {
        nsvgDeleteRasterizer(svg_rasterizer_);
    }
}

bool IconAsset::load_from_file(const std::string& filepath) {
    // Check file extension
    std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "svg") {
        return load_from_svg(filepath);
    } else if (ext == "png" || ext == "jpg" || ext == "jpeg") {
        return load_from_png(filepath);
    }
    
    std::cerr << "Unsupported icon format: " << ext << std::endl;
    return false;
}

bool IconAsset::load_from_svg(const std::string& svg_path) {
    // Load SVG file into memory
    std::ifstream file(svg_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open SVG file: " << svg_path << std::endl;
        return false;
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    svg_data_.resize(size + 1); // +1 for null terminator
    file.read(reinterpret_cast<char*>(svg_data_.data()), size);
    svg_data_[size] = '\0'; // Null terminate for NanoSVG
    
    // Replace currentColor with white (will be tinted during rendering)
    // This is a simple implementation - for production you'd want a proper XML parser
    std::string svg_str(reinterpret_cast<char*>(svg_data_.data()));
    size_t pos = 0;
    while ((pos = svg_str.find("currentColor", pos)) != std::string::npos) {
        svg_str.replace(pos, 12, "#FFFFFF");  // Replace with white
        pos += 7; // Move past the replacement
    }
    
    // Copy modified string back to svg_data_
    std::copy(svg_str.begin(), svg_str.end(), svg_data_.begin());
    svg_data_[svg_str.size()] = '\0';
    
    // Parse SVG with NanoSVG
    svg_image_ = nsvgParse(reinterpret_cast<char*>(svg_data_.data()), "px", 96.0f);
    
    if (!svg_image_) {
        std::cerr << "Failed to parse SVG: " << svg_path << std::endl;
        return false;
    }
    
    is_svg_ = true;
    original_width_ = svg_image_->width;
    original_height_ = svg_image_->height;
    
    // Don't rasterize immediately - wait for first render request
    return true;
}

bool IconAsset::load_from_png(const std::string& png_path) {
    int width, height, channels;
    unsigned char* data = stbi_load(png_path.c_str(), &width, &height, &channels, 4); // Force RGBA
    
    if (!data) {
        std::cerr << "Failed to load PNG icon: " << png_path << std::endl;
        return false;
    }
    
    bool success = create_texture(data, width, height, 4);
    stbi_image_free(data);
    
    if (success) {
        width_ = static_cast<float>(width);
        height_ = static_cast<float>(height);
    }
    
    return success;
}

bool IconAsset::load_from_memory(const uint8_t* data, size_t size, bool is_svg) {
    if (is_svg) {
        svg_data_.assign(data, data + size + 1);
        svg_data_[size] = '\0'; // Null terminate for NanoSVG
        
        // Parse SVG with NanoSVG
        svg_image_ = nsvgParse(reinterpret_cast<char*>(svg_data_.data()), "px", 96.0f);
        
        if (!svg_image_) {
            return false;
        }
        
        is_svg_ = true;
        original_width_ = svg_image_->width;
        original_height_ = svg_image_->height;
        return true;
    } else {
        int width, height, channels;
        unsigned char* img_data = stbi_load_from_memory(data, static_cast<int>(size), 
                                                        &width, &height, &channels, 4);
        if (!img_data) {
            return false;
        }
        
        bool success = create_texture(img_data, width, height, 4);
        stbi_image_free(img_data);
        
        if (success) {
            width_ = static_cast<float>(width);
            height_ = static_cast<float>(height);
        }
        
        return success;
    }
}

bool IconAsset::rasterize_svg(int target_size) {
    if (!svg_image_) {
        return false;
    }
    
    // Create rasterizer if needed
    if (!svg_rasterizer_) {
        svg_rasterizer_ = nsvgCreateRasterizer();
        if (!svg_rasterizer_) {
            std::cerr << "Failed to create SVG rasterizer" << std::endl;
            return false;
        }
    }
    
    // Calculate scale for pixel-perfect rendering
    // Use exact target size, no rounding to avoid size drift
    float scale = target_size / std::max(svg_image_->width, svg_image_->height);
    int width = target_size;  // Force exact size
    int height = target_size; // Force square for consistency
    
    // Allocate buffer for rasterization (initialize to transparent)
    std::vector<uint8_t> pixels(width * height * 4, 0);
    
    // Rasterize SVG
    nsvgRasterize(svg_rasterizer_, svg_image_, 0, 0, scale,
                  pixels.data(), width, height, width * 4);
    
    // Debug: Check if we got any non-zero pixels
    int non_zero_count = 0;
    int max_alpha = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        if (pixels[i] > 0 || pixels[i+1] > 0 || pixels[i+2] > 0 || pixels[i+3] > 0) {
            non_zero_count++;
            max_alpha = std::max(max_alpha, (int)pixels[i+3]);
        }
    }
    
    // SVG rasterized: width x height with non_zero_count pixels
    std::cout << "SVG rasterized: " << name_ << " size=" << width << "x" << height 
              << " non_zero_pixels=" << non_zero_count 
              << " max_alpha=" << max_alpha << std::endl;
    
    if (non_zero_count == 0) {
        std::cerr << "Warning: SVG rasterized to empty image (all pixels transparent)" << std::endl;
        std::cerr << "Creating debug checkerboard pattern" << std::endl;
        // Create a checkerboard pattern so we can see something
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                bool checker = ((x / 4) + (y / 4)) % 2;
                uint8_t color = checker ? 255 : 128;
                pixels[idx] = color;     // R
                pixels[idx + 1] = color; // G
                pixels[idx + 2] = color; // B
                pixels[idx + 3] = 255;   // A
            }
        }
    }
    
    // Create texture from rasterized data
    bool success = create_texture(pixels.data(), width, height, 4);
    if (success) {
        width_ = static_cast<float>(width);
        height_ = static_cast<float>(height);
        last_rendered_size_ = target_size;
    }
    
    return success;
}

bool IconAsset::create_texture(const uint8_t* pixels, int width, int height, int channels) {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
    }
    
    // Creating texture with specified dimensions and channels
    std::cout << "Creating icon texture: " << width << "x" << height << " channels=" << channels << std::endl;
    
    glGenTextures(1, &texture_id_);
    std::cout << "Generated texture ID: " << texture_id_ << std::endl;
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Upload texture data
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error creating texture: " << error << std::endl;
    } else {
        // Texture created successfully
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return error == GL_NO_ERROR;
}

void IconAsset::render(CanvasRenderer* renderer, const Point2D& position, float scaled_size, const ColorRGBA& tint) {
    // Size is pre-scaled from ScaledTheme - no additional scaling needed
    
    // For SVG icons, use multi-size cache for optimal quality
    if (is_svg_) {
        // Size is already scaled, use directly for rasterization
        int raster_size = static_cast<int>(scaled_size);
        
        // Get or create texture for this size
        GLuint tex_id = get_texture_for_size(raster_size);
        if (tex_id == 0) return;  // Failed to get texture
        
        // Use the size-specific texture
        texture_id_ = tex_id;
        width_ = height_ = static_cast<float>(raster_size);
    }
    
    if (!is_loaded()) return;
    
    // Pixel-perfect alignment: round position to nearest pixel
    float x = std::round(position.x);
    float y = std::round(position.y);
    
    // Size is already scaled, render directly
    float physical_size = std::round(scaled_size);
    
    // Render the icon texture at the pre-scaled size
    renderer->draw_texture(texture_id_, Rect2D(x, y, physical_size, physical_size), tint);
}



void IconAsset::prerender_at_size(CanvasRenderer* renderer, float size) {
    // Pre-render SVG at specific size for faster display
    // NOTE: Size should be pre-scaled from ScaledTheme
    if (is_svg_) {
        // Size is already scaled, use directly
        int raster_size = static_cast<int>(size);
        
        // Pre-cache this size
        get_texture_for_size(raster_size);
    }
}

GLuint IconAsset::get_texture_for_size(int size) {
    // Check if we already have this size cached
    auto it = size_cache_.find(size);
    if (it != size_cache_.end()) {
        // Update last used time
        it->second.last_used = std::chrono::steady_clock::now();
        return it->second.texture_id;
    }
    
    // Need to rasterize at this size
    if (rasterize_svg(size)) {
        // Clean up old caches if we have too many
        if (size_cache_.size() >= MAX_CACHED_SIZES) {
            cleanup_old_caches();
        }
        
        // Add to cache
        SizeCache cache;
        cache.texture_id = texture_id_;
        cache.size = size;
        cache.last_used = std::chrono::steady_clock::now();
        size_cache_[size] = cache;
        
        return texture_id_;
    }
    
    return 0;  // Failed to rasterize
}

void IconAsset::cleanup_old_caches() {
    if (size_cache_.size() < MAX_CACHED_SIZES) return;
    
    // Find the least recently used entry
    auto oldest = size_cache_.begin();
    auto oldest_time = oldest->second.last_used;
    
    for (auto it = size_cache_.begin(); it != size_cache_.end(); ++it) {
        if (it->second.last_used < oldest_time) {
            oldest = it;
            oldest_time = it->second.last_used;
        }
    }
    
    // Delete the oldest texture
    if (oldest != size_cache_.end()) {
        if (oldest->second.texture_id != 0) {
            glDeleteTextures(1, &oldest->second.texture_id);
        }
        size_cache_.erase(oldest);
    }
}

void IconAsset::clear_cache() {
    // Delete all cached textures
    for (auto& [size, cache] : size_cache_) {
        if (cache.texture_id != 0) {
            glDeleteTextures(1, &cache.texture_id);
        }
    }
    size_cache_.clear();
    
    // Reset the current texture as well
    if (texture_id_ != 0 && !is_svg_) {
        // Don't delete non-SVG textures as they're the original loaded image
    } else if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    
    last_rendered_size_ = 0;
}

// IconSystem implementation

IconSystem::IconSystem() {
}

IconSystem::~IconSystem() {
    shutdown();
}

std::string IconSystem::make_cache_key(const std::string& icon_name, int size) const {
    return icon_name + "_" + std::to_string(size);
}

void IconSystem::evict_old_cache_entries() {
    // Calculate total cache size
    size_t total_size = 0;
    for (const auto& [key, cached] : size_cache_) {
        // Estimate: 4 bytes per pixel
        total_size += cached.width * cached.height * 4;
    }
    
    size_t max_size_bytes = MAX_CACHE_SIZE_MB * 1024 * 1024;
    
    if (total_size > max_size_bytes) {
        // Find oldest entries
        std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> entries;
        for (const auto& [key, cached] : size_cache_) {
            entries.push_back({key, cached.last_used});
        }
        
        // Sort by last used time
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      return a.second < b.second;
                  });
        
        // Remove oldest entries until we're under the limit
        size_t removed_size = 0;
        for (const auto& [key, _] : entries) {
            if (total_size - removed_size <= max_size_bytes * 0.75f) {
                break; // Keep 25% free space
            }
            
            auto it = size_cache_.find(key);
            if (it != size_cache_.end()) {
                glDeleteTextures(1, &it->second.texture_id);
                removed_size += it->second.width * it->second.height * 4;
                size_cache_.erase(it);
            }
        }
    }
}

bool IconSystem::initialize(const std::string& icon_directory) {
    if (initialized_) {
        return true;
    }
    
    icon_directory_ = icon_directory;
    
    // Load default icons
    load_default_icons();
    
    initialized_ = true;
    return true;
}

void IconSystem::shutdown() {
    icons_.clear();
    initialized_ = false;
}

bool IconSystem::load_icon(const std::string& name, const std::string& filepath) {
    std::cout << "Loading icon: " << name << " from " << filepath << std::endl;
    auto icon = std::make_unique<IconAsset>(name);
    
    if (!icon->load_from_file(filepath)) {
        std::cerr << "Failed to load icon: " << filepath << std::endl;
        return false;
    }
    
    std::cout << "Successfully loaded icon: " << name << std::endl;
    icons_[name] = std::move(icon);
    return true;
}

bool IconSystem::load_icon_set(const std::string& set_name, const std::string& directory) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(directory)) {
        std::cerr << "Icon directory does not exist: " << directory << std::endl;
        return false;
    }
    
    int loaded_count = 0;
    
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filepath = entry.path().string();
            std::string filename = entry.path().stem().string();
            
            if (is_svg_file(filepath) || is_png_file(filepath)) {
                std::string icon_name = set_name + "_" + filename;
                if (load_icon(icon_name, filepath)) {
                    loaded_count++;
                }
            }
        }
    }
    return loaded_count > 0;
}

IconAsset* IconSystem::get_icon(const std::string& name) {
    auto it = icons_.find(name);
    return (it != icons_.end()) ? it->second.get() : nullptr;
}

const IconAsset* IconSystem::get_icon(const std::string& name) const {
    auto it = icons_.find(name);
    return (it != icons_.end()) ? it->second.get() : nullptr;
}

bool IconSystem::has_icon(const std::string& name) const {
    return icons_.find(name) != icons_.end();
}

void IconSystem::load_default_icons() {
    // Load built-in icons
    load_builtin_icons();
    
    // Try to load icons from the assets directory
    std::string icons_dir = icon_directory_;
    
    if (std::filesystem::exists(icons_dir)) {
        // Load tool icons
        load_icon_set("tool", icons_dir + "tools/");
        
        // Load UI icons
        load_icon_set("ui", icons_dir + "ui/");
        
        // Load file icons
        load_icon_set("file", icons_dir + "file/");
    }
}

void IconSystem::render_icon(CanvasRenderer* renderer, const std::string& icon_name,
                             const Point2D& position, float scaled_size, const ColorRGBA& tint) {
    IconAsset* icon = get_icon(icon_name);
    if (icon) {
        // Check if we have a cached version at this size
        std::string cache_key = make_cache_key(icon_name, static_cast<int>(scaled_size));
        auto cache_it = size_cache_.find(cache_key);
        
        if (cache_it != size_cache_.end()) {
            // Update last used time
            cache_it->second.last_used = std::chrono::steady_clock::now();
        }
        
        icon->render(renderer, position, scaled_size, tint);
        
        // Periodically evict old cache entries
        static int render_count = 0;
        if (++render_count % 100 == 0) {
            evict_old_cache_entries();
        }
    }
}

IconSystem& IconSystem::get_instance() {
    static IconSystem instance;
    return instance;
}

void IconSystem::clear_all_caches() {
    // Clear cache for all icons
    for (auto& [name, icon] : icons_) {
        if (icon) {
            icon->clear_cache();
        }
    }
    
    // Also clear the system-level size cache
    for (auto& [key, cached] : size_cache_) {
        if (cached.texture_id != 0) {
            glDeleteTextures(1, &cached.texture_id);
        }
    }
    size_cache_.clear();
}

void IconSystem::load_builtin_icons() {
    // Create placeholder icons for essential UI elements
    // These would normally be embedded as resources
    
    // For now, we'll create simple colored squares as placeholders
    for (const auto& icon_name : {
        Icons::SELECT, Icons::PAINT, Icons::ERASE, Icons::FILL,
        Icons::MOVE, Icons::ROTATE, Icons::SCALE,
        Icons::NEW_FILE, Icons::OPEN_FILE, Icons::SAVE_FILE,
        Icons::UNDO, Icons::REDO,
        Icons::EXPAND, Icons::COLLAPSE
    }) {
        auto icon = std::make_unique<IconAsset>(icon_name);
        
        // Create a simple colored texture as placeholder
        std::vector<uint8_t> pixels(32 * 32 * 4);
        
        // Generate a unique color for each icon type
        uint32_t hash = std::hash<std::string>{}(icon_name);
        uint8_t r = (hash >> 16) & 0xFF;
        uint8_t g = (hash >> 8) & 0xFF;
        uint8_t b = hash & 0xFF;
        
        for (size_t i = 0; i < pixels.size(); i += 4) {
            pixels[i] = r;
            pixels[i + 1] = g;
            pixels[i + 2] = b;
            pixels[i + 3] = 255;
        }
        
        icon->load_from_memory(pixels.data(), pixels.size(), false);
        icons_[icon_name] = std::move(icon);
    }
}

bool IconSystem::is_svg_file(const std::string& filepath) const {
    std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "svg";
}

bool IconSystem::is_png_file(const std::string& filepath) const {
    std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "png" || ext == "jpg" || ext == "jpeg";
}

// IconButton implementation

IconButton::IconButton(const std::string& icon_name, const Rect2D& bounds, float scaled_icon_size)
    : icon_name_(icon_name)
    , bounds_(bounds)
    , scaled_icon_size_(scaled_icon_size)
    , icon_tint_(1.0f, 1.0f, 1.0f, 1.0f) {
}

void IconButton::render(CanvasRenderer* renderer) {
    const auto& theme = renderer->get_theme();
    
    // Draw button background
    ColorRGBA bg_color = theme.gray_5;
    if (!enabled_) {
        bg_color = theme.gray_4;
        bg_color.a = 0.5f;
    } else if (pressed_) {
        bg_color = theme.gray_5;  // Darker shade for pressed state
        bg_color.r *= 0.8f;
        bg_color.g *= 0.8f;
        bg_color.b *= 0.8f;
    } else if (selected_) {
        bg_color = theme.accent_primary;
    } else if (hovered_) {
        bg_color = theme.gray_4;
    }
    
    renderer->draw_rect(bounds_, bg_color);
    
    // Draw icon
    if (!icon_name_.empty()) {
        Point2D icon_pos(
            bounds_.x + (bounds_.width - scaled_icon_size_) * 0.5f,
            bounds_.y + (bounds_.height - scaled_icon_size_) * 0.5f
        );
        
        ColorRGBA tint = icon_tint_;
        if (!enabled_) {
            tint.a *= 0.5f;
        }
        
        IconSystem::get_instance().render_icon(renderer, icon_name_, icon_pos, scaled_icon_size_, tint);
    }
    
    // Draw border for selected state
    if (selected_) {
        renderer->draw_rect_outline(bounds_, theme.accent_primary, 2.0f);
    }
}

bool IconButton::handle_event(const InputEvent& event) {
    if (!enabled_) {
        return false;
    }
    
    if (event.type == EventType::MOUSE_MOVE) {
        bool was_hovered = hovered_;
        hovered_ = bounds_.contains(event.mouse_pos);
        return hovered_ != was_hovered;
    }
    
    if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
        if (bounds_.contains(event.mouse_pos)) {
            pressed_ = true;
            return true;
        }
    }
    
    if (event.type == EventType::MOUSE_RELEASE && event.mouse_button == MouseButton::LEFT) {
        if (pressed_) {
            pressed_ = false;
            if (bounds_.contains(event.mouse_pos)) {
                if (click_callback_) {
                    click_callback_();
                }
                return true;
            }
        }
    }
    
    return false;
}

} // namespace voxel_canvas