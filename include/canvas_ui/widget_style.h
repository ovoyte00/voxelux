/*
 * Copyright (C) 2024 Voxelux
 * 
 * Widget Style System - CSS-like styling for Canvas UI
 */

#pragma once

#include "canvas_core.h"
#include "glad/gl.h"
#include <optional>
#include <string>
#include <memory>
#include <variant>
#include <map>
#include <array>
#include <vector>

namespace voxel_canvas {

// Forward declarations
class ScaledTheme;
class WidgetStyle;

/**
 * Spacing value that can be either pixels or a theme reference
 */
struct SpacingValue {
    enum Type {
        Pixels,          // Raw pixel value (will be scaled)
        ThemeSpacing,    // Reference to theme spacing (xs, sm, md, lg, xl)
        Auto,            // Automatic spacing
        Inherit          // Inherit from parent
    };
    
    Type type = ThemeSpacing;
    float value = 2.0f;  // Default to md (index 2)
    
    SpacingValue() = default;
    SpacingValue(float pixels) : type(Pixels), value(pixels) {}
    SpacingValue(Type t, float v = 0) : type(t), value(v) {}
    
    static SpacingValue theme_xs() { return SpacingValue(ThemeSpacing, 0); }
    static SpacingValue theme_sm() { return SpacingValue(ThemeSpacing, 1); }
    static SpacingValue theme_md() { return SpacingValue(ThemeSpacing, 2); }
    static SpacingValue theme_lg() { return SpacingValue(ThemeSpacing, 3); }
    static SpacingValue theme_xl() { return SpacingValue(ThemeSpacing, 4); }
    static SpacingValue auto_spacing() { return SpacingValue(Auto); }
    static SpacingValue inherit() { return SpacingValue(Inherit); }
    
    // Resolve to actual pixels using theme
    float resolve(const ScaledTheme& theme) const;
    
    // Comparison operators
    bool operator==(const SpacingValue& other) const {
        return type == other.type && value == other.value;
    }
    bool operator!=(const SpacingValue& other) const {
        return !(*this == other);
    }
};

/**
 * Box model spacing (top, right, bottom, left)
 */
struct BoxSpacing {
    SpacingValue top, right, bottom, left;
    
    BoxSpacing() = default;
    BoxSpacing(const SpacingValue& all) 
        : top(all), right(all), bottom(all), left(all) {}
    BoxSpacing(const SpacingValue& vertical, const SpacingValue& horizontal)
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    BoxSpacing(const SpacingValue& t, const SpacingValue& r, const SpacingValue& b, const SpacingValue& l)
        : top(t), right(r), bottom(b), left(l) {}
    
    // Convenience factory methods
    static BoxSpacing all(float pixels) { 
        return BoxSpacing(SpacingValue(pixels)); 
    }
    static BoxSpacing theme(int level = 2) { 
        return BoxSpacing(SpacingValue(SpacingValue::ThemeSpacing, level)); 
    }
    static BoxSpacing zero() { 
        return BoxSpacing(SpacingValue(0.0f)); 
    }
    static BoxSpacing auto_all() {
        return BoxSpacing(SpacingValue::auto_spacing());
    }
    static BoxSpacing auto_horizontal() {
        return BoxSpacing(SpacingValue(0.0f), SpacingValue::auto_spacing());
    }
    
    // Check if any side is auto
    bool has_auto() const {
        return top.type == SpacingValue::Auto || right.type == SpacingValue::Auto ||
               bottom.type == SpacingValue::Auto || left.type == SpacingValue::Auto;
    }
    bool is_left_auto() const { return left.type == SpacingValue::Auto; }
    bool is_right_auto() const { return right.type == SpacingValue::Auto; }
    bool is_top_auto() const { return top.type == SpacingValue::Auto; }
    bool is_bottom_auto() const { return bottom.type == SpacingValue::Auto; }
    
