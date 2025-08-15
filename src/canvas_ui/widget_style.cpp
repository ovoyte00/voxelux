/*
 * Copyright (C) 2024 Voxelux
 * 
 * Widget Style System Implementation
 */

#include "canvas_ui/widget_style.h"
#include "canvas_ui/scaled_theme.h"
#include <sstream>
#include <string>
#include "canvas_ui/canvas_renderer.h"  // For get_scaled_theme inline function
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// SpacingValue implementation

float SpacingValue::resolve(const ScaledTheme& theme) const {
    switch (type) {
        case Pixels:
            return theme.scale(value);  // Apply DPI scaling
            
        case ThemeSpacing:
            switch (static_cast<int>(value)) {
                case 0: return theme.spacing_xs();
                case 1: return theme.spacing_sm();
                case 2: return theme.spacing_md();
                case 3: return theme.spacing_lg();
                case 4: return theme.spacing_xl();
                default: return theme.spacing_md();  // Default to medium
            }
            
        case Auto:
            return 0;  // Will be calculated based on content
            
        case Inherit:
            return 0;  // Will be inherited from parent
            
        default:
            return 0;
    }
}

// SizeValue implementation

float SizeValue::resolve(float parent_size, float content_size, const ScaledTheme& theme) const {
    switch (unit) {
        case Pixels:
            return theme.scale(value);  // Apply DPI scaling
            
        case Percent:
            return parent_size * (value / 100.0f);
            
        case Auto:
            return content_size;  // Use content size
            
        case FitContent:
            return content_size;  // For now, same as auto
            
        case Flex:
            return 0;  // Handled by flex layout algorithm
            
        case Inherit:
            return parent_size;  // Use parent size
            
        default:
            return content_size;
    }
}

// ColorValue implementation

ColorRGBA ColorValue::resolve(const ScaledTheme& theme) const {
    if (type == Direct) {
        return color;
    } else if (type == ThemeColor) {
        // Map theme reference to actual color
        if (theme_ref == "gray_1") return theme.gray_1();
        else if (theme_ref == "gray_2") return theme.gray_2();
        else if (theme_ref == "gray_3") return theme.gray_3();
        else if (theme_ref == "gray_4") return theme.gray_4();
        else if (theme_ref == "gray_5") return theme.gray_5();
        else if (theme_ref == "accent_primary") return theme.accent_primary();
        else if (theme_ref == "accent_secondary") return theme.accent_secondary();
        else if (theme_ref == "accent_warning") return theme.accent_warning();
        else if (theme_ref == "accent_hover") return theme.accent_hover();
        else if (theme_ref == "accent_pressed") return theme.accent_pressed();
        else if (theme_ref == "transparent") return ColorRGBA(0, 0, 0, 0);
        else if (theme_ref == "white") return ColorRGBA(1, 1, 1, 1);
        else if (theme_ref == "black") return ColorRGBA(0, 0, 0, 1);
        else return theme.gray_3();  // Default fallback
    }
    return ColorRGBA(0, 0, 0, 0);  // Transparent for inherit
}

// WidgetStyle implementation

