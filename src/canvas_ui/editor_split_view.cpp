/*
 * Copyright (C) 2024 Voxelux
 * 
 * Implementation of Editor Split View System
 */

#include "canvas_ui/editor_split_view.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/viewport_3d_editor.h"
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// EditorSplitView implementation

EditorSplitView::EditorSplitView(const Rect2D& bounds)
    : bounds_(bounds) {
    // Create default 3D viewport editor for leaf nodes
    editor_ = std::make_shared<Viewport3DEditor>();
}

EditorSplitView::~EditorSplitView() {
    // Cleanup handled by smart pointers
}

void EditorSplitView::render(CanvasRenderer* renderer) {
    if (split_type_ == SplitType::NONE) {
        // Leaf node - render editor
        if (editor_) {
            editor_->render(renderer, bounds_);
        }
        
        // Draw active indicator
        if (is_active_) {
            const auto& theme = renderer->get_theme();
            renderer->draw_rect_outline(bounds_, theme.accent_primary, 2.0f);
        }
    } else {
        // Branch node - render children
        if (first_child_) {
            first_child_->render(renderer);
        }
        if (second_child_) {
            second_child_->render(renderer);
        }
        
        // Render resize handle
        render_resize_handle(renderer);
    }
}

bool EditorSplitView::handle_event(const InputEvent& event) {
    // Handle resize handle interaction
    if (split_type_ != SplitType::NONE) {
        Rect2D handle_bounds = get_resize_handle_bounds();
        
        if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
            if (is_over_resize_handle(event.mouse_pos)) {
                is_resizing_ = true;
                resize_start_ratio_ = split_ratio_;
                resize_start_pos_ = event.mouse_pos;
                return true;
            }
        } else if (event.type == EventType::MOUSE_RELEASE && event.mouse_button == MouseButton::LEFT) {
            if (is_resizing_) {
                is_resizing_ = false;
                return true;
            }
        } else if (event.type == EventType::MOUSE_MOVE && is_resizing_) {
            // Calculate new split ratio based on mouse position
            float delta;
            if (split_type_ == SplitType::HORIZONTAL) {
                delta = (event.mouse_pos.x - resize_start_pos_.x) / bounds_.width;
            } else {
                delta = (event.mouse_pos.y - resize_start_pos_.y) / bounds_.height;
            }
            
            set_split_ratio(resize_start_ratio_ + delta);
            return true;
        }
        
        // Forward to children
        if (first_child_ && first_child_->handle_event(event)) {
            return true;
        }
        if (second_child_ && second_child_->handle_event(event)) {
            return true;
        }
    } else {
        // Leaf node - forward to editor
        if (editor_) {
            return editor_->handle_event(event, bounds_);
        }
    }
    
    return false;
}

bool EditorSplitView::can_split() const {
    // Can only split leaf nodes with sufficient size
    if (split_type_ != SplitType::NONE) {
        return false;
    }
    
    return bounds_.width >= MIN_VIEW_SIZE * 2 && bounds_.height >= MIN_VIEW_SIZE * 2;
}

void EditorSplitView::split(SplitType type, float ratio) {
    if (!can_split() || type == SplitType::NONE) {
        return;
    }
    
    split_type_ = type;
    split_ratio_ = std::clamp(ratio, 0.1f, 0.9f);
    
    // Create child views
    Rect2D first_bounds = get_first_child_bounds();
    Rect2D second_bounds = get_second_child_bounds();
    
    first_child_ = std::make_unique<EditorSplitView>(first_bounds);
    first_child_->parent_ = this;
    first_child_->editor_ = editor_;  // Transfer editor to first child
    
    second_child_ = std::make_unique<EditorSplitView>(second_bounds);
    second_child_->parent_ = this;
    
    // Clear our editor as we're now a branch node
    editor_.reset();
}

void EditorSplitView::split_at_position(SplitType type, Point2D position) {
    if (!can_split()) {
        return;
    }
    
    float ratio;
    if (type == SplitType::HORIZONTAL) {
        ratio = (position.x - bounds_.x) / bounds_.width;
    } else {
        ratio = (position.y - bounds_.y) / bounds_.height;
    }
    
    split(type, ratio);
}

