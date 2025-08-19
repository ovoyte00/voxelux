/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Region and Editor system
 * Flexible region-based layout with pluggable editors.
 */

#pragma once

#include "canvas_core.h"
#include <memory>
#include <vector>
#include <string>

namespace voxel_canvas {
class EditorTypeDropdown;
class RegionSplitter;
class RegionContextMenu;
class WidgetManager;
}

namespace voxel_canvas {

// Forward declarations
class EditorSpace;
class CanvasRenderer;

// Types of editors that can be assigned to regions
enum class EditorType {
    VIEWPORT_3D,     // Main 3D voxel editing viewport
    PROPERTIES,      // Object/material properties
    OUTLINER,        // Scene hierarchy
    TIMELINE,        // Animation timeline  
    MATERIAL_EDITOR, // Voxel material/texture editor
    ASSET_BROWSER,   // Asset library browser
    CONSOLE,         // Debug/scripting console
    PREFERENCES      // Application settings
};

// Canvas UI Invalidation Flags (original implementation)
enum class RegionUpdateFlags : uint32_t {
    NONE = 0,
    CONTENT_CHANGED = 1 << 0,    // Content needs redraw
    LAYOUT_CHANGED = 1 << 1,     // Layout needs recalculation  
    OVERLAY_CHANGED = 1 << 2,    // Overlays need refresh
    SIZE_CHANGED = 1 << 3,       // Region bounds changed
    THEME_CHANGED = 1 << 4,      // Theme/colors changed
    ANIMATION_ACTIVE = 1 << 5,   // Animation in progress
    FORCE_UPDATE = 1 << 6        // Force complete refresh
};

// Bitwise operators for flag combinations
constexpr RegionUpdateFlags operator|(RegionUpdateFlags a, RegionUpdateFlags b) {
    return static_cast<RegionUpdateFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr RegionUpdateFlags operator&(RegionUpdateFlags a, RegionUpdateFlags b) {
    return static_cast<RegionUpdateFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * A region represents a rectangular area of the window that can contain
 * an editor. Regions can be split, merged, and resized dynamically.
 */
class CanvasRegion {
public:
    CanvasRegion(RegionID id, const Rect2D& bounds);
    ~CanvasRegion();
    
    // Basic properties
    RegionID get_id() const { return id_; }
    const Rect2D& get_bounds() const { return bounds_; }
    void set_bounds(const Rect2D& bounds);
    
    // Editor management
    void set_editor(std::unique_ptr<EditorSpace> editor);
    EditorSpace* get_editor() const { return editor_.get(); }
    EditorType get_editor_type() const;
    
    // Header and controls
    bool has_header() const { return show_header_; }
    void set_show_header(bool show) { show_header_ = show; }
    Rect2D get_header_bounds() const;
    Rect2D get_content_bounds() const;
    
    // Splitting state
    bool is_split() const { return !children_.empty(); }
    bool is_horizontal_split() const { return horizontal_split_; }
    const std::vector<std::unique_ptr<CanvasRegion>>& get_children() const { return children_; }
    
    // Region splitting
    bool split_horizontal(float split_ratio = 0.5f);
    bool split_vertical(float split_ratio = 0.5f);
    bool can_split() const;
    
    // Rendering
    void render(CanvasRenderer* renderer);
    void render_header(CanvasRenderer* renderer);
    void render_content(CanvasRenderer* renderer);
    void render_splitter_handles(CanvasRenderer* renderer);
    
    // Event handling
    bool handle_event(const InputEvent& event);
    bool handle_header_event(const InputEvent& event);
    CanvasRegion* find_region_at_point(const Point2D& point);
    
    // Layout updates
    void update_layout();
    void set_split_ratio(float ratio);
    
    // Canvas UI Professional Invalidation System (original implementation)
    void mark_content_changed() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::CONTENT_CHANGED; 
        }
    }
    void mark_layout_changed() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::LAYOUT_CHANGED; 
        }
    }
    void mark_overlay_changed() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::OVERLAY_CHANGED; 
        }
    }
    void mark_size_changed() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::SIZE_CHANGED; 
        }
    }
    void mark_theme_changed() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::THEME_CHANGED; 
        }
    }
    void mark_animation_active() { invalidation_flags_ = invalidation_flags_ | RegionUpdateFlags::ANIMATION_ACTIVE; }
    void force_full_update() { 
        if (!is_currently_drawing()) {
            invalidation_flags_ = RegionUpdateFlags::FORCE_UPDATE; 
        }
    }
    
    // Professional continuous rendering for visible UI
    void ensure_visible() { 
        if (invalidation_flags_ == RegionUpdateFlags::NONE) {
            invalidation_flags_ = RegionUpdateFlags::CONTENT_CHANGED;
        }
    }
    
    bool needs_update() const { return invalidation_flags_ != RegionUpdateFlags::NONE; }
    bool has_flag(RegionUpdateFlags flag) const { return (invalidation_flags_ & flag) != RegionUpdateFlags::NONE; }
    void clear_update_flags() { invalidation_flags_ = RegionUpdateFlags::NONE; }
    
    // Canvas UI Drawing Safety System (professional UI framework)
    bool is_currently_drawing() const { return has_flag(RegionUpdateFlags::ANIMATION_ACTIVE); }
    float get_split_ratio() const { return split_ratio_; }
    
    // Minimum size constraints
    Point2D get_minimum_size() const;
    bool can_resize_to(const Rect2D& new_bounds) const;
    