WidgetStyle::WidgetStyle(const WidgetStyle& other) 
    : display(other.display)
    , position(other.position)
    , padding(other.padding)
    , margin(other.margin)
    , border(other.border)
    , width(other.width)
    , height(other.height)
    , min_width(other.min_width)
    , max_width(other.max_width)
    , min_height(other.min_height)
    , max_height(other.max_height)
    , flex_direction(other.flex_direction)
    , align_items(other.align_items)
    , align_self(other.align_self)
    , align_content(other.align_content)
    , justify_content(other.justify_content)
    , flex_wrap(other.flex_wrap)
    , gap(other.gap)
    , row_gap(other.row_gap)
    , column_gap(other.column_gap)
    , flex_grow(other.flex_grow)
    , flex_shrink(other.flex_shrink)
    , flex_basis(other.flex_basis)
    , background(other.background)
    , text_color(other.text_color)
    , opacity(other.opacity)
    , font_family(other.font_family)
    , font_size(other.font_size)
    , font_weight(other.font_weight)
    , text_align(other.text_align)
    , vertical_align(other.vertical_align)
    , line_height(other.line_height)
    , letter_spacing(other.letter_spacing)
    , box_shadow(other.box_shadow)
    , box_shadows(other.box_shadows)
    , overflow_x(other.overflow_x)
    , overflow_y(other.overflow_y)
    , top(other.top)
    , right(other.right)
    , bottom(other.bottom)
    , left(other.left)
    , z_index(other.z_index)
    , cursor(other.cursor) {
    
    // Deep copy state variants
    if (other.hover_style) {
        hover_style = std::make_unique<WidgetStyle>(*other.hover_style);
    }
    if (other.active_style) {
        active_style = std::make_unique<WidgetStyle>(*other.active_style);
    }
    if (other.focus_style) {
        focus_style = std::make_unique<WidgetStyle>(*other.focus_style);
    }
    if (other.disabled_style) {
        disabled_style = std::make_unique<WidgetStyle>(*other.disabled_style);
    }
}

WidgetStyle& WidgetStyle::operator=(const WidgetStyle& other) {
    if (this != &other) {
        display = other.display;
        position = other.position;
        padding = other.padding;
        margin = other.margin;
        border = other.border;
        width = other.width;
        height = other.height;
        min_width = other.min_width;
        max_width = other.max_width;
        min_height = other.min_height;
        max_height = other.max_height;
        flex_direction = other.flex_direction;
        align_items = other.align_items;
        align_self = other.align_self;
        align_content = other.align_content;
        justify_content = other.justify_content;
        flex_wrap = other.flex_wrap;
        gap = other.gap;
        row_gap = other.row_gap;
        column_gap = other.column_gap;
        flex_grow = other.flex_grow;
        flex_shrink = other.flex_shrink;
        flex_basis = other.flex_basis;
        background = other.background;
        text_color = other.text_color;
        opacity = other.opacity;
        font_family = other.font_family;
        font_size = other.font_size;
        font_weight = other.font_weight;
        text_align = other.text_align;
        vertical_align = other.vertical_align;
        line_height = other.line_height;
        letter_spacing = other.letter_spacing;
        box_shadow = other.box_shadow;
        box_shadows = other.box_shadows;
        overflow_x = other.overflow_x;
        overflow_y = other.overflow_y;
        top = other.top;
        right = other.right;
        bottom = other.bottom;
        left = other.left;
        z_index = other.z_index;
        cursor = other.cursor;
        
        // Deep copy state variants
        hover_style = other.hover_style ? std::make_unique<WidgetStyle>(*other.hover_style) : nullptr;
        active_style = other.active_style ? std::make_unique<WidgetStyle>(*other.active_style) : nullptr;
        focus_style = other.focus_style ? std::make_unique<WidgetStyle>(*other.focus_style) : nullptr;
        disabled_style = other.disabled_style ? std::make_unique<WidgetStyle>(*other.disabled_style) : nullptr;
    }
    return *this;
}

