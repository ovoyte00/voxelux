/*
 * Copyright (C) 2024 Voxelux
 * 
 * Drag Manager - Coordinates drag and drop operations for panels
 */

#pragma once

#include "canvas_ui/canvas_core.h"
#include <memory>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class Panel;
class PanelGroup;
class DockColumn;
class DockContainer;
class CanvasRenderer;

/**
 * DragManager - Singleton that coordinates drag and drop operations
 */
class DragManager {
public:
    enum class DragType {
        NONE,
        PANEL,          // Single panel being dragged
        PANEL_GROUP,    // Entire panel group being dragged
        TOOL_PANEL      // Special tool panel drag
    };
    
    enum class DragSource {
        GRIP_HANDLE,    // Dragged by grip handle (whole group)
        PANEL_ICON,     // Dragged by icon (single panel)
        TAB,            // Dragged by tab (single panel)
        FLOATING_HEADER // Dragged from floating window header
    };
    
    struct DragData {
        DragType type = DragType::NONE;
        DragSource source = DragSource::GRIP_HANDLE;
        std::shared_ptr<Panel> panel;
        std::shared_ptr<PanelGroup> panel_group;
        std::shared_ptr<DockColumn> source_column;
        Point2D drag_offset;  // Offset from mouse to panel origin
        Point2D start_pos;    // Starting mouse position
        Rect2D original_bounds;
        bool is_floating = false;
    };
    
    // Singleton access
    static DragManager& get_instance();
    
    // Drag operations
    bool start_drag(const DragData& data, const Point2D& mouse_pos);
    void update_drag(const Point2D& mouse_pos);
    void end_drag(const Point2D& mouse_pos);
    void cancel_drag();
    
    // Query state
    bool is_dragging() const { return drag_data_.type != DragType::NONE; }
    const DragData& get_drag_data() const { return drag_data_; }
    DragType get_drag_type() const { return drag_data_.type; }
    
    // Rendering
    void render_drag_preview(CanvasRenderer* renderer);
    void render_drop_indicators(CanvasRenderer* renderer);
    
    // Drop target management
    void set_hover_container(DockContainer* container) { hover_container_ = container; }
    void clear_hover_container() { hover_container_ = nullptr; }
    
    // Visual feedback
    void set_ghost_opacity(float opacity) { ghost_opacity_ = opacity; }
    float get_ghost_opacity() const { return ghost_opacity_; }
    
    // Callbacks
    using DropCallback = std::function<void(const DragData&, const Point2D&)>;
    void set_drop_callback(DropCallback callback) { drop_callback_ = callback; }
    
private:
    DragManager();
    ~DragManager();
    
    // Prevent copying
    DragManager(const DragManager&) = delete;
    DragManager& operator=(const DragManager&) = delete;
    
    // Current drag state
    DragData drag_data_;
    Point2D current_mouse_pos_;
    bool drag_active_;
    
    // Visual state
    float ghost_opacity_;
    DockContainer* hover_container_;
    
    // Floating window for dragged panel
    std::unique_ptr<class FloatingWindow> floating_preview_;
    
    // Callbacks
    DropCallback drop_callback_;
    
    // Helper methods
    void create_floating_preview();
    void update_floating_preview(const Point2D& mouse_pos);
    void destroy_floating_preview();
    bool is_valid_drop_target(const Point2D& mouse_pos);
};

} // namespace voxel_canvas