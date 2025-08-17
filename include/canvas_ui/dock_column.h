/*
 * Copyright (C) 2024 Voxelux
 * 
 * Dock Column - Manages a vertical column of panel groups
 * MIGRATED to new styled widget system
 */

#pragma once

#include "canvas_ui/styled_widgets.h"
#include <vector>
#include <memory>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;

/**
 * PanelGroup - A group of panels with tabs
 * Now uses the new styled widget system
 */
class PanelGroup : public Container, public std::enable_shared_from_this<PanelGroup> {
public:
    PanelGroup() : Container("panel-group") {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.background = ColorValue("gray_4");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_5");
        
        // Create tab container
        tabs_ = create_widget<TabContainer>(false);
        add_child(tabs_);
    }
    
    // Panel management
    void add_panel(std::shared_ptr<Panel> panel) {
        panels_.push_back(panel);
        tabs_->add_tab(panel->get_id(), panel->get_title(), panel);
        invalidate_layout();
    }
    
    void remove_panel(std::shared_ptr<Panel> panel) {
        auto it = std::find(panels_.begin(), panels_.end(), panel);
        if (it != panels_.end()) {
            panels_.erase(it);
            tabs_->remove_tab(panel->get_id());
            invalidate_layout();
        }
    }
    
    void set_active_panel(std::shared_ptr<Panel> panel) {
        tabs_->set_active_tab(panel->get_id());
    }
    
    std::shared_ptr<Panel> get_active_panel() const {
        const std::string& active_tab_id = tabs_->get_active_tab();
        if (!active_tab_id.empty()) {
            for (const auto& panel : panels_) {
                if (panel->get_id() == active_tab_id) {
                    return panel;
                }
            }
        }
        return nullptr;
    }
    
    const std::vector<std::shared_ptr<Panel>>& get_panels() const { return panels_; }
    bool has_panels() const { return !panels_.empty(); }
    
    std::string get_widget_type() const override { return "panel-group"; }
    
private:
    std::vector<std::shared_ptr<Panel>> panels_;
    std::shared_ptr<TabContainer> tabs_;
};

/**
 * DockColumn - A vertical container that holds panel groups
 * Can be collapsed to show icons or expanded to show full panels
 */
class DockColumn : public Container {
public:
    enum class DockSide {
        LEFT,
        RIGHT
    };
    
    enum class ColumnState {
        COLLAPSED,  // Shows icon bar
        EXPANDED,   // Shows full panels with tabs
        HIDDEN      // Not visible
    };
    
    enum class ColumnType {
        REGULAR,    // Normal dock column with panels
        TOOL        // Special tool dock with fixed widths
    };

    DockColumn(DockSide side, ColumnType type = ColumnType::REGULAR, ColumnState initial_state = ColumnState::EXPANDED) 
        : Container("dock-column"), side_(side), type_(type), state_(initial_state) {
        
        // Configure column container styling
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.background = ColorValue("transparent");  // No background on column itself
        base_style_.gap = SpacingValue(0);  // No gap between header and content
        base_style_.height = SizeValue::percent(100);  // Extend full height
        
        // Set default widths based on type
        if (type == ColumnType::TOOL) {
            collapsed_width_ = 36.0f;   // Collapsed width for all columns
            expanded_width_ = 72.0f;    // Tool panel expanded (2-column grid)
            min_width_ = 36.0f;
            max_width_ = 72.0f;          // Tool dock has fixed widths
        } else {
            collapsed_width_ = 36.0f;   // Icon bar width (standard)
            expanded_width_ = 250.0f;   // Default panel width
            min_width_ = 36.0f;         // Minimum width when collapsed
            max_width_ = 500.0f;
        }
        
        // Set initial width based on state
        current_width_ = (state_ == ColumnState::COLLAPSED) ? collapsed_width_ : expanded_width_;
        base_style_.width = SizeValue(current_width_);
        
        // Create header (14px tall with expand/collapse button)
        header_ = create_widget<Container>("dock-column-header");
        header_->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Row)
            .set_height(SizeValue(14))
            .set_background(ColorValue("gray_5"))
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_justify_content(WidgetStyle::JustifyContent::Center);
        