void WidgetStyle::merge(const WidgetStyle& other) {
    // Only override if the other style has explicitly set values
    // This is a simplified merge - in production you'd track which properties are set
    
    if (other.display != Display::Block) display = other.display;
    if (other.position != Position::Static) position = other.position;
    
    // Only override if values are non-default
    // Check if padding is non-zero
    if (other.padding.top.value != 0 || other.padding.right.value != 0 ||
        other.padding.bottom.value != 0 || other.padding.left.value != 0) {
        padding = other.padding;
    }
    if (other.margin.top.value != 0 || other.margin.right.value != 0 ||
        other.margin.bottom.value != 0 || other.margin.left.value != 0) {
        margin = other.margin;
    }
    if (other.border.width.value != 0) {
        border = other.border;
    }
    if (other.width.unit != SizeValue::Auto) width = other.width;
    if (other.height.unit != SizeValue::Auto) height = other.height;
    if (other.min_width.unit != SizeValue::Auto) min_width = other.min_width;
    if (other.max_width.unit != SizeValue::Auto) max_width = other.max_width;
    if (other.min_height.unit != SizeValue::Auto) min_height = other.min_height;
    if (other.max_height.unit != SizeValue::Auto) max_height = other.max_height;
    
    flex_direction = other.flex_direction;
    align_items = other.align_items;
    justify_content = other.justify_content;
    flex_wrap = other.flex_wrap;
    gap = other.gap;
    flex_grow = other.flex_grow;
    flex_shrink = other.flex_shrink;
    flex_basis = other.flex_basis;
    
    // Only override colors if they've been explicitly set
    // Check if it's not using the default theme color
    if (other.background.type == ColorValue::Direct || 
        (other.background.type == ColorValue::ThemeColor && other.background.theme_ref != "gray_3")) {
        background = other.background;
    }
    if (other.text_color.type == ColorValue::Direct || 
        (other.text_color.type == ColorValue::ThemeColor && other.text_color.theme_ref != "gray_3")) {
        text_color = other.text_color;
    }
    if (other.opacity != 1.0f) {
        opacity = other.opacity;
    }
    
    if (!other.font_family.empty()) font_family = other.font_family;
    // Only override font_size if it's been explicitly set (non-zero)
    if (other.font_size.value != 0) {
        font_size = other.font_size;
    }
    // Only override font_weight if it's non-default (400 is normal)
    if (other.font_weight != 0) {
        font_weight = other.font_weight;
    }
    if (other.text_align != TextAlign::Left) text_align = other.text_align;
    vertical_align = other.vertical_align;
    line_height = other.line_height;
    letter_spacing = other.letter_spacing;
    
    // Check if box_shadow has been set (non-zero blur or offset)
    if (other.box_shadow.blur_radius != 0 || other.box_shadow.offset_x != 0 || 
        other.box_shadow.offset_y != 0) {
        box_shadow = other.box_shadow;
    }
    if (other.box_shadows) box_shadows = other.box_shadows;
    
    overflow_x = other.overflow_x;
    overflow_y = other.overflow_y;
    
    if (other.top) top = other.top;
    if (other.right) right = other.right;
    if (other.bottom) bottom = other.bottom;
    if (other.left) left = other.left;
    if (other.z_index) z_index = other.z_index;
    
    if (!other.cursor.empty()) cursor = other.cursor;
    
    // Merge state variants
    if (other.hover_style) {
        if (hover_style) {
            hover_style->merge(*other.hover_style);
        } else {
            hover_style = std::make_unique<WidgetStyle>(*other.hover_style);
        }
    }
    if (other.active_style) {
        if (active_style) {
            active_style->merge(*other.active_style);
        } else {
            active_style = std::make_unique<WidgetStyle>(*other.active_style);
        }
    }
    if (other.focus_style) {
        if (focus_style) {
            focus_style->merge(*other.focus_style);
        } else {
            focus_style = std::make_unique<WidgetStyle>(*other.focus_style);
        }
    }
    if (other.disabled_style) {
        if (disabled_style) {
            disabled_style->merge(*other.disabled_style);
        } else {
            disabled_style = std::make_unique<WidgetStyle>(*other.disabled_style);
        }
    }
}

void WidgetStyle::apply_state(const WidgetStyle* state_style) {
    if (state_style) {
        merge(*state_style);
    }
}

