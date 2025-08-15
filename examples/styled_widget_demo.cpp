/*
 * Example demonstrating the new styled widget system
 * Shows how to add icons and text using the new CSS-like API
 */

#include "canvas_ui/styled_widgets.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/layout_builder.h"

using namespace voxel_canvas;

// Example 1: Simple button with icon and text
std::shared_ptr<Button> create_save_button() {
    auto button = create_widget<Button>("Save Document");
    button->set_icon("save_file");
    button->get_style()
        .set_padding(BoxSpacing(8, 16))  // 8px vertical, 16px horizontal
        .set_gap(SpacingValue(8))        // 8px gap between icon and text
        .set_background(ColorValue("gray_4"))
        .set_border(BorderStyle{
            .width = SpacingValue(1),
            .color = ColorValue("gray_5"),
            .radius = SpacingValue(4)
        });
    
    // Add hover effect
    button->get_style().hover_style = std::make_unique<WidgetStyle>();
    button->get_style().hover_style->background = ColorValue("accent_primary");
    
    button->set_on_click([](StyledWidget* w) {
        std::cout << "Save button clicked!" << std::endl;
    });
    
    return button;
}

// Example 2: Toolbar with multiple icon buttons
std::shared_ptr<Container> create_toolbar() {
    auto toolbar = LayoutBuilder::row();
    toolbar->get_style()
        .set_height(SizeValue(40))
        .set_padding(BoxSpacing::all(4))
        .set_background(ColorValue("gray_5"))
        .set_gap(SpacingValue(4));
    
    // File operations
    std::vector<std::pair<std::string, std::string>> tools = {
        {"New", "new_file"},
        {"Open", "open_file"},
        {"Save", "save_file"},
        {"", "separator"},  // Visual separator
        {"Undo", "undo"},
        {"Redo", "redo"},
        {"", "separator"},
        {"Cut", "cut"},
        {"Copy", "copy"},
        {"Paste", "paste"}
    };
    
    for (const auto& [text, icon] : tools) {
        if (icon == "separator") {
            // Add a visual separator
            auto sep = create_widget<Container>();
            sep->get_style()
                .set_width(SizeValue(1))
                .set_background(ColorValue("gray_3"))
                .set_margin(BoxSpacing(4, 2));  // Vertical margin
            toolbar->add_child(sep);
        } else {
            auto btn = create_widget<Button>(text);
            btn->set_icon(icon);
            if (text.empty()) {
                // Icon-only button
                btn->get_style()
                    .set_width(SizeValue(32))
                    .set_height(SizeValue(32));
            }
            toolbar->add_child(btn);
        }
    }
    
    // Add spacer to push remaining items to the right
    toolbar->add_child(LayoutBuilder::spacer());
    
    // Settings button (icon only, aligned right)
    auto settings = create_widget<Button>();
    settings->set_icon("settings");
    settings->get_style()
        .set_width(SizeValue(32))
        .set_height(SizeValue(32));
    toolbar->add_child(settings);
    
    return toolbar;
}

// Example 3: Sidebar with labeled icons
std::shared_ptr<Container> create_sidebar() {
    auto sidebar = LayoutBuilder::column();
    sidebar->get_style()
        .set_width(SizeValue(200))
        .set_background(ColorValue("gray_4"))
        .set_padding(BoxSpacing::theme())
        .set_gap(SpacingValue(2));
    
    // Section header
    auto header = create_widget<Text>("PROJECT");
    header->get_style()
        .set_font_size(SpacingValue(10))
        .set_text_color(ColorValue("gray_2"))
        .set_padding(BoxSpacing(8, 4));
    sidebar->add_child(header);
    
    // Navigation items with icons
    std::vector<std::tuple<std::string, std::string, bool>> items = {
        {"Voxels", "cube", true},      // Selected
        {"Materials", "palette", false},
        {"Layers", "layers", false},
        {"Animation", "play", false},
        {"Settings", "settings", false}
    };
    
    for (const auto& [text, icon, selected] : items) {
        auto item = create_widget<Label>(text, icon);
        item->get_style()
            .set_padding(BoxSpacing(10, 12))
            .set_cursor("pointer");
        
        if (selected) {
            item->get_style()
                .set_background(ColorValue("accent_primary"))
                .set_text_color(ColorValue("white"));
        } else {
            // Hover effect for non-selected items
            item->get_style().hover_style = std::make_unique<WidgetStyle>();
            item->get_style().hover_style->background = ColorValue("gray_3");
        }
        
        sidebar->add_child(item);
    }
    
    return sidebar;
}

