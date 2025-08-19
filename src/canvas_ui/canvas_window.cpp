/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Main window implementation
 */

#include "canvas_ui/canvas_window.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/event_router.h"
#include "canvas_ui/canvas_region.h"
#include "canvas_ui/icon_system.h"
#include "voxelux/platform/native_input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "glad/gl.h"
#include <iostream>
#include <stdexcept>

namespace voxel_canvas {

// Static callback functions for GLFW
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void glfw_window_size_callback(GLFWwindow* window, int width, int height) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        canvas_window->on_window_resize(width, height);
    }
}

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        canvas_window->on_framebuffer_resize(width, height);
    }
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        canvas_window->on_mouse_button(button, action, mods);
    } else {
    }
}

static void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        // Check if cursor is captured for infinite dragging
        int cursor_mode = glfwGetInputMode(window, GLFW_CURSOR);
        
        if (cursor_mode == GLFW_CURSOR_DISABLED) {
            // When cursor is disabled, always process movement (infinite dragging)
            canvas_window->on_mouse_move(x, y);
        } else {
            // Only process mouse move if cursor is inside the window
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            if (x >= 0 && y >= 0 && x < width && y < height) {
                canvas_window->on_mouse_move(x, y);
            }
        }
    }
}

static void glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        canvas_window->on_mouse_scroll(x_offset, y_offset);
    }
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        canvas_window->on_key_event(key, scancode, action, mods);
    }
}

static void glfw_window_content_scale_callback(GLFWwindow* window, float xscale, float yscale) {
    CanvasWindow* canvas_window = static_cast<CanvasWindow*>(glfwGetWindowUserPointer(window));
    if (canvas_window) {
        std::cout << "Window content scale changed to: " << xscale << "x" << yscale << std::endl;
        // The framebuffer resize callback will handle the actual DPI recalculation
    }
}

// CanvasWindow implementation

CanvasWindow::CanvasWindow(int width, int height, const std::string& title)
    : window_(nullptr)
    , title_(title)
    , width_(width)
    , height_(height)
    , framebuffer_width_(width)
    , framebuffer_height_(height)
    , content_scale_(1.0f)
    , dpi_(72.0f)  // Base DPI following Blender/macOS convention
    , ui_scale_(1.0f)
    , pixelsize_(1.0f)
    , ui_line_width_(0)
    , last_mouse_pos_(0, 0)
    , keyboard_modifiers_(0) {
}

CanvasWindow::~CanvasWindow() {
    shutdown();
}

bool CanvasWindow::initialize() {
    if (initialized_) {
        return true;
    }

    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Set OpenGL version and profile (4.1 Core for better macOS compatibility)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    // macOS requires forward compatibility for Core Profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // TEMPORARILY DISABLE multisampling to debug rendering issue
    // With MSAA, edge fragments can be blended multiple times causing bright rims
    glfwWindowHint(GLFW_SAMPLES, 0);  // Was 4
    
    // Create window
    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Make window visible and focused
    glfwShowWindow(window_);
    glfwFocusWindow(window_);
    
    // Position window on screen
    glfwSetWindowPos(window_, 100, 100);
    
    std::cout << "Created window at position 100,100 with size " << width_ << "x" << height_ << std::endl;

    // Setup OpenGL context
    setup_opengl_context();
    setup_callbacks();

    // Initialize renderer
    renderer_ = std::make_unique<CanvasRenderer>(this);
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize Canvas renderer" << std::endl;
        return false;
    }

    // Initialize event router
    event_router_ = std::make_unique<EventRouter>(this);

    // Initialize region manager
    region_manager_ = std::make_unique<RegionManager>(this);

    // Initialize workspace manager
    workspace_manager_ = std::make_unique<WorkspaceManager>(this);
    workspace_manager_->create_default_workspaces();

    // Set default theme
    theme_ = CanvasTheme(); // Uses default professional dark theme colors
    renderer_->set_theme(theme_);

    // Initialize icon system
    IconSystem& icon_system = IconSystem::get_instance();
    if (!icon_system.initialize("assets/icons/")) {
        std::cerr << "Warning: Failed to initialize icon system" << std::endl;
    }

    // Initialize native input helper for better scroll detection
    if (voxelux::platform::NativeInput::is_available()) {
        if (voxelux::platform::NativeInput::initialize(window_)) {
            std::cout << "Native input helper initialized successfully" << std::endl;
            
            // Detect natural scroll direction from system preferences
#ifdef __APPLE__
            // On macOS, check the system preference for natural scrolling
            // This is typically enabled by default on macOS ("natural" scrolling)
            // For now, default to natural scrolling on macOS
            natural_scroll_direction_ = true;  // macOS default is natural scrolling
            std::cout << "Scroll direction: " << (natural_scroll_direction_ ? "Natural" : "Regular") << std::endl;
#else
            // On other platforms, use traditional scrolling by default
            natural_scroll_direction_ = false;
#endif
        } else {
            std::cout << "Warning: Native input helper failed to initialize" << std::endl;
        }
    }

    initialized_ = true;
    return true;
}