WidgetStyle::ComputedStyle WidgetStyle::compute(const ScaledTheme& theme,
                                               const Rect2D& parent_bounds,
                                               const Point2D& content_size) const {
    ComputedStyle computed;
    
    // Resolve spacing values
    computed.padding_top = padding.top.resolve(theme);
    computed.padding_right = padding.right.resolve(theme);
    computed.padding_bottom = padding.bottom.resolve(theme);
    computed.padding_left = padding.left.resolve(theme);
    
    computed.margin_top = margin.top.resolve(theme);
    computed.margin_right = margin.right.resolve(theme);
    computed.margin_bottom = margin.bottom.resolve(theme);
    computed.margin_left = margin.left.resolve(theme);
    
    // Resolve border
    computed.border_width_top = border.width_top ? border.width_top->resolve(theme) : border.width.resolve(theme);
    computed.border_width_right = border.width_right ? border.width_right->resolve(theme) : border.width.resolve(theme);
    computed.border_width_bottom = border.width_bottom ? border.width_bottom->resolve(theme) : border.width.resolve(theme);
    computed.border_width_left = border.width_left ? border.width_left->resolve(theme) : border.width.resolve(theme);
    
    computed.border_radius_tl = border.radius_top_left ? border.radius_top_left->resolve(theme) : border.radius.resolve(theme);
    computed.border_radius_tr = border.radius_top_right ? border.radius_top_right->resolve(theme) : border.radius.resolve(theme);
    computed.border_radius_bl = border.radius_bottom_left ? border.radius_bottom_left->resolve(theme) : border.radius.resolve(theme);
    computed.border_radius_br = border.radius_bottom_right ? border.radius_bottom_right->resolve(theme) : border.radius.resolve(theme);
    
    computed.border_color = border.color.resolve(theme);
    
    // Resolve sizes
    float parent_width = parent_bounds.width;
    float parent_height = parent_bounds.height;
    
    computed.width = width.resolve(parent_width, content_size.x, theme);
    computed.height = height.resolve(parent_height, content_size.y, theme);
    computed.min_width = min_width.resolve(parent_width, 0, theme);
    computed.max_width = max_width.resolve(parent_width, INFINITY, theme);
    computed.min_height = min_height.resolve(parent_height, 0, theme);
    computed.max_height = max_height.resolve(parent_height, INFINITY, theme);
    
    // Apply constraints
    computed.width = std::max(computed.min_width, std::min(computed.max_width, computed.width));
    computed.height = std::max(computed.min_height, std::min(computed.max_height, computed.height));
    
    // Resolve colors
    computed.background_color = background.resolve(theme);
    computed.text_color_rgba = text_color.resolve(theme);
    
    // Resolve typography
    computed.font_size_pixels = font_size.resolve(theme);
    computed.letter_spacing_pixels = letter_spacing.resolve(theme);
    
    // Resolve flex gaps
    computed.gap_pixels = gap.resolve(theme);  // Keep for backward compatibility
    computed.row_gap_pixels = row_gap.resolve(theme);
    computed.column_gap_pixels = column_gap.resolve(theme);
    
    // Copy non-resolved values
    computed.display = display;
    computed.position = position;
    computed.flex_direction = flex_direction;
    computed.align_items = align_items;
    computed.align_self = align_self;
    computed.align_content = align_content;
    computed.justify_content = justify_content;
    computed.flex_wrap = flex_wrap;
    computed.flex_grow = flex_grow;
    computed.flex_shrink = flex_shrink;
    computed.text_align = text_align;
    computed.vertical_align = vertical_align;
    computed.line_height = line_height;
    computed.font_weight = font_weight;
    computed.font_family = font_family;
    computed.opacity = opacity;
    computed.overflow_x = overflow_x;
    computed.overflow_y = overflow_y;
    computed.visibility = visibility;
    computed.user_select = user_select;
    computed.white_space = white_space;
    computed.word_break = word_break;
    computed.text_overflow = text_overflow;
    computed.transform = transform;
    computed.transform_origin = transform_origin;
    computed.outline = outline;
    computed.box_shadow = box_shadow;
    computed.background_image = background_image;
    computed.background_images = background_images;
    computed.background_gradient = background_gradient;
    computed.background_gradients = background_gradients;
    computed.border_gradient = border_gradient;
    computed.outline_gradient = outline_gradient;
    
    // Copy grid properties
    computed.grid_template_columns = grid_template_columns;
    computed.grid_template_rows = grid_template_rows;
    computed.grid_column_start = grid_column_start;
    computed.grid_column_end = grid_column_end;
    computed.grid_row_start = grid_row_start;
    computed.grid_row_end = grid_row_end;
    computed.grid_auto_flow = grid_auto_flow;
    computed.justify_items = justify_items;
    computed.grid_area = grid_area;
    computed.grid_area_map = grid_area_map;
    
    // Resolve absolute positioning
    if (top) computed.pos_top = top->resolve(theme);
    if (right) computed.pos_right = right->resolve(theme);
    if (bottom) computed.pos_bottom = bottom->resolve(theme);
    if (left) computed.pos_left = left->resolve(theme);
    
    computed.z_index = z_index;
    
    return computed;
}

