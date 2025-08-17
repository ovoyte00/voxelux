/*
 * Copyright (C) 2024 Voxelux
 * 
 * Dock Container - Manages multiple dock columns on one side
 * MIGRATED to new styled widget system
 */

#pragma once

#include "canvas_ui/styled_widgets.h"
#include "canvas_ui/dock_column.h"  // TODO: Migrate this too
#include "canvas_ui/drag_manager.h"
#include <vector>
#include <memory>
#include <functional>

namespace voxel_canvas {

class CanvasRenderer;

/**
 * DockContainer - Manages multiple dock columns on one side of the application
 * Now uses the new styled widget system with automatic layout
 */
class DockContainer : public Container {
public:
    enum class DockSide {
        LEFT,
        RIGHT,
        BOTTOM
    };
    
    DockContainer(DockSide side) : Container("dock-container"), side_(side) {
        // Configure styling based on side
        switch (side) {
            case DockSide::LEFT:
            case DockSide::RIGHT:
                base_style_.display = WidgetStyle::Display::Flex;
                base_style_.flex_direction = WidgetStyle::FlexDirection::Row;
                base_style_.width = SizeValue::auto_size();  // Auto-size based on content
                base_style_.height = SizeValue::percent(100);
                break;
            case DockSide::BOTTOM:
                base_style_.display = WidgetStyle::Display::Flex;
                base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
                base_style_.width = SizeValue::percent(100);
                base_style_.height = SizeValue::auto_size();
                break;
        }
        
        // No background, just 1px gray_5 outline with center alignment
        base_style_.background = ColorValue("transparent");
        base_style_.outline.width = SpacingValue(1);
        base_style_.outline.color = ColorValue("gray_5");
        base_style_.outline.style = WidgetStyle::OutlineStyle::Style::Solid;
        base_style_.outline.alignment = WidgetStyle::OutlineStyle::Alignment::Center;
        base_style_.gap = SpacingValue(0);  // No gap between columns - they touch
    }
    
    ~DockContainer() override = default;
    
    // Column management
    std::shared_ptr<DockColumn> add_column(DockColumn::ColumnType type = DockColumn::ColumnType::REGULAR) {
        // TODO: Create DockColumn using new system
        auto column = std::make_shared<DockColumn>(
            side_ == DockSide::LEFT ? DockColumn::DockSide::LEFT : DockColumn::DockSide::RIGHT,
            type
        );
        columns_.push_back(column);
        // Add as child for rendering
        add_child(column);
        
        if (column_change_callback_) {
            column_change_callback_();
        }
        
        invalidate_layout();
        return column;
    }
    
    void remove_column(std::shared_ptr<DockColumn> column) {
        auto it = std::find(columns_.begin(), columns_.end(), column);
        if (it != columns_.end()) {
            columns_.erase(it);
            remove_child(column);
            
            if (column_change_callback_) {
                column_change_callback_();
            }
            
            invalidate_layout();
        }
    }
    
    void clear_columns() {
        columns_.clear();
        remove_all_children();
        
        if (column_change_callback_) {
            column_change_callback_();
        }
        
        invalidate_layout();
    }
    
    std::vector<std::shared_ptr<DockColumn>>& get_columns() { return columns_; }
    bool has_columns() const { return !columns_.empty(); }
    
    float get_total_width() const {
        float total = 0;
        for (const auto& column : columns_) {
            total += column->get_current_width();
        }
        return total;
    }
    
    // Find specific column types
    std::shared_ptr<DockColumn> find_tool_column() const {
        for (const auto& column : columns_) {
            if (column->get_type() == DockColumn::ColumnType::TOOL) {
                return column;
            }
        }
        return nullptr;
    }
    
    std::shared_ptr<DockColumn> find_empty_column() const {
        for (const auto& column : columns_) {
            if (!column->has_panel_groups()) {
                return column;
            }
        }
        return nullptr;
    }
    
    // Panel docking
    std::shared_ptr<DockColumn> dock_panel(std::shared_ptr<Panel> panel, int column_index = -1) {
        std::shared_ptr<DockColumn> target_column;
        
        if (column_index >= 0 && column_index < columns_.size()) {
            target_column = columns_[column_index];
        } else {
            // Find or create suitable column
            target_column = find_empty_column();
            if (!target_column) {
                target_column = add_column();
            }
        }
        
        if (target_column) {
            target_column->add_panel(panel);
        }
        
        return target_column;
    }
    
    // Drop zone detection
    enum class DropZone {
        NONE,
        BEFORE_COLUMN,
        AFTER_COLUMN,
        INTO_COLUMN,
        INTO_GROUP,
        BETWEEN_GROUPS,
        EDGE
    };
    
