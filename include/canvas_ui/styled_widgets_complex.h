/*
 * Copyright (C) 2024 Voxelux
 * 
 * Complex Styled Widgets - Advanced UI components
 * Replaces Panel, TabContainer, MenuBar, etc. from ui_components.h
 */

#pragma once

#include "styled_widget.h"
#include "styled_widgets_core.h"
#include "layout_builder.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>

namespace voxel_canvas {

// Bring create_widget into scope for this file
using voxel_canvas::create_widget;

/**
 * Panel widget - Container with optional header
 * Replaces Panel from ui_components.h
 */
class Panel : public Container {
public:
    Panel(const std::string& title = "") 
        : Container("panel"), title_(title) {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.background = ColorValue("gray_4");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_5");
        base_style_.border.radius = SpacingValue(4);
        
        rebuild_layout();
    }
    
    void set_title(const std::string& title) {
        title_ = title;
        rebuild_layout();
    }
    
    void set_icon(const std::string& icon) {
        icon_name_ = icon;
        rebuild_layout();
    }
    
    void set_collapsible(bool collapsible) {
        is_collapsible_ = collapsible;
        rebuild_layout();
    }
    
    void set_collapsed(bool collapsed) {
        is_collapsed_ = collapsed;
        if (content_container_) {
            content_container_->set_visible(!collapsed);
        }
    }
    
    bool is_collapsed() const { return is_collapsed_; }
    
    // Panel identification
    const std::string& get_title() const { return title_; }
    const std::string& get_icon() const { return icon_name_; }
    
    void add_content(std::shared_ptr<StyledWidget> widget) {
        if (content_container_) {
            content_container_->add_child(widget);
        }
    }
    
    std::string get_widget_type() const override { return "panel"; }
    
private:
    void rebuild_layout() {
        remove_all_children();
        
        // Create header if title exists
        if (!title_.empty()) {
            auto header = LayoutBuilder::row();
            header->set_id("panel-header");
            header->get_style()
                .set_padding(BoxSpacing::theme())
                .set_background(ColorValue("gray_5"))
                .set_gap(SpacingValue::theme_sm())
                .set_align_items(WidgetStyle::AlignItems::Center);
            
            // Collapse/expand button
            if (is_collapsible_) {
                auto collapse_btn = voxel_canvas::create_widget<Button>();
                collapse_btn->set_icon(is_collapsed_ ? "chevron_right" : "chevron_down");
                collapse_btn->get_style()
                    .set_width(SizeValue(20))
                    .set_height(SizeValue(20))
                    .set_padding(BoxSpacing::all(2));
                collapse_btn->set_on_click([this](StyledWidget*) {
                    set_collapsed(!is_collapsed_);
                    rebuild_layout();
                });
                header->add_child(collapse_btn);
            }
            
            // Icon
            if (!icon_name_.empty()) {
                header->add_child(voxel_canvas::create_widget<Icon>(icon_name_, 16));
            }
            
            // Title
            auto title_text = voxel_canvas::create_widget<Text>(title_);
            title_text->get_style()
                .set_font_size(SpacingValue(14))
                .set_font_weight(700);  // Bold
            header->add_child(title_text);
            
            // Spacer
            header->add_child(LayoutBuilder::spacer());
            
            add_child(header);
        }
        
        // Create content container
        content_container_ = LayoutBuilder::column();
        content_container_->set_id("panel-content");
        content_container_->get_style()
            .set_padding(BoxSpacing::theme())
            .set_flex_grow(1);
        
        if (is_collapsed_) {
            content_container_->set_visible(false);
        }
        
        add_child(content_container_);
    }
    
private:
    std::string title_;
    std::string icon_name_;
    bool is_collapsible_ = false;
    bool is_collapsed_ = false;
    std::shared_ptr<Container> content_container_;
};

/**
 * Tab Container - Implements tabbed interface
 * Replaces TabContainer from ui_components.h
 */
class TabContainer : public Container {
public:
    struct Tab {
        std::string id;
        std::string label;
        std::string icon;
        bool is_closable = true;
        bool is_modified = false;
        std::shared_ptr<StyledWidget> content;
    };
    
    TabContainer(bool is_file_tabs = false) 
        : Container("tab-container"), is_file_tabs_(is_file_tabs) {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        
        rebuild_layout();
    }
    