// Forward declaration
static SizeValue parse_single_grid_value(const std::string& value_str);
static SizeValue parse_minmax(const std::string& minmax_str);

static SizeValue parse_single_grid_value(const std::string& value_str) {
    if (value_str == "auto") {
        return SizeValue::auto_size();
    } else if (value_str == "min-content") {
        return SizeValue::min_content();
    } else if (value_str == "max-content") {
        return SizeValue::max_content();
    } else if (value_str == "fit-content") {
        return SizeValue::fit_content();
    } else if (value_str.substr(0, 7) == "minmax(") {
        return parse_minmax(value_str);
    } else if (value_str.size() > 2 && value_str.substr(value_str.size() - 2) == "fr") {
        // Parse fr units (e.g., "1fr", "2.5fr")
        float fr_value = std::stof(value_str.substr(0, value_str.size() - 2));
        return SizeValue::flex(fr_value);
    } else if (value_str.size() > 2 && value_str.substr(value_str.size() - 2) == "px") {
        // Parse pixel values (e.g., "100px")
        float px_value = std::stof(value_str.substr(0, value_str.size() - 2));
        return SizeValue::pixels(px_value);
    } else if (value_str.size() > 1 && value_str.back() == '%') {
        // Parse percentage values (e.g., "50%")
        float pct_value = std::stof(value_str.substr(0, value_str.size() - 1));
        return SizeValue::percent(pct_value);
    } else {
        // Try to parse as number (assume pixels)
        try {
            float num_value = std::stof(value_str);
            return SizeValue::pixels(num_value);
        } catch (...) {
            // Default to auto if we can't parse
            return SizeValue::auto_size();
        }
    }
}

// Parse helper for minmax() function
static SizeValue parse_minmax(const std::string& minmax_str) {
    // Parse minmax(min, max)
    if (minmax_str.substr(0, 7) != "minmax(") {
        return SizeValue::auto_size();
    }
    
    size_t comma_pos = minmax_str.find(',', 7);
    if (comma_pos == std::string::npos) {
        return SizeValue::auto_size();
    }
    
    std::string min_str = minmax_str.substr(7, comma_pos - 7);
    std::string max_str = minmax_str.substr(comma_pos + 1);
    
    // Remove trailing parenthesis
    if (!max_str.empty() && max_str.back() == ')') {
        max_str.pop_back();
    }
    
    // Trim whitespace
    min_str.erase(0, min_str.find_first_not_of(" \t"));
    min_str.erase(min_str.find_last_not_of(" \t") + 1);
    max_str.erase(0, max_str.find_first_not_of(" \t"));
    max_str.erase(max_str.find_last_not_of(" \t") + 1);
    
    return SizeValue::minmax(parse_single_grid_value(min_str), parse_single_grid_value(max_str));
}

