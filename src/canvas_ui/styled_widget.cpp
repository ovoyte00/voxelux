/*
 * Copyright (C) 2024 Voxelux
 * 
 * Styled Widget Implementation
 */

#include "canvas_ui/styled_widget.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/scaled_theme.h"
#include "canvas_ui/font_system.h"
#include <typeinfo>
#include "canvas_ui/render_block.h"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <chrono>

namespace voxel_canvas {

// StyledWidget implementation

StyledWidget::StyledWidget() : id_("widget") {}

StyledWidget::StyledWidget(const std::string& id) : id_(id) {}

void StyledWidget::set_style(const WidgetStyle& style) {
    base_style_ = style;
    invalidate_style();
    invalidate_layout();
}

void StyledWidget::invalidate_layout() {
    needs_layout_ = true;
    
    // Check if we're in a render block's build phase
    RenderBlock* current = RenderBlock::get_current();
    if (!current || !current->is_building()) {
        // Not in batch mode, mark dirty immediately
        mark_dirty();
    }
    // Otherwise, defer the dirty marking until block resolution
}

void StyledWidget::invalidate_style() {
    needs_style_computation_ = true;
    
    // Check if we're in a render block's build phase
    RenderBlock* current = RenderBlock::get_current();
    if (!current || !current->is_building()) {
        // Not in batch mode, mark dirty immediately
        mark_dirty();
    }
    // Otherwise, defer the dirty marking until block resolution
}

void StyledWidget::set_style_json(const std::string& json) {
    // TODO: Implement JSON parsing
    // For now, this is a placeholder
    invalidate_style();
    invalidate_layout();
}

void StyledWidget::set_style_property(const std::string& property, const std::string& value) {
    // TODO: Implement property setting
    invalidate_style();
    invalidate_layout();
}

void StyledWidget::add_class(const std::string& class_name) {
    if (std::find(style_classes_.begin(), style_classes_.end(), class_name) == style_classes_.end()) {
        style_classes_.push_back(class_name);
        invalidate_style();
    }
}

void StyledWidget::remove_class(const std::string& class_name) {
    auto it = std::find(style_classes_.begin(), style_classes_.end(), class_name);
    if (it != style_classes_.end()) {
        style_classes_.erase(it);
        invalidate_style();
    }
}

bool StyledWidget::has_class(const std::string& class_name) const {
    return std::find(style_classes_.begin(), style_classes_.end(), class_name) != style_classes_.end();
}

void StyledWidget::set_state(WidgetState state, bool enabled) {
    uint32_t flag = static_cast<uint32_t>(state);
    uint32_t old_flags = state_flags_;
    
    if (enabled) {
        state_flags_ |= flag;
    } else {
        state_flags_ &= ~flag;
    }
    
    if (old_flags != state_flags_) {
        apply_state_style();
        invalidate_style();
    }
}

bool StyledWidget::has_state(WidgetState state) const {
    return (state_flags_ & static_cast<uint32_t>(state)) != 0;
}

void StyledWidget::add_child(std::shared_ptr<StyledWidget> child) {
    if (child) {
        // Debug: Check for duplicate children and self-reference
        if (child.get() == this) {
            std::cout << "ERROR: Widget trying to add itself as child! " << get_widget_type() 
                      << " " << id_ << std::endl;
            return;
        }
        
        for (const auto& existing : children_) {
            if (existing == child) {
                std::cout << "WARNING: Attempting to add duplicate child to " << get_widget_type() 
                          << " " << id_ << std::endl;
                return;  // Don't add duplicates
            }
        }
        
        // Log what's being added
        if (get_widget_type() == "voxelux-layout" || child->get_widget_type() == "voxelux-layout") {
            std::cout << "Adding " << child->get_widget_type() << " " << child->get_id() 
                      << " to " << get_widget_type() << " " << id_ << std::endl;
        }
        
        children_.push_back(child);
        child->set_parent(this);
        invalidate_layout();
        // Don't perform layout immediately - wait for render or explicit request
    }
}

void StyledWidget::remove_child(std::shared_ptr<StyledWidget> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->set_parent(nullptr);
        children_.erase(it);
        invalidate_layout();
    }
}

void StyledWidget::remove_all_children() {
    for (auto& child : children_) {
        child->set_parent(nullptr);
    }
    children_.clear();
    invalidate_layout();
}