void CanvasWindow::shutdown() {
    if (!initialized_) {
        return;
    }

    // Shutdown native input helper
    voxelux::platform::NativeInput::shutdown();

    // Shutdown in reverse order
    workspace_manager_.reset();
    region_manager_.reset();
    event_router_.reset();
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
    initialized_ = false;
}

bool CanvasWindow::should_close() const {
    return window_ && glfwWindowShouldClose(window_);
}

void CanvasWindow::swap_buffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

void CanvasWindow::poll_events() {
    glfwPollEvents();
    
    // Process any pending native events (like pinch gestures)
    // Only process pinch gestures here - regular scroll events go through GLFW callback
    // We need to accumulate small deltas to avoid spam
    static float accumulated_pinch_delta = 0.0f;
    static double last_pinch_time = 0.0;
    const float MIN_PINCH_DELTA = 0.5f;  // Minimum delta to process
    const double PINCH_TIMEOUT = 0.1;    // Reset accumulator after 100ms
    
    voxelux::platform::NativeScrollEvent native_event;
    while (voxelux::platform::NativeInput::get_next_scroll_event(native_event)) {
        if (native_event.is_pinch) {
            double now = glfwGetTime();
            
            // Reset accumulator if too much time has passed
            if (now - last_pinch_time > PINCH_TIMEOUT) {
                accumulated_pinch_delta = 0.0f;
            }
            
            // Accumulate the delta
            accumulated_pinch_delta += static_cast<float>(native_event.delta_y);
            last_pinch_time = now;
            
            // Only send event if we've accumulated enough delta
            if (std::abs(accumulated_pinch_delta) >= MIN_PINCH_DELTA) {
                InputEvent event = create_input_event(EventType::TRACKPAD_ZOOM);
                event.wheel_delta = accumulated_pinch_delta;
                event.mouse_delta = Point2D(0, accumulated_pinch_delta);
                
                // Route through normal event system
                if (region_manager_) {
                    if (region_manager_->handle_event(event)) {
                        accumulated_pinch_delta = 0.0f;  // Reset after handling
                        continue;
                    }
                }
                
                if (event_router_) {
                    event_router_->route_event(event);
                }
                
                accumulated_pinch_delta = 0.0f;  // Reset after processing
            }
        }
        // Non-pinch events should not be in the queue here
        // They're handled by the GLFW scroll callback
    }
    
    // Process deferred events from event router
    if (event_router_) {
        event_router_->process_deferred_events();
    }
}

void CanvasWindow::set_title(const std::string& title) {
    title_ = title;
    if (window_) {
        glfwSetWindowTitle(window_, title_.c_str());
    }
}

Point2D CanvasWindow::get_size() const {
    return Point2D(static_cast<float>(width_), static_cast<float>(height_));
}

void CanvasWindow::set_size(int width, int height) {
    width_ = width;
    height_ = height;
    if (window_) {
        glfwSetWindowSize(window_, width, height);
    }
}

Point2D CanvasWindow::get_framebuffer_size() const {
    return Point2D(static_cast<float>(framebuffer_width_), static_cast<float>(framebuffer_height_));
}

float CanvasWindow::get_content_scale() const {
    return content_scale_;
}

void CanvasWindow::update_viewport_from_render_thread() {
    // Only call this from render thread with GL context current
    if (viewport_needs_update_ && renderer_) {
        std::lock_guard<std::mutex> lock{viewport_mutex_};
        renderer_->set_viewport(0, 0, framebuffer_width_, framebuffer_height_);
        viewport_needs_update_ = false;
    }
}

void CanvasWindow::set_ui_scale(float scale) {
    ui_scale_ = std::max(0.25f, std::min(4.0f, scale));  // Clamp to reasonable range
    calculate_dpi_and_scaling();
}

void CanvasWindow::set_ui_line_width(int width) {
    ui_line_width_ = std::max(-2, std::min(2, width));  // Clamp to -2 to 2
    calculate_dpi_and_scaling();
}

