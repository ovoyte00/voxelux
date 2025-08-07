/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional theme and color management system.
 * Independent implementation using standard JSON parsing techniques.
 */

#pragma once

#include <array>
#include <string>
#include <QColor>
#include <QJsonObject>

/**
 * @brief Centralized theme and color management for Voxelux
 * 
 * Loads colors from assets/styles/colors.json and provides consistent
 * access throughout the codebase. Colors are defined once and referenced
 * everywhere to ensure visual consistency.
 */
class Theme {
public:
    // RGB color arrays for OpenGL rendering
    using Color3f = std::array<float, 3>;
    using Color4f = std::array<float, 4>;
    
    // Axis colors for 3D navigation widget and grid
    struct AxisColors {
        Color3f x_axis;       // Red #F53E5F
        Color3f y_axis;       // Green #7ECA3C  
        Color3f z_axis;       // Blue #4498DB
    };
    
    // Grid colors for infinite grid system
    struct GridColors {
        Color3f major_lines;  // Major grid lines
        Color3f minor_lines;  // Minor grid lines  
        Color3f origin;       // Origin highlight
    };
    
    // UI colors for interface elements
    struct UIColors {
        Color3f background;
        Color3f panel;
        Color3f text;
        Color3f text_secondary;
        Color3f selection;
        Color3f hover;
    };
    
    // Widget transparency values
    struct WidgetAlpha {
        float positive_sphere;      // 1.0 - fully opaque
        float negative_sphere_fill; // 0.25 - low opacity fill
        float negative_sphere_ring; // 0.8 - visible ring outline
        float line;                 // 0.8 - semi-transparent lines
        float hover_background;     // 0.3 - subtle hover effect
    };
    
    static Theme& instance();
    bool loadFromFile(const std::string& filepath);
    
    // Accessors for different color categories
    const AxisColors& axes() const { return axis_colors_; }
    const GridColors& grid() const { return grid_colors_; }
    const UIColors& ui() const { return ui_colors_; }
    const WidgetAlpha& alpha() const { return widget_alpha_; }
    
    // Convenience methods for common operations
    QColor axisQColor(int axis) const; // 0=X, 1=Y, 2=Z
    Color4f axisColor4f(int axis, float alpha = 1.0f) const;
    
private:
    Theme() = default;
    void setDefaults();
    Color3f jsonToColor3f(const QJsonObject& colorObj) const;
    
    AxisColors axis_colors_;
    GridColors grid_colors_;
    UIColors ui_colors_;
    WidgetAlpha widget_alpha_;
    
    std::string theme_name_ = "voxelux_professional";
    std::string version_ = "1.0";
};

// Global convenience functions
namespace Colors {
    inline const Theme::Color3f& XAxis() { return Theme::instance().axes().x_axis; }
    inline const Theme::Color3f& YAxis() { return Theme::instance().axes().y_axis; }
    inline const Theme::Color3f& ZAxis() { return Theme::instance().axes().z_axis; }
    
    inline const Theme::Color3f& GridMajor() { return Theme::instance().grid().major_lines; }
    inline const Theme::Color3f& GridMinor() { return Theme::instance().grid().minor_lines; }
    
    inline const Theme::Color3f& Background() { return Theme::instance().ui().background; }
    
    inline float PositiveSphere() { return Theme::instance().alpha().positive_sphere; }
    inline float NegativeFill() { return Theme::instance().alpha().negative_sphere_fill; }
    inline float NegativeRing() { return Theme::instance().alpha().negative_sphere_ring; }
}