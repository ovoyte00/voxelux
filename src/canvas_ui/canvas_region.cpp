/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Region and Editor system implementation
 */

#include "canvas_ui/canvas_region.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/ui_widgets.h"
#include "canvas_ui/viewport_3d_editor.h"
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// CanvasRegion implementation

CanvasRegion::CanvasRegion(RegionID id, const Rect2D& bounds)
    : id_(id)
    , bounds_(bounds)
    , show_header_(true)
    , show_dropdown_(true)
    , horizontal_split_(false)
    , split_ratio_(0.5f)
    , is_resizing_(false)
    , resize_start_pos_(0, 0)
    , resize_start_ratio_(0.5f)
    , is_hovered_(false)
    , header_hovered_(false) {
    
    // Initialize widget manager
    widget_manager_ = std::make_shared<WidgetManager>();
    
    // Create editor dropdown if showing header
    if (show_header_ && show_dropdown_) {
        // Delay setup until bounds are properly set
        // setup_editor_dropdown() will be called in set_bounds()
    }
}

CanvasRegion::~CanvasRegion() = default;

void CanvasRegion::set_bounds(const Rect2D& bounds) {
    if (bounds_.x != bounds.x || bounds_.y != bounds.y || 
        bounds_.width != bounds.width || bounds_.height != bounds.height) {
        mark_size_changed();
        mark_layout_changed();
    }
    bounds_ = bounds;
    
    // Setup or update dropdown bounds
    if (show_header_ && show_dropdown_) {
        if (!editor_dropdown_) {
            setup_editor_dropdown();
        }
        const float dropdown_width = 120.0f;
        const float margin = 4.0f;
        Rect2D header_rect = get_header_bounds();
        
        Rect2D dropdown_bounds(
            header_rect.x + margin,
            header_rect.y + margin,
            dropdown_width,
            header_rect.height - margin * 2
        );
        
        if (editor_dropdown_) {
            editor_dropdown_->set_bounds(dropdown_bounds);
        }
    }
    
    update_layout();
}

void CanvasRegion::set_editor(std::unique_ptr<EditorSpace> editor) {
    if (editor_) {
        editor_->shutdown();
    }
    
    editor_ = std::move(editor);
    
    if (editor_) {
        editor_->initialize();
    }
    
    // Invalidate content when editor changes
    mark_content_changed();
}

EditorType CanvasRegion::get_editor_type() const {
    return editor_ ? editor_->get_type() : EditorType::VIEWPORT_3D;
}

Rect2D CanvasRegion::get_header_bounds() const {
    if (!show_header_) {
        return Rect2D(0, 0, 0, 0);
    }
    
    // Scale UI elements for high DPI displays
    const float base_header_height = 24.0f;
    const float scale_factor = 2.0f; // Assume 2x scaling for now (will make this dynamic later)
    const float header_height = base_header_height * scale_factor;
    return Rect2D(bounds_.x, bounds_.y, bounds_.width, header_height);
}

Rect2D CanvasRegion::get_content_bounds() const {
    if (!show_header_) {
        return bounds_;
    }
    
    // Use same scaling as get_header_bounds()
    const float base_header_height = 24.0f;
    const float scale_factor = 2.0f; // Assume 2x scaling for now
    const float header_height = base_header_height * scale_factor;
    return Rect2D(
        bounds_.x,
        bounds_.y + header_height,
        bounds_.width,
        bounds_.height - header_height
    );
}

bool CanvasRegion::split_horizontal(float split_ratio) {
    if (!can_split()) {
        return false;
    }
    
    split_region(true, split_ratio);
    return true;
}

bool CanvasRegion::split_vertical(float split_ratio) {
    if (!can_split()) {
        return false;
    }
    
    split_region(false, split_ratio);
    return true;
}

bool CanvasRegion::can_split() const {
    const float min_size = 100.0f; // Minimum region size
    return bounds_.width >= min_size * 2 && bounds_.height >= min_size * 2;
}

