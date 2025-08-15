/*
 * Copyright (C) 2024 Voxelux
 * 
 * Scaled Theme System - Automatically handles DPI scaling for all UI dimensions
 */

#pragma once

#include "canvas_core.h"

namespace voxel_canvas {

/**
 * ScaledTheme - Wrapper around CanvasTheme that automatically scales all dimensions
 * based on content scale (DPI). This ensures consistent scaling without manual
 * multiplication throughout the codebase.
 */
class ScaledTheme {
public:
    ScaledTheme(const CanvasTheme& base_theme, float content_scale)
        : base_theme_(base_theme)
        , content_scale_(content_scale) {}
    
    // Automatically scaled spacing values
    float spacing_xs() const { return base_theme_.spacing_xs * content_scale_; }
    float spacing_sm() const { return base_theme_.spacing_sm * content_scale_; }
    float spacing_md() const { return base_theme_.spacing_md * content_scale_; }
    float spacing_lg() const { return base_theme_.spacing_lg * content_scale_; }
    float spacing_xl() const { return base_theme_.spacing_xl * content_scale_; }
    
    // Scaled dimensions
    float menu_height() const { return base_theme_.menu_height * content_scale_; }
    float footer_height() const { return base_theme_.footer_height * content_scale_; }
    float file_bar_height() const { return base_theme_.file_bar_height * content_scale_; }
    float dock_header_height() const { return base_theme_.dock_header_height * content_scale_; }
    float dock_collapsed_width() const { return base_theme_.dock_collapsed_width * content_scale_; }
    float dock_expanded_width() const { return base_theme_.dock_expanded_width * content_scale_; }
    float timeline_default_height() const { return base_theme_.timeline_default_height * content_scale_; }
    
    // Scaled UI element sizes
    float button_height() const { return base_theme_.button_height * content_scale_; }
    float input_height() const { return base_theme_.input_height * content_scale_; }
    float tool_icon_size() const { return base_theme_.tool_icon_size * content_scale_; }
    float tab_close_size() const { return base_theme_.tab_close_size * content_scale_; }
    float panel_tab_height() const { return base_theme_.panel_tab_height * content_scale_; }
    
    // Scaled border and corner radii
    float button_border_radius() const { return base_theme_.button_border_radius * content_scale_; }
    float border_radius() const { return base_theme_.button_border_radius * content_scale_; }  // Alias
    float border_width() const { return 1.0f * content_scale_; }  // Standard border width
    float resize_handle_hit_area() const { return base_theme_.resize_handle_hit_area * content_scale_; }
    
    // Scaled font sizes - predefined
    float font_size_xs() const { return 10.0f * content_scale_; }
    float font_size_sm() const { return 11.0f * content_scale_; }
    float font_size_md() const { return 12.0f * content_scale_; }
    float font_size_lg() const { return 14.0f * content_scale_; }
    float font_size_xl() const { return 16.0f * content_scale_; }
    float font_size_xxl() const { return 20.0f * content_scale_; }
    float font_size_huge() const { return 24.0f * content_scale_; }
    
    // Scaled icon sizes - predefined (matches IconSize enum)
    float icon_size_tiny() const { return 12.0f * content_scale_; }   // For inline UI elements
    float icon_size_small() const { return 16.0f * content_scale_; }  // For buttons, tabs, menu bar
    float icon_size_medium() const { return 24.0f * content_scale_; } // For toolbar buttons
    float icon_size_large() const { return 32.0f * content_scale_; }  // For tool palette
    float icon_size_xlarge() const { return 48.0f * content_scale_; } // For large buttons
    
    // Custom font size - pass any pixel value
    float font_size(float pixels) const { return pixels * content_scale_; }
    
    // Custom icon size - pass any pixel value
    float icon_size(float pixels) const { return pixels * content_scale_; }
    
    // Custom spacing - pass any pixel value  
    float spacing(float pixels) const { return pixels * content_scale_; }
    
    // Custom dimension - pass any pixel value
    float dimension(float pixels) const { return pixels * content_scale_; }
    
    // Colors remain unscaled
    const ColorRGBA& gray_1() const { return base_theme_.gray_1; }
    const ColorRGBA& gray_2() const { return base_theme_.gray_2; }
    const ColorRGBA& gray_3() const { return base_theme_.gray_3; }
    const ColorRGBA& gray_4() const { return base_theme_.gray_4; }
    const ColorRGBA& gray_5() const { return base_theme_.gray_5; }
    const ColorRGBA& accent_primary() const { return base_theme_.accent_primary; }
    const ColorRGBA& accent_secondary() const { return base_theme_.accent_secondary; }
    const ColorRGBA& accent_warning() const { return base_theme_.accent_warning; }
    
    // Convenience color accessors
    const ColorRGBA& error_color() const { return base_theme_.accent_warning; }
    const ColorRGBA& warning_color() const { return base_theme_.accent_warning; }
    const ColorRGBA& success_color() const { return base_theme_.accent_secondary; }
    const ColorRGBA& accent_hover() const { return base_theme_.widget_hover; }
    const ColorRGBA& accent_pressed() const { return base_theme_.widget_active; }
    
    // Get scale factor
    float get_scale() const { return content_scale_; }
    
    // Get base theme for cases where unscaled values are needed
    const CanvasTheme& get_base() const { return base_theme_; }
    
    // Helper to scale any arbitrary value
    float scale(float value) const { return value * content_scale_; }
    
    // Helper to create scaled rectangles with proper spacing
    Rect2D inset_rect(const Rect2D& rect, float inset_amount) const {
        float scaled_inset = inset_amount * content_scale_;
        return Rect2D(
            rect.x + scaled_inset,
            rect.y + scaled_inset,
            rect.width - scaled_inset * 2,
            rect.height - scaled_inset * 2
        );
    }
    
    // Create standard padding rect
    Rect2D padded_rect(const Rect2D& rect) const {
        return inset_rect(rect, base_theme_.spacing_md);
    }
    
private:
    const CanvasTheme& base_theme_;
    float content_scale_;
};

/**
 * Helper to get scaled theme from renderer
 * This is the primary way to access theme values in render functions
 * Implementation moved to canvas_renderer.h to avoid circular dependency
 */
ScaledTheme get_scaled_theme(CanvasRenderer* renderer);

} // namespace voxel_canvas