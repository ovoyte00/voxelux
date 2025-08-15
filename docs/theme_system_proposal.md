# Voxelux Canvas UI Theme System

## Overview

A comprehensive theming system integrated with the existing Voxelux Canvas UI framework. This system extends your current UI architecture to provide centralized styling with hot-reload capabilities, building upon your established event-driven design patterns.

## Integration with Existing Architecture

### Canvas UI Framework Integration

The theme system integrates with your existing Canvas UI components:

- **Event-Driven**: Uses your existing `event_router.h` system for theme change notifications
- **Region-Aware**: Supports different themes per canvas region type
- **Renderer Integration**: Extends `canvas_renderer.h` with theme-aware rendering
- **Widget System**: Enhances `ui_widgets.h` with automatic style application

## File Structure Integration

### New Theme System Files

```
voxelux/
├── include/canvas_ui/
│   ├── theming/                    # NEW: Theme system headers
│   │   ├── theme_manager.h         # Central theme management
│   │   ├── style_registry.h        # Component style definitions
│   │   ├── styled_component.h      # Base class for themed components
│   │   ├── theme_events.h          # Theme-related events
│   │   └── theme_validator.h       # Theme validation utilities
│   ├── canvas_core.h               # MODIFIED: Add theme integration
│   ├── canvas_renderer.h           # MODIFIED: Add theme-aware rendering
│   ├── ui_widgets.h                # MODIFIED: Add style support
│   └── ... (existing files)
├── src/canvas_ui/
│   ├── theming/                    # NEW: Theme system implementation
│   │   ├── CMakeLists.txt          # Theme module build config
│   │   ├── theme_manager.cpp
│   │   ├── style_registry.cpp
│   │   ├── styled_component.cpp
│   │   ├── theme_events.cpp
│   │   └── theme_validator.cpp
│   ├── canvas_core.cpp             # MODIFIED: Theme initialization
│   ├── canvas_renderer.cpp         # MODIFIED: Theme-aware rendering
│   ├── ui_widgets.cpp              # MODIFIED: Style application
│   └── ... (existing files)
├── themes/                         # NEW: Theme definition files
│   ├── voxelux_dark.json          # Default dark theme (matches your current colors)
│   ├── voxelux_light.json         # Light theme variant
│   └── custom/                     # User custom themes
└── tools/                          # NEW: Theme development tools
    └── theme_editor/               # Visual theme editor (future)
```

## Theme Definition Format

Based on your existing color scheme from the GUI structure document:

```json
{
  "metadata": {
    "name": "Voxelux Dark",
    "version": "1.0",
    "description": "Default Voxelux theme matching current design"
  },
  "colors": {
    "gray_1": "#F2F2F2",
    "gray_2": "#B4B4B4", 
    "gray_3": "#575757",
    "gray_4": "#454545",
    "gray_5": "#303030",
    "x_axis": "#BC4252",
    "y_axis": "#6CAC34",
    "z_axis": "#3B83BE",
    "background_primary": "#454545",
    "background_secondary": "#303030",
    "text_primary": "#F2F2F2",
    "text_secondary": "#B4B4B4",
    "border": "#575757"
  },
  "dimensions": {
    "menu_height": 25,
    "footer_height": 25,
    "dock_collapsed_width": 36,
    "border_width": 1,
    "padding_small": 5,
    "padding_medium": 10,
    "menu_padding_horizontal": 10,
    "logo_height": 15,
    "menu_font_size": 11,
    "menu_item_spacing": 20
  },
  "component_styles": {
    "application_menu": {
      "background": "gray_5",
      "text_color": "text_primary",
      "border_bottom": "gray_5",
      "height": "menu_height",
      "padding_horizontal": "menu_padding_horizontal"
    },
    "application_content": {
      "background": "gray_4"
    },
    "application_footer": {
      "background": "gray_5",
      "height": "footer_height"
    },
    "dock_panel": {
      "background": "gray_4",
      "border": "gray_3",
      "collapsed_width": "dock_collapsed_width"
    },
    "viewport_3d": {
      "background": "gray_4",
      "grid_color": "gray_3",
      "axis_x_color": "x_axis",
      "axis_y_color": "y_axis",
      "axis_z_color": "z_axis"
    }
  }
}
```

