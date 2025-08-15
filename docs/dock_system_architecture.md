# Voxelux Dock System Architecture

## Overview
The Voxelux dock system is a flexible, multi-column docking framework inspired by Adobe Illustrator's panel management. It allows users to organize their workspace with draggable, dockable, and floating panels arranged in columns around the main content area.

## Core Concepts

### Dock Columns
- **Definition**: Vertical containers that hold panel groups on the left/right sides of the application
- **Behavior**: 
  - Automatically created when panels are dragged to dock zones
  - Automatically destroyed when empty (with animation)
  - Support multiple columns per side (left/right)
  - Resizable width: 250-500px (expanded), 36-225px (collapsed)
  - Tool dock special case: Fixed 72px (expanded), 36px (collapsed)

### Panel Groups
- **Definition**: Container holding one or more panels with tabs
- **Components**:
  - Grip handle for dragging (visible in collapsed state)
  - Tab bar (visible in expanded state)
  - Panel content area
- **Behavior**:
  - Can be dragged between dock columns
  - Can be reordered within a column
  - Height resizable within column
  - Automatically created when single panel is docked

### Panels
- **Definition**: Individual UI components (Properties, Layers, Tools, etc.)
- **States**:
  - Docked in panel group
  - Floating as window
  - Icon in collapsed dock
- **Special Types**:
  - Tool Panel: Cannot share dock column with other panels
  - Bottom Panels: Timeline, Console, etc. - only dock to bottom

## Dock States

### Expanded Dock Column
```
┌──────────────────────┐
│ [Tab1][Tab2][Tab3]   │ <- Panel group tabs
├──────────────────────┤
│                      │
│   Panel Content      │
│                      │
├──────────────────────┤ <- 2px gap (resize handle)
│ [Tab1][Tab2]         │ <- Second panel group
├──────────────────────┤
│   Panel Content      │
└──────────────────────┘
Width: 250-500px (resizable)
```

### Collapsed Dock Column
```
┌────┐
│ ≡  │ <- Grip icon
├────┤
│ 📄 │ <- Panel icon
│ 🎨 │
│ 📊 │
├────┤ <- Panel group separator
│ ≡  │ <- Second group grip
├────┤
│ 🔍 │
│ ⚙️ │
└────┘
Width: 36-225px (resizable)
```

### Tool Dock (Special)
- **Collapsed**: 36px wide, vertical tool icons
- **Expanded**: 72px wide, 2-column tool grid
- **Cannot resize**: Fixed widths only
- **Exclusive**: Creates separate dock column, won't merge with others

## Drag & Drop System

### Drag Initiation
1. **By Grip Handle**: Drags entire panel group
2. **By Panel Icon**: Extracts single panel from group
3. **By Tab**: Drags single panel
4. **By Utility Header** (floating): Moves floating window

### Drop Zones & Visual Feedback

#### Valid Drop Zones
1. **Between Dock Columns**: Light blue vertical line indicator
2. **Edge of Screen**: Light blue border highlight
3. **Within Dock Column**: Shaded ghost preview
4. **Between Panel Groups**: Blue horizontal line
5. **Into Panel Group**: Blue outline around target group

#### Visual States During Drag
- **Valid drop zone**: Panel becomes translucent (ghost mode)
- **Invalid drop zone**: Panel remains solid, acts as floating window
- **Tool panel special**: Only shows indicators at valid zones (edges, between columns)

### Drop Rules
- Regular panels can dock anywhere except tool dock columns
- Tool panels create new columns, cannot join existing panel groups
- Single panels auto-create panel group when docked
- Empty panel groups are destroyed
- Empty dock columns animate closed

## Floating Panel Windows

### Structure
```
┌─────────────────[×]─┐ <- Utility header (draggable)
├──[Tab1][Tab2]───────┤ <- Tabs (even if single panel)
│                     │
│   Panel Content     │
│                     │
└─────────────────────┘
```

