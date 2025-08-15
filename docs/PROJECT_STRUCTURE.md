# Voxelux Project Structure

## Overview
Voxelux is a professional 3D voxel editor built with C++20, featuring a custom OpenGL rendering pipeline and a proprietary UI framework inspired by Blender's modular architecture.

## Directory Structure

```
voxelux/
├── bin/                    # Compiled executables
├── build/                  # CMake build directory (generated)
├── docs/                   # Documentation
├── include/                # Public header files
├── lib/                    # Third-party libraries
├── shaders/                # GLSL shader files
├── src/                    # Source code
├── tests/                  # Test suite
└── assets/                 # Resources and assets (future)
```

## Detailed File System

### Root Configuration Files
```
/
├── CMakeLists.txt              # Main CMake configuration
├── README.md                   # Project overview
├── CLAUDE.md                   # Claude Code instructions
└── IMPLEMENTATION_PLAN.md      # Development roadmap
```

### Documentation (`/docs`)
```
docs/
├── ghost_trackpad_study.md     # Research on trackpad input handling
└── viewport_navigation_report.md # 3D navigation implementation notes
```

### Headers (`/include`)

#### Canvas UI System (`/include/canvas_ui`)
Core UI framework components:
```
canvas_ui/
├── camera_3d.h                 # Professional 3D camera with quaternion support
├── canvas_core.h               # Core canvas functionality
├── canvas_region.h             # Region management for multi-viewport
├── canvas_renderer.h           # OpenGL rendering abstraction
├── canvas_window.h             # Window management and input handling
├── event_router.h              # Event distribution system
├── font_system.h               # Text rendering system
├── grid_3d_renderer.h          # Shader-based 3D grid rendering
├── navigation_widget.h         # 3D navigation cube widget
├── polyline_shader.h           # Line rendering shader system
├── ui_widgets.h                # UI component library
├── viewport_3d_editor.h        # 3D viewport editor space
├── viewport_navigation_handler.h # Input handling for 3D navigation
└── viewport_navigator.h        # Navigation state management
```

#### Core Engine (`/include/voxelux/core`)
Core voxel engine components:
```
core/
├── event.h                     # Event system base
├── events.h                    # Event type definitions
├── simple_event.h              # Lightweight event implementation
├── vector3.h                   # 3D vector mathematics
├── voxel.h                     # Voxel data structure
└── voxel_grid.h                # Voxel grid container
```

#### Platform Layer (`/include/voxelux/platform`)
Platform-specific abstractions:
```
platform/
└── native_input.h              # Native input handling (macOS trackpad, etc.)
```

#### Graphics (`/include`)
OpenGL and graphics utilities:
```
├── glad/gl.h                   # OpenGL loader
└── KHR/khrplatform.h          # Khronos platform definitions
```

### Source Code (`/src`)

#### Canvas UI Implementation (`/src/canvas_ui`)
```
canvas_ui/
├── CMakeLists.txt              # Canvas UI module build config
├── camera_3d.cpp               # 3D camera implementation with quaternions
├── canvas_core.cpp             # Canvas system core
├── canvas_region.cpp           # Region splitting and management
├── canvas_renderer.cpp         # OpenGL rendering implementation
├── canvas_window.cpp           # GLFW window and input processing
├── event_router.cpp            # Event routing implementation
├── font_system.cpp             # FreeType font rendering
├── grid_3d_renderer.cpp        # 3D grid with XY/YZ plane support
├── navigation_widget.cpp       # Navigation cube implementation
├── polyline_shader.cpp         # Shader-based line rendering
├── ui_widgets.cpp              # UI widget implementations
├── viewport_3d_editor.cpp      # 3D viewport with grid and navigation
├── viewport_navigation_handler.cpp # Mouse/trackpad navigation handling
└── viewport_navigator.cpp      # Camera navigation state machine
```

#### Core Engine (`/src/core`)
```
core/
├── CMakeLists.txt              # Core module build config
└── voxel_grid.cpp              # Voxel grid implementation
```