std::shared_ptr<StyledWidget> StyledWidget::find_child(const std::string& id) const {
    for (const auto& child : children_) {
        if (child->get_id() == id) {
            return child;
        }
        // Recursive search
        auto found = child->find_child(id);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

void StyledWidget::set_bounds(const Rect2D& bounds) {
    // Check if bounds actually changed
    if (bounds_.x != bounds.x || bounds_.y != bounds.y || 
        bounds_.width != bounds.width || bounds_.height != bounds.height) {
        // Mark old bounds as dirty
        mark_dirty();
        
        bounds_ = bounds;
        
        // Mark new bounds as dirty  
        mark_dirty();
        
        invalidate_layout();
    }
}

void StyledWidget::mark_dirty() {
    // Mark this widget's bounds as needing redraw
    // TODO: Need to get access to the renderer to mark dirty regions
    // For now, this is a placeholder - in a real implementation,
    // we'd propagate this up to the window/renderer
    
}

Rect2D StyledWidget::get_content_bounds() const {
    // CSS collapsing behavior: if width/height is 0, ignore padding/border in that dimension
    float left_offset = 0, top_offset = 0;
    float width_reduction = 0, height_reduction = 0;
    
    if (bounds_.width > 0) {
        // Width is non-zero, apply horizontal padding/border
        left_offset = computed_style_.padding_left + computed_style_.border_width_left;
        width_reduction = computed_style_.padding_left + computed_style_.padding_right + 
                         computed_style_.border_width_left + computed_style_.border_width_right;
    }
    
    if (bounds_.height > 0) {
        // Height is non-zero, apply vertical padding/border  
        top_offset = computed_style_.padding_top + computed_style_.border_width_top;
        height_reduction = computed_style_.padding_top + computed_style_.padding_bottom +
                          computed_style_.border_width_top + computed_style_.border_width_bottom;
    }
    
    Rect2D content_bounds = Rect2D(
        bounds_.x + left_offset,
        bounds_.y + top_offset,
        std::max(0.0f, bounds_.width - width_reduction),
        std::max(0.0f, bounds_.height - height_reduction)
    );
    
    return content_bounds;
}

Rect2D StyledWidget::get_padding_bounds() const {
    return Rect2D(
        bounds_.x + computed_style_.border_width_left,
        bounds_.y + computed_style_.border_width_top,
        bounds_.width - computed_style_.border_width_left - computed_style_.border_width_right,
        bounds_.height - computed_style_.border_width_top - computed_style_.border_width_bottom
    );
}

// Calculate intrinsic sizes (Pass 1 - bottom-up)
StyledWidget::IntrinsicSizes StyledWidget::calculate_intrinsic_sizes() {
    IntrinsicSizes sizes;
    
    // Start with style-defined sizes if available
    if (computed_style_.width > 0) {
        sizes.min_width = sizes.preferred_width = sizes.max_width = computed_style_.width;
    }
    if (computed_style_.min_width > 0) {
        sizes.min_width = computed_style_.min_width;
    }
    if (computed_style_.max_width < std::numeric_limits<float>::max()) {
        sizes.max_width = computed_style_.max_width;
    }
    
    if (computed_style_.height > 0) {
        sizes.min_height = sizes.preferred_height = sizes.max_height = computed_style_.height;
    }
    if (computed_style_.min_height > 0) {
        sizes.min_height = computed_style_.min_height;
    }
    if (computed_style_.max_height < std::numeric_limits<float>::max()) {
        sizes.max_height = computed_style_.max_height;
    }
    
    // If no explicit size, calculate based on children
    if (sizes.preferred_width == 0 || sizes.preferred_height == 0) {
        // Get children's intrinsic sizes first
        std::vector<IntrinsicSizes> child_sizes;
        for (const auto& child : children_) {
            if (!child->is_visible()) continue;
            child_sizes.push_back(child->calculate_intrinsic_sizes());
        }
        
        // Aggregate based on layout type
        if (computed_style_.display == WidgetStyle::Display::Flex) {
            bool is_row = (computed_style_.flex_direction == WidgetStyle::FlexDirection::Row ||
                          computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse);
            
            float total_width = 0, total_height = 0;
            float max_width = 0, max_height = 0;
            
            for (const auto& child_size : child_sizes) {
                if (is_row) {
                    total_width += child_size.preferred_width;
                    max_height = std::max(max_height, child_size.preferred_height);
                } else {
                    total_height += child_size.preferred_height;
                    max_width = std::max(max_width, child_size.preferred_width);
                }
            }
            
            // Add gaps
            if (child_sizes.size() > 1) {
                float gap = computed_style_.gap_pixels;
                if (is_row) {
                    total_width += gap * (child_sizes.size() - 1);
                } else {
                    total_height += gap * (child_sizes.size() - 1);
                }
            }
            
            if (sizes.preferred_width == 0) {
                sizes.preferred_width = is_row ? total_width : max_width;
            }
            if (sizes.preferred_height == 0) {
                sizes.preferred_height = is_row ? max_height : total_height;
            }
        } else {
            // Block/other: use maximum dimensions
            for (const auto& child_size : child_sizes) {
                sizes.preferred_width = std::max(sizes.preferred_width, child_size.preferred_width);
                sizes.preferred_height = std::max(sizes.preferred_height, child_size.preferred_height);
            }
        }
    }
    
    // Add padding
    sizes.min_width += computed_style_.padding_left + computed_style_.padding_right;
    sizes.preferred_width += computed_style_.padding_left + computed_style_.padding_right;
    sizes.min_height += computed_style_.padding_top + computed_style_.padding_bottom;
    sizes.preferred_height += computed_style_.padding_top + computed_style_.padding_bottom;
    
    // Add border
    float border_width = computed_style_.border_width_left + computed_style_.border_width_right;
    float border_height = computed_style_.border_width_top + computed_style_.border_width_bottom;
    sizes.min_width += border_width;
    sizes.preferred_width += border_width;
    sizes.min_height += border_height;
    sizes.preferred_height += border_height;
    
    // Add margins (they affect the space this widget takes in its parent)
    float margin_width = computed_style_.margin_left + computed_style_.margin_right;
    float margin_height = computed_style_.margin_top + computed_style_.margin_bottom;
    sizes.min_width += margin_width;
    sizes.preferred_width += margin_width;
    sizes.min_height += margin_height;
    sizes.preferred_height += margin_height;
    
    return sizes;
}

// Resolve actual sizes (Pass 2 - top-down)
void StyledWidget::resolve_sizes(float available_width, float available_height) {
    // Use intrinsic sizes as a starting point
    IntrinsicSizes intrinsic = calculate_intrinsic_sizes();
    
    // Resolve width
    float resolved_width = bounds_.width;
    if (computed_style_.width_unit == WidgetStyle::ComputedStyle::AUTO) {
        resolved_width = intrinsic.preferred_width;
    } else if (computed_style_.width_unit == WidgetStyle::ComputedStyle::PERCENT) {
        resolved_width = available_width * (computed_style_.width / 100.0f);
    } else if (computed_style_.width_unit == WidgetStyle::ComputedStyle::PIXELS && bounds_.width == 0) {
        // Only use computed width if bounds haven't been set by parent
        resolved_width = computed_style_.width;
    }
    
    // Apply min/max constraints
    resolved_width = std::max(intrinsic.min_width, std::min(intrinsic.max_width, resolved_width));
    
    // Resolve height
    float resolved_height = bounds_.height;
    if (computed_style_.height_unit == WidgetStyle::ComputedStyle::AUTO) {
        resolved_height = intrinsic.preferred_height;
    } else if (computed_style_.height_unit == WidgetStyle::ComputedStyle::PERCENT) {
        resolved_height = available_height * (computed_style_.height / 100.0f);
    } else if (computed_style_.height_unit == WidgetStyle::ComputedStyle::PIXELS && bounds_.height == 0) {
        // Only use computed height if bounds haven't been set by parent
        resolved_height = computed_style_.height;
    }
    
    // Apply min/max constraints
    resolved_height = std::max(intrinsic.min_height, std::min(intrinsic.max_height, resolved_height));
    
    // Update bounds with resolved sizes (preserve position!)
    // Only update size, not position
    bounds_.width = resolved_width;
    bounds_.height = resolved_height;
}

// Position children (Pass 3)
void StyledWidget::position_children() {
    // This calls the appropriate layout algorithm
    layout_children();
}

Point2D StyledWidget::get_content_size() const {
    // Default: calculate based on children
    if (children_.empty()) {
        return content_size_;
    }
    
    float max_x = 0, max_y = 0;
    for (const auto& child : children_) {
        if (!child->is_visible()) continue;
        
        const auto& child_bounds = child->get_bounds();
        max_x = std::max(max_x, child_bounds.x + child_bounds.width);
        max_y = std::max(max_y, child_bounds.y + child_bounds.height);
    }
    
    // Debug for dock containers
    if (get_widget_type() == "dock-container" && max_x == 0 && max_y == 0 && !children_.empty()) {
        std::cout << "WARNING: DockContainer get_content_size returning 0,0 with " 
                  << children_.size() << " children" << std::endl;
        for (const auto& child : children_) {
            if (child->is_visible()) {
                const auto& bounds = child->get_bounds();
                std::cout << "  Child " << child->get_widget_type() << " bounds: " 
                         << bounds.x << "," << bounds.y << " " 
                         << bounds.width << "x" << bounds.height << std::endl;
            }
        }
    }
    
    return Point2D(max_x, max_y);
}

void StyledWidget::render(CanvasRenderer* renderer) {
    if (!visible_) return;
    
    // Text rendering is now batched with UI rendering
    
    // Check visibility property
    if (computed_style_.visibility == WidgetStyle::Visibility::Hidden ||
        computed_style_.visibility == WidgetStyle::Visibility::Collapse) {
        // Hidden elements still take up space but don't render
        return;
    }
    
    // Ensure style is computed first
    if (needs_style_computation_) {
        compute_style(renderer->get_scaled_theme());
    }
    
    // Then ensure layout is up to date (this happens automatically now)
    if (needs_layout_) {
        perform_layout();
    }
    
    // Apply opacity
    float saved_opacity = renderer->get_global_opacity();
    renderer->set_global_opacity(saved_opacity * computed_style_.opacity);
    
    // Apply transform if present
    bool has_transform = (computed_style_.transform.translate_x != 0 ||
                         computed_style_.transform.translate_y != 0 ||
                         computed_style_.transform.scale_x != 1 ||
                         computed_style_.transform.scale_y != 1 ||
                         computed_style_.transform.rotate != 0);
    
    if (has_transform) {
        renderer->push_transform();
        
        // Calculate transform origin
        float origin_x = bounds_.x + bounds_.width * computed_style_.transform_origin.x;
        float origin_y = bounds_.y + bounds_.height * computed_style_.transform_origin.y;
        
        // Apply transform around origin
        renderer->translate(origin_x, origin_y);
        renderer->translate(computed_style_.transform.translate_x, computed_style_.transform.translate_y);
        renderer->rotate(computed_style_.transform.rotate);
        renderer->scale(computed_style_.transform.scale_x, computed_style_.transform.scale_y);
        renderer->translate(-origin_x, -origin_y);
    }
    
    // Apply clipping for overflow
    bool needs_clip = (computed_style_.overflow_x == WidgetStyle::Overflow::Hidden ||
                      computed_style_.overflow_y == WidgetStyle::Overflow::Hidden ||
                      computed_style_.overflow_x == WidgetStyle::Overflow::Scroll ||
                      computed_style_.overflow_y == WidgetStyle::Overflow::Scroll);
    
    if (needs_clip) {
        renderer->push_clip(get_content_bounds());
    }
    
    // Render layers in order
    render_effects(renderer);    // Shadows
    render_background(renderer); // Background
    render_border(renderer);     // Border
    render_outline(renderer);    // Outline (new)
    render_content(renderer);    // Widget-specific content
    render_children(renderer);   // Child widgets
    
    // Restore state
    if (needs_clip) {
        renderer->pop_clip();
    }
    
    if (has_transform) {
        renderer->pop_transform();
    }
    
    // Restore opacity
    renderer->set_global_opacity(saved_opacity);
}

void StyledWidget::render_background(CanvasRenderer* renderer) {
    Rect2D bg_bounds = get_padding_bounds();
    
    // SINGLE-PASS RENDERING: Background, border, and outline together
    
    // Collect all rendering properties
    float border_width = std::max({computed_style_.border_width_top, computed_style_.border_width_right,
                                   computed_style_.border_width_bottom, computed_style_.border_width_left});
    ColorRGBA border_color = computed_style_.border_color;
    
    // Outline properties
    float outline_width = 0.0f;
    float outline_offset = 0.0f;
    ColorRGBA outline_color(0, 0, 0, 0);
    
    if (computed_style_.outline.style != WidgetStyle::OutlineStyle::None && 
        computed_style_.outline.width.value > 0) {
        ScaledTheme theme = renderer->get_scaled_theme();
        outline_width = computed_style_.outline.width.resolve(theme);
        
        // Calculate offset based on alignment
        float base_offset = computed_style_.outline.offset.resolve(theme);
        
        // Adjust offset based on alignment
        // The offset tells the shader where to position the outline relative to the border
        switch (computed_style_.outline.alignment) {
            case WidgetStyle::OutlineStyle::Inside:
                // Outline is inside the widget boundary
                // Positive offset moves inward (toward fill)
                outline_offset = base_offset + border_width;
                break;
            case WidgetStyle::OutlineStyle::Center:
                // Outline is centered on the widget boundary
                outline_offset = base_offset + border_width * 0.5f - outline_width * 0.5f;
                break;
            case WidgetStyle::OutlineStyle::Outside:
            default:
                // Outline is outside the widget boundary
                // Negative offset moves outward
                outline_offset = base_offset - outline_width;
                break;
        }
        
        outline_color = computed_style_.outline.color.resolve(theme);
    }
    
    
    // Only render if we have something to draw
    bool should_render = computed_style_.background_color.a > 0.001f || border_width > 0.001f || outline_width > 0.001f;
    
    
    if (should_render) {
        // Get the actual background color
        ColorRGBA bg_color = computed_style_.background_color;
        
        // Use the highest corner radius value for now (proper varied radius support coming)
        float corner_radius = std::max({computed_style_.border_radius_tl, computed_style_.border_radius_tr,
                                        computed_style_.border_radius_br, computed_style_.border_radius_bl});
        
        // SINGLE draw call for background, border, AND outline!
        renderer->add_widget_instance(bg_bounds, bg_color, corner_radius, border_width, border_color,
                                     outline_width, outline_offset, outline_color);
    }
    
    // 2. Background images (if any)
    if (!computed_style_.background_images.empty()) {
        for (const auto& bg_image : computed_style_.background_images) {
            if (bg_image.texture_id != 0) {
                // Calculate image position and size based on properties
                CanvasRenderer::ObjectFit fit = CanvasRenderer::ObjectFit::Fill;
                switch (bg_image.size) {
                    case WidgetStyle::BackgroundImage::Cover:
                        fit = CanvasRenderer::ObjectFit::Cover;
                        break;
                    case WidgetStyle::BackgroundImage::Contain:
                        fit = CanvasRenderer::ObjectFit::Contain;
                        break;
                    case WidgetStyle::BackgroundImage::Auto:
                        fit = CanvasRenderer::ObjectFit::None;
                        break;
                    default:
                        fit = CanvasRenderer::ObjectFit::Fill;
                        break;
                }
                
                // TODO: Handle repeat, position, and attachment properties
                renderer->draw_image(bg_image.texture_id, bg_bounds, fit);
            }
        }
    } else if (computed_style_.background_image.is_set()) {
        // Single background image
        if (computed_style_.background_image.texture_id != 0) {
            CanvasRenderer::ObjectFit fit = CanvasRenderer::ObjectFit::Fill;
            switch (computed_style_.background_image.size) {
                case WidgetStyle::BackgroundImage::Cover:
                    fit = CanvasRenderer::ObjectFit::Cover;
                    break;
                case WidgetStyle::BackgroundImage::Contain:
                    fit = CanvasRenderer::ObjectFit::Contain;
                    break;
                case WidgetStyle::BackgroundImage::Auto:
                    fit = CanvasRenderer::ObjectFit::None;
                    break;
                default:
                    fit = CanvasRenderer::ObjectFit::Fill;
                    break;
            }
            
            renderer->draw_image(computed_style_.background_image.texture_id, bg_bounds, fit);
        }
    }
    
    // 3. Background gradients (on top of images)
    if (!computed_style_.background_gradients.empty()) {
        for (const auto& gradient : computed_style_.background_gradients) {
            render_gradient(renderer, bg_bounds, gradient);
        }
    } else if (computed_style_.background_gradient.is_set()) {
        // Single gradient
        render_gradient(renderer, bg_bounds, computed_style_.background_gradient);
    }
}

void StyledWidget::render_gradient(CanvasRenderer* renderer, const Rect2D& bounds, const WidgetStyle::Gradient& gradient) {
    if (!gradient.is_set()) {
        return;
    }
    
    // Convert gradient stops to renderer format
    std::vector<std::pair<ColorRGBA, float>> stops;
    for (const auto& stop : gradient.stops) {
        ScaledTheme theme = get_scaled_theme(renderer);
        ColorRGBA color = stop.color.resolve(theme);
        stops.emplace_back(color, stop.position);
    }
    
    switch (gradient.type) {
        case WidgetStyle::Gradient::Linear:
            renderer->draw_linear_gradient(bounds, gradient.angle, stops);
            break;
            
        case WidgetStyle::Gradient::Radial: {
            // Calculate center position
            ScaledTheme theme = get_scaled_theme(renderer);
            float center_x = gradient.center_x.resolve(bounds.width, bounds.width / 2, theme);
            float center_y = gradient.center_y.resolve(bounds.height, bounds.height / 2, theme);
            Point2D center(bounds.x + center_x, bounds.y + center_y);
            
            // Calculate radii
            float radius_x = gradient.radius_x.resolve(bounds.width, bounds.width / 2, theme);
            float radius_y = gradient.radius_y.resolve(bounds.height, bounds.height / 2, theme);
            
            // Adjust radii based on size setting
            if (gradient.radial_size == WidgetStyle::Gradient::ClosestSide) {
                radius_x = std::min(center_x, bounds.width - center_x);
                radius_y = std::min(center_y, bounds.height - center_y);
                if (gradient.shape == WidgetStyle::Gradient::Circle) {
                    radius_x = radius_y = std::min(radius_x, radius_y);
                }
            } else if (gradient.radial_size == WidgetStyle::Gradient::FarthestSide) {
                radius_x = std::max(center_x, bounds.width - center_x);
                radius_y = std::max(center_y, bounds.height - center_y);
                if (gradient.shape == WidgetStyle::Gradient::Circle) {
                    radius_x = radius_y = std::max(radius_x, radius_y);
                }
            } else if (gradient.radial_size == WidgetStyle::Gradient::ClosestCorner) {
                float dx = std::min(center_x, bounds.width - center_x);
                float dy = std::min(center_y, bounds.height - center_y);
                float dist = std::sqrt(dx * dx + dy * dy);
                radius_x = radius_y = dist;
            } else if (gradient.radial_size == WidgetStyle::Gradient::FarthestCorner) {
                float dx = std::max(center_x, bounds.width - center_x);
                float dy = std::max(center_y, bounds.height - center_y);
                float dist = std::sqrt(dx * dx + dy * dy);
                radius_x = radius_y = dist;
            }
            
            renderer->draw_radial_gradient(bounds, center, radius_x, radius_y, stops);
            break;
        }
        
        case WidgetStyle::Gradient::Conic: {
            // Calculate center position
            ScaledTheme theme = get_scaled_theme(renderer);
            float center_x = gradient.center_x.resolve(bounds.width, bounds.width / 2, theme);
            float center_y = gradient.center_y.resolve(bounds.height, bounds.height / 2, theme);
            Point2D center(bounds.x + center_x, bounds.y + center_y);
            
            renderer->draw_conic_gradient(bounds, center, gradient.start_angle, stops);
            break;
        }
        
        default:
            break;
    }
}

void StyledWidget::render_border(CanvasRenderer* renderer) {
    // BORDER IS NOW RENDERED IN render_background() for single-pass efficiency
    // This prevents overlapping geometry and double blending issues
    return;
    
    if (computed_style_.border_width_top > 0 || computed_style_.border_width_right > 0 ||
        computed_style_.border_width_bottom > 0 || computed_style_.border_width_left > 0) {
        
        float border_width = std::max({computed_style_.border_width_top, computed_style_.border_width_right,
                                       computed_style_.border_width_bottom, computed_style_.border_width_left});
        
        if (border_width > 0) {
            // Check if we have a gradient border
            if (computed_style_.border_gradient.is_set()) {
                // Create a path for the border area
                Rect2D outer_bounds = bounds_;
                Rect2D inner_bounds = bounds_;
                inner_bounds.x += border_width;
                inner_bounds.y += border_width;
                inner_bounds.width -= border_width * 2;
                inner_bounds.height -= border_width * 2;
                
                // Render gradient in the border area
                // We'll render the gradient across the full bounds and then clip
                renderer->push_clip(outer_bounds);
                
                // Draw the gradient background
                render_gradient(renderer, outer_bounds, computed_style_.border_gradient);
                
                // Now cut out the inner area (simulate by drawing background color)
                if (computed_style_.background_color.a > 0) {
                    if (computed_style_.border_radius_tl > 0 || computed_style_.border_radius_tr > 0 ||
                        computed_style_.border_radius_bl > 0 || computed_style_.border_radius_br > 0) {
                        // Account for border radius in inner area
                        float inner_tl = std::max(0.0f, computed_style_.border_radius_tl - border_width);
                        float inner_tr = std::max(0.0f, computed_style_.border_radius_tr - border_width);
                        float inner_br = std::max(0.0f, computed_style_.border_radius_br - border_width);
                        float inner_bl = std::max(0.0f, computed_style_.border_radius_bl - border_width);
                        renderer->draw_rounded_rect_varied(inner_bounds, computed_style_.background_color,
                                                          inner_tl, inner_tr, inner_br, inner_bl);
                    } else {
                        renderer->draw_rect(inner_bounds, computed_style_.background_color);
                    }
                }
                
                renderer->pop_clip();
            } else if (computed_style_.border_color.a > 0) {
                // Solid color border - NOW HANDLED IN render_background() to avoid overlapping draws
                // renderer->draw_rect_outline(bounds_, computed_style_.border_color, border_width);
            }
        }
    }
}

void StyledWidget::render_content(CanvasRenderer* renderer) {
    // Base class has no content
    // Subclasses override this
}

void StyledWidget::render_children(CanvasRenderer* renderer) {
    // Debug output disabled
    if (!children_.empty()) {
    }
    
    for (auto& child : children_) {
        if (child->is_visible()) {
            child->render(renderer);
        }
    }
}

void StyledWidget::render_effects(CanvasRenderer* renderer) {
    // Render box shadow using the new draw_shadow method
    if (computed_style_.box_shadow.blur_radius > 0 || computed_style_.box_shadow.spread != 0) {
        // Resolve color from theme
        ColorRGBA shadow_color = computed_style_.box_shadow.color.resolve(
            get_scaled_theme(renderer));
        renderer->draw_shadow(bounds_, 
                            computed_style_.box_shadow.blur_radius,
                            computed_style_.box_shadow.spread,
                            computed_style_.box_shadow.offset_x,
                            computed_style_.box_shadow.offset_y,
                            shadow_color);
    }
}

void StyledWidget::render_outline(CanvasRenderer* renderer) {
    // OUTLINE IS NOW RENDERED IN render_background() for single-pass efficiency
    return;
    if (computed_style_.outline.style != WidgetStyle::OutlineStyle::None && 
        computed_style_.outline.width.value > 0) {
        
        // Resolve outline values
        float outline_width = computed_style_.outline.width.resolve(renderer->get_scaled_theme());
        float outline_offset = computed_style_.outline.offset.resolve(renderer->get_scaled_theme());
        
        // Calculate outline bounds based on alignment (Illustrator-style stroke alignment)
        Rect2D outline_bounds;
        Rect2D inner_bounds;
        
        switch (computed_style_.outline.alignment) {
            case WidgetStyle::OutlineStyle::Outside:
                // Stroke is entirely outside the boundary (CSS default)
                outline_bounds = Rect2D(
                    bounds_.x - outline_offset - outline_width,
                    bounds_.y - outline_offset - outline_width,
                    bounds_.width + (outline_offset + outline_width) * 2,
                    bounds_.height + (outline_offset + outline_width) * 2
                );
                inner_bounds = Rect2D(
                    bounds_.x - outline_offset,
                    bounds_.y - outline_offset,
                    bounds_.width + outline_offset * 2,
                    bounds_.height + outline_offset * 2
                );
                break;
                
            case WidgetStyle::OutlineStyle::Center:
                // Stroke is centered on the boundary (Illustrator default)
                // Half the stroke is inside, half is outside
                {
                    float half_width = outline_width / 2.0f;
                    outline_bounds = Rect2D(
                        bounds_.x - outline_offset - half_width,
                        bounds_.y - outline_offset - half_width,
                        bounds_.width + (outline_offset + half_width) * 2,
                        bounds_.height + (outline_offset + half_width) * 2
                    );
                    inner_bounds = Rect2D(
                        bounds_.x - outline_offset + half_width,
                        bounds_.y - outline_offset + half_width,
                        bounds_.width + outline_offset * 2 - half_width * 2,
                        bounds_.height + outline_offset * 2 - half_width * 2
                    );
                }
                break;
                
            case WidgetStyle::OutlineStyle::Inside:
                // Stroke is entirely inside the boundary
                outline_bounds = Rect2D(
                    bounds_.x + outline_offset,
                    bounds_.y + outline_offset,
                    bounds_.width - outline_offset * 2,
                    bounds_.height - outline_offset * 2
                );
                inner_bounds = Rect2D(
                    bounds_.x + outline_offset + outline_width,
                    bounds_.y + outline_offset + outline_width,
                    bounds_.width - (outline_offset + outline_width) * 2,
                    bounds_.height - (outline_offset + outline_width) * 2
                );
                break;
        }
        
        // Check if we have a gradient outline
        if (computed_style_.outline_gradient.is_set()) {
            // Render gradient in the outline area
            renderer->push_clip(outline_bounds);
            
            // Draw the gradient background
            render_gradient(renderer, outline_bounds, computed_style_.outline_gradient);
            
            // Cut out the inner area based on alignment
            // For inside alignment, we need to be careful not to overwrite content
            if (computed_style_.outline.alignment != WidgetStyle::OutlineStyle::Inside) {
                // For outside and center, we can safely cut out with transparent
                ColorRGBA transparent(0, 0, 0, 0);
                
                // Check if we need rounded corners for the cutout
                if (computed_style_.border_radius_tl > 0 || computed_style_.border_radius_tr > 0 ||
                    computed_style_.border_radius_bl > 0 || computed_style_.border_radius_br > 0) {
                    // Adjust radius for inner bounds
                    float radius_adjust = (computed_style_.outline.alignment == WidgetStyle::OutlineStyle::Center) 
                                        ? outline_width / 2.0f : outline_width;
                    float inner_tl = std::max(0.0f, computed_style_.border_radius_tl - radius_adjust);
                    float inner_tr = std::max(0.0f, computed_style_.border_radius_tr - radius_adjust);
                    float inner_br = std::max(0.0f, computed_style_.border_radius_br - radius_adjust);
                    float inner_bl = std::max(0.0f, computed_style_.border_radius_bl - radius_adjust);
                    
                    renderer->draw_rounded_rect_varied(inner_bounds, transparent,
                                                      inner_tl, inner_tr, inner_br, inner_bl);
                } else {
                    renderer->draw_rect(inner_bounds, transparent);
                }
            }
            
            renderer->pop_clip();
        } else {
            // Solid color outline
            ColorRGBA outline_color = computed_style_.outline.color.resolve(renderer->get_scaled_theme());
            
            // For center alignment with solid color, we can optimize by drawing a single outline
            // at the correct position rather than filling and cutting
            if (computed_style_.outline.alignment == WidgetStyle::OutlineStyle::Center) {
                // Draw centered outline
                Rect2D center_bounds(
                    bounds_.x - outline_offset,
                    bounds_.y - outline_offset,
                    bounds_.width + outline_offset * 2,
                    bounds_.height + outline_offset * 2
                );
                
                if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Solid) {
                    renderer->draw_rect_outline(center_bounds, outline_color, outline_width);
                } else if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Dashed) {
                    Point2D tl(center_bounds.x, center_bounds.y);
                    Point2D tr(center_bounds.x + center_bounds.width, center_bounds.y);
                    Point2D br(center_bounds.x + center_bounds.width, center_bounds.y + center_bounds.height);
                    Point2D bl(center_bounds.x, center_bounds.y + center_bounds.height);
                    
                    renderer->draw_dashed_line(tl, tr, outline_color, outline_width);
                    renderer->draw_dashed_line(tr, br, outline_color, outline_width);
                    renderer->draw_dashed_line(br, bl, outline_color, outline_width);
                    renderer->draw_dashed_line(bl, tl, outline_color, outline_width);
                } else if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Dotted) {
                    Point2D tl(center_bounds.x, center_bounds.y);
                    Point2D tr(center_bounds.x + center_bounds.width, center_bounds.y);
                    Point2D br(center_bounds.x + center_bounds.width, center_bounds.y + center_bounds.height);
                    Point2D bl(center_bounds.x, center_bounds.y + center_bounds.height);
                    
                    renderer->draw_dotted_line(tl, tr, outline_color, outline_width);
                    renderer->draw_dotted_line(tr, br, outline_color, outline_width);
                    renderer->draw_dotted_line(br, bl, outline_color, outline_width);
                    renderer->draw_dotted_line(bl, tl, outline_color, outline_width);
                }
            } else {
                // For inside/outside alignment, we need to fill and cut
                // This approach works for all styles
                if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Solid) {
                    // Fill the outline area
                    renderer->draw_rect(outline_bounds, outline_color);
                    // Cut out the inner area
                    if (computed_style_.outline.alignment != WidgetStyle::OutlineStyle::Inside) {
                        renderer->draw_rect(inner_bounds, ColorRGBA(0, 0, 0, 0));
                    }
                } else {
                    // For dashed/dotted, draw lines at the appropriate position
                    Rect2D line_bounds = (computed_style_.outline.alignment == WidgetStyle::OutlineStyle::Inside)
                                       ? Rect2D(bounds_.x + outline_offset + outline_width/2,
                                               bounds_.y + outline_offset + outline_width/2,
                                               bounds_.width - (outline_offset + outline_width/2) * 2,
                                               bounds_.height - (outline_offset + outline_width/2) * 2)
                                       : Rect2D(outline_bounds.x + outline_width/2,
                                               outline_bounds.y + outline_width/2,
                                               outline_bounds.width - outline_width,
                                               outline_bounds.height - outline_width);
                    
                    Point2D tl(line_bounds.x, line_bounds.y);
                    Point2D tr(line_bounds.x + line_bounds.width, line_bounds.y);
                    Point2D br(line_bounds.x + line_bounds.width, line_bounds.y + line_bounds.height);
                    Point2D bl(line_bounds.x, line_bounds.y + line_bounds.height);
                    
                    if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Dashed) {
                        renderer->draw_dashed_line(tl, tr, outline_color, outline_width);
                        renderer->draw_dashed_line(tr, br, outline_color, outline_width);
                        renderer->draw_dashed_line(br, bl, outline_color, outline_width);
                        renderer->draw_dashed_line(bl, tl, outline_color, outline_width);
                    } else if (computed_style_.outline.style == WidgetStyle::OutlineStyle::Dotted) {
                        renderer->draw_dotted_line(tl, tr, outline_color, outline_width);
                        renderer->draw_dotted_line(tr, br, outline_color, outline_width);
                        renderer->draw_dotted_line(br, bl, outline_color, outline_width);
                        renderer->draw_dotted_line(bl, tl, outline_color, outline_width);
                    }
                }
            }
        }
    }
}

