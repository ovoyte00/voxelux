# Voxelux Styling System Architecture

## Overview

The Voxelux styling system provides a structured, CSS-like approach to UI styling for both internal development and plugin APIs. This system is designed to be familiar to developers while maintaining the performance benefits of native C++ execution.

## Motivation

Professional C++ applications handle UI styling in different ways:

- **Blender**: Uses a structured layout system with manual property management
- **Adobe (CEP)**: Embeds Chromium and uses actual HTML/CSS/JavaScript
- **Qt Framework**: Provides QSS (Qt Style Sheets) but suffers from performance issues

Voxelux aims to combine the best of these approaches: the familiarity of CSS-like styling with the performance of native C++ execution.

## Core Components

### 1. Widget Style Structure

```cpp
struct WidgetStyle {
    // Box model
    struct Spacing {
        float top, right, bottom, left;
        
        // Convenience constructors
        Spacing(float all) : top(all), right(all), bottom(all), left(all) {}
        Spacing(float vertical, float horizontal) 
            : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
        Spacing(float top, float right, float bottom, float left)
            : top(top), right(right), bottom(bottom), left(left) {}
    };
    
    Spacing padding;
    Spacing margin;
    
    // Border
    struct Border {
        float width;
        ColorRGBA color;
        float radius;
        enum Style { None, Solid, Dashed, Dotted } style;
    } border;
    
    // Layout
    enum class Display { 
        Block,      // Stack vertically
        Flex,       // Flexible box layout
        Grid,       // Grid layout
        Inline      // Flow horizontally
    } display = Display::Block;
    
    enum class FlexDirection { Row, Column, RowReverse, ColumnReverse };
    enum class AlignItems { Start, Center, End, Stretch, Baseline };
    enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly };
    
    FlexDirection flex_direction = FlexDirection::Row;
    AlignItems align_items = AlignItems::Stretch;
    JustifyContent justify_content = JustifyContent::Start;
    float flex_gap = 0;  // Gap between flex items
    
    // Sizing
    enum SizeUnit { Pixels, Percent, Auto, FitContent };
    struct Size {
        float value;
        SizeUnit unit;
        
        Size(float px) : value(px), unit(Pixels) {}
        static Size auto() { return {0, Auto}; }
        static Size percent(float p) { return {p, Percent}; }
    };
    
    Size width = Size::auto();
    Size height = Size::auto();
    Size min_width = Size(0);
    Size max_width = Size(INFINITY);
    Size min_height = Size(0);
    Size max_height = Size(INFINITY);
    
    // Colors
    ColorRGBA background = ColorRGBA(0, 0, 0, 0);  // Transparent by default
    ColorRGBA text_color = ColorRGBA(1, 1, 1, 1);
    
    // Typography
    std::string font_family = "Inter";
    float font_size = 12.0f;  // Base size before DPI scaling
    enum FontWeight { 
        Thin = 100, Light = 300, Regular = 400, 
        Medium = 500, SemiBold = 600, Bold = 700 
    } font_weight = Regular;
    enum TextAlign { Left, Center, Right, Justify } text_align = Left;
    float line_height = 1.5f;  // Multiplier of font size
    
    // Visual effects
    float opacity = 1.0f;
    struct Shadow {
        float offset_x, offset_y;
        float blur_radius;
        ColorRGBA color;
    };
    std::optional<Shadow> box_shadow;
    
    // State variants
    std::optional<WidgetStyle> hover_style;
    std::optional<WidgetStyle> active_style;
    std::optional<WidgetStyle> disabled_style;
};
```

### 2. Style Application System

```cpp
class StyledWidget : public UIWidget {
private:
    WidgetStyle base_style_;
    WidgetStyle computed_style_;  // After inheritance and state
    
public:
    // Direct style setting
    void set_style(const WidgetStyle& style);
    
    // JSON/YAML style setting
    void set_style_json(const std::string& json);
    void set_style_yaml(const std::string& yaml);
    
    // Style classes (like CSS classes)
    void add_class(const std::string& class_name);
    void remove_class(const std::string& class_name);
    
    // Computed style access
    const WidgetStyle& get_computed_style() const;
    
protected:
    // Override for custom style computation
    virtual void compute_style();
    
    // Apply style to rendering
    virtual void apply_style(CanvasRenderer* renderer);
};
```

