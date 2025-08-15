/*
 * Copyright (C) 2024 Voxelux
 * 
 * Styled Widget - Base class for all UI widgets with CSS-like styling
 */

#pragma once

#include "canvas_core.h"
#include "widget_style.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace voxel_canvas {

// Forward declarations
class CanvasRenderer;
class InputEvent;
class RenderBlock;

/**
 * Base class for all styled UI widgets
 * Any widget can be a container with layout properties
 */
class StyledWidget : public std::enable_shared_from_this<StyledWidget> {
    friend class RenderBlock;  // Allow RenderBlock to manage rendering efficiently
    
public:
    // Widget state flags
    enum class WidgetState {
        Normal = 0,
        Hovered = 1 << 0,
        Active = 1 << 1,
        Focused = 1 << 2,
        Disabled = 1 << 3,
        Selected = 1 << 4
    };
    
    // Constructor/Destructor
    StyledWidget();
    explicit StyledWidget(const std::string& id);
    virtual ~StyledWidget() = default;
    
    // Styling
    void set_style(const WidgetStyle& style);
    void set_style_json(const std::string& json);
    void set_style_property(const std::string& property, const std::string& value);
    WidgetStyle& get_style() { return base_style_; }
    const WidgetStyle& get_style() const { return base_style_; }
    
    // Style classes (like CSS classes)
    void add_class(const std::string& class_name);
    void remove_class(const std::string& class_name);
    bool has_class(const std::string& class_name) const;
    void set_classes(const std::vector<std::string>& classes) { style_classes_ = classes; }
    const std::vector<std::string>& get_classes() const { return style_classes_; }
    
    // ID for styling and identification
    void set_id(const std::string& id) { id_ = id; }
    const std::string& get_id() const { return id_; }
    
    // Widget type (for type-based styling)
    virtual std::string get_widget_type() const { return "widget"; }
    
    // State management
    void set_state(WidgetState state, bool enabled);
    bool has_state(WidgetState state) const;
    void set_hovered(bool hovered) { set_state(WidgetState::Hovered, hovered); }
    void set_active(bool active) { set_state(WidgetState::Active, active); }
    void set_focused(bool focused) { set_state(WidgetState::Focused, focused); }
    void set_disabled(bool disabled) { set_state(WidgetState::Disabled, disabled); }
    void set_selected(bool selected) { set_state(WidgetState::Selected, selected); }
    
    bool is_hovered() const { return has_state(WidgetState::Hovered); }
    bool is_active() const { return has_state(WidgetState::Active); }
    bool is_focused() const { return has_state(WidgetState::Focused); }
    bool is_disabled() const { return has_state(WidgetState::Disabled); }
    bool is_selected() const { return has_state(WidgetState::Selected); }
    
    // Hierarchy (any widget can be a container)
    void add_child(std::shared_ptr<StyledWidget> child);
    void remove_child(std::shared_ptr<StyledWidget> child);
    void remove_all_children();
    std::shared_ptr<StyledWidget> find_child(const std::string& id) const;
    std::vector<std::shared_ptr<StyledWidget>>& get_children() { return children_; }
    const std::vector<std::shared_ptr<StyledWidget>>& get_children() const { return children_; }
    void set_parent(StyledWidget* parent) { parent_ = parent; }
    StyledWidget* get_parent() { return parent_; }
    const StyledWidget* get_parent() const { return parent_; }
    
    // Layout and bounds
    void set_bounds(const Rect2D& bounds);
    const Rect2D& get_bounds() const { return bounds_; }
    Rect2D get_content_bounds() const;  // Bounds minus padding and border
    Rect2D get_padding_bounds() const;  // Bounds minus border
    
    // Content size (for auto sizing)
    virtual Point2D get_content_size() const;
    void set_content_size(const Point2D& size) { content_size_ = size; }
    
    // Visibility
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    // Rendering
    void render(CanvasRenderer* renderer);
    
    // Event handling
    virtual bool handle_event(const InputEvent& event);
    
    // Mouse state tracking
    bool is_mouse_over() const { return is_mouse_over_; }
    bool is_pressed() const { return is_pressed_; }
    
    // Callbacks
    using EventCallback = std::function<void(StyledWidget*)>;
    void set_on_click(EventCallback callback) { on_click_ = callback; }
    void set_on_hover(EventCallback callback) { on_hover_ = callback; }
    void set_on_focus(EventCallback callback) { on_focus_ = callback; }
    
protected:
    // Event handlers (override these for custom behavior)
    virtual void on_mouse_enter() {}
    virtual void on_mouse_leave() {}
    virtual void on_mouse_press(const InputEvent& event) {}
    virtual void on_mouse_release(const InputEvent& event) {}
    virtual void on_mouse_move(const InputEvent& event) {}
    
    // Layout management
    void perform_layout();  // Recursively layout this widget and children
    void invalidate_layout();  // Mark as needing layout (respects batch mode)
    bool needs_layout() const { return needs_layout_; }
    bool needs_style_computation() const { return needs_style_computation_; }
    void request_layout();  // Request layout to be performed (will be done automatically before render)
    