## Implementation Classes

### Theme Manager Integration

```cpp
// include/canvas_ui/theming/theme_manager.h
#ifndef VOXELUX_THEME_MANAGER_H
#define VOXELUX_THEME_MANAGER_H

#include "canvas_ui/event_router.h"
#include "canvas_ui/theming/theme_events.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>

namespace voxelux::canvas_ui {

using Color = glm::vec4;

class ThemeManager {
private:
    std::unordered_map<std::string, Color> colors_;
    std::unordered_map<std::string, float> dimensions_;
    std::string currentThemePath_;
    std::shared_ptr<EventRouter> eventRouter_;
    
    ThemeManager() = default;
    
public:
    static ThemeManager& instance();
    
    // Integration with existing event system
    void setEventRouter(std::shared_ptr<EventRouter> router);
    
    // Theme loading
    bool loadTheme(const std::string& themePath);
    void reloadTheme();
    
    // Value accessors (matching your existing patterns)
    Color getColor(const std::string& name) const;
    float getDimension(const std::string& name) const;
    
    // Current theme info
    std::string getCurrentThemeName() const;
    std::string getCurrentThemePath() const;
    
    // Validation
    bool validateTheme(const std::string& themePath) const;
};

} // namespace voxelux::canvas_ui

#endif
```

### Theme Events Integration

```cpp
// include/canvas_ui/theming/theme_events.h
#ifndef VOXELUX_THEME_EVENTS_H
#define VOXELUX_THEME_EVENTS_H

#include "voxelux/core/event.h"
#include <string>

namespace voxelux::canvas_ui {

class ThemeChangedEvent : public voxelux::core::Event {
public:
    ThemeChangedEvent(const std::string& themeName) 
        : themeName_(themeName) {}
    
    const std::string& getThemeName() const { return themeName_; }
    
private:
    std::string themeName_;
};

class ThemeReloadEvent : public voxelux::core::Event {
public:
    ThemeReloadEvent() = default;
};

} // namespace voxelux::canvas_ui

#endif
```

### Styled Component Base Class

```cpp
// include/canvas_ui/theming/styled_component.h
#ifndef VOXELUX_STYLED_COMPONENT_H
#define VOXELUX_STYLED_COMPONENT_H

#include "canvas_ui/theming/theme_manager.h"
#include "canvas_ui/theming/theme_events.h"
#include "voxelux/core/event.h"
#include <string>
#include <memory>

namespace voxelux::canvas_ui {

class StyledComponent {
protected:
    std::string styleId_;
    bool needsStyleRefresh_;
    
public:
    StyledComponent(const std::string& styleId);
    virtual ~StyledComponent() = default;
    
    void setStyleId(const std::string& styleId);
    void refreshStyle();
    
    // Event handling for theme changes
    virtual void onThemeChanged(const ThemeChangedEvent& event);
    virtual void onStyleRefresh() = 0;
    
protected:
    // Helper methods for common style properties
    Color getStyleColor(const std::string& property) const;
    float getStyleDimension(const std::string& property) const;
};

} // namespace voxelux::canvas_ui

#endif
```

## Integration Points

### 1. Canvas Core Integration

Modify `canvas_core.cpp` to initialize the theme system:

```cpp
// In CanvasCore constructor
void CanvasCore::initialize() {
    // Existing initialization...
    
    // Initialize theme system
    auto& themeManager = ThemeManager::instance();
    themeManager.setEventRouter(eventRouter_);
    themeManager.loadTheme("themes/voxelux_dark.json");
    
    // Register for theme events
    eventRouter_->subscribe<ThemeChangedEvent>(
        [this](const ThemeChangedEvent& e) {
            onThemeChanged(e);
        });
}
```

### 2. Viewport 3D Editor Integration

Extend your existing `viewport_3d_editor.h`:

```cpp
// Add to Viewport3DEditor class
class Viewport3DEditor : public StyledComponent {
private:
    // Existing members...
    Color gridColor_;
    Color axisXColor_;
    Color axisYColor_;
    Color axisZColor_;
    
public:
    Viewport3DEditor() : StyledComponent("viewport_3d") {
        // Existing constructor...
    }
    
    void onStyleRefresh() override {
        // Update colors from theme
        gridColor_ = getStyleColor("grid_color");
        axisXColor_ = getStyleColor("axis_x_color");
        axisYColor_ = getStyleColor("axis_y_color");
        axisZColor_ = getStyleColor("axis_z_color");
        
        // Update grid renderer colors
        if (gridRenderer_) {
            gridRenderer_->setColors(gridColor_, axisXColor_, axisYColor_, axisZColor_);
        }
    }
};
```

### 3. UI Widgets Integration

Enhance your existing `ui_widgets.h`:

```cpp
// Add themed button example
class ThemedButton : public StyledComponent {
private:
    Color backgroundColor_;
    Color textColor_;
    Color borderColor_;
    float borderWidth_;
    
public:
    ThemedButton() : StyledComponent("button") {}
    
    void onStyleRefresh() override {
        backgroundColor_ = getStyleColor("background");
        textColor_ = getStyleColor("text_color");
        borderColor_ = getStyleColor("border");
        borderWidth_ = getStyleDimension("border_width");
    }
    
    void render(CanvasRenderer& renderer) {
        // Use themed colors for rendering
        renderer.setFillColor(backgroundColor_);
        renderer.setBorderColor(borderColor_);
        renderer.setBorderWidth(borderWidth_);
        // ... render implementation
    }
};
```

## Implementation Phases

### Phase 1: Core Integration (Week 1)
1. **Add theme system files** to existing Canvas UI structure
2. **Integrate ThemeManager** with existing EventRouter
3. **Create base StyledComponent** class
4. **Update CMakeLists.txt** files for new modules

### Phase 2: Component Migration (Week 2)
1. **Convert Viewport3DEditor** to use theme system
2. **Update Grid3DRenderer** to accept themed colors
3. **Migrate NavigationWidget** to theme system
4. **Update existing UI widgets**

### Phase 3: Default Theme (Week 3)
1. **Create voxelux_dark.json** matching current design
2. **Validate all components** use theme correctly
3. **Add theme validation** and error handling
4. **Test hot reload** functionality

### Phase 4: Advanced Features (Week 4+)
1. **Add light theme** variant
2. **Implement theme editor** tools
3. **Add user customization** support
4. **Performance optimization**

## Benefits for Voxelux

### Immediate Benefits
- **Preserves Current Design**: Default theme matches your existing gray scale
- **Minimal Disruption**: Integrates with existing event system and architecture
- **Professional Appearance**: Maintains the industry-standard look you've achieved

### Future Benefits
- **Easy Customization**: Users can create themes matching their workflow
- **Accessibility**: High-contrast themes for visually impaired users
- **Branding**: Custom themes for different use cases (Minecraft, Unity export, etc.)
- **Development Speed**: Rapid visual iteration without recompilation

### Technical Benefits
- **Consistent with Architecture**: Uses your existing event-driven patterns
- **Performance**: Leverages your existing rendering optimizations
- **Maintainable**: Centralized styling reduces code duplication
- **Extensible**: Easy to add new components and style properties

## Required Modifications Summary

### New Files to Create
- All files in `include/canvas_ui/theming/` and `src/canvas_ui/theming/`
- `themes/voxelux_dark.json` (default theme)
- Updated CMakeLists.txt files

### Existing Files to Modify
- `canvas_core.h/cpp` - Theme system initialization
- `canvas_renderer.h/cpp` - Theme-aware rendering methods
- `viewport_3d_editor.h/cpp` - Theme integration
- `grid_3d_renderer.h/cpp` - Accept themed colors
- `ui_widgets.h/cpp` - Add styled component examples
- `navigation_widget.h/cpp` - Theme integration

### Dependencies to Add
- JSON parsing library (nlohmann/json or similar)
- File watching library for hot reload (optional)

This approach maintains your existing architecture while adding powerful theming capabilities that will scale with your application's growth.