std::vector<SizeValue> WidgetStyle::parse_grid_template(const std::string& template_str) {
    std::vector<SizeValue> result;
    
    if (template_str.empty()) {
        return result;
    }
    
    // Handle named grid lines and tracks
    std::string cleaned_str = template_str;
    
    // For now, simple tokenizer - more complex parsing for named lines can be added later
    std::istringstream iss(cleaned_str);
    std::string token;
    
    while (iss >> token) {
        // Check for repeat() function
        if (token.substr(0, 7) == "repeat(") {
            // Find the closing parenthesis
            std::string full_repeat = token;
            int paren_count = 1;
            size_t pos = 7;
            
            while (paren_count > 0 && pos < full_repeat.size()) {
                if (full_repeat[pos] == '(') paren_count++;
                else if (full_repeat[pos] == ')') paren_count--;
                pos++;
            }
            
            // If we didn't find the closing paren in this token, read more
            while (paren_count > 0 && iss >> token) {
                full_repeat += " " + token;
                for (char c : token) {
                    if (c == '(') paren_count++;
                    else if (c == ')') paren_count--;
                }
            }
            
            // Parse repeat function
            size_t comma_pos = full_repeat.find(',', 7);
            if (comma_pos != std::string::npos) {
                std::string count_str = full_repeat.substr(7, comma_pos - 7);
                std::string value_str = full_repeat.substr(comma_pos + 1);
                
                // Remove trailing parenthesis
                if (!value_str.empty() && value_str.back() == ')') {
                    value_str.pop_back();
                }
                
                // Trim whitespace
                count_str.erase(0, count_str.find_first_not_of(" \t"));
                count_str.erase(count_str.find_last_not_of(" \t") + 1);
                value_str.erase(0, value_str.find_first_not_of(" \t"));
                value_str.erase(value_str.find_last_not_of(" \t") + 1);
                
                // Check for auto-fill or auto-fit
                if (count_str == "auto-fill" || count_str == "auto-fit") {
                    // Store as a special flex value that will be resolved during layout
                    // We'll use negative values to indicate auto-fill (-1) and auto-fit (-2)
                    SizeValue auto_repeat = parse_single_grid_value(value_str);
                    auto_repeat.value = (count_str == "auto-fill") ? -1.0f : -2.0f;
                    result.push_back(auto_repeat);
                } else {
                    // Regular repeat with count
                    try {
                        int count = std::stoi(count_str);
                        
                        // Check if value_str contains minmax
                        if (value_str.find("minmax(") != std::string::npos) {
                            SizeValue value = parse_minmax(value_str);
                            for (int i = 0; i < count; ++i) {
                                result.push_back(value);
                            }
                        } else {
                            SizeValue value = parse_single_grid_value(value_str);
                            for (int i = 0; i < count; ++i) {
                                result.push_back(value);
                            }
                        }
                    } catch (...) {
                        // Invalid count
                    }
                }
            }
        } else if (token.substr(0, 7) == "minmax(") {
            // Parse minmax directly
            std::string full_minmax = token;
            int paren_count = 1;
            size_t pos = 7;
            
            while (paren_count > 0 && pos < full_minmax.size()) {
                if (full_minmax[pos] == '(') paren_count++;
                else if (full_minmax[pos] == ')') paren_count--;
                pos++;
            }
            
            // If we didn't find the closing paren in this token, read more
            while (paren_count > 0 && iss >> token) {
                full_minmax += " " + token;
                for (char c : token) {
                    if (c == '(') paren_count++;
                    else if (c == ')') paren_count--;
                }
            }
            
            result.push_back(parse_minmax(full_minmax));
        } else {
            // Parse single value
            result.push_back(parse_single_grid_value(token));
        }
    }
    
    return result;
}

void WidgetStyle::parse_grid_areas() {
    grid_area_map.clear();
    
    if (grid_template_areas.empty()) {
        return;
    }
    
    // Map to track area start positions
    std::map<std::string, std::pair<int, int>> area_starts;  // area_name -> (row, col)
    
    int row = 0;
    for (const auto& row_str : grid_template_areas) {
        std::istringstream iss(row_str);
        std::string area;
        int col = 0;
        
        while (iss >> area) {
            if (area != ".") {  // "." means empty cell
                auto it = area_starts.find(area);
                if (it == area_starts.end()) {
                    // First occurrence of this area
                    area_starts[area] = {row + 1, col + 1};  // 1-based indexing
                    grid_area_map[area] = {row + 1, col + 1, row + 2, col + 2};
                } else {
                    // Extend existing area
                    auto& bounds = grid_area_map[area];
                    bounds[2] = std::max(bounds[2], row + 2);  // row_end
                    bounds[3] = std::max(bounds[3], col + 2);  // col_end
                }
            }
            col++;
        }
        row++;
    }
}

} // namespace voxel_canvas