    // Style computation
    void compute_style(const ScaledTheme& theme);
    const WidgetStyle::ComputedStyle& get_computed_style() const { return computed_style_; }
    void invalidate_style();  // Mark as needing style update (respects batch mode)
    
    // Mark this widget's area as needing redraw
    void mark_dirty();
    
protected:
    // Virtual methods for subclasses
    virtual void render_background(CanvasRenderer* renderer);
    virtual void render_border(CanvasRenderer* renderer);
    virtual void render_content(CanvasRenderer* renderer);
    virtual void render_children(CanvasRenderer* renderer);
    virtual void render_effects(CanvasRenderer* renderer);
    
    // Helper method for gradient rendering
    void render_gradient(CanvasRenderer* renderer, const Rect2D& bounds, const WidgetStyle::Gradient& gradient);
    virtual void render_outline(CanvasRenderer* renderer);  // Shadows, etc.
    
    // Layout algorithms
    virtual void layout_children();  // Apply layout based on display type
    void layout_flex();              // Flexbox layout
    void layout_block();             // Block layout (vertical stack)
    void layout_inline();            // Inline layout (horizontal flow)
    void layout_grid();              // Grid layout (future)
    
    // Helper methods
    Rect2D calculate_child_bounds(std::shared_ptr<StyledWidget> child, int index);
    void apply_state_style();  // Apply hover/active/focus/disabled styles
    
protected:
    // Identity
    std::string id_;
    std::vector<std::string> style_classes_;
    
    // Styling
    WidgetStyle base_style_;
    WidgetStyle current_style_;  // After state applied
    WidgetStyle::ComputedStyle computed_style_;
    bool needs_style_computation_ = true;
    
    // State
    uint32_t state_flags_ = 0;
    
    // Mouse interaction state
    bool is_mouse_over_ = false;
    bool is_pressed_ = false;
    
    // Layout
    Rect2D bounds_;
    Point2D content_size_;  // Intrinsic content size
    bool needs_layout_ = true;
    bool visible_ = true;
    
    // Hierarchy
    StyledWidget* parent_ = nullptr;
    std::vector<std::shared_ptr<StyledWidget>> children_;
    
    // Callbacks
    EventCallback on_click_;
    EventCallback on_hover_;
    EventCallback on_focus_;
    
private:
    // Flexbox layout helpers
    struct FlexItem {
        StyledWidget* widget;  // Pointer to child widget
        float flex_grow;
        float flex_shrink;
        float flex_basis;
        float main_size;   // Computed size on main axis
        float cross_size;  // Computed size on cross axis
        float margin_before = 0;  // Margin before the item on main axis
        float margin_after = 0;   // Margin after the item on main axis
        float margin_cross_before = 0;  // Margin on cross axis start
        float margin_cross_after = 0;   // Margin on cross axis end
    };
    
    struct FlexLine {
        std::vector<FlexItem*> items;
        float main_size = 0;       // Total size on main axis
        float cross_size = 0;       // Max size on cross axis  
        float main_space_used = 0; // Space used including margins
    };
    
    void position_flex_line(const FlexLine& line, const Rect2D& content_bounds, 
                           float cross_pos, float line_cross_size);
    
    // Legacy functions - may be removed after refactoring
    void calculate_flex_main_axis(std::vector<FlexItem>& items, float available_space);
    void calculate_flex_cross_axis(std::vector<FlexItem>& items, float available_space);
    void position_flex_items(const std::vector<FlexItem>& items);
};

/**
 * Container widget - A simple widget that just contains other widgets
 * Useful for grouping and layout
 */
class Container : public StyledWidget {
public:
    Container() : StyledWidget("container") {}
    Container(const std::string& id) : StyledWidget(id) {}
    std::string get_widget_type() const override { return "container"; }
};

/**
 * Spacer widget - Takes up space in layouts
 */
class Spacer : public StyledWidget {
public:
    explicit Spacer(float size = 0) : StyledWidget("spacer") {
        if (size > 0) {
            base_style_.width = SizeValue(size);
            base_style_.height = SizeValue(size);
        } else {
            // Flex spacer that grows to fill available space
            base_style_.flex_grow = 1;
        }
    }
    std::string get_widget_type() const override { return "spacer"; }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        // Spacers don't render anything
    }
};

/**
 * Helper function to create a widget
 */
template<typename T, typename... Args>
std::shared_ptr<T> create_widget(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * Layout builder helpers for fluent API
 */
class LayoutBuilder {
public:
    static std::shared_ptr<Container> row() {
        auto container = create_widget<Container>();
        container->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Row);
        return container;
    }
    
    static std::shared_ptr<Container> column() {
        auto container = create_widget<Container>();
        container->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Column);
        return container;
    }
    
    static std::shared_ptr<Container> flex(WidgetStyle::FlexDirection direction = WidgetStyle::FlexDirection::Row) {
        auto container = create_widget<Container>();
        container->get_style()
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(direction);
        return container;
    }
    
    static std::shared_ptr<Spacer> spacer(float size = 0) {
        return create_widget<Spacer>(size);
    }
};

} // namespace voxel_canvas