bool EditorSplitView::can_join() const {
    // Can join if we're a child and our sibling exists
    return parent_ != nullptr && parent_->split_type_ != SplitType::NONE;
}

void EditorSplitView::join_with_sibling() {
    if (!can_join() || !parent_) {
        return;
    }
    
    // Determine which child we are
    bool is_first = (parent_->first_child_.get() == this);
    
    // Get the sibling's editor (if it's a leaf)
    EditorSplitView* sibling = is_first ? parent_->second_child_.get() : parent_->first_child_.get();
    std::shared_ptr<EditorSpace> sibling_editor;
    if (sibling && sibling->split_type_ == SplitType::NONE) {
        sibling_editor = sibling->editor_;
    }
    
    // Convert parent back to leaf node
    parent_->split_type_ = SplitType::NONE;
    parent_->editor_ = is_first ? editor_ : sibling_editor;
    parent_->first_child_.reset();
    parent_->second_child_.reset();
}

void EditorSplitView::set_bounds(const Rect2D& bounds) {
    bounds_ = bounds;
    update_child_bounds();
}

void EditorSplitView::set_split_ratio(float ratio) {
    split_ratio_ = std::clamp(ratio, 0.1f, 0.9f);
    update_child_bounds();
}

EditorSplitView* EditorSplitView::find_view_at(Point2D position) {
    if (!bounds_.contains(position)) {
        return nullptr;
    }
    
    if (split_type_ == SplitType::NONE) {
        return this;
    }
    
    // Check children
    if (first_child_) {
        if (auto* view = first_child_->find_view_at(position)) {
            return view;
        }
    }
    if (second_child_) {
        if (auto* view = second_child_->find_view_at(position)) {
            return view;
        }
    }
    
    return nullptr;
}

void EditorSplitView::set_active(bool active) {
    is_active_ = active;
}

void EditorSplitView::update_child_bounds() {
    if (split_type_ == SplitType::NONE) {
        return;
    }
    
    if (first_child_) {
        first_child_->set_bounds(get_first_child_bounds());
    }
    if (second_child_) {
        second_child_->set_bounds(get_second_child_bounds());
    }
}

Rect2D EditorSplitView::get_first_child_bounds() const {
    if (split_type_ == SplitType::HORIZONTAL) {
        return Rect2D(
            bounds_.x,
            bounds_.y,
            bounds_.width * split_ratio_ - 1,  // Account for resize handle
            bounds_.height
        );
    } else {
        return Rect2D(
            bounds_.x,
            bounds_.y,
            bounds_.width,
            bounds_.height * split_ratio_ - 1  // Account for resize handle
        );
    }
}

Rect2D EditorSplitView::get_second_child_bounds() const {
    if (split_type_ == SplitType::HORIZONTAL) {
        float first_width = bounds_.width * split_ratio_;
        return Rect2D(
            bounds_.x + first_width + 1,  // Account for resize handle
            bounds_.y,
            bounds_.width - first_width - 1,
            bounds_.height
        );
    } else {
        float first_height = bounds_.height * split_ratio_;
        return Rect2D(
            bounds_.x,
            bounds_.y + first_height + 1,  // Account for resize handle
            bounds_.width,
            bounds_.height - first_height - 1
        );
    }
}

Rect2D EditorSplitView::get_resize_handle_bounds() const {
    if (split_type_ == SplitType::HORIZONTAL) {
        float x = bounds_.x + bounds_.width * split_ratio_ - 1;
        return Rect2D(x, bounds_.y, 2, bounds_.height);
    } else {
        float y = bounds_.y + bounds_.height * split_ratio_ - 1;
        return Rect2D(bounds_.x, y, bounds_.width, 2);
    }
}

bool EditorSplitView::is_over_resize_handle(Point2D position) const {
    if (split_type_ == SplitType::NONE) {
        return false;
    }
    
    Rect2D handle = get_resize_handle_bounds();
    // Expand hit area for easier interaction
    handle.x -= 2;
    handle.y -= 2;
    handle.width += 4;
    handle.height += 4;
    
    return handle.contains(position);
}

