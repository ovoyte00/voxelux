#include "theme.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

Theme& Theme::instance() {
    static Theme instance;
    return instance;
}

bool Theme::loadFromFile(const std::string& filepath) {
    QFile file(QString::fromStdString(filepath));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open theme file:" << filepath.c_str();
        setDefaults();
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in theme file:" << filepath.c_str();
        setDefaults();
        return false;
    }
    
    QJsonObject root = doc.object();
    theme_name_ = root["theme"].toString().toStdString();
    version_ = root["version"].toString().toStdString();
    
    QJsonObject colors = root["colors"].toObject();
    
    // Load axis colors
    QJsonObject axes = colors["axes"].toObject();
    axis_colors_.x_axis = jsonToColor3f(axes["x_axis"].toObject());
    axis_colors_.y_axis = jsonToColor3f(axes["y_axis"].toObject());
    axis_colors_.z_axis = jsonToColor3f(axes["z_axis"].toObject());
    
    // Load grid colors
    QJsonObject grid = colors["grid"].toObject();
    grid_colors_.major_lines = jsonToColor3f(grid["major_lines"].toObject());
    grid_colors_.minor_lines = jsonToColor3f(grid["minor_lines"].toObject());
    grid_colors_.origin = jsonToColor3f(grid["origin"].toObject());
    
    // Load UI colors
    QJsonObject ui = colors["ui"].toObject();
    ui_colors_.background = jsonToColor3f(ui["background"].toObject());
    ui_colors_.panel = jsonToColor3f(ui["panel"].toObject());
    ui_colors_.text = jsonToColor3f(ui["text"].toObject());
    ui_colors_.text_secondary = jsonToColor3f(ui["text_secondary"].toObject());
    ui_colors_.selection = jsonToColor3f(ui["selection"].toObject());
    ui_colors_.hover = jsonToColor3f(ui["hover"].toObject());
    
    // Load widget alpha values
    QJsonObject widget = colors["widget"].toObject();
    widget_alpha_.positive_sphere = widget["positive_sphere_alpha"].toDouble(1.0);
    widget_alpha_.negative_sphere_fill = widget["negative_sphere_fill_alpha"].toDouble(0.25);
    widget_alpha_.negative_sphere_ring = widget["negative_sphere_ring_alpha"].toDouble(0.8);
    widget_alpha_.line = widget["line_alpha"].toDouble(0.8);
    widget_alpha_.hover_background = widget["hover_background_alpha"].toDouble(0.3);
    
    qDebug() << "Loaded theme:" << theme_name_.c_str() << "version" << version_.c_str();
    return true;
}

void Theme::setDefaults() {
    // Fallback colors if JSON loading fails
    axis_colors_.x_axis = {0.96f, 0.24f, 0.37f}; // #F53E5F
    axis_colors_.y_axis = {0.49f, 0.79f, 0.24f}; // #7ECA3C
    axis_colors_.z_axis = {0.27f, 0.60f, 0.86f}; // #4498DB
    
    grid_colors_.major_lines = {0.3f, 0.3f, 0.3f};
    grid_colors_.minor_lines = {0.2f, 0.2f, 0.2f};
    grid_colors_.origin = {1.0f, 1.0f, 1.0f};
    
    ui_colors_.background = {0.168f, 0.168f, 0.168f};
    ui_colors_.panel = {0.196f, 0.196f, 0.196f};
    ui_colors_.text = {0.8f, 0.8f, 0.8f};
    ui_colors_.text_secondary = {0.6f, 0.6f, 0.6f};
    ui_colors_.selection = {0.337f, 0.502f, 0.761f};
    ui_colors_.hover = {1.0f, 1.0f, 1.0f};
    
    widget_alpha_.positive_sphere = 1.0f;
    widget_alpha_.negative_sphere_fill = 0.25f;
    widget_alpha_.negative_sphere_ring = 0.8f;
    widget_alpha_.line = 0.8f;
    widget_alpha_.hover_background = 0.3f;
}

Theme::Color3f Theme::jsonToColor3f(const QJsonObject& colorObj) const {
    QJsonArray rgb = colorObj["rgb"].toArray();
    if (rgb.size() != 3) {
        qWarning() << "Invalid RGB array in color definition";
        return {1.0f, 0.0f, 1.0f}; // Magenta for error
    }
    
    return {
        static_cast<float>(rgb[0].toDouble()),
        static_cast<float>(rgb[1].toDouble()),
        static_cast<float>(rgb[2].toDouble())
    };
}

QColor Theme::axisQColor(int axis) const {
    const Color3f* color = nullptr;
    switch (axis) {
        case 0: color = &axis_colors_.x_axis; break;
        case 1: color = &axis_colors_.y_axis; break;
        case 2: color = &axis_colors_.z_axis; break;
        default: 
            qWarning() << "Invalid axis index:" << axis;
            return QColor(255, 0, 255); // Magenta for error
    }
    
    return QColor::fromRgbF((*color)[0], (*color)[1], (*color)[2]);
}

Theme::Color4f Theme::axisColor4f(int axis, float alpha) const {
    const Color3f* color = nullptr;
    switch (axis) {
        case 0: color = &axis_colors_.x_axis; break;
        case 1: color = &axis_colors_.y_axis; break; 
        case 2: color = &axis_colors_.z_axis; break;
        default:
            qWarning() << "Invalid axis index:" << axis;
            return {1.0f, 0.0f, 1.0f, alpha}; // Magenta for error
    }
    
    return {(*color)[0], (*color)[1], (*color)[2], alpha};
}