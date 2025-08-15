# Voxelux Widget Style System Guide

## Quick Reference - All Style Properties

### Layout Properties
- `display` - Block, Inline, InlineBlock, Flex, Grid, None
- `position` - Static, Relative, Absolute, Fixed, Sticky
- `width` / `height` - Pixels, percent, auto, fit-content
- `min_width` / `max_width` / `min_height` / `max_height` - Size constraints
- `padding` - Top, right, bottom, left spacing inside element
- `margin` - Top, right, bottom, left spacing outside element

### Flexbox Properties
- `flex_direction` - Row, RowReverse, Column, ColumnReverse
- `align_items` - Start, Center, End, Stretch, Baseline
- `justify_content` - Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly
- `flex_wrap` - NoWrap, Wrap, WrapReverse
- `flex_grow` - How much the item should grow (0-n)
- `flex_shrink` - How much the item should shrink (0-n)
- `gap` - Space between flex items

### Visual Properties
- `background` - Background color (theme color or RGBA)
- `opacity` - Transparency (0.0 - 1.0)
- `border` - Border width, color, and radius
- `border_radius` - Corner rounding
- `box_shadow` - Drop shadow effects

### Typography Properties
- `font_family` - Font face (default: "Inter")
- `font_size` - Text size in pixels or theme units
- `font_weight` - 100-900 (400=normal, 700=bold)
- `text_color` - Text color (theme color or RGBA)
- `text_align` - Left, Center, Right, Justify
- `vertical_align` - Top, Middle, Bottom, Baseline
- `line_height` - Line height multiplier (default: 1.5)
- `letter_spacing` - Space between characters

### Interaction Properties
- `cursor` - Mouse cursor type (default, pointer, text, etc.)
- `pointer_events` - Auto, None (whether element receives mouse events)
- `overflow_x` / `overflow_y` - Visible, Hidden, Scroll, Auto

### Positioning Properties (for absolute/fixed position)
- `top` / `right` / `bottom` / `left` - Distance from edges
- `z_index` - Stacking order

---

## Overview

The Voxelux Widget Style System provides a CSS-like styling API for UI components with automatic DPI scaling built in at the lowest level. Every pixel value is automatically scaled based on the display's content scale factor.

## Core Concepts

### 1. Style Values with Automatic DPI Scaling

All dimension values in the style system automatically scale with DPI:

```cpp
// These pixel values will be multiplied by content_scale (e.g., 2x for Retina)
widget->get_style()
    .set_width(SizeValue(100))      // 100px → 200px on 2x display
    .set_padding(BoxSpacing(8))     // 8px → 16px on 2x display
    .set_font_size(SpacingValue(14)); // 14px → 28px on 2x display
```

### 2. Value Types

#### SpacingValue
Used for padding, margins, gaps, font sizes, etc.
```cpp
SpacingValue(10)                    // 10 pixels (will be scaled)
SpacingValue::theme_xs()           // Extra small theme spacing
SpacingValue::theme_sm()           // Small theme spacing  
SpacingValue::theme_md()           // Medium theme spacing
SpacingValue::theme_lg()           // Large theme spacing
SpacingValue::theme_xl()           // Extra large theme spacing
SpacingValue::auto_value()         // Automatic spacing
```

#### SizeValue
Used for width, height, and other dimensions.
```cpp
SizeValue(100)                      // 100 pixels (will be scaled)
SizeValue::percent(50)              // 50% of parent
SizeValue::auto_size()              // Size based on content
SizeValue::fit_content()            // Fit to content
SizeValue::flex(1)                  // Flex unit for flex layouts
```

#### ColorValue
Used for colors throughout the system.
```cpp
ColorValue("gray_1")                // Theme color reference
ColorValue(ColorRGBA(1, 0, 0, 1))  // Direct RGBA color
ColorValue("transparent")           // Transparent
ColorValue("accent_primary")        // Primary accent color
```

#### BoxSpacing
Used for padding and margin with individual side control.
```cpp
BoxSpacing(10)                      // All sides
BoxSpacing(10, 20)                  // Vertical, horizontal
BoxSpacing(10, 20, 30, 40)         // Top, right, bottom, left
BoxSpacing::all(10)                 // All sides same value
BoxSpacing::symmetric(10, 20)      // Vertical, horizontal
BoxSpacing::theme()                 // Theme default spacing
BoxSpacing::zero()                  // No spacing
```

---

## Style Properties Reference

### Display and Layout

#### `display`
Controls how an element is displayed.
```cpp
widget->get_style().set_display(WidgetStyle::Display::Flex);
```
- `None` - Element is not displayed
- `Block` - Block-level element (stacks vertically)
- `Inline` - Inline element (flows horizontally)
- `InlineBlock` - Inline with block properties
- `Flex` - Flexible box layout
- `Grid` - Grid layout (future)

#### `position`
Controls positioning method.
```cpp
widget->get_style().set_position(WidgetStyle::Position::Absolute);
```
- `Static` - Normal document flow
- `Relative` - Relative to normal position
- `Absolute` - Positioned relative to parent
- `Fixed` - Fixed to viewport
- `Sticky` - Sticky positioning

