/*
 * Copyright (C) 2024 Voxelux
 * 
 * Core Styled Widgets - Basic UI components using the styling system
 * Replaces the old UIWidget system from ui_widgets.h
 */

#pragma once

#include "styled_widget.h"
#include "canvas_renderer.h"
#include "icon_system.h"
#include "font_system.h"
#include "layout_builder.h"
#include <string>
#include <functional>
#include <iostream>

namespace voxel_canvas {

/**
 * Text widget - Displays text with styling
 */
class Text : public StyledWidget {
public:
    explicit Text(const std::string& text = "") 
        : StyledWidget("text"), text_(text) {
        base_style_.display = WidgetStyle::Display::Inline;
    }
    
    void set_text(const std::string& text) { 
        text_ = text; 
        invalidate_layout();
    }
    const std::string& get_text() const { return text_; }
    
    std::string get_widget_type() const override { return "text"; }
    
    // Override for multi-pass layout to use text metrics
    IntrinsicSizes calculate_intrinsic_sizes() override {
        IntrinsicSizes sizes = StyledWidget::calculate_intrinsic_sizes();
        
        // If no explicit size set, use text measurement
        if (sizes.preferred_width == 0 && !text_.empty()) {
            Point2D text_size = get_content_size();
            sizes.preferred_width = text_size.x;
            sizes.preferred_height = text_size.y;
            sizes.min_width = text_size.x;
            sizes.min_height = text_size.y;
        }
        
        return sizes;
    }
    
    Point2D get_content_size() const override {
        if (text_.empty()) {
            return Point2D(0, 0);
        }
        
        // Use accurate text measurement with kerning
        if (g_font_system && computed_style_.font_size_pixels > 0) {
            TextMeasurement measurement = g_font_system->measure_text_accurate(
                text_, computed_style_.font_family.empty() ? "default" : computed_style_.font_family,
                computed_style_.font_size_pixels);
            return Point2D(measurement.width, measurement.height);
        }
        
        // Fallback if font system not available
        float char_width = 7.0f;  // Approximate
        float line_height = computed_style_.font_size_pixels * computed_style_.line_height;
        return Point2D(text_.length() * char_width, line_height);
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        if (text_.empty()) return;
        
        Rect2D content_bounds = get_content_bounds();
        Point2D text_pos;
        
        // Calculate text position based on alignment
        switch (computed_style_.text_align) {
            case WidgetStyle::TextAlign::Center:
                text_pos.x = content_bounds.x + content_bounds.width / 2;
                break;
            case WidgetStyle::TextAlign::Right:
                text_pos.x = content_bounds.x + content_bounds.width;
                break;
            default: // Left
                text_pos.x = content_bounds.x;
                break;
        }
        
        // Vertical alignment
        switch (computed_style_.vertical_align) {
            case WidgetStyle::VerticalAlign::Middle:
                text_pos.y = content_bounds.y + content_bounds.height / 2;
                break;
            case WidgetStyle::VerticalAlign::Bottom:
                text_pos.y = content_bounds.y + content_bounds.height;
                break;
            default: // Top
                text_pos.y = content_bounds.y + computed_style_.font_size_pixels;
                break;
        }
        
        // Map to renderer enums
        voxel_canvas::TextAlign align = voxel_canvas::TextAlign::LEFT;
        if (computed_style_.text_align == WidgetStyle::TextAlign::Center) {
            align = voxel_canvas::TextAlign::CENTER;
        } else if (computed_style_.text_align == WidgetStyle::TextAlign::Right) {
            align = voxel_canvas::TextAlign::RIGHT;
        }
        
        voxel_canvas::TextBaseline baseline = voxel_canvas::TextBaseline::TOP;
        if (computed_style_.vertical_align == WidgetStyle::VerticalAlign::Middle) {
            baseline = voxel_canvas::TextBaseline::MIDDLE;
        } else if (computed_style_.vertical_align == WidgetStyle::VerticalAlign::Bottom) {
            baseline = voxel_canvas::TextBaseline::BOTTOM;
        }
        
        renderer->draw_text(text_, text_pos, computed_style_.text_color_rgba, 
                          computed_style_.font_size_pixels, align, baseline);
    }
    
private:
    std::string text_;
};

/**
 * Icon widget - Displays an icon from the icon system
 */
class Icon : public StyledWidget {
public:
    explicit Icon(const std::string& icon_name = "", float size = 16.0f) 
        : StyledWidget("icon"), icon_name_(icon_name) {
        base_style_.width = SizeValue(size);
        base_style_.height = SizeValue(size);
        base_style_.display = WidgetStyle::Display::InlineBlock;
    }
    
    void set_icon(const std::string& name) { 
        icon_name_ = name; 
        invalidate_layout();
    }
    const std::string& get_icon() const { return icon_name_; }
    
    void set_icon_color(const ColorRGBA& color) {
        icon_color_ = color;
    }
    
    std::string get_widget_type() const override { return "icon"; }
    
