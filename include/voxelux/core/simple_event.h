/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional simple event system implementation.
 * Independent lightweight event handling for rapid development.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>

namespace voxelux::core {

// Simple event base class
class SimpleEvent {
public:
    virtual ~SimpleEvent() = default;
    bool is_handled() const { return handled_; }
    void set_handled(bool handled = true) { handled_ = handled; }

private:
    bool handled_ = false;
};

// Simple event dispatcher
class SimpleEventDispatcher {
public:
    using EventCallback = std::function<bool(const SimpleEvent&)>;
    
    template<typename EventType>
    void subscribe(std::function<bool(const EventType&)> callback) {
        auto type_id = std::type_index(typeid(EventType));
        
        auto wrapper = [callback](const SimpleEvent& event) -> bool {
            const auto* typed_event = dynamic_cast<const EventType*>(&event);
            if (typed_event) {
                return callback(*typed_event);
            }
            return false;
        };
        
        callbacks_[type_id].push_back(wrapper);
    }
    
    template<typename EventType>
    void publish(const EventType& event) {
        auto type_id = std::type_index(typeid(EventType));
        auto it = callbacks_.find(type_id);
        
        if (it != callbacks_.end()) {
            for (auto& callback : it->second) {
                if (callback(event)) {
                    const_cast<EventType&>(event).set_handled(true);
                    break;
                }
            }
        }
    }
    
    void clear() {
        callbacks_.clear();
    }

private:
    std::unordered_map<std::type_index, std::vector<EventCallback>> callbacks_;
};

}