# Voxelux Complete Style Guide

## Color Theme System

### Base Color Palette
```cpp
// Gray scale colors - foundation of the interface
#define COLOR_GRAY_1 0xF2F2F2FF  // Lightest - primary text, active icons
#define COLOR_GRAY_2 0xB4B4B4FF  // Light - secondary text, inactive elements
#define COLOR_GRAY_3 0x575757FF  // Medium - panel backgrounds, active states
#define COLOR_GRAY_4 0x393939FF  // Dark - main application background
#define COLOR_GRAY_5 0x303030FF  // Darkest - borders, headers, footers
```

### Axis Colors (3D Navigation)
```cpp
#define COLOR_X_AXIS 0xBC4252FF  // Red - X axis representation
#define COLOR_Y_AXIS 0x6CAC34FF  // Green - Y axis representation  
#define COLOR_Z_AXIS 0x3B83BEFF  // Blue - Z axis representation
```

### Semantic Colors
```cpp
#define COLOR_ACCENT_PRIMARY   0x3B83BEFF  // Primary actions, selections, playhead
#define COLOR_ACCENT_SECONDARY 0x6CAC34FF  // Secondary actions, success states
#define COLOR_ACCENT_WARNING   0xBC4252FF  // Warnings, errors, unsaved indicators
```

## Typography System

### Font Configuration
```cpp
// Primary font: Inter Variable
// Fallback chain: system fonts
FontFamily: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif
BaseFontSize: 12px
FontSmoothing: antialiased
FontFeatures: 'ss01', 'ss02', 'cv01', 'cv03', 'cv04'  // Stylistic sets
```

### Typography Scale
```cpp
FONT_SIZE_LOGO:     13px  // Application branding
FONT_SIZE_STANDARD: 12px  // Base text size
FONT_SIZE_SMALL:    11px  // UI labels, controls
FONT_SIZE_TINY:     10px  // Timeline labels

FONT_WEIGHT_LIGHT:   400  // Body text, descriptions
FONT_WEIGHT_MEDIUM:  450  // UI elements, tabs
FONT_WEIGHT_SEMIBOLD: 500  // Headings
FONT_WEIGHT_BOLD:    600  // Logo, emphasis
```

## Layout Dimensions

### Core Layout Constants
```cpp
MENU_BAR_HEIGHT:        25px
FOOTER_HEIGHT:          25px
FILE_BAR_HEIGHT:        25px
EDITOR_HEADER_HEIGHT:   25px
DOCK_HEADER_HEIGHT:     14px
PANEL_TAB_HEIGHT:       25px
TIMELINE_RULER_HEIGHT:  25px
TIMELINE_TRACK_HEIGHT:  30px
```

### Dock System Dimensions
```cpp
DOCK_COLLAPSED_WIDTH:    36px   // Minimum dock width
DOCK_COLLAPSED_MAX:      225px  // Maximum when resizing collapsed dock
DOCK_EXPANDED_WIDTH:     250px  // Default expanded width
DOCK_EXPANDED_MIN:       250px  // Minimum expanded width
DOCK_EXPANDED_MAX:       500px  // Maximum expanded width
TOOL_DOCK_EXPANDED:      72px   // Fixed width for tool dock
```

### Spacing System
```cpp
SPACING_XS:  2px   // Tight spacing (tool grid gaps)
SPACING_SM:  4px   // Small spacing (icon gaps)
SPACING_MD:  8px   // Medium spacing (panel padding)
SPACING_LG:  10px  // Large spacing (content padding)
SPACING_XL:  20px  // Extra large (menu items)
```

## Component Specifications

### Buttons
```cpp
// Standard Control Button
BUTTON_HEIGHT:        22px
BUTTON_WIDTH:         22px
BUTTON_BORDER_RADIUS: 3px
BUTTON_BORDER_WIDTH:  1px

// Tool Icons
TOOL_ICON_SIZE:       32px
TOOL_ICON_INNER:      18px  // SVG content size

// File Tab Buttons
TAB_CLOSE_SIZE:       16px
TAB_MIN_WIDTH:        120px
TAB_MAX_WIDTH:        200px
TAB_HEIGHT:           25px
```

### Input Controls
```cpp
DROPDOWN_HEIGHT:      19px
INPUT_HEIGHT:         19px
INPUT_BORDER_RADIUS:  3px
SELECT_MIN_WIDTH:     80px
```

