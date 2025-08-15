/*
 * Copyright (C) 2024 Voxelux
 * 
 * Form Styled Widgets - Input and form UI components
 */

#pragma once

#include "styled_widget.h"
#include "styled_widgets_core.h"
#include "layout_builder.h"
#include <string>
#include <vector>
#include <functional>

namespace voxel_canvas {

/**
 * Input widget - Text input field
 */
class Input : public StyledWidget {
public:
    explicit Input(const std::string& placeholder = "")
        : StyledWidget("input"), placeholder_(placeholder) {
        base_style_.display = WidgetStyle::Display::InlineBlock;
        base_style_.width = SizeValue(200);
        base_style_.height = SizeValue(28);
        base_style_.padding = BoxSpacing(4, 8);
        base_style_.background = ColorValue("gray_3");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_5");
        base_style_.border.radius = SpacingValue(3);
        base_style_.cursor = "text";
        
        // Focus state
        base_style_.focus_style = std::make_unique<WidgetStyle>();
        base_style_.focus_style->border.color = ColorValue("accent_primary");
    }
    
    void set_value(const std::string& value) {
        value_ = value;
        invalidate_layout();
    }
    
    const std::string& get_value() const { return value_; }
    
    void set_placeholder(const std::string& placeholder) {
        placeholder_ = placeholder;
    }
    
    void set_on_change(std::function<void(const std::string&)> callback) {
        on_change_ = callback;
    }
    
    std::string get_widget_type() const override { return "input"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D content_bounds = get_content_bounds();
        
        std::string display_text = value_.empty() ? placeholder_ : value_;
        ColorRGBA text_color = value_.empty() ? 
            ColorRGBA(0.5f, 0.5f, 0.5f, 1.0f) : computed_style_.text_color_rgba;
        
        // Draw text
        renderer->draw_text(display_text, 
                          Point2D(content_bounds.x, content_bounds.y + content_bounds.height / 2),
                          text_color, computed_style_.font_size_pixels,
                          voxel_canvas::TextAlign::LEFT, 
                          voxel_canvas::TextBaseline::MIDDLE);
        
        // Draw cursor if focused
        if (is_focused()) {
            float cursor_x = content_bounds.x + (value_.length() * 7.0f);  // Approximate
            renderer->draw_line(
                Point2D(cursor_x, content_bounds.y + 2),
                Point2D(cursor_x, content_bounds.y + content_bounds.height - 2),
                computed_style_.text_color_rgba, 1.0f
            );
        }
    }
    
    bool handle_event(const InputEvent& event) override {
        if (StyledWidget::handle_event(event)) {
            // Handle text input when focused
            if (is_focused() && event.type == EventType::KEY_PRESS) {
                // TODO: Handle keyboard input
                if (on_change_) {
                    on_change_(value_);
                }
            }
            return true;
        }
        return false;
    }
    
private:
    std::string value_;
    std::string placeholder_;
    std::function<void(const std::string&)> on_change_;
};

/**
 * Checkbox widget
 */
class Checkbox : public StyledWidget {
public:
    explicit Checkbox(const std::string& label = "", bool checked = false)
        : StyledWidget("checkbox"), label_(label), checked_(checked) {
        base_style_.display = WidgetStyle::Display::InlineBlock;
        base_style_.cursor = "pointer";
    }
    
    void set_checked(bool checked) {
        checked_ = checked;
        invalidate_layout();
    }
    
    bool is_checked() const { return checked_; }
    
    void set_on_change(std::function<void(bool)> callback) {
        on_change_ = callback;
    }
    
    std::string get_widget_type() const override { return "checkbox"; }
    
