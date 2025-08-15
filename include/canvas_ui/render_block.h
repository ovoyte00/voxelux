/*
 * Copyright (C) 2024 Voxelux
 * 
 * Render Block - Batched widget construction and rendering system
 * 
 * This system provides a three-phase approach to UI rendering:
 * 1. Construction Phase: Widgets are added without triggering updates
 * 2. Resolution Phase: Layout and styles are computed in batch
 * 3. Rendering Phase: Actual GPU rendering happens
 */

#pragma once

#include "canvas_core.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace voxel_canvas {

// Forward declarations
class StyledWidget;
class CanvasRenderer;
class ScaledTheme;

/**
 * RenderBlock - Container for batched widget operations
 * 
 * Allows efficient construction of UI hierarchies without triggering
 * multiple layout recalculations or renders during setup.
 */
class RenderBlock {
public:
    enum class Phase {
        Idle,           // Not active
        Building,       // Accepting widget operations
        Resolved,       // Layout/styles computed, ready to render
    };
    
    explicit RenderBlock(const std::string& name = "");
    ~RenderBlock();
    
    // Block lifecycle
    void begin();                                       // Start building phase
    void end(const ScaledTheme& theme);               // Resolve layout and styles
    void render(CanvasRenderer* renderer);            // Render all widgets
    
    // Widget management
    void add_widget(std::shared_ptr<StyledWidget> widget);
    void set_root_widget(std::shared_ptr<StyledWidget> root);
    
    // State queries
    bool is_building() const { return phase_ == Phase::Building; }
    bool is_resolved() const { return phase_ == Phase::Resolved; }
    Phase get_phase() const { return phase_; }
    
    // Bounds
    void set_bounds(const Rect2D& bounds) { bounds_ = bounds; }
    const Rect2D& get_bounds() const { return bounds_; }
    
    // Invalidation control
    void suspend_invalidations() { invalidations_suspended_ = true; }
    void resume_invalidations() { invalidations_suspended_ = false; }
    bool are_invalidations_suspended() const { return invalidations_suspended_; }
    
    // Name/ID
    const std::string& get_name() const { return name_; }
    
    // Static: Get current active block (for widgets to check)
    static RenderBlock* get_current() { return current_block_; }
    
private:
    // Resolve all widget layouts and styles
    void resolve_widgets(const ScaledTheme& theme);
    void compute_styles_recursive(StyledWidget* widget, const ScaledTheme& theme);
    void perform_layout_recursive(StyledWidget* widget);
    
private:
    std::string name_;
    Phase phase_ = Phase::Idle;
    Rect2D bounds_;
    
    // Widget hierarchy
    std::shared_ptr<StyledWidget> root_widget_;
    std::vector<std::shared_ptr<StyledWidget>> widgets_;  // All widgets for iteration
    
    // Invalidation control
    bool invalidations_suspended_ = false;
    
    // Track widgets that need updates
    std::vector<StyledWidget*> widgets_needing_style_update_;
    std::vector<StyledWidget*> widgets_needing_layout_update_;
    
    // Static current block for context
    static thread_local RenderBlock* current_block_;
};

/**
 * RAII helper for render block scope
 */
class RenderBlockScope {
public:
    explicit RenderBlockScope(RenderBlock& block) : block_(block) {
        block_.begin();
    }
    
    ~RenderBlockScope() {
        // Don't automatically end - let user control when to resolve
    }
    
    RenderBlock& block() { return block_; }
    
private:
    RenderBlock& block_;
};

} // namespace voxel_canvas