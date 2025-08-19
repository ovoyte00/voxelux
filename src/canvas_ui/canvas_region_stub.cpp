/*
 * Copyright (C) 2024 Voxelux
 * 
 * Canvas Region Stub Implementation
 * Temporary stub to allow compilation while CanvasRegion is being migrated
 */

#include "canvas_ui/canvas_region.h"
#include "canvas_ui/canvas_renderer.h"

namespace voxel_canvas {

// EditorSpace stub implementation - concrete class for stubs
class StubEditorSpace : public EditorSpace {
public:
    explicit StubEditorSpace(EditorType type) : EditorSpace(type) {}
    
    std::string get_name() const override {
        return "Stub Editor";
    }
    
    void render([[maybe_unused]] CanvasRenderer* renderer, [[maybe_unused]] const Rect2D& bounds) override {
        // TODO: Implement when migrated to new widget system
    }
    
    bool handle_event([[maybe_unused]] const InputEvent& event, [[maybe_unused]] const Rect2D& bounds) override {
        // TODO: Implement when migrated to new widget system
        return false;
    }
    
    void update([[maybe_unused]] float delta_time) override {}
};

// EditorSpace base class constructor
EditorSpace::EditorSpace(EditorType type) : type_(type) {}

// CanvasRegion implementation
CanvasRegion::CanvasRegion(RegionID id, const Rect2D& bounds) 
    : id_(id), 
      bounds_(bounds),
      show_header_(true),
      show_dropdown_(true),
      horizontal_split_(false),
      split_ratio_(0.5f),
      is_resizing_(false),
      invalidation_flags_(RegionUpdateFlags::FORCE_UPDATE),
      is_hovered_(false),
      header_hovered_(false) {
}

CanvasRegion::~CanvasRegion() = default;

void CanvasRegion::set_bounds(const Rect2D& bounds) {
    bounds_ = bounds;
    mark_size_changed();
    // Editor will be notified on next render
}

void CanvasRegion::render(CanvasRenderer* renderer) {
    // TODO: Implement when migrated to new widget system
    if (editor_) {
        editor_->render(renderer, get_content_bounds());
    }
}

bool CanvasRegion::handle_event(const InputEvent& event) {
    // TODO: Implement when migrated to new widget system
    if (editor_) {
        return editor_->handle_event(event, get_content_bounds());
    }
    return false;
}

void CanvasRegion::update_layout() {
    // TODO: Implement when migrated to new widget system
}

void CanvasRegion::set_editor(std::unique_ptr<EditorSpace> editor) {
    if (editor_) {
        editor_->shutdown();
    }
    editor_ = std::move(editor);
    if (editor_) {
        editor_->initialize();
    }
    mark_content_changed();
}

EditorType CanvasRegion::get_editor_type() const {
    return editor_ ? editor_->get_type() : EditorType::VIEWPORT_3D;
}

Rect2D CanvasRegion::get_header_bounds() const {
    if (!show_header_) {
        return Rect2D(bounds_.x, bounds_.y, bounds_.width, 0);
    }
    const float header_height = 25.0f; // TODO: Get from theme
    return Rect2D(bounds_.x, bounds_.y, bounds_.width, header_height);
}

Rect2D CanvasRegion::get_content_bounds() const {
    if (!show_header_) {
        return bounds_;
    }
    const float header_height = 25.0f; // TODO: Get from theme
    return Rect2D(bounds_.x, bounds_.y + header_height, 
                  bounds_.width, bounds_.height - header_height);
}

Point2D CanvasRegion::get_minimum_size() const {
    // Minimum size for a region
    return Point2D(100.0f, 100.0f);
}

bool CanvasRegion::can_resize_to(const Rect2D& new_bounds) const {
    Point2D min_size = get_minimum_size();
    return new_bounds.width >= min_size.x && new_bounds.height >= min_size.y;
}

void CanvasRegion::split_region([[maybe_unused]] bool horizontal, [[maybe_unused]] float ratio) {
    // TODO: Implement when migrated to new widget system
}

void CanvasRegion::update_children_bounds() {
    // TODO: Implement when migrated to new widget system
}

void CanvasRegion::render_resize_handles([[maybe_unused]] CanvasRenderer* renderer) {
    // TODO: Implement when migrated to new widget system
}

bool CanvasRegion::is_point_on_splitter([[maybe_unused]] const Point2D& point) const {
    // TODO: Implement when migrated to new widget system
    return false;
}

void CanvasRegion::setup_editor_dropdown() {
    // TODO: Implement when migrated to new widget system
}

void CanvasRegion::on_editor_type_changed([[maybe_unused]] EditorType new_type) {
    // TODO: Implement when migrated to new widget system
}

// EditorFactory stub implementation
std::unique_ptr<EditorSpace> EditorFactory::create_editor(EditorType type) {
    // Create a stub editor space for now
    return std::make_unique<StubEditorSpace>(type);
}

} // namespace voxel_canvas