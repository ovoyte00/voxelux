# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Voxelux is a state-of-the-art professional voxel 3D editor with a sleek, modern interface designed to compete with leading tools like Qubicle. Built with modern C++20, it features a modular architecture where specialized modules (Minecraft, Unity, Unreal, etc.) extend the core voxel editing capabilities. The core focuses on general voxel 3D editing with a professional, modern aesthetic.

### Key Technologies
- **Language**: C++20 (with modern syntax requirements)
- **Build System**: CMake (minimum version 3.20)
- **Graphics**: Custom OpenGL rendering pipeline with proprietary framework
- **Architecture**: Event-driven system with planned plugin support
- **Monetization**: Subscription-based model

### C++ Coding Standards

**Compiler Requirements**:
- All warnings must be treated as errors (`-Werror` / `/WX`)
- Enable all reasonable warnings (`-Wall -Wextra` / `/W4`)
- Code must compile with zero warnings
- Use `[[maybe_unused]]` to suppress intentional unused parameter warnings
- Use explicit casts to suppress intentional conversion warnings

**Modern C++20 Requirements**:
- Use modern C++20 features and idioms
- All variable initialization MUST use direct list initialization: `int x{}` or `int x{1}`
- NEVER use `=` for initialization: ~~`int x = 0`~~ → `int x{0}`
- Prefer `auto` with brace initialization where appropriate: `auto value{42}`
- Use uniform initialization for all types including containers: `std::vector<int> v{1, 2, 3}`
- Member variables in classes: `int width_{}; float scale_{1.0f};`
- Function parameters can still use `=` for default values: `void func(int x = 0)`
- Use designated initializers: `Point p{.x = 10, .y = 20}`
- Prefer `std::string_view` over `const std::string&` for string parameters
- Use concepts and ranges where appropriate
- Variables and function parameters that are defined but never used MUST have the `[[maybe_unused]]` attribute

**Comment Style Requirements**:
- Comments above functions explain **WHAT** the function does
- Comments above statements/expressions explain **WHY** it's being done, not what is obvious from the code
- Bad: `// Increment x` above `x++`
- Good: `// Need to advance past the header bytes` above `x++`
- Function example:
  ```cpp
  // Calculates the viewport transformation matrix for rendering
  Matrix4 calculate_viewport_transform() { ... }
  ```
- Inline example:
  ```cpp
  // Must clear before resize to avoid stale bounds affecting layout
  computed_style_ = WidgetStyle::ComputedStyle{};
  ```

### Core Features

**Core Voxel Engine**
- Advanced block modeling and structure creation with sleek, modern UI
- Material assignment system with texture mapping
- Blender-like features: masking, modifiers, splines, particles
- Professional rendering with real-time lighting preview
- Modern, intuitive interface design

**Minecraft Integration Module**
- Full NBT data preservation for .schem/.nbt exports
- Smart block placement with automatic blockstate adjustment
- Special blocks (torches, rails, etc.) with intelligent alignment
- Layer-by-layer blueprint exports with build order data
- Macro editing capabilities for structures and entire worlds
- Animation system for build previews and walkthroughs
- Professional rendering with Minecraft lighting simulation
- World file upload and editing capabilities

**Project-Specific Workflows**
- Generic Voxel Art projects
- Minecraft Structure projects (individual buildings/constructions)
- Minecraft World projects (entire world editing with terrain tools)
- Minecraft Animation projects (cinematic previews and showcases)
- Unity Asset pipeline
- Unreal Engine export compatibility

## Build System

The project uses CMake with a minimum version requirement of 3.20.

### Common Commands

```bash
# Configure the build
cmake -B build

# Build the project
cmake --build build

# Build in release mode
cmake --build build --config Release
```

## Architecture

The project follows a modular C++ architecture:

- **src/core/**: Core engine functionality and data structures
- **src/renderer/**: Rendering system components
- **src/canvas_ui/**: Custom UI framework developed for Voxelux
- **include/**: Public header files
- **tests/**: Test suite (empty currently)
- **assets/**: Resources and assets
- **docs/**: Documentation (empty currently)
- **lib/**: Third-party libraries

## Custom UI System

Voxelux features a completely custom-developed Canvas UI framework built from the ground up with our own proprietary rendering and layout system. Inspired by Blender's multi-use modular architecture, our custom framework allows every region to be any use type - enabling users to configure their workspace with any combination of viewports, panels, and editors. This custom approach gives us complete control over performance optimization, rendering pipeline, and user interaction patterns, providing maximum flexibility for professional workflows where users can adapt the interface to their specific needs.

## Blender Reference Implementation - Clean-Room Approach

**IMPORTANT**: This project has a reference implementation available at `./lib/blender/` for studying 3D navigation and UI patterns. When implementing viewport controls, UI patterns, or 3D navigation features, follow the clean-room development process below.

### Clean-Room Development Process (MANDATORY)

When referencing Blender or any GPL-licensed code, you MUST follow this strict three-phase approach:

#### Phase 1: Read and Study
- Study the Blender implementation to understand concepts, algorithms, and approaches
- Focus on understanding the mathematical concepts and high-level logic
- Create a detailed report documenting the concepts WITHOUT using any original text, variable names, or function names
- Document the problem being solved and the approach taken
- Identify key mathematical formulas and transformations

#### Phase 2: Research Industry Standards
- Research industry-standard approaches to solve the same problems
- Look for MIT-licensed or public domain implementations for reference
- Study academic papers, tutorials, and documentation about the concepts
- Compare different approaches and understand trade-offs
- Document alternative solutions and why certain approaches are preferred

#### Phase 3: Original Implementation
- Implement the solution using completely original code
- Use different variable names, function names, and code structure
- Apply the understood concepts but express them in your own way
- Add your own optimizations and improvements
- Ensure the implementation is license-violation-proof and completely independent

**This clean-room approach ensures**:
- We learn from proven solutions
- We avoid any licensing violations
- Our code is completely original and defensible
- We understand the "why" behind implementations, not just the "how"
- We can improve upon existing solutions

### Key Blender Reference Files:
- **Viewport Navigation**: `/lib/blender/source/blender/editors/space_view3d/view3d_navigate.cc`
- **Camera Controls**: `/lib/blender/source/blender/editors/space_view3d/view3d_gizmo_navigate.cc`
- **UI Layout System**: `/lib/blender/source/blender/editors/interface/interface_layout.cc`
- **Theme System**: `/lib/blender/source/blender/editors/interface/resources.cc`
- **Workspace Management**: `/lib/blender/source/blender/editors/screen/workspace_*.cc`
- **Viewport Splitting**: `/lib/blender/source/blender/editors/screen/screen_edit.cc`
- **Grid System**: `/lib/blender/source/blender/editors/space_view3d/view3d_draw.cc`
- **3D Viewport**: `/lib/blender/source/blender/editors/space_view3d/space_view3d.cc`

### Blender's Exact Controls (Reference Implementation):
- **Middle Mouse Drag** = Orbit camera around object
- **Shift + Middle Mouse** = Pan view  
- **Mouse Wheel** = Zoom in/out
- **Left Mouse** = Select objects (no camera movement)
- **Numpad 1-9** = Predefined camera views
- **G** = Grab/Move, **R** = Rotate, **S** = Scale
- **Home/Period** = Frame all objects

### Implementation Guidelines:
1. **Study Blender First**: Before implementing any 3D viewport feature, examine how Blender does it
2. **Copy Patterns, Not Code**: Use their architectural patterns and algorithms, but write original code
3. **Match Behavior**: Controls should feel identical to Blender for professional users
4. **Reference Camera Math**: Use their camera matrix calculations and projection setups as reference
5. **UI Consistency**: Follow their color schemes, spacing, and interaction patterns

### Blender Color Palette (for UI theming):
- Background: `#2b2b2b` (43, 43, 43)
- Header: `#323232` (50, 50, 50)
- Selection: `#5680c2` (86, 128, 194)
- Text: `#cccccc` (204, 204, 204)
- Grid: `#3c3c3c` with distance-based fading

## Development Notes

**Event-Driven Architecture**
The entire system is built on event-driven programming for maximum speed and scalability. Currently using SimpleEvent system for rapid development, designed to be upgradeable to a robust event system as the application scales. This architecture enables real-time collaboration, responsive UI, and efficient plugin communication.

**Modular Architecture**
Voxelux starts with a powerful core voxel editing engine, extended by specialized modules:

**Core Engine**: Universal voxel 3D editing with modern, sleek interface
- Professional modeling tools
- Advanced rendering pipeline
- Animation capabilities
- Modern UI/UX design

**Specialized Modules** (planned):
- **Minecraft Module**: World editing, NBT preservation, blockstate management
- **Game Engine Modules**: Unity, Unreal Engine export pipelines
- **Animation Module**: Cinematic content creation
- **Collaboration Module**: Real-time team editing with enterprise features

**Plugin API System** (planned):
- **Developer API**: Robust C++ API for third-party plugin development
- **Paid Access Model**: Premium API access for professional developers
- **Closed Source**: Core remains proprietary while enabling extensibility
- **Plugin Marketplace**: Curated ecosystem of professional plugins
- **Event-Based Integration**: Plugins integrate through the event system
- **Sandboxed Execution**: Safe plugin execution environment

**Real-Time Collaboration System** (enterprise feature):
- **Multi-User World Editing**: Simultaneous editing of large Minecraft worlds
- **Conflict Resolution**: Intelligent merging of concurrent edits
- **User Permissions**: Role-based access control (Admin, Editor, Viewer)
- **Live Cursors**: See team members' active editing locations
- **Version Control**: Built-in versioning with rollback capabilities
- **Team Management**: Enterprise user management and billing
- **Cloud Sync**: Automatic world synchronization across team members
- **Audit Logging**: Track all changes for enterprise compliance

This creates a professional voxel editing platform with both developer ecosystem and enterprise collaboration capabilities.

**Key Technical Requirements**
- Event-driven architecture for performance and scalability (upgradeable from SimpleEvent to robust system)
- Modular plugin system for specialized use cases
- Professional Plugin API with paid developer access
- Completely custom UI/UX framework with proprietary rendering system
- Custom OpenGL rendering pipeline with real-time lighting (no external graphics APIs)
- Animation system for previews and cinematic content
- Cross-platform deployment (Windows, macOS, Linux)
- **Multi-tier monetization**: Individual subscriptions + Team/Enterprise plans + Plugin API licensing
- Professional-grade tools competing with industry standards like Qubicle

**Development Priority**: 
1. Core voxel editing engine
2. Specialized modules (Minecraft, Unity, etc.)
3. Plugin API framework and developer ecosystem
4. Real-time collaboration system for enterprise
5. Upgrade to robust event system as scale demands

**Monetization Tiers**:
- **Individual**: Basic voxel editing and personal projects
- **Professional**: Advanced features, plugin access, commercial use
- **Team**: Multi-user collaboration, shared cloud storage
- **Enterprise**: Advanced collaboration, user management, audit logging, SLA support

**Plugin Development Strategy**:
- C++ API exposing core functionality without source code
- Event-driven plugin communication for loose coupling
- Revenue sharing model for plugin marketplace
- Professional developer support and documentation

**Collaboration Architecture Requirements**:
- Network layer for real-time synchronization
- Conflict resolution algorithms for concurrent edits
- WebSocket or custom protocol for low-latency communication
- Cloud infrastructure for world hosting and sync
- Enterprise-grade security and user management