    // Comparison operators
    bool operator==(const BoxSpacing& other) const {
        return top == other.top && right == other.right && 
               bottom == other.bottom && left == other.left;
    }
    bool operator!=(const BoxSpacing& other) const {
        return !(*this == other);
    }
};

/**
 * Size value that can be pixels, percentage, auto, or content-based
 */
struct SizeValue {
    enum Unit {
        Pixels,          // Fixed pixel size (will be scaled)
        Percent,         // Percentage of parent
        Auto,            // Size based on content
        FitContent,      // Size to fit content with constraints
        Flex,            // Flex grow/shrink value (fr units)
        Inherit,         // Inherit from parent
        MinMax,          // minmax(min, max) function
        MaxContent,      // Maximum content size
        MinContent       // Minimum content size
    };
    
    Unit unit = Auto;
    float value = 0;
    
    // For minmax() function
    std::unique_ptr<SizeValue> min_value;
    std::unique_ptr<SizeValue> max_value;
    
    SizeValue() = default;
    SizeValue(float pixels) : unit(Pixels), value(pixels) {}
    SizeValue(Unit u, float v = 0) : unit(u), value(v) {}
    
    // Copy constructor
    SizeValue(const SizeValue& other) : unit(other.unit), value(other.value) {
        if (other.min_value) {
            min_value = std::make_unique<SizeValue>(*other.min_value);
        }
        if (other.max_value) {
            max_value = std::make_unique<SizeValue>(*other.max_value);
        }
    }
    
    // Move constructor
    SizeValue(SizeValue&& other) noexcept = default;
    
    // Copy assignment
    SizeValue& operator=(const SizeValue& other) {
        if (this != &other) {
            unit = other.unit;
            value = other.value;
            min_value = other.min_value ? std::make_unique<SizeValue>(*other.min_value) : nullptr;
            max_value = other.max_value ? std::make_unique<SizeValue>(*other.max_value) : nullptr;
        }
        return *this;
    }
    
    // Move assignment
    SizeValue& operator=(SizeValue&& other) noexcept = default;
    
    static SizeValue pixels(float px) { return SizeValue(px); }
    static SizeValue percent(float pct) { return SizeValue(Percent, pct); }
    static SizeValue auto_size() { return SizeValue(Auto); }
    static SizeValue fit_content() { return SizeValue(FitContent); }
    static SizeValue flex(float factor = 1.0f) { return SizeValue(Flex, factor); }
    static SizeValue inherit() { return SizeValue(Inherit); }
    static SizeValue max_content() { return SizeValue(MaxContent); }
    static SizeValue min_content() { return SizeValue(MinContent); }
    
    static SizeValue minmax(const SizeValue& min, const SizeValue& max) {
        SizeValue result(MinMax);
        result.min_value = std::make_unique<SizeValue>(min);
        result.max_value = std::make_unique<SizeValue>(max);
        return result;
    }
    
    // Resolve to actual pixels
    float resolve(float parent_size, float content_size, const ScaledTheme& theme) const;
    
    // Comparison operators
    bool operator==(const SizeValue& other) const {
        if (unit != other.unit || value != other.value) return false;
        if ((min_value == nullptr) != (other.min_value == nullptr)) return false;
        if (min_value && *min_value != *other.min_value) return false;
        if ((max_value == nullptr) != (other.max_value == nullptr)) return false;
        if (max_value && *max_value != *other.max_value) return false;
        return true;
    }
    bool operator!=(const SizeValue& other) const {
        return !(*this == other);
    }
};

/**
 * Color value that can be a direct color or theme reference
 */
struct ColorValue {
    enum Type {
        Direct,          // Direct RGBA color
        ThemeColor,      // Reference to theme color
        Inherit          // Inherit from parent
    };
    
    Type type = ThemeColor;
    ColorRGBA color;
    std::string theme_ref = "gray_3";  // Default theme color
    
    ColorValue() = default;
    ColorValue(const ColorRGBA& c) : type(Direct), color(c) {}
    ColorValue(const std::string& theme_name) : type(ThemeColor), theme_ref(theme_name) {}
    
    static ColorValue inherit() { return ColorValue(); }
    
    // Resolve to actual color
    ColorRGBA resolve(const ScaledTheme& theme) const;
    
    // Comparison operators
    bool operator==(const ColorValue& other) const {
        if (type != other.type) return false;
        if (type == Direct) return color == other.color;
        if (type == ThemeColor) return theme_ref == other.theme_ref;
        return true;  // Both are Inherit
    }
    bool operator!=(const ColorValue& other) const {
        return !(*this == other);
    }
};

/**
 * Border style definition
 */
struct BorderStyle {
    enum Style {
        None,
        Solid,
        Dashed,
        Dotted,
        Double
    };
    
