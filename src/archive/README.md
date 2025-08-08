# Archived Legacy Files

This directory contains legacy implementations that have been replaced by the modern Canvas UI system. These files are preserved for reference and historical purposes.

## Files Overview

### Navigation and Viewport System (Ported to Canvas UI)
- **`viewport_nav_widget.cpp`** - Original navigation widget with 6-axis sphere system, depth sorting, and professional interaction logic. **Logic ported to:** `canvas_ui/viewport_3d_editor.cpp`
- **`viewport_widget.cpp`** - Base widget class with mouse handling, projection math, and 3D viewport utilities. **Logic ported to:** `canvas_ui/viewport_3d_editor.cpp`

### Rendering and UI Systems (Replaced by Canvas UI)
- **`text_renderer.cpp`** - Legacy text rendering system. **Replaced by:** Canvas UI text rendering in `canvas_ui/canvas_renderer.cpp`
- **`theme.cpp`** - Old color theming system with grid colors, axis colors, and UI colors. **Replaced by:** `CanvasTheme` in `canvas_ui/canvas_core.h`
- **`typography.cpp`** - Legacy typography and font management. **Replaced by:** Canvas UI text system

### Animation System
- **`view_animation.cpp/.h`** - Camera animation system for smooth viewport transitions. **Status:** Not yet ported, may be integrated into future Canvas UI camera system.

### Legacy Application Entry
- **`voxelux_opengl.cpp`** - Old main application with different UI framework. **Replaced by:** `voxelux_canvas.cpp`

## Key Features Successfully Ported

### From viewport_nav_widget.cpp:
- 6-axis navigation sphere (+/-X, +/-Y, +/-Z)
- Professional depth sorting algorithm
- Billboard rendering with screen-space projection
- Distance-based scaling for visual depth perception
- Interactive hover states and axis click handling
- Axis labels and color-coded indicators

### From viewport_widget.cpp:
- Mouse event handling (press/move/release/wheel)
- 3D projection/unprojection math
- Screen coordinate conversion
- Ray casting for 3D interaction
- OpenGL state management utilities

### From theme.cpp:
- Professional color schemes
- Grid line color differentiation (major/minor)
- Axis color coding (X=Red, Y=Green, Z=Blue)
- UI background and panel colors

## Modern Replacements

All functionality has been reimplemented in the Canvas UI system:
- **3D Viewport:** `canvas_ui/viewport_3d_editor.cpp`
- **UI Rendering:** `canvas_ui/canvas_renderer.cpp`
- **Event System:** `canvas_ui/event_router.cpp`
- **Widget System:** `canvas_ui/ui_widgets.cpp`

The new system provides the same professional functionality with:
- Modern OpenGL rendering
- SDF-based UI widgets
- Efficient batch rendering
- Event-driven architecture
- Blender-inspired UX patterns