bool StyledWidget::handle_event(const InputEvent& event) {
    if (!visible_ || is_disabled()) {
        return false;
    }
    
    // Check if event is within bounds
    bool in_bounds = bounds_.contains(event.mouse_pos);
    
    
    // Update hover state - ALWAYS update, not just when state changes
    // This ensures widgets lose hover when mouse moves away quickly
    if (event.type == EventType::MOUSE_MOVE) {
        bool was_hovered = is_hovered();
        
        
        // If mouse is not in bounds, always clear hover
        if (!in_bounds && is_hovered()) {
            set_hovered(false);
            apply_state_style();
            invalidate_style();
        }
        // If mouse is in bounds, set hover
        else if (in_bounds && !is_hovered()) {
            set_hovered(true);
            apply_state_style();
            invalidate_style();
            if (on_hover_) {
                on_hover_(this);
            }
        }
    }
    
    // Handle click
    if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
        if (in_bounds) {
            set_active(true);
            set_focused(true);
            if (on_click_) {
                on_click_(this);
            }
            return true;
        } else {
            set_focused(false);
        }
    }
    
    if (event.type == EventType::MOUSE_RELEASE && event.mouse_button == MouseButton::LEFT) {
        set_active(false);
    }
    
    // Pass to children (in reverse order for proper stacking)
    // For MOUSE_MOVE, always propagate to all children so they can update hover states
    if (event.type == EventType::MOUSE_MOVE) {
        bool any_handled = false;
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->handle_event(event)) {
                any_handled = true;
                // Don't return early - let all children process mouse move
            }
        }
        return any_handled || in_bounds;
    } else {
        // For other events, stop at first child that handles it
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->handle_event(event)) {
                return true;
            }
        }
    }
    
    return in_bounds;  // Consume event if in bounds
}

