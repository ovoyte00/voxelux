# GHOST Trackpad Event Handling Study

## Phase 1: Understanding GHOST Architecture

### Event Types
GHOST (Generic Handy Operating System Toolkit) uses a hierarchical event system:
- Base `GHOST_Event` class
- Specialized events like `GHOST_EventTrackpad`, `GHOST_EventWheel`
- Trackpad events have subtypes: Scroll, Rotate, Magnify, SmartMagnify

### Key Patterns Observed

1. **Phase Detection for Device Type**
   - Uses `NSEventPhase` to detect trackpad vs mouse wheel
   - Phase != NSEventPhaseNone indicates trackpad/gesture device
   - Phase == NSEventPhaseNone indicates traditional mouse wheel

2. **Delta Handling**
   - Trackpad: Uses `scrollingDeltaX/Y` for smooth scrolling
   - Mouse wheel: Uses discrete `deltaX/Y` values
   - Special case: WACOM tablets use old deltas with zero phase/momentum

3. **Momentum Handling**
   - Tracks momentum phase separately from gesture phase
   - Can ignore momentum after key presses to prevent mode changes
   - Uses static state to track multi-touch scroll status

4. **Coordinate Systems**
   - Converts points to backing coordinates for high-DPI displays
   - Tracks both screen position and delta movements
   - Preserves direction inversion from system preferences

## Phase 2: Industry Standard Approaches

### Apple NSEvent Documentation
- `scrollingDeltaX/Y`: Pixel-precise deltas for trackpad
- `deltaX/Y`: Line-based deltas for mouse wheel
- `hasPreciseScrollingDeltas`: Indicates trackpad vs wheel
- `isDirectionInvertedFromDevice`: Natural scrolling preference

### Common Patterns
1. **Gesture Recognition**
   - Begin/Changed/Ended phases for gesture tracking
   - Momentum phase for inertial scrolling
   - Stationary phase for held gestures

2. **Modifier Key Handling**
   - Shift typically converts vertical to horizontal scroll
   - Command/Ctrl for zoom operations
   - Option/Alt for alternative behaviors

3. **Device Differentiation**
   - Trackpad: Continuous, precise deltas
   - Mouse wheel: Discrete, line-based deltas
   - Magic Mouse: Hybrid behavior with precise deltas

## Phase 3: Clean Implementation Strategy

### Core Concepts

1. **Event Capture Layer**
   - Monitor NSEvent at system level
   - Capture raw deltas before OS processing
   - Preserve modifier state with event

2. **Device Detection**
   ```
   if (event.phase != NSEventPhaseNone || event.momentumPhase != NSEventPhaseNone) {
       // Trackpad or gesture-capable device
       use scrollingDeltaX/Y for smooth movement
   } else {
       // Traditional mouse wheel
       use deltaX/Y for discrete steps
   }
   ```

3. **Pan Direction Handling**
   - Capture both X and Y deltas from scrollingDelta
   - Don't rely on conversion - use raw values
   - Apply modifiers at application level, not OS level

4. **Momentum Management**
   - Track gesture phases to identify momentum
   - Allow momentum for zoom/rotate
   - Optionally suppress momentum for pan operations

### Implementation Requirements

1. **Native Event Structure**
   - Store both scrolling and discrete deltas
   - Include phase information
   - Preserve modifier flags
   - Track device type

2. **Event Processing**
   - Use phase to determine device type
   - Select appropriate delta source
   - Apply application-level modifier logic
   - Handle momentum based on current mode

3. **Coordinate Transformation**
   - Convert to backing coordinates for Retina
   - Transform screen space to world space
   - Maintain consistent scaling factors

### Key Differences from Current Implementation

1. **Delta Source**: Should use `scrollingDeltaX/Y` for trackpad, not `deltaX/Y`
2. **Phase Tracking**: Must track gesture phases to properly identify device
3. **Modifier Logic**: Apply at application level after capturing raw deltas
4. **Momentum Handling**: Separate logic for different navigation modes

## Recommendations

1. Extend native event capture to include:
   - Both scrolling and discrete deltas
   - Full phase information
   - Raw modifier state

2. Process events based on phase:
   - Use scrolling deltas for gesture devices
   - Use discrete deltas for mouse wheels
   - Handle momentum appropriately per mode

3. Apply modifier logic in application:
   - Don't rely on OS conversion
   - Implement custom shift+scroll behavior
   - Maintain separate X/Y handling

This approach will provide proper trackpad support while maintaining compatibility with traditional mouse input.