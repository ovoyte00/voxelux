/*
 * Copyright (C) 2024 Voxelux
 * 
 * Native Platform Input Helper
 * Provides access to platform-specific input features not exposed by GLFW
 */

#pragma once

#include <queue>
#include <mutex>

namespace voxelux {
namespace platform {

/**
 * Type of scroll input device detected by native OS
 */
enum class ScrollDeviceType {
    Unknown,        // Could not determine
    MouseWheel,     // Traditional mouse wheel (discrete steps)
    Trackpad,       // Trackpad with gesture support
    SmartMouse      // Apple Magic Mouse or similar
};

/**
 * Native scroll event with platform-specific information
 */
struct NativeScrollEvent {
    ScrollDeviceType device_type;
    double delta_x;
    double delta_y;
    
    // Platform-specific flags
    bool has_phase;           // Gesture phase information available
    bool is_momentum;         // Momentum/inertia scrolling
    bool is_precise;          // Precise/pixel-perfect scrolling
    bool is_inverted;         // Natural/inverted scrolling
    bool is_pinch;            // This is a pinch/magnification gesture
    bool shift_held;          // Shift key was held during scroll
    bool axis_swapped_by_os;  // OS swapped vertical to horizontal (macOS+shift)
    
    // Timing
    double timestamp;
    
    NativeScrollEvent() 
        : device_type(ScrollDeviceType::Unknown)
        , delta_x(0), delta_y(0)
        , has_phase(false), is_momentum(false)
        , is_precise(false), is_inverted(false)
        , is_pinch(false), shift_held(false)
        , axis_swapped_by_os(false)
        , timestamp(0) {}
};

/**
 * Native input helper for platform-specific features
 */
class NativeInput {
public:
    /**
     * Initialize native input helper for a GLFW window
     * Must be called after window creation
     * 
     * @param glfw_window The GLFW window handle
     * @return true if initialization succeeded
     */
    static bool initialize(void* glfw_window);
    
    /**
     * Shutdown and cleanup native input helper
     */
    static void shutdown();
    
    /**
     * Check if native input helper is available on this platform
     */
    static bool is_available();
    
    /**
     * Get the next native scroll event if available
     * This should be called from GLFW's scroll callback
     * 
     * @param event Output event data
     * @return true if an event was available
     */
    static bool get_next_scroll_event(NativeScrollEvent& event);
    
    /**
     * Clear all pending scroll events
     */
    static void clear_scroll_events();
    
    /**
     * Get the current default scroll device type
     * Useful for settings UI
     */
    static ScrollDeviceType get_default_scroll_device();
    
    /**
     * Internal helper for adding events from platform code
     */
    static void add_scroll_event(const NativeScrollEvent& event);
    
private:
    static std::queue<NativeScrollEvent> scroll_event_queue_;
    static std::mutex queue_mutex_;
    static bool initialized_;
    static void* platform_data_;  // Platform-specific data
};

} // namespace platform
} // namespace voxelux