void StyledWidget::perform_layout() {
    // First, ensure our style is computed
    if (needs_style_computation_) {
        // Need access to theme - this should be passed down
        // For now, we'll require compute_style to be called before layout
        return;
    }
    
    // Debug: Check computed values for menu buttons
    if (get_widget_type() == "button" && parent_ && parent_->get_widget_type() == "menubar") {
        std::cout << "MenuButton layout: computed height=" << computed_style_.height 
                  << " unit=" << computed_style_.height_unit
                  << " current bounds=" << bounds_.width << "x" << bounds_.height << std::endl;
    }
    
    // Multi-pass layout system:
    // Pass 1: Calculate intrinsic sizes (bottom-up)
    IntrinsicSizes intrinsic = calculate_intrinsic_sizes();
    
    // Pass 2: Resolve sizes based on constraints
    // If we're the root or have explicit sizes, use those
    // Otherwise use parent's content bounds
    float available_width = bounds_.width;
    float available_height = bounds_.height;
    
    if (parent_) {
        Rect2D parent_content = parent_->get_content_bounds();
        if (computed_style_.width_unit == WidgetStyle::ComputedStyle::AUTO) {
            available_width = parent_content.width;
        }
        if (computed_style_.height_unit == WidgetStyle::ComputedStyle::AUTO) {
            available_height = parent_content.height;
        }
    }
    
    resolve_sizes(available_width, available_height);
    
    // Pass 3: Position children
    position_children();
    
    // Recursively layout children with multi-pass
    for (auto& child : children_) {
        child->perform_layout();
    }
    
    needs_layout_ = false;
}

void StyledWidget::request_layout() {
    if (needs_layout_) {
        perform_layout();
    }
}

void StyledWidget::layout_children() {
    // Apply CSS-compliant layout based on display type
    // Display types now match CSS behavior:
    // - Block: Full width by default, stacks vertically
    // - Inline: Content width, ignores width/height settings, only horizontal margins
    // - InlineBlock: Content width, respects width/height, all margins work
    // - Flex: Creates flex container, children become flex items
    // - Grid: Grid layout (TODO)
    // - None: Not rendered or laid out
    
    if (children_.empty()) return;
    
    switch (computed_style_.display) {
        case WidgetStyle::Display::Flex:
            layout_flex();
            break;
        case WidgetStyle::Display::Block:
            layout_block();
            break;
        case WidgetStyle::Display::Inline:
        case WidgetStyle::Display::InlineBlock:
            layout_inline();  // Handles both inline and inline-block with proper distinctions
            break;
        case WidgetStyle::Display::Grid:
            layout_grid();
            break;
        case WidgetStyle::Display::None:
            // Don't layout children of display:none elements
            break;
        default:
            break;
    }
}

void StyledWidget::layout_block() {
    // Block layout - stack children vertically (CSS-compliant)
    Rect2D content_bounds = get_content_bounds();
    float y = content_bounds.y;
    
    for (auto& child : children_) {
        if (!child->is_visible()) continue;
        
        // Skip elements with display: none
        const auto& child_style = child->get_computed_style();
        if (child_style.display == WidgetStyle::Display::None) continue;
        
        // Margin collapse handling (simplified - full CSS margin collapse is complex)
        y += child_style.margin_top;
        
        // Calculate child width based on display type
        float child_width;
        float child_x;
        
        // Check for margin auto (for horizontal centering)
        // Get the raw margin BoxSpacing from the style (before computation)
        const auto& raw_margin = child->get_style().margin;
        bool margin_left_auto = raw_margin.is_left_auto();
        bool margin_right_auto = raw_margin.is_right_auto();
        float actual_margin_left = margin_left_auto ? 0 : child_style.margin_left;
        float actual_margin_right = margin_right_auto ? 0 : child_style.margin_right;
        
        // Block elements take full available width by default
        if (child_style.display == WidgetStyle::Display::Block || 
            child_style.display == WidgetStyle::Display::Flex ||
            child_style.display == WidgetStyle::Display::Grid) {
            // If width is explicitly set, use it; otherwise use available width
            if (child_style.width > 0) {
                child_width = child_style.width;
                
                // Handle margin auto for centering
                if (margin_left_auto && margin_right_auto) {
                    // Center horizontally
                    float available = content_bounds.width - child_width;
                    actual_margin_left = available / 2;
                    actual_margin_right = available / 2;
                } else if (margin_left_auto) {
                    // Push to right
                    actual_margin_left = content_bounds.width - child_width - actual_margin_right;
                } else if (margin_right_auto) {
                    // Push to left (default behavior)
                    actual_margin_right = content_bounds.width - child_width - actual_margin_left;
                }
            } else {
                // Full available width minus margins
                child_width = content_bounds.width - actual_margin_left - actual_margin_right;
            }
        } else {
            // Inline, inline-block elements use their natural width
            Point2D content_size = child->get_content_size();
            child_width = child_style.width > 0 ? child_style.width : 
                         (content_size.x + child_style.padding_left + child_style.padding_right +
                          child_style.border_width_left + child_style.border_width_right);
                          
            // Inline-block can also use margin auto
            if (child_style.display == WidgetStyle::Display::InlineBlock) {
                if (margin_left_auto && margin_right_auto) {
                    float available = content_bounds.width - child_width;
                    actual_margin_left = available / 2;
                    actual_margin_right = available / 2;
                }
            }
        }
        
        child_x = content_bounds.x + actual_margin_left;
        
        // Calculate height
        float child_height;
        if (child_style.height > 0) {
            child_height = child_style.height;
        } else {
            // Use intrinsic content height
            Point2D content_size = child->get_content_size();
            child_height = content_size.y + child_style.padding_top + child_style.padding_bottom +
                          child_style.border_width_top + child_style.border_width_bottom;
        }
        
        child->set_bounds(Rect2D(child_x, y, child_width, child_height));
        
        y += child_height + child_style.margin_bottom;
    }
}

