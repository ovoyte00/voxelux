/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Interactive UI Widgets Implementation
 */

#include "canvas_ui/ui_widgets.h"
#include "canvas_ui/canvas_renderer.h"
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// EditorTypeDropdown implementation

EditorTypeDropdown::EditorTypeDropdown(const Rect2D& bounds, EditorType current_type)
    : UIWidget(bounds)
    , selected_type_(current_type)
    , dropdown_open_(false)
    , button_hovered_(false)
    , button_pressed_(false)
    , hovered_item_(-1) {
    
    // Get all available editor types
    available_types_ = EditorFactory::get_available_editor_types();
}

void EditorTypeDropdown::render(CanvasRenderer* renderer) {
    if (!visible_) return;
    
    render_button(renderer);
    
    if (dropdown_open_) {
        render_dropdown_list(renderer);
    }
}

bool EditorTypeDropdown::handle_event(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    if (dropdown_open_) {
        // Handle dropdown list events first (higher priority)
        if (handle_dropdown_event(event)) {
            return true;
        }
    }
    
    // Handle button events
    return handle_button_event(event);
}

void EditorTypeDropdown::set_selected_type(EditorType type) {
    selected_type_ = type;
    dropdown_open_ = false; // Close dropdown when selection changes
}

void EditorTypeDropdown::show_dropdown() {
    dropdown_open_ = true;
    hovered_item_ = -1;
}

void EditorTypeDropdown::hide_dropdown() {
    dropdown_open_ = false;
    hovered_item_ = -1;
}

void EditorTypeDropdown::render_button(CanvasRenderer* renderer) {
    const CanvasTheme& theme = renderer->get_theme();
    
    // Button background
    ColorRGBA bg_color = theme.widget_normal;
    if (button_pressed_) {
        bg_color = theme.widget_active;
    } else if (button_hovered_) {
        bg_color = theme.widget_hover;
    }
    
    renderer->draw_rect(bounds_, bg_color);
    renderer->draw_rect_outline(bounds_, theme.border_normal, 1.0f);
    
    // Button text
    std::string text = EditorFactory::get_editor_name(selected_type_);
    Point2D text_pos(bounds_.x + 8, bounds_.y + 6);
    renderer->draw_text(text, text_pos, theme.text_primary, 11.0f);
    
    // Dropdown arrow
    Point2D arrow_pos(bounds_.x + bounds_.width - 16, bounds_.y + 8);
    std::string arrow = dropdown_open_ ? "^" : "v";
    renderer->draw_text(arrow, arrow_pos, theme.text_primary, 10.0f);
}

void EditorTypeDropdown::render_dropdown_list(CanvasRenderer* renderer) {
    const CanvasTheme& theme = renderer->get_theme();
    Rect2D dropdown_bounds = get_dropdown_bounds();
    
    // Dropdown background
    renderer->draw_rect(dropdown_bounds, theme.widget_normal);
    renderer->draw_rect_outline(dropdown_bounds, theme.border_normal, 1.0f);
    
    // Render items
    float item_y = dropdown_bounds.y;
    for (size_t i = 0; i < available_types_.size(); i++) {
        Rect2D item_bounds(dropdown_bounds.x, item_y, dropdown_bounds.width, ITEM_HEIGHT);
        
        // Item background
        ColorRGBA item_bg = theme.widget_normal;
        if (static_cast<int>(i) == hovered_item_) {
            item_bg = theme.accent_selection;
        } else if (available_types_[i] == selected_type_) {
            item_bg = theme.widget_hover;
        }
        
        renderer->draw_rect(item_bounds, item_bg);
        
        // Item text
        std::string item_text = EditorFactory::get_editor_name(available_types_[i]);
        Point2D text_pos(item_bounds.x + 8, item_bounds.y + 4);
        ColorRGBA text_color = (static_cast<int>(i) == hovered_item_) ? 
            ColorRGBA::white() : theme.text_primary;
        renderer->draw_text(item_text, text_pos, text_color, 11.0f);
        
        item_y += ITEM_HEIGHT;
    }
}

