/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxelux - Professional voxel editing application with Canvas UI
 */

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "canvas_ui/canvas_window.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/event_router.h"
#include "canvas_ui/canvas_region.h"
#include "canvas_ui/voxelux_layout.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

using namespace voxel_canvas;

class VoxeluxApplication {
public:
    VoxeluxApplication() {
        window_ = std::make_unique<CanvasWindow>(1600, 1000, "Voxelux - Professional Voxel Editor");
        layout_ = std::make_unique<VoxeluxLayout>();
    }
    
    ~VoxeluxApplication() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "Initializing Voxelux with Canvas UI..." << std::endl;
        
        if (!window_->initialize()) {
            std::cerr << "Failed to initialize Canvas window" << std::endl;
            return false;
        }
        
        // Initialize the layout system with content scale for high-DPI displays
        Point2D window_size = window_->get_framebuffer_size();
        float content_scale = window_->get_content_scale();
        if (!layout_->initialize(window_size.x, window_size.y, content_scale)) {
            std::cerr << "Failed to initialize layout" << std::endl;
            return false;
        }
        
        std::cout << "Using content scale: " << content_scale << "x for high-DPI display" << std::endl;
        
        // Connect layout to the region manager for central viewport area
        auto* region_manager = window_->get_region_manager();
        layout_->set_region_manager(region_manager);
        
        // Setup resize callback for smooth window resizing
        window_->set_resize_callback([this](int width, int height) {
            // Update layout dimensions immediately
            layout_->on_window_resize(width, height);
            
            // During resize drag, the OS blocks the main loop on macOS/Windows
            // We must render immediately here for smooth resizing
            auto* renderer{window_->get_renderer()};
            if (renderer) {
                // Critical: Update OpenGL viewport immediately for correct rendering
                glViewport(0, 0, width, height);
                
                // Update renderer's viewport tracking
                renderer->set_viewport(0, 0, width, height);
                
                // Refresh layout with new dimensions
                layout_->refresh_layout(renderer);
                
                // Render frame immediately
                renderer->begin_frame();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                layout_->render(renderer);
                renderer->end_frame();
                renderer->present_frame();
                
                // Show the frame immediately
                window_->swap_buffers();
            }
        });
        
        // Setup event handlers after layout is initialized
        setup_event_handlers();
        setup_default_workspace();
        
        std::cout << "Voxelux initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        std::cout << "Starting Voxelux..." << std::endl;
        std::cout << "Press ESC or close window to exit." << std::endl;
        
        // Track frame timing for consistent updates
        auto last_frame_time{std::chrono::high_resolution_clock::now()};
        const auto target_frame_time{std::chrono::milliseconds(16)}; // ~60 FPS
        
        // Track size for detecting changes
        Point2D last_size{window_->get_framebuffer_size()};
        float last_scale{window_->get_content_scale()};
        
        // Main render loop with continuous updates
        while (!window_->should_close()) {
            auto frame_start{std::chrono::high_resolution_clock::now()};
            
            // Process events non-blocking for continuous rendering
            window_->poll_events();
            
            // Check for size or scale changes
            Point2D current_size{window_->get_framebuffer_size()};
            float current_scale{window_->get_content_scale()};
            
            bool size_changed{current_size.x != last_size.x || current_size.y != last_size.y};
            bool scale_changed{current_scale != last_scale};
            
            // Handle size changes immediately for smooth resizing
            if (size_changed) {
                layout_->on_window_resize(static_cast<int>(current_size.x), 
                                        static_cast<int>(current_size.y));
                last_size = current_size;
            }
            
            // Handle DPI scale changes
            if (scale_changed) {
                layout_->set_content_scale(current_scale);
                last_scale = current_scale;
                std::cout << "Content scale changed to: " << current_scale << "x" << std::endl;
            }
            
            // Update viewport if window system requires it
            if (window_->needs_viewport_update()) {
                window_->update_viewport_from_render_thread();
            }
            
            // Get renderer for this frame
            auto* renderer{window_->get_renderer()};
            if (!renderer) {
                continue;
            }
            
            // Refresh layout when size or scale changed
            if (size_changed || scale_changed) {
                layout_->refresh_layout(renderer);
            }
            
            // Render every frame for smooth updates
            renderer->begin_frame();
            
            // Clear background
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Render UI
            layout_->render(renderer);
            
            // Complete frame
            renderer->end_frame();
            renderer->present_frame();
            window_->swap_buffers();
            
            // Frame timing for consistent 60 FPS
            auto frame_end{std::chrono::high_resolution_clock::now()};
            auto elapsed{frame_end - frame_start};
            
            // Only sleep if we finished early
            if (elapsed < target_frame_time) {
                auto sleep_time{target_frame_time - elapsed};
                std::this_thread::sleep_for(sleep_time);
            }
        }
        