void StyledWidget::layout_inline() {
    // CSS-compliant inline/inline-block layout with text-align and vertical-align
    Rect2D content_bounds = get_content_bounds();
    float x = content_bounds.x;
    float y = content_bounds.y;
    float line_height = 0;
    
    // Track items on current line for text-align
    struct LineItem {
        StyledWidget* widget;
        float width;
        float height;
        float margin_left;
        float margin_right;
        float margin_top;
        float margin_bottom;
    };
    std::vector<LineItem> current_line_items;
    float current_line_width = 0;
    
    auto apply_text_align = [&]() {
        if (current_line_items.empty()) return;
        
        // Calculate total line width
        float total_width = current_line_width;
        float available_width = content_bounds.width;
        float offset = 0;
        
        // Apply text-align to position the line
        switch (computed_style_.text_align) {
            case WidgetStyle::TextAlign::Center:
                offset = (available_width - total_width) / 2;
                break;
            case WidgetStyle::TextAlign::Right:
                offset = available_width - total_width;
                break;
            case WidgetStyle::TextAlign::Justify:
                // Justify: distribute extra space between items (not after last item)
                if (current_line_items.size() > 1) {
                    // Don't justify the last line or single-item lines
                    // In CSS, justify only applies to full lines, not the last line
                    // For simplicity, we'll always justify for now
                    float extra_space = available_width - total_width;
                    float space_between_items = extra_space / (current_line_items.size() - 1);
                    
                    // Position items with distributed space
                    float justify_x = content_bounds.x;
                    for (size_t j = 0; j < current_line_items.size(); ++j) {
                        const auto& item = current_line_items[j];
                        float item_y = y;
                        
                        // Apply vertical-align for inline-block elements
                        const auto& item_style = item.widget->get_computed_style();
                        if (item_style.display == WidgetStyle::Display::InlineBlock) {
                            switch (item_style.vertical_align) {
                                case WidgetStyle::VerticalAlign::Middle:
                                    item_y = y + (line_height - item.height) / 2;
                                    break;
                                case WidgetStyle::VerticalAlign::Bottom:
                                    item_y = y + line_height - item.height - item.margin_bottom;
                                    break;
                                case WidgetStyle::VerticalAlign::Top:
                                    item_y = y + item.margin_top;
                                    break;
                                default: // Baseline
                                    item_y = y + line_height - item.height - item.margin_bottom;
                                    break;
                            }
                        } else {
                            item_y = y + item.margin_top;
                        }
                        
                        item.widget->set_bounds(Rect2D(
                            justify_x + item.margin_left,
                            item_y,
                            item.width,
                            item.height
                        ));
                        
                        justify_x += item.margin_left + item.width + item.margin_right;
                        if (j < current_line_items.size() - 1) {
                            justify_x += space_between_items;
                        }
                    }
                    
                    // Skip normal positioning since we handled it
                    current_line_items.clear();
                    current_line_width = 0;
                    return;
                }
                // Fall through to left align for single items
                offset = 0;
                break;
            default: // Left
                offset = 0;
                break;
        }
        
        // Position items on the line with vertical-align
        float line_x = content_bounds.x + offset;
        for (const auto& item : current_line_items) {
            float item_y = y;
            
            // Apply vertical-align for inline-block elements
            const auto& item_style = item.widget->get_computed_style();
            if (item_style.display == WidgetStyle::Display::InlineBlock) {
                switch (item_style.vertical_align) {
                    case WidgetStyle::VerticalAlign::Middle:
                        item_y = y + (line_height - item.height) / 2;
                        break;
                    case WidgetStyle::VerticalAlign::Bottom:
                        item_y = y + line_height - item.height - item.margin_bottom;
                        break;
                    case WidgetStyle::VerticalAlign::Top:
                        item_y = y + item.margin_top;
                        break;
                    default: // Baseline
                        // Calculate baseline position
                        // For text widgets, get the font metrics
                        if (item_style.font_size_pixels > 0 && g_font_system) {
                            // Get font ascender (height above baseline)
                            FontFace* font = g_font_system->get_font(
                                item_style.font_family.empty() ? "default" : item_style.font_family
                            );
                            float ascender = font ? font->get_ascender(item_style.font_size_pixels) : 0;
                            
                            // Position so text baseline aligns with line baseline
                            // Line baseline is at: y + line_height - descender_space
                            // For now, approximate baseline at 80% of line height
                            float baseline_y = y + line_height * 0.8f;
                            item_y = baseline_y - ascender;
                        } else {
                            // For non-text widgets, align bottom edge to baseline
                            item_y = y + line_height - item.height - item.margin_bottom;
                        }
                        break;
                }
            } else {
                // Inline elements sit on baseline
                item_y = y + item.margin_top;
            }
            
            item.widget->set_bounds(Rect2D(
                line_x + item.margin_left,
                item_y,
                item.width,
                item.height
            ));
            
            line_x += item.margin_left + item.width + item.margin_right;
        }
        
        // Clear for next line
        current_line_items.clear();
        current_line_width = 0;
    };
    
    for (auto& child : children_) {
        if (!child->is_visible()) continue;
        
        if (child->needs_style_computation_) {
            continue;
        }
        
        const auto& child_style = child->get_computed_style();
        if (child_style.display == WidgetStyle::Display::None) continue;
        
        // Get intrinsic content size
        Point2D content_size = child->get_content_size();
        
        float child_width, child_height;
        float margin_left = 0, margin_right = 0;
        float margin_top = 0, margin_bottom = 0;
        
        if (child_style.display == WidgetStyle::Display::Inline) {
            // Pure inline elements:
            // - Ignore explicit width/height settings
            // - Use only horizontal margins
            // - Vertical padding is visual only (doesn't affect layout)
            child_width = content_size.x + child_style.padding_left + child_style.padding_right;
            child_height = content_size.y;  // Just content height, padding doesn't affect line height
            
            // Only horizontal margins work for inline
            margin_left = child_style.margin_left;
            margin_right = child_style.margin_right;
            // Vertical margins are ignored for inline elements
            margin_top = 0;
            margin_bottom = 0;
            
        } else if (child_style.display == WidgetStyle::Display::InlineBlock) {
            // Inline-block elements:
            // - Respect explicit width/height if set
            // - All margins work
            // - All padding affects layout
            
            // Use explicit width if set, otherwise intrinsic
            if (child_style.width > 0) {
                child_width = child_style.width;
            } else {
                child_width = content_size.x + child_style.padding_left + child_style.padding_right +
                             child_style.border_width_left + child_style.border_width_right;
            }
            
            // Use explicit height if set, otherwise intrinsic
            if (child_style.height > 0) {
                child_height = child_style.height;
            } else {
                child_height = content_size.y + child_style.padding_top + child_style.padding_bottom +
                              child_style.border_width_top + child_style.border_width_bottom;
            }
            
            // All margins work for inline-block
            margin_left = child_style.margin_left;
            margin_right = child_style.margin_right;
            margin_top = child_style.margin_top;
            margin_bottom = child_style.margin_bottom;
            
        } else if (child_style.display == WidgetStyle::Display::Block || 
                   child_style.display == WidgetStyle::Display::Flex ||
                   child_style.display == WidgetStyle::Display::Grid) {
            // Block-level elements force a new line
            // Move to next line if not at start
            if (x > content_bounds.x) {
                x = content_bounds.x;
                y += line_height;
                line_height = 0;
            }
            
            // Block elements use full width
            child_width = child_style.width > 0 ? child_style.width : 
                         (content_bounds.width - child_style.margin_left - child_style.margin_right);
            child_height = child_style.height > 0 ? child_style.height :
                          (content_size.y + child_style.padding_top + child_style.padding_bottom);
            
            margin_left = child_style.margin_left;
            margin_right = child_style.margin_right;
            margin_top = child_style.margin_top;
            margin_bottom = child_style.margin_bottom;
            
            // Position block element
            child->set_bounds(Rect2D(
                content_bounds.x + margin_left,
                y + margin_top,
                child_width,
                child_height
            ));
            
            // Move to next line after block
            y += margin_top + child_height + margin_bottom;
            x = content_bounds.x;
            continue;  // Skip the inline positioning code
        } else {
            // Skip other display types in inline context
            continue;
        }
        
        // Check if we need to wrap to next line
        float total_width = margin_left + child_width + margin_right;
        if (current_line_width + total_width > content_bounds.width && !current_line_items.empty()) {
            // Apply text-align to current line before wrapping
            apply_text_align();
            y += line_height;
            line_height = 0;
        }
        
        // Add item to current line
        LineItem item;
        item.widget = child.get();
        item.width = child_width;
        item.height = child_height;
        item.margin_left = margin_left;
        item.margin_right = margin_right;
        item.margin_top = margin_top;
        item.margin_bottom = margin_bottom;
        
        current_line_items.push_back(item);
        current_line_width += total_width;
        
        // Update line height (only inline-block vertical margins affect it)
        float effective_height = child_height;
        if (child_style.display == WidgetStyle::Display::InlineBlock) {
            effective_height += margin_top + margin_bottom;
        }
        line_height = std::max(line_height, effective_height);
    }
    
    // Apply text-align to final line
    apply_text_align();
}