private:
    void split_region(bool horizontal, float ratio);
    void update_children_bounds();
    void render_resize_handles(CanvasRenderer* renderer);
    bool is_point_on_splitter(const Point2D& point) const;
    void setup_editor_dropdown();
    void on_editor_type_changed(EditorType new_type);
    
    // Core properties
    RegionID id_;
    Rect2D bounds_;
    
    // Editor content
    std::unique_ptr<EditorSpace> editor_;
    
    // UI elements
    bool show_header_ = true;
    bool show_dropdown_ = true;
    
    // Splitting state
    std::vector<std::unique_ptr<CanvasRegion>> children_;
    bool horizontal_split_ = false;
    float split_ratio_ = 0.5f;
    
    // Interaction state
    bool is_resizing_ = false;
    
    // Canvas UI Invalidation System
    RegionUpdateFlags invalidation_flags_ = RegionUpdateFlags::FORCE_UPDATE; // Start with full update
    Point2D resize_start_pos_;
    float resize_start_ratio_;
    
    // Visual state
    bool is_hovered_ = false;
    bool header_hovered_ = false;
    
    // Interactive widgets
    std::shared_ptr<EditorTypeDropdown> editor_dropdown_;
    std::shared_ptr<WidgetManager> widget_manager_;
};

/**
 * Base class for all editor types that can be displayed in regions.
 * Each editor provides its own rendering and event handling.
 */
class EditorSpace {
public:
    explicit EditorSpace(EditorType type);
    virtual ~EditorSpace() = default;
    
    // Editor identification
    EditorType get_type() const { return type_; }
    virtual std::string get_name() const = 0;
    virtual std::string get_icon() const { return ""; }
    
    // Lifecycle
    virtual void initialize() {}
    virtual void shutdown() {}
    virtual void update([[maybe_unused]] float delta_time) {}
    
    // Rendering
    virtual void render(CanvasRenderer* renderer, const Rect2D& bounds) = 0;
    virtual void render_overlay([[maybe_unused]] CanvasRenderer* renderer, [[maybe_unused]] const Rect2D& bounds) {}
    
    // Event handling
    virtual bool handle_event([[maybe_unused]] const InputEvent& event, [[maybe_unused]] const Rect2D& bounds) { return false; }
    virtual bool wants_keyboard_focus() const { return false; }
    virtual bool wants_mouse_capture() const { return false; }
    
    // Configuration
    virtual void save_state() {}
    virtual void load_state() {}
    virtual bool has_settings() const { return false; }
    virtual void show_settings_dialog() {}
    
    // Context menu
    virtual std::vector<std::string> get_context_menu_items() const { return {}; }
    virtual void handle_context_menu_selection(const std::string& item) {}
    
protected:
    EditorType type_;
};

/**
 * Factory for creating editor instances.
 */
class EditorFactory {
public:
    static std::unique_ptr<EditorSpace> create_editor(EditorType type);
    static std::vector<EditorType> get_available_editor_types();
    static std::string get_editor_name(EditorType type);
    static std::string get_editor_description(EditorType type);
    
    // Plugin registration (for future extensibility)
    using EditorCreator = std::function<std::unique_ptr<EditorSpace>()>;
    static void register_editor_type(EditorType type, const std::string& name, EditorCreator creator);
    
private:
    struct EditorInfo {
        std::string name;
        std::string description;
        EditorCreator creator;
    };
    
    static std::unordered_map<EditorType, EditorInfo> registered_editors_;
};

} // namespace voxel_canvas