void EditorSplitView::render_resize_handle(CanvasRenderer* renderer) {
    if (split_type_ == SplitType::NONE) {
        return;
    }
    
    const auto& theme = renderer->get_theme();
    Rect2D handle = get_resize_handle_bounds();
    
    // Draw handle line
    auto handle_color = is_resizing_ ? theme.accent_primary : theme.gray_3;
    
    if (split_type_ == SplitType::HORIZONTAL) {
        renderer->draw_line_batched(
            Point2D(handle.x + 1, handle.y),
            Point2D(handle.x + 1, handle.y + handle.height),
            handle_color,
            1.0f
        );
    } else {
        renderer->draw_line_batched(
            Point2D(handle.x, handle.y + 1),
            Point2D(handle.x + handle.width, handle.y + 1),
            handle_color,
            1.0f
        );
    }
}

void EditorSplitView::render_split_indicator(CanvasRenderer* renderer, Point2D position) {
    // TODO: Render split preview indicator
}

// EditorSplitManager implementation

EditorSplitManager::EditorSplitManager() {
}

EditorSplitManager::~EditorSplitManager() {
}

void EditorSplitManager::initialize(const Rect2D& bounds) {
    root_view_ = std::make_unique<EditorSplitView>(bounds);
    active_view_ = root_view_.get();
    active_view_->set_active(true);
}

void EditorSplitManager::render(CanvasRenderer* renderer) {
    if (root_view_) {
        root_view_->render(renderer);
    }
    
    // Render split preview if active
    if (showing_split_preview_) {
        // TODO: Render split preview
    }
}

bool EditorSplitManager::handle_event(const InputEvent& event) {
    // Handle split shortcuts
    if (event.type == EventType::KEY_PRESS) {
        // Check for split shortcuts (like in Blender)
        // TODO: Implement keyboard shortcuts for splitting
    }
    
    // Update active view on click
    if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
        if (root_view_) {
            if (auto* view = root_view_->find_view_at(event.mouse_pos)) {
                set_active_view(view);
            }
        }
    }
    
    // Forward to root view
    if (root_view_ && root_view_->handle_event(event)) {
        return true;
    }
    
    return false;
}

void EditorSplitManager::split_active_view(SplitDirection direction) {
    if (!active_view_ || !active_view_->can_split()) {
        return;
    }
    
    SplitType type;
    float ratio = 0.5f;
    
    switch (direction) {
        case SplitDirection::LEFT:
            type = SplitType::HORIZONTAL;
            ratio = 0.5f;
            break;
        case SplitDirection::RIGHT:
            type = SplitType::HORIZONTAL;
            ratio = 0.5f;
            break;
        case SplitDirection::TOP:
            type = SplitType::VERTICAL;
            ratio = 0.5f;
            break;
        case SplitDirection::BOTTOM:
            type = SplitType::VERTICAL;
            ratio = 0.5f;
            break;
    }
    
    active_view_->split(type, ratio);
    
    // Set new active view to appropriate child
    if (direction == SplitDirection::RIGHT || direction == SplitDirection::BOTTOM) {
        set_active_view(active_view_->get_second_child());
    } else {
        set_active_view(active_view_->get_first_child());
    }
}

void EditorSplitManager::join_active_view() {
    if (active_view_ && active_view_->can_join()) {
        EditorSplitView* parent = active_view_->get_parent();
        active_view_->join_with_sibling();
        set_active_view(parent);
    }
}

void EditorSplitManager::set_active_view(EditorSplitView* view) {
    if (active_view_) {
        active_view_->set_active(false);
    }
    
    active_view_ = view;
    
    if (active_view_) {
        active_view_->set_active(true);
    }
}

void EditorSplitManager::set_bounds(const Rect2D& bounds) {
    if (root_view_) {
        root_view_->set_bounds(bounds);
    }
}

Rect2D EditorSplitManager::get_bounds() const {
    if (root_view_) {
        return root_view_->get_bounds();
    }
    return Rect2D();
}

} // namespace voxel_canvas