void StyledWidget::layout_flex() {
    Rect2D content_bounds = get_content_bounds();
    
    // Prepare flex items
    std::vector<FlexItem> flex_items;
    for (auto& child : children_) {
        if (!child->is_visible()) continue;
        
        // Ensure child style is computed
        if (child->needs_style_computation_) {
            // We need the theme to compute styles
            // This should have been passed down from render()
            continue;  // Skip for now if we can't compute
        }
        
        FlexItem item;
        item.widget = child.get();
        item.flex_grow = child->get_style().flex_grow;
        item.flex_shrink = child->get_style().flex_shrink;
        
        // Get flex basis - use intrinsic content size if computed size is 0
        const auto& child_style = child->get_computed_style();
        Point2D intrinsic_size = child->get_content_size();
        bool is_row = (computed_style_.flex_direction == WidgetStyle::FlexDirection::Row ||
                      computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse);
        
        // Calculate actual widget dimensions based on display type
        float content_width, content_height;
        
        
        // In a flex container, all direct children become flex items
        // Their display type affects their intrinsic sizing but they all participate in flex
        if (child_style.display == WidgetStyle::Display::Inline) {
            // Even inline elements become flex items and can have width/height in flex context
            // But if no explicit size is set, use intrinsic content size
            content_width = intrinsic_size.x + child_style.padding_left + child_style.padding_right;
            content_height = intrinsic_size.y + child_style.padding_top + child_style.padding_bottom;
        } else {
            // For block, inline-block, flex, grid - all use full box model
            content_width = intrinsic_size.x + child_style.padding_left + child_style.padding_right;
            content_height = intrinsic_size.y + child_style.padding_top + child_style.padding_bottom;
        }
        
        // Add border width for all display types (border is between padding and margin)
        content_width += child_style.border_width_left + child_style.border_width_right;
        content_height += child_style.border_width_top + child_style.border_width_bottom;
        
        // Store margins separately - they affect positioning but not the widget's own size
        if (is_row) {
            item.flex_basis = (child_style.width > 0) ? child_style.width : content_width;
            item.cross_size = (child_style.height > 0) ? child_style.height : content_height;
            item.margin_before = child_style.margin_left;
            item.margin_after = child_style.margin_right;
            item.margin_cross_before = child_style.margin_top;
            item.margin_cross_after = child_style.margin_bottom;
            
            // Layout calculation complete for this item
        } else {
            item.flex_basis = (child_style.height > 0) ? child_style.height : content_height;
            item.cross_size = (child_style.width > 0) ? child_style.width : content_width;
            item.margin_before = child_style.margin_top;
            item.margin_after = child_style.margin_bottom;
            item.margin_cross_before = child_style.margin_left;
            item.margin_cross_after = child_style.margin_right;
        }
        
        
        flex_items.push_back(item);
    }
    
    if (flex_items.empty()) return;
    
    // Determine layout direction
    bool is_row = (computed_style_.flex_direction == WidgetStyle::FlexDirection::Row ||
                  computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse);
    
    float main_axis_size = is_row ? content_bounds.width : content_bounds.height;
    float cross_axis_size = is_row ? content_bounds.height : content_bounds.width;
    
    // Determine which gaps to use based on direction
    // In row direction: column-gap between items, row-gap between lines
    // In column direction: row-gap between items, column-gap between lines
    float item_gap = is_row ? computed_style_.column_gap_pixels : computed_style_.row_gap_pixels;
    float line_gap = is_row ? computed_style_.row_gap_pixels : computed_style_.column_gap_pixels;
    
    // Group items into lines if wrapping is enabled
    std::vector<FlexLine> lines;
    
    if (computed_style_.flex_wrap == WidgetStyle::FlexWrap::Wrap || 
        computed_style_.flex_wrap == WidgetStyle::FlexWrap::WrapReverse) {
        // Create wrapped lines
        FlexLine current_line;
        float line_main_size = 0;
        
        for (auto& item : flex_items) {
            float item_main = item.flex_basis + item.margin_before + item.margin_after;
            
            // Check if item fits on current line
            if (!current_line.items.empty() && 
                line_main_size + item_main + item_gap > main_axis_size) {
                // Start new line
                lines.push_back(current_line);
                current_line = FlexLine();
                line_main_size = 0;
            }
            
            current_line.items.push_back(&item);
            current_line.main_size += item.flex_basis;
            current_line.cross_size = std::max(current_line.cross_size, 
                                              item.cross_size + item.margin_cross_before + item.margin_cross_after);
            line_main_size += item_main;
            if (!current_line.items.empty() && current_line.items.size() > 1) {
                line_main_size += item_gap;
            }
            current_line.main_space_used = line_main_size;
        }
        
        if (!current_line.items.empty()) {
            lines.push_back(current_line);
        }
    } else {
        // Single line (no wrap)
        FlexLine single_line;
        for (auto& item : flex_items) {
            single_line.items.push_back(&item);
            single_line.main_size += item.flex_basis;
            single_line.cross_size = std::max(single_line.cross_size, 
                                             item.cross_size + item.margin_cross_before + item.margin_cross_after);
        }
        float total_margins = 0;
        for (const auto& item : flex_items) {
            total_margins += item.margin_before + item.margin_after;
        }
        single_line.main_space_used = single_line.main_size + total_margins + 
                                      item_gap * (flex_items.size() - 1);
        lines.push_back(single_line);
    }
    
    // Calculate total cross size of all lines including gaps
    float total_lines_cross_size = 0;
    for (const auto& line : lines) {
        total_lines_cross_size += line.cross_size;
    }
    if (lines.size() > 1) {
        total_lines_cross_size += line_gap * (lines.size() - 1);
    }
    
    // Apply align-content to position lines
    float cross_pos = is_row ? content_bounds.y : content_bounds.x;
    float cross_free_space = cross_axis_size - total_lines_cross_size;
    
    switch (computed_style_.align_content) {
        case WidgetStyle::AlignContent::Start:
            // Lines packed at start
            break;
        case WidgetStyle::AlignContent::End:
            cross_pos += cross_free_space;
            break;
        case WidgetStyle::AlignContent::Center:
            cross_pos += cross_free_space / 2;
            break;
        case WidgetStyle::AlignContent::Stretch:
            // Lines stretch to fill available space
            if (lines.size() > 0) {
                float stretch_per_line = cross_free_space / lines.size();
                for (auto& line : lines) {
                    line.cross_size += stretch_per_line;
                }
            }
            break;
        case WidgetStyle::AlignContent::SpaceBetween:
            // Space between lines
            if (lines.size() > 1) {
                cross_free_space = cross_free_space / (lines.size() - 1);
            }
            break;
        case WidgetStyle::AlignContent::SpaceAround:
            cross_free_space = cross_free_space / lines.size();
            cross_pos += cross_free_space / 2;
            break;
        case WidgetStyle::AlignContent::SpaceEvenly:
            cross_free_space = cross_free_space / (lines.size() + 1);
            cross_pos += cross_free_space;
            break;
    }
    
    // Layout each line
    for (const auto& line : lines) {
        // Calculate flex for items in this line
        float line_free_space = main_axis_size - line.main_space_used;
        
        // Apply flex grow/shrink for this line
        if (line_free_space != 0) {
            float total_flex_grow = 0;
            float total_flex_shrink = 0;
            for (auto* item : line.items) {
                total_flex_grow += item->flex_grow;
                total_flex_shrink += item->flex_shrink;
            }
            
            if (line_free_space > 0 && total_flex_grow > 0) {
                // Distribute extra space
                for (auto* item : line.items) {
                    if (item->flex_grow > 0) {
                        float grow_amount = (line_free_space * item->flex_grow) / total_flex_grow;
                        item->main_size = item->flex_basis + grow_amount;
                    } else {
                        item->main_size = item->flex_basis;
                    }
                }
            } else if (line_free_space < 0 && total_flex_shrink > 0) {
                // Shrink items
                float shrink_space = -line_free_space;
                for (auto* item : line.items) {
                    if (item->flex_shrink > 0) {
                        float shrink_amount = (shrink_space * item->flex_shrink) / total_flex_shrink;
                        item->main_size = std::max(0.0f, item->flex_basis - shrink_amount);
                    } else {
                        item->main_size = item->flex_basis;
                    }
                }
            } else {
                // No flex needed
                for (auto* item : line.items) {
                    item->main_size = item->flex_basis;
                }
            }
        } else {
            for (auto* item : line.items) {
                item->main_size = item->flex_basis;
            }
        }
        
        // Position items within the line
        position_flex_line(line, content_bounds, cross_pos, line.cross_size);
        
        // Move to next line position
        cross_pos += line.cross_size;
        
        // Add line gap between lines (but not after last line)
        if (&line != &lines.back()) {
            cross_pos += line_gap;
        }
        
        // Add spacing for align-content
        if (computed_style_.align_content == WidgetStyle::AlignContent::SpaceBetween ||
            computed_style_.align_content == WidgetStyle::AlignContent::SpaceAround ||
            computed_style_.align_content == WidgetStyle::AlignContent::SpaceEvenly) {
            cross_pos += cross_free_space;
        }
    }
}

void StyledWidget::position_flex_line(const FlexLine& line, const Rect2D& content_bounds, 
                                      float cross_pos, float line_cross_size) {
    if (line.items.empty()) return;
    
    bool is_row = (computed_style_.flex_direction == WidgetStyle::FlexDirection::Row ||
                  computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse);
    bool is_reverse = (computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse ||
                      computed_style_.flex_direction == WidgetStyle::FlexDirection::ColumnReverse);
    
    float main_axis_size = is_row ? content_bounds.width : content_bounds.height;
    
    // Get the correct gap for items on main axis
    float item_gap = is_row ? computed_style_.column_gap_pixels : computed_style_.row_gap_pixels;
    
    // Calculate total main size used by items
    float total_main_size = 0;
    for (auto* item : line.items) {
        total_main_size += item->main_size + item->margin_before + item->margin_after;
    }
    total_main_size += item_gap * (line.items.size() - 1);
    
    // Determine starting position based on justify-content
    float main_pos = is_row ? content_bounds.x : content_bounds.y;
    float free_space = main_axis_size - total_main_size;
    
    switch (computed_style_.justify_content) {
        case WidgetStyle::JustifyContent::End:
            main_pos += free_space;
            break;
        case WidgetStyle::JustifyContent::Center:
            main_pos += free_space / 2;
            break;
        case WidgetStyle::JustifyContent::SpaceBetween:
            if (line.items.size() > 1) {
                free_space = free_space / (line.items.size() - 1);
            }
            break;
        case WidgetStyle::JustifyContent::SpaceAround:
            free_space = free_space / line.items.size();
            main_pos += free_space / 2;
            break;
        case WidgetStyle::JustifyContent::SpaceEvenly:
            free_space = free_space / (line.items.size() + 1);
            main_pos += free_space;
            break;
        default: // Start
            break;
    }
    
    // Position each item
    for (size_t i = 0; i < line.items.size(); ++i) {
        auto* item = is_reverse ? line.items[line.items.size() - 1 - i] : line.items[i];
        
        // Add margin before
        main_pos += item->margin_before;
        
        // Calculate cross position for this item based on align-items/align-self
        float item_cross_pos = cross_pos;
        
        // Get alignment for this item
        const auto& item_style = item->widget->get_computed_style();
        WidgetStyle::AlignItems item_align;
        
        if (item_style.align_self == WidgetStyle::AlignSelf::Auto) {
            item_align = computed_style_.align_items;
        } else {
            // Convert AlignSelf to AlignItems
            switch (item_style.align_self) {
                case WidgetStyle::AlignSelf::Start:
                    item_align = WidgetStyle::AlignItems::Start;
                    break;
                case WidgetStyle::AlignSelf::Center:
                    item_align = WidgetStyle::AlignItems::Center;
                    break;
                case WidgetStyle::AlignSelf::End:
                    item_align = WidgetStyle::AlignItems::End;
                    break;
                case WidgetStyle::AlignSelf::Stretch:
                    item_align = WidgetStyle::AlignItems::Stretch;
                    break;
                case WidgetStyle::AlignSelf::Baseline:
                    item_align = WidgetStyle::AlignItems::Baseline;
                    break;
                default:
                    item_align = computed_style_.align_items;
                    break;
            }
        }
        
        // Apply alignment
        switch (item_align) {
            case WidgetStyle::AlignItems::Start:
                item_cross_pos += item->margin_cross_before;
                break;
            case WidgetStyle::AlignItems::End:
                item_cross_pos += line_cross_size - item->cross_size - item->margin_cross_after;
                break;
            case WidgetStyle::AlignItems::Center:
                item_cross_pos += item->margin_cross_before + 
                                  (line_cross_size - item->cross_size - item->margin_cross_before - item->margin_cross_after) / 2;
                break;
            case WidgetStyle::AlignItems::Stretch:
                item_cross_pos += item->margin_cross_before;
                // TODO: Actually stretch the item's cross size
                break;
            case WidgetStyle::AlignItems::Baseline:
                // Baseline alignment
                if (is_row) {
                    float baseline_y = cross_pos + line_cross_size * 0.8f;
                    if (item_style.font_size_pixels > 0 && g_font_system) {
                        FontFace* font = g_font_system->get_font(
                            item_style.font_family.empty() ? "default" : item_style.font_family
                        );
                        float ascender = font ? font->get_ascender(item_style.font_size_pixels) : 0;
                        item_cross_pos = baseline_y - ascender;
                    } else {
                        item_cross_pos = baseline_y - item->cross_size;
                    }
                } else {
                    item_cross_pos += item->margin_cross_before;
                }
                break;
        }
        
        // Set child bounds
        Rect2D child_bounds;
        if (is_row) {
            child_bounds = Rect2D(main_pos, item_cross_pos, item->main_size, item->cross_size);
        } else {
            child_bounds = Rect2D(item_cross_pos, main_pos, item->cross_size, item->main_size);
        }
        
        item->widget->set_bounds(child_bounds);
        
        // Move to next position
        main_pos += item->main_size + item->margin_after + item_gap;
        
        // Add extra space for justify-content
        if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceBetween && i < line.items.size() - 1) {
            main_pos += free_space;
        } else if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceAround) {
            main_pos += free_space;
        } else if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceEvenly) {
            main_pos += free_space;
        }
    }
}

