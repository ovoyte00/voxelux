/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Main window and region management
 * Professional OpenGL-based windowing system for voxel editing.
 */

#pragma once

#include "canvas_core.h"
#include <memory>
#include <vector>
#include <unordered_map>

// Forward declare GLFW types to avoid including GLFW in header
struct GLFWwindow;

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;
class EventRouter;
class RegionManager;
class WorkspaceManager;

/**
 * Main application window that hosts the Canvas UI system.
 * Manages OpenGL context, input events, and the overall layout.
 */
class CanvasWindow {
public:
    CanvasWindow(int width = 1600, int height = 1000, const std::string& title = "Voxelux");
    ~CanvasWindow();
    
    // Window lifecycle
    bool initialize();
    void shutdown();
    bool should_close() const;
    void swap_buffers();
    void poll_events();
    
    // Window properties
    void set_title(const std::string& title);
    Point2D get_size() const;
    void set_size(int width, int height);
    Point2D get_framebuffer_size() const;
    float get_content_scale() const;
    
    // Main render loop
    void render_frame();
    
    // Component access
    CanvasRenderer* get_renderer() const { return renderer_.get(); }
    EventRouter* get_event_router() const { return event_router_.get(); }
    RegionManager* get_region_manager() const { return region_manager_.get(); }
    WorkspaceManager* get_workspace_manager() const { return workspace_manager_.get(); }
    
    // System access
    GLFWwindow* get_glfw_handle() const { return window_; }
    
    // Cursor control for infinite mouse dragging
    void capture_cursor();  // Hide cursor and enable unlimited movement
    void release_cursor();  // Show cursor and restore normal behavior
    void set_cursor_position(double x, double y);  // Set cursor position
    Point2D get_cursor_position() const;  // Get current cursor position
    
    // Theme management
    void set_theme(const CanvasTheme& theme);
    const CanvasTheme& get_theme() const { return theme_; }
    
    // Event callbacks (called by GLFW)
    void on_window_resize(int width, int height);
    void on_framebuffer_resize(int width, int height);
    void on_mouse_button(int button, int action, int mods);
    void on_mouse_move(double x, double y);
    void on_mouse_scroll(double x_offset, double y_offset);
    void on_key_event(int key, int scancode, int action, int mods);
    
private:
    void setup_opengl_context();
    void setup_callbacks();
    InputEvent create_input_event(EventType type) const;
    bool is_smart_mouse_or_trackpad(double x_offset, double y_offset);
    
    // GLFW window handle
    GLFWwindow* window_;
    
    // Window properties
    std::string title_;
    int width_, height_;
    int framebuffer_width_, framebuffer_height_;
    float content_scale_;
    
    // UI theme
    CanvasTheme theme_;
    
    // Core systems
    std::unique_ptr<CanvasRenderer> renderer_;
    std::unique_ptr<EventRouter> event_router_;
    std::unique_ptr<RegionManager> region_manager_;
    std::unique_ptr<WorkspaceManager> workspace_manager_;
    
    // Input state
    Point2D last_mouse_pos_;
    bool mouse_buttons_[3] = {false, false, false}; // left, right, middle
    uint32_t keyboard_modifiers_ = 0;
    bool natural_scroll_direction_ = false; // Smart mouse/trackpad scroll direction
    double scroll_ignore_until_ = 0.0; // Timestamp until which to ignore scroll events
    
    // Cursor capture state for infinite dragging
    bool cursor_captured_ = false;
    Point2D cursor_capture_pos_{0, 0};
    
    // Gesture tracking to prevent event type changes mid-gesture
    enum class GestureType {
        None,
        Rotate,     // Trackpad rotate gesture
        Pan,        // Shift + trackpad pan
        Zoom        // CMD/CTRL + trackpad scroll
    };
    GestureType current_gesture_ = GestureType::None;
    double last_scroll_time_ = 0.0;  // Track when last scroll event occurred
    static constexpr double GESTURE_TIMEOUT = 0.15; // 150ms timeout for gesture continuity
    
    // Initialization state
    bool initialized_ = false;
    bool gl_context_initialized_ = false;
};

/**
 * Manages the hierarchical region layout system.
 * Handles dynamic splitting, merging, and resizing of UI regions.
 */
class RegionManager {
public:
    explicit RegionManager(CanvasWindow* window);
    ~RegionManager();
    
    // Region management
    RegionID create_region(const Rect2D& bounds);
    void destroy_region(RegionID region_id);
    CanvasRegion* get_region(RegionID region_id) const;
    
    // Region splitting
    std::pair<RegionID, RegionID> split_region_horizontal(RegionID region_id, float split_ratio = 0.5f);
    std::pair<RegionID, RegionID> split_region_vertical(RegionID region_id, float split_ratio = 0.5f);
    bool merge_regions(RegionID region1, RegionID region2);
    
    // Layout updates
    void update_layout(const Rect2D& window_bounds);
    void render_regions();
    
    // Event handling
    bool handle_event(const InputEvent& event);
    RegionID find_region_at_point(const Point2D& point) const;
    bool handle_region_resize(const InputEvent& event);
    
    // Access methods
    CanvasRegion* get_root_region() const;
    
    // Workspace integration
    void save_layout_to_workspace(const std::string& workspace_name);
    bool load_layout_from_workspace(const std::string& workspace_name);
    
private:
    void create_default_layout();
    void update_region_bounds();
    RegionID allocate_region_id();
    
    CanvasWindow* window_;
    std::unordered_map<RegionID, std::unique_ptr<CanvasRegion>> regions_;
    RegionID root_region_id_;
    RegionID next_region_id_;
};

/**
 * Manages workspace configurations and layouts.
 * Provides preset layouts for different voxel editing workflows.
 */
class WorkspaceManager {
public:
    explicit WorkspaceManager(CanvasWindow* window);
    ~WorkspaceManager();
    
    // Workspace management
    void create_workspace(const std::string& name, const std::string& description = "");
    void delete_workspace(const std::string& name);
    bool set_active_workspace(const std::string& name);
    std::string get_active_workspace() const { return active_workspace_; }
    
    // Workspace operations
    void save_current_layout();
    bool restore_workspace_layout(const std::string& name);
    std::vector<std::string> get_workspace_names() const;
    
    // Preset workspaces
    void create_default_workspaces();
    void create_3d_editing_workspace();
    void create_sculpting_workspace();
    void create_animation_workspace();
    
private:
    struct WorkspaceData {
        std::string name;
        std::string description;
        std::string layout_data; // Serialized region layout
    };
    
    CanvasWindow* window_;
    std::unordered_map<std::string, WorkspaceData> workspaces_;
    std::string active_workspace_;
};

} // namespace voxel_canvas