### Size and Constraints

```cpp
widget->get_style()
    .set_width(SizeValue(200))          // Fixed width
    .set_height(SizeValue::percent(50)) // 50% of parent height
    .set_min_width(SizeValue(100))      // Minimum width
    .set_max_width(SizeValue(500))      // Maximum width
    .set_min_height(SizeValue(50))      // Minimum height
    .set_max_height(SizeValue::auto_size()); // No max height
```

### Spacing

```cpp
widget->get_style()
    .set_padding(BoxSpacing::all(10))        // 10px padding all sides
    .set_margin(BoxSpacing(5, 10, 15, 20));  // T, R, B, L margins
```

### Flexbox Layout

```cpp
container->get_style()
    .set_display(WidgetStyle::Display::Flex)
    .set_flex_direction(WidgetStyle::FlexDirection::Row)
    .set_align_items(WidgetStyle::AlignItems::Center)
    .set_justify_content(WidgetStyle::JustifyContent::SpaceBetween)
    .set_gap(SpacingValue::theme_md())
    .set_flex_wrap(WidgetStyle::FlexWrap::Wrap);

// Child items
child->get_style()
    .set_flex_grow(1)      // Take available space
    .set_flex_shrink(0);    // Don't shrink
```

#### Flex Direction Options
- `Row` - Items flow left to right
- `RowReverse` - Items flow right to left
- `Column` - Items flow top to bottom
- `ColumnReverse` - Items flow bottom to top

#### Align Items Options (cross-axis)
- `Start` - Align to start of cross axis
- `Center` - Center on cross axis
- `End` - Align to end of cross axis
- `Stretch` - Stretch to fill cross axis
- `Baseline` - Align text baselines

#### Justify Content Options (main axis)
- `Start` - Pack items at start
- `Center` - Center items
- `End` - Pack items at end
- `SpaceBetween` - Even space between items
- `SpaceAround` - Even space around items
- `SpaceEvenly` - Even space including edges

### Visual Styling

```cpp
widget->get_style()
    .set_background(ColorValue("gray_3"))
    .set_opacity(0.9f)
    .set_border(BorderStyle{
        .width = SpacingValue(1),
        .color = ColorValue("gray_5"),
        .radius = SpacingValue(4)
    })
    .set_border_radius(SpacingValue(8));

// Box shadow
widget->get_style().box_shadow = ShadowStyle{
    .offset_x = 0,
    .offset_y = 2,
    .blur_radius = 10,
    .spread = 0,
    .color = ColorValue(ColorRGBA(0, 0, 0, 0.2f))
};
```

### Typography

```cpp
text_widget->get_style()
    .set_font_family("Inter")
    .set_font_size(SpacingValue(14))
    .set_font_weight(700)  // Bold
    .set_text_color(ColorValue("gray_1"))
    .set_text_align(WidgetStyle::TextAlign::Center)
    .set_line_height(1.6f)
    .set_letter_spacing(SpacingValue(0.5f));
```

### Interaction

```cpp
widget->get_style()
    .set_cursor("pointer")  // Mouse cursor on hover
    .set_pointer_events(WidgetStyle::PointerEvents::None)  // Disable mouse events
    .set_overflow(WidgetStyle::Overflow::Hidden);  // Hide overflow content
```

### Absolute Positioning

```cpp
widget->get_style()
    .set_position(WidgetStyle::Position::Absolute)
    .set_top(SpacingValue(10))
    .set_left(SpacingValue(20))
    .set_z_index(100);  // Stack above other elements
```

---

## State-Based Styling

Widgets can have different styles for different states:

```cpp
button->get_style().hover_style = std::make_unique<WidgetStyle>();
button->get_style().hover_style->background = ColorValue("gray_2");

button->get_style().active_style = std::make_unique<WidgetStyle>();
button->get_style().active_style->background = ColorValue("accent_primary");

button->get_style().disabled_style = std::make_unique<WidgetStyle>();
button->get_style().disabled_style->opacity = 0.5f;
```

---

## Theme Colors

The system provides semantic color references that adapt to light/dark themes:

### Gray Scale
- `gray_1` - Darkest (usually text)
- `gray_2` - Dark
- `gray_3` - Medium dark
- `gray_4` - Medium
- `gray_5` - Medium light
- `gray_6` - Light
- `gray_7` - Lightest (usually background)

### Semantic Colors
- `accent_primary` - Primary brand color
- `accent_secondary` - Secondary accent
- `accent_success` - Success/positive state
- `accent_warning` - Warning state
- `accent_error` - Error/danger state
- `transparent` - Fully transparent

---

## Common Patterns

### Centered Content
```cpp
container->get_style()
    .set_display(WidgetStyle::Display::Flex)
    .set_align_items(WidgetStyle::AlignItems::Center)
    .set_justify_content(WidgetStyle::JustifyContent::Center);
```