    Point2D get_content_size() const override {
        return Point2D(computed_style_.width, computed_style_.height);
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        if (icon_name_.empty()) return;
        
        Rect2D content_bounds = get_content_bounds();
        IconSystem& icon_system = IconSystem::get_instance();
        
        // Get the icon asset from the system
        IconAsset* icon_asset = icon_system.get_icon(icon_name_);
        if (!icon_asset) {
            // Icon not found - render a placeholder rectangle for debugging
            ColorRGBA debug_color(1.0f, 0.0f, 0.0f, 0.5f); // Semi-transparent red
            renderer->draw_rect(content_bounds, debug_color);
            return;
        }
        
        // Icon should be rendered from top-left, not centered
        // The icon render function expects the top-left corner position
        Point2D icon_pos(
            content_bounds.x,
            content_bounds.y
        );
        
        float icon_size = std::min(content_bounds.width, content_bounds.height);
        ColorRGBA color = icon_color_.a > 0 ? icon_color_ : computed_style_.text_color_rgba;
        
        icon_asset->render(renderer, icon_pos, icon_size, color);
    }
    
private:
    std::string icon_name_;
    ColorRGBA icon_color_ = ColorRGBA(0, 0, 0, 0);  // Transparent = use text color
};


/**
 * Button widget - Clickable button with text and/or icon
 */
class Button : public StyledWidget {
public:
    explicit Button(const std::string& text = "") 
        : StyledWidget("button"), text_(text) {
        // No default styling - let the user decide
        // Just set the cursor to indicate it's clickable
        base_style_.cursor = "pointer";
    }
    
    void set_text(const std::string& text) { 
        text_ = text; 
        invalidate_layout();
    }
    
    void set_icon(const std::string& icon_name) {
        icon_name_ = icon_name;
        invalidate_layout();
    }
    
    void set_toggle_state(bool toggled) {
        is_toggled_ = toggled;
        if (toggled) {
            set_state(WidgetState::Active, true);
        } else {
            set_state(WidgetState::Active, false);
        }
    }
    
    bool is_toggled() const { return is_toggled_; }
    
    std::string get_widget_type() const override { return "button"; }
    
    // Override for multi-pass layout to use text metrics
    IntrinsicSizes calculate_intrinsic_sizes() override {
        // For buttons with AUTO width/height, we need to calculate from text
        // Don't call base class first since it might set wrong values
        IntrinsicSizes sizes;
        
        // Calculate content size
        Point2D content_size = get_content_size();
        
        // Set the base sizes from content
        if (computed_style_.width_unit == WidgetStyle::ComputedStyle::AUTO) {
            sizes.preferred_width = content_size.x;
            sizes.min_width = content_size.x;
            sizes.max_width = std::numeric_limits<float>::max();
        } else {
            // Use the base class calculation for non-AUTO widths
            IntrinsicSizes base_sizes = StyledWidget::calculate_intrinsic_sizes();
            sizes.preferred_width = base_sizes.preferred_width;
            sizes.min_width = base_sizes.min_width;
            sizes.max_width = base_sizes.max_width;
        }
        
        if (computed_style_.height_unit == WidgetStyle::ComputedStyle::AUTO) {
            sizes.preferred_height = content_size.y;
            sizes.min_height = content_size.y;
            sizes.max_height = std::numeric_limits<float>::max();
        } else {
            // Use the base class calculation for non-AUTO heights
            IntrinsicSizes base_sizes = StyledWidget::calculate_intrinsic_sizes();
            sizes.preferred_height = base_sizes.preferred_height;
            sizes.min_height = base_sizes.min_height;
            sizes.max_height = base_sizes.max_height;
        }
        
        // Add padding (like base class does)
        sizes.min_width += computed_style_.padding_left + computed_style_.padding_right;
        sizes.preferred_width += computed_style_.padding_left + computed_style_.padding_right;
        sizes.min_height += computed_style_.padding_top + computed_style_.padding_bottom;
        sizes.preferred_height += computed_style_.padding_top + computed_style_.padding_bottom;
        
        // Add border (like base class does)
        float border_width = computed_style_.border_width_left + computed_style_.border_width_right;
        float border_height = computed_style_.border_width_top + computed_style_.border_width_bottom;
        sizes.min_width += border_width;
        sizes.preferred_width += border_width;
        sizes.min_height += border_height;
        sizes.preferred_height += border_height;
        
        // Add margins
        sizes.min_width += computed_style_.margin_left + computed_style_.margin_right;
        sizes.preferred_width += computed_style_.margin_left + computed_style_.margin_right;
        sizes.min_height += computed_style_.margin_top + computed_style_.margin_bottom;
        sizes.preferred_height += computed_style_.margin_top + computed_style_.margin_bottom;
        
        return sizes;
    }
    