void CanvasRegion::render(CanvasRenderer* renderer) {
    if (!renderer) return;
    
    // **Canvas UI Professional Rendering System** (original implementation inspired by Blender)
    // Check if this region needs to be redrawn
    if (!needs_update()) {
        return; // Skip rendering if no updates needed
    }
    
    // **Canvas UI Drawing Safety** - Prevent recursive invalidation during rendering
    // Mark region as currently drawing (Blender uses RGN_DRAWING)
    bool was_drawing = has_flag(RegionUpdateFlags::ANIMATION_ACTIVE); // Reuse existing flag
    if (was_drawing) {
        return; // Already drawing, prevent recursion
    }
    
    // Set drawing flag to prevent recursive invalidation
    mark_animation_active();
    
    try {
        // Render based on whether this is a split region or leaf region
        if (is_split()) {
            // Render children
            for (auto& child : children_) {
                child->render(renderer);
            }
            
            // Render splitter handles
            render_splitter_handles(renderer);
        } else {
            
            // Render header
            if (show_header_) {
                render_header(renderer);
            }
            
            // Render content
            render_content(renderer);
            
            // Render widgets
            if (widget_manager_) {
                widget_manager_->render_widgets(renderer);
            }
        }
        
        // **Canvas UI Continuous Visibility System** (original approach)
        // For visible UI regions, ensure they stay renderable for next frame
        bool has_visible_content = !is_split() && (show_header_ || editor_);
        
        // **FIX**: Don't clear flags too aggressively - keep regions visible
        // Only clear specific flags, but maintain content changed for continuous rendering
        // This ensures regions stay visible without flickering
        if (has_visible_content) {
            // Keep content changed for continuous visibility
            invalidation_flags_ = RegionUpdateFlags::CONTENT_CHANGED;
        } else {
            clear_update_flags();
        }
        
    } catch (...) {
        // Ensure drawing flag is cleared even if rendering fails
        clear_update_flags();
        throw;
    }
}

void CanvasRegion::render_header(CanvasRenderer* renderer) {
    
    Rect2D header_rect = get_header_bounds();
    
    // Header background with hover state
    ColorRGBA header_color = ColorRGBA("#5680c2"); // Bright blue
    if (header_hovered_) {
        header_color = ColorRGBA("#6890d2");
    }
    renderer->draw_rect(header_rect, header_color);
    
    // Render editor type text (temporary until dropdown is working)
    const float scale_factor = 2.0f;
    if (editor_) {
        Point2D editor_text_pos(header_rect.x + 10 * scale_factor, header_rect.y + 12 * scale_factor);
        std::string editor_name = EditorFactory::get_editor_name(editor_->get_type());
        renderer->draw_text(editor_name, editor_text_pos, ColorRGBA("#ffffff"), 14.0f * scale_factor);
    }
}

void CanvasRegion::render_content(CanvasRenderer* renderer) {
    
    Rect2D content_rect = get_content_bounds();
    
    // Content background - temporarily disabled to test if this is causing gray screen
    // renderer->draw_rect(content_rect, renderer->get_theme().background_primary);
    
    // Render editor content
    if (editor_) {
        editor_->render(renderer, content_rect);
        // Render overlay (text, widgets, etc.)
        editor_->render_overlay(renderer, content_rect);
    } else {
        // Render placeholder
        Point2D center(content_rect.x + content_rect.width * 0.5f, content_rect.y + content_rect.height * 0.5f);
        renderer->draw_text("No Editor", center, renderer->get_theme().text_secondary, 14.0f);
    }
}

void CanvasRegion::render_splitter_handles(CanvasRenderer* renderer) {
    if (!is_split() || children_.size() != 2) {
        return;
    }
    
    const float splitter_size = 4.0f;
    ColorRGBA splitter_color = renderer->get_theme().border_normal;
    
    if (horizontal_split_) {
        // Horizontal splitter between top and bottom
        float split_y = bounds_.y + bounds_.height * split_ratio_;
        Rect2D splitter_rect(
            bounds_.x,
            split_y - splitter_size * 0.5f,
            bounds_.width,
            splitter_size
        );
        renderer->draw_rect(splitter_rect, splitter_color);
    } else {
        // Vertical splitter between left and right
        float split_x = bounds_.x + bounds_.width * split_ratio_;
        Rect2D splitter_rect(
            split_x - splitter_size * 0.5f,
            bounds_.y,
            splitter_size,
            bounds_.height
        );
        renderer->draw_rect(splitter_rect, splitter_color);
    }
}

bool CanvasRegion::handle_event(const InputEvent& event) {
    // Debug: Only log clicks, not mouse movement
    if (event.type == EventType::MOUSE_PRESS) {
        std::cout << "CanvasRegion received mouse click at (" << event.mouse_pos.x << ", " << event.mouse_pos.y << ")" << std::endl;
    }
    
    // If this is a split region, route to children
    if (is_split()) {
        for (auto& child : children_) {
            if (child->handle_event(event)) {
                return true;
            }
        }
        
        // Check for splitter interaction
        if (event.is_mouse_event() && is_point_on_splitter(event.mouse_pos)) {
            // Handle splitter resize
            if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
                is_resizing_ = true;
                resize_start_pos_ = event.mouse_pos;
                resize_start_ratio_ = split_ratio_;
                return true;
            }
        }
        
        return false;
    }
    
    // Handle header events
    if (show_header_ && handle_header_event(event)) {
        return true;
    }
    
    // Check widget events first (they have higher priority)
    if (widget_manager_ && widget_manager_->handle_widget_event(event)) {
        return true;
    }
    
    // Route to editor
    if (editor_) {
        Rect2D content_rect = get_content_bounds();
        if (event.type == EventType::MOUSE_PRESS) {
            std::cout << "Routing click to editor" << std::endl;
        }
        return editor_->handle_event(event, content_rect);
    }
    
    return false;
}

