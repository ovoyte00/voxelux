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

namespace voxel_canvas {

void VoxeluxLayout::render(CanvasRenderer* renderer) {
    // If the menu bar needs to build its items, do it now with a RenderBlock
    if (menu_bar_ && menu_bar_->get_children().empty()) {
        // Use RenderBlock for efficient menu construction
        RenderBlock block("menu-build");
        block.begin();
        
        // Build the menu items
        menu_bar_->build_menus();
        
        // End the block with the proper theme
        ScaledTheme theme = renderer->get_scaled_theme();
        block.end(theme);
    }
    
    // Normal rendering
    Container::render(renderer);
}

} // namespace voxel_canvas