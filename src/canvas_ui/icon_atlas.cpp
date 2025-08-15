/*
 * Copyright (C) 2024 Voxelux
 * 
 * Icon Atlas Implementation for efficient batch rendering
 */

#include "canvas_ui/icon_system.h"
#include <glad/gl.h>
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

IconAtlas::IconAtlas(int width, int height)
    : atlas_width_(width)
    , atlas_height_(height) {
    
    // Create OpenGL texture for atlas
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture memory (RGBA)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_width_, atlas_height_, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

IconAtlas::~IconAtlas() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
    }
}

bool IconAtlas::add_icon(const std::string& key, const uint8_t* pixels, int width, int height) {
    // Check if icon already exists
    if (entries_.find(key) != entries_.end()) {
        return true; // Already in atlas
    }
    
    // Simple row-based packing algorithm
    // For production, you'd use rectpack2D here
    
    // Check if we need to move to next row
    if (next_x_ + width > atlas_width_) {
        next_x_ = 0;
        next_y_ += row_height_ + 2; // 2 pixel padding
        row_height_ = 0;
    }
    
    // Check if we have space in atlas
    if (next_y_ + height > atlas_height_) {
        // Atlas is full, need to evict or resize
        // For now, clear and start over
        clear();
    }
    
    // Upload texture data to specific region
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, next_x_, next_y_, width, height,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Calculate UV coordinates
    AtlasEntry entry;
    entry.texture_coords.x = static_cast<float>(next_x_) / atlas_width_;
    entry.texture_coords.y = static_cast<float>(next_y_) / atlas_height_;
    entry.texture_coords.width = static_cast<float>(width) / atlas_width_;
    entry.texture_coords.height = static_cast<float>(height) / atlas_height_;
    entry.width = static_cast<float>(width);
    entry.height = static_cast<float>(height);
    entry.last_used = 0; // Will be updated on use
    
    entries_[key] = entry;
    
    // Update packing position
    next_x_ += width + 2; // 2 pixel padding
    row_height_ = std::max(row_height_, height);
    
    return true;
}

const IconAtlas::AtlasEntry* IconAtlas::get_entry(const std::string& key) const {
    auto it = entries_.find(key);
    return (it != entries_.end()) ? &it->second : nullptr;
}

void IconAtlas::clear() {
    entries_.clear();
    next_x_ = 0;
    next_y_ = 0;
    row_height_ = 0;
    
    // Clear texture to transparent
    std::vector<uint8_t> clear_data(atlas_width_ * atlas_height_ * 4, 0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlas_width_, atlas_height_,
                    GL_RGBA, GL_UNSIGNED_BYTE, clear_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void IconAtlas::evict_lru(size_t target_count) {
    if (entries_.size() <= target_count) {
        return;
    }
    
    // Collect entries with their last used time
    std::vector<std::pair<std::string, uint32_t>> usage;
    for (const auto& [key, entry] : entries_) {
        usage.push_back({key, entry.last_used});
    }
    
    // Sort by last used time
    std::sort(usage.begin(), usage.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });
    
    // Remove oldest entries
    size_t to_remove = entries_.size() - target_count;
    for (size_t i = 0; i < to_remove && i < usage.size(); ++i) {
        entries_.erase(usage[i].first);
    }
    
    // After eviction, we should reorganize the atlas
    // For now, we'll just mark it as needing reorganization
    // In production, you'd repack all remaining icons
}

} // namespace voxel_canvas