    void add_tab(const std::string& id, const std::string& label, 
                 std::shared_ptr<StyledWidget> content = nullptr) {
        tabs_.push_back({id, label, "", true, false, content});
        if (active_tab_id_.empty()) {
            active_tab_id_ = id;
        }
        rebuild_layout();
    }
    
    void remove_tab(const std::string& id) {
        auto it = std::remove_if(tabs_.begin(), tabs_.end(),
            [&id](const Tab& tab) { return tab.id == id; });
        tabs_.erase(it, tabs_.end());
        
        if (active_tab_id_ == id && !tabs_.empty()) {
            active_tab_id_ = tabs_[0].id;
        }
        rebuild_layout();
    }
    
    void set_active_tab(const std::string& id) {
        active_tab_id_ = id;
        rebuild_layout();
    }
    
    void set_tab_modified(const std::string& id, bool modified) {
        for (auto& tab : tabs_) {
            if (tab.id == id) {
                tab.is_modified = modified;
                rebuild_layout();
                break;
            }
        }
    }
    
    void set_on_tab_change(std::function<void(const std::string&)> callback) {
        on_tab_change_ = callback;
    }
    
    void set_on_tab_close(std::function<bool(const std::string&)> callback) {
        on_tab_close_ = callback;
    }
    
    const std::string& get_active_tab() const { return active_tab_id_; }
    
    std::string get_widget_type() const override { return "tab-container"; }
    
private:
    void rebuild_layout() {
        remove_all_children();
        
        // Create tab bar
        auto tab_bar = LayoutBuilder::row();
        tab_bar->set_id("tab-bar");
        tab_bar->get_style()
            .set_height(SizeValue(is_file_tabs_ ? 35 : 28))
            .set_background(ColorValue("gray_5"))
            .set_gap(SpacingValue(1));
        
        // Add tabs
        for (const auto& tab : tabs_) {
            auto tab_button = create_tab_button(tab);
            tab_bar->add_child(tab_button);
        }
        
        add_child(tab_bar);
        
        // Create content area
        auto content_area = create_widget<Container>("tab-content");
        content_area->get_style()
            .set_flex_grow(1)
            .set_background(ColorValue("gray_3"))
            .set_padding(BoxSpacing::theme());
        
        // Show active tab content
        for (const auto& tab : tabs_) {
            if (tab.id == active_tab_id_ && tab.content) {
                content_area->add_child(tab.content);
            }
        }
        
        add_child(content_area);
    }
    
    std::shared_ptr<Container> create_tab_button(const Tab& tab) {
        auto button = LayoutBuilder::row();
        button->set_id("tab-" + tab.id);
        button->get_style()
            .set_padding(BoxSpacing(6, 12))
            .set_gap(SpacingValue(6))
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_cursor("pointer");
        
        // Active tab styling
        if (tab.id == active_tab_id_) {
            button->get_style()
                .set_background(ColorValue("gray_3"))
                .set_border(BorderStyle{
                    .width = SpacingValue(0),
                    .color = ColorValue("transparent")
                });
            // Add bottom border for active tab
            button->get_style().border.width = SpacingValue(0);
            button->get_style().border.color = ColorValue("accent_primary");
            // TODO: Add proper bottom border support
        } else {
            button->get_style().hover_style = std::make_unique<WidgetStyle>();
            button->get_style().hover_style->background = ColorValue("gray_4");
        }
        
        // Icon
        if (!tab.icon.empty()) {
            button->add_child(create_widget<Icon>(tab.icon, 14));
        }
        
        // Label
        auto label = create_widget<Text>(tab.label);
        button->add_child(label);
        
        // Modified indicator
        if (tab.is_modified) {
            auto dot = create_widget<Container>("modified-dot");
            dot->get_style()
                .set_width(SizeValue(6))
                .set_height(SizeValue(6))
                .set_background(ColorValue("accent_warning"))
                .set_border_radius(SpacingValue(3));
            button->add_child(dot);
        }
        
        // Close button
        if (tab.is_closable && is_file_tabs_) {
            auto close_btn = create_widget<Button>();
            close_btn->set_icon("close");
            close_btn->get_style()
                .set_width(SizeValue(16))
                .set_height(SizeValue(16))
                .set_padding(BoxSpacing::all(2))
                .set_background(ColorValue("transparent"))
                .set_border(BorderStyle{.width = SpacingValue(0)});
            
            close_btn->set_on_click([this, id = tab.id](StyledWidget*) {
                if (!on_tab_close_ || on_tab_close_(id)) {
                    remove_tab(id);
                }
            });
            
            button->add_child(close_btn);
        }
        
        // Tab click handler
        button->set_on_click([this, id = tab.id](StyledWidget*) {
            set_active_tab(id);
            if (on_tab_change_) {
                on_tab_change_(id);
            }
        });
        
        return button;
    }
    
private:
    std::vector<Tab> tabs_;
    std::string active_tab_id_;
    bool is_file_tabs_;
    std::function<void(const std::string&)> on_tab_change_;
    std::function<bool(const std::string&)> on_tab_close_;
};

/**
 * Menu Bar - Application menu bar
 * Replaces MenuBar from ui_components.h
 */
class MenuBar : public Container {
public:
    struct MenuItem {
        std::string id;
        std::string label;
        std::string shortcut;
        std::string icon;
        bool is_separator = false;
        bool is_enabled = true;
        std::vector<MenuItem> submenu;
        std::function<void()> callback;
    };
    
