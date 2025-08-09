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
        // Only process mouse move if cursor is inside the window
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        if (x >= 0 && y >= 0 && x < width && y < height) {
            canvas_window->on_mouse_move(x, y);
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

// CanvasWindow implementation

CanvasWindow::CanvasWindow(int width, int height, const std::string& title)
    : window_(nullptr)
    , title_(title)
    , width_(width)
    , height_(height)
    , framebuffer_width_(width)
    , framebuffer_height_(height)
    , content_scale_(1.0f)
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
    
    // Enable multisampling for better quality
    glfwWindowHint(GLFW_SAMPLES, 4);
    
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
    theme_ = CanvasTheme(); // Uses default Blender-inspired colors
    renderer_->set_theme(theme_);

    // Initialize native input helper for better scroll detection
    if (voxelux::platform::NativeInput::is_available()) {
        if (voxelux::platform::NativeInput::initialize(window_)) {
            std::cout << "Native input helper initialized successfully" << std::endl;
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

void CanvasWindow::render_frame() {
    if (!initialized_ || !renderer_) {
        return;
    }

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
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    
    // Enable v-sync
    glfwSwapInterval(1);
    
    // Get framebuffer size and content scale
    glfwGetFramebufferSize(window_, &framebuffer_width_, &framebuffer_height_);
    
    float x_scale, y_scale;
    glfwGetWindowContentScale(window_, &x_scale, &y_scale);
    content_scale_ = x_scale; // Use x_scale as representative
    
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
}

InputEvent CanvasWindow::create_input_event(EventType type) const {
    InputEvent event;
    event.type = type;
    event.timestamp = glfwGetTime();
    event.mouse_pos = last_mouse_pos_;
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
    
    if (renderer_) {
        renderer_->set_viewport(0, 0, framebuffer_width_, framebuffer_height_);
    }
}

void CanvasWindow::on_framebuffer_resize(int width, int height) {
    framebuffer_width_ = width;
    framebuffer_height_ = height;
    
    if (renderer_) {
        renderer_->set_viewport(0, 0, width, height);
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
        std::cout << "Event router result: " << (int)result << std::endl;
    } else {
        std::cout << "ERROR: No event router!" << std::endl;
    }
}

void CanvasWindow::on_mouse_move(double x, double y) {
    last_mouse_pos_ = Point2D(static_cast<float>(x), static_cast<float>(y));
    
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
    // Ignore tiny movements that might be noise
    if (std::abs(x_offset) < 0.001 && std::abs(y_offset) < 0.001) {
        return;
    }
    
    // First check if we have a native event with device info
    voxelux::platform::NativeScrollEvent native_event;
    bool has_native_event = voxelux::platform::NativeInput::get_next_scroll_event(native_event);
    
    bool is_smart_device = false;
    
    if (has_native_event) {
        // Use native detection - this is 100% accurate
        is_smart_device = (native_event.device_type == voxelux::platform::ScrollDeviceType::SmartMouse ||
                          native_event.device_type == voxelux::platform::ScrollDeviceType::Trackpad);
        
        // Debug output shows native detection
        const char* device_name = "Unknown";
        switch (native_event.device_type) {
            case voxelux::platform::ScrollDeviceType::MouseWheel:
                device_name = "MouseWheel";
                break;
            case voxelux::platform::ScrollDeviceType::Trackpad:
                device_name = "Trackpad";
                break;
            case voxelux::platform::ScrollDeviceType::SmartMouse:
                device_name = "SmartMouse";
                break;
            default:
                break;
        }
        std::cout << "[Native] " << device_name << " x=" << x_offset << " y=" << y_offset << std::endl;
    } else {
        // Fallback to pattern detection
        is_smart_device = is_smart_mouse_or_trackpad(x_offset, y_offset);
        std::cout << "[Pattern] smart=" << is_smart_device << " x=" << x_offset << " y=" << y_offset << std::endl;
    }
    
    EventType event_type;
    
    // Smart mouse/trackpad surface gestures (following Blender's implementation)
    if (is_smart_device) {
        // Check modifiers to determine gesture type
        if (keyboard_modifiers_ & static_cast<uint32_t>(KeyModifier::CMD)) {
            // Cmd + trackpad surface = zoom
            event_type = EventType::TRACKPAD_ZOOM;
            // CMD + smart mouse = zoom
        } else if (keyboard_modifiers_ & static_cast<uint32_t>(KeyModifier::SHIFT)) {
            // Shift + trackpad surface = pan
            event_type = EventType::TRACKPAD_PAN; 
            // SHIFT + smart mouse = pan
        } else {
            // Trackpad surface alone = rotate/orbit
            event_type = EventType::TRACKPAD_ROTATE;
            // Smart mouse alone = orbit
        }
    } else {
        // Traditional mouse wheel or simple trackpad scroll
        event_type = is_smart_device ? EventType::TRACKPAD_SCROLL : EventType::MOUSE_WHEEL;
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
        // Store both X and Y deltas for trackpad gestures
        event.mouse_delta = Point2D(static_cast<float>(x_offset), static_cast<float>(y_offset));
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

void CanvasWindow::on_key_event(int key, int scancode, int action, int mods) {
    keyboard_modifiers_ = static_cast<uint32_t>(mods);
    
    if (action == GLFW_REPEAT) {
        return; // Skip repeat events for now
    }
    
    InputEvent event = create_input_event((action == GLFW_PRESS) ? EventType::KEY_PRESS : EventType::KEY_RELEASE);
    event.key_code = key;
    
    if (event_router_) {
        event_router_->route_event(event);
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