    SpacingValue width = SpacingValue(0.0f);  // Default to no border
    ColorValue color = ColorValue("gray_5");
    SpacingValue radius = SpacingValue(0.0f);
    Style style = Solid;
    
    // Per-side border widths (optional)
    std::optional<SpacingValue> width_top, width_right, width_bottom, width_left;
    
    // Per-corner radii (optional)
    std::optional<SpacingValue> radius_top_left, radius_top_right;
    std::optional<SpacingValue> radius_bottom_left, radius_bottom_right;
    
    // Per-side colors (optional)
    std::optional<ColorValue> color_top, color_right, color_bottom, color_left;
    
    // Border gradient support added as member of WidgetStyle
    // (see border_gradient member below)
    
    // Comparison operators
    bool operator==(const BorderStyle& other) const {
        return width == other.width && color == other.color && 
               radius == other.radius && style == other.style &&
               width_top == other.width_top && width_right == other.width_right &&
               width_bottom == other.width_bottom && width_left == other.width_left &&
               radius_top_left == other.radius_top_left && radius_top_right == other.radius_top_right &&
               radius_bottom_left == other.radius_bottom_left && radius_bottom_right == other.radius_bottom_right &&
               color_top == other.color_top && color_right == other.color_right &&
               color_bottom == other.color_bottom && color_left == other.color_left;
    }
    bool operator!=(const BorderStyle& other) const {
        return !(*this == other);
    }
};

/**
 * Shadow definition
 */
struct ShadowStyle {
    float offset_x = 0;
    float offset_y = 0;
    float blur_radius = 0;  // No shadow by default
    float spread = 0;
    ColorValue color = ColorValue(ColorRGBA(0, 0, 0, 0.2f));
    bool inset = false;
    
    // Comparison operators
    bool operator==(const ShadowStyle& other) const {
        return offset_x == other.offset_x && offset_y == other.offset_y &&
               blur_radius == other.blur_radius && spread == other.spread &&
               color == other.color && inset == other.inset;
    }
    bool operator!=(const ShadowStyle& other) const {
        return !(*this == other);
    }
};

/**
 * Complete widget style definition
 */
class WidgetStyle {
public:
    // Display and Layout
    enum class Display {
        None,            // Don't display
        Block,           // Stack vertically
        Inline,          // Flow horizontally
        InlineBlock,     // Inline with block properties
        Flex,            // Flexible box layout
        Grid,            // Grid layout
    };
    
    enum class Position {
        Static,          // Normal flow
        Relative,        // Relative to normal position
        Absolute,        // Absolute positioning
        Fixed,           // Fixed to viewport
        Sticky           // Sticky positioning
    };
    
    enum class FlexDirection {
        Row,
        RowReverse,
        Column,
        ColumnReverse
    };
    
    enum class AlignItems {
        Start,
        Center,
        End,
        Stretch,
        Baseline
    };
    
    // AlignSelf - per-item override of parent's align-items
    enum class AlignSelf {
        Auto,      // Use parent's align-items value
        Start,
        Center,
        End,
        Stretch,
        Baseline
    };
    
    // AlignContent - for multi-line flex containers
    enum class AlignContent {
        Start,
        Center,
        End,
        Stretch,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    };
    
    enum class JustifyContent {
        Start,
        Center,
        End,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    };
    
    // JustifyItems - for grid item alignment on main axis
    enum class JustifyItems {
        Start,
        Center,
        End,
        Stretch
    };
    
    enum class FlexWrap {
        NoWrap,
        Wrap,
        WrapReverse
    };
    
    enum class TextAlign {
        Left,
        Center,
        Right,
        Justify
    };
    
    enum class VerticalAlign {
        Top,
        Middle,
        Bottom,
        Baseline
    };
    
    enum class Overflow {
        Visible,
        Hidden,
        Scroll,
        Auto
    };
    
    enum class PointerEvents {
        Auto,    // Normal behavior - element can be target of pointer events
        None     // Element is never the target of pointer events
    };
    