void CanvasWindow::render_frame() {
    if (!initialized_ || !renderer_) {
        return;
    }

    // NOTE: This default rendering is now handled by VoxeluxLayout in the main app
    // Keeping this for backward compatibility with other uses of CanvasWindow
    
    // Begin frame
    renderer_->begin_frame();
    
    // Update and render regions
    if (region_manager_) {
        Rect2D window_bounds(0, 0, static_cast<float>(framebuffer_width_), static_cast<float>(framebuffer_height_));
        region_manager_->update_layout(window_bounds);
        region_manager_->render_regions();
    }
    
    // End frame and present
    renderer_->end_frame();
    renderer_->present_frame();
}

void CanvasWindow::capture_cursor() {
    if (window_) {
        // Store current position before capturing
        double x, y;
        glfwGetCursorPos(window_, &x, &y);
        cursor_capture_pos_ = Point2D(static_cast<float>(x), static_cast<float>(y));
        
        // Disable cursor (hides it and provides unlimited movement)
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cursor_captured_ = true;
    }
}

void CanvasWindow::release_cursor() {
    if (window_ && cursor_captured_) {
        // Restore normal cursor mode
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        // Restore cursor to original position
        glfwSetCursorPos(window_, static_cast<double>(cursor_capture_pos_.x), static_cast<double>(cursor_capture_pos_.y));
        cursor_captured_ = false;
    }
}

void CanvasWindow::set_cursor_position(double x, double y) {
    if (window_) {
        glfwSetCursorPos(window_, x, y);
    }
}

Point2D CanvasWindow::get_cursor_position() const {
    if (window_) {
        double x, y;
        glfwGetCursorPos(window_, &x, &y);
        return Point2D(static_cast<float>(x), static_cast<float>(y));
    }
    return Point2D(0, 0);
}

void CanvasWindow::set_theme(const CanvasTheme& theme) {
    theme_ = theme;
    if (renderer_) {
        renderer_->set_theme(theme);
    }
}


void CanvasWindow::setup_opengl_context() {
    // Make context current
    glfwMakeContextCurrent(window_);
    
    // Initialize GLAD
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    
    // Enable v-sync
    glfwSwapInterval(1);
    
    // Get framebuffer size and content scale
    glfwGetFramebufferSize(window_, &framebuffer_width_, &framebuffer_height_);
    
    float x_scale, y_scale;
    glfwGetWindowContentScale(window_, &x_scale, &y_scale);
    content_scale_ = x_scale; // Store initial scale
    
    // Calculate proper DPI and scaling factors
    calculate_dpi_and_scaling();
    
    gl_context_initialized_ = true;
}

void CanvasWindow::setup_callbacks() {
    // Store pointer to this instance for callbacks
    glfwSetWindowUserPointer(window_, this);
    
    // Setup GLFW callbacks
    glfwSetWindowSizeCallback(window_, glfw_window_size_callback);
    glfwSetFramebufferSizeCallback(window_, glfw_framebuffer_size_callback);
    glfwSetMouseButtonCallback(window_, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window_, glfw_cursor_pos_callback);
    glfwSetScrollCallback(window_, glfw_scroll_callback);
    glfwSetKeyCallback(window_, glfw_key_callback);
    glfwSetWindowContentScaleCallback(window_, glfw_window_content_scale_callback);
}

InputEvent CanvasWindow::create_input_event(EventType type) const {
    InputEvent event;
    event.type = type;
    event.timestamp = glfwGetTime();
    event.mouse_pos = last_mouse_pos_;  // Already in framebuffer space
    event.mouse_delta = Point2D(0, 0);
    event.mouse_button = MouseButton::LEFT;
    event.key_code = 0;
    event.modifiers = keyboard_modifiers_;
    event.wheel_delta = 0.0f;
    return event;
}

// Event callbacks

void CanvasWindow::on_window_resize(int width, int height) {
    width_ = width;
    height_ = height;
    
    // Recalculate DPI and scaling when window changes
    calculate_dpi_and_scaling();
    
    // Mark viewport as needing update - render thread will handle it
    viewport_needs_update_ = true;
}