#### Platform Layer (`/src/platform`)
```
platform/
├── CMakeLists.txt              # Platform module build config
├── native_input_macos.mm       # macOS-specific input (Objective-C++)
└── native_input_stub.cpp       # Fallback implementation
```

#### Main Application
```
src/
├── CMakeLists.txt              # Main app build config
└── voxelux_canvas.cpp          # Application entry point
```

### Shaders (`/shaders`)
GLSL shader programs:
```
shaders/
├── vertex.vert                 # Basic vertex shader
├── fragment.frag               # Basic fragment shader
└── grid/
    ├── grid.vert               # 3D grid vertex shader
    └── grid.frag               # 3D grid fragment shader with plane support
```

### Third-Party Libraries (`/lib`)
```
lib/
├── blender/                    # Blender reference (clean-room study only)
├── glad/                       # OpenGL loader
├── glfw/                       # Window and input library
├── glm/                        # OpenGL mathematics
├── imgui/                      # Dear ImGui (future UI elements)
├── freetype/                   # Font rendering
└── stb/                        # STB single-header libraries
```

### Tests (`/tests`)
```
tests/
├── CMakeLists.txt              # Test suite configuration
└── test_placeholder.cpp        # Placeholder test file
```

## Key Components

### 1. Custom Canvas UI Framework
- **Purpose**: Professional multi-viewport 3D editing interface
- **Features**: 
  - Blender-inspired modular regions
  - Any region can be any editor type
  - Custom OpenGL rendering pipeline
  - Event-driven architecture

### 2. Professional 3D Camera System
- **Camera3D**: Quaternion-based camera with multiple navigation modes
- **Features**:
  - Orbit and FPS navigation modes
  - Turntable and trackball rotation
  - Pre-calculated quaternions for axis views (Front/Back/Top/Bottom/Left/Right)
  - Smooth transitions between views

### 3. 3D Grid System
- **Grid3DRenderer**: Shader-based infinite grid
- **Features**:
  - Horizontal XZ plane (always visible)
  - Vertical grids (XY and YZ planes) for side views
  - Distance-based fading
  - Professional appearance matching industry standards

### 4. Navigation Widget
- **NavigationWidget**: 3D orientation cube
- **Features**:
  - Axis-aligned view shortcuts
  - Visual feedback for camera orientation
  - Drag-to-rotate functionality
  - Professional appearance with depth-based rendering

### 5. Input System
- **Multi-device Support**:
  - Mouse with wheel
  - Trackpad with gestures (macOS)
  - Magic Mouse support
  - Modifier-based navigation (Shift+drag for pan, etc.)

## Build System

### CMake Configuration
- Minimum version: 3.20
- C++20 standard
- Modular architecture with static libraries
- Platform-specific configurations

### Build Commands
```bash
# Configure
cmake -B build

# Build Debug
cmake --build build

# Build Release
cmake --build build --config Release

# Run
./bin/voxelux
```

## Architecture Principles

1. **Event-Driven**: Entire system built on events for scalability
2. **Modular Design**: Specialized modules extend core functionality
3. **Clean-Room Development**: Reference implementations studied, original code written
4. **Professional UX**: Matching industry standards (Blender, Maya, etc.)
5. **Performance First**: Custom rendering pipeline for maximum control

## Current State

### Implemented
- ✅ Custom Canvas UI framework with region management
- ✅ Professional 3D camera with quaternion rotations
- ✅ 3D grid rendering with multiple planes
- ✅ Navigation widget with axis views
- ✅ Multi-device input handling
- ✅ Viewport navigation (orbit, pan, zoom)
- ✅ Clean build system

### In Progress
- 🚧 Voxel rendering system
- 🚧 Voxel editing tools
- 🚧 Material system

### Planned
- 📋 Minecraft module with NBT support
- 📋 Animation system
- 📋 Plugin API
- 📋 Real-time collaboration
- 📋 Unity/Unreal export modules

## Development Notes

- **Clean-Room Process**: Blender source in `/lib/blender/` for reference only
- **No Debug Logs**: All debug output has been removed for clean operation
- **Quaternion System**: All rotations use quaternions to avoid gimbal lock
- **Coordinate System**: Right-handed, Y-up, Z-towards viewer