    struct DropTarget {
        DropZone zone = DropZone::NONE;
        std::shared_ptr<DockColumn> column;
        std::shared_ptr<PanelGroup> group;
        int index = -1;
        Rect2D preview_bounds;
    };
    
    void handle_drop(const DragManager::DragData& drag_data, DropZone zone) {
        // Handle the drop based on zone
        switch (zone) {
            case DropZone::INTO_COLUMN:
                // Add to column
                break;
            case DropZone::BEFORE_COLUMN:
            case DropZone::AFTER_COLUMN:
                // Create new column
                break;
            default:
                break;
        }
    }
    
    DropTarget get_drop_target(const Point2D& pos, bool is_tool_panel = false) const {
        DropTarget target;
        
        // Check each column
        for (size_t i = 0; i < columns_.size(); ++i) {
            const auto& column = columns_[i];
            Rect2D column_bounds = column->get_bounds();
            
            // Check if position is over this column
            if (column_bounds.contains(pos)) {
                target.zone = DropZone::INTO_COLUMN;
                target.column = column;
                target.index = i;
                
                // TODO: Check for more specific drop zones within column
                break;
            }
            
            // Check for drop between columns
            if (i < columns_.size() - 1) {
                float gap_x = column_bounds.x + column_bounds.width;
                if (std::abs(pos.x - gap_x) < 10) {
                    target.zone = DropZone::BETWEEN_GROUPS;
                    target.index = i + 1;
                    break;
                }
            }
        }
        
        // Check for edge drop
        if (target.zone == DropZone::NONE) {
            Rect2D bounds = get_bounds();
            if (side_ == DockSide::LEFT && pos.x < bounds.x + 20) {
                target.zone = DropZone::EDGE;
                target.index = 0;
            } else if (side_ == DockSide::RIGHT && pos.x > bounds.x + bounds.width - 20) {
                target.zone = DropZone::EDGE;
                target.index = columns_.size();
            }
        }
        
        return target;
    }
    
    // Animation
    void update(float delta_time) {
        // Update column animations
        for (auto& column : columns_) {
            column->update(delta_time);
        }
        
        // Handle removing columns
        auto it = removing_columns_.begin();
        while (it != removing_columns_.end()) {
            // TODO: Animate column removal
            it = removing_columns_.erase(it);
        }
    }
    
    // StyledWidget interface
    std::string get_widget_type() const override { return "dock-container"; }
    
    // Callbacks
    using ColumnChangeCallback = std::function<void()>;
    void set_column_change_callback(ColumnChangeCallback callback) {
        column_change_callback_ = callback;
    }
    
protected:
    void render_content(CanvasRenderer* renderer) override {
        // Render drop zone preview if active
        if (current_drop_target_.zone != DropZone::NONE) {
            renderer->draw_rect(current_drop_target_.preview_bounds, 
                              ColorRGBA(0.2f, 0.5f, 1.0f, 0.3f));
        }
        
        // Base class handles rendering children (columns)
    }
    
private:
    DockSide side_;
    std::vector<std::shared_ptr<DockColumn>> columns_;
    std::vector<std::shared_ptr<DockColumn>> removing_columns_;
    
    mutable DropTarget current_drop_target_;
    ColumnChangeCallback column_change_callback_;
};

/**
 * BottomDockContainer - Special container for bottom dock with horizontal rows
 * Now uses the new styled widget system
 */
class BottomDockContainer : public Container {
public:
    BottomDockContainer() : Container("bottom-dock") {
        base_style_.display = WidgetStyle::Display::Flex;
        base_style_.flex_direction = WidgetStyle::FlexDirection::Column;
        base_style_.width = SizeValue::percent(100);
        base_style_.height = SizeValue::auto_size();
        base_style_.background = ColorValue("transparent");
        // Bottom dock gets its background from individual panels
    }
    
    ~BottomDockContainer() override = default;
    
    // Row management
    void add_row(std::shared_ptr<Panel> panel) {
        rows_.push_back(panel);
        row_heights_.push_back(DEFAULT_ROW_HEIGHT);
        
        // Create row container with splitter
        auto row_container = LayoutBuilder::column();
        row_container->set_id("row-" + panel->get_id());
        
        // Add panel
        panel->get_style().set_height(SizeValue(DEFAULT_ROW_HEIGHT));
        row_container->add_child(panel);
        
        // Add resize handle
        if (!rows_.empty()) {
            auto splitter = create_widget<Splitter>(Splitter::Orientation::HORIZONTAL);
            splitter->set_on_resize([this, index = rows_.size() - 1](float delta) {
                resize_row(index, delta);
            });
            row_container->add_child(splitter);
        }
        
        add_child(row_container);
        invalidate_layout();
    }
    
