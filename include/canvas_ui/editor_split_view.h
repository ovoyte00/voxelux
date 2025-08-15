/*
 * Copyright (C) 2024 Voxelux
 * 
 * Editor Split View System
 * Allows splitting editor views horizontally and vertically like Blender
 */

#pragma once

#include "canvas_core.h"
#include "canvas_region.h"
#include <memory>
#include <vector>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;
class Viewport3DEditor;
class EditorSpace;

enum class SplitType {
    NONE,
    HORIZONTAL,  // Split horizontally (left/right)
    VERTICAL     // Split vertically (top/bottom)
};

enum class SplitDirection {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

/**
 * Editor Split View - Container that can split into multiple editor views
 * Similar to Blender's area splitting system
 */
class EditorSplitView {
public:
    EditorSplitView(const Rect2D& bounds);
    ~EditorSplitView();
    
    // Rendering
    void render(CanvasRenderer* renderer);
    
    // Event handling
    bool handle_event(const InputEvent& event);
    
    // Split operations
    bool can_split() const;
    void split(SplitType type, float ratio = 0.5f);
    void split_at_position(SplitType type, Point2D position);
    
    // Join/merge operations
    bool can_join() const;
    void join_with_sibling();
    
    // Bounds management
    void set_bounds(const Rect2D& bounds);
    Rect2D get_bounds() const { return bounds_; }
    
    // Editor management
    void set_editor(std::shared_ptr<EditorSpace> editor);
    std::shared_ptr<EditorSpace> get_editor() { return editor_; }
    
    // Split management
    bool is_split() const { return split_type_ != SplitType::NONE; }
    SplitType get_split_type() const { return split_type_; }
    float get_split_ratio() const { return split_ratio_; }
    void set_split_ratio(float ratio);
    
    // Child access
    EditorSplitView* get_first_child() { return first_child_.get(); }
    EditorSplitView* get_second_child() { return second_child_.get(); }
    EditorSplitView* get_parent() { return parent_; }
    
    // Find split view at position
    EditorSplitView* find_view_at(Point2D position);
    
    // Active view management
    void set_active(bool active);
    bool is_active() const { return is_active_; }
    
    // Minimum size constraints
    static constexpr float MIN_VIEW_SIZE = 100.0f;
    
private:
    // Split state
    SplitType split_type_ = SplitType::NONE;
    float split_ratio_ = 0.5f;
    
    // Hierarchy
    EditorSplitView* parent_ = nullptr;
    std::unique_ptr<EditorSplitView> first_child_;
    std::unique_ptr<EditorSplitView> second_child_;
    
    // Editor content (only for leaf nodes)
    std::shared_ptr<EditorSpace> editor_;
    
    // Layout
    Rect2D bounds_;
    bool is_active_ = false;
    
    // Resize handle interaction
    bool is_resizing_ = false;
    float resize_start_ratio_ = 0.5f;
    Point2D resize_start_pos_;
    
    // Helper methods
    void update_child_bounds();
    Rect2D get_first_child_bounds() const;
    Rect2D get_second_child_bounds() const;
    Rect2D get_resize_handle_bounds() const;
    bool is_over_resize_handle(Point2D position) const;
    
    // Rendering helpers
    void render_resize_handle(CanvasRenderer* renderer);
    void render_split_indicator(CanvasRenderer* renderer, Point2D position);
};

/**
 * Editor Split Manager - Manages the root split view and provides high-level operations
 */
class EditorSplitManager {
public:
    EditorSplitManager();
    ~EditorSplitManager();
    
    // Initialize with bounds
    void initialize(const Rect2D& bounds);
    
    // Rendering
    void render(CanvasRenderer* renderer);
    
    // Event handling
    bool handle_event(const InputEvent& event);
    
    // Split operations
    void split_active_view(SplitDirection direction);
    void join_active_view();
    
    // Active view management
    void set_active_view(EditorSplitView* view);
    EditorSplitView* get_active_view() { return active_view_; }
    
    // Bounds management
    void set_bounds(const Rect2D& bounds);
    Rect2D get_bounds() const;
    
    // Factory for creating editors
    using EditorFactory = std::function<std::shared_ptr<EditorSpace>(const Rect2D&)>;
    void set_editor_factory(EditorFactory factory) { editor_factory_ = factory; }
    
private:
    std::unique_ptr<EditorSplitView> root_view_;
    EditorSplitView* active_view_ = nullptr;
    EditorFactory editor_factory_;
    
    // Split preview state
    bool showing_split_preview_ = false;
    SplitType preview_split_type_ = SplitType::NONE;
    Point2D preview_position_;
};

} // namespace voxel_canvas