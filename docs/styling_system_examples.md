# Voxelux Styling System - Usage Examples

## Basic Text and Icons

### Example 1: Simple Text
```cpp
auto text = create_widget<Text>("Hello World");
text->get_style()
    .set_font_size(SpacingValue(16.0f))
    .set_text_color(ColorValue("gray_1"));
```

### Example 2: Icon Only
```cpp
auto icon = create_widget<Icon>("save_file", 24.0f);
icon->get_style()
    .set_text_color(ColorValue("accent_primary"));
```

### Example 3: Label with Icon and Text
```cpp
// Label automatically creates a row with icon and text
auto label = create_widget<Label>("Save Document", "save_file");
label->get_style()
    .set_padding(BoxSpacing::theme())
    .set_background(ColorValue("gray_4"));
```

## Button Examples

### Example 4: Text Button
```cpp
auto button = create_widget<Button>("Click Me");
button->set_on_click([](StyledWidget* w) {
    std::cout << "Button clicked!" << std::endl;
});
```

### Example 5: Icon Button
```cpp
auto icon_button = create_widget<Button>();
icon_button->set_icon("settings");
icon_button->get_style()
    .set_padding(BoxSpacing::all(8))
    .set_width(SizeValue(32))
    .set_height(SizeValue(32));
```

### Example 6: Button with Icon and Text
```cpp
auto save_button = create_widget<Button>("Save");
save_button->set_icon("save_file");
save_button->get_style()
    .set_padding(BoxSpacing(10, 20))  // Vertical: 10px, Horizontal: 20px
    .set_gap(SpacingValue::theme_sm());  // Gap between icon and text
```

## Layout Examples

### Example 7: Toolbar with Icons and Text
```cpp
// Create horizontal toolbar
auto toolbar = LayoutBuilder::row();
toolbar->get_style()
    .set_padding(BoxSpacing::all(5))
    .set_background(ColorValue("gray_5"))
    .set_gap(SpacingValue(5));

// Add file operations
auto new_btn = create_widget<Button>("New");
new_btn->set_icon("new_file");
toolbar->add_child(new_btn);

auto open_btn = create_widget<Button>("Open");
open_btn->set_icon("open_file");
toolbar->add_child(open_btn);

auto save_btn = create_widget<Button>("Save");
save_btn->set_icon("save_file");
toolbar->add_child(save_btn);

// Add spacer to push remaining items to the right
toolbar->add_child(LayoutBuilder::spacer());

// Add settings icon (no text)
auto settings_btn = create_widget<Button>();
settings_btn->set_icon("settings");
toolbar->add_child(settings_btn);
```

### Example 8: Sidebar with Icon Labels
```cpp
auto sidebar = LayoutBuilder::column();
sidebar->get_style()
    .set_width(SizeValue(200))
    .set_padding(BoxSpacing::theme())
    .set_background(ColorValue("gray_4"))
    .set_gap(SpacingValue(2));

// Add navigation items
std::vector<std::pair<std::string, std::string>> nav_items = {
    {"Projects", "folder"},
    {"Search", "search"},
    {"Settings", "settings"},
    {"Help", "help"}
};

for (const auto& [text, icon] : nav_items) {
    auto item = create_widget<Label>(text, icon);
    item->get_style()
        .set_padding(BoxSpacing(8, 12))
        .set_cursor("pointer");
    
    // Add hover effect
    item->get_style().hover_style = std::make_unique<WidgetStyle>();
    item->get_style().hover_style->background = ColorValue("gray_3");
    
    sidebar->add_child(item);
}
```

### Example 9: Card with Icon Header
```cpp
auto card = LayoutBuilder::column();
card->get_style()
    .set_padding(BoxSpacing::zero())
    .set_background(ColorValue("gray_3"))
    .set_border(BorderStyle{
        .width = SpacingValue(1),
        .color = ColorValue("gray_5"),
        .radius = SpacingValue(8)
    });

// Header with icon and title
auto header = LayoutBuilder::row();
header->get_style()
    .set_padding(BoxSpacing::theme())
    .set_background(ColorValue("gray_4"))
    .set_gap(SpacingValue::theme_sm())
    .set_align_items(WidgetStyle::AlignItems::Center);

header->add_child(create_widget<Icon>("document", 20));
header->add_child(create_widget<Text>("Document Properties"));

card->add_child(header);

// Content area
auto content = create_widget<Container>();
content->get_style().set_padding(BoxSpacing::theme());
// Add content widgets...
card->add_child(content);
```

### Example 10: Menu Item with Icon, Text, and Shortcut
```cpp
auto menu_item = LayoutBuilder::row();
menu_item->get_style()
    .set_padding(BoxSpacing(6, 12))
    .set_align_items(WidgetStyle::AlignItems::Center)
    .set_gap(SpacingValue::theme_md());

// Icon
auto icon = create_widget<Icon>("save_file", 16);
menu_item->add_child(icon);

// Label
auto label = create_widget<Text>("Save");
label->get_style().set_flex_grow(1);  // Take up available space
menu_item->add_child(label);

// Shortcut
auto shortcut = create_widget<Text>("Ctrl+S");
shortcut->get_style()
    .set_text_color(ColorValue("gray_2"))
    .set_font_size(SpacingValue(11));
menu_item->add_child(shortcut);
```