    Point2D get_content_size() const override {
        float width = 16;  // Checkbox size
        if (!label_.empty()) {
            width += 8 + label_.length() * 7;  // Gap + text
        }
        return Point2D(width, 16);
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D content_bounds = get_content_bounds();
        
        // Draw checkbox box
        Rect2D box_rect(content_bounds.x, content_bounds.y, 16, 16);
        renderer->draw_rect(box_rect, ColorRGBA(0.2f, 0.2f, 0.2f, 1.0f));
        renderer->draw_rect_outline(box_rect, computed_style_.text_color_rgba, 1.0f);
        
        // Draw checkmark if checked
        if (checked_) {
            IconSystem& icon_system = IconSystem::get_instance();
            icon_system.render_icon(renderer, "check", 
                                   Point2D(box_rect.x + 8, box_rect.y + 8), 
                                   12, ColorRGBA(0, 1, 0, 1));
        }
        
        // Draw label
        if (!label_.empty()) {
            renderer->draw_text(label_, 
                              Point2D(content_bounds.x + 24, content_bounds.y + 8),
                              computed_style_.text_color_rgba, 
                              computed_style_.font_size_pixels,
                              voxel_canvas::TextAlign::LEFT, 
                              voxel_canvas::TextBaseline::MIDDLE);
        }
    }
    
    bool handle_event(const InputEvent& event) override {
        if (StyledWidget::handle_event(event)) {
            if (event.type == EventType::MOUSE_PRESS) {
                checked_ = !checked_;
                if (on_change_) {
                    on_change_(checked_);
                }
                invalidate_layout();
            }
            return true;
        }
        return false;
    }
    
private:
    std::string label_;
    bool checked_;
    std::function<void(bool)> on_change_;
};

/**
 * Radio button widget
 */
class RadioButton : public StyledWidget {
public:
    explicit RadioButton(const std::string& label = "", const std::string& group = "default")
        : StyledWidget("radio"), label_(label), group_(group) {
        base_style_.display = WidgetStyle::Display::InlineBlock;
        base_style_.cursor = "pointer";
    }
    
    void set_selected(bool selected) {
        selected_ = selected;
        invalidate_layout();
    }
    
    bool is_selected() const { return selected_; }
    
    const std::string& get_group() const { return group_; }
    
    void set_on_change(std::function<void(bool)> callback) {
        on_change_ = callback;
    }
    
    std::string get_widget_type() const override { return "radio"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D content_bounds = get_content_bounds();
        
        // Draw radio circle
        Point2D center(content_bounds.x + 8, content_bounds.y + 8);
        renderer->draw_circle_ring(center, 8, computed_style_.text_color_rgba, 1.0f);
        
        // Draw inner circle if selected
        if (selected_) {
            renderer->draw_circle(center, 4, ColorRGBA(0, 0.5f, 1, 1));
        }
        
        // Draw label
        if (!label_.empty()) {
            renderer->draw_text(label_, 
                              Point2D(content_bounds.x + 24, content_bounds.y + 8),
                              computed_style_.text_color_rgba, 
                              computed_style_.font_size_pixels,
                              voxel_canvas::TextAlign::LEFT, 
                              voxel_canvas::TextBaseline::MIDDLE);
        }
    }
    
private:
    std::string label_;
    std::string group_;
    bool selected_ = false;
    std::function<void(bool)> on_change_;
};

/**
 * Dropdown/Select widget - Replaces EditorTypeDropdown from old system
 */
template<typename T>
class Dropdown : public StyledWidget {
public:
    struct Option {
        T value;
        std::string label;
        std::string icon;
    };
    
    explicit Dropdown(const std::string& placeholder = "Select...")
        : StyledWidget("dropdown"), placeholder_(placeholder) {
        base_style_.display = WidgetStyle::Display::InlineBlock;
        base_style_.width = SizeValue(200);
        base_style_.height = SizeValue(28);
        base_style_.padding = BoxSpacing(4, 8);
        base_style_.background = ColorValue("gray_4");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_5");
        base_style_.border.radius = SpacingValue(3);
        base_style_.cursor = "pointer";
        
        // Hover state
        base_style_.hover_style = std::make_unique<WidgetStyle>();
        base_style_.hover_style->background = ColorValue("gray_3");
    }
    
