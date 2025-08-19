/*
 * Copyright (C) 2024 Voxelux
 * 
 * Native Platform Input Helper - macOS Implementation
 */

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h> // For key codes
#include "voxelux/platform/native_input.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <iostream>

// macOS-specific scroll interceptor - must be outside namespace
@interface VoxeluxScrollInterceptor : NSObject {
    id eventMonitor;
    id magnificationMonitor;
}
- (void)startMonitoring;
- (void)stopMonitoring;
- (void)handleScrollEvent:(NSEvent*)event;
- (void)handleMagnificationEvent:(NSEvent*)event;
@end

// Need forward declaration for friend access
namespace voxelux {
namespace platform {
class NativeInput;
}
}

@implementation VoxeluxScrollInterceptor

- (void)startMonitoring {
    // Use a local event monitor to intercept scroll events
    // Note: We use addLocalMonitorForEventsMatchingMask to see events before GLFW
    eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel 
        handler:^NSEvent*(NSEvent* event) {
            [self handleScrollEvent:event];
            return event; // Still pass to GLFW
        }];
    
    // Also monitor magnification (pinch) gestures
    magnificationMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
        handler:^NSEvent*(NSEvent* event) {
            [self handleMagnificationEvent:event];
            return event; // Still pass to GLFW
        }];
}

- (void)stopMonitoring {
    if (eventMonitor) {
        [NSEvent removeMonitor:eventMonitor];
        eventMonitor = nil;
    }
    if (magnificationMonitor) {
        [NSEvent removeMonitor:magnificationMonitor];
        magnificationMonitor = nil;
    }
}

- (void)handleScrollEvent:(NSEvent*)event {
    voxelux::platform::NativeScrollEvent native_event;
    native_event.timestamp = event.timestamp;
    native_event.is_pinch = false; // Regular scroll, not pinch
    
    // Detect device type based on NSEvent properties
    NSEventPhase phase = event.phase;
    NSEventPhase momentumPhase = event.momentumPhase;
    
    // Check if this is a gesture (trackpad/smart mouse) or discrete scroll (mouse wheel)
    if (phase != NSEventPhaseNone || momentumPhase != NSEventPhaseNone) {
        // This is a trackpad or smart mouse gesture
        native_event.has_phase = true;
        native_event.is_momentum = (momentumPhase != NSEventPhaseNone);
        native_event.is_precise = event.hasPreciseScrollingDeltas;
        
        // Distinguish between trackpad and smart mouse
        // Smart Mouse typically has precise deltas but different characteristics
        // This is a heuristic - both devices are very similar in practice
        if (event.subtype == NSEventSubtypeMouseEvent) {
            native_event.device_type = voxelux::platform::ScrollDeviceType::SmartMouse;
        } else {
            native_event.device_type = voxelux::platform::ScrollDeviceType::Trackpad;
        }
        
        // CRITICAL: Use scrollingDelta for trackpad/smart mouse (pixel-precise)
        // macOS converts vertical scrolls to horizontal when Shift is held
        // We need to detect and restore the original orientation
        native_event.delta_x = event.scrollingDeltaX;
        native_event.delta_y = event.scrollingDeltaY;
        
        // Debug: Log the raw values for Smart Mouse
        if (native_event.device_type == voxelux::platform::ScrollDeviceType::SmartMouse) {
            NSLog(@"[SmartMouse] scrollingDelta: (%.3f, %.3f) hasPrecise: %d", 
                  event.scrollingDeltaX, event.scrollingDeltaY, event.hasPreciseScrollingDeltas);
        }
        
        // Detect and correct for macOS shift+scroll axis conversion
        // When shift is held and we only get horizontal delta, it was likely vertical originally
        if (native_event.shift_held) {
            // Check if this looks like a converted vertical scroll:
            // - We have horizontal delta but no vertical
            // - The horizontal delta magnitude is typical of vertical scrolling
            if (std::abs(native_event.delta_x) > 0.1 && std::abs(native_event.delta_y) < 0.1) {
                // This was likely a vertical scroll converted to horizontal by macOS
                // Store a flag so the application can restore it
                native_event.axis_swapped_by_os = true;
                
                // Debug output
                NSLog(@"[Native] Detected OS axis swap: dx=%.2f dy=%.2f (was vertical)",
                      native_event.delta_x, native_event.delta_y);
            } else {
                native_event.axis_swapped_by_os = false;
            }
        } else {
            native_event.axis_swapped_by_os = false;
        }
        
        // Debug output
        [[maybe_unused]] const char* phase_str{"none"};
        if (phase == NSEventPhaseBegan) phase_str = "began";
        else if (phase == NSEventPhaseChanged) phase_str = "changed";
        else if (phase == NSEventPhaseEnded) phase_str = "ended";
        else if (phase == NSEventPhaseCancelled) phase_str = "cancelled";
        else if (phase == NSEventPhaseStationary) phase_str = "stationary";
        
        // std::cout << "[Native] " 
        //           << (native_event.device_type == voxelux::platform::ScrollDeviceType::SmartMouse ? "SmartMouse" : "Trackpad")
        //           << " phase=" << phase_str
        //           << " momentum=" << (native_event.is_momentum ? "yes" : "no")
        //           << " delta=(" << native_event.delta_x << ", " << native_event.delta_y << ")"
        //           << std::endl;
    } else {
        // Traditional mouse wheel - discrete scrolling
        native_event.device_type = voxelux::platform::ScrollDeviceType::MouseWheel;
        native_event.has_phase = false;
        native_event.is_momentum = false;
        native_event.is_precise = false;
        
        // Mouse wheel uses integer deltas
        native_event.delta_x = event.deltaX;
        native_event.delta_y = event.deltaY;
        
    }
    
    // Check for natural scrolling
    native_event.is_inverted = event.isDirectionInvertedFromDevice;
    
    // Check modifier flags
    NSEventModifierFlags modifiers = event.modifierFlags;
    native_event.shift_held = (modifiers & NSEventModifierFlagShift) != 0;
    
    // Debug: If shift is held and we only get X delta, this might be a converted vertical scroll
    if (native_event.shift_held && std::abs(native_event.delta_x) > 0.1 && std::abs(native_event.delta_y) < 0.1) {
        // This is likely a vertical scroll converted to horizontal by macOS
        // We can't recover the original direction, but we can flag it
        // std::cout << "[Native] Shift+scroll detected: dx=" << native_event.delta_x 
        //           << " (likely converted from vertical)" << std::endl;
    }
    
    // Add to queue
    {
        // Use the public interface to add event
        voxelux::platform::NativeInput::add_scroll_event(native_event);
    }
}