### Sidebar Layout
```cpp
sidebar->get_style()
    .set_width(SizeValue(250))
    .set_height(SizeValue::percent(100))
    .set_background(ColorValue("gray_5"));

content->get_style()
    .set_flex_grow(1)  // Take remaining space
    .set_padding(BoxSpacing::theme());
```

### Card/Panel
```cpp
card->get_style()
    .set_background(ColorValue("gray_4"))
    .set_padding(BoxSpacing::theme())
    .set_border_radius(SpacingValue(8))
    .set_box_shadow(ShadowStyle{
        .offset_y = 2,
        .blur_radius = 8,
        .color = ColorValue(ColorRGBA(0, 0, 0, 0.1f))
    });
```

### Responsive Text
```cpp
// Text automatically scales with DPI
text->get_style()
    .set_font_size(SpacingValue(14));  // 14px logical, scales to physical pixels
```

---

## DPI Scaling

All pixel values are automatically scaled based on the display's content scale:

1. **Automatic Scaling**: Every pixel value (width, height, padding, font-size, etc.) is multiplied by the content scale factor
2. **Content Scale**: Obtained from the window system (e.g., 2.0 for Retina displays)
3. **No Manual Calculation**: Developers specify logical pixels; the system handles physical pixels

Example on a 2x Retina display:
```cpp
widget->get_style()
    .set_width(SizeValue(100))      // Renders at 200 physical pixels
    .set_font_size(SpacingValue(14)); // Renders at 28 physical pixels
```

---

## Widget Creation with Styles

### Using LayoutBuilder
```cpp
auto row = LayoutBuilder::row()
    ->with_style([](WidgetStyle& style) {
        style.set_gap(SpacingValue::theme_md())
             .set_padding(BoxSpacing::theme())
             .set_background(ColorValue("gray_4"));
    })
    ->add_child(create_widget<Text>("Label"))
    ->add_child(create_widget<Button>("Click Me"));
```

### Direct Style Modification
```cpp
auto button = create_widget<Button>("Submit");
button->get_style()
    .set_background(ColorValue("accent_primary"))
    .set_padding(BoxSpacing(8, 16))
    .set_border_radius(SpacingValue(4));
```

---

## Performance Considerations

1. **Style Computation**: Styles are computed once per frame when needed
2. **Invalidation**: Use `invalidate_style()` when changing styles dynamically
3. **Inherited Values**: Some properties inherit from parent, reducing redundancy
4. **State Styles**: Hover/active states only compute when state changes

---

## Migration from Old System

| Old System | New Style System |
|------------|------------------|
| `set_size(w, h)` | `.set_width(SizeValue(w)).set_height(SizeValue(h))` |
| `set_pos(x, y)` | `.set_position(Absolute).set_left(x).set_top(y)` |
| Manual DPI multiply | Automatic scaling built-in |
| Fixed layouts | Flexbox layouts |
| Manual hover handling | `.hover_style` property |

---

## Best Practices

1. **Use Theme Values**: Prefer `SpacingValue::theme_*()` over hardcoded pixels
2. **Flexbox First**: Use flexbox for layouts instead of absolute positioning
3. **Semantic Colors**: Use theme colors (`gray_1`, etc.) instead of hardcoded RGBA
4. **Let Content Size**: Use `auto_size()` and `fit_content()` when possible
5. **Consistent Spacing**: Use `BoxSpacing::theme()` for consistent padding
6. **State Styles**: Define hover/active states for interactive elements
7. **Avoid Fixed Sizes**: Use relative units (percent, flex) for responsive layouts

---

## Examples

### Complete Button Style
```cpp
auto button = create_widget<Button>("Save Changes");
button->get_style()
    // Layout
    .set_display(WidgetStyle::Display::InlineBlock)
    .set_padding(BoxSpacing(8, 16))
    
    // Visual
    .set_background(ColorValue("accent_primary"))
    .set_border_radius(SpacingValue(4))
    .set_cursor("pointer")
    
    // Typography
    .set_font_size(SpacingValue(14))
    .set_font_weight(600)
    .set_text_color(ColorValue("white"));

// Hover state
button->get_style().hover_style = std::make_unique<WidgetStyle>();
button->get_style().hover_style->background = ColorValue("accent_primary_dark");

// Active state
button->get_style().active_style = std::make_unique<WidgetStyle>();
button->get_style().active_style->transform = "scale(0.98)";
```

### Responsive Layout
```cpp
auto container = LayoutBuilder::row()
    ->with_style([](WidgetStyle& s) {
        s.set_display(WidgetStyle::Display::Flex)
         .set_flex_wrap(WidgetStyle::FlexWrap::Wrap)
         .set_gap(SpacingValue::theme_md())
         .set_padding(BoxSpacing::theme());
    });

// Add responsive children
for (int i = 0; i < 10; i++) {
    auto item = create_widget<Panel>();
    item->get_style()
        .set_flex_basis(SizeValue(200))  // Base width
        .set_flex_grow(1)                // Grow to fill
        .set_flex_shrink(0);              // Don't shrink below basis
    container->add_child(item);
}
```