        // Add expand/collapse button to header
        expand_button_ = create_widget<Button>();
        expand_button_->set_icon(state_ == ColumnState::COLLAPSED ? "chevron_right" : "chevron_left");
        expand_button_->get_style()
            .set_width(SizeValue(14))
            .set_height(SizeValue(14))
            .set_padding(BoxSpacing::all(1))
            .set_background(ColorValue("transparent"))
            .set_border(BorderStyle());  // No border
        expand_button_->set_on_click([this](StyledWidget*) {
            toggle_state();
        });
        header_->add_child(expand_button_);
        add_child(header_);
        
        // Create content container with gray_3 background
        content_ = create_widget<Container>("dock-column-content");
        content_->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Column)
            .set_flex_grow(1)  // Take remaining space
            .set_background(ColorValue("gray_3"))
            .set_gap(SpacingValue(1));  // Gap between panel groups
        
        // Start with content hidden if collapsed
        if (state_ == ColumnState::COLLAPSED) {
            content_->set_visible(false);
        }
        
        add_child(content_);
        
        // Apply initial state styling
        update_style_for_state();
    }
    
    ~DockColumn() override = default;
    
    // State management
    void set_state(ColumnState state) {
        if (state_ != state) {
            state_ = state;
            update_style_for_state();
            
            // Trigger animation
            if (state == ColumnState::COLLAPSED) {
                target_width_ = collapsed_width_;
                animating_ = true;
            } else if (state == ColumnState::EXPANDED) {
                target_width_ = expanded_width_;
                animating_ = true;
            } else {
                set_visible(false);
            }
            
            if (state_change_callback_) {
                state_change_callback_();
            }
            
            invalidate_layout();
        }
    }
    
    ColumnState get_state() const { return state_; }
    
    void toggle_state() {
        if (state_ == ColumnState::COLLAPSED) {
            set_state(ColumnState::EXPANDED);
            expand_button_->set_icon("chevron_left");
        } else if (state_ == ColumnState::EXPANDED) {
            set_state(ColumnState::COLLAPSED);
            expand_button_->set_icon("chevron_right");
        }
    }
    
    // Panel group management
    void add_panel_group(std::shared_ptr<PanelGroup> group) {
        panel_groups_.push_back(group);
        
        // Add splitter between groups if not first
        if (panel_groups_.size() > 1) {
            auto splitter = create_widget<Splitter>(Splitter::Orientation::HORIZONTAL);
            splitter->set_on_resize([this, index = panel_groups_.size() - 1](float delta) {
                resize_group(index, delta);
            });
            content_->add_child(splitter);  // Add to content container
        }
        
        content_->add_child(group);  // Add to content container
        invalidate_layout();
    }
    
    void add_panel(std::shared_ptr<Panel> panel) {
        // Find or create a panel group
        std::shared_ptr<PanelGroup> target_group;
        
        if (!panel_groups_.empty()) {
            // Add to last group by default
            target_group = panel_groups_.back();
        } else {
            // Create new group
            target_group = std::make_shared<PanelGroup>();
            add_panel_group(target_group);
        }
        
        target_group->add_panel(panel);
    }
    
    void remove_panel_group(std::shared_ptr<PanelGroup> group) {
        auto it = std::find(panel_groups_.begin(), panel_groups_.end(), group);
        if (it != panel_groups_.end()) {
            panel_groups_.erase(it);
            content_->remove_child(group);  // Remove from content container
            
            // Check if column is now empty
            if (panel_groups_.empty() && empty_callback_) {
                empty_callback_();
            }
            
            invalidate_layout();
        }
    }
    
    const std::vector<std::shared_ptr<PanelGroup>>& get_panel_groups() const { return panel_groups_; }
    bool has_panel_groups() const { return !panel_groups_.empty(); }
    
    // Type and side
    ColumnType get_type() const { return type_; }
    DockSide get_side() const { return side_; }
    
    // Sizing
    float get_current_width() const { return current_width_; }
    float get_collapsed_width() const { return collapsed_width_; }
    float get_expanded_width() const { return expanded_width_; }
    
    void set_width(float width) {
        expanded_width_ = std::max(min_width_, std::min(max_width_, width));
        if (state_ == ColumnState::EXPANDED) {
            current_width_ = expanded_width_;
            base_style_.width = SizeValue(current_width_);
            invalidate_layout();
        }
    }
    
    // Animation
    void update(float delta_time) {
        if (animating_) {
            float speed = 500.0f; // pixels per second
            float max_change = speed * delta_time;
            
            if (std::abs(current_width_ - target_width_) < max_change) {
                current_width_ = target_width_;
                animating_ = false;
            } else {
                current_width_ += (target_width_ > current_width_ ? max_change : -max_change);
            }
            
            base_style_.width = SizeValue(current_width_);
            invalidate_layout();
        }
    }
    
    // Callbacks
    using StateChangeCallback = std::function<void()>;
    using EmptyCallback = std::function<void()>;
    
    void set_state_change_callback(StateChangeCallback callback) {
        state_change_callback_ = callback;
    }
    
    void set_empty_callback(EmptyCallback callback) {
        empty_callback_ = callback;
    }
    
    std::string get_widget_type() const override { return "dock-column"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        // Render collapsed state icons if needed
        if (state_ == ColumnState::COLLAPSED) {
            render_collapsed_icons(renderer);
        }
        
        // Base class handles rendering children (panel groups)
    }
    
