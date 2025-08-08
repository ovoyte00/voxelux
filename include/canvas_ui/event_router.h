/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Event routing and input handling
 * Professional event system with context-aware routing and modal operations.
 */

#pragma once

#include "canvas_core.h"
#include <memory>
#include <vector>
#include <functional>
#include <queue>

namespace voxel_canvas {

// Forward declarations
class CanvasWindow;
class CanvasRegion;

// Input context types for prioritized event handling
enum class InputContext {
    MODAL_DIALOG,      // Highest priority - modal dialogs capture all input
    POPUP_MENU,        // Popup menus and tooltips
    UI_WIDGET,         // UI buttons, sliders, text fields
    REGION_HEADER,     // Region headers and splitters
    EDITOR_TOOL,       // Active tool in editor (brush, select, etc.)
    EDITOR_NAVIGATION, // 3D viewport navigation
    GLOBAL_SHORTCUTS   // Global keyboard shortcuts
};

// Event handler with context and priority
class ContextualInputHandler : public InputHandler {
public:
    ContextualInputHandler(InputContext context, int priority = 0);
    virtual ~ContextualInputHandler() = default;
    
    InputContext get_context() const { return context_; }
    int get_priority() const override { return priority_; }
    
    // Context activation/deactivation
    virtual void activate() { active_ = true; }
    virtual void deactivate() { active_ = false; }
    bool is_active() const { return active_; }
    
protected:
    InputContext context_;
    int priority_;
    bool active_ = true;
};

/**
 * Routes input events through a prioritized chain of handlers.
 * Supports modal operations, context-sensitive handling, and event bubbling.
 */
class EventRouter {
public:
    explicit EventRouter(CanvasWindow* window);
    ~EventRouter();
    
    // Main event processing
    EventResult route_event(const InputEvent& event);
    
    // Handler management
    void add_handler(std::shared_ptr<ContextualInputHandler> handler);
    void remove_handler(std::shared_ptr<ContextualInputHandler> handler);
    void clear_handlers();
    
    // Modal operations
    void push_modal_handler(std::shared_ptr<ContextualInputHandler> handler);
    void pop_modal_handler();
    bool has_modal_handler() const { return !modal_stack_.empty(); }
    
    // Context management
    void activate_context(InputContext context);
    void deactivate_context(InputContext context);
    void set_context_priority(InputContext context, int priority);
    
    // Keyboard focus
    void set_keyboard_focus(std::shared_ptr<InputHandler> handler);
    void clear_keyboard_focus();
    bool has_keyboard_focus() const { return !keyboard_focus_.expired(); }
    
    // Mouse capture (for drag operations)
    void capture_mouse(std::shared_ptr<InputHandler> handler);
    void release_mouse_capture();
    bool has_mouse_capture() const { return !mouse_capture_.expired(); }
    
    // Event filtering and transformation
    using EventFilter = std::function<bool(const InputEvent&)>;
    void add_event_filter(EventFilter filter);
    void remove_event_filter(EventFilter filter);
    
    // Deferred events (processed after current event)
    void defer_event(const InputEvent& event);
    void process_deferred_events();
    
    // Debug and profiling
    void enable_event_logging(bool enable) { log_events_ = enable; }
    void log_handler_chain() const;
    
private:
    void sort_handlers();
    bool should_handle_event(const ContextualInputHandler* handler, const InputEvent& event) const;
    void log_event(const InputEvent& event, const std::string& handler_name, EventResult result) const;
    
    CanvasWindow* window_;
    
    // Handler chains
    std::vector<std::shared_ptr<ContextualInputHandler>> handlers_;
    std::vector<std::shared_ptr<ContextualInputHandler>> modal_stack_;
    
    // Special handlers
    std::weak_ptr<InputHandler> keyboard_focus_;
    std::weak_ptr<InputHandler> mouse_capture_;
    
    // Event processing
    std::vector<EventFilter> event_filters_;
    std::queue<InputEvent> deferred_events_;
    
    // State
    bool handlers_dirty_ = false; // Need to re-sort handlers
    bool processing_event_ = false;
    bool log_events_ = false;
};

/**
 * Specialized input handlers for common UI operations
 */

// Global keyboard shortcuts
class GlobalShortcutHandler : public ContextualInputHandler {
public:
    GlobalShortcutHandler();
    
    bool can_handle(const InputEvent& event) const override;
    EventResult handle_event(const InputEvent& event) override;
    
    // Shortcut registration
    using ShortcutCallback = std::function<void()>;
    void register_shortcut(int key_code, uint32_t modifiers, ShortcutCallback callback);
    void unregister_shortcut(int key_code, uint32_t modifiers);
    
private:
    struct Shortcut {
        int key_code;
        uint32_t modifiers;
        ShortcutCallback callback;
    };
    
    std::vector<Shortcut> shortcuts_;
};

// Region resizing and splitting
class RegionInteractionHandler : public ContextualInputHandler {
public:
    explicit RegionInteractionHandler(CanvasWindow* window);
    
    bool can_handle(const InputEvent& event) const override;
    EventResult handle_event(const InputEvent& event) override;
    
private:
    bool handle_region_resize(const InputEvent& event);
    bool handle_region_split(const InputEvent& event);
    bool is_on_region_splitter(const Point2D& point) const;
    
    CanvasWindow* window_;
    CanvasRegion* active_region_ = nullptr;
    bool is_resizing_ = false;
    Point2D resize_start_pos_;
};

// UI widget interaction (buttons, dropdowns, etc.)
class WidgetInteractionHandler : public ContextualInputHandler {
public:
    WidgetInteractionHandler();
    
    bool can_handle(const InputEvent& event) const override;
    EventResult handle_event(const InputEvent& event) override;
    
    // Widget registration
    class Widget;
    void register_widget(std::shared_ptr<Widget> widget);
    void unregister_widget(std::shared_ptr<Widget> widget);
    
private:
    std::vector<std::weak_ptr<Widget>> widgets_;
};

} // namespace voxel_canvas