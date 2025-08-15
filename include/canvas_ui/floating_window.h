/*
 * Copyright (C) 2024 Voxelux
 * 
 * Floating Window - Window for undocked panels
 * MIGRATED to new styled widget system
 */

#pragma once

#include "canvas_ui/styled_widgets.h"
#include "canvas_ui/dock_column.h"
#include <memory>

namespace voxel_canvas {

class CanvasRenderer;

/**
 * FloatingWindow - A window that contains floating panels
 * Now uses the new styled widget system with automatic layout
 */
class FloatingWindow : public Container {
public:
    // Constants
    static constexpr float HEADER_HEIGHT = 30.0f;
    
    FloatingWindow(const Rect2D& initial_bounds) : Container("floating-window") {
        bounds_ = initial_bounds;
        
        // Configure window styling
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.position = WidgetStyle::Position::Absolute;
        base_style_.left = SpacingValue(initial_bounds.x);
        base_style_.top = SpacingValue(initial_bounds.y);
        base_style_.width = SizeValue(initial_bounds.width);
        base_style_.height = SizeValue(initial_bounds.height);
        base_style_.background = ColorValue("gray_4");
        base_style_.border.width = SpacingValue(1);
        base_style_.border.color = ColorValue("gray_5");
        base_style_.border.radius = SpacingValue(4);
        base_style_.box_shadow = ShadowStyle{
            .offset_x = 2,
            .offset_y = 2,
            .blur_radius = 10,
            .spread = 0,
            .color = ColorValue(ColorRGBA(0, 0, 0, 0.3f))
        };
        base_style_.z_index = 100;  // Float above other content
        
        // Create window header
        create_header();
        
        // Create content area
        content_area_ = LayoutBuilder::column();
        content_area_->set_id("floating-content");
        content_area_->get_style()
            .set_flex_grow(1)
            .set_padding(BoxSpacing::theme());
        add_child(content_area_);
    }
    
    ~FloatingWindow() override = default;
    
    // Panel management
    void set_panel_group(std::shared_ptr<PanelGroup> group) {
        panel_group_ = group;
        content_area_->remove_all_children();
        if (group) {
            content_area_->add_child(group);
            
            // Update title from first panel
            if (group->has_panels()) {
                auto panels = group->get_panels();
                if (!panels.empty()) {
                    set_title(panels[0]->get_title());
                }
            }
        }
        invalidate_layout();
    }
    
    std::shared_ptr<PanelGroup> get_panel_group() const { return panel_group_; }
    
    // Window state
    void set_title(const std::string& title) { 
        title_ = title;
        if (title_text_) {
            title_text_->set_text(title);
        }
    }
    
    const std::string& get_title() const { return title_; }
    
    void set_ghost_mode(bool ghost) { 
        is_ghost_ = ghost;
        if (ghost) {
            base_style_.opacity = 0.5f;
            base_style_.pointer_events = WidgetStyle::PointerEvents::None;
        } else {
            base_style_.opacity = 1.0f;
            base_style_.pointer_events = WidgetStyle::PointerEvents::Auto;
        }
        invalidate_style();
    }
    
    bool is_ghost_mode() const { return is_ghost_; }
    
    void set_opacity(float opacity) { 
        opacity_ = opacity;
        base_style_.opacity = opacity;
        invalidate_style();
    }
    
    float get_opacity() const { return opacity_; }
    
    // Movement
    void start_drag(const Point2D& mouse_pos) {
        is_dragging_ = true;
        drag_offset_ = Point2D(mouse_pos.x - bounds_.x, mouse_pos.y - bounds_.y);
    }
    
    void update_drag(const Point2D& mouse_pos) {
        if (is_dragging_) {
            float new_x = mouse_pos.x - drag_offset_.x;
            float new_y = mouse_pos.y - drag_offset_.y;
            
            base_style_.left = SpacingValue(new_x);
            base_style_.top = SpacingValue(new_y);
            bounds_.x = new_x;
            bounds_.y = new_y;
            
            invalidate_layout();
        }
    }
    
    void end_drag() {
        is_dragging_ = false;
    }
    
    bool is_dragging() const { return is_dragging_; }
    
    // Resize handle types
    enum class ResizeHandle {
        NONE,
        TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT,
        TOP, BOTTOM, LEFT, RIGHT
    };
    
    // Resizing
    void start_resize(const Point2D& mouse_pos, ResizeHandle handle) {
        is_resizing_ = true;
        resize_handle_ = handle;
        resize_start_pos_ = mouse_pos;
        resize_start_bounds_ = bounds_;
    }
    