    // Visibility - different from display:none
    enum class Visibility {
        Visible,
        Hidden,     // Takes up space but not visible
        Collapse    // For table rows/columns
    };
    
    // User selection behavior
    enum class UserSelect {
        Auto,
        None,       // Prevent selection
        Text,       // Allow text selection
        All         // Select all on click
    };
    
    // White space handling
    enum class WhiteSpace {
        Normal,     // Collapse whitespace, wrap
        NoWrap,     // Collapse whitespace, no wrap
        Pre,        // Preserve whitespace, no wrap
        PreWrap,    // Preserve whitespace, wrap
        PreLine     // Preserve line breaks
    };
    
    // Word breaking
    enum class WordBreak {
        Normal,     // Default breaking
        BreakAll,   // Break anywhere
        KeepAll,    // Don't break words
        BreakWord   // Break long words
    };
    
    // Text overflow
    enum class TextOverflow {
        Clip,       // Cut off text
        Ellipsis    // Show ...
    };
    
    // Transform data
    struct Transform {
        float translate_x = 0;
        float translate_y = 0;
        float scale_x = 1;
        float scale_y = 1;
        float rotate = 0;  // Degrees
        float skew_x = 0;
        float skew_y = 0;
    };
    
    // Gradient support - moved here before BorderStyle that uses it
    struct GradientStop {
        ColorValue color;
        float position = 0.0f;  // 0.0 to 1.0 (can use percentages)
        
        GradientStop() = default;
        GradientStop(const ColorValue& c, float p) : color(c), position(p) {}
    };
    
    struct Gradient {
        enum Type {
            None,
            Linear,
            Radial,
            Conic    // Conic/angular gradient
        };
        Type type = None;
        
        // Linear gradient properties
        float angle = 180.0f;  // Degrees (180 = top to bottom)
        
        // Radial gradient properties
        enum Shape {
            Circle,
            Ellipse
        };
        Shape shape = Ellipse;
        
        enum RadialSize {
            ClosestSide,
            FarthestSide,
            ClosestCorner,
            FarthestCorner,
            CustomSize
        };
        RadialSize radial_size = FarthestCorner;
        SizeValue radius_x = SizeValue::percent(50);
        SizeValue radius_y = SizeValue::percent(50);
        
        // Position for radial/conic gradients
        SizeValue center_x = SizeValue::percent(50);
        SizeValue center_y = SizeValue::percent(50);
        
        // Conic gradient start angle
        float start_angle = 0.0f;
        
        // Color stops
        std::vector<GradientStop> stops;
        
        // Helper constructors
        static Gradient linear(float angle_deg) {
            Gradient g;
            g.type = Linear;
            g.angle = angle_deg;
            return g;
        }
        
        static Gradient radial(Shape s = Ellipse) {
            Gradient g;
            g.type = Radial;
            g.shape = s;
            return g;
        }
        
        static Gradient conic(float start_deg = 0) {
            Gradient g;
            g.type = Conic;
            g.start_angle = start_deg;
            return g;
        }
        
        Gradient& add_stop(const ColorValue& color, float position) {
            stops.emplace_back(color, position);
            return *this;
        }
        
        bool is_set() const { return type != None && stops.size() >= 2; }
    };
    
    // Transform origin
    struct TransformOrigin {
        float x = 0.5f;  // 0-1 (0.5 = center)
        float y = 0.5f;  // 0-1 (0.5 = center)
    };
    
    // Outline style (similar to border but doesn't affect layout)
    struct OutlineStyle {
        SpacingValue width = SpacingValue(0.0f);
        ColorValue color = ColorValue("accent_primary");
        SpacingValue offset = SpacingValue(0.0f);  // Additional offset from the stroke position
        
        enum Style {
            None,
            Solid,
            Dashed,
            Dotted
        };
        Style style = None;
        
        // Stroke alignment (like Illustrator, not available in CSS)
        enum Alignment {
            Outside,  // Stroke is entirely outside the boundary (CSS default)
            Center,   // Stroke is centered on the boundary (Illustrator default)
            Inside    // Stroke is entirely inside the boundary
        };
        Alignment alignment = Outside;  // Default to CSS-like behavior
        
