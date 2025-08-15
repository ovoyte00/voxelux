/*
 * Copyright (C) 2024 Voxelux
 * 
 * Render Block Implementation
 */

#include "canvas_ui/render_block.h"
#include "canvas_ui/styled_widget.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/scaled_theme.h"
#include <iostream>

namespace voxel_canvas {

// Static member initialization
thread_local RenderBlock* RenderBlock::current_block_ = nullptr;

RenderBlock::RenderBlock(const std::string& name) 
    : name_(name) {
}

RenderBlock::~RenderBlock() {
    // Ensure we're not the current block when destroyed
    if (current_block_ == this) {
        current_block_ = nullptr;
    }
}

void RenderBlock::begin() {
    if (phase_ != Phase::Idle) {
        std::cerr << "Warning: RenderBlock::begin() called while not idle" << std::endl;
        return;
    }
    
    phase_ = Phase::Building;
    invalidations_suspended_ = true;
    
    // Set as current block
    current_block_ = this;
    
    // Clear any previous state
    widgets_.clear();
    widgets_needing_style_update_.clear();
    widgets_needing_layout_update_.clear();
}

void RenderBlock::end(const ScaledTheme& theme) {
    if (phase_ != Phase::Building) {
        std::cerr << "Warning: RenderBlock::end() called while not building" << std::endl;
        return;
    }
    
    // No longer the current block
    if (current_block_ == this) {
        current_block_ = nullptr;
    }
    
    // Resume invalidations before resolution
    invalidations_suspended_ = false;
    
    // Resolve all widgets
    resolve_widgets(theme);
    
    phase_ = Phase::Resolved;
}

void RenderBlock::render(CanvasRenderer* renderer) {
    if (phase_ == Phase::Building) {
        std::cerr << "Warning: RenderBlock::render() called while still building" << std::endl;
        // Auto-resolve with default theme
        ScaledTheme theme = renderer->get_scaled_theme();
        end(theme);
    }
    
    if (phase_ != Phase::Resolved) {
        std::cerr << "Warning: RenderBlock::render() called while not resolved" << std::endl;
        return;
    }
    
    // Render the root widget (which renders its children)
    if (root_widget_) {
        root_widget_->render(renderer);
    }
}

void RenderBlock::add_widget(std::shared_ptr<StyledWidget> widget) {
    if (!widget) return;
    
    widgets_.push_back(widget);
    
    // If this widget needs updates, track it
    if (widget->needs_style_computation()) {
        widgets_needing_style_update_.push_back(widget.get());
    }
    if (widget->needs_layout()) {
        widgets_needing_layout_update_.push_back(widget.get());
    }
}

void RenderBlock::set_root_widget(std::shared_ptr<StyledWidget> root) {
    root_widget_ = root;
    
    // Also add to widgets list
    add_widget(root);
    
    // Set bounds if we have them
    if (bounds_.width > 0 && bounds_.height > 0) {
        root->set_bounds(bounds_);
    }
}

void RenderBlock::resolve_widgets(const ScaledTheme& theme) {
    if (!root_widget_) return;
    
    // Phase 1: Compute all styles
    compute_styles_recursive(root_widget_.get(), theme);
    
    // Phase 2: Perform layout
    perform_layout_recursive(root_widget_.get());
}

void RenderBlock::compute_styles_recursive(StyledWidget* widget, const ScaledTheme& theme) {
    if (!widget) return;
    
    // Compute this widget's style
    if (widget->needs_style_computation()) {
        widget->compute_style(theme);
    }
    
    // Recursively compute children's styles
    for (auto& child : widget->get_children()) {
        compute_styles_recursive(child.get(), theme);
    }
}

void RenderBlock::perform_layout_recursive(StyledWidget* widget) {
    if (!widget) return;
    
    // Perform layout for this widget
    if (widget->needs_layout()) {
        widget->perform_layout();
    }
    
    // Recursively layout children
    for (auto& child : widget->get_children()) {
        perform_layout_recursive(child.get());
    }
}

} // namespace voxel_canvas