void CanvasWindow::calculate_dpi_and_scaling() {
    // Store previous pixelsize to detect changes
    float previous_pixelsize = pixelsize_;
    
    // Calculate actual scale from framebuffer vs window dimensions
    // This handles the case where framebuffer isn't exactly 2x window size
    float scale_x = static_cast<float>(framebuffer_width_) / static_cast<float>(width_);
    float scale_y = static_cast<float>(framebuffer_height_) / static_cast<float>(height_);
    
    // Use average scale (they should be similar but might differ slightly)
    float actual_scale = (scale_x + scale_y) / 2.0f;
    
    // Following Blender's approach:
    // Base DPI is 72 (macOS/Blender convention)
    // GLFW/system assumes 96 DPI, so we convert
    const float base_dpi = 72.0f;
    [[maybe_unused]] const float system_dpi = 96.0f;
    
    // Calculate actual DPI from the native pixel scale
    // On macOS with retina, actual_scale will be ~2.0
    dpi_ = base_dpi * actual_scale * ui_scale_;
    
    // Calculate pixelsize (following Blender's formula)
    // pixelsize = max(1, DPI/64 + ui_line_width)
    pixelsize_ = std::max(1.0f, (dpi_ / 64.0f) + static_cast<float>(ui_line_width_));
    
    // Store the calculated content scale for compatibility
    content_scale_ = actual_scale;
    
    std::cout << "DPI Calculation: window=" << width_ << "x" << height_ 
              << ", framebuffer=" << framebuffer_width_ << "x" << framebuffer_height_
              << ", actual_scale=" << actual_scale
              << ", dpi=" << dpi_
              << ", pixelsize=" << pixelsize_ << std::endl;
    
    // If DPI changed significantly, clear icon cache
    if (initialized_ && std::abs(pixelsize_ - previous_pixelsize) > 0.01f) {
        std::cout << "DPI changed from " << previous_pixelsize << " to " << pixelsize_ 
                  << ", clearing icon cache" << std::endl;
        
        // Clear all cached icon sizes to force re-rendering at new DPI
        IconSystem& icon_system = IconSystem::get_instance();
        icon_system.clear_all_caches();
    }
}

void CanvasWindow::on_framebuffer_resize(int width, int height) {
    framebuffer_width_ = width;
    framebuffer_height_ = height;
    
    // Recalculate DPI and scaling when framebuffer changes
    calculate_dpi_and_scaling();
    
    // Mark viewport as needing update - render thread will handle it
    viewport_needs_update_ = true;
    
    // Trigger resize callback for immediate layout update (professional UI pattern)
    if (resize_callback_) {
        resize_callback_(width, height);
    }
}

void CanvasWindow::on_mouse_button(int button, int action, int mods) {
    
    if (button >= 0 && button < 3) {
        mouse_buttons_[button] = (action == GLFW_PRESS);
    }
    
    keyboard_modifiers_ = static_cast<uint32_t>(mods);
    
    InputEvent event = create_input_event((action == GLFW_PRESS) ? EventType::MOUSE_PRESS : EventType::MOUSE_RELEASE);
    event.mouse_button = static_cast<MouseButton>(button);
    
    
    // Route to regions first (like reference implementation)
    if (region_manager_) {
        std::cout << "Trying to route event to regions directly" << std::endl;
        if (region_manager_->handle_event(event)) {
            std::cout << "Event handled by region system" << std::endl;
            return; // Event was handled by regions, don't pass to event router
        }
        std::cout << "Event not handled by regions, trying event router" << std::endl;
    }
    
    if (event_router_) {
        std::cout << "Routing event through event router" << std::endl;
        EventResult result = event_router_->route_event(event);
        std::cout << "Event router result: " << static_cast<int>(result) << std::endl;
    } else {
        std::cout << "ERROR: No event router!" << std::endl;
    }
}

void CanvasWindow::on_mouse_move(double x, double y) {
    // Convert mouse coordinates from window space to framebuffer space
    float scale = get_content_scale();
    last_mouse_pos_ = Point2D(static_cast<float>(x) * scale, static_cast<float>(y) * scale);
    
    
    InputEvent event = create_input_event(EventType::MOUSE_MOVE);
    
    // Debug: Log mouse move events only when dragging
    // Remove drag debug output
    
    // Route to regions first (like reference implementation)
    if (region_manager_) {
        if (region_manager_->handle_event(event)) {
            return; // Event was handled by regions
        }
    }
    
    if (event_router_) {
        event_router_->route_event(event);
    }
}

