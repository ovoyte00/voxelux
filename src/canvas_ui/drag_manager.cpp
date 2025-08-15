/*
 * Copyright (C) 2024 Voxelux
 * 
 * Implementation of DragManager
 */

#include "canvas_ui/drag_manager.h"
#include "canvas_ui/floating_window.h"
#include "canvas_ui/dock_container.h"
#include "canvas_ui/canvas_renderer.h"
#include <iostream>

namespace voxel_canvas {

DragManager& DragManager::get_instance() {
    static DragManager instance;
    return instance;
}

DragManager::DragManager()
    : drag_active_(false)
    , ghost_opacity_(0.5f)
    , hover_container_(nullptr) {
}

DragManager::~DragManager() = default;

bool DragManager::start_drag(const DragData& data, const Point2D& mouse_pos) {
    if (is_dragging()) {
        return false;  // Already dragging something
    }
    
    drag_data_ = data;
    drag_data_.start_pos = mouse_pos;
    current_mouse_pos_ = mouse_pos;
    drag_active_ = true;
    
    // Calculate drag offset if not set
    if (drag_data_.drag_offset.x == 0 && drag_data_.drag_offset.y == 0) {
        drag_data_.drag_offset = Point2D(
            mouse_pos.x - drag_data_.original_bounds.x,
            mouse_pos.y - drag_data_.original_bounds.y
        );
    }
    
    // Create floating preview
    create_floating_preview();
    
    std::cout << "DragManager: Started drag of type " << static_cast<int>(drag_data_.type) << std::endl;
    
    return true;
}

void DragManager::update_drag(const Point2D& mouse_pos) {
    if (!is_dragging()) return;
    
    current_mouse_pos_ = mouse_pos;
    
    // Update floating preview position
    update_floating_preview(mouse_pos);
    
    // Check if over a valid drop target
    if (is_valid_drop_target(mouse_pos)) {
        ghost_opacity_ = 0.5f;  // Ghost mode when over valid target
    } else {
        ghost_opacity_ = 1.0f;  // Solid when floating
    }
}

void DragManager::end_drag(const Point2D& mouse_pos) {
    if (!is_dragging()) return;
    
    current_mouse_pos_ = mouse_pos;
    
    // Check if we have a valid drop target
    bool dropped = false;
    if (drop_callback_ && is_valid_drop_target(mouse_pos)) {
        drop_callback_(drag_data_, mouse_pos);
        dropped = true;
    }
    
    // If not dropped on a valid target, leave as floating window
    if (!dropped && floating_preview_) {
        // Create permanent floating window from drag data
        auto& manager = FloatingWindowManager::get_instance();
        
        if (drag_data_.type == DragType::PANEL && drag_data_.panel) {
            manager.float_panel(drag_data_.panel, mouse_pos);
        } else if (drag_data_.type == DragType::PANEL_GROUP && drag_data_.panel_group) {
            manager.float_panel_group(drag_data_.panel_group, mouse_pos);
        }
    }
    
    // Clean up
    destroy_floating_preview();
    drag_data_ = DragData();
    drag_active_ = false;
    hover_container_ = nullptr;
    
    std::cout << "DragManager: Ended drag" << (dropped ? " (dropped)" : " (floating)") << std::endl;
}

void DragManager::cancel_drag() {
    if (!is_dragging()) return;
    
    // Return to original position if possible
    destroy_floating_preview();
    
    // Clean up
    drag_data_ = DragData();
    drag_active_ = false;
    hover_container_ = nullptr;
    
    std::cout << "DragManager: Cancelled drag" << std::endl;
}

void DragManager::render_drag_preview(CanvasRenderer* renderer) {
    if (!is_dragging() || !floating_preview_) return;
    
    // Set ghost mode based on drop target
    floating_preview_->set_ghost_mode(ghost_opacity_ < 1.0f);
    floating_preview_->set_opacity(ghost_opacity_);
    
    // Render the floating preview
    floating_preview_->render(renderer);
}

void DragManager::render_drop_indicators(CanvasRenderer* renderer) {
    if (!is_dragging() || !hover_container_) return;
    
    // Get drop target from hover container
    bool is_tool = (drag_data_.type == DragType::TOOL_PANEL);
    auto drop_target = hover_container_->get_drop_target(current_mouse_pos_, is_tool);
    
    if (drop_target.zone == DockContainer::DropZone::NONE) return;
    
    const auto& theme = renderer->get_theme();
    
    // Render drop indicator based on zone type
    switch (drop_target.zone) {
        case DockContainer::DropZone::BEFORE_COLUMN:
        case DockContainer::DropZone::AFTER_COLUMN:
        case DockContainer::DropZone::EDGE:
            // Vertical line indicator
            renderer->draw_line(
                Point2D(drop_target.preview_bounds.x, drop_target.preview_bounds.y),
                Point2D(drop_target.preview_bounds.x, drop_target.preview_bounds.y + drop_target.preview_bounds.height),
                theme.accent_primary,
                3.0f
            );
            break;
            
        case DockContainer::DropZone::INTO_COLUMN:
            // Ghost preview of where panel would appear
            renderer->draw_rect(
                drop_target.preview_bounds,
                ColorRGBA(theme.accent_primary.r, theme.accent_primary.g, theme.accent_primary.b, 0.3f)
            );
            break;
            
        case DockContainer::DropZone::INTO_GROUP:
            // Outline around target group
            renderer->draw_rect_outline(
                drop_target.preview_bounds,
                theme.accent_primary,
                2.0f
            );
            break;
            
        case DockContainer::DropZone::BETWEEN_GROUPS:
            // Horizontal line between groups
            renderer->draw_line(
                Point2D(drop_target.preview_bounds.x, drop_target.preview_bounds.y),
                Point2D(drop_target.preview_bounds.x + drop_target.preview_bounds.width, drop_target.preview_bounds.y),
                theme.accent_primary,
                3.0f
            );
            break;
            
        default:
            break;
    }
}

void DragManager::create_floating_preview() {
    if (!drag_data_.panel && !drag_data_.panel_group) return;
    
    // Calculate initial bounds for floating window
    Rect2D window_bounds = drag_data_.original_bounds;
    window_bounds.x = current_mouse_pos_.x - drag_data_.drag_offset.x;
    window_bounds.y = current_mouse_pos_.y - drag_data_.drag_offset.y;
    
    // Add space for utility header
    window_bounds.height += FloatingWindow::HEADER_HEIGHT;
    
    // Create floating window
    floating_preview_ = std::make_unique<FloatingWindow>(window_bounds);
    
    // Set content based on drag type
    if (drag_data_.type == DragType::PANEL || drag_data_.type == DragType::TOOL_PANEL) {
        // Create a temporary panel group for single panel
        auto temp_group = std::make_shared<PanelGroup>();
        temp_group->add_panel(drag_data_.panel);
        floating_preview_->set_panel_group(temp_group);
        
        if (drag_data_.panel) {
            floating_preview_->set_title(drag_data_.panel->get_bounds().width > 0 ? "Panel" : "");
        }
    } else if (drag_data_.type == DragType::PANEL_GROUP) {
        floating_preview_->set_panel_group(drag_data_.panel_group);
        floating_preview_->set_title("Panel Group");
    }
    
    // Set initial ghost mode
    floating_preview_->set_ghost_mode(true);
    floating_preview_->set_opacity(ghost_opacity_);
}

void DragManager::update_floating_preview(const Point2D& mouse_pos) {
    if (!floating_preview_) return;
    
    // Update position based on mouse and drag offset
    Rect2D new_bounds = floating_preview_->get_bounds();
    new_bounds.x = mouse_pos.x - drag_data_.drag_offset.x;
    new_bounds.y = mouse_pos.y - drag_data_.drag_offset.y;
    
    floating_preview_->set_bounds(new_bounds);
}

void DragManager::destroy_floating_preview() {
    floating_preview_.reset();
}

bool DragManager::is_valid_drop_target(const Point2D& mouse_pos) {
    if (!hover_container_) return false;
    
    bool is_tool = (drag_data_.type == DragType::TOOL_PANEL);
    auto drop_target = hover_container_->get_drop_target(mouse_pos, is_tool);
    
    return drop_target.zone != DockContainer::DropZone::NONE;
}

} // namespace voxel_canvas