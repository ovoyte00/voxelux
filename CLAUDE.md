# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Voxelux is a state-of-the-art professional voxel 3D editor with a sleek, modern interface designed to compete with leading tools like Qubicle. Built with modern C++20, it features a modular architecture where specialized modules (Minecraft, Unity, Unreal, etc.) extend the core voxel editing capabilities. The core focuses on general voxel 3D editing with a professional, modern aesthetic.

### Key Technologies
- **Language**: C++20
- **Build System**: CMake (minimum version 3.20)
- **Graphics**: OpenGL/Vulkan rendering pipeline
- **Architecture**: Event-driven system with planned plugin support
- **Monetization**: Subscription-based model

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
- **include/**: Public header files
- **tests/**: Test suite (empty currently)
- **assets/**: Resources and assets
- **docs/**: Documentation (empty currently)
- **lib/**: Third-party libraries

## Blender Reference Implementation

**IMPORTANT**: This project has a reference implementation available at `./lib/blender/` for studying 3D navigation and UI patterns. When implementing viewport controls, UI patterns, or 3D navigation features, reference this implementation for architectural guidance.

**CRITICAL LICENSING REQUIREMENT**: When referencing the source code:
- NEVER use function names, variable names, or comments from the reference code
- NEVER mention the reference project name in code comments or identifiers  
- Study patterns and algorithms, then write completely original implementations
- All code must be license-violation-proof and completely independent

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
- Modern, sleek UI/UX with professional aesthetic
- Advanced rendering pipeline with real-time lighting
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