        // Outline gradient support (note: outline_gradient member added to WidgetStyle)
    };
    
    // Display and positioning
    Display display = Display::Block;
    Position position = Position::Static;
    
    // Box model
    BoxSpacing padding = BoxSpacing::zero();
    BoxSpacing margin = BoxSpacing::zero();
    BorderStyle border;
    Gradient border_gradient;  // Gradient for border
    
    // Sizing
    SizeValue width = SizeValue::auto_size();
    SizeValue height = SizeValue::auto_size();
    SizeValue min_width = SizeValue(0.0f);
    SizeValue max_width = SizeValue(SizeValue::Pixels, INFINITY);
    SizeValue min_height = SizeValue(0.0f);
    SizeValue max_height = SizeValue(SizeValue::Pixels, INFINITY);
    
    // Flexbox properties
    FlexDirection flex_direction = FlexDirection::Row;
    AlignItems align_items = AlignItems::Stretch;
    AlignSelf align_self = AlignSelf::Auto;  // Per-item alignment override
    AlignContent align_content = AlignContent::Stretch;  // Multi-line alignment
    JustifyContent justify_content = JustifyContent::Start;
    JustifyItems justify_items = JustifyItems::Stretch;  // For grid items
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    SpacingValue gap = SpacingValue(0.0f);  // Shorthand for row-gap and column-gap
    SpacingValue row_gap = SpacingValue(0.0f);  // Gap between rows (cross-axis)
    SpacingValue column_gap = SpacingValue(0.0f);  // Gap between columns (main-axis)
    float flex_grow = 0;
    float flex_shrink = 1;
    SizeValue flex_basis = SizeValue::auto_size();
    
    // Grid container properties
    std::vector<SizeValue> grid_template_columns;  // Track sizes for columns
    std::vector<SizeValue> grid_template_rows;     // Track sizes for rows
    SizeValue grid_auto_columns = SizeValue::auto_size();  // Size for implicit columns
    SizeValue grid_auto_rows = SizeValue::auto_size();     // Size for implicit rows
    
    // Named grid lines (e.g., [header-start] 100px [header-end content-start])
    std::vector<std::vector<std::string>> grid_column_line_names;  // Names for column lines
    std::vector<std::vector<std::string>> grid_row_line_names;     // Names for row lines
    
    // Grid template areas
    std::vector<std::string> grid_template_areas;  // Array of area strings
    std::map<std::string, std::array<int, 4>> grid_area_map;  // area_name -> [row_start, col_start, row_end, col_end]
    
    // Grid item placement properties
    int grid_column_start = 0;  // 0 = auto
    int grid_column_end = 0;    // 0 = auto
    int grid_row_start = 0;     // 0 = auto
    int grid_row_end = 0;       // 0 = auto
    int grid_column_span = 1;   // Shorthand for span
    int grid_row_span = 1;      // Shorthand for span
    std::string grid_area;      // Named grid area (e.g., "header", "sidebar")
    
    // Grid auto-placement
    enum class GridAutoFlow {
        Row,         // Fill row by row
        Column,      // Fill column by column
        RowDense,    // Row with dense packing
        ColumnDense  // Column with dense packing
    };
    GridAutoFlow grid_auto_flow = GridAutoFlow::Row;
    
    // Grid alignment (uses existing properties)
    // align_items - aligns items within their grid area (cross axis)
    // justify_items - aligns items within their grid area (main axis)
    // align_content - aligns the grid within container (cross axis)
    // justify_content - aligns the grid within container (main axis)
    
    // Background image properties
    struct BackgroundImage {
        std::string url;  // Path or resource ID
        GLuint texture_id = 0;  // Cached texture ID
        
        enum Repeat {
            NoRepeat,    // Don't repeat
            RepeatX,     // Repeat horizontally
            RepeatY,     // Repeat vertically
            RepeatBoth,  // Repeat both directions
            Space,       // Repeat with space between
            Round        // Repeat and scale to fit
        };
        Repeat repeat = NoRepeat;
        
        enum Size {
            Auto,        // Natural size
            Cover,       // Cover entire area
            Contain,     // Fit within area
            Custom       // Use size_x and size_y
        };
        Size size = Auto;
        SizeValue size_x = SizeValue::auto_size();
        SizeValue size_y = SizeValue::auto_size();
        
