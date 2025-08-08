/*
 * Copyright (C) 2024 Voxelux
 * 
 * Native Platform Input Helper - Stub Implementation
 * Used for platforms without native implementation
 */

#include "voxelux/platform/native_input.h"
#include <iostream>

namespace voxelux {
namespace platform {

// Static member definitions
std::queue<NativeScrollEvent> NativeInput::scroll_event_queue_;
std::mutex NativeInput::queue_mutex_;
bool NativeInput::initialized_ = false;
void* NativeInput::platform_data_ = nullptr;

bool NativeInput::initialize(void* glfw_window) {
    // Stub implementation - native input not available
    initialized_ = false;
    std::cout << "NativeInput: Not available on this platform (using stub)" << std::endl;
    return false;
}

void NativeInput::shutdown() {
    initialized_ = false;
}

bool NativeInput::is_available() {
    return false;
}

bool NativeInput::get_next_scroll_event(NativeScrollEvent& event) {
    // No native events available in stub
    return false;
}

void NativeInput::clear_scroll_events() {
    // Nothing to clear in stub
}

ScrollDeviceType NativeInput::get_default_scroll_device() {
    return ScrollDeviceType::Unknown;
}

} // namespace platform
} // namespace voxelux