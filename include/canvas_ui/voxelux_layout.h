/*
 * Copyright (C) 2024 Voxelux
 * 
 * Voxelux Main Application Layout
 * Integrates all UI components into the complete application interface
 * MIGRATED to new styled widget system
 */

#pragma once

#include "canvas_core.h"
#include "styled_widgets.h"
#include "render_block.h"
#include "dock_container.h"
#include "dock_column.h"
#include "viewport_3d_editor.h"
#include <memory>
#include <vector>

namespace voxel_canvas {

// Forward declarations
class CanvasWindow;
class CanvasRenderer;
class Viewport3DEditor;

/**
 * Main application layout manager
 * Orchestrates all UI components into the complete Voxelux interface
 * Now uses the new styled widget system with automatic layout
 */
class VoxeluxLayout : public Container {
public:
    VoxeluxLayout() : Container("voxelux-layout") {
        // Configure main layout as vertical column
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.width = SizeValue::percent(100);
        base_style_.height = SizeValue::percent(100);
        base_style_.background = ColorValue("gray_3");  // Match viewport background
        base_style_.padding = BoxSpacing(0);  // No padding - menu bar should be flush with window edge
        base_style_.margin = BoxSpacing(0);   // No margin
    }
    
    ~VoxeluxLayout() = default;
    
    // Initialize the layout with window dimensions
    bool initialize(float window_width, float window_height, float content_scale = 1.0f) {
        window_width_ = window_width;
        window_height_ = window_height;
        content_scale_ = content_scale;
        
        // Set bounds for the entire layout
        set_bounds(Rect2D(0, 0, window_width, window_height));
        
        // Build the UI structure
        build_layout();
        
        // Trigger initial style computation and layout
        invalidate_style();
        invalidate_layout();
        
        return true;
    }
    
    void shutdown() {
        // Clean up
        remove_all_children();
        menu_bar_.reset();
        main_content_.reset();
        left_dock_.reset();
        right_dock_.reset();
        bottom_dock_.reset();
        viewport_.reset();
    }
    
    // Update layout when window resizes
    void on_window_resize(float width, float height) {
        window_width_ = width;
        window_height_ = height;
        set_bounds(Rect2D(0, 0, width, height));
        invalidate_layout();
    }
    
    // Set content scale for high-DPI displays
    void set_content_scale(float scale) {
        content_scale_ = scale;
        // TODO: Update theme scale
        invalidate_layout();
    }
    
    // Render all UI components  
    void render(CanvasRenderer* renderer);  // Move to .cpp for debug output
    
    // Handle input events
    bool handle_event(const InputEvent& event) override {
        return Container::handle_event(event);
    }
    
    // Access components
    std::shared_ptr<MenuBar> get_menu_bar() { return menu_bar_; }
    std::shared_ptr<DockContainer> get_left_dock() { return left_dock_; }
    std::shared_ptr<DockContainer> get_right_dock() { return right_dock_; }
    std::shared_ptr<BottomDockContainer> get_bottom_dock() { return bottom_dock_; }
    Viewport3DEditor* get_viewport() { return viewport_.get(); }
    
    // Show/hide control dock
    void toggle_control_dock() {
        if (control_dock_) {
            control_dock_->set_visible(!control_dock_->is_visible());
            invalidate_layout();
        }
    }
    
    // Show/hide docks
    void toggle_left_dock() {
        if (left_dock_) {
            left_dock_->set_visible(!left_dock_->is_visible());
            invalidate_layout();
        }
    }
    
    void toggle_right_dock() {
        if (right_dock_) {
            right_dock_->set_visible(!right_dock_->is_visible());
            invalidate_layout();
        }
    }
    
    void toggle_bottom_dock() {
        if (bottom_dock_) {
            bottom_dock_->set_visible(!bottom_dock_->is_visible());
            invalidate_layout();
        }
    }
    
    std::string get_widget_type() const override { return "voxelux-layout"; }
    
