/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional event system architecture.
 * Independent implementation of event-driven programming patterns.
 */

#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

namespace voxelux::core {

// Base event class - all events inherit from this
class Event {
public:
    virtual ~Event() = default;
    
    bool is_handled() const { return handled_; }
    void set_handled(bool handled = true) { handled_ = handled; }
    
protected:
    std::atomic<bool> handled_{false};
};

// Event handler interface
template<typename EventType>
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual bool handle(const EventType& event) = 0;
};

// Function-based event handler
template<typename EventType>
class FunctionEventHandler : public EventHandler<EventType> {
public:
    using HandlerFunction = std::function<bool(const EventType&)>;
    
    explicit FunctionEventHandler(HandlerFunction handler) 
        : handler_(std::move(handler)) {}
    
    bool handle(const EventType& event) override {
        return handler_(event);
    }

private:
    HandlerFunction handler_;
};

// Event dispatcher/manager
class EventDispatcher {
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;
    
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) = delete;
    EventDispatcher& operator=(EventDispatcher&&) = delete;
    
    // Subscribe to events with a handler object
    template<typename EventType>
    void subscribe(std::shared_ptr<EventHandler<EventType>> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& handlers = get_handlers<EventType>();
        handlers.push_back(std::weak_ptr<EventHandler<EventType>>(handler));
    }
    
    // Subscribe with a function
    template<typename EventType>
    void subscribe(std::function<bool(const EventType&)> handler) {
        auto handler_obj = std::make_shared<FunctionEventHandler<EventType>>(std::move(handler));
        subscribe<EventType>(handler_obj);
    }
    
    // Publish event immediately (synchronous)
    template<typename EventType>
    void publish(const EventType& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& handlers = get_handlers<EventType>();
        
        // Clean up expired handlers and call valid ones
        auto it = handlers.begin();
        while (it != handlers.end()) {
            if (auto handler = it->lock()) {
                if (handler->handle(event)) {
                    const_cast<EventType&>(event).set_handled(true);
                    break; // Event was handled, stop processing
                }
                ++it;
            } else {
                // Handler expired, remove it
                it = handlers.erase(it);
            }
        }
    }
    
    // Queue event for later processing (asynchronous)
    template<typename EventType>
    void queue(std::unique_ptr<EventType> event) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        event_queue_.push(std::move(event));
    }
    
    // Process all queued events
    void process_queued_events() {
        std::queue<std::unique_ptr<Event>> temp_queue;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            temp_queue.swap(event_queue_);
        }
        
        while (!temp_queue.empty()) {
            auto event = std::move(temp_queue.front());
            temp_queue.pop();
            dispatch_event(*event);
        }
    }
    
    // Clear all handlers
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.clear();
    }
    
    // Get handler count for debugging
    template<typename EventType>
    size_t handler_count() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& handlers = get_handlers<EventType>();
        
        // Clean up expired handlers while counting
        auto it = handlers.begin();
        while (it != handlers.end()) {
            if (it->expired()) {
                it = handlers.erase(it);
            } else {
                ++it;
            }
        }
        
        return handlers.size();
    }

private:
    template<typename EventType>
    std::vector<std::weak_ptr<EventHandler<EventType>>>& get_handlers() {
        std::type_index type_id = std::type_index(typeid(EventType));
        
        // This is safe because we only ever store the correct type
        auto& storage = handlers_[type_id];
        if (storage == nullptr) {
            storage = std::make_unique<std::vector<std::weak_ptr<EventHandler<EventType>>>>();
        }
        
        return *static_cast<std::vector<std::weak_ptr<EventHandler<EventType>>>*>(storage.get());
    }
    
    void dispatch_event(const Event& event) {
        // This would need runtime type dispatch - simplified for now
        // In practice, you'd use a registry of type dispatchers
    }
    
    std::mutex mutex_;
    std::mutex queue_mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<void, void(*)(void*)>> handlers_;
    std::queue<std::unique_ptr<Event>> event_queue_;
};

// Singleton access to global event dispatcher
class EventSystem {
public:
    static EventDispatcher& instance() {
        static EventDispatcher instance;
        return instance;
    }
    
    template<typename EventType>
    static void subscribe(std::function<bool(const EventType&)> handler) {
        instance().subscribe<EventType>(std::move(handler));
    }
    
    template<typename EventType>
    static void publish(const EventType& event) {
        instance().publish(event);
    }
    
    template<typename EventType>
    static void queue(std::unique_ptr<EventType> event) {
        instance().queue(std::move(event));
    }
    
    static void process_events() {
        instance().process_queued_events();
    }
};

}