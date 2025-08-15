/*
 * Copyright (C) 2024 Voxelux
 * 
 * Styled Widgets - Unified widget library using the CSS-like styling system
 * 
 * This is the main include file for all styled widgets.
 * It replaces the old ui_widgets.h and ui_components.h systems.
 * 
 * Widget Organization:
 * - Core widgets: Text, Icon, Button, Label, Separator
 * - Form widgets: Input, Checkbox, RadioButton, Dropdown, Slider
 * - Complex widgets: Panel, TabContainer, MenuBar, ContextMenu, Splitter, ToolPalette
 * - Layout helpers: Container, LayoutBuilder
 */

#pragma once

// Core widgets and layout system
#include "layout_builder.h"
#include "styled_widgets_core.h"
#include "styled_widgets_form.h"
#include "styled_widgets_complex.h"

// Legacy widget system references (for migration tracking only)
// - ui_widgets.h: Old UIWidget base class system
// - ui_components.h: Old component system without styling
// 
// Migration Guide:
// - UIWidget -> StyledWidget
// - EditorTypeDropdown -> Dropdown<EditorType>
// - RegionSplitter -> Splitter
// - RegionContextMenu -> ContextMenu
// - Button (old) -> Button (new with styling)
// - Panel (old) -> Panel (new with flexbox)
// - TabContainer (old) -> TabContainer (new)
// - MenuBar (old) -> MenuBar (new)
// - DockPanel -> Use Panel with custom layout
// - Timeline -> Create custom widget extending StyledWidget
// - ToolPalette (old) -> ToolPalette (new)
//
// Key differences:
// 1. All widgets now use CSS-like styling with box model
// 2. Layout is handled by flexbox/grid instead of manual positioning
// 3. DPI scaling is automatic through ScaledTheme integration
// 4. No manual pixel calculations needed
// 5. Consistent theming through ColorValue and SpacingValue

namespace voxel_canvas {

/**
 * Widget factory functions for convenient creation
 */
namespace Widgets {
    
    // Core widgets
    inline auto text(const std::string& content) {
        return create_widget<Text>(content);
    }
    
    inline auto icon(const std::string& name, float size = 16.0f) {
        return create_widget<Icon>(name, size);
    }
    
    inline auto button(const std::string& text = "") {
        return create_widget<Button>(text);
    }
    
    inline auto label(const std::string& text = "", const std::string& icon = "") {
        return create_widget<Label>(text, icon);
    }
    
    inline auto separator(Separator::Orientation orientation = Separator::Orientation::HORIZONTAL) {
        return create_widget<Separator>(orientation);
    }
    
    // Form widgets
    inline auto input(const std::string& placeholder = "") {
        return create_widget<Input>(placeholder);
    }
    
    inline auto checkbox(const std::string& label = "", bool checked = false) {
        return create_widget<Checkbox>(label, checked);
    }
    
    inline auto radio(const std::string& label = "", const std::string& group = "default") {
        return create_widget<RadioButton>(label, group);
    }
    
    template<typename T>
    inline auto dropdown(const std::string& placeholder = "Select...") {
        return create_widget<Dropdown<T>>(placeholder);
    }
    
    inline auto slider(float min = 0, float max = 100, float value = 50) {
        return create_widget<Slider>(min, max, value);
    }
    
    // Complex widgets
    inline auto panel(const std::string& title = "") {
        return create_widget<Panel>(title);
    }
    
    inline auto tabs(bool is_file_tabs = false) {
        return create_widget<TabContainer>(is_file_tabs);
    }
    
    inline auto menubar() {
        return create_widget<MenuBar>();
    }
    
    inline auto context_menu() {
        return create_widget<ContextMenu>();
    }
    
    inline auto splitter(Splitter::Orientation orientation = Splitter::Orientation::VERTICAL) {
        return create_widget<Splitter>(orientation);
    }
    
    inline auto tool_palette() {
        return create_widget<ToolPalette>();
    }
}

/**
 * Common widget styling presets
 */
namespace Styles {
    
    // Button styles
    inline WidgetStyle primary_button() {
        WidgetStyle style;
        style.background = ColorValue("accent_primary");
        style.text_color = ColorValue("white");
        style.padding = BoxSpacing(8, 16);
        style.border.radius = SpacingValue(4);
        style.cursor = "pointer";
        
        style.hover_style = std::make_unique<WidgetStyle>();
        style.hover_style->background = ColorValue("accent_hover");
        
        return style;
    }
    
    inline WidgetStyle secondary_button() {
        WidgetStyle style;
        style.background = ColorValue("gray_4");
        style.text_color = ColorValue("gray_1");
        style.padding = BoxSpacing(8, 16);
        style.border.width = SpacingValue(1);
        style.border.color = ColorValue("gray_5");
        style.border.radius = SpacingValue(4);
        style.cursor = "pointer";
        
        style.hover_style = std::make_unique<WidgetStyle>();
        style.hover_style->background = ColorValue("gray_3");
        
        return style;
    }
    
    inline WidgetStyle danger_button() {
        WidgetStyle style;
        style.background = ColorValue("accent_warning");
        style.text_color = ColorValue("white");
        style.padding = BoxSpacing(8, 16);
        style.border.radius = SpacingValue(4);
        style.cursor = "pointer";
        
        style.hover_style = std::make_unique<WidgetStyle>();
        style.hover_style->background = ColorValue("error_color");
        
        return style;
    }
    
    // Panel styles
    inline WidgetStyle card_panel() {
        WidgetStyle style;
        style.background = ColorValue("gray_3");
        style.padding = BoxSpacing::theme();
        style.border.width = SpacingValue(1);
        style.border.color = ColorValue("gray_5");
        style.border.radius = SpacingValue(8);
        style.box_shadow = ShadowStyle{
            .offset_x = 0,
            .offset_y = 2,
            .blur_radius = 8,
            .spread = 0,
            .color = ColorValue(ColorRGBA(0, 0, 0, 0.1f))
        };
        return style;
    }
    
    inline WidgetStyle dark_panel() {
        WidgetStyle style;
        style.background = ColorValue("gray_5");
        style.padding = BoxSpacing::theme();
        return style;
    }
}

} // namespace voxel_canvas