    void add_option(const T& value, const std::string& label, const std::string& icon = "") {
        options_.push_back({value, label, icon});
    }
    
    void set_selected(const T& value) {
        for (size_t i = 0; i < options_.size(); ++i) {
            if (options_[i].value == value) {
                selected_index_ = i;
                invalidate_layout();
                break;
            }
        }
    }
    
    T get_selected() const {
        if (selected_index_ >= 0 && selected_index_ < options_.size()) {
            return options_[selected_index_].value;
        }
        return T{};
    }
    
    void set_on_change(std::function<void(const T&)> callback) {
        on_change_ = callback;
    }
    
    std::string get_widget_type() const override { return "dropdown"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D content_bounds = get_content_bounds();
        
        // Draw selected value or placeholder
        std::string display_text = placeholder_;
        if (selected_index_ >= 0 && selected_index_ < options_.size()) {
            display_text = options_[selected_index_].label;
        }
        
        renderer->draw_text(display_text,
                          Point2D(content_bounds.x, content_bounds.y + content_bounds.height / 2),
                          computed_style_.text_color_rgba, 
                          computed_style_.font_size_pixels,
                          voxel_canvas::TextAlign::LEFT, 
                          voxel_canvas::TextBaseline::MIDDLE);
        
        // Draw dropdown arrow
        IconSystem& icon_system = IconSystem::get_instance();
        icon_system.render_icon(renderer, "chevron_down",
                               Point2D(content_bounds.right() - 12, 
                                     content_bounds.y + content_bounds.height / 2),
                               12, computed_style_.text_color_rgba);
        
        // Draw dropdown list if open
        if (is_open_) {
            render_dropdown_list(renderer);
        }
    }
    
    bool handle_event(const InputEvent& event) override {
        if (event.type == EventType::MOUSE_PRESS) {
            if (!is_open_) {
                is_open_ = true;
            } else {
                // Check if clicked on an option
                int clicked_option = get_option_at_point(event.mouse_pos);
                if (clicked_option >= 0) {
                    selected_index_ = clicked_option;
                    if (on_change_) {
                        on_change_(options_[selected_index_].value);
                    }
                }
                is_open_ = false;
            }
            invalidate_layout();
            return true;
        }
        return StyledWidget::handle_event(event);
    }
    
private:
    void render_dropdown_list(CanvasRenderer* renderer) {
        Rect2D list_bounds(
            bounds_.x,
            bounds_.y + bounds_.height,
            bounds_.width,
            options_.size() * 24  // Item height
        );
        
        // Draw background
        renderer->draw_rect(list_bounds, ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f));
        renderer->draw_rect_outline(list_bounds, ColorRGBA(0.3f, 0.3f, 0.3f, 1.0f));
        
        // Draw options
        for (size_t i = 0; i < options_.size(); ++i) {
            Rect2D item_bounds(
                list_bounds.x,
                list_bounds.y + i * 24,
                list_bounds.width,
                24
            );
            
            // Highlight hovered item
            if (i == hovered_index_) {
                renderer->draw_rect(item_bounds, ColorRGBA(0.2f, 0.3f, 0.5f, 1.0f));
            }
            
            // Draw option text
            renderer->draw_text(options_[i].label,
                              Point2D(item_bounds.x + 8, item_bounds.y + 12),
                              ColorRGBA(0.9f, 0.9f, 0.9f, 1.0f),
                              12, voxel_canvas::TextAlign::LEFT, 
                              voxel_canvas::TextBaseline::MIDDLE);
        }
    }
    