// Example 4: Property panel with icons and inputs
std::shared_ptr<Container> create_property_panel() {
    auto panel = LayoutBuilder::column();
    panel->get_style()
        .set_padding(BoxSpacing::theme())
        .set_gap(SpacingValue::theme_md());
    
    // Transform section
    auto transform_header = LayoutBuilder::row();
    transform_header->get_style()
        .set_gap(SpacingValue::theme_sm())
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    transform_header->add_child(create_widget<Icon>("transform", 16));
    transform_header->add_child(create_widget<Text>("Transform"));
    panel->add_child(transform_header);
    
    // Position row
    auto pos_row = LayoutBuilder::row();
    pos_row->get_style()
        .set_gap(SpacingValue::theme_sm())
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    pos_row->add_child(create_widget<Icon>("position", 14));
    pos_row->add_child(create_widget<Text>("Position"));
    // Would add input fields here
    panel->add_child(pos_row);
    
    // Rotation row
    auto rot_row = LayoutBuilder::row();
    rot_row->get_style()
        .set_gap(SpacingValue::theme_sm())
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    rot_row->add_child(create_widget<Icon>("rotate", 14));
    rot_row->add_child(create_widget<Text>("Rotation"));
    // Would add input fields here
    panel->add_child(rot_row);
    
    // Scale row
    auto scale_row = LayoutBuilder::row();
    scale_row->get_style()
        .set_gap(SpacingValue::theme_sm())
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    scale_row->add_child(create_widget<Icon>("scale", 14));
    scale_row->add_child(create_widget<Text>("Scale"));
    // Would add input fields here
    panel->add_child(scale_row);
    
    return panel;
}

// Example 5: Complete application layout
std::shared_ptr<Container> create_app_layout() {
    auto root = LayoutBuilder::column();
    root->get_style()
        .set_width(SizeValue::percent(100))
        .set_height(SizeValue::percent(100));
    
    // Add toolbar at top
    root->add_child(create_toolbar());
    
    // Main content area (horizontal split)
    auto content = LayoutBuilder::row();
    content->get_style()
        .set_flex_grow(1);  // Take remaining vertical space
    
    // Left sidebar
    content->add_child(create_sidebar());
    
    // Center viewport (would be the 3D view)
    auto viewport = create_widget<Container>();
    viewport->get_style()
        .set_flex_grow(1)  // Take remaining horizontal space
        .set_background(ColorValue("gray_3"));
    content->add_child(viewport);
    
    // Right properties panel
    auto props_panel = create_widget<Container>();
    props_panel->get_style()
        .set_width(SizeValue(250))
        .set_background(ColorValue("gray_4"))
        .set_padding(BoxSpacing::theme());
    props_panel->add_child(create_property_panel());
    content->add_child(props_panel);
    
    root->add_child(content);
    
    // Status bar at bottom
    auto status_bar = LayoutBuilder::row();
    status_bar->get_style()
        .set_height(SizeValue(24))
        .set_padding(BoxSpacing(2, 8))
        .set_background(ColorValue("gray_5"))
        .set_gap(SpacingValue(16))
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    // Add status items with icons
    auto add_status = [&](const std::string& icon, const std::string& text) {
        auto group = LayoutBuilder::row();
        group->get_style()
            .set_gap(SpacingValue(4))
            .set_align_items(WidgetStyle::AlignItems::Center);
        
        group->add_child(create_widget<Icon>(icon, 12));
        group->add_child(create_widget<Text>(text));
        status_bar->add_child(group);
    };
    
    add_status("info", "Ready");
    add_status("cube", "1,024 voxels");
    add_status("layers", "8 layers");
    
    root->add_child(status_bar);
    
    return root;
}

// Demonstration of how to use the new system in practice
void demonstrate_styled_widgets(CanvasRenderer* renderer) {
    // Create the complete app layout
    auto app = create_app_layout();
    
    // Compute styles with theme integration
    ScaledTheme theme = renderer->get_scaled_theme();
    app->compute_style(theme);
    
    // Set bounds to window size
    Point2D viewport_size = renderer->get_viewport_size();
    app->set_bounds(Rect2D(0, 0, viewport_size.x, viewport_size.y));
    
    // Perform layout
    app->perform_layout();
    
    // Render
    app->render(renderer);
    
    // Handle events
    InputEvent event;
    // ... get event from window system ...
    app->handle_event(event);
}