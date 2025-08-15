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
#include <thread>
#include <chrono>

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
        
        // Setup event handlers after layout is initialized
        setup_event_handlers();
        setup_default_workspace();
        
        std::cout << "Voxelux initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        std::cout << "Starting Voxelux main loop..." << std::endl;
        std::cout << "Press ESC or close window to exit." << std::endl;
        
        while (!window_->should_close()) {
            window_->poll_events();
            
            // Handle window resize and DPI changes
            Point2D current_size = window_->get_framebuffer_size();
            float current_scale = window_->get_content_scale();
            static Point2D last_size = current_size;
            static float last_scale = current_scale;
            
            if (current_size.x != last_size.x || current_size.y != last_size.y) {
                layout_->on_window_resize(current_size.x, current_size.y);
                last_size = current_size;
            }
            
            if (current_scale != last_scale) {
                layout_->set_content_scale(current_scale);
                last_scale = current_scale;
                std::cout << "Content scale changed to: " << current_scale << "x" << std::endl;
            }
            
            // Custom render (replacing window_->render_frame())
            auto* renderer = window_->get_renderer();
            if (renderer) {
                renderer->begin_frame();
                
                // Render the VoxeluxLayout (which includes menu, docks, etc.)
                layout_->render(renderer);
                
                // TODO: Re-enable RegionManager once layout is working
                // For now, just render a test viewport in the central area
                /*
                if (auto* region_manager = window_->get_region_manager()) {
                    Rect2D central_area = layout_->get_central_area();
                    region_manager->update_layout(central_area);
                    region_manager->render_regions();
                }
                */
                
                renderer->end_frame();
                renderer->present_frame();
            }
            
            window_->swap_buffers();
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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