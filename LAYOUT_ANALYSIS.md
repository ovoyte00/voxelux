# Layout System Analysis - CSS Flexbox Comparison

## Executive Summary
After reviewing our multi-pass layout implementation against CSS flexbox specifications, I've identified several critical issues causing incorrect positioning and sizing of dock containers.

## Multi-Pass Layout Algorithm Comparison

### CSS Browser Engine Process
1. **Style Computation**: Apply cascading styles, compute final values
2. **Intrinsic Size Calculation**: Bottom-up calculation of content sizes
3. **Layout**: Top-down size resolution and positioning
4. **Paint**: Render to screen

### Our Implementation
1. **Style Computation**: ✅ Working correctly
2. **Intrinsic Size Calculation**: ⚠️ Partially correct (issues with flex items)
3. **Size Resolution**: ❌ Major issues identified
4. **Position Children**: ⚠️ Issues with flex positioning

## Critical Issues Found

### 1. Flex Container Width Calculation (CRITICAL BUG)
**Location**: `styled_widget.cpp:1983-1985`
```cpp
// INCORRECT - Row flex sets cross_size as main_size
if (is_row) {
    child_bounds = Rect2D(main_pos, item_cross_pos, item->main_size, item->cross_size);
} else {
    child_bounds = Rect2D(item_cross_pos, main_pos, item->cross_size, item->main_size);
}
```

**CSS Specification**: In a row flex container, children should use:
- Width: `item->main_size` (grows with flex-grow)
- Height: `item->cross_size` (stretches to container height by default)

**Our Bug**: We're correctly setting the bounds, but the flex item calculation is wrong.

### 2. Intrinsic Size Calculation for Flex Items
**Issue**: Flex items in a row should NOT constrain their preferred width when they have flex-grow.

**Current Code** (`calculate_intrinsic_sizes`):
```cpp
// For flex items with flex-grow, we're returning intrinsic width
// This causes the parent to think the child only needs its content width
```

**CSS Spec**: Flex items with flex-grow should signal they can use available space.

### 3. AUTO Width Resolution Issue
**Location**: `resolve_sizes:396-399`
```cpp
} else if (computed_style_.width_unit == WidgetStyle::ComputedStyle::AUTO) {
    // If parent has already set our width (e.g., through flex stretch), keep it
    // Otherwise use intrinsic width
    resolved_width = (bounds_.width > 0) ? bounds_.width : intrinsic.preferred_width;
}
```

**Problem**: This logic is correct, but the parent flex container isn't setting the width properly for the main-axis.

### 4. Percentage Width in Column Flex
**Location**: `layout_flex:1656-1658`
```cpp
if (child_style.width_unit == WidgetStyle::ComputedStyle::PERCENT) {
    // Use the parent's content width * percentage
    item.cross_size = content_bounds.width * (child_style.width_percent / 100.0f);
}
```

**Issue**: This is correct for cross-axis sizing, but we're not handling AUTO width correctly in column layouts.

### 5. Align-Items Default
**CSS Spec**: Default is `stretch`
**Our Implementation**: ✅ Correctly defaults to stretch (checked in `widget_style.h`)

### 6. Main Content Positioning Bug
**Symptom**: Right dock at x=499 instead of x=1564
**Root Cause**: The main-content widget has flex-grow:1 but its width is being calculated as intrinsic width (499px) instead of filling available space.

## The Core Problem

The issue is in how flex items communicate their size requirements up the tree:

1. **DockContainer** has `width: auto` 
2. Its intrinsic width is calculated from children (36px per column)
3. **MainContent** has `flex-grow: 1` and `width: 0` (initial)
4. Its intrinsic width is calculated from its children (~499px from center-area)
5. **Parent flex container** distributes space:
   - Left dock: Gets 72px (its intrinsic)
   - Main content: Should get remaining space but only gets 499px
   - Right dock: Gets 72px but positioned at wrong x coordinate

## Required Fixes

### Fix 1: Flex Item Width Calculation
In `calculate_intrinsic_sizes()`, flex items with flex-grow in row containers should return:
- `min_width`: Actual minimum content width
- `preferred_width`: 0 (to indicate it wants to grow)
- `max_width`: infinity

### Fix 2: Flex Container Space Distribution
The flex container needs to:
1. Calculate total intrinsic size of non-flex-grow items
2. Subtract from available space
3. Distribute remaining space to flex-grow items

### Fix 3: Position Calculation
After calculating sizes, positions need to account for actual resolved widths, not intrinsic widths.

## CSS Flexbox Rules We're Violating

1. **Flex items become flex containers**: ✅ Correct
2. **Default align-items: stretch**: ✅ Correct in theory, ❌ not working in practice
3. **Flex-grow: 1 fills available space**: ❌ Not working for main-content
4. **Min-width: auto for flex items**: ❌ Not implemented
5. **Width: auto resolution**: ⚠️ Partially working

## Next Steps

1. Fix intrinsic size calculation for flex items with flex-grow
2. Fix flex container space distribution algorithm
3. Ensure AUTO width respects flex-grow properly
4. Fix position calculation to use resolved widths
5. Add min-width: auto behavior for flex items