### Panels and Windows
```cpp
PANEL_MIN_HEIGHT:     100px  // Minimum for resizable panels
EDITOR_MIN_SIZE:      100px  // Minimum for split views
POPOUT_MIN_WIDTH:     250px
POPOUT_MIN_HEIGHT:    200px
BOTTOM_DOCK_DEFAULT:  200px
BOTTOM_DOCK_MIN:      100px
BOTTOM_DOCK_MAX:      50vh   // Half viewport height
```

## Visual Effects

### Transitions
```cpp
TRANSITION_FAST:     0.15s ease  // Hover states, color changes
TRANSITION_MEDIUM:   0.2s ease   // Dock width changes
TRANSITION_SLOW:     0.3s ease   // Navigation cube rotation
```

### Shadows
```cpp
SHADOW_POPOUT:       0 4px 12px rgba(0, 0, 0, 0.3)
SHADOW_HEADER:       0px 0px 5px rgba(0, 0, 0, 0.4)
SHADOW_TOOL_ACTIVE:  inset 0 1px 3px rgba(0, 0, 0, 0.3)
```

### Borders
```cpp
BORDER_STANDARD:     1px solid COLOR_GRAY_5
BORDER_ACTIVE:       2px solid COLOR_ACCENT_PRIMARY
BORDER_RADIUS_SM:    2px
BORDER_RADIUS_MD:    3px
BORDER_RADIUS_LG:    4px
```

## Interactive States

### Hover States
```cpp
// Menu items and panels
HOVER_BACKGROUND: COLOR_GRAY_4 (from COLOR_GRAY_5)
HOVER_TEXT:       COLOR_GRAY_1 (from COLOR_GRAY_2)

// Buttons
BUTTON_HOVER_BG:  COLOR_GRAY_4
BUTTON_HOVER_FG:  COLOR_GRAY_1

// Resize handles
RESIZE_HOVER:     COLOR_ACCENT_PRIMARY
```

### Active States
```cpp
// Selected tools
TOOL_ACTIVE_BG:   COLOR_GRAY_5
TOOL_ACTIVE_FG:   COLOR_ACCENT_SECONDARY

// Tabs and panels
TAB_ACTIVE_BG:    COLOR_GRAY_3
PANEL_ACTIVE:     COLOR_GRAY_3

// Dock indicators
DOCK_ACTIVE_BORDER: 2px solid COLOR_ACCENT_PRIMARY
```

### Modified/Unsaved States
```cpp
MODIFIED_INDICATOR_SIZE:  6px
MODIFIED_INDICATOR_COLOR: COLOR_ACCENT_WARNING
MODIFIED_INDICATOR_SHAPE: circle
```

## Special UI Components

### Split View System
```cpp
RESIZE_HANDLE_WIDTH:     2px
RESIZE_HANDLE_HIT_AREA:  4px  // Invisible interaction area
RESIZE_HANDLE_COLOR:     transparent
RESIZE_HANDLE_HOVER:     COLOR_ACCENT_PRIMARY
SPLIT_MIN_SIZE:          100px
```

### Timeline Component
```cpp
TIMELINE_HEIGHT:         200px (default)
TIMELINE_PLAYHEAD_WIDTH: 2px
TIMELINE_PLAYHEAD_COLOR: COLOR_ACCENT_PRIMARY
KEYFRAME_SIZE:           10px
KEYFRAME_ROTATION:       45deg  // Diamond shape
TRACK_HEADER_WIDTH:      120px
```

### Navigation Widget
```cpp
NAV_WIDGET_SIZE:         80px
NAV_CUBE_FACE_SIZE:      60px
NAV_CUBE_ROTATION_X:     -20deg
NAV_CUBE_ROTATION_Y:     25deg
NAV_WIDGET_POSITION:     top: 10px, right: 10px
```

### Scrollbars
```cpp
SCROLLBAR_WIDTH:         8px
SCROLLBAR_TRACK_COLOR:   COLOR_GRAY_5
SCROLLBAR_THUMB_COLOR:   COLOR_GRAY_3
SCROLLBAR_THUMB_RADIUS:  4px
```

## Layout Patterns