    MenuBar() : Container("menubar") {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Row;
        base_style_.align_items = WidgetStyle::AlignItems::Center;
        base_style_.width = SizeValue::percent(100);  // 100vw (100% width)
        base_style_.height = SizeValue(25);           // 25px tall
        base_style_.background = ColorValue("gray_5");
        base_style_.padding = BoxSpacing(0, 12);      // No vertical padding, horizontal padding for logo
        base_style_.margin = BoxSpacing(0);           // No margin
        base_style_.gap = SpacingValue(0);            // NO gap between menu items
        base_style_.border = BorderStyle{             // No borders
            .width = SpacingValue(0),
            .color = ColorValue("transparent"),
            .radius = SpacingValue(0)
        };
    }
    
    void set_logo(const std::string& logo_text) {
        logo_text_ = logo_text;  // Keep for compatibility but ignore
        invalidate_layout();
    }
    
    void add_menu(const std::string& label, const std::vector<MenuItem>& items) {
        menus_.push_back({label, items});
        invalidate_layout();
    }
    
    void build_menus() {
        // Build all menu items in one batch
        rebuild_menu_items();
    }
    
    std::string get_widget_type() const override { return "menubar"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        // Normal rendering
        Container::render_content(renderer);
    }
    
private:
    void rebuild_menu_items() {
        // Clear existing children
        remove_all_children();
        
        // Voxelux logo icon - always show  
        // Icon is loaded from assets/icons/ui/voxelux.svg as "ui_voxelux"
        auto logo_icon = create_widget<Icon>("ui_voxelux", 16);  // 16px icon
        logo_icon->get_style()
            .set_text_color(ColorValue("white"))    // Make it more visible
            .set_width(SizeValue(16))     // Fixed width - 16px
            .set_height(SizeValue(16))    // Fixed height - 16px
            .set_margin(BoxSpacing(0, 8, 0, 0))  // Add 8px right margin for spacing
            .set_flex_shrink(0);          // Don't shrink the logo
        
        // Ensure no borders on icon
        logo_icon->get_style().border.width = SpacingValue(0);
        logo_icon->get_style().border.color = ColorValue("transparent");
        
        add_child(logo_icon);
        
        // Create menu buttons with proper spacing
        for (const auto& [label, items] : menus_) {
            auto menu_button = create_widget<Button>(label);
            
            // Apply menu-specific styling - clear defaults first
            auto& style = menu_button->get_style();
            style.set_background(ColorValue("transparent"));  // Transparent background
            style.set_padding(BoxSpacing(0, 10));             // No vertical padding, 10px horizontal
            style.set_margin(BoxSpacing(0));                  // No margin
            style.set_width(SizeValue::auto_size());          // Auto width
            style.set_font_size(SpacingValue(13));            // Fixed font size
            style.set_font_weight(400);                       // Normal weight
            style.set_height(SizeValue(25));                  // 25px height
            style.set_text_color(ColorValue("gray_1"));
            style.set_flex_shrink(0);                         // Don't shrink
            style.set_cursor("pointer");
            
            // Explicitly set border to none
            BorderStyle no_border;
            no_border.width = SpacingValue(0);
            no_border.color = ColorValue("transparent");
            no_border.radius = SpacingValue(0);
            style.set_border(no_border);
            
            // Hover style - only change background color
            menu_button->get_style().hover_style = std::make_unique<WidgetStyle>();
            menu_button->get_style().hover_style->background = ColorValue("gray_4");  // Gray hover background
            
            menu_button->set_on_click([this, items](StyledWidget*) {
                // TODO: Show dropdown menu
                show_dropdown_menu(items);
            });
            
            add_child(menu_button);
        }
        
        // Spacer to push right-aligned items
        add_child(LayoutBuilder::spacer());
    }
    
