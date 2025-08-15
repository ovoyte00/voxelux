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
#include <iostream>

namespace voxel_canvas {

void VoxeluxLayout::render(CanvasRenderer* renderer) {
    // Compute styles with theme
    ScaledTheme theme = renderer->get_scaled_theme();
    compute_style(theme);
    
    // Perform layout if needed
    if (needs_layout_) {
        perform_layout();
    }
    
    // Render
    Container::render(renderer);
}

} // namespace voxel_canvas