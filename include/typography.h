/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional typography and font management system.
 * Independent implementation using standard font loading techniques.
 */

#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QJsonObject>
#include <QString>
#include <string>
#include <unordered_map>

/**
 * @brief Centralized typography management for Voxelux
 * 
 * Loads typography settings from assets/styles/typography.json and provides
 * consistent font access throughout the codebase. Fonts, sizes, and weights
 * are defined once and referenced everywhere for visual consistency.
 */
class Typography {
public:
    // Font weight enumeration
    enum FontWeight {
        Normal = QFont::Normal,
        Bold = QFont::Bold,
        Light = QFont::Light
    };
    
    // Text style definition
    struct TextStyle {
        QString font_family;
        int size;
        FontWeight weight;
        std::string description;
    };
    
    // Font definition 
    struct FontDefinition {
        QString family_name;
        QString file_path;
        std::string description;
        int font_id = -1; // QFontDatabase ID after loading
    };
    
    // Singleton access
    static Typography& instance();
    
    // Configuration loading
    bool loadFromFile(const std::string& filepath);
    void setDefaults();
    
    // Font access methods
    QFont getFont(const std::string& style_name) const;
    QFont navigationLabelsFont() const { return getFont("navigation_labels"); }
    QFont uiPrimaryFont() const { return getFont("ui_primary"); }
    QFont uiSecondaryFont() const { return getFont("ui_secondary"); }
    QFont headingsFont() const { return getFont("headings"); }
    QFont codeFont() const { return getFont("code"); }
    
    // Text style access
    const TextStyle* getTextStyle(const std::string& style_name) const;
    
    // Font loading management
    bool loadApplicationFonts();
    
private:
    Typography() { setDefaults(); }
    ~Typography() = default;
    Typography(const Typography&) = delete;
    Typography& operator=(const Typography&) = delete;
    
    // JSON parsing helpers
    TextStyle jsonToTextStyle(const QJsonObject& styleObj) const;
    FontDefinition jsonToFontDefinition(const QJsonObject& fontObj) const;
    FontWeight stringToFontWeight(const QString& weight) const;
    
    // Data storage
    std::unordered_map<std::string, FontDefinition> font_definitions_;
    std::unordered_map<std::string, TextStyle> text_styles_;
    
    std::string typography_name_ = "voxelux_professional";
    std::string version_ = "1.0";
};