bool CanvasWindow::is_smart_mouse_or_trackpad(double x_offset, double y_offset) {
    // Smart mouse/trackpad detection
    // The key difference: smart mouse sends continuous streams of values
    // while regular mouse wheel sends discrete integer steps
    
    // Any non-zero x_offset indicates trackpad/smart mouse
    if (std::abs(x_offset) > 0.001) {
        return true; // Has horizontal component = trackpad/smart mouse
    }
    
    // Check for traditional mouse wheel patterns
    // Regular mouse wheel typically sends exact integer values: ±1.0, ±2.0, ±3.0, etc.
    if (x_offset == 0.0) {
        double abs_y = std::abs(y_offset);
        
        // Check if this is exactly an integer (within floating point tolerance)
        double fractional_part = abs_y - std::floor(abs_y);
        if (fractional_part < 0.001 || fractional_part > 0.999) {
            // This looks like an integer value
            // But smart mouse can also send integer-like values in rapid succession
            // Check our recent history to distinguish
            
            // For now, only treat clean multiples of 1.0 as mouse wheel
            // Smart mouse tends to send many varying values, not clean integers
            if (abs_y == 1.0 || abs_y == 2.0 || abs_y == 3.0) {
                // Could be mouse wheel, but check context
                // Smart mouse sends many events rapidly with varying values
                // For simplicity, treat as smart mouse if values vary
                return false; // Assume regular mouse for now
            }
        }
        
        // Any fractional value indicates smart mouse
        return true;
    }
    
    // Default to smart mouse for safety
    return true;
}