        enum Position {
            TopLeft, TopCenter, TopRight,
            CenterLeft, Center, CenterRight,
            BottomLeft, BottomCenter, BottomRight,
            CustomPosition  // Use position_x and position_y
        };
        Position position = Center;
        SizeValue position_x = SizeValue::percent(50);
        SizeValue position_y = SizeValue::percent(50);
        
        enum Attachment {
            Scroll,  // Scroll with content
            Fixed,   // Fixed to viewport
            Local    // Fixed to element
        };
        Attachment attachment = Scroll;
        
        bool is_set() const { return !url.empty() || texture_id != 0; }
    };
    
    // Colors and appearance
    ColorValue background = ColorValue("transparent");
    Gradient background_gradient;  // Gradient background
    BackgroundImage background_image;
    std::vector<BackgroundImage> background_images;  // Multiple backgrounds
    std::vector<Gradient> background_gradients;  // Multiple gradients
    ColorValue text_color = ColorValue("gray_1");
    float opacity = 1.0f;
    Visibility visibility = Visibility::Visible;
    ShadowStyle box_shadow;
    OutlineStyle outline;
    Gradient outline_gradient;  // Gradient for outline
    
    // Typography
    std::string font_family = "Inter";
    SpacingValue font_size = SpacingValue::theme_md();
    int font_weight = 400;  // 100-900
    TextAlign text_align = TextAlign::Left;
    VerticalAlign vertical_align = VerticalAlign::Baseline;
    float line_height = 1.5f;  // Multiplier
    SpacingValue letter_spacing = SpacingValue(0.0f);
    WhiteSpace white_space = WhiteSpace::Normal;
    WordBreak word_break = WordBreak::Normal;
    TextOverflow text_overflow = TextOverflow::Clip;
    
    // Effects (multiple shadows support)
    std::optional<std::vector<ShadowStyle>> box_shadows;  // Multiple shadows
    
    // Overflow
    Overflow overflow_x = Overflow::Visible;
    Overflow overflow_y = Overflow::Visible;
    
    // Absolute positioning (when position != Static)
    std::optional<SpacingValue> top, right, bottom, left;
    
    // Z-index for stacking
    std::optional<int> z_index;
    
    // Transform
    Transform transform;
    TransformOrigin transform_origin;
    
    // Cursor and interaction
    std::string cursor = "default";  // default, pointer, text, etc.
    PointerEvents pointer_events = PointerEvents::Auto;
    UserSelect user_select = UserSelect::Auto;
    
    // Transitions (future)
    // float transition_duration = 0;
    // std::string transition_property = "all";
    // std::string transition_timing = "ease";
    
    // State variants
    std::unique_ptr<WidgetStyle> hover_style;
    std::unique_ptr<WidgetStyle> active_style;
    std::unique_ptr<WidgetStyle> focus_style;
    std::unique_ptr<WidgetStyle> disabled_style;
    
    // Methods
    WidgetStyle() = default;
    WidgetStyle(const WidgetStyle& other);
    WidgetStyle& operator=(const WidgetStyle& other);
    WidgetStyle(WidgetStyle&&) = default;
    WidgetStyle& operator=(WidgetStyle&&) = default;
    
    // Merge another style into this one (for cascading)
    void merge(const WidgetStyle& other);
    
    // Apply state variant
    void apply_state(const WidgetStyle* state_style);
    
    // Compute final values using theme
    struct ComputedStyle {
        // Unit types for tracking auto/percent/etc
        enum SizeUnit {
            PIXELS,
            PERCENT,
            AUTO,
            FIT_CONTENT,
            FLEX
        };
        
        // Resolved pixel values
        float padding_top, padding_right, padding_bottom, padding_left;
        float margin_top, margin_right, margin_bottom, margin_left;
        float border_width_top, border_width_right, border_width_bottom, border_width_left;
        float border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
        ColorRGBA border_color;
        float width, height, min_width, max_width, min_height, max_height;
        SizeUnit width_unit = AUTO;
        SizeUnit height_unit = AUTO;
        ColorRGBA background_color;
        ColorRGBA text_color_rgba;
        float font_size_pixels;
        float gap_pixels;  // Deprecated - use row_gap_pixels and column_gap_pixels
        float row_gap_pixels;
        float column_gap_pixels;
        float letter_spacing_pixels;
        