void StyledWidget::calculate_flex_main_axis(std::vector<FlexItem>& items, float available_space) {
    // Step 1: Calculate total flex basis (widget sizes, not including margins)
    float total_flex_basis = 0;
    float total_flex_grow = 0;
    float total_flex_shrink = 0;
    
    for (auto& item : items) {
        total_flex_basis += item.flex_basis;  // This is the widget size including padding but NOT margin
        total_flex_grow += item.flex_grow;
        total_flex_shrink += item.flex_shrink;
    }
    
    float free_space = available_space - total_flex_basis;
    
    // Step 2: Distribute space
    if (free_space > 0 && total_flex_grow > 0) {
        // Grow items
        for (auto& item : items) {
            if (item.flex_grow > 0) {
                float grow_amount = (free_space * item.flex_grow) / total_flex_grow;
                item.main_size = item.flex_basis + grow_amount;
            } else {
                item.main_size = item.flex_basis;
            }
        }
    } else if (free_space < 0 && total_flex_shrink > 0) {
        // Shrink items
        float shrink_space = -free_space;
        for (auto& item : items) {
            if (item.flex_shrink > 0) {
                float shrink_amount = (shrink_space * item.flex_shrink) / total_flex_shrink;
                item.main_size = std::max(0.0f, item.flex_basis - shrink_amount);
            } else {
                item.main_size = item.flex_basis;
            }
        }
    } else {
        // No flex needed
        for (auto& item : items) {
            item.main_size = item.flex_basis;
        }
    }
}

void StyledWidget::calculate_flex_cross_axis(std::vector<FlexItem>& items, float available_space) {
    // Handle align-items
    switch (computed_style_.align_items) {
        case WidgetStyle::AlignItems::Stretch:
            // Stretch items to fill cross axis
            for (auto& item : items) {
                item.cross_size = available_space;
            }
            break;
            
        case WidgetStyle::AlignItems::Start:
        case WidgetStyle::AlignItems::End:
        case WidgetStyle::AlignItems::Center:
        case WidgetStyle::AlignItems::Baseline:
            // Keep original cross size
            break;
    }
}

void StyledWidget::position_flex_items(const std::vector<FlexItem>& items) {
    Rect2D content_bounds = get_content_bounds();
    
    bool is_row = (computed_style_.flex_direction == WidgetStyle::FlexDirection::Row ||
                  computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse);
    bool is_reverse = (computed_style_.flex_direction == WidgetStyle::FlexDirection::RowReverse ||
                      computed_style_.flex_direction == WidgetStyle::FlexDirection::ColumnReverse);
    
    // Calculate total main axis size used
    float total_main_size = 0;
    for (const auto& item : items) {
        total_main_size += item.main_size;
    }
    total_main_size += computed_style_.gap_pixels * (items.size() - 1);
    
    // Determine starting position based on justify-content
    float main_pos = 0;
    float main_axis_size = is_row ? content_bounds.width : content_bounds.height;
    float free_space = main_axis_size - total_main_size;
    
    switch (computed_style_.justify_content) {
        case WidgetStyle::JustifyContent::Start:
            main_pos = is_row ? content_bounds.x : content_bounds.y;
            break;
        case WidgetStyle::JustifyContent::End:
            main_pos = (is_row ? content_bounds.x : content_bounds.y) + free_space;
            break;
        case WidgetStyle::JustifyContent::Center:
            main_pos = (is_row ? content_bounds.x : content_bounds.y) + free_space / 2;
            break;
        case WidgetStyle::JustifyContent::SpaceBetween:
            main_pos = is_row ? content_bounds.x : content_bounds.y;
            if (items.size() > 1) {
                free_space = free_space / (items.size() - 1);
            }
            break;
        case WidgetStyle::JustifyContent::SpaceAround:
            free_space = free_space / items.size();
            main_pos = (is_row ? content_bounds.x : content_bounds.y) + free_space / 2;
            break;
        case WidgetStyle::JustifyContent::SpaceEvenly:
            free_space = free_space / (items.size() + 1);
            main_pos = (is_row ? content_bounds.x : content_bounds.y) + free_space;
            break;
    }
    
    // Position each item
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = is_reverse ? items[items.size() - 1 - i] : items[i];
        
        // Apply margin before the item
        main_pos += item.margin_before;
        
        // Calculate cross axis position based on align-self (or align-items if auto)
        float cross_pos = 0;
        float cross_axis_size = is_row ? content_bounds.height : content_bounds.width;
        
        // Get the alignment for this item (align-self overrides align-items)
        const auto& item_style = item.widget->get_computed_style();
        WidgetStyle::AlignItems item_align;
        
        if (item_style.align_self == WidgetStyle::AlignSelf::Auto) {
            // Use parent's align-items
            item_align = computed_style_.align_items;
        } else {
            // Convert AlignSelf to AlignItems (they have the same values except Auto)
            switch (item_style.align_self) {
                case WidgetStyle::AlignSelf::Start:
                    item_align = WidgetStyle::AlignItems::Start;
                    break;
                case WidgetStyle::AlignSelf::Center:
                    item_align = WidgetStyle::AlignItems::Center;
                    break;
                case WidgetStyle::AlignSelf::End:
                    item_align = WidgetStyle::AlignItems::End;
                    break;
                case WidgetStyle::AlignSelf::Stretch:
                    item_align = WidgetStyle::AlignItems::Stretch;
                    break;
                case WidgetStyle::AlignSelf::Baseline:
                    item_align = WidgetStyle::AlignItems::Baseline;
                    break;
                default:
                    item_align = computed_style_.align_items;
                    break;
            }
        }
        
        // Apply the alignment
        switch (item_align) {
            case WidgetStyle::AlignItems::Start:
                cross_pos = (is_row ? content_bounds.y : content_bounds.x) + item.margin_cross_before;
                break;
            case WidgetStyle::AlignItems::End:
                cross_pos = (is_row ? content_bounds.y : content_bounds.x) + 
                           (cross_axis_size - item.cross_size - item.margin_cross_after);
                break;
            case WidgetStyle::AlignItems::Center:
                cross_pos = (is_row ? content_bounds.y : content_bounds.x) + 
                           item.margin_cross_before + 
                           (cross_axis_size - item.cross_size - item.margin_cross_before - item.margin_cross_after) / 2;
                break;
            case WidgetStyle::AlignItems::Stretch:
                cross_pos = (is_row ? content_bounds.y : content_bounds.x) + item.margin_cross_before;
                break;
            case WidgetStyle::AlignItems::Baseline:
                // Baseline alignment for flex items
                // Only makes sense for row direction
                if (is_row) {
                    // Calculate a common baseline for all items
                    // For simplicity, place baseline at 80% of container height
                    float baseline_y = content_bounds.y + cross_axis_size * 0.8f;
                    
                    // Get item's baseline offset
                    if (item_style.font_size_pixels > 0 && g_font_system) {
                        FontFace* font = g_font_system->get_font(
                            item_style.font_family.empty() ? "default" : item_style.font_family
                        );
                        float ascender = font ? font->get_ascender(item_style.font_size_pixels) : 0;
                        cross_pos = baseline_y - ascender;
                    } else {
                        // For non-text, align bottom to baseline
                        cross_pos = baseline_y - item.cross_size;
                    }
                } else {
                    // Baseline doesn't apply to column direction
                    cross_pos = (is_row ? content_bounds.y : content_bounds.x) + item.margin_cross_before;
                }
                break;
        }
        
        // Set child bounds (the widget's actual size, not including margins)
        Rect2D child_bounds;
        if (is_row) {
            child_bounds = Rect2D(main_pos, cross_pos, item.main_size, item.cross_size);
        } else {
            child_bounds = Rect2D(cross_pos, main_pos, item.cross_size, item.main_size);
        }
        
        item.widget->set_bounds(child_bounds);
        
        
        // Move to next position (widget size + margin after + gap)
        main_pos += item.main_size + item.margin_after + computed_style_.gap_pixels;
        
        // Add extra space for justify-content
        if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceBetween && i < items.size() - 1) {
            main_pos += free_space;
        } else if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceAround) {
            main_pos += free_space;
        } else if (computed_style_.justify_content == WidgetStyle::JustifyContent::SpaceEvenly) {
            main_pos += free_space;
        }
    }
}