### 3. Style Sheet System

```cpp
class StyleSheet {
private:
    // Class-based styles (like CSS classes)
    std::unordered_map<std::string, WidgetStyle> class_styles_;
    
    // Type-based styles (like CSS element selectors)
    std::unordered_map<std::string, WidgetStyle> type_styles_;
    
    // ID-based styles (like CSS ID selectors)
    std::unordered_map<std::string, WidgetStyle> id_styles_;
    
public:
    // Load from file
    static StyleSheet from_file(const std::string& path);
    static StyleSheet from_json(const std::string& json);
    static StyleSheet from_yaml(const std::string& yaml);
    
    // Style queries
    WidgetStyle get_style_for_class(const std::string& class_name) const;
    WidgetStyle get_style_for_type(const std::string& type_name) const;
    WidgetStyle get_style_for_id(const std::string& id) const;
    
    // Merge with another stylesheet (cascade)
    void merge(const StyleSheet& other);
};
```

## JSON Style Definition Format

```json
{
  "styles": {
    ".button": {
      "padding": [10, 15],
      "margin": 5,
      "border": {
        "width": 1,
        "color": "#cccccc",
        "radius": 4
      },
      "background": "#3c3c3c",
      "text_color": "#ffffff",
      "font_size": 14,
      "hover": {
        "background": "#4a4a4a",
        "border": {
          "color": "#ffffff"
        }
      }
    },
    
    ".panel": {
      "display": "flex",
      "flex_direction": "column",
      "padding": 20,
      "background": "#2b2b2b",
      "border": {
        "width": 1,
        "color": "#1a1a1a"
      }
    },
    
    "#main-toolbar": {
      "display": "flex",
      "flex_direction": "row",
      "justify_content": "space-between",
      "padding": [5, 10],
      "background": "#323232",
      "height": 40
    }
  }
}
```

## Layout System

### Box Model
Every widget follows the CSS box model:
- **Content**: The actual content area
- **Padding**: Space between content and border
- **Border**: The border around the padding
- **Margin**: Space outside the border

### Flexbox Layout
When `display: flex` is set:
- Children are laid out according to `flex_direction`
- Main axis alignment controlled by `justify_content`
- Cross axis alignment controlled by `align_items`
- Gaps between items controlled by `flex_gap`

### Size Calculation
1. **Auto**: Size based on content
2. **Pixels**: Fixed size (scaled by DPI)
3. **Percent**: Percentage of parent size
4. **Fit-Content**: Size to fit content with min/max constraints

## Plugin API Integration

### C++ Plugins

```cpp
class MyPlugin : public VoxeluxPlugin {
    void create_ui() override {
        auto panel = create_widget<Panel>();
        
        // Method 1: Direct style object
        WidgetStyle panel_style;
        panel_style.padding = {20, 20, 20, 20};
        panel_style.background = ColorRGBA("#2b2b2b");
        panel->set_style(panel_style);
        
        // Method 2: JSON styling
        panel->set_style_json(R"({
            "padding": 20,
            "background": "#2b2b2b",
            "display": "flex",
            "flex_direction": "column"
        })");
        
        // Method 3: Style classes
        panel->add_class("dark-panel");
        
        auto button = create_widget<Button>("Click Me");
        button->set_style_json(R"({
            "padding": [10, 20],
            "margin": 5,
            "background": "#5680c2",
            "text_color": "white",
            "border_radius": 4
        })");
        
        panel->add_child(button);
    }
};
```

### Python Plugins

```python
class MyPlugin(VoxeluxPlugin):
    def create_ui(self):
        panel = self.create_widget("Panel")
        
        # Python dictionary style
        panel.set_style({
            "padding": 20,
            "background": "#2b2b2b",
            "display": "flex",
            "flex_direction": "column"
        })
        
        button = self.create_widget("Button", text="Click Me")
        button.set_style({
            "padding": [10, 20],
            "margin": 5,
            "background": "#5680c2",
            "text_color": "white",
            "border_radius": 4
        })
        
        panel.add_child(button)
```

## Style Inheritance and Cascade

Styles are applied in the following order (later overrides earlier):