        // Copy of non-resolved values
        Display display;
        Position position;
        FlexDirection flex_direction;
        AlignItems align_items;
        AlignSelf align_self;
        AlignContent align_content;
        JustifyContent justify_content;
        JustifyItems justify_items;
        FlexWrap flex_wrap;
        
        // Grid properties (computed)
        std::vector<SizeValue> grid_template_columns;  // Grid template definitions
        std::vector<SizeValue> grid_template_rows;     // Grid template definitions
        std::vector<float> grid_column_sizes;  // Computed column widths
        std::vector<float> grid_row_sizes;     // Computed row heights
        int grid_column_start, grid_column_end;
        int grid_row_start, grid_row_end;
        GridAutoFlow grid_auto_flow;
        std::string grid_area;  // Named grid area
        std::map<std::string, std::array<int, 4>> grid_area_map;  // Container's area mapping
        float flex_grow, flex_shrink;
        TextAlign text_align;
        VerticalAlign vertical_align;
        float line_height;
        int font_weight;
        std::string font_family;
        float opacity;
        Overflow overflow_x, overflow_y;
        
        // New properties
        Visibility visibility;
        UserSelect user_select;
        WhiteSpace white_space;
        WordBreak word_break;
        TextOverflow text_overflow;
        Transform transform;
        TransformOrigin transform_origin;
        OutlineStyle outline;
        Gradient outline_gradient;
        ShadowStyle box_shadow;
        BackgroundImage background_image;
        std::vector<BackgroundImage> background_images;
        Gradient background_gradient;
        std::vector<Gradient> background_gradients;
        Gradient border_gradient;
        
        // Absolute positioning (if applicable)
        std::optional<float> pos_top, pos_right, pos_bottom, pos_left;
        std::optional<int> z_index;
    };
    
    ComputedStyle compute(const ScaledTheme& theme, 
                         const Rect2D& parent_bounds = Rect2D(),
                         const Point2D& content_size = Point2D()) const;
    
    // Builder pattern for easy construction
    WidgetStyle& set_display(Display d) { display = d; return *this; }
    WidgetStyle& set_padding(const BoxSpacing& p) { padding = p; return *this; }
    WidgetStyle& set_margin(const BoxSpacing& m) { margin = m; return *this; }
    WidgetStyle& set_background(const ColorValue& bg) { background = bg; return *this; }
    WidgetStyle& set_background_gradient(const Gradient& grad) { background_gradient = grad; return *this; }
    WidgetStyle& set_background_image(const BackgroundImage& img) { background_image = img; return *this; }
    
    // Helper for linear gradient
    WidgetStyle& set_linear_gradient(float angle, const std::vector<GradientStop>& stops) {
        background_gradient = Gradient::linear(angle);
        background_gradient.stops = stops;
        return *this;
    }
    
    // Helper for radial gradient
    WidgetStyle& set_radial_gradient(const std::vector<GradientStop>& stops, 
                                    Gradient::Shape shape = Gradient::Ellipse) {
        background_gradient = Gradient::radial(shape);
        background_gradient.stops = stops;
        return *this;
    }
    WidgetStyle& set_width(const SizeValue& w) { width = w; return *this; }
    WidgetStyle& set_height(const SizeValue& h) { height = h; return *this; }
    WidgetStyle& set_flex_direction(FlexDirection fd) { flex_direction = fd; return *this; }
    WidgetStyle& set_align_items(AlignItems ai) { align_items = ai; return *this; }
    WidgetStyle& set_align_self(AlignSelf as) { align_self = as; return *this; }
    WidgetStyle& set_align_content(AlignContent ac) { align_content = ac; return *this; }
    WidgetStyle& set_justify_content(JustifyContent jc) { justify_content = jc; return *this; }
    WidgetStyle& set_gap(const SpacingValue& g) { 
        gap = g; 
        row_gap = g;
        column_gap = g;
        return *this; 
    }
    WidgetStyle& set_gap(const SpacingValue& row, const SpacingValue& col) { 
        row_gap = row;
        column_gap = col;
        return *this; 
    }
    WidgetStyle& set_row_gap(const SpacingValue& g) { row_gap = g; return *this; }
    WidgetStyle& set_column_gap(const SpacingValue& g) { column_gap = g; return *this; }
    WidgetStyle& set_font_size(const SpacingValue& fs) { font_size = fs; return *this; }
    WidgetStyle& set_text_color(const ColorValue& tc) { text_color = tc; return *this; }
    WidgetStyle& set_font_weight(int fw) { font_weight = fw; return *this; }
    WidgetStyle& set_text_align(TextAlign ta) { text_align = ta; return *this; }
    WidgetStyle& set_flex_grow(float fg) { flex_grow = fg; return *this; }
    WidgetStyle& set_flex_shrink(float fs) { flex_shrink = fs; return *this; }
    