void CanvasWindow::on_mouse_scroll(double x_offset, double y_offset) {
    // Check if we should ignore scroll events temporarily
    double now = glfwGetTime();
    if (scroll_ignore_until_ > 0 && now < scroll_ignore_until_) {
        // Ignoring event during modifier transition window
        return;
    }
    
    // Ignore tiny movements that might be noise
    if (std::abs(x_offset) < 0.001 && std::abs(y_offset) < 0.001) {
        return;
    }
    
    // Debug: Log scroll events periodically
    static double last_scroll_time = 0;
    static int scroll_event_count = 0;
    [[maybe_unused]] double time_delta = now - last_scroll_time;
    if (++scroll_event_count % 20 == 0) {  // Log every 20th event
        // Scroll event: x, y, modifiers, time_delta
    }
    last_scroll_time = now;
    
    // Track previous modifiers to detect mode changes
    static uint32_t previous_modifiers = 0;
    bool modifiers_changed = (previous_modifiers != keyboard_modifiers_);
    
    // If modifiers changed, skip this event to avoid jumps
    if (modifiers_changed && previous_modifiers != 0) {
        previous_modifiers = keyboard_modifiers_;
        return;  // Skip this event to prevent accumulated delta jump
    }
    previous_modifiers = keyboard_modifiers_;
    
    // Check if we have a native event with device info
    // Only look for non-pinch events here (pinch is handled in poll_events)
    voxelux::platform::NativeScrollEvent native_event;
    bool has_native_event = false;
    
    // Peek at native events without consuming them if they're pinch events
    while (voxelux::platform::NativeInput::get_next_scroll_event(native_event)) {
        if (!native_event.is_pinch) {
            has_native_event = true;
            break;  // Found a non-pinch event, use it
        }
        // Skip pinch events - they're handled in poll_events
    }
    
    bool is_smart_device = false;
    
    // Track pan state and timing
    static bool was_panning = false;
    static double pan_end_time = 0.0;
    bool shift_held = (keyboard_modifiers_ & static_cast<uint32_t>(KeyModifier::SHIFT)) != 0;
    // 'now' is already defined above
    
    if (has_native_event) {
        // NEVER allow momentum events to start a new gesture
        // Momentum can only continue an existing gesture
        if (native_event.is_momentum && current_gesture_ == GestureType::None) {
            // Ignoring momentum - cannot start new gesture with momentum
            return;
        }
        
        // Handle momentum scrolling intelligently:
        // ONLY block momentum for PAN (shift+scroll)
        // Allow momentum for rotate and zoom
        if (native_event.is_momentum) {
            // Only manage momentum for pan gestures
            if (shift_held || was_panning) {
                // If we ended a pan recently, block momentum for a short time
                if (pan_end_time > 0 && (now - pan_end_time) < 0.5) {
                    // Block momentum shortly after pan ends
                    return;
                }
                
                // If shift was just released, mark pan as ended
                if (was_panning && !shift_held) {
                    was_panning = false;
                    pan_end_time = now;
                    return;  // Block this momentum event
                }
                
                // If shift is pressed but we weren't panning, this is old momentum
                if (!was_panning && shift_held) {
                    // Blocking old momentum when starting new pan
                    return;  // Block old momentum when starting new pan
                }
            }
            // For non-pan gestures (rotate/zoom), allow momentum to pass through
        } else {
            // Non-momentum event
            if (shift_held) {
                was_panning = true;
                pan_end_time = 0;  // Clear end time
            }
        }
        
        // Use native detection - this is 100% accurate
        is_smart_device = (native_event.device_type == voxelux::platform::ScrollDeviceType::SmartMouse ||
                          native_event.device_type == voxelux::platform::ScrollDeviceType::Trackpad);
        
        // Check if this is a pinch gesture
        if (native_event.is_pinch) {
            // For pinch gestures, the zoom delta is in native_event.delta_y (scaled magnification)
            float pinch_delta = static_cast<float>(native_event.delta_y);
            // std::cout << "[Canvas] Handling pinch as TRACKPAD_ZOOM, pinch_delta=" << pinch_delta << std::endl;
            // Handle pinch gesture for zoom
            InputEvent event = create_input_event(EventType::TRACKPAD_ZOOM);
            event.wheel_delta = pinch_delta;  // Use the magnification delta
            event.mouse_delta = Point2D(0, pinch_delta);
            
            // Route through normal event system
            if (region_manager_) {
                if (region_manager_->handle_event(event)) {
                    // std::cout << "[Canvas] Event handled by region_manager" << std::endl;
                    return; // Event was handled by regions
                }
            }
            
            if (event_router_) {
                // std::cout << "[Canvas] Routing event through event_router" << std::endl;
                event_router_->route_event(event);
            }
            return; // Skip normal scroll handling
        }
        
        // Debug output shows native detection (commented out)
        // const char* device_name = "Unknown";
        // switch (native_event.device_type) {
        //     case voxelux::platform::ScrollDeviceType::MouseWheel:
        //         device_name = "MouseWheel";
        //         break;
        //     case voxelux::platform::ScrollDeviceType::Trackpad:
        //         device_name = "Trackpad";
        //         break;
        //     case voxelux::platform::ScrollDeviceType::SmartMouse:
        //         device_name = "SmartMouse";
        //         break;
        //     default:
        //         break;
        // }
        // std::cout << "[Native] " << device_name << " x=" << x_offset << " y=" << y_offset << std::endl;
    } else {
        // Fallback to pattern detection
        is_smart_device = is_smart_mouse_or_trackpad(x_offset, y_offset);
        static int pattern_debug = 0;
        if (++pattern_debug % 20 == 0) {
            std::cout << "[Pattern] smart=" << is_smart_device << " x=" << x_offset << " y=" << y_offset << std::endl;
        }
    }
    
    // Get current time for gesture tracking
    double current_time = glfwGetTime();
    double time_since_last_scroll = current_time - last_scroll_time_;
    last_scroll_time_ = current_time;
    
    // Check if gesture has timed out
    if (time_since_last_scroll > GESTURE_TIMEOUT) {
        current_gesture_ = GestureType::None;
    }
    
    EventType event_type;
    
    // Smart mouse/trackpad surface gestures for professional 3D navigation
    if (is_smart_device) {
        // Check modifiers to determine gesture type
        bool cmd_ctrl_held = (keyboard_modifiers_ & static_cast<uint32_t>(KeyModifier::CMD)) != 0 || 
                            (keyboard_modifiers_ & static_cast<uint32_t>(KeyModifier::CTRL)) != 0;
        
        // If we're in an active gesture, check if we should maintain it
        if (current_gesture_ != GestureType::None) {
            // For zoom gesture, immediately stop ALL processing if CMD/CTRL is released
            if (current_gesture_ == GestureType::Zoom && !cmd_ctrl_held) {
                current_gesture_ = GestureType::None;
                // Don't process this event at all - discard it
                return;
            }
            // For pan gesture, immediately stop ALL processing if shift is released  
            else if (current_gesture_ == GestureType::Pan && !shift_held) {
                current_gesture_ = GestureType::None;
                // Don't process this event at all - discard it
                return;
            }
            // For rotate gesture, check if modifiers were pressed (user wants different mode)
            else if (current_gesture_ == GestureType::Rotate && (cmd_ctrl_held || shift_held)) {
                current_gesture_ = GestureType::None;
                // Don't process this event - require fresh start
                return;
            }
            // Otherwise maintain the current gesture
            else {
                switch (current_gesture_) {
                    case GestureType::Pan:
                        event_type = EventType::TRACKPAD_PAN;
                        break;
                    case GestureType::Zoom:
                        event_type = EventType::TRACKPAD_SCROLL;
                        break;
                    case GestureType::Rotate:
                        event_type = EventType::TRACKPAD_ROTATE;
                        break;
                    default:
                        event_type = EventType::TRACKPAD_ROTATE;
                        break;
                }
            }
        }
        
        // If no active gesture, determine type based on current modifiers
        if (current_gesture_ == GestureType::None) {
            if (shift_held) {
                // Shift + trackpad surface = pan
                event_type = EventType::TRACKPAD_PAN;
                current_gesture_ = GestureType::Pan;
            } else if (cmd_ctrl_held) {
                // CMD/CTRL + trackpad = zoom (keep as scroll for handler to process)
                event_type = EventType::TRACKPAD_SCROLL;
                current_gesture_ = GestureType::Zoom;
            } else {
                // No modifiers with trackpad = rotation (orbit camera)
                // Two-finger scroll without modifiers is the standard rotation gesture
                event_type = EventType::TRACKPAD_ROTATE;
                current_gesture_ = GestureType::Rotate;
            }
        }
    } else {
        // Traditional mouse wheel
        event_type = EventType::MOUSE_WHEEL;
    }
    
    InputEvent event = create_input_event(event_type);
    
    // Only set wheel_delta for actual scroll/zoom events
    if (event_type == EventType::MOUSE_WHEEL || 
        event_type == EventType::TRACKPAD_SCROLL || 
        event_type == EventType::TRACKPAD_ZOOM) {
        event.wheel_delta = static_cast<float>(y_offset);
    }
    
    // Add trackpad-specific data for all smart device events
    if (is_smart_device) {
        event.trackpad.direction_inverted = natural_scroll_direction_;
        // Set device type flag based on native detection
        if (has_native_event) {
            event.trackpad.is_smart_mouse = (native_event.device_type == voxelux::platform::ScrollDeviceType::SmartMouse);
        } else {
            // Heuristic: if we have both X and Y deltas, it's likely a smart mouse
            event.trackpad.is_smart_mouse = (std::abs(x_offset) > 0.01 && std::abs(y_offset) > 0.01);
        }
        // Store both X and Y deltas for trackpad gestures EXCEPT zoom
        // Zoom should only use wheel_delta, not mouse_delta
        if (event_type != EventType::TRACKPAD_ZOOM) {
            // Use native event deltas if available
            // Otherwise fall back to GLFW offsets
            if (has_native_event && !native_event.is_pinch) {
                // Handle macOS axis swap restoration
                if (native_event.axis_swapped_by_os && event_type == EventType::TRACKPAD_PAN) {
                    // macOS swapped vertical to horizontal due to shift key
                    // When axis is swapped, delta_x contains what was originally delta_y
                    // We want to preserve both axes for proper 2D panning
                    // If only horizontal movement detected but shift is held, it was originally vertical
                    if (std::abs(native_event.delta_x) > 0.1 && std::abs(native_event.delta_y) < 0.1) {
                        // Pure vertical scroll that was converted to horizontal by macOS
                        event.mouse_delta = Point2D(0, static_cast<float>(native_event.delta_x));
                        std::cout << "[Canvas] Restored vertical pan: dy=" << native_event.delta_x << std::endl;
                    } else {
                        // Mixed movement or true horizontal - use as-is
                        event.mouse_delta = Point2D(static_cast<float>(native_event.delta_x), 
                                                   static_cast<float>(native_event.delta_y));
                    }
                } else {
                    // Use the raw deltas as-is
                    event.mouse_delta = Point2D(static_cast<float>(native_event.delta_x), 
                                               static_cast<float>(native_event.delta_y));
                }
            } else {
                // Fall back to GLFW offsets
                event.mouse_delta = Point2D(static_cast<float>(x_offset), static_cast<float>(y_offset));
            }
        }
    }
    
    // Event created with appropriate type and deltas
    
    // Route to regions first (like reference implementation)
    if (region_manager_) {
        if (region_manager_->handle_event(event)) {
            return; // Event was handled by regions
        }
    }
    
    if (event_router_) {
        event_router_->route_event(event);
    }
}

