#include "typography.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

Typography& Typography::instance() {
    static Typography instance;
    return instance;
}

bool Typography::loadFromFile(const std::string& filepath) {
    QFile file(QString::fromStdString(filepath));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open typography file:" << filepath.c_str();
        setDefaults();
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in typography file:" << filepath.c_str();
        setDefaults();
        return false;
    }
    
    QJsonObject root = doc.object();
    typography_name_ = root["typography"].toString().toStdString();
    version_ = root["version"].toString().toStdString();
    
    // Load font definitions
    QJsonObject fonts = root["fonts"].toObject();
    for (auto it = fonts.begin(); it != fonts.end(); ++it) {
        font_definitions_[it.key().toStdString()] = jsonToFontDefinition(it.value().toObject());
    }
    
    // Load text styles
    QJsonObject textStyles = root["text_styles"].toObject();
    for (auto it = textStyles.begin(); it != textStyles.end(); ++it) {
        text_styles_[it.key().toStdString()] = jsonToTextStyle(it.value().toObject());
    }
    
    // Load application fonts
    loadApplicationFonts();
    
    qDebug() << "Loaded typography:" << typography_name_.c_str() << "version" << version_.c_str();
    return true;
}

void Typography::setDefaults() {
    // Fallback typography configuration
    typography_name_ = "voxelux_professional";
    version_ = "1.0";
    
    // Default font definition
    FontDefinition primaryFont;
    primaryFont.family_name = "Arial"; // System fallback
    primaryFont.file_path = "";
    primaryFont.description = "System fallback font";
    font_definitions_["primary"] = primaryFont;
    
    // Default text styles
    TextStyle navLabels;
    navLabels.font_family = "primary";
    navLabels.size = 14;
    navLabels.weight = Bold;
    navLabels.description = "Navigation widget axis labels";
    text_styles_["navigation_labels"] = navLabels;
    
    TextStyle uiPrimary;
    uiPrimary.font_family = "primary";
    uiPrimary.size = 12;
    uiPrimary.weight = Normal;
    uiPrimary.description = "Primary UI text";
    text_styles_["ui_primary"] = uiPrimary;
    
    TextStyle uiSecondary;
    uiSecondary.font_family = "primary";
    uiSecondary.size = 10;
    uiSecondary.weight = Normal;
    uiSecondary.description = "Secondary UI text";
    text_styles_["ui_secondary"] = uiSecondary;
    
    TextStyle headings;
    headings.font_family = "primary";
    headings.size = 16;
    headings.weight = Bold;
    headings.description = "Section headings";
    text_styles_["headings"] = headings;
    
    TextStyle code;
    code.font_family = "primary";
    code.size = 11;
    code.weight = Normal;
    code.description = "Code and monospace text";
    text_styles_["code"] = code;
}

bool Typography::loadApplicationFonts() {
    bool allLoaded = true;
    
    for (auto& [name, fontDef] : font_definitions_) {
        if (!fontDef.file_path.isEmpty()) {
            // Convert relative path to absolute path
            QString absolutePath = fontDef.file_path;
            if (!QDir::isAbsolutePath(absolutePath)) {
                absolutePath = QDir::currentPath() + "/" + fontDef.file_path;
            }
            qDebug() << "Attempting to load font from:" << absolutePath;
            
            // Try to load font from file
            int fontId = QFontDatabase::addApplicationFont(absolutePath);
            if (fontId != -1) {
                QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
                if (!fontFamilies.isEmpty()) {
                    fontDef.font_id = fontId;
                    fontDef.family_name = fontFamilies.first(); // Use actual family name from font file
                    qDebug() << "Loaded font:" << fontDef.family_name << "from" << fontDef.file_path << "- Available families:" << fontFamilies;
                } else {
                    qWarning() << "Failed to get font families for:" << fontDef.file_path;
                    allLoaded = false;
                }
            } else {
                qWarning() << "Failed to load font from:" << absolutePath;
                allLoaded = false;
            }
        }
    }
    
    return allLoaded;
}

QFont Typography::getFont(const std::string& style_name) const {
    const TextStyle* style = getTextStyle(style_name);
    if (!style) {
        qWarning() << "Text style not found:" << style_name.c_str();
        return QFont(); // Return default font
    }
    
    // Find the font definition
    auto fontIt = font_definitions_.find(style->font_family.toStdString());
    if (fontIt == font_definitions_.end()) {
        qWarning() << "Font definition not found:" << style->font_family;
        return QFont(); // Return default font
    }
    
    const FontDefinition& fontDef = fontIt->second;
    
    // Create QFont with the loaded font family
    QFont font(fontDef.family_name, style->size);
    font.setWeight(static_cast<QFont::Weight>(style->weight));
    
    return font;
}

const Typography::TextStyle* Typography::getTextStyle(const std::string& style_name) const {
    auto it = text_styles_.find(style_name);
    if (it != text_styles_.end()) {
        return &it->second;
    }
    return nullptr;
}

Typography::TextStyle Typography::jsonToTextStyle(const QJsonObject& styleObj) const {
    TextStyle style;
    style.font_family = styleObj["font_family"].toString();
    style.size = styleObj["size"].toInt(12);
    style.weight = stringToFontWeight(styleObj["weight"].toString("normal"));
    style.description = styleObj["description"].toString().toStdString();
    return style;
}

Typography::FontDefinition Typography::jsonToFontDefinition(const QJsonObject& fontObj) const {
    FontDefinition fontDef;
    fontDef.family_name = fontObj["family"].toString();
    fontDef.file_path = fontObj["path"].toString();
    fontDef.description = fontObj["description"].toString().toStdString();
    return fontDef;
}

Typography::FontWeight Typography::stringToFontWeight(const QString& weight) const {
    QString lowerWeight = weight.toLower();
    if (lowerWeight == "bold") return Bold;
    if (lowerWeight == "light") return Light;
    return Normal; // Default to normal weight
}