    // Grid setters
    WidgetStyle& set_grid_template_columns(const std::vector<SizeValue>& cols) { 
        grid_template_columns = cols; 
        return *this; 
    }
    WidgetStyle& set_grid_template_rows(const std::vector<SizeValue>& rows) { 
        grid_template_rows = rows; 
        return *this; 
    }
    WidgetStyle& set_grid_column(int start, int end) {
        grid_column_start = start;
        grid_column_end = end;
        return *this;
    }
    WidgetStyle& set_grid_row(int start, int end) {
        grid_row_start = start;
        grid_row_end = end;
        return *this;
    }
    WidgetStyle& set_grid_column_span(int span) { grid_column_span = span; return *this; }
    WidgetStyle& set_grid_row_span(int span) { grid_row_span = span; return *this; }
    WidgetStyle& set_grid_auto_flow(GridAutoFlow flow) { grid_auto_flow = flow; return *this; }
    WidgetStyle& set_justify_items(JustifyItems ji) { justify_items = ji; return *this; }
    
    // Set grid area for an item
    WidgetStyle& set_grid_area(const std::string& area) { 
        grid_area = area; 
        return *this; 
    }
    
    // Set grid template areas for container
    // Example: {"header header", "sidebar content", "footer footer"}
    WidgetStyle& set_grid_template_areas(const std::vector<std::string>& areas) {
        grid_template_areas = areas;
        parse_grid_areas();
        return *this;
    }
    
    // Parse grid areas to create mapping
    void parse_grid_areas();
    
    // Grid template helper methods
    WidgetStyle& set_grid_template(const std::string& columns_str, const std::string& rows_str = "") {
        grid_template_columns = parse_grid_template(columns_str);
        if (!rows_str.empty()) {
            grid_template_rows = parse_grid_template(rows_str);
        }
        return *this;
    }
    
    // Parse grid template string like "1fr 200px auto" or "repeat(3, 1fr)"
    static std::vector<SizeValue> parse_grid_template(const std::string& template_str);
    
    // Helper to create uniform grid
    WidgetStyle& set_grid_columns(int count, const SizeValue& size = SizeValue::flex(1)) {
        grid_template_columns.clear();
        for (int i = 0; i < count; ++i) {
            grid_template_columns.push_back(size);
        }
        return *this;
    }
    
    WidgetStyle& set_grid_rows(int count, const SizeValue& size = SizeValue::flex(1)) {
        grid_template_rows.clear();
        for (int i = 0; i < count; ++i) {
            grid_template_rows.push_back(size);
        }
        return *this;
    }
    
    WidgetStyle& set_position(Position p) { position = p; return *this; }
    WidgetStyle& set_border(const BorderStyle& b) { border = b; return *this; }
    WidgetStyle& set_border_radius(const SpacingValue& br) { border.radius = br; return *this; }
    WidgetStyle& set_opacity(float o) { opacity = o; return *this; }
    WidgetStyle& set_overflow(Overflow o) { overflow_x = overflow_y = o; return *this; }
    WidgetStyle& set_cursor(const std::string& c) { cursor = c; return *this; }
    WidgetStyle& set_z_index(int z) { z_index = z; return *this; }
    WidgetStyle& set_pointer_events(PointerEvents pe) { pointer_events = pe; return *this; }
};

} // namespace voxel_canvas