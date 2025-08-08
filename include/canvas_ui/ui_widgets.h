/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Interactive UI Widgets
 * Professional UI components for the Canvas UI system.
 */

#pragma once

#include "canvas_core.h"
#include "canvas_region.h"
#include <vector>
#include <string>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;

/**
 * Base class for interactive UI widgets
 */
class UIWidget {
public:
    UIWidget(const Rect2D& bounds) : bounds_(bounds), visible_(true), enabled_(true) {}
    virtual ~UIWidget() = default;
    
    // Rendering
    virtual void render(CanvasRenderer* renderer) = 0;
    
    // Event handling
    virtual bool handle_event(const InputEvent& event) = 0;
    virtual bool contains_point(const Point2D& point) const { return bounds_.contains(point); }
    
    // Properties
    const Rect2D& get_bounds() const { return bounds_; }
    void set_bounds(const Rect2D& bounds) { bounds_ = bounds; }
    
    bool is_visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
protected:
    Rect2D bounds_;
    bool visible_;
    bool enabled_;
};

/**
 * Dropdown menu for selecting editor types
 */
class EditorTypeDropdown : public UIWidget {
public:
    using SelectionCallback = std::function<void(EditorType)>;
    
    EditorTypeDropdown(const Rect2D& bounds, EditorType current_type = EditorType::VIEWPORT_3D);
    ~EditorTypeDropdown() override = default;
    
    // Widget interface
    void render(CanvasRenderer* renderer) override;
    bool handle_event(const InputEvent& event) override;
    
    // Dropdown functionality
    void set_selected_type(EditorType type);
    EditorType get_selected_type() const { return selected_type_; }
    void set_selection_callback(SelectionCallback callback) { selection_callback_ = callback; }
    
    void show_dropdown();
    void hide_dropdown();
    bool is_dropdown_open() const { return dropdown_open_; }
    
private:
    void render_button(CanvasRenderer* renderer);
    void render_dropdown_list(CanvasRenderer* renderer);
    
    bool handle_button_event(const InputEvent& event);
    bool handle_dropdown_event(const InputEvent& event);
    
    Rect2D get_dropdown_bounds() const;
    int get_item_at_point(const Point2D& point) const;
    
    EditorType selected_type_;
    std::vector<EditorType> available_types_;
    
    bool dropdown_open_;
    bool button_hovered_;
    bool button_pressed_;
    int hovered_item_;
    
    SelectionCallback selection_callback_;
    
    static constexpr float ITEM_HEIGHT = 24.0f;
    static constexpr float DROPDOWN_MAX_HEIGHT = 200.0f;
};

/**
 * Region splitter handle for resizing regions
 */
class RegionSplitter : public UIWidget {
public:
    enum class Orientation {
        HORIZONTAL,  // Horizontal line, resizes vertically
        VERTICAL     // Vertical line, resizes horizontally
    };
    
    using ResizeCallback = std::function<void(float)>;
    
    RegionSplitter(const Rect2D& bounds, Orientation orientation);
    ~RegionSplitter() override = default;
    
    // Widget interface
    void render(CanvasRenderer* renderer) override;
    bool handle_event(const InputEvent& event) override;
    
    // Splitter functionality
    void set_resize_callback(ResizeCallback callback) { resize_callback_ = callback; }
    void set_split_ratio(float ratio) { split_ratio_ = ratio; }
    float get_split_ratio() const { return split_ratio_; }
    
private:
    void update_cursor();
    
    Orientation orientation_;
    bool is_dragging_;
    bool is_hovered_;
    Point2D drag_start_pos_;
    float drag_start_ratio_;
    float split_ratio_;
    
    ResizeCallback resize_callback_;
};

/**
 * Context menu for region operations
 */
class RegionContextMenu : public UIWidget {
public:
    struct MenuItem {
        std::string text;
        std::string shortcut;
        std::function<void()> callback;
        bool enabled = true;
        bool separator_after = false;
    };
    
    RegionContextMenu(const Point2D& position);
    ~RegionContextMenu() override = default;
    
    // Widget interface
    void render(CanvasRenderer* renderer) override;
    bool handle_event(const InputEvent& event) override;
    
    // Menu functionality
    void add_item(const MenuItem& item);
    void add_separator();
    void clear_items();
    
    void show();
    void hide();
    bool is_visible() const { return visible_ && !items_.empty(); }
    
private:
    void calculate_bounds();
    int get_item_at_point(const Point2D& point) const;
    void render_item(CanvasRenderer* renderer, const MenuItem& item, const Rect2D& item_bounds, bool hovered);
    
    std::vector<MenuItem> items_;
    int hovered_item_;
    float total_width_;
    float total_height_;
    
    static constexpr float ITEM_HEIGHT = 22.0f;
    static constexpr float PADDING = 8.0f;
    static constexpr float SEPARATOR_HEIGHT = 8.0f;
};

/**
 * Widget manager for handling UI widget interactions
 */
class WidgetManager {
public:
    WidgetManager() = default;
    ~WidgetManager() = default;
    
    // Widget management
    void add_widget(std::shared_ptr<UIWidget> widget);
    void remove_widget(std::shared_ptr<UIWidget> widget);
    void clear_widgets();
    
    // Rendering and events
    void render_widgets(CanvasRenderer* renderer);
    bool handle_widget_event(const InputEvent& event);
    
    // Modal widgets (capture all input)
    void set_modal_widget(std::shared_ptr<UIWidget> widget);
    void clear_modal_widget();
    bool has_modal_widget() const { return modal_widget_ != nullptr; }
    
private:
    std::vector<std::shared_ptr<UIWidget>> widgets_;
    std::shared_ptr<UIWidget> modal_widget_;
};

} // namespace voxel_canvas