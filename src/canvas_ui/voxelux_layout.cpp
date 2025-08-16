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

void VoxeluxLayout::render(CanvasRenderer* renderer) {
    // The menu bar is already built in build_layout(), no need to check here
    // This was causing potential issues with duplicate builds
    
    // Normal rendering
    Container::render(renderer);
}

} // namespace voxel_canvas