    Point2D get_content_size() const override {
        float width = 0, height = 0;
        
        if (!text_.empty()) {
            // Use accurate text measurement with kerning
            if (g_font_system && computed_style_.font_size_pixels > 0) {
                TextMeasurement measurement = g_font_system->measure_text_accurate(
                    text_, computed_style_.font_family.empty() ? "default" : computed_style_.font_family, 
                    computed_style_.font_size_pixels);
                width += measurement.width;
                height = measurement.height;
            } else {
                // Fallback if font system not available
                float char_width = computed_style_.font_size_pixels * 0.6f;
                width += text_.length() * char_width;
                height = computed_style_.font_size_pixels * computed_style_.line_height;
            }
        }
        
        if (!icon_name_.empty()) {
            float icon_size = computed_style_.font_size_pixels * 1.2f;
            if (!text_.empty()) {
                width += icon_size + computed_style_.gap_pixels;
            } else {
                width += icon_size;
            }
            height = std::max(height, icon_size);
        }
        
        return Point2D(width, height);
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D button_bounds = get_bounds();
        
        // Simple approach: just center text in the button
        // Icons can be handled separately if needed
        
        float center_x = button_bounds.x + button_bounds.width / 2;
        float center_y = button_bounds.y + button_bounds.height / 2;
        
        // If we have both icon and text, we need to calculate layout
        if (!icon_name_.empty() && !text_.empty()) {
            float icon_size = computed_style_.font_size_pixels * 1.2f;
            float char_width = computed_style_.font_size_pixels * 0.6f;
            float text_width = text_.length() * char_width;
            float total_width = icon_size + computed_style_.gap_pixels + text_width;
            
            // Start from left of centered content
            float x = center_x - total_width / 2;
            
            // Render icon
            IconSystem& icon_system = IconSystem::get_instance();
            Point2D icon_pos(x + icon_size / 2, center_y);
            icon_system.render_icon(renderer, icon_name_, icon_pos, icon_size, 
                                  computed_style_.text_color_rgba);
            
            // Render text after icon
            x += icon_size + computed_style_.gap_pixels;
            renderer->draw_text(text_, Point2D(x, center_y), computed_style_.text_color_rgba,
                              computed_style_.font_size_pixels, 
                              voxel_canvas::TextAlign::LEFT, 
                              voxel_canvas::TextBaseline::MIDDLE);
        }
        // Just icon
        else if (!icon_name_.empty()) {
            float icon_size = computed_style_.font_size_pixels * 1.2f;
            IconSystem& icon_system = IconSystem::get_instance();
            Point2D icon_pos(center_x, center_y);
            icon_system.render_icon(renderer, icon_name_, icon_pos, icon_size, 
                                  computed_style_.text_color_rgba);
        }
        // Just text - use CENTER alignment
        else if (!text_.empty()) {
            renderer->draw_text(text_, Point2D(center_x, center_y), computed_style_.text_color_rgba,
                              computed_style_.font_size_pixels, 
                              voxel_canvas::TextAlign::CENTER, 
                              voxel_canvas::TextBaseline::MIDDLE);
        }
    }
    
private:
    std::string text_;
    std::string icon_name_;
    bool is_toggled_ = false;
};

/**
 * Label widget - Combines icon and text in a row
 */
class Label : public Container {
public:
    Label(const std::string& text = "", const std::string& icon = "") 
        : Container() {
        set_id("label");
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Row;
        base_style_.align_items = WidgetStyle::AlignItems::Center;
        base_style_.gap = SpacingValue::theme_sm();
        
        if (!icon.empty()) {
            icon_widget_ = create_widget<Icon>(icon);
            add_child(icon_widget_);
        }
        
        if (!text.empty()) {
            text_widget_ = create_widget<Text>(text);
            add_child(text_widget_);
        }
    }
    
    void set_text(const std::string& text) {
        if (!text_widget_ && !text.empty()) {
            text_widget_ = create_widget<Text>(text);
            add_child(text_widget_);
        } else if (text_widget_) {
            text_widget_->set_text(text);
        }
    }
    
    void set_icon(const std::string& icon_name) {
        if (!icon_widget_ && !icon_name.empty()) {
            icon_widget_ = create_widget<Icon>(icon_name);
            // Remove all children and re-add in correct order
            std::vector<std::shared_ptr<StyledWidget>> temp_children = children_;
            remove_all_children();
            add_child(icon_widget_);
            for (auto& child : temp_children) {
                add_child(child);
            }
        } else if (icon_widget_) {
            icon_widget_->set_icon(icon_name);
        }
    }
    
    std::string get_widget_type() const override { return "label"; }
    
private:
    std::shared_ptr<Icon> icon_widget_;
    std::shared_ptr<Text> text_widget_;
};

/**
 * Separator widget - Visual separator line
 */
class Separator : public StyledWidget {
public:
    enum class Orientation {
        HORIZONTAL,
        VERTICAL
    };
    
    explicit Separator(Orientation orientation = Orientation::HORIZONTAL)
        : StyledWidget("separator"), orientation_(orientation) {
        if (orientation == Orientation::HORIZONTAL) {
            base_style_.width = SizeValue::percent(100);
            base_style_.height = SizeValue(1);
        } else {
            base_style_.width = SizeValue(1);
            base_style_.height = SizeValue::percent(100);
        }
        base_style_.background = ColorValue("gray_5");
        base_style_.margin = BoxSpacing(4, 0);
    }
    
    std::string get_widget_type() const override { return "separator"; }
    
private:
    [[maybe_unused]] Orientation orientation_;
};

} // namespace voxel_canvas