    void show_dropdown_menu(const std::vector<MenuItem>& items) {
        // TODO: Implement dropdown menu display
    }
    
private:
    std::string logo_text_;
    std::vector<std::pair<std::string, std::vector<MenuItem>>> menus_;
};

/**
 * Context Menu - Right-click context menu
 * Replaces RegionContextMenu from ui_widgets.h
 */
class ContextMenu : public Container {
public:
    using MenuItem = MenuBar::MenuItem;
    
    ContextMenu() : Container("context-menu") {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.position = WidgetStyle::Position::Absolute;
        base_style_.background = ColorValue("gray_5");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_6");
        base_style_.border.radius = SpacingValue(4);
        base_style_.padding = BoxSpacing(4, 0);
        base_style_.box_shadow = ShadowStyle{
            .offset_x = 2,
            .offset_y = 2,
            .blur_radius = 8,
            .spread = 0,
            .color = ColorValue(ColorRGBA(0, 0, 0, 0.3f))
        };
        base_style_.z_index = 1000;
    }
    
    void add_item(const MenuItem& item) {
        items_.push_back(item);
        rebuild_layout();
    }
    
    void add_separator() {
        items_.push_back({.is_separator = true});
        rebuild_layout();
    }
    
    void clear_items() {
        items_.clear();
        rebuild_layout();
    }
    
    void show_at(const Point2D& position) {
        base_style_.left = SpacingValue(position.x);
        base_style_.top = SpacingValue(position.y);
        set_visible(true);
    }
    
    void hide() {
        set_visible(false);
    }
    
    std::string get_widget_type() const override { return "context-menu"; }
    
private:
    void rebuild_layout() {
        remove_all_children();
        
        for (const auto& item : items_) {
            if (item.is_separator) {
                // Add a horizontal divider
                auto divider = create_widget<Separator>(Separator::Orientation::HORIZONTAL);
                add_child(divider);
            } else {
                auto menu_item = create_menu_item(item);
                add_child(menu_item);
            }
        }
    }
    
    std::shared_ptr<Container> create_menu_item(const MenuItem& item) {
        auto container = LayoutBuilder::row();
        container->get_style()
            .set_padding(BoxSpacing(6, 12))
            .set_gap(SpacingValue::theme_md())
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_cursor(item.is_enabled ? "pointer" : "default");
        
        if (item.is_enabled) {
            container->get_style().hover_style = std::make_unique<WidgetStyle>();
            container->get_style().hover_style->background = ColorValue("accent_primary");
        }
        
        // Icon
        if (!item.icon.empty()) {
            auto icon = create_widget<Icon>(item.icon, 16);
            if (!item.is_enabled) {
                icon->get_style().set_opacity(0.5f);
            }
            container->add_child(icon);
        }
        
        // Label
        auto label = create_widget<Text>(item.label);
        label->get_style().set_flex_grow(1);
        if (!item.is_enabled) {
            label->get_style().set_text_color(ColorValue("gray_2"));
        }
        container->add_child(label);
        
        // Shortcut
        if (!item.shortcut.empty()) {
            auto shortcut = create_widget<Text>(item.shortcut);
            shortcut->get_style()
                .set_text_color(ColorValue("gray_2"))
                .set_font_size(SpacingValue(11));
            container->add_child(shortcut);
        }
        
        // Click handler
        if (item.is_enabled && item.callback) {
            container->set_on_click([callback = item.callback](StyledWidget*) {
                callback();
            });
        }
        
        return container;
    }
    
private:
    std::vector<MenuItem> items_;
};

/**
 * Splitter - Resizable splitter between panels
 * Replaces RegionSplitter from ui_widgets.h
 */
class Splitter : public StyledWidget {
public:
    enum class Orientation {
        HORIZONTAL,  // Horizontal line, resizes vertically
        VERTICAL     // Vertical line, resizes horizontally
    };
    
