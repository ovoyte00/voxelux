# Archived Legacy Header Files

This directory contains header files for the legacy implementations that have been replaced by the modern Canvas UI system. These headers correspond to the source files archived in `src/archive/`.

## Archived Headers

### Navigation and Viewport System Headers
- **`viewport_nav_widget.h`** - Navigation widget class definition with 6-axis sphere system, professional interaction callbacks, and text rendering integration. **Replaced by:** `canvas_ui/viewport_3d_editor.h`
- **`viewport_widget.h`** - Base widget class with mouse event handling, 3D projection math, and OpenGL utilities. **Functionality integrated into:** `canvas_ui/viewport_3d_editor.h`

### Rendering and UI System Headers  
- **`text_renderer.h`** - Legacy text rendering class with font management and glyph rendering. **Replaced by:** Text rendering integrated into `canvas_ui/canvas_renderer.h`
- **`theme.h`** - Color theme system with grid colors, axis colors, and UI colors. **Replaced by:** `CanvasTheme` struct in `canvas_ui/canvas_core.h`
- **`typography.h`** - Font loading and typography management system. **Replaced by:** Text system integrated into Canvas UI

### Animation System Header
- **`view_animation.h`** - Camera animation and transition system for smooth viewport changes. **Status:** Not yet ported, may be integrated into future Canvas UI camera system.

## Key Type Definitions Successfully Ported

### From viewport_nav_widget.h:
```cpp
// Ported to canvas_ui/viewport_3d_editor.h as:
struct AxisSphere {
    int axis; bool positive; float depth; 
    Point2D position; ColorRGBA color;
};
struct BillboardData { ... };
```

### From viewport_widget.h:
```cpp
// Ported functionalities to canvas_ui/viewport_3d_editor.h:
// - Mouse event handling
// - 3D projection/unprojection
// - Screen coordinate conversion
// - Click callbacks and interaction
```

### From theme.h:
```cpp
// Ported to canvas_ui/canvas_core.h as CanvasTheme:
struct CanvasTheme {
    ColorRGBA background_primary, background_secondary;
    ColorRGBA accent_selection, text_primary;
    ColorRGBA grid_subtle, border_normal;
    // ... professional color scheme
};
```

## Modern Replacement Headers

All functionality has been reimplemented in the Canvas UI headers:
- **Navigation/Viewport:** `canvas_ui/viewport_3d_editor.h`
- **UI Rendering:** `canvas_ui/canvas_renderer.h`  
- **Core Types:** `canvas_ui/canvas_core.h`
- **Event System:** `canvas_ui/event_router.h`
- **Widget System:** `canvas_ui/ui_widgets.h`
- **Region System:** `canvas_ui/canvas_region.h`
- **Window Management:** `canvas_ui/canvas_window.h`

## Correspondence Table

| Legacy Header | Legacy Source | Modern Replacement |
|---------------|---------------|-------------------|
| `viewport_nav_widget.h` | `src/archive/viewport_nav_widget.cpp` | `canvas_ui/viewport_3d_editor.h` |
| `viewport_widget.h` | `src/archive/viewport_widget.cpp` | `canvas_ui/viewport_3d_editor.h` |
| `text_renderer.h` | `src/archive/text_renderer.cpp` | `canvas_ui/canvas_renderer.h` |
| `theme.h` | `src/archive/theme.cpp` | `canvas_ui/canvas_core.h` |
| `typography.h` | `src/archive/typography.cpp` | `canvas_ui/canvas_renderer.h` |
| `view_animation.h` | `src/archive/view_animation.cpp` | *Not yet ported* |