1. **Default styles**: Built-in widget defaults
2. **Theme styles**: Current application theme
3. **Type styles**: Styles for widget type (e.g., all buttons)
4. **Class styles**: Styles from assigned classes
5. **ID styles**: Styles for specific widget ID
6. **Inline styles**: Directly set styles
7. **State styles**: Hover, active, disabled overrides

## Performance Considerations

### Caching
- Computed styles are cached and only recalculated when:
  - Style properties change
  - Parent layout changes
  - DPI scaling changes
  
### Batch Updates
- Multiple style changes can be batched:
```cpp
widget->begin_style_update();
widget->set_style_property("padding", 10);
widget->set_style_property("margin", 5);
widget->set_style_property("background", "#3c3c3c");
widget->end_style_update();  // Triggers single recomputation
```

### Memory Management
- Styles use copy-on-write for state variants
- Unused style variants are lazily allocated
- Style sheets can be shared across widgets

## Migration from Current System

### Phase 1: Foundation
1. Implement `WidgetStyle` structure
2. Add `StyledWidget` base class
3. Create JSON parser for styles

### Phase 2: Integration
1. Migrate `UIWidget` to inherit from `StyledWidget`
2. Update rendering to use computed styles
3. Convert hardcoded values to style properties

### Phase 3: Plugin API
1. Expose style system to plugin API
2. Create style sheet loading system
3. Add Python bindings

### Phase 4: Advanced Features
1. Implement CSS-like selectors
2. Add animation support
3. Create visual style editor

## Benefits

### For Core Development
- **Consistency**: All UI elements use same styling system
- **Maintainability**: Styles separated from logic
- **Theming**: Easy to create and switch themes
- **DPI Scaling**: Automatic handling through ScaledTheme integration

### For Plugin Developers
- **Familiar**: CSS-like syntax and concepts
- **Flexible**: Multiple ways to apply styles
- **Powerful**: Full layout system with flexbox
- **Type-Safe**: Compile-time checking for C++ plugins

### For End Users
- **Customizable**: Can modify UI through style sheets
- **Consistent**: Plugins match application look and feel
- **Accessible**: Easier to create high-contrast or large-text themes
- **Professional**: Modern, polished interface

## Example: Complete Panel Style

```json
{
  "styles": {
    ".property-panel": {
      "display": "flex",
      "flex_direction": "column",
      "padding": 15,
      "margin": 0,
      "background": "#2b2b2b",
      "border": {
        "width": 1,
        "color": "#1a1a1a",
        "radius": 0
      },
      "width": 320,
      "height": "auto",
      "min_height": 200,
      "max_height": "100%",
      
      ".panel-header": {
        "display": "flex",
        "flex_direction": "row",
        "justify_content": "space-between",
        "align_items": "center",
        "padding": [10, 15],
        "margin": [-15, -15, 10, -15],
        "background": "#323232",
        "border": {
          "width": [0, 0, 1, 0],
          "color": "#1a1a1a"
        }
      },
      
      ".property-row": {
        "display": "flex",
        "flex_direction": "row",
        "justify_content": "space-between",
        "align_items": "center",
        "padding": [5, 0],
        "margin": [2, 0],
        
        "hover": {
          "background": "rgba(255, 255, 255, 0.05)"
        }
      },
      
      ".property-label": {
        "text_color": "#cccccc",
        "font_size": 12,
        "width": "40%"
      },
      
      ".property-value": {
        "width": "60%",
        "display": "flex",
        "flex_gap": 5
      }
    }
  }
}
```

## Future Enhancements

### Planned Features
- **Animations**: Transition support for style changes
- **Variables**: CSS-like custom properties
- **Media Queries**: Responsive design based on window size
- **Pseudo-selectors**: :hover, :active, :focus in style sheets
- **Grid Layout**: CSS Grid-like layout system
- **Transforms**: Rotation, scale, translation
- **Filters**: Blur, brightness, contrast effects

### Potential Optimizations
- **GPU Acceleration**: Cache rendered styles as textures
- **Dirty Tracking**: Only re-render changed portions
- **Style Pooling**: Reuse common style combinations
- **Lazy Evaluation**: Compute styles only when needed

## Conclusion

The Voxelux styling system provides a modern, flexible approach to UI styling that combines the familiarity of web development with the performance of native C++ applications. By implementing this system, we create a consistent, maintainable, and extensible foundation for both core development and plugin ecosystem growth.