    // Region manager integration (TODO: implement when RegionManager is migrated)
    void set_region_manager(void* region_manager) {
        // TODO: Connect to region manager when it's migrated to new system
        region_manager_ = region_manager;
    }
    
private:
    void build_layout() {
        // Clear existing layout
        remove_all_children();
        
        // 1. MENU BAR
        // Create menu bar
        menu_bar_ = create_widget<MenuBar>();
        // Logo is now automatically included as voxelux.svg icon
        
        // Menu bar styling is handled internally
        
        // Add menus
        std::vector<MenuBar::MenuItem> file_menu = {
            {"new_project", "New Project", "Ctrl+N", "new_file", false, true, {}, 
             []() { /* TODO: New project */ }},
            {"open_project", "Open Project", "Ctrl+O", "open_file", false, true, {}, 
             []() { /* TODO: Open project */ }},
            {"", "", "", "", true, false, {}, nullptr},  // Separator
            {"save", "Save", "Ctrl+S", "save_file", false, true, {}, 
             []() { /* TODO: Save */ }},
            {"save_as", "Save As...", "Ctrl+Shift+S", "save_file", false, true, {}, 
             []() { /* TODO: Save as */ }},
            {"", "", "", "", true, false, {}, nullptr},  // Separator
            {"exit", "Exit", "Alt+F4", "", false, true, {}, 
             []() { /* TODO: Exit */ }}
        };
        menu_bar_->add_menu("File", file_menu);
        
        std::vector<MenuBar::MenuItem> edit_menu = {
            {"undo", "Undo", "Ctrl+Z", "undo", false, true, {}, 
             []() { /* TODO: Undo */ }},
            {"redo", "Redo", "Ctrl+Y", "redo", false, true, {}, 
             []() { /* TODO: Redo */ }},
            {"", "", "", "", true, false, {}, nullptr},  // Separator
            {"cut", "Cut", "Ctrl+X", "cut", false, true, {}, 
             []() { /* TODO: Cut */ }},
            {"copy", "Copy", "Ctrl+C", "copy", false, true, {}, 
             []() { /* TODO: Copy */ }},
            {"paste", "Paste", "Ctrl+V", "paste", false, true, {}, 
             []() { /* TODO: Paste */ }}
        };
        menu_bar_->add_menu("Edit", edit_menu);
        
        menu_bar_->add_menu("View", {});
        
        // Window menu with control dock toggle
        std::vector<MenuBar::MenuItem> window_menu = {
            {"toggle_controls", "Controls", "Ctrl+Shift+C", "controls", false, true, {},
             [this]() { toggle_control_dock(); }},
            {"", "", "", "", true, false, {}, nullptr},  // Separator
            {"toggle_left_dock", "Left Dock", "", "dock_left", false, true, {},
             [this]() { toggle_left_dock(); }},
            {"toggle_right_dock", "Right Dock", "", "dock_right", false, true, {},
             [this]() { toggle_right_dock(); }},
            {"toggle_bottom_dock", "Bottom Dock", "", "dock_bottom", false, true, {},
             [this]() { toggle_bottom_dock(); }}
        };
        menu_bar_->add_menu("Window", window_menu);
        menu_bar_->add_menu("Help", {});
        
        // Build all menu items now that we've added them all
        menu_bar_->build_menus();
        
        add_child(menu_bar_);
        
        // 2. CONTROL DOCK (Empty for now, will contain playback controls, tools, etc.)
        control_dock_ = LayoutBuilder::row();
        control_dock_->set_id("control-dock");
        control_dock_->get_style()
            .set_height(SizeValue(40))
            .set_background(ColorValue("gray_4"))
            .set_padding(BoxSpacing(4, SpacingValue::theme_md()))
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_gap(SpacingValue::theme_md());
        
        // Add bottom border to control dock
        control_dock_->get_style().border.width = SpacingValue(1);
        control_dock_->get_style().border.color = ColorValue("gray_5");
        
        // Control dock is empty for now - will add controls later
        // For now, just add a placeholder text
        auto placeholder = create_widget<Text>("Control Bar (Toggle with Window > Controls)");
        placeholder->get_style()
            .set_text_color(ColorValue("gray_2"))
            .set_font_size(SpacingValue(11));
        control_dock_->add_child(placeholder);
        
        // Initially hidden - user must toggle it on
        control_dock_->set_visible(false);
        add_child(control_dock_);
        
        // 3. MAIN CONTENT AREA (horizontal split of left dock, center, right dock)
        main_content_ = LayoutBuilder::row();
        main_content_->set_id("main-content");
        main_content_->get_style()
            .set_flex_grow(1);  // Take remaining vertical space
        
        // Create left dock
        left_dock_ = std::make_shared<DockContainer>(DockContainer::DockSide::LEFT);
        
        // Add tool palette to left dock
        auto tool_column = left_dock_->add_column(DockColumn::ColumnType::TOOL);
        auto tool_palette = create_widget<ToolPalette>();
        tool_palette->add_tool({"select", "Select", "select", "Select objects"});
        tool_palette->add_tool({"draw", "Draw", "pencil", "Draw voxels"});
        tool_palette->add_tool({"erase", "Erase", "eraser", "Erase voxels"});
        tool_palette->add_tool({"fill", "Fill", "paint_bucket", "Fill area"});
        tool_palette->add_tool({"pick", "Pick Color", "eyedropper", "Pick color from voxel"});
        // Add tool palette as a panel in a group
        auto tool_group = std::make_shared<PanelGroup>();
        tool_group->add_panel(tool_palette);
        tool_column->add_panel_group(tool_group);
        
        // Set tool column to collapsed state to show icon bar
        tool_column->set_state(DockColumn::ColumnState::COLLAPSED);
        
        // Add layers panel to left dock in its own column
        auto layers_column = left_dock_->add_column(DockColumn::ColumnType::REGULAR);
        auto layers_panel = create_widget<Panel>("Layers");
        layers_panel->set_icon("layers");
        auto layers_group = std::make_shared<PanelGroup>();
        layers_group->add_panel(layers_panel);
        layers_column->add_panel_group(layers_group);
        
        // Set layers column to collapsed state to show icon bar
        layers_column->set_state(DockColumn::ColumnState::COLLAPSED);
        
        main_content_->add_child(left_dock_);
        
        // Create center area with file tabs and viewport
        auto center_area = LayoutBuilder::column();
        center_area->set_id("center-area");
        center_area->get_style()
            .set_flex_grow(1);  // Take remaining horizontal space
        
        // FILE TABS - Above the viewport in center area
        file_tabs_ = create_widget<TabContainer>(true);  // true = file tabs style
        file_tabs_->get_style().set_height(SizeValue(35));  // Ensure visible height
        file_tabs_->add_tab("untitled", "Untitled.vox");
        file_tabs_->add_tab("scene1", "Scene1.vox");
        file_tabs_->set_active_tab("untitled");
        center_area->add_child(file_tabs_);
        
        // VIEWPORT CONTAINER - Below file tabs
        viewport_container_ = create_widget<Container>("viewport-container");
        viewport_container_->get_style()
            .set_flex_grow(1)  // Take remaining vertical space
            .set_background(ColorValue("gray_3"));
        
        // TODO: Add actual Viewport3DEditor here
        // viewport_ = std::make_unique<Viewport3DEditor>();
        // viewport_container_->add_child(viewport_);
        
        center_area->add_child(viewport_container_);
        main_content_->add_child(center_area);
        
        // Create right dock
        right_dock_ = std::make_shared<DockContainer>(DockContainer::DockSide::RIGHT);
        
        // Add a column with properties and materials panels
        auto props_column = right_dock_->add_column(DockColumn::ColumnType::REGULAR);
        
        // Create a panel group with properties panel
        auto props_group = std::make_shared<PanelGroup>();
        auto properties_panel = create_widget<Panel>("Properties");
        properties_panel->set_icon("settings");
        properties_panel->set_collapsible(true);
        props_group->add_panel(properties_panel);
        props_column->add_panel_group(props_group);
        
        // Create another panel group with materials panel
        auto materials_group = std::make_shared<PanelGroup>();
        auto materials_panel = create_widget<Panel>("Materials");
        materials_panel->set_icon("palette");
        materials_group->add_panel(materials_panel);
        props_column->add_panel_group(materials_group);
        
        // Set properties column to collapsed state to show icon bar
        props_column->set_state(DockColumn::ColumnState::COLLAPSED);
        
        main_content_->add_child(right_dock_);
        
        add_child(main_content_);
        
        // 4. BOTTOM DOCK (timeline, console, etc.) - spans full width
        bottom_dock_ = std::make_shared<BottomDockContainer>();
        
        // Add timeline panel
        auto timeline_panel = create_widget<Panel>("Timeline");
        timeline_panel->set_icon("timeline");
        bottom_dock_->add_row(timeline_panel);
        
        // Add console panel
        auto console_panel = create_widget<Panel>("Console");
        console_panel->set_icon("terminal");
        bottom_dock_->add_row(console_panel);
        
        add_child(bottom_dock_);
        
        // 5. STATUS BAR (footer)
        create_status_bar();
        
        // Force initial layout calculation
        // Note: We're not using RenderBlock here since we need the renderer's theme
        // The batching will happen during the first render when we have access to the theme
        invalidate_layout();
    }
    