void StyledWidget::layout_grid() {
    Rect2D content_bounds = get_content_bounds();
    
    // Step 1: Calculate grid track sizes (columns and rows)
    std::vector<float> column_sizes;
    std::vector<float> row_sizes;
    
    // Determine number of columns and rows from template
    size_t num_cols = computed_style_.grid_template_columns.empty() ? 1 : computed_style_.grid_template_columns.size();
    size_t num_rows = computed_style_.grid_template_rows.empty() ? 1 : computed_style_.grid_template_rows.size();
    
    // Calculate column widths
    if (!computed_style_.grid_template_columns.empty()) {
        float available_width = content_bounds.width - (computed_style_.column_gap_pixels * (num_cols - 1));
        float total_fr = 0;
        float used_width = 0;
        
        // First pass: calculate fixed sizes and count fr units
        for (const auto& col : computed_style_.grid_template_columns) {
            if (col.unit == SizeValue::MinMax) {
                // Handle minmax() - for now use minimum
                if (col.min_value) {
                    float min_width = 0;
                    if (col.min_value->unit == SizeValue::Pixels) {
                        min_width = col.min_value->value;
                    } else if (col.min_value->unit == SizeValue::Percent) {
                        min_width = content_bounds.width * (col.min_value->value / 100.0f);
                    }
                    column_sizes.push_back(min_width);
                    used_width += min_width;
                }
            } else if (col.unit == SizeValue::Flex) {
                // Check for auto-fill/auto-fit (negative values)
                if (col.value < 0) {
                    // This is auto-fill (-1) or auto-fit (-2)
                    // For now, treat as 1fr
                    total_fr += 1;
                    column_sizes.push_back(0);
                } else {
                    total_fr += col.value;
                    column_sizes.push_back(0);  // Will be calculated
                }
            } else if (col.unit == SizeValue::Pixels) {
                float width = col.value;
                column_sizes.push_back(width);
                used_width += width;
            } else if (col.unit == SizeValue::Percent) {
                float width = content_bounds.width * (col.value / 100.0f);
                column_sizes.push_back(width);
                used_width += width;
            } else if (col.unit == SizeValue::MinContent || col.unit == SizeValue::MaxContent) {
                // For now, treat as auto
                column_sizes.push_back(0);
            } else {  // Auto
                // For now, distribute evenly
                column_sizes.push_back(0);
            }
        }
        
        // Second pass: distribute remaining space to fr units
        if (total_fr > 0) {
            float fr_size = (available_width - used_width) / total_fr;
            for (size_t i = 0; i < column_sizes.size(); ++i) {
                if (computed_style_.grid_template_columns[i].unit == SizeValue::Flex) {
                    column_sizes[i] = fr_size * computed_style_.grid_template_columns[i].value;
                }
            }
        }
        
        // Handle auto columns
        float auto_count = 0;
        for (size_t i = 0; i < column_sizes.size(); ++i) {
            if (computed_style_.grid_template_columns[i].unit == SizeValue::Auto && column_sizes[i] == 0) {
                auto_count++;
            }
        }
        if (auto_count > 0) {
            float auto_size = (available_width - used_width) / auto_count;
            for (size_t i = 0; i < column_sizes.size(); ++i) {
                if (computed_style_.grid_template_columns[i].unit == SizeValue::Auto && column_sizes[i] == 0) {
                    column_sizes[i] = auto_size;
                }
            }
        }
    } else {
        // No template, use auto sizing
        column_sizes.push_back(content_bounds.width);
    }
    
    // Calculate row heights (similar to columns)
    if (!computed_style_.grid_template_rows.empty()) {
        float available_height = content_bounds.height - (computed_style_.row_gap_pixels * (num_rows - 1));
        float total_fr = 0;
        float used_height = 0;
        
        // First pass: calculate fixed sizes and count fr units
        for (const auto& row : computed_style_.grid_template_rows) {
            if (row.unit == SizeValue::MinMax) {
                // Handle minmax() - for now use minimum
                if (row.min_value) {
                    float min_height = 0;
                    if (row.min_value->unit == SizeValue::Pixels) {
                        min_height = row.min_value->value;
                    } else if (row.min_value->unit == SizeValue::Percent) {
                        min_height = content_bounds.height * (row.min_value->value / 100.0f);
                    }
                    row_sizes.push_back(min_height);
                    used_height += min_height;
                }
            } else if (row.unit == SizeValue::Flex) {
                total_fr += row.value;
                row_sizes.push_back(0);
            } else if (row.unit == SizeValue::Pixels) {
                float height = row.value;
                row_sizes.push_back(height);
                used_height += height;
            } else if (row.unit == SizeValue::Percent) {
                float height = content_bounds.height * (row.value / 100.0f);
                row_sizes.push_back(height);
                used_height += height;
            } else if (row.unit == SizeValue::MinContent || row.unit == SizeValue::MaxContent) {
                // For now, treat as auto
                row_sizes.push_back(0);
            } else {  // Auto
                row_sizes.push_back(0);
            }
        }
        
        // Second pass: distribute remaining space to fr units
        if (total_fr > 0) {
            float fr_size = (available_height - used_height) / total_fr;
            for (size_t i = 0; i < row_sizes.size(); ++i) {
                if (computed_style_.grid_template_rows[i].unit == SizeValue::Flex) {
                    row_sizes[i] = fr_size * computed_style_.grid_template_rows[i].value;
                }
            }
        }
        
        // Handle auto rows
        float auto_count = 0;
        for (size_t i = 0; i < row_sizes.size(); ++i) {
            if (computed_style_.grid_template_rows[i].unit == SizeValue::Auto && row_sizes[i] == 0) {
                auto_count++;
            }
        }
        if (auto_count > 0) {
            float auto_size = (available_height - used_height) / auto_count;
            for (size_t i = 0; i < row_sizes.size(); ++i) {
                if (computed_style_.grid_template_rows[i].unit == SizeValue::Auto && row_sizes[i] == 0) {
                    row_sizes[i] = auto_size;
                }
            }
        }
    } else {
        // No template, calculate based on children
        size_t child_count = children_.size();
        num_rows = (child_count + num_cols - 1) / num_cols;  // Ceiling division
        float row_height = (content_bounds.height - (computed_style_.row_gap_pixels * (num_rows - 1))) / num_rows;
        for (size_t i = 0; i < num_rows; ++i) {
            row_sizes.push_back(row_height);
        }
    }
    
    // Step 2: Place items in grid
    struct GridCell {
        int col, row;
        int col_span, row_span;
        StyledWidget* widget;
    };
    std::vector<GridCell> placed_items;
    
    // Track occupied cells for auto-placement
    std::vector<std::vector<bool>> occupied(num_rows, std::vector<bool>(num_cols, false));
    
    // Place explicitly positioned items first
    for (auto& child : children_) {
        if (!child->is_visible()) continue;
        
        const auto& child_style = child->get_computed_style();
        if (child_style.display == WidgetStyle::Display::None) continue;
        
        GridCell cell;
        cell.widget = child.get();
        
        // Check for grid area first
        if (!child_style.grid_area.empty() && computed_style_.grid_area_map.find(child_style.grid_area) != computed_style_.grid_area_map.end()) {
            // Use grid area bounds
            const auto& area_bounds = computed_style_.grid_area_map.at(child_style.grid_area);
            cell.row = area_bounds[0] - 1;  // Convert to 0-based
            cell.col = area_bounds[1] - 1;
            cell.row_span = area_bounds[2] - area_bounds[0];
            cell.col_span = area_bounds[3] - area_bounds[1];
            
            // Mark cells as occupied
            for (int r = cell.row; r < std::min((int)num_rows, cell.row + cell.row_span); ++r) {
                for (int c = cell.col; c < std::min((int)num_cols, cell.col + cell.col_span); ++c) {
                    occupied[r][c] = true;
                }
            }
            
            placed_items.push_back(cell);
        }
        // Check for explicit positioning
        else if (child_style.grid_column_start > 0 && child_style.grid_row_start > 0) {
            cell.col = child_style.grid_column_start - 1;  // Convert to 0-based
            cell.row = child_style.grid_row_start - 1;
            cell.col_span = child_style.grid_column_end > 0 ? 
                            child_style.grid_column_end - child_style.grid_column_start : 1;
            cell.row_span = child_style.grid_row_end > 0 ? 
                           child_style.grid_row_end - child_style.grid_row_start : 1;
            
            // Mark cells as occupied
            for (int r = cell.row; r < std::min((int)num_rows, cell.row + cell.row_span); ++r) {
                for (int c = cell.col; c < std::min((int)num_cols, cell.col + cell.col_span); ++c) {
                    occupied[r][c] = true;
                }
            }
            
            placed_items.push_back(cell);
        }
    }
    
    // Auto-place remaining items
    int auto_col = 0, auto_row = 0;
    bool row_flow = (computed_style_.grid_auto_flow == WidgetStyle::GridAutoFlow::Row || 
                     computed_style_.grid_auto_flow == WidgetStyle::GridAutoFlow::RowDense);
    
    for (auto& child : children_) {
        if (!child->is_visible()) continue;
        
        const auto& child_style = child->get_computed_style();
        if (child_style.display == WidgetStyle::Display::None) continue;
        
        // Skip if already placed
        bool already_placed = false;
        for (const auto& placed : placed_items) {
            if (placed.widget == child.get()) {
                already_placed = true;
                break;
            }
        }
        if (already_placed) continue;
        
        // Find next available cell
        GridCell cell;
        cell.widget = child.get();
        cell.col_span = 1;
        cell.row_span = 1;
        
        // Find next empty cell
        bool found = false;
        while (!found) {
            if (auto_row >= (int)num_rows) {
                // Need to add more rows - use grid_auto_rows
                float auto_row_size = 100;  // Default
                if (computed_style_.grid_template_rows.size() > 0) {
                    // Use grid_auto_rows if available
                    // Note: grid_auto_rows is stored in grid_template_rows (we should fix this)
                    auto_row_size = 100;  // For now, use default
                }
                row_sizes.push_back(auto_row_size);
                occupied.push_back(std::vector<bool>(num_cols, false));
                num_rows++;
            }
            
            if (!occupied[auto_row][auto_col]) {
                cell.col = auto_col;
                cell.row = auto_row;
                occupied[auto_row][auto_col] = true;
                found = true;
            }
            
            // Move to next cell
            if (row_flow) {
                auto_col++;
                if (auto_col >= (int)num_cols) {
                    auto_col = 0;
                    auto_row++;
                }
            } else {
                auto_row++;
                if (auto_row >= (int)num_rows) {
                    auto_row = 0;
                    auto_col++;
                    if (auto_col >= (int)num_cols) {
                        // Need to add more columns
                        column_sizes.push_back(100);  // Default size
                        for (auto& row : occupied) {
                            row.push_back(false);
                        }
                        num_cols++;
                    }
                }
            }
        }
        
        placed_items.push_back(cell);
    }
    
    // Step 3: Position and size items
    float y = content_bounds.y;
    for (size_t row = 0; row < num_rows; ++row) {
        float x = content_bounds.x;
        float row_height = row < row_sizes.size() ? row_sizes[row] : 100;
        
        for (size_t col = 0; col < num_cols; ++col) {
            float col_width = col < column_sizes.size() ? column_sizes[col] : 100;
            
            // Find item at this cell
            for (const auto& cell : placed_items) {
                if (cell.col == (int)col && cell.row == (int)row) {
                    // Calculate cell bounds
                    float cell_width = col_width;
                    float cell_height = row_height;
                    
                    // Add span sizes
                    for (int c = 1; c < cell.col_span && col + c < num_cols; ++c) {
                        cell_width += column_sizes[col + c] + computed_style_.column_gap_pixels;
                    }
                    for (int r = 1; r < cell.row_span && row + r < num_rows; ++r) {
                        cell_height += row_sizes[row + r] + computed_style_.row_gap_pixels;
                    }
                    
                    // Apply alignment within cell
                    float item_x = x;
                    float item_y = y;
                    float item_width = cell_width;
                    float item_height = cell_height;
                    
                    // Apply justify-items (horizontal alignment)
                    const auto& item_style = cell.widget->get_computed_style();
                    if (item_style.width > 0 && item_style.width < cell_width) {
                        switch (computed_style_.justify_items) {
                            case WidgetStyle::JustifyItems::Center:
                                item_x += (cell_width - item_style.width) / 2;
                                item_width = item_style.width;
                                break;
                            case WidgetStyle::JustifyItems::End:
                                item_x += cell_width - item_style.width;
                                item_width = item_style.width;
                                break;
                            case WidgetStyle::JustifyItems::Start:
                                item_width = item_style.width;
                                break;
                            default: // Stretch
                                break;
                        }
                    }
                    
                    // Apply align-items (vertical alignment)
                    if (item_style.height > 0 && item_style.height < cell_height) {
                        switch (computed_style_.align_items) {
                            case WidgetStyle::AlignItems::Center:
                                item_y += (cell_height - item_style.height) / 2;
                                item_height = item_style.height;
                                break;
                            case WidgetStyle::AlignItems::End:
                                item_y += cell_height - item_style.height;
                                item_height = item_style.height;
                                break;
                            case WidgetStyle::AlignItems::Start:
                                item_height = item_style.height;
                                break;
                            default: // Stretch
                                break;
                        }
                    }
                    
                    cell.widget->set_bounds(Rect2D(item_x, item_y, item_width, item_height));
                }
            }
            
            x += col_width;
            if (col < num_cols - 1) {
                x += computed_style_.column_gap_pixels;
            }
        }
        
        y += row_height;
        if (row < num_rows - 1) {
            y += computed_style_.row_gap_pixels;
        }
    }
}

void StyledWidget::compute_style(const ScaledTheme& theme) {
    // Debug for buttons
    
    
    // Start with base style
    current_style_ = base_style_;
    
    // Apply state styles
    apply_state_style();
    
    
    // Compute final values
    computed_style_ = current_style_.compute(theme, 
                                            parent_ ? parent_->get_bounds() : bounds_,
                                            get_content_size());
    
    
    needs_style_computation_ = false;
    
    // IMPORTANT: Propagate to children so they have computed styles for layout
    for (auto& child : children_) {
        if (child->needs_style_computation_) {
            child->compute_style(theme);
        }
    }
}

void StyledWidget::apply_state_style() {
    // Reset to base style first to avoid accumulating changes
    current_style_ = base_style_;
    
    
    // Apply state-based style overrides
    if (is_disabled() && base_style_.disabled_style) {
        current_style_.apply_state(base_style_.disabled_style.get());
    } else if (is_active() && base_style_.active_style) {
        current_style_.apply_state(base_style_.active_style.get());
    } else if (is_hovered() && base_style_.hover_style) {
        current_style_.apply_state(base_style_.hover_style.get());
    } else if (is_focused() && base_style_.focus_style) {
        current_style_.apply_state(base_style_.focus_style.get());
    }
}

} // namespace voxel_canvas