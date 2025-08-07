#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QString>
#include <QFont>
#include <unordered_map>
#include <vector>
#include "typography.h"

/**
 * Professional OpenGL text rendering system with texture atlas caching.
 * Uses FreeType-style glyph rasterization for crisp, scalable text rendering.
 */
class TextRenderer : protected QOpenGLFunctions_3_3_Core
{
public:
    TextRenderer();
    ~TextRenderer();
    
    // Lifecycle
    bool initialize();
    void cleanup();
    
    // Text rendering
    void renderText(const QString& text, float x, float y, float scale, 
                   const QVector3D& color, const QMatrix4x4& projection,
                   float offsetX = 0.0f, float offsetY = 0.0f);
    void renderTextCentered(const QString& text, float x, float y, float scale,
                           const QVector3D& color, const QMatrix4x4& projection,
                           float offsetX = 0.0f, float offsetY = 0.0f);
    
    // Text rendering with depth
    void renderTextCentered(const QString& text, float x, float y, float z, float scale,
                           const QVector3D& color, const QMatrix4x4& projection,
                           float offsetX = 0.0f, float offsetY = 0.0f);
    
    // Font management
    void setFont(const QFont& font);
    void setFontSize(int pixelSize);
    
    // Batch rendering for performance
    void beginBatch(const QMatrix4x4& projection);
    void addText(const QString& text, float x, float y, float scale, const QVector3D& color);
    void addTextCentered(const QString& text, float x, float y, float scale, const QVector3D& color);
    void endBatch();
    
private:
    struct Character {
        GLuint textureId;    // Texture ID for the glyph
        QSize size;          // Size of glyph
        QPoint bearing;      // Offset from baseline to left/top of glyph
        GLuint advance;      // Offset to advance to next glyph
    };
    
    struct TextVertex {
        float position[3];   // x, y, z
        float texCoord[2];   // s, t
        float color[3];      // r, g, b
    };
    
    // Glyph management
    void generateCharacterTextures();
    Character loadCharacter(QChar c);
    Character loadStringTexture(const QString& text, float offsetX = 0.0f, float offsetY = 0.0f);
    void createTextureAtlas();
    
    // Rendering internals
    void setupShaders();
    void renderTextInternal(const QString& text, float x, float y, float scale, 
                           const QVector3D& color, bool centered, float z = 0.0f,
                           float offsetX = 0.0f, float offsetY = 0.0f);
    void renderStringAsTexture(const QString& text, float x, float y, float scale, 
                              const QVector3D& color, bool centered, float z = 0.0f,
                              float offsetX = 0.0f, float offsetY = 0.0f);
    
    // OpenGL resources
    QOpenGLShaderProgram text_shader_;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_;
    
    // Font and glyph data
    QFont current_font_;
    std::unordered_map<QChar, Character> characters_;
    
    // Batch rendering
    std::vector<TextVertex> batch_vertices_;
    bool batching_active_ = false;
    QMatrix4x4 batch_projection_;
    
    // Current rendering projection
    QMatrix4x4 current_projection_;
    
    // Atlas texture for efficient rendering
    QOpenGLTexture* atlas_texture_ = nullptr;
    int atlas_width_ = 512;
    int atlas_height_ = 512;
    
    bool initialized_ = false;
};