bool CanvasRegion::handle_header_event(const InputEvent& event) {
    Rect2D header_rect = get_header_bounds();
    
    bool was_hovered = header_hovered_;
    
    if (!header_rect.contains(event.mouse_pos)) {
        header_hovered_ = false;
    } else {
        header_hovered_ = true;
    }
    
    // Invalidate if hover state changed
    if (was_hovered != header_hovered_) {
        mark_overlay_changed();
    }
    
    if (!header_rect.contains(event.mouse_pos)) {
        return false;
    }
    
    // Header events are now mostly handled by widgets
    // This could handle right-click context menu, etc.
    
    return false;
}

CanvasRegion* CanvasRegion::find_region_at_point(const Point2D& point) {
    if (!bounds_.contains(point)) {
        return nullptr;
    }
    
    if (is_split()) {
        for (auto& child : children_) {
            if (auto* found = child->find_region_at_point(point)) {
                return found;
            }
        }
    }
    
    return this;
}

void CanvasRegion::update_layout() {
    if (is_split()) {
        update_children_bounds();
        
        for (auto& child : children_) {
            child->update_layout();
        }
    }
}

void CanvasRegion::set_split_ratio(float ratio) {
    float new_ratio = std::clamp(ratio, 0.1f, 0.9f);
    if (split_ratio_ != new_ratio) {
        split_ratio_ = new_ratio;
        mark_layout_changed();
        update_layout();
    }
}

Point2D CanvasRegion::get_minimum_size() const {
    const float min_size = 100.0f;
    const float header_height = show_header_ ? 24.0f : 0.0f;
    return Point2D(min_size, min_size + header_height);
}

bool CanvasRegion::can_resize_to(const Rect2D& new_bounds) const {
    Point2D min_size = get_minimum_size();
    return new_bounds.width >= min_size.x && new_bounds.height >= min_size.y;
}

void CanvasRegion::split_region(bool horizontal, float ratio) {
    // Store current editor
    auto current_editor = std::move(editor_);
    
    // Create two child regions
    horizontal_split_ = horizontal;
    split_ratio_ = ratio;
    
    RegionID child1_id = id_ * 2 + 1;  // Simple ID generation
    RegionID child2_id = id_ * 2 + 2;
    
    children_.push_back(std::make_unique<CanvasRegion>(child1_id, Rect2D()));
    children_.push_back(std::make_unique<CanvasRegion>(child2_id, Rect2D()));
    
    // Move current editor to first child
    if (current_editor) {
        children_[0]->set_editor(std::move(current_editor));
    }
    
    // Set default editor for second child
    children_[1]->set_editor(EditorFactory::create_editor(EditorType::PROPERTIES));
    
    update_children_bounds();
}

void CanvasRegion::update_children_bounds() {
    if (!is_split() || children_.size() != 2) {
        return;
    }
    
    if (horizontal_split_) {
        // Split horizontally (top/bottom)
        float split_y = bounds_.y + bounds_.height * split_ratio_;
        
        children_[0]->set_bounds(Rect2D(bounds_.x, bounds_.y, bounds_.width, split_y - bounds_.y));
        children_[1]->set_bounds(Rect2D(bounds_.x, split_y, bounds_.width, bounds_.y + bounds_.height - split_y));
    } else {
        // Split vertically (left/right)
        float split_x = bounds_.x + bounds_.width * split_ratio_;
        
        children_[0]->set_bounds(Rect2D(bounds_.x, bounds_.y, split_x - bounds_.x, bounds_.height));
        children_[1]->set_bounds(Rect2D(split_x, bounds_.y, bounds_.x + bounds_.width - split_x, bounds_.height));
    }
}

bool CanvasRegion::is_point_on_splitter(const Point2D& point) const {
    if (!is_split() || children_.size() != 2) {
        return false;
    }
    
    const float splitter_tolerance = 4.0f;
    
    if (horizontal_split_) {
        float split_y = bounds_.y + bounds_.height * split_ratio_;
        return std::abs(point.y - split_y) <= splitter_tolerance &&
               point.x >= bounds_.x && point.x <= bounds_.x + bounds_.width;
    } else {
        float split_x = bounds_.x + bounds_.width * split_ratio_;
        return std::abs(point.x - split_x) <= splitter_tolerance &&
               point.y >= bounds_.y && point.y <= bounds_.y + bounds_.height;
    }
}

// EditorSpace implementation

EditorSpace::EditorSpace(EditorType type)
    : type_(type) {
}