    void create_status_bar() {
        auto status_bar = LayoutBuilder::row();
        status_bar->set_id("status-bar");
        status_bar->get_style()
            .set_height(SizeValue(24))
            .set_padding(BoxSpacing(2, 8))
            .set_background(ColorValue("gray_5"))
            .set_gap(SpacingValue(16))
            .set_align_items(WidgetStyle::AlignItems::Center);
        
        // Status text
        auto status_text = create_widget<Text>("Ready");
        status_text->get_style()
            .set_font_size(SpacingValue(11))
            .set_text_color(ColorValue("gray_2"));
        status_bar->add_child(status_text);
        
        // Spacer
        status_bar->add_child(LayoutBuilder::spacer());
        
        // Voxel count
        auto voxel_count = create_widget<Label>("0 voxels", "cube");
        voxel_count->get_style()
            .set_font_size(SpacingValue(11))
            .set_text_color(ColorValue("gray_2"));
        status_bar->add_child(voxel_count);
        
        // FPS counter
        auto fps_counter = create_widget<Text>("60 FPS");
        fps_counter->get_style()
            .set_font_size(SpacingValue(11))
            .set_text_color(ColorValue("gray_2"));
        status_bar->add_child(fps_counter);
        
        add_child(status_bar);
    }
    
private:
    // Window dimensions
    float window_width_ = 0;
    float window_height_ = 0;
    float content_scale_ = 1.0f;
    
    // Main UI components (in order from top to bottom)
    std::shared_ptr<MenuBar> menu_bar_;                    // 1. Menu bar
    std::shared_ptr<Container> control_dock_;              // 2. Control dock (toggleable toolbar)
    std::shared_ptr<Container> main_content_;              // 3. Main content area
    std::shared_ptr<DockContainer> left_dock_;             //    - Left dock
    std::shared_ptr<TabContainer> file_tabs_;              //    - File tabs (in center area)
    std::shared_ptr<Container> viewport_container_;        //    - Viewport (below file tabs)
    std::shared_ptr<DockContainer> right_dock_;            //    - Right dock
    std::shared_ptr<BottomDockContainer> bottom_dock_;     // 4. Bottom dock (full width)
    // Status bar is created inline                        // 5. Status bar (footer)
    
    void* region_manager_ = nullptr;  // TODO: Use proper type when RegionManager is migrated
    
    // Viewport (special handling as it's not a styled widget yet)
    std::unique_ptr<Viewport3DEditor> viewport_;
};

} // namespace voxel_canvas