- (void)handleMagnificationEvent:(NSEvent*)event {
    // Create a special scroll event for magnification gestures
    voxelux::platform::NativeScrollEvent native_event;
    native_event.timestamp = event.timestamp;
    native_event.device_type = voxelux::platform::ScrollDeviceType::Trackpad;
    native_event.has_phase = true;
    native_event.is_momentum = false;
    native_event.is_precise = true;
    native_event.is_inverted = false;
    
    // For magnification, use the magnification value as the delta
    // Positive magnification = zoom in, negative = zoom out
    // Scale the magnification to match expected scroll delta ranges
    float magnification{static_cast<float>(event.magnification)};
    native_event.delta_x = 0;
    native_event.delta_y = static_cast<double>(magnification * 10.0f); // Scale factor for zoom sensitivity
    
    // Set a special flag to indicate this is a pinch gesture
    native_event.is_pinch = true;
    
    // Debug output
    // std::cout << "[Native] Trackpad magnification=" << magnification 
    //           << " scaled_delta=" << native_event.delta_y << std::endl;
    
    // Add to queue
    voxelux::platform::NativeInput::add_scroll_event(native_event);
}

- (void)dealloc {
    [self stopMonitoring];
    [super dealloc];
}

@end

namespace voxelux {
namespace platform {

// Static member definitions
std::queue<NativeScrollEvent> NativeInput::scroll_event_queue_;
std::mutex NativeInput::queue_mutex_;
bool NativeInput::initialized_ = false;
void* NativeInput::platform_data_ = nullptr;

// Helper to add events from Objective-C
void NativeInput::add_scroll_event(const NativeScrollEvent& event) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    scroll_event_queue_.push(event);
    
    // Limit queue size
    while (scroll_event_queue_.size() > 100) {
        scroll_event_queue_.pop();
    }
}

// NativeInput implementation

bool NativeInput::initialize(void* glfw_window) {
    if (initialized_) {
        return true;
    }
    
    GLFWwindow* window = static_cast<GLFWwindow*>(glfw_window);
    if (!window) {
        std::cerr << "NativeInput: Invalid GLFW window" << std::endl;
        return false;
    }
    
    // Get the native NSWindow from GLFW
    NSWindow* nsWindow = glfwGetCocoaWindow(window);
    if (!nsWindow) {
        std::cerr << "NativeInput: Could not get NSWindow from GLFW" << std::endl;
        return false;
    }
    
    // Create and start our scroll interceptor
    VoxeluxScrollInterceptor* interceptor = [[VoxeluxScrollInterceptor alloc] init];
    [interceptor startMonitoring];
    
    // Store the interceptor so we can clean it up later
    platform_data_ = interceptor;
    initialized_ = true;
    
    // std::cout << "NativeInput: Initialized for macOS" << std::endl;
    return true;
}

void NativeInput::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (platform_data_) {
        VoxeluxScrollInterceptor* interceptor{static_cast<VoxeluxScrollInterceptor*>(platform_data_)};
        [interceptor stopMonitoring];
        [interceptor release];
        platform_data_ = nullptr;
    }
    
    // Clear any remaining events
    clear_scroll_events();
    
    initialized_ = false;
    // std::cout << "NativeInput: Shutdown complete" << std::endl;
}

bool NativeInput::is_available() {
    // Native input is available on macOS 10.7+
    return true;
}

bool NativeInput::get_next_scroll_event(NativeScrollEvent& event) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (scroll_event_queue_.empty()) {
        return false;
    }
    
    event = scroll_event_queue_.front();
    scroll_event_queue_.pop();
    return true;
}

void NativeInput::clear_scroll_events() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // Clear the queue
    std::queue<NativeScrollEvent> empty;
    std::swap(scroll_event_queue_, empty);
}

ScrollDeviceType NativeInput::get_default_scroll_device() {
    // Try to detect the default device
    // This is a simple heuristic - you might want to check system preferences
    
    // Check if a Magic Mouse is connected
    // For now, return Unknown and let auto-detection work
    return ScrollDeviceType::Unknown;
}

} // namespace platform
} // namespace voxelux