        std::cout << "Voxelux main loop ended" << std::endl;
    }
    
    void shutdown() {
        if (window_) {
            window_->shutdown();
        }
    }
    
private:
    void setup_event_handlers() {
        auto* event_router = window_->get_event_router();
        
        // Create a layout event handler that forwards events to VoxeluxLayout
        class LayoutEventHandler : public ContextualInputHandler {
        public:
            LayoutEventHandler(VoxeluxLayout* layout) 
                : ContextualInputHandler(InputContext::UI_WIDGET, 100)  // High priority for UI
                , layout_(layout) {}
            
            bool can_handle(const InputEvent& event) const override {
                // Layout can potentially handle all events
                return layout_ != nullptr;
            }
            
            EventResult handle_event(const InputEvent& event) override {
                if (layout_ && layout_->handle_event(event)) {
                    return EventResult::HANDLED;
                }
                return EventResult::IGNORED;
            }
            
        private:
            VoxeluxLayout* layout_;
        };
        
        // Add layout handler first (highest priority for UI elements)
        auto layout_handler = std::make_shared<LayoutEventHandler>(layout_.get());
        event_router->add_handler(layout_handler);
        
        // Add global shortcut handler
        auto global_shortcuts = std::make_shared<GlobalShortcutHandler>();
        
        // Register some basic shortcuts (using GLFW key codes)
        global_shortcuts->register_shortcut(256, 0, [this]() { // GLFW_KEY_ESCAPE
            std::cout << "ESC pressed - closing application" << std::endl;
            glfwSetWindowShouldClose(window_->get_glfw_handle(), GLFW_TRUE);
        });
        
        global_shortcuts->register_shortcut(81, 1024, [this]() { // Q key + GLFW_MOD_SUPER (Cmd on macOS)
            std::cout << "Quit shortcut pressed" << std::endl;
            glfwSetWindowShouldClose(window_->get_glfw_handle(), GLFW_TRUE);
        });
        
        event_router->add_handler(global_shortcuts);
        
        // Add region interaction handler
        auto region_handler = std::make_shared<RegionInteractionHandler>(window_.get());
        event_router->add_handler(region_handler);
    }
    
    void setup_default_workspace() {
        auto* workspace_manager = window_->get_workspace_manager();
        
        // Set up default "3D Editing" workspace
        workspace_manager->set_active_workspace("3D Editing");
        
        std::cout << "Active workspace: " << workspace_manager->get_active_workspace() << std::endl;
        
        // The region manager already creates a default 3D viewport
        // TODO: Later we'll add more complex layout configurations
    }
    
    std::unique_ptr<CanvasWindow> window_;
    std::unique_ptr<VoxeluxLayout> layout_;
};

int main(int argc, char* argv[]) {
    std::cout << "Voxelux v0.1.0 - Professional Voxel Editor" << std::endl;
    std::cout << "Copyright (C) 2024 Voxelux. All rights reserved." << std::endl;
    std::cout << std::endl;
    
    try {
        VoxeluxApplication app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize Voxelux application" << std::endl;
            return 1;
        }
        
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
    
    std::cout << "Voxelux exited successfully" << std::endl;
    return 0;
}