bool EditorTypeDropdown::handle_button_event(const InputEvent& event) {
    bool button_contains = bounds_.contains(event.mouse_pos);
    
    switch (event.type) {
        case EventType::MOUSE_MOVE:
            button_hovered_ = button_contains;
            return button_contains;
            
        case EventType::MOUSE_PRESS:
            if (button_contains && event.mouse_button == MouseButton::LEFT) {
                button_pressed_ = true;
                return true;
            }
            break;
            
        case EventType::MOUSE_RELEASE:
            if (button_pressed_ && event.mouse_button == MouseButton::LEFT) {
                button_pressed_ = false;
                if (button_contains) {
                    // Toggle dropdown
                    if (dropdown_open_) {
                        hide_dropdown();
                    } else {
                        show_dropdown();
                    }
                } else if (dropdown_open_) {
                    hide_dropdown();
                }
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

bool EditorTypeDropdown::handle_dropdown_event(const InputEvent& event) {
    Rect2D dropdown_bounds = get_dropdown_bounds();
    bool dropdown_contains = dropdown_bounds.contains(event.mouse_pos);
    
    switch (event.type) {
        case EventType::MOUSE_MOVE:
            if (dropdown_contains) {
                hovered_item_ = get_item_at_point(event.mouse_pos);
                return true;
            } else {
                hovered_item_ = -1;
                return false;
            }
            
        case EventType::MOUSE_PRESS:
            if (!dropdown_contains && event.mouse_button == MouseButton::LEFT) {
                hide_dropdown();
                return false; // Allow event to propagate to close dropdown
            }
            return dropdown_contains;
            
        case EventType::MOUSE_RELEASE:
            if (dropdown_contains && event.mouse_button == MouseButton::LEFT) {
                int item_index = get_item_at_point(event.mouse_pos);
                if (item_index >= 0 && item_index < static_cast<int>(available_types_.size())) {
                    EditorType new_type = available_types_[item_index];
                    set_selected_type(new_type);
                    
                    if (selection_callback_) {
                        selection_callback_(new_type);
                    }
                }
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return dropdown_contains;
}

Rect2D EditorTypeDropdown::get_dropdown_bounds() const {
    float dropdown_height = std::min(
        static_cast<float>(available_types_.size()) * ITEM_HEIGHT,
        DROPDOWN_MAX_HEIGHT
    );
    
    return Rect2D(
        bounds_.x,
        bounds_.y + bounds_.height,
        bounds_.width,
        dropdown_height
    );
}

int EditorTypeDropdown::get_item_at_point(const Point2D& point) const {
    Rect2D dropdown_bounds = get_dropdown_bounds();
    if (!dropdown_bounds.contains(point)) {
        return -1;
    }
    
    float relative_y = point.y - dropdown_bounds.y;
    int item_index = static_cast<int>(relative_y / ITEM_HEIGHT);
    
    if (item_index >= 0 && item_index < static_cast<int>(available_types_.size())) {
        return item_index;
    }
    
    return -1;
}

// RegionSplitter implementation

RegionSplitter::RegionSplitter(const Rect2D& bounds, Orientation orientation)
    : UIWidget(bounds)
    , orientation_(orientation)
    , is_dragging_(false)
    , is_hovered_(false)
    , drag_start_pos_(0, 0)
    , drag_start_ratio_(0.5f)
    , split_ratio_(0.5f) {
}

void RegionSplitter::render(CanvasRenderer* renderer) {
    if (!visible_) return;
    
    const CanvasTheme& theme = renderer->get_theme();
    
    // Splitter background
    ColorRGBA splitter_color = theme.border_normal;
    if (is_hovered_ || is_dragging_) {
        splitter_color = theme.accent_selection;
    }
    
    renderer->draw_rect(bounds_, splitter_color);
}

bool RegionSplitter::handle_event(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool contains = bounds_.contains(event.mouse_pos);
    
    switch (event.type) {
        case EventType::MOUSE_MOVE:
            is_hovered_ = contains;
            
            if (is_dragging_) {
                // Calculate new split ratio
                float delta = (orientation_ == Orientation::HORIZONTAL) ?
                    (event.mouse_pos.y - drag_start_pos_.y) :
                    (event.mouse_pos.x - drag_start_pos_.x);
                
                float parent_size = (orientation_ == Orientation::HORIZONTAL) ?
                    bounds_.height : bounds_.width;
                
                if (parent_size > 0) {
                    float new_ratio = drag_start_ratio_ + (delta / parent_size);
                    split_ratio_ = std::clamp(new_ratio, 0.1f, 0.9f);
                    
                    if (resize_callback_) {
                        resize_callback_(split_ratio_);
                    }
                }
                return true;
            }
            
            return contains;
            
        case EventType::MOUSE_PRESS:
            if (contains && event.mouse_button == MouseButton::LEFT) {
                is_dragging_ = true;
                drag_start_pos_ = event.mouse_pos;
                drag_start_ratio_ = split_ratio_;
                return true;
            }
            break;
            
        case EventType::MOUSE_RELEASE:
            if (is_dragging_ && event.mouse_button == MouseButton::LEFT) {
                is_dragging_ = false;
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

void RegionSplitter::update_cursor() {
    // TODO: Set appropriate cursor based on orientation
    // This would require cursor management in the window system
}

// RegionContextMenu implementation

RegionContextMenu::RegionContextMenu(const Point2D& position)
    : UIWidget(Rect2D(position.x, position.y, 0, 0))
    , hovered_item_(-1)
    , total_width_(0)
    , total_height_(0) {
    
    visible_ = false; // Start hidden
}

void RegionContextMenu::render(CanvasRenderer* renderer) {
    if (!visible_ || items_.empty()) return;
    
    const CanvasTheme& theme = renderer->get_theme();
    
    // Menu background
    renderer->draw_rect(bounds_, theme.widget_normal);
    renderer->draw_rect_outline(bounds_, theme.border_normal, 1.0f);
    
    // Render items
    float item_y = bounds_.y + PADDING;
    int item_index = 0;
    
    for (const auto& item : items_) {
        if (item.text.empty() && item.separator_after) {
            // Separator
            Rect2D sep_bounds(bounds_.x + PADDING, item_y, bounds_.width - PADDING * 2, 1);
            renderer->draw_rect(sep_bounds, theme.border_normal);
            item_y += SEPARATOR_HEIGHT;
        } else {
            // Regular item
            Rect2D item_bounds(bounds_.x, item_y, bounds_.width, ITEM_HEIGHT);
            render_item(renderer, item, item_bounds, item_index == hovered_item_);
            item_y += ITEM_HEIGHT;
            
            if (item.separator_after) {
                Rect2D sep_bounds(bounds_.x + PADDING, item_y, bounds_.width - PADDING * 2, 1);
                renderer->draw_rect(sep_bounds, theme.border_normal);
                item_y += SEPARATOR_HEIGHT;
            }
        }
        
        item_index++;
    }
}

bool RegionContextMenu::handle_event(const InputEvent& event) {
    if (!visible_ || items_.empty()) return false;
    
    bool contains = bounds_.contains(event.mouse_pos);
    
    switch (event.type) {
        case EventType::MOUSE_MOVE:
            if (contains) {
                hovered_item_ = get_item_at_point(event.mouse_pos);
                return true;
            } else {
                hovered_item_ = -1;
                return false;
            }
            
        case EventType::MOUSE_PRESS:
            if (!contains && event.mouse_button == MouseButton::LEFT) {
                hide(); // Close menu when clicking outside
                return false;
            }
            return contains;
            
        case EventType::MOUSE_RELEASE:
            if (contains && event.mouse_button == MouseButton::LEFT) {
                int item_index = get_item_at_point(event.mouse_pos);
                if (item_index >= 0 && item_index < static_cast<int>(items_.size())) {
                    const auto& item = items_[item_index];
                    if (item.enabled && item.callback) {
                        item.callback();
                    }
                }
                hide();
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return contains;
}

void RegionContextMenu::add_item(const MenuItem& item) {
    items_.push_back(item);
    calculate_bounds();
}

void RegionContextMenu::add_separator() {
    if (!items_.empty()) {
        items_.back().separator_after = true;
        calculate_bounds();
    }
}

void RegionContextMenu::clear_items() {
    items_.clear();
    total_width_ = 0;
    total_height_ = 0;
}

void RegionContextMenu::show() {
    if (!items_.empty()) {
        visible_ = true;
    }
}

void RegionContextMenu::hide() {
    visible_ = false;
    hovered_item_ = -1;
}

void RegionContextMenu::calculate_bounds() {
    if (items_.empty()) {
        total_width_ = 0;
        total_height_ = 0;
        return;
    }
    
    // Calculate required width and height
    total_width_ = 150.0f; // Minimum width
    total_height_ = PADDING * 2;
    
    for (const auto& item : items_) {
        if (!item.text.empty()) {
            total_height_ += ITEM_HEIGHT;
            // TODO: Measure text width properly
            float text_width = item.text.length() * 8.0f + item.shortcut.length() * 6.0f;
            total_width_ = std::max(total_width_, text_width + PADDING * 3);
        }
        
        if (item.separator_after) {
            total_height_ += SEPARATOR_HEIGHT;
        }
    }
    
    bounds_.width = total_width_;
    bounds_.height = total_height_;
}

int RegionContextMenu::get_item_at_point(const Point2D& point) const {
    if (!bounds_.contains(point)) return -1;
    
    float item_y = bounds_.y + PADDING;
    int item_index = 0;
    
    for (const auto& item : items_) {
        if (!item.text.empty()) {
            if (point.y >= item_y && point.y < item_y + ITEM_HEIGHT) {
                return item_index;
            }
            item_y += ITEM_HEIGHT;
        }
        
        if (item.separator_after) {
            item_y += SEPARATOR_HEIGHT;
        }
        
        item_index++;
    }
    
    return -1;
}

void RegionContextMenu::render_item(CanvasRenderer* renderer, const MenuItem& item, 
                                   const Rect2D& item_bounds, bool hovered) {
    const CanvasTheme& theme = renderer->get_theme();
    
    // Item background
    if (hovered && item.enabled) {
        renderer->draw_rect(item_bounds, theme.accent_selection);
    }
    
    // Item text
    ColorRGBA text_color = item.enabled ? 
        (hovered ? ColorRGBA::white() : theme.text_primary) : 
        theme.text_secondary;
    
    Point2D text_pos(item_bounds.x + PADDING, item_bounds.y + 4);
    renderer->draw_text(item.text, text_pos, text_color, 11.0f);
    
    // Shortcut text
    if (!item.shortcut.empty()) {
        Point2D shortcut_pos(item_bounds.x + item_bounds.width - PADDING - item.shortcut.length() * 6.0f, 
                            item_bounds.y + 4);
        ColorRGBA shortcut_color = item.enabled ? theme.text_secondary : theme.text_secondary;
        renderer->draw_text(item.shortcut, shortcut_pos, shortcut_color, 10.0f);
    }
}

// WidgetManager implementation

void WidgetManager::add_widget(std::shared_ptr<UIWidget> widget) {
    widgets_.push_back(widget);
}

void WidgetManager::remove_widget(std::shared_ptr<UIWidget> widget) {
    widgets_.erase(
        std::remove(widgets_.begin(), widgets_.end(), widget),
        widgets_.end()
    );
}

void WidgetManager::clear_widgets() {
    widgets_.clear();
}

void WidgetManager::render_widgets(CanvasRenderer* renderer) {
    // Render modal widget last (on top)
    for (const auto& widget : widgets_) {
        if (widget != modal_widget_ && widget->is_visible()) {
            widget->render(renderer);
        }
    }
    
    if (modal_widget_ && modal_widget_->is_visible()) {
        modal_widget_->render(renderer);
    }
}

bool WidgetManager::handle_widget_event(const InputEvent& event) {
    // Modal widget gets first priority
    if (modal_widget_ && modal_widget_->is_visible()) {
        return modal_widget_->handle_event(event);
    }
    
    // Handle widgets in reverse order (top to bottom)
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        if ((*it)->is_visible() && (*it)->handle_event(event)) {
            return true;
        }
    }
    
    return false;
}

void WidgetManager::set_modal_widget(std::shared_ptr<UIWidget> widget) {
    modal_widget_ = widget;
}

void WidgetManager::clear_modal_widget() {
    modal_widget_.reset();
}

} // namespace voxel_canvas