    void update_resize(const Point2D& mouse_pos) {
        if (!is_resizing_) return;
        
        float dx = mouse_pos.x - resize_start_pos_.x;
        float dy = mouse_pos.y - resize_start_pos_.y;
        
        Rect2D new_bounds = resize_start_bounds_;
        
        // Apply resize based on handle
        switch (resize_handle_) {
            case ResizeHandle::TOP_LEFT:
                new_bounds.x += dx;
                new_bounds.y += dy;
                new_bounds.width -= dx;
                new_bounds.height -= dy;
                break;
            case ResizeHandle::TOP_RIGHT:
                new_bounds.y += dy;
                new_bounds.width += dx;
                new_bounds.height -= dy;
                break;
            case ResizeHandle::BOTTOM_LEFT:
                new_bounds.x += dx;
                new_bounds.width -= dx;
                new_bounds.height += dy;
                break;
            case ResizeHandle::BOTTOM_RIGHT:
                new_bounds.width += dx;
                new_bounds.height += dy;
                break;
            case ResizeHandle::TOP:
                new_bounds.y += dy;
                new_bounds.height -= dy;
                break;
            case ResizeHandle::BOTTOM:
                new_bounds.height += dy;
                break;
            case ResizeHandle::LEFT:
                new_bounds.x += dx;
                new_bounds.width -= dx;
                break;
            case ResizeHandle::RIGHT:
                new_bounds.width += dx;
                break;
            case ResizeHandle::NONE:
                // No resize action needed
                break;
        }
        
        // Apply minimum size constraints
        new_bounds.width = std::max(MIN_WIDTH, new_bounds.width);
        new_bounds.height = std::max(MIN_HEIGHT, new_bounds.height);
        
        // Update bounds and style
        bounds_ = new_bounds;
        base_style_.left = SpacingValue(new_bounds.x);
        base_style_.top = SpacingValue(new_bounds.y);
        base_style_.width = SizeValue(new_bounds.width);
        base_style_.height = SizeValue(new_bounds.height);
        
        invalidate_layout();
    }
    
    void end_resize() {
        is_resizing_ = false;
    }
    
    bool is_resizing() const { return is_resizing_; }
    
    // StyledWidget interface
    std::string get_widget_type() const override { return "floating-window"; }
    
    bool handle_event(const InputEvent& event) override {
        // Handle window-specific events
        if (event.type == EventType::MOUSE_PRESS) {
            // Check if clicking on header for drag
            if (header_ && header_->get_bounds().contains(event.mouse_pos)) {
                start_drag(event.mouse_pos);
                return true;
            }
            
            // Check for resize handles
            ResizeHandle handle = get_resize_handle_at(event.mouse_pos);
            if (handle != ResizeHandle::NONE) {
                start_resize(event.mouse_pos, handle);
                return true;
            }
        } else if (event.type == EventType::MOUSE_RELEASE) {
            if (is_dragging_) {
                end_drag();
                return true;
            }
            if (is_resizing_) {
                end_resize();
                return true;
            }
        } else if (event.type == EventType::MOUSE_MOVE) {
            if (is_dragging_) {
                update_drag(event.mouse_pos);
                return true;
            }
            if (is_resizing_) {
                update_resize(event.mouse_pos);
                return true;
            }
        }
        
        // Pass to base class for child handling
        return Container::handle_event(event);
    }
    
    // Close callback
    using CloseCallback = std::function<void()>;
    void set_close_callback(CloseCallback callback) { close_callback_ = callback; }
    
private:
    void create_header() {
        header_ = LayoutBuilder::row();
        header_->set_id("floating-header");
        header_->get_style()
            .set_height(SizeValue(30))
            .set_padding(BoxSpacing(0, SpacingValue::theme_md()))
            .set_background(ColorValue("gray_5"))
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_gap(SpacingValue::theme_sm());
        
        // Title text
        title_text_ = create_widget<Text>(title_);
        title_text_->get_style()
            .set_font_weight(700)  // Bold
            .set_flex_grow(1);
        header_->add_child(title_text_);
        
        // Close button
        auto close_btn = create_widget<Button>();
        close_btn->set_icon("close");
        close_btn->get_style()
            .set_width(SizeValue(20))
            .set_height(SizeValue(20))
            .set_padding(BoxSpacing::all(2))
            .set_background(ColorValue("transparent"))
            .set_border(BorderStyle{.width = SpacingValue(0)});
        
        close_btn->set_on_click([this](StyledWidget*) {
            if (close_callback_) {
                close_callback_();
            }
        });
        
        header_->add_child(close_btn);
        add_child(header_);
    }
    