    Splitter(Orientation orientation = Orientation::VERTICAL)
        : StyledWidget("splitter"), orientation_(orientation) {
        if (orientation == Orientation::HORIZONTAL) {
            base_style_.width = SizeValue::percent(100);
            base_style_.height = SizeValue(4);
            base_style_.cursor = "ns-resize";
        } else {
            base_style_.width = SizeValue(4);
            base_style_.height = SizeValue::percent(100);
            base_style_.cursor = "ew-resize";
        }
        base_style_.background = ColorValue("gray_5");
        
        base_style_.hover_style = std::make_unique<WidgetStyle>();
        base_style_.hover_style->background = ColorValue("accent_primary");
    }
    
    void set_on_resize(std::function<void(float)> callback) {
        on_resize_ = callback;
    }
    
    std::string get_widget_type() const override { return "splitter"; }
    
protected:
    bool handle_event(const InputEvent& event) override {
        if (event.type == EventType::MOUSE_PRESS) {
            is_dragging_ = true;
            drag_start_ = event.mouse_pos;
            return true;
        } else if (event.type == EventType::MOUSE_RELEASE) {
            is_dragging_ = false;
            return true;
        } else if (event.type == EventType::MOUSE_MOVE && is_dragging_) {
            float delta = (orientation_ == Orientation::VERTICAL) ?
                event.mouse_pos.x - drag_start_.x :
                event.mouse_pos.y - drag_start_.y;
            
            if (on_resize_) {
                on_resize_(delta);
            }
            drag_start_ = event.mouse_pos;
            return true;
        }
        
        return StyledWidget::handle_event(event);
    }
    
private:
    Orientation orientation_;
    bool is_dragging_ = false;
    Point2D drag_start_;
    std::function<void(float)> on_resize_;
};

/**
 * Tool Palette - Tool selection palette
 * Replaces ToolPalette from ui_components.h
 */
class ToolPalette : public Panel {
public:
    struct Tool {
        std::string id;
        std::string name;
        std::string icon;
        std::string tooltip;
    };
    
    ToolPalette() : Panel("Tools") {
        set_icon("tools");
        // Create a container with grid-like layout
        tools_container_ = create_widget<Container>("tools-grid");
        tools_container_->get_style()
            .set_gap(SpacingValue(4));
        add_content(tools_container_);
    }
    
    void add_tool(const Tool& tool) {
        tools_.push_back(tool);
        
        auto tool_button = create_widget<Button>();
        tool_button->set_icon(tool.icon);
        tool_button->get_style()
            .set_width(SizeValue(32))
            .set_height(SizeValue(32))
            .set_padding(BoxSpacing::all(4))
            .set_background(ColorValue("gray_4"))
            .set_border(BorderStyle{
                .width = SpacingValue(1.0f),
                .color = ColorValue("gray_5"),
                .radius = SpacingValue(4.0f)
            });
        
        // Add hover state
        tool_button->get_style().hover_style = std::make_unique<WidgetStyle>();
        tool_button->get_style().hover_style->background = ColorValue("gray_3");
        
        // Active tool styling
        if (tool.id == active_tool_id_) {
            tool_button->get_style()
                .set_background(ColorValue("accent_primary"));
        }
        
        tool_button->set_on_click([this, id = tool.id](StyledWidget*) {
            set_active_tool(id);
            if (on_tool_select_) {
                on_tool_select_(id);
            }
        });
        
        // TODO: Add tooltip on hover
        
        tools_container_->add_child(tool_button);
    }
    
    void set_active_tool(const std::string& id) {
        active_tool_id_ = id;
        // TODO: Update button styles
    }
    
    void set_on_tool_select(std::function<void(const std::string&)> callback) {
        on_tool_select_ = callback;
    }
    
    std::string get_widget_type() const override { return "tool-palette"; }
    
private:
    std::vector<Tool> tools_;
    std::string active_tool_id_;
    std::shared_ptr<Container> tools_container_;
    std::function<void(const std::string&)> on_tool_select_;
};

} // namespace voxel_canvas