    void remove_row(std::shared_ptr<Panel> panel) {
        auto it = std::find(rows_.begin(), rows_.end(), panel);
        if (it != rows_.end()) {
            size_t index = std::distance(rows_.begin(), it);
            rows_.erase(it);
            row_heights_.erase(row_heights_.begin() + index);
            
            // Remove from children
            auto& children = get_children();
            auto child_it = std::find_if(children.begin(), children.end(),
                [&panel](const auto& child) {
                    return child->get_id() == "row-" + panel->get_id();
                });
            if (child_it != children.end()) {
                remove_child(*child_it);
            }
            
            invalidate_layout();
        }
    }
    
    void clear_rows() {
        rows_.clear();
        row_heights_.clear();
        remove_all_children();
        invalidate_layout();
    }
    
    std::vector<std::shared_ptr<Panel>>& get_rows() { return rows_; }
    bool has_rows() const { return !rows_.empty(); }
    
    float get_total_height() const {
        float total = 0;
        for (float height : row_heights_) {
            total += height;
        }
        total += RESIZE_HANDLE_HEIGHT * (rows_.size() - 1);
        return total;
    }
    
    void set_row_height(int row_index, float height) {
        if (row_index >= 0 && row_index < row_heights_.size()) {
            row_heights_[row_index] = std::max(MIN_ROW_HEIGHT, 
                                              std::min(MAX_ROW_HEIGHT, height));
            update_row_heights();
            invalidate_layout();
        }
    }
    
    std::string get_widget_type() const override { return "bottom-dock-container"; }
    
    // Override handle_event to handle resize handles
    bool handle_event(const InputEvent& event) override {
        // First check resize handles if we have rows
        if (!rows_.empty()) {
            auto resize_handle = get_resize_handle_at(event.mouse_pos);
            if (resize_handle >= 0) {
                if (event.type == EventType::MOUSE_PRESS && event.mouse_button == MouseButton::LEFT) {
                    resizing_row_index_ = resize_handle;
                    resize_start_y_ = event.mouse_pos.y;
                    resize_start_height_ = row_heights_[resize_handle];
                    return true;
                }
            }
            
            if (resizing_row_index_ >= 0) {
                if (event.type == EventType::MOUSE_MOVE) {
                    float delta = event.mouse_pos.y - resize_start_y_;
                    set_row_height(resizing_row_index_, resize_start_height_ + delta);
                    return true;
                } else if (event.type == EventType::MOUSE_RELEASE) {
                    resizing_row_index_ = -1;
                    return true;
                }
            }
        }
        
        // Let base class handle other events
        return Container::handle_event(event);
    }
    
    // Find which resize handle is at the given position
    int get_resize_handle_at(const Point2D& pos) const {
        // Ensure we have rows before accessing them
        if (rows_.empty()) {
            return -1;
        }
        
        Rect2D bounds = get_bounds();
        float y = bounds.y;
        
        // Check each resize handle between rows
        for (size_t i = 0; i < rows_.size() - 1; ++i) {
            // Ensure index is valid before accessing row_heights_
            if (i >= row_heights_.size()) {
                return -1;
            }
            
            y += row_heights_[i];
            
            // Check if mouse is over resize handle
            if (pos.y >= y - 2 && pos.y <= y + RESIZE_HANDLE_HEIGHT + 2) {
                return static_cast<int>(i);
            }
            
            y += RESIZE_HANDLE_HEIGHT;
        }
        
        return -1;
    }
    
private:
    std::vector<std::shared_ptr<Panel>> rows_;
    std::vector<float> row_heights_;
    
    // Resize tracking
    int resizing_row_index_ = -1;
    float resize_start_y_ = 0;
    float resize_start_height_ = 0;
    
    static constexpr float MIN_ROW_HEIGHT = 100.0f;
    static constexpr float MAX_ROW_HEIGHT = 500.0f;
    static constexpr float DEFAULT_ROW_HEIGHT = 200.0f;
    static constexpr float RESIZE_HANDLE_HEIGHT = 4.0f;
    
    void resize_row(int index, float delta) {
        if (index >= 0 && index < row_heights_.size()) {
            set_row_height(index, row_heights_[index] + delta);
        }
    }
    
    void update_row_heights() {
        for (size_t i = 0; i < rows_.size(); ++i) {
            rows_[i]->get_style().set_height(SizeValue(row_heights_[i]));
        }
    }
};

} // namespace voxel_canvas