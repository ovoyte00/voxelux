/*
 * Copyright (C) 2024 Voxelux
 * 
 * Implementation of Voxelux main application layout
 * Methods that can't be inline in the header
 */

#include "canvas_ui/voxelux_layout.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/viewport_3d_editor.h"
#include "canvas_ui/icon_system.h"
#include "canvas_ui/drag_manager.h"
#include "canvas_ui/floating_window.h"
#include "canvas_ui/scaled_theme.h"
#include "canvas_ui/styled_widgets.h"
#include "canvas_ui/render_block.h"
#include <iostream>
#include <chrono>

namespace voxel_canvas {

// External flags to enable resize debugging
extern bool has_resized;
extern bool has_resized_2;
extern bool has_resized_3;
extern bool has_resized_4;

void VoxeluxLayout::on_window_resize(float width, float height) {
    if (window_width_ != width || window_height_ != height) {
        std::cout << "\n=== WINDOW RESIZE: " << window_width_ << "x" << window_height_ 
                  << " -> " << width << "x" << height << " ===" << std::endl;
        
        // Enable debug flags in styled_widget.cpp
        has_resized = true;
        has_resized_2 = true;
        has_resized_3 = true;
        has_resized_4 = true;
        
        window_width_ = width;
        window_height_ = height;
        
        // Industry-standard approach: immediate full refresh on resize
        // Set new bounds for the root
        set_bounds(Rect2D(0, 0, width, height));
        
        invalidate_style();  // This cascades to all children
        invalidate_layout(); // This cascades to all children
        needs_immediate_refresh_ = true;
    }
}

// Removed - now handled in base StyledWidget::invalidate_layout()

void VoxeluxLayout::render(CanvasRenderer* renderer) {
    // The menu bar is already built in build_layout(), no need to check here
    // This was causing potential issues with duplicate builds
    
    // Normal rendering
    Container::render(renderer);
}

} // namespace voxel_canvas