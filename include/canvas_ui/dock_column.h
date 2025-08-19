/*
 * Copyright (C) 2024 Voxelux
 * 
 * Dock Column - Manages a vertical column of panel groups
 * MIGRATED to new styled widget system
 */

#pragma once

#include "canvas_ui/styled_widgets.h"
#include "canvas_ui/icon_system.h"
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <limits>

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
        
        // Initialize user widths with defaults (logical pixels)
        if (type == ColumnType::TOOL) {
            // Tool columns have fixed widths
            collapsed_user_width_ = 36.0f;   // Fixed collapsed width
            expanded_user_width_ = 72.0f;    // Fixed expanded width (2-column grid)
        } else {
            // Regular columns have resizable widths
            collapsed_user_width_ = 36.0f;   // Default collapsed width
            expanded_user_width_ = 250.0f;   // Default expanded width
        }
        
        // Apply initial state styling
        update_style_for_state();
        
        // Create header (14px tall with expand/collapse button)
        header_ = create_widget<Container>("dock-column-header");
        header_->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Row)
            .set_height(SizeValue(14))
            .set_background(ColorValue("gray_4"))
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
            
            // Trigger animation with appropriate target width
            if (state == ColumnState::COLLAPSED) {
                target_width_ = (type_ == ColumnType::TOOL) ? 36.0f : collapsed_user_width_;
                animating_ = true;
            } else if (state == ColumnState::EXPANDED) {
                target_width_ = (type_ == ColumnType::TOOL) ? 72.0f : expanded_user_width_;
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
                resize_group(static_cast<int>(index), delta);
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
    
    // Sizing - all in logical pixels
    float get_current_width() const { 
        return (state_ == ColumnState::COLLAPSED) ? collapsed_user_width_ : expanded_user_width_;
    }
    
    // Resize handling for dragging
    void resize_by_delta(float delta_logical_pixels) {
        if (type_ == ColumnType::TOOL) {
            // Tool columns don't resize
            return;
        }
        
        if (state_ == ColumnState::COLLAPSED) {
            // Collapsed: min 36px, max 225px
            collapsed_user_width_ = std::max(36.0f, std::min(225.0f, collapsed_user_width_ + delta_logical_pixels));
            base_style_.width = SizeValue(collapsed_user_width_);
        } else {
            // Expanded: min 250px, no max
            expanded_user_width_ = std::max(250.0f, expanded_user_width_ + delta_logical_pixels);
            base_style_.width = SizeValue(expanded_user_width_);
        }
        invalidate_layout();
    }
    
    // Animation
    void update(float delta_time) {
        if (animating_) {
            float speed = 500.0f; // logical pixels per second
            float max_change = speed * delta_time;
            
            float current_width = get_current_width();
            if (std::abs(current_width - target_width_) < max_change) {
                if (state_ == ColumnState::COLLAPSED) {
                    collapsed_user_width_ = target_width_;
                } else {
                    expanded_user_width_ = target_width_;
                }
                animating_ = false;
            } else {
                float new_width = current_width + (target_width_ > current_width ? max_change : -max_change);
                if (state_ == ColumnState::COLLAPSED) {
                    collapsed_user_width_ = new_width;
                } else {
                    expanded_user_width_ = new_width;
                }
            }
            
            base_style_.width = SizeValue(get_current_width());
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
    
    // Override to provide intrinsic sizes based on state
    IntrinsicSizes calculate_intrinsic_sizes() override {
        IntrinsicSizes sizes;
        
        // Let the style system handle width - it will apply scaling
        // Don't override with unscaled values
        // The base class will use our style settings
        sizes = Container::calculate_intrinsic_sizes();
        
        // Height comes from content
        float height = 0;
        
        // Add header height
        if (header_ && header_->is_visible()) {
            height += 14.0f;  // Fixed header height
        }
        
        // If expanded, add content height
        if (state_ == ColumnState::EXPANDED && content_ && content_->is_visible()) {
            // Call parent's method to calculate children sizes
            IntrinsicSizes content_sizes = Container::calculate_intrinsic_sizes();
            height += content_sizes.preferred_height;
        } else if (state_ == ColumnState::COLLAPSED) {
            // In collapsed state, calculate height for icons
            float icon_size = 24.0f;
            float padding = 6.0f;
            int icon_count = 0;
            
            for (const auto& group : panel_groups_) {
                for (const auto& panel : group->get_panels()) {
                    if (!panel->get_icon().empty()) {
                        icon_count++;
                    }
                }
            }
            
            if (icon_count > 0) {
                height += padding + (icon_size + padding) * icon_count;
            }
        }
        
        sizes.min_height = height;
        sizes.preferred_height = height;
        // Don't limit max height - allow it to grow to fill available space
        sizes.max_height = std::numeric_limits<float>::max();
        
        return sizes;
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        // Render background for collapsed state
        if (state_ == ColumnState::COLLAPSED) {
            // Draw gray_3 background for the icon area (below the header)
            Rect2D icon_area = get_bounds();
            float header_height = 14.0f; // Header is 14px tall
            icon_area.y += header_height;
            icon_area.height -= header_height;
            
            // Use the theme's gray_3 color
            ColorRGBA gray_3 = renderer->get_scaled_theme().gray_3();
            renderer->draw_rect(icon_area, gray_3);
            
            // Render icons on top of the background
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
    
    // Sizing - stored in logical pixels (unscaled)
    float collapsed_user_width_;  // User's resized collapsed width
    float expanded_user_width_;   // User's resized expanded width
    float target_width_;          // For animation
    bool animating_ = false;
    
    // Callbacks
    StateChangeCallback state_change_callback_;
    EmptyCallback empty_callback_;
    
    void update_style_for_state() {
        switch (state_) {
            case ColumnState::COLLAPSED:
                if (type_ == ColumnType::TOOL) {
                    // Tool columns have fixed width
                    base_style_.width = SizeValue(36);  // Fixed, will be scaled
                } else {
                    // Regular columns use remembered width
                    base_style_.width = SizeValue(collapsed_user_width_);  // Will be scaled
                    base_style_.min_width = SizeValue(36);
                    base_style_.max_width = SizeValue(225);
                }
                base_style_.set_overflow(WidgetStyle::Overflow::Hidden);
                // Hide content when collapsed - only show header
                if (content_) {
                    content_->set_visible(false);
                }
                // Force style recomputation and layout
                invalidate_style();
                invalidate_layout();
                break;
                
            case ColumnState::EXPANDED:
                if (type_ == ColumnType::TOOL) {
                    // Tool columns have fixed width
                    base_style_.width = SizeValue(72);  // Fixed for 2-column grid, will be scaled
                } else {
                    // Regular columns use remembered width
                    base_style_.width = SizeValue(expanded_user_width_);  // Will be scaled
                    base_style_.min_width = SizeValue(250);
                    // No max_width for expanded state - unlimited
                }
                base_style_.set_overflow(WidgetStyle::Overflow::Visible);
                // Show content when expanded
                if (content_) {
                    content_->set_visible(true);
                }
                // Force style recomputation and layout
                invalidate_style();
                invalidate_layout();
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
        float y = bounds.y + header_->get_bounds().height + padding;  // Start after header
        
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
    
    void resize_group([[maybe_unused]] int index, [[maybe_unused]] float delta) {
        // TODO: Implement group resizing
    }
};

} // namespace voxel_canvas