    int get_option_at_point(const Point2D& point) const {
        if (!is_open_) return -1;
        
        Rect2D list_bounds(
            bounds_.x,
            bounds_.y + bounds_.height,
            bounds_.width,
            options_.size() * 24
        );
        
        if (!list_bounds.contains(point)) return -1;
        
        int index = (point.y - list_bounds.y) / 24;
        if (index >= 0 && index < options_.size()) {
            return index;
        }
        return -1;
    }
    
private:
    std::string placeholder_;
    std::vector<Option> options_;
    int selected_index_ = -1;
    int hovered_index_ = -1;
    bool is_open_ = false;
    std::function<void(const T&)> on_change_;
};

/**
 * Slider widget
 */
class Slider : public StyledWidget {
public:
    explicit Slider(float min_value = 0, float max_value = 100, float value = 50)
        : StyledWidget("slider"), min_value_(min_value), max_value_(max_value), value_(value) {
        base_style_.display = WidgetStyle::Display::InlineBlock;
        base_style_.width = SizeValue(200);
        base_style_.height = SizeValue(20);
        base_style_.cursor = "pointer";
    }
    
    void set_value(float value) {
        value_ = std::max(min_value_, std::min(max_value_, value));
        invalidate_layout();
    }
    
    float get_value() const { return value_; }
    
    void set_range(float min, float max) {
        min_value_ = min;
        max_value_ = max;
        value_ = std::max(min_value_, std::min(max_value_, value_));
        invalidate_layout();
    }
    
    void set_on_change(std::function<void(float)> callback) {
        on_change_ = callback;
    }
    
    std::string get_widget_type() const override { return "slider"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        Rect2D content_bounds = get_content_bounds();
        
        // Draw track
        Rect2D track_rect(
            content_bounds.x,
            content_bounds.y + content_bounds.height / 2 - 2,
            content_bounds.width,
            4
        );
        renderer->draw_rounded_rect(track_rect, ColorRGBA(0.3f, 0.3f, 0.3f, 1.0f), 2);
        
        // Draw filled portion
        float ratio = (value_ - min_value_) / (max_value_ - min_value_);
        Rect2D filled_rect(
            track_rect.x,
            track_rect.y,
            track_rect.width * ratio,
            track_rect.height
        );
        renderer->draw_rounded_rect(filled_rect, ColorRGBA(0.2f, 0.5f, 1.0f, 1.0f), 2);
        
        // Draw handle
        float handle_x = content_bounds.x + content_bounds.width * ratio;
        renderer->draw_circle(Point2D(handle_x, content_bounds.y + content_bounds.height / 2),
                            8, ColorRGBA(0.8f, 0.8f, 0.8f, 1.0f));
        
        // Draw value text if hovered
        if (is_hovered()) {
            std::string value_text = std::to_string(static_cast<int>(value_));
            renderer->draw_text(value_text,
                              Point2D(handle_x, content_bounds.y - 5),
                              ColorRGBA(0.9f, 0.9f, 0.9f, 1.0f),
                              10, voxel_canvas::TextAlign::CENTER, 
                              voxel_canvas::TextBaseline::BOTTOM);
        }
    }
    
    bool handle_event(const InputEvent& event) override {
        if (event.type == EventType::MOUSE_PRESS || 
            (event.type == EventType::MOUSE_MOVE && is_dragging_)) {
            
            if (bounds_.contains(event.mouse_pos)) {
                float ratio = (event.mouse_pos.x - bounds_.x) / bounds_.width;
                ratio = std::max(0.0f, std::min(1.0f, ratio));
                float new_value = min_value_ + ratio * (max_value_ - min_value_);
                
                if (new_value != value_) {
                    value_ = new_value;
                    if (on_change_) {
                        on_change_(value_);
                    }
                    invalidate_layout();
                }
                
                is_dragging_ = (event.type == EventType::MOUSE_PRESS);
                return true;
            }
        } else if (event.type == EventType::MOUSE_RELEASE) {
            is_dragging_ = false;
        }
        
        return StyledWidget::handle_event(event);
    }
    
private:
    float min_value_;
    float max_value_;
    float value_;
    bool is_dragging_ = false;
    std::function<void(float)> on_change_;
};

} // namespace voxel_canvas