private:
    DockSide side_;
    ColumnType type_;
    ColumnState state_;
    
    // UI components
    std::shared_ptr<Container> header_;
    std::shared_ptr<Container> content_;
    std::shared_ptr<Button> expand_button_;
    
    std::vector<std::shared_ptr<PanelGroup>> panel_groups_;
    
    // Sizing
    float collapsed_width_;
    float expanded_width_;
    float current_width_;
    float target_width_;
    float min_width_;
    float max_width_;
    bool animating_ = false;
    
    // Callbacks
    StateChangeCallback state_change_callback_;
    EmptyCallback empty_callback_;
    
    void update_style_for_state() {
        switch (state_) {
            case ColumnState::COLLAPSED:
                base_style_.width = SizeValue(collapsed_width_);  // 36px
                base_style_.min_width = SizeValue(collapsed_width_);
                base_style_.set_overflow(WidgetStyle::Overflow::Hidden);
                current_width_ = collapsed_width_;
                // Hide content when collapsed - only show header
                if (content_) {
                    content_->set_visible(false);
                }
                break;
            case ColumnState::EXPANDED:
                base_style_.width = SizeValue(expanded_width_);
                base_style_.min_width = SizeValue(min_width_);
                base_style_.set_overflow(WidgetStyle::Overflow::Visible);
                current_width_ = expanded_width_;
                // Show content when expanded
                if (content_) {
                    content_->set_visible(true);
                }
                break;
            case ColumnState::HIDDEN:
                set_visible(false);
                break;
        }
    }
    
    void render_collapsed_icons(CanvasRenderer* renderer) {
        // Render vertical icon bar for collapsed state
        float icon_size = 24.0f;
        float padding = 6.0f;
        Rect2D bounds = get_bounds();
        float y = bounds.y + padding;
        
        for (const auto& group : panel_groups_) {
            for (const auto& panel : group->get_panels()) {
                // Get panel icon
                std::string icon_name = panel->get_icon();
                if (!icon_name.empty()) {
                    Point2D icon_pos(bounds.x + bounds.width / 2, y + icon_size / 2);
                    
                    // Draw icon
                    IconSystem& icon_system = IconSystem::get_instance();
                    IconAsset* icon_asset = icon_system.get_icon(icon_name);
                    if (icon_asset) {
                        icon_asset->render(renderer, icon_pos, icon_size,
                                          ColorRGBA(0.8f, 0.8f, 0.8f, 1.0f));
                    }
                    
                    y += icon_size + padding;
                }
            }
        }
    }
    
    void resize_group(int index, float delta) {
        // TODO: Implement group resizing
    }
};

} // namespace voxel_canvas