// EditorFactory implementation

std::unordered_map<EditorType, EditorFactory::EditorInfo> EditorFactory::registered_editors_;

std::unique_ptr<EditorSpace> EditorFactory::create_editor(EditorType type) {
    switch (type) {
        case EditorType::VIEWPORT_3D:
            return std::make_unique<Viewport3DEditor>();
            
        default: {
            // Placeholder editors for other types
            class PlaceholderEditor : public EditorSpace {
            public:
                explicit PlaceholderEditor(EditorType type) : EditorSpace(type) {}
                
                std::string get_name() const override {
                    return get_editor_name(get_type());
                }
                
                void render(CanvasRenderer* renderer, const Rect2D& bounds) override {
                    // Render placeholder content
                    ColorRGBA bright_bg = ColorRGBA("#4a4a4a");
                    renderer->draw_rect(bounds, bright_bg);
                    renderer->draw_rect_outline(bounds, ColorRGBA("#ffffff"), 2.0f);
                    
                    Point2D title_pos(bounds.x + 20, bounds.y + 50);
                    std::string title = get_name() + " Editor";
                    renderer->draw_text(title, title_pos, ColorRGBA("#ffffff"), 24.0f);
                    
                    Point2D help_pos(bounds.x + 20, bounds.y + 100);
                    renderer->draw_text("Not yet implemented", help_pos, ColorRGBA("#cccccc"), 16.0f);
                }
            };
            
            return std::make_unique<PlaceholderEditor>(type);
        }
    }
}

std::vector<EditorType> EditorFactory::get_available_editor_types() {
    return {
        EditorType::VIEWPORT_3D,
        EditorType::PROPERTIES,
        EditorType::OUTLINER,
        EditorType::TIMELINE,
        EditorType::MATERIAL_EDITOR,
        EditorType::ASSET_BROWSER,
        EditorType::CONSOLE,
        EditorType::PREFERENCES
    };
}

std::string EditorFactory::get_editor_name(EditorType type) {
    switch (type) {
        case EditorType::VIEWPORT_3D: return "3D Viewport";
        case EditorType::PROPERTIES: return "Properties";
        case EditorType::OUTLINER: return "Outliner";
        case EditorType::TIMELINE: return "Timeline";
        case EditorType::MATERIAL_EDITOR: return "Material Editor";
        case EditorType::ASSET_BROWSER: return "Asset Browser";
        case EditorType::CONSOLE: return "Console";
        case EditorType::PREFERENCES: return "Preferences";
    }
    return "Unknown";
}

std::string EditorFactory::get_editor_description(EditorType type) {
    switch (type) {
        case EditorType::VIEWPORT_3D: return "Main 3D voxel editing viewport";
        case EditorType::PROPERTIES: return "Object and material properties";
        case EditorType::OUTLINER: return "Scene hierarchy and object tree";
        case EditorType::TIMELINE: return "Animation timeline and keyframes";
        case EditorType::MATERIAL_EDITOR: return "Voxel material and texture editor";
        case EditorType::ASSET_BROWSER: return "Asset library and file browser";
        case EditorType::CONSOLE: return "Debug console and scripting";
        case EditorType::PREFERENCES: return "Application settings and preferences";
    }
    return "Unknown editor type";
}

void EditorFactory::register_editor_type(EditorType type, const std::string& name, EditorCreator creator) {
    EditorInfo info;
    info.name = name;
    info.creator = creator;
    registered_editors_[type] = info;
}

void CanvasRegion::setup_editor_dropdown() {
    if (!show_header_ || !show_dropdown_) return;
    
    const float dropdown_width = 120.0f;
    const float margin = 4.0f;
    Rect2D header_rect = get_header_bounds();
    
    Rect2D dropdown_bounds(
        header_rect.x + margin,
        header_rect.y + margin,
        dropdown_width,
        header_rect.height - margin * 2
    );
    
    EditorType current_type = editor_ ? editor_->get_type() : EditorType::VIEWPORT_3D;
    editor_dropdown_ = std::make_shared<EditorTypeDropdown>(dropdown_bounds, current_type);
    
    // Set callback for editor type changes
    editor_dropdown_->set_selection_callback([this](EditorType new_type) {
        on_editor_type_changed(new_type);
    });
    
    // Add to widget manager
    if (widget_manager_) {
        widget_manager_->add_widget(editor_dropdown_);
    }
}

void CanvasRegion::on_editor_type_changed(EditorType new_type) {
    std::cout << "Changing editor type to: " << EditorFactory::get_editor_name(new_type) << std::endl;
    
    // Create new editor of the selected type
    auto new_editor = EditorFactory::create_editor(new_type);
    set_editor(std::move(new_editor));
}

} // namespace voxel_canvas