    ResizeHandle get_resize_handle_at(const Point2D& pos) const {
        const float HANDLE_SIZE = 8.0f;
        Rect2D bounds = get_bounds();
        
        // Check corners first
        if (pos.x < bounds.x + HANDLE_SIZE && pos.y < bounds.y + HANDLE_SIZE)
            return ResizeHandle::TOP_LEFT;
        if (pos.x > bounds.x + bounds.width - HANDLE_SIZE && pos.y < bounds.y + HANDLE_SIZE)
            return ResizeHandle::TOP_RIGHT;
        if (pos.x < bounds.x + HANDLE_SIZE && pos.y > bounds.y + bounds.height - HANDLE_SIZE)
            return ResizeHandle::BOTTOM_LEFT;
        if (pos.x > bounds.x + bounds.width - HANDLE_SIZE && pos.y > bounds.y + bounds.height - HANDLE_SIZE)
            return ResizeHandle::BOTTOM_RIGHT;
        
        // Check edges
        if (pos.x < bounds.x + HANDLE_SIZE)
            return ResizeHandle::LEFT;
        if (pos.x > bounds.x + bounds.width - HANDLE_SIZE)
            return ResizeHandle::RIGHT;
        if (pos.y < bounds.y + HANDLE_SIZE)
            return ResizeHandle::TOP;
        if (pos.y > bounds.y + bounds.height - HANDLE_SIZE)
            return ResizeHandle::BOTTOM;
        
        return ResizeHandle::NONE;
    }
    
private:
    std::string title_;
    std::shared_ptr<PanelGroup> panel_group_;
    std::shared_ptr<Container> header_;
    std::shared_ptr<Container> content_area_;
    std::shared_ptr<Text> title_text_;
    
    // Window state
    bool is_ghost_ = false;
    float opacity_ = 1.0f;
    
    // Dragging
    bool is_dragging_ = false;
    Point2D drag_offset_;
    
    // Resizing
    bool is_resizing_ = false;
    ResizeHandle resize_handle_ = ResizeHandle::NONE;
    Point2D resize_start_pos_;
    Rect2D resize_start_bounds_;
    
    // Callbacks
    CloseCallback close_callback_;
    
    // Constants
    static constexpr float MIN_WIDTH = 200.0f;
    static constexpr float MIN_HEIGHT = 150.0f;
};

/**
 * FloatingWindowManager - Manages all floating windows in the application
 */
class FloatingWindowManager {
public:
    static FloatingWindowManager& get_instance() {
        static FloatingWindowManager instance;
        return instance;
    }
    
    // Create floating window from panel
    void float_panel(std::shared_ptr<Panel> panel, const Point2D& position) {
        auto window = std::make_shared<FloatingWindow>(Rect2D(position.x, position.y, 400, 300));
        
        // Create a panel group with just this panel
        auto group = std::make_shared<PanelGroup>();
        group->add_panel(panel);
        window->set_panel_group(group);
        
        // Set close callback to remove from manager
        window->set_close_callback([this, window]() {
            remove_window(window);
        });
        
        windows_.push_back(window);
    }
    
    // Create floating window from panel group
    void float_panel_group(std::shared_ptr<PanelGroup> group, const Point2D& position) {
        auto window = std::make_shared<FloatingWindow>(Rect2D(position.x, position.y, 400, 300));
        window->set_panel_group(group);
        
        // Set close callback to remove from manager
        window->set_close_callback([this, window]() {
            remove_window(window);
        });
        
        windows_.push_back(window);
    }
    
    // Render all floating windows
    void render(CanvasRenderer* renderer) {
        for (auto& window : windows_) {
            window->render(renderer);
        }
    }
    
    // Handle events for all floating windows
    bool handle_event(const InputEvent& event) {
        // Process in reverse order so topmost windows get events first
        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if ((*it)->handle_event(event)) {
                return true;
            }
        }
        return false;
    }
    
    // Get all windows
    const std::vector<std::shared_ptr<FloatingWindow>>& get_windows() const {
        return windows_;
    }
    
    // Clear all windows
    void clear() {
        windows_.clear();
    }
    
private:
    FloatingWindowManager() = default;
    
    void remove_window(std::shared_ptr<FloatingWindow> window) {
        auto it = std::find(windows_.begin(), windows_.end(), window);
        if (it != windows_.end()) {
            windows_.erase(it);
        }
    }
    
    std::vector<std::shared_ptr<FloatingWindow>> windows_;
};

} // namespace voxel_canvas