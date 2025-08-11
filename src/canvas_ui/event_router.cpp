/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Event routing implementation
 */

#include "canvas_ui/event_router.h"
#include "canvas_ui/canvas_window.h"
#include <algorithm>
#include <iostream>

namespace voxel_canvas {

// ContextualInputHandler implementation

ContextualInputHandler::ContextualInputHandler(InputContext context, int priority)
    : context_(context)
    , priority_(priority)
    , active_(true) {
}

// EventRouter implementation

EventRouter::EventRouter(CanvasWindow* window)
    : window_(window)
    , handlers_dirty_(false)
    , processing_event_(false)
    , log_events_(false) {
}

EventRouter::~EventRouter() = default;

EventResult EventRouter::route_event(const InputEvent& event) {
    if (processing_event_) {
        // Defer event to avoid recursion
        defer_event(event);
        return EventResult::IGNORED;
    }
    
    processing_event_ = true;
    
    
    // Apply event filters first
    for (const auto& filter : event_filters_) {
        if (!filter(event)) {
            processing_event_ = false;
            return EventResult::IGNORED;
        }
    }
    
    EventResult result = EventResult::IGNORED;
    
    // Region routing is handled by CanvasWindow before events reach the EventRouter
    // So we don't need to route to regions here
    
    // Check modal handlers first (they capture all input)
    if (!modal_stack_.empty()) {
        auto& modal_handler = modal_stack_.back();
        if (modal_handler->is_active() && modal_handler->can_handle(event)) {
            result = modal_handler->handle_event(event);
            if (log_events_) {
                log_event(event, "Modal:" + std::to_string(static_cast<int>(modal_handler->get_context())), result);
            }
            processing_event_ = false;
            return result;
        }
    }
    
    // Check keyboard focus handler
    if (auto focused_handler = keyboard_focus_.lock()) {
        if (event.is_keyboard_event()) {
            result = focused_handler->handle_event(event);
            if (result == EventResult::HANDLED) {
                if (log_events_) {
                    log_event(event, "KeyboardFocus", result);
                }
                processing_event_ = false;
                return result;
            }
        }
    }
    
    // Check mouse capture handler
    if (auto capture_handler = mouse_capture_.lock()) {
        if (event.is_mouse_event()) {
            result = capture_handler->handle_event(event);
            if (result == EventResult::HANDLED) {
                if (log_events_) {
                    log_event(event, "MouseCapture", result);
                }
                processing_event_ = false;
                return result;
            }
        }
    }
    
    // Sort handlers if dirty
    if (handlers_dirty_) {
        sort_handlers();
        handlers_dirty_ = false;
    }
    
    // Route through normal handler chain
    for (auto& handler : handlers_) {
        if (!handler->is_active() || !should_handle_event(handler.get(), event)) {
            continue;
        }
        
        if (handler->can_handle(event)) {
            result = handler->handle_event(event);
            
            if (log_events_) {
                log_event(event, std::to_string(static_cast<int>(handler->get_context())), result);
            }
            
            if (result == EventResult::HANDLED) {
                break;
            }
        }
    }
    
    processing_event_ = false;
    return result;
}

void EventRouter::add_handler(std::shared_ptr<ContextualInputHandler> handler) {
    handlers_.push_back(handler);
    handlers_dirty_ = true;
}

void EventRouter::remove_handler(std::shared_ptr<ContextualInputHandler> handler) {
    handlers_.erase(
        std::remove(handlers_.begin(), handlers_.end(), handler),
        handlers_.end()
    );
}

void EventRouter::clear_handlers() {
    handlers_.clear();
    handlers_dirty_ = false;
}

void EventRouter::push_modal_handler(std::shared_ptr<ContextualInputHandler> handler) {
    modal_stack_.push_back(handler);
}

void EventRouter::pop_modal_handler() {
    if (!modal_stack_.empty()) {
        modal_stack_.pop_back();
    }
}

void EventRouter::activate_context(InputContext context) {
    for (auto& handler : handlers_) {
        if (handler->get_context() == context) {
            handler->activate();
        }
    }
}

void EventRouter::deactivate_context(InputContext context) {
    for (auto& handler : handlers_) {
        if (handler->get_context() == context) {
            handler->deactivate();
        }
    }
}

void EventRouter::set_context_priority(InputContext context, int priority) {
    bool any_changed = false;
    for (auto& handler : handlers_) {
        if (handler->get_context() == context) {
            // Note: Can't directly set priority_ as it's protected
            // Would need to add a setter method to ContextualInputHandler
            any_changed = true;
        }
    }
    if (any_changed) {
        handlers_dirty_ = true;
    }
}

void EventRouter::set_keyboard_focus(std::shared_ptr<InputHandler> handler) {
    keyboard_focus_ = handler;
}

void EventRouter::clear_keyboard_focus() {
    keyboard_focus_.reset();
}

void EventRouter::capture_mouse(std::shared_ptr<InputHandler> handler) {
    mouse_capture_ = handler;
}

void EventRouter::release_mouse_capture() {
    mouse_capture_.reset();
}

void EventRouter::add_event_filter(EventFilter filter) {
    event_filters_.push_back(filter);
}

void EventRouter::remove_event_filter(EventFilter filter) {
    // Note: This is tricky with std::function, would need better ID system in real implementation
    // For now, just clear all filters if exact match needed
}

void EventRouter::defer_event(const InputEvent& event) {
    deferred_events_.push(event);
}

void EventRouter::process_deferred_events() {
    while (!deferred_events_.empty()) {
        InputEvent event = deferred_events_.front();
        deferred_events_.pop();
        route_event(event);
    }
}

void EventRouter::log_handler_chain() const {
    std::cout << "EventRouter Handler Chain:" << std::endl;
    
    if (!modal_stack_.empty()) {
        std::cout << "  Modal Stack:" << std::endl;
        for (const auto& handler : modal_stack_) {
            std::cout << "    - Context: " << static_cast<int>(handler->get_context()) 
                     << ", Priority: " << handler->get_priority()
                     << ", Active: " << handler->is_active() << std::endl;
        }
    }
    
    std::cout << "  Normal Handlers:" << std::endl;
    for (const auto& handler : handlers_) {
        std::cout << "    - Context: " << static_cast<int>(handler->get_context())
                 << ", Priority: " << handler->get_priority()
                 << ", Active: " << handler->is_active() << std::endl;
    }
}

void EventRouter::sort_handlers() {
    std::sort(handlers_.begin(), handlers_.end(),
        [](const std::shared_ptr<ContextualInputHandler>& a, const std::shared_ptr<ContextualInputHandler>& b) {
            // Higher priority first
            if (a->get_priority() != b->get_priority()) {
                return a->get_priority() > b->get_priority();
            }
            // Then by context enum order
            return static_cast<int>(a->get_context()) < static_cast<int>(b->get_context());
        });
}

bool EventRouter::should_handle_event(const ContextualInputHandler* handler, const InputEvent& event) const {
    // Context-specific filtering logic
    switch (handler->get_context()) {
        case InputContext::MODAL_DIALOG:
            return true; // Modal dialogs handle everything
            
        case InputContext::POPUP_MENU:
            return true; // Popups handle most input
            
        case InputContext::UI_WIDGET:
            return true; // UI widgets handle relevant input
            
        case InputContext::REGION_HEADER:
            // Only handle input in region header areas
            return true; // TODO: Add bounds checking
            
        case InputContext::EDITOR_TOOL:
            // Only handle input when tool is active
            return true; // TODO: Add tool state checking
            
        case InputContext::EDITOR_NAVIGATION:
            // Handle navigation input in 3D editors
            return event.is_mouse_event() || event.is_keyboard_event();
            
        case InputContext::GLOBAL_SHORTCUTS:
            // Only keyboard shortcuts
            return event.is_keyboard_event();
    }
    
    return true;
}

void EventRouter::log_event(const InputEvent& event, const std::string& handler_name, EventResult result) const {
    const char* type_name = "UNKNOWN";
    switch (event.type) {
        case EventType::MOUSE_PRESS: type_name = "MOUSE_PRESS"; break;
        case EventType::MOUSE_RELEASE: type_name = "MOUSE_RELEASE"; break;
        case EventType::MOUSE_MOVE: type_name = "MOUSE_MOVE"; break;
        case EventType::MOUSE_WHEEL: type_name = "MOUSE_WHEEL"; break;
        case EventType::KEY_PRESS: type_name = "KEY_PRESS"; break;
        case EventType::KEY_RELEASE: type_name = "KEY_RELEASE"; break;
        case EventType::WINDOW_RESIZE: type_name = "WINDOW_RESIZE"; break;
        case EventType::WINDOW_CLOSE: type_name = "WINDOW_CLOSE"; break;
        case EventType::TRACKPAD_SCROLL: type_name = "TRACKPAD_SCROLL"; break;
        case EventType::TRACKPAD_PAN: type_name = "TRACKPAD_PAN"; break;
        case EventType::TRACKPAD_ZOOM: type_name = "TRACKPAD_ZOOM"; break;
        case EventType::TRACKPAD_ROTATE: type_name = "TRACKPAD_ROTATE"; break;
        case EventType::SMART_MOUSE_GESTURE: type_name = "SMART_MOUSE_GESTURE"; break;
    }
    
    const char* result_name = "UNKNOWN";
    switch (result) {
        case EventResult::HANDLED: result_name = "HANDLED"; break;
        case EventResult::IGNORED: result_name = "IGNORED"; break;
        case EventResult::PASS_THROUGH: result_name = "PASS_THROUGH"; break;
    }
    
    // std::cout << "[EventRouter] " << type_name << " -> " << handler_name << " -> " << result_name << std::endl;
}

// GlobalShortcutHandler implementation

GlobalShortcutHandler::GlobalShortcutHandler()
    : ContextualInputHandler(InputContext::GLOBAL_SHORTCUTS, 100) {
}

bool GlobalShortcutHandler::can_handle(const InputEvent& event) const {
    return event.type == EventType::KEY_PRESS;
}

EventResult GlobalShortcutHandler::handle_event(const InputEvent& event) {
    if (event.type != EventType::KEY_PRESS) {
        return EventResult::IGNORED;
    }
    
    // Look for matching shortcut
    for (const auto& shortcut : shortcuts_) {
        if (shortcut.key_code == event.key_code && shortcut.modifiers == event.modifiers) {
            shortcut.callback();
            return EventResult::HANDLED;
        }
    }
    
    return EventResult::IGNORED;
}

void GlobalShortcutHandler::register_shortcut(int key_code, uint32_t modifiers, ShortcutCallback callback) {
    Shortcut shortcut;
    shortcut.key_code = key_code;
    shortcut.modifiers = modifiers;
    shortcut.callback = callback;
    shortcuts_.push_back(shortcut);
}

void GlobalShortcutHandler::unregister_shortcut(int key_code, uint32_t modifiers) {
    shortcuts_.erase(
        std::remove_if(shortcuts_.begin(), shortcuts_.end(),
            [key_code, modifiers](const Shortcut& s) {
                return s.key_code == key_code && s.modifiers == modifiers;
            }),
        shortcuts_.end()
    );
}

// RegionInteractionHandler implementation

RegionInteractionHandler::RegionInteractionHandler(CanvasWindow* window)
    : ContextualInputHandler(InputContext::REGION_HEADER, 200)
    , window_(window)
    , active_region_(nullptr)
    , is_resizing_(false) {
}

bool RegionInteractionHandler::can_handle(const InputEvent& event) const {
    return event.is_mouse_event();
}

EventResult RegionInteractionHandler::handle_event(const InputEvent& event) {
    switch (event.type) {
        case EventType::MOUSE_PRESS:
            if (event.mouse_button == MouseButton::LEFT) { // Left click
                if (is_on_region_splitter(event.mouse_pos)) {
                    is_resizing_ = true;
                    resize_start_pos_ = event.mouse_pos;
                    return EventResult::HANDLED;
                }
            }
            break;
            
        case EventType::MOUSE_RELEASE:
            if (event.mouse_button == MouseButton::LEFT && is_resizing_) {
                is_resizing_ = false;
                active_region_ = nullptr;
                return EventResult::HANDLED;
            }
            break;
            
        case EventType::MOUSE_MOVE:
            if (is_resizing_ && active_region_) {
                handle_region_resize(event);
                return EventResult::HANDLED;
            }
            break;
            
        default:
            break;
    }
    
    return EventResult::IGNORED;
}

bool RegionInteractionHandler::handle_region_resize(const InputEvent& event) {
    // TODO: Implement region resizing logic
    return false;
}

bool RegionInteractionHandler::handle_region_split(const InputEvent& event) {
    // TODO: Implement region splitting logic
    return false;
}

bool RegionInteractionHandler::is_on_region_splitter(const Point2D& point) const {
    // TODO: Check if point is on a region splitter
    return false;
}

// WidgetInteractionHandler implementation

WidgetInteractionHandler::WidgetInteractionHandler()
    : ContextualInputHandler(InputContext::UI_WIDGET, 300) {
}

bool WidgetInteractionHandler::can_handle(const InputEvent& event) const {
    return event.is_mouse_event();
}

EventResult WidgetInteractionHandler::handle_event(const InputEvent& event) {
    // TODO: Route events to registered widgets
    return EventResult::IGNORED;
}

void WidgetInteractionHandler::register_widget(std::shared_ptr<Widget> widget) {
    widgets_.push_back(widget);
}

void WidgetInteractionHandler::unregister_widget(std::shared_ptr<Widget> widget) {
    widgets_.erase(
        std::remove_if(widgets_.begin(), widgets_.end(),
            [&widget](const std::weak_ptr<Widget>& weak_widget) {
                return weak_widget.expired() || weak_widget.lock() == widget;
            }),
        widgets_.end()
    );
}

} // namespace voxel_canvas