### Application Grid
```
┌─────────────────────────────────────────┐
│ Menu Bar (25px)                         │
├─────────────────────────────────────────┤
│ Top Dock (32px) - Control Bar (optional)│
├──┬──────────────────────────────────┬───┤
│  │                                  │   │
│D │     Main Content Area           │ D │
│o │  ┌──────────────────────────┐   │ o │
│c │  │ File Bar (25px)          │   │ c │
│k │  ├──────────────────────────┤   │ k │
│  │  │ Editor Window            │   │   │
│L │  │ - Split Views            │   │ R │
│e │  │ - 3D Viewport            │   │ i │
│f │  │ - Text Editor            │   │ g │
│t │  └──────────────────────────┘   │ h │
│  │                                  │ t │
├──┴──────────────────────────────────┴───┤
│ Bottom Dock (200px) - Timeline          │
├─────────────────────────────────────────┤
│ Footer (25px)                           │
└─────────────────────────────────────────┘
```

### Dock Column Structure
```
Collapsed State (36px):
┌────┐
│ ≡  │ <- Grip icon
├────┤
│ 🔧 │ <- Tool icon
│ 📄 │
│ 🎨 │
├────┤
│ ≡  │ <- Panel group separator
├────┤
│ 📊 │ <- Second group
│ 🔍 │
└────┘

Expanded State (250px):
┌──────────────────────┐
│ [Tab1][Tab2][Tab3]   │ <- Panel tabs
├──────────────────────┤
│                      │
│   Panel Content      │
│                      │
├──────────────────────┤ <- 2px gap (COLOR_GRAY_4)
│ [Tab1][Tab2]         │ <- Second panel group
├──────────────────────┤
│   Panel Content      │
└──────────────────────┘
```

## Implementation Notes

### Clean-Room Development Process
When implementing based on reference code (e.g., Blender):
1. Study reference implementation for concepts
2. Research industry-standard approaches
3. Create original implementation with unique structure

### Event-Driven Architecture
- SimpleEvent system for initial development
- Upgradeable to robust event system as needed
- Event delegation for dynamic content
- Loose coupling between components

### Responsive Design
- Breakpoint at 1200px: Reduce expanded dock to 200px
- Breakpoint at 800px: Force all docks to collapsed state
- Minimum viewport: 800px × 600px

### Performance Considerations
- Use CSS transforms for animations (GPU accelerated)
- Implement virtual scrolling for large lists
- Lazy load panel content when expanded
- Debounce resize events (16ms threshold)

### Accessibility
- Keyboard navigation support
- Focus indicators on interactive elements
- ARIA labels for screen readers
- Tooltip delays: 500ms hover before show

## File Organization

### Required Assets
```
/assets/
  /icons/
    grip.svg          # Dock grip handle
    view_split.svg    # Editor split icon
    /tools/           # Tool icons directory
  /fonts/
    Inter-Variable.ttf # Primary font file
```

### Component Hierarchy
```
Application
├── MenuBar
├── ContentArea
│   ├── TopDock (ControlBar)
│   ├── LeftDock
│   │   ├── ToolDock
│   │   └── PanelDock
│   ├── MainContent
│   │   ├── FileBar
│   │   └── EditorWindow
│   │       └── SplitContainer(s)
│   │           └── EditorView(s)
│   ├── RightDock
│   │   └── PanelDock(s)
│   └── BottomDock (Timeline)
└── Footer

```

## State Management

### Dock States
```cpp
enum DockState {
    DOCK_COLLAPSED,
    DOCK_EXPANDED,
    DOCK_HIDDEN
};

struct DockConfig {
    DockState state;
    int width;           // Current width
    int expandedWidth;   // Remembered width when expanded
    bool resizable;      // Can user resize?
    int minWidth;
    int maxWidth;
};
```

### Editor States
```cpp
enum EditorViewType {
    VIEW_3D,
    VIEW_UV,
    VIEW_SHADER,
    VIEW_TEXT,
    VIEW_TIMELINE
};

struct EditorConfig {
    EditorViewType type;
    bool canSplit;
    bool canClose;
    bool isModified;
};
```

### File Tab States
```cpp
struct FileTab {
    string filename;
    string filepath;
    bool isActive;
    bool isModified;
    bool isPinned;
};
```

This comprehensive style guide provides all necessary specifications to implement the Voxelux UI system in C++, maintaining consistency with the web prototype while allowing for native performance optimizations.