### Behavior
- No title bar, minimal utility header
- Draggable by utility header or empty tab area
- Resizable window
- Close button (×) in top-right
- Can contain multiple tabs (panel group)
- Remembers size/position

## Bottom Dock System

### Characteristics
- **Orientation**: Horizontal rows (not columns)
- **Supported Views**: Timeline, Dopesheet, Console, Output
- **No Collapse State**: Panels are either present or removed
- **Stacking**: Multiple rows can stack vertically
- **Resizing**: Each row height adjustable (drags main content up)

### Bottom Panel Structure
```
┌────────────────────────────────────────┐
│ [Dropdown ▼] Panel Name    [─][×]      │ <- View header (like editor)
├────────────────────────────────────────┤
│                                        │
│         Panel Content                  │
│                                        │
└────────────────────────────────────────┘
```

## Interaction Patterns

### Collapsed Dock Interactions
1. **Click Icon**: Shows popout of panel
2. **Click Different Icon**: Previous popout closes, new one opens
3. **Drag Icon**: Detaches panel as floating window
4. **Expand Button**: Expands entire dock column

### Expanded Dock Interactions
1. **Click Tab**: Switches active panel
2. **Drag Tab**: Detaches panel
3. **Drag Grip**: Moves entire panel group
4. **Resize Handle**: Adjusts panel group height
5. **Collapse Button**: Collapses entire dock column

### Popout Behavior
- **Trigger**: Click icon in collapsed dock
- **Position**: Adjacent to dock column
- **Persistence**: Stays open until:
  - User clicks close (×)
  - User clicks different icon in same dock
  - User drags panel to dock
- **No Auto-hide**: Clicking elsewhere keeps popout visible

## Layout Rules

### Space Management
- Main content minimum width: Sum of editor view minimums (100px each)
- Dock columns appear/disappear based on content
- Animation on column show/hide
- Main content expands to fill available space

### Constraints
- No limit on dock columns (space permitting)
- Panel groups minimum height: 100px
- Dock column collapsed max: 225px
- Dock column expanded min: 250px, max: 500px
- Tool dock fixed: 36px or 72px only

## State Persistence

### Saved State
- Dock column positions and widths
- Panel group arrangements
- Collapsed/expanded states
- Panel tab order
- Active tabs
- Tool selection

### Fresh File State
- Dock layout preserved from last session
- Main content resets to single view
- Future: Workspace presets/templates

## Implementation Priority

### Phase 1: Multi-Column Support
- Refactor DockPanel to DockColumn class
- Implement DockContainer to manage multiple columns
- Add column creation/destruction with animation
- Support variable column widths

### Phase 2: Drag & Drop System
- Implement drag detection (grip, icon, tab)
- Create ghost/preview rendering
- Add drop zone detection and highlighting
- Implement panel extraction and insertion

### Phase 3: Floating Panels
- Create FloatingWindow class
- Implement utility header with close button
- Add window movement and resizing
- Support docking from floating state

### Phase 4: Bottom Dock
- Create BottomDock with row support
- Implement view-style headers
- Add row stacking and resizing
- Integrate Timeline, Console views

### Phase 5: Polish & Persistence
- Add animations for all transitions
- Implement state serialization
- Add workspace preset system
- Optimize performance

## Technical Considerations

### Event System
- Drag events need capture for reliable tracking
- Drop zones need z-order management
- Animation timing coordination required
- State changes must trigger layout recalculation

### Rendering
- Ghost mode requires alpha blending
- Drop indicators need overlay rendering
- Animations require frame-based updates
- Efficient damage tracking for redraws

### Memory Management
- Panel ownership during drag operations
- Cleanup when columns are destroyed
- Floating window lifecycle management
- Event handler registration/cleanup

## Color Specifications
- Drop indicators: `--accent-primary` (#3B83BE)
- Ghost mode: 50% opacity
- Resize handles: `--gray-4` (#393939)
- Active resize: `--accent-primary`
- Panel borders: `--gray-5` (#303030)