## Advanced Styling

### Example 11: Custom Icon Button Style
```cpp
// Define a reusable style for icon buttons
WidgetStyle icon_button_style;
icon_button_style
    .set_display(WidgetStyle::Display::Flex)
    .set_align_items(WidgetStyle::AlignItems::Center)
    .set_justify_content(WidgetStyle::JustifyContent::Center)
    .set_width(SizeValue(40))
    .set_height(SizeValue(40))
    .set_background(ColorValue("transparent"))
    .set_border(BorderStyle{
        .width = SpacingValue(0),
        .radius = SpacingValue(20)  // Circular
    })
    .set_cursor("pointer");

// Hover state
icon_button_style.hover_style = std::make_unique<WidgetStyle>();
icon_button_style.hover_style->background = ColorValue("gray_4");

// Active state
icon_button_style.active_style = std::make_unique<WidgetStyle>();
icon_button_style.active_style->background = ColorValue("accent_primary");

// Apply to multiple buttons
auto play_btn = create_widget<Icon>("play", 24);
play_btn->set_style(icon_button_style);

auto pause_btn = create_widget<Icon>("pause", 24);
pause_btn->set_style(icon_button_style);

auto stop_btn = create_widget<Icon>("stop", 24);
stop_btn->set_style(icon_button_style);
```

### Example 12: Status Bar with Multiple Icons
```cpp
auto status_bar = LayoutBuilder::row();
status_bar->get_style()
    .set_padding(BoxSpacing(4, 8))
    .set_background(ColorValue("gray_5"))
    .set_gap(SpacingValue(16))
    .set_align_items(WidgetStyle::AlignItems::Center);

// Status indicators with icons
auto add_status = [&](const std::string& icon, const std::string& text, const ColorRGBA& color) {
    auto group = LayoutBuilder::row();
    group->get_style()
        .set_gap(SpacingValue(4))
        .set_align_items(WidgetStyle::AlignItems::Center);
    
    auto icon_widget = create_widget<Icon>(icon, 14);
    icon_widget->get_style().set_text_color(ColorValue(color));
    group->add_child(icon_widget);
    
    auto text_widget = create_widget<Text>(text);
    text_widget->get_style()
        .set_font_size(SpacingValue(11))
        .set_text_color(ColorValue("gray_2"));
    group->add_child(text_widget);
    
    status_bar->add_child(group);
};

add_status("check", "Saved", ColorRGBA(0, 1, 0, 1));  // Green check
add_status("warning", "2 Warnings", ColorRGBA(1, 0.7f, 0, 1));  // Orange warning
add_status("error", "0 Errors", ColorRGBA(0.5f, 0.5f, 0.5f, 1));  // Gray (no errors)
```

## Integration with Existing MenuBar

Here's how the current MenuBar could be rewritten using the new system:

```cpp
class MenuBar : public StyledWidget {
public:
    MenuBar() : StyledWidget("menubar") {
        // Configure as horizontal flex container
        base_style_
            .set_display(WidgetStyle::Display::Flex)
            .set_flex_direction(WidgetStyle::FlexDirection::Row)
            .set_align_items(WidgetStyle::AlignItems::Center)
            .set_height(SizeValue(25))
            .set_background(ColorValue("gray_5"))
            .set_padding(BoxSpacing(0, SpacingValue::theme_md()));
        
        // Add logo
        auto logo = create_widget<Icon>("voxelux", 16);
        logo->get_style().set_margin(BoxSpacing(0, SpacingValue::theme_lg(), 0, 0));
        add_child(logo);
        
        // Add menu items
        add_menu("File", {
            {"New Project", "new_file", "Ctrl+N"},
            {"Open", "open_file", "Ctrl+O"},
            {"Save", "save_file", "Ctrl+S"}
        });
        
        add_menu("Edit", {
            {"Undo", "undo", "Ctrl+Z"},
            {"Redo", "redo", "Ctrl+Y"},
            {"Copy", "copy", "Ctrl+C"},
            {"Paste", "paste", "Ctrl+V"}
        });
        
        add_menu("View", {});
        add_menu("Window", {});
        add_menu("Help", {});
    }
    
private:
    void add_menu(const std::string& title, 
                  const std::vector<std::tuple<std::string, std::string, std::string>>& items) {
        auto menu_button = create_widget<Button>(title);
        menu_button->get_style()
            .set_background(ColorValue("transparent"))
            .set_border(BorderStyle{.width = SpacingValue(0)})
            .set_padding(BoxSpacing(0, SpacingValue::theme_md()));
        
        // Store items for dropdown
        menu_items_[title] = items;
        
        add_child(menu_button);
    }
    
    std::map<std::string, std::vector<std::tuple<std::string, std::string, std::string>>> menu_items_;
};
```

## Benefits of This Approach

1. **No Manual Calculations** - The layout system handles all positioning
2. **Consistent Spacing** - Using theme values ensures consistency
3. **Easy Hover/Active States** - Built into the style system
4. **Reusable Components** - Create once, use everywhere
5. **Flexible Layouts** - Change from row to column with one line
6. **Automatic DPI Scaling** - All sizes scaled through ScaledTheme

This system makes creating complex UIs with icons and text much simpler than manual pixel positioning!