void CanvasWindow::on_key_event(int key, [[maybe_unused]] int scancode, int action, int mods) {
    // Store previous modifiers to detect changes
    uint32_t prev_modifiers = keyboard_modifiers_;
    keyboard_modifiers_ = static_cast<uint32_t>(mods);
    
    // Handle shift key changes
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        // Clear pending scroll events on both press and release
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            // Clear ALL pending native scroll events using the dedicated method
            voxelux::platform::NativeInput::clear_scroll_events();
            
            // Also set a flag to ignore scroll events for a brief period
            // This prevents any in-flight events from being processed
            scroll_ignore_until_ = glfwGetTime() + 0.05; // 50ms window
        }
    }
    
    if (action == GLFW_REPEAT) {
        return; // Skip repeat events for now
    }
    
    InputEvent event = create_input_event((action == GLFW_PRESS) ? EventType::KEY_PRESS : EventType::KEY_RELEASE);
    event.key_code = key;
    
    // If modifiers changed, also send a modifier change event
    // This helps navigation handler detect mode changes even without mouse movement
    if (prev_modifiers != keyboard_modifiers_) {
        // First route the key event
        if (event_router_) {
            event_router_->route_event(event);
        }
        
        // Then send a special modifier change notification
        InputEvent mod_event = create_input_event(EventType::MODIFIER_CHANGE);
        mod_event.modifiers = keyboard_modifiers_;
        if (event_router_) {
            event_router_->route_event(mod_event);
        }
    } else {
        if (event_router_) {
            event_router_->route_event(event);
        }
    }
}

// RegionManager implementation

RegionManager::RegionManager(CanvasWindow* window)
    : window_(window)
    , root_region_id_(0)
    , next_region_id_(1) {
    
    // Create default layout with single region
    create_default_layout();
}

RegionManager::~RegionManager() = default;

RegionID RegionManager::create_region(const Rect2D& bounds) {
    RegionID id = allocate_region_id();
    regions_[id] = std::make_unique<CanvasRegion>(id, bounds);
    return id;
}

void RegionManager::destroy_region(RegionID region_id) {
    regions_.erase(region_id);
}

CanvasRegion* RegionManager::get_region(RegionID region_id) const {
    auto it = regions_.find(region_id);
    return (it != regions_.end()) ? it->second.get() : nullptr;
}

void RegionManager::update_layout(const Rect2D& window_bounds) {
    if (auto* root = get_region(root_region_id_)) {
        root->set_bounds(window_bounds);
        root->update_layout();
    }
}

void RegionManager::render_regions() {
    if (auto* root = get_region(root_region_id_)) {
        root->render(window_->get_renderer());
    }
}

void RegionManager::create_default_layout() {
    // Create root region covering entire window
    Point2D window_size = window_->get_framebuffer_size();
    Point2D logical_size = window_->get_size();
    float content_scale = window_->get_content_scale();
    
    std::cout << "Window logical size: " << logical_size.x << "x" << logical_size.y << std::endl;
    std::cout << "Framebuffer size: " << window_size.x << "x" << window_size.y << std::endl;
    std::cout << "Content scale: " << content_scale << std::endl;
    
    Rect2D full_bounds(0, 0, window_size.x, window_size.y);
    root_region_id_ = create_region(full_bounds);
    
    std::cout << "Created root region with bounds: " << full_bounds.x << ", " << full_bounds.y 
              << ", " << full_bounds.width << ", " << full_bounds.height << std::endl;
    
    // Set default editor to 3D Viewport
    if (auto* root = get_region(root_region_id_)) {
        root->set_editor(EditorFactory::create_editor(EditorType::VIEWPORT_3D));
        std::cout << "Set default editor to 3D Viewport" << std::endl;
    }
}

bool RegionManager::handle_event(const InputEvent& event) {
    // Route event to root region, which will handle recursive routing
    if (auto* root = get_root_region()) {
        return root->handle_event(event);
    }
    return false;
}

CanvasRegion* RegionManager::get_root_region() const {
    return get_region(root_region_id_);
}

RegionID RegionManager::allocate_region_id() {
    return next_region_id_++;
}

// WorkspaceManager implementation

WorkspaceManager::WorkspaceManager(CanvasWindow* window)
    : window_(window)
    , active_workspace_("Default") {
}

WorkspaceManager::~WorkspaceManager() = default;

void WorkspaceManager::create_workspace(const std::string& name, const std::string& description) {
    WorkspaceData data;
    data.name = name;
    data.description = description;
    workspaces_[name] = data;
}

void WorkspaceManager::create_default_workspaces() {
    create_workspace("Default", "General voxel editing workspace");
    create_workspace("3D Editing", "Optimized for 3D voxel modeling and sculpting");
    create_workspace("Animation", "Timeline and animation-focused layout");
    active_workspace_ = "Default";
}

bool WorkspaceManager::set_active_workspace(const std::string& name) {
    if (workspaces_.find(name) != workspaces_.end()) {
        active_workspace_ = name;
        return true;
    }
    return false;
}

std::vector<std::string> WorkspaceManager::get_workspace_names() const {
    std::vector<std::string> names;
    for (const auto& pair : workspaces_) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace voxel_canvas