#include "text_renderer.h"
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QDebug>
#include <cmath>

TextRenderer::TextRenderer()
{
    // Font will be set in initialize() after typography system is loaded
}

TextRenderer::~TextRenderer()
{
    cleanup();
}

bool TextRenderer::initialize()
{
    if (initialized_) return true;
    
    initializeOpenGLFunctions();
    
    // Load font from typography system after it's initialized
    current_font_ = Typography::instance().navigationLabelsFont();
    qDebug() << "TextRenderer initialized with font:" << current_font_.family() << "size:" << current_font_.pixelSize() << "weight:" << current_font_.weight();
    
    // Clear any existing character textures to force regeneration with new padding
    for (auto& pair : characters_) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    characters_.clear();
    
    // Setup shaders
    setupShaders();
    
    // Create VAO and VBO
    vao_.create();
    vbo_.create();
    
    vao_.bind();
    vbo_.bind();
    
    // Set up vertex attributes for text rendering
    // Layout: position(3) + texCoord(2) + color(3) = 8 floats per vertex
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    
    vbo_.release();
    vao_.release();
    
    // Generate character textures
    generateCharacterTextures();
    
    initialized_ = true;
    return true;
}

void TextRenderer::cleanup()
{
    if (!initialized_) return;
    
    // Clean up character textures
    for (auto& pair : characters_) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    characters_.clear();
    
    if (atlas_texture_) {
        delete atlas_texture_;
        atlas_texture_ = nullptr;
    }
    
    if (vao_.isCreated()) vao_.destroy();
    if (vbo_.isCreated()) vbo_.destroy();
    
    initialized_ = false;
}

void TextRenderer::setupShaders()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec3 aColor;
        
        out vec2 TexCoord;
        out vec3 TextColor;
        
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
            TextColor = aColor;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        in vec3 TextColor;
        
        out vec4 FragColor;
        
        uniform sampler2D textTexture;
        
        void main() {
            vec4 texColor = texture(textTexture, TexCoord);
            float alpha = texColor.a;
            FragColor = vec4(TextColor, alpha);
        }
    )";
    
    text_shader_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    text_shader_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    text_shader_.link();
    
    if (!text_shader_.isLinked()) {
        qWarning() << "Text shader linking failed:" << text_shader_.log();
    }
}

void TextRenderer::generateCharacterTextures()
{
    // Generate textures for ASCII printable characters (32-126)
    QFontMetrics metrics(current_font_);
    
    for (int c = 32; c <= 126; c++) {
        QChar ch(c);
        characters_[ch] = loadCharacter(ch);
    }
}

TextRenderer::Character TextRenderer::loadCharacter(QChar c)
{
    QFontMetrics metrics(current_font_);
    QRect boundingRect = metrics.boundingRect(c);
    
    // Create texture sized to fit the character with reasonable padding
    int padding = 4;  // Reasonable padding
    // Use the actual character bounds plus padding
    int width = boundingRect.width() + padding * 2;
    int height = metrics.ascent() + metrics.descent() + padding * 2;
    
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::black); // Black background
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(current_font_);
    painter.setPen(Qt::white); // White text
    
    // Draw the character properly centered in the texture
    // Use baseline positioning to ensure character fits within texture bounds
    int x = padding - boundingRect.left();  // Left align with padding
    int y = padding + metrics.ascent();      // Position at baseline with padding from top
    painter.drawText(x, y, c);
    painter.end();
    
    // Convert to OpenGL texture with alpha channel for proper text rendering
    QImage image = pixmap.toImage();
    image = image.convertToFormat(QImage::Format_RGBA8888);
    
    // Extract alpha from the white text on black background
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            QColor pixel = image.pixelColor(x, y);
            // Use the luminance as alpha (white = 1.0, black = 0.0)
            int alpha = qGray(pixel.rgb());
            image.setPixelColor(x, y, QColor(255, 255, 255, alpha));
        }
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Use GL_RGBA format for proper alpha blending
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    Character character;
    character.textureId = texture;
    character.size = QSize(width, height);
    
    // Update bearing to match our new character positioning
    // bearing.x = distance from left edge of texture to left edge of character
    // bearing.y = distance from baseline to top of texture
    character.bearing = QPoint(padding, metrics.ascent() + padding);
    character.advance = metrics.horizontalAdvance(c);
    
    return character;
}

TextRenderer::Character TextRenderer::loadStringTexture(const QString& text, float offsetX, float offsetY)
{
    QFontMetrics metrics(current_font_);
    QRect boundingRect = metrics.boundingRect(text);
    
    // Use higher resolution for sharper text (2x supersample)
    int supersample = 2;
    int logicalPadding = 4;
    int physicalPadding = logicalPadding * supersample;
    
    // Use advance width for texture sizing to ensure proper spacing
    int textWidth = metrics.horizontalAdvance(text);
    int textHeight = metrics.ascent() + metrics.descent();
    
    // Create texture sized for the entire string at supersample resolution
    int width = (textWidth + logicalPadding * 2) * supersample;
    int height = (textHeight + logicalPadding * 2) * supersample;
    
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::black); // Black background
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Use higher quality font rendering
    QFont renderFont = current_font_;
    renderFont.setPixelSize(current_font_.pixelSize() * supersample);
    painter.setFont(renderFont);
    painter.setPen(Qt::white); // White text
    
    // Center the text within the texture both horizontally and vertically
    QFontMetrics superMetrics(renderFont);
    QRect textBounds = superMetrics.boundingRect(text);
    
    // Horizontal centering: center the actual text width within the texture
    int x = (width - textBounds.width()) / 2 - textBounds.left();
    
    // Vertical centering: center the text height within the texture
    int textCenterY = (height - textBounds.height()) / 2;
    int y = textCenterY - textBounds.top();
    
    // Apply manual offsets for fine-tuning (scaled by supersample)
    x += offsetX * supersample;
    y += offsetY * supersample;
    
    painter.drawText(x, y, text);  // Draw entire string at once
    painter.end();
    
    // Convert to OpenGL texture with alpha channel
    QImage image = pixmap.toImage();
    image = image.convertToFormat(QImage::Format_RGBA8888);
    
    // Extract alpha from the white text on black background
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            QColor pixel = image.pixelColor(x, y);
            int alpha = qGray(pixel.rgb());
            image.setPixelColor(x, y, QColor(255, 255, 255, alpha));
        }
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Use better filtering for sharper text
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    Character stringChar;
    stringChar.textureId = texture;
    // Return logical size based on original font metrics (what final rendering should be)
    stringChar.size = QSize(textWidth + logicalPadding * 2, textHeight + logicalPadding * 2);
    // Bearing uses logical padding values for proper positioning
    stringChar.bearing = QPoint(logicalPadding, metrics.ascent() + logicalPadding);
    stringChar.advance = metrics.horizontalAdvance(text);
    
    /*
    // DEBUG: Print all the values to understand what's happening
    printf("=== loadStringTexture DEBUG for '%s' ===\n", text.toStdString().c_str());
    printf("Font pixel size: %d\n", current_font_.pixelSize());
    printf("supersample: %d\n", supersample);
    printf("logicalPadding: %d\n", logicalPadding);
    printf("textWidth: %d, textHeight: %d\n", textWidth, textHeight);
    printf("Physical texture: %dx%d\n", width, height);
    printf("Returned size: %dx%d\n", stringChar.size.width(), stringChar.size.height());
    printf("Returned bearing: (%d, %d)\n", stringChar.bearing.x(), stringChar.bearing.y());
    printf("Returned advance: %d\n", stringChar.advance);
    printf("========================================\n");
    */

    return stringChar;
}

void TextRenderer::setFont(const QFont& font)
{
    if (current_font_ == font) return;
    
    // Clean up old textures
    for (auto& pair : characters_) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    characters_.clear();
    
    current_font_ = font;
    
    if (initialized_) {
        generateCharacterTextures();
    }
}

void TextRenderer::setFontSize(int pixelSize)
{
    QFont newFont = current_font_;
    newFont.setPixelSize(pixelSize);
    setFont(newFont);
}

void TextRenderer::renderText(const QString& text, float x, float y, float scale, 
                             const QVector3D& color, const QMatrix4x4& projection,
                             float offsetX, float offsetY)
{
    current_projection_ = projection;
    renderTextInternal(text, x, y, scale, color, false, 0.0f, offsetX, offsetY);
}

void TextRenderer::renderTextCentered(const QString& text, float x, float y, float scale,
                                     const QVector3D& color, const QMatrix4x4& projection,
                                     float offsetX, float offsetY)
{
    current_projection_ = projection;
    renderTextInternal(text, x, y, scale, color, true, 0.0f, offsetX, offsetY);
}

void TextRenderer::renderTextCentered(const QString& text, float x, float y, float z, float scale,
                                     const QVector3D& color, const QMatrix4x4& projection,
                                     float offsetX, float offsetY)
{
    current_projection_ = projection;
    renderTextInternal(text, x, y, scale, color, true, z, offsetX, offsetY);
}

void TextRenderer::renderTextInternal(const QString& text, float x, float y, float scale, 
                                     const QVector3D& color, bool centered, float z, 
                                     float offsetX, float offsetY)
{
    if (!initialized_ || text.isEmpty()) return;
    
    // Always use single texture approach for consistent quality and simpler code
    renderStringAsTexture(text, x, y, scale, color, centered, z, offsetX, offsetY);
    return;
    
    // Enable blending for text rendering
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    if (!blendEnabled) {
        glEnable(GL_BLEND);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    text_shader_.bind();
    text_shader_.setUniformValue("projection", current_projection_);
    vao_.bind();
    
    // Render each character
    float currentX = x;
    for (const QChar& c : text) {
        auto it = characters_.find(c);
        if (it == characters_.end()) continue;
        
        const Character& ch = it->second;
        
        float xpos = currentX + ch.bearing.x() * scale;
        float ypos = y - ch.bearing.y() * scale;  // Invert Y bearing for proper baseline
        
        float w = ch.size.width() * scale;
        float h = ch.size.height() * scale;
       
        // Generate quad vertices for this character
        std::vector<float> vertices = {
            // positions        // texture coords   // colors
            xpos,     ypos + h,   z,   0.0f, 0.0f,   color.x(), color.y(), color.z(),
            xpos,     ypos,       z,   0.0f, 1.0f,   color.x(), color.y(), color.z(),
            xpos + w, ypos,       z,   1.0f, 1.0f,   color.x(), color.y(), color.z(),
            
            xpos,     ypos + h,   z,   0.0f, 0.0f,   color.x(), color.y(), color.z(),
            xpos + w, ypos,       z,   1.0f, 1.0f,   color.x(), color.y(), color.z(),
            xpos + w, ypos + h,   z,   1.0f, 0.0f,   color.x(), color.y(), color.z()
        };
        
        // Upload vertex data
        vbo_.bind();
        vbo_.allocate(vertices.data(), vertices.size() * sizeof(float));
        
        // Bind character texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ch.textureId);
        text_shader_.setUniformValue("textTexture", 0);
        
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Advance to next character
        currentX += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels
    }
    
    vao_.release();
    text_shader_.release();
    
    // Restore blend state
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
}

void TextRenderer::beginBatch(const QMatrix4x4& projection)
{
    if (batching_active_) return;
    
    batching_active_ = true;
    batch_projection_ = projection;
    batch_vertices_.clear();
}

void TextRenderer::addText(const QString& text, float x, float y, float scale, const QVector3D& color)
{
    if (!batching_active_) return;
    
    // Add text vertices to batch (implementation would be similar to renderTextInternal)
    // This is a placeholder for batch rendering optimization
}

void TextRenderer::addTextCentered(const QString& text, float x, float y, float scale, const QVector3D& color)
{
    if (!batching_active_) return;
    
    // Add centered text vertices to batch
    // This is a placeholder for batch rendering optimization
}

void TextRenderer::endBatch()
{
    if (!batching_active_) return;
    
    if (!batch_vertices_.empty()) {
        // Render all batched vertices at once
        // This would be implemented for performance optimization
    }
    
    batching_active_ = false;
    batch_vertices_.clear();
}

void TextRenderer::renderStringAsTexture(const QString& text, float x, float y, float scale, 
                                             const QVector3D& color, bool centered, float z,
                                             float offsetX, float offsetY)
{
    if (!initialized_ || text.isEmpty()) return;
    
    // Create or get string texture with offsets
    Character stringTexture = loadStringTexture(text, offsetX, offsetY);
    
    // Calculate positioning for centering
    if (centered) {
        float textureWidth = stringTexture.size.width() * scale;
        float textureHeight = stringTexture.size.height() * scale;
        float bearingX = stringTexture.bearing.x() * scale;
        float bearingY = stringTexture.bearing.y() * scale;
        
        // Center the string texture just like single characters
        x = x - bearingX - textureWidth * 0.5f;
        y = y + bearingY - textureHeight * 0.5f;
    }
    
    // Enable blending for text rendering
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    if (!blendEnabled) {
        glEnable(GL_BLEND);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    text_shader_.bind();
    text_shader_.setUniformValue("projection", current_projection_);
    vao_.bind();
    
    // Calculate rendering position
    float xpos = x + stringTexture.bearing.x() * scale;
    float ypos = y - stringTexture.bearing.y() * scale;
    float w = stringTexture.size.width() * scale;
    float h = stringTexture.size.height() * scale;
    
    /*
    // DEBUG: Print rendering values
    printf("=== renderStringAsTexture DEBUG for '%s' ===\n", text.toStdString().c_str());
    printf("Input: x=%.1f, y=%.1f, scale=%.2f\n", x, y, scale);
    printf("stringTexture.size: %dx%d\n", stringTexture.size.width(), stringTexture.size.height());
    printf("stringTexture.bearing: (%d, %d)\n", stringTexture.bearing.x(), stringTexture.bearing.y());
    printf("Final quad: xpos=%.1f, ypos=%.1f, w=%.1f, h=%.1f\n", xpos, ypos, w, h);
    printf("==========================================\n");
    */
    
    /*
    // DEBUG: Show red background for string texture placement
    if (text.length() > 0) {
        std::vector<float> bg_vertices = {
            // Red background to show string texture bounding box
            xpos,     ypos + h,   z - 0.001f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Red color
            xpos,     ypos,       z - 0.001f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,
            xpos + w, ypos,       z - 0.001f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
            
            xpos,     ypos + h,   z - 0.001f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
            xpos + w, ypos,       z - 0.001f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
            xpos + w, ypos + h,   z - 0.001f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f
        };
        
        // Upload and render background
        vbo_.bind();
        vbo_.allocate(bg_vertices.data(), bg_vertices.size() * sizeof(float));
        
        // Bind a white texture for background
        GLuint white_texture;
        glGenTextures(1, &white_texture);
        glBindTexture(GL_TEXTURE_2D, white_texture);
        unsigned char white_pixel[] = {255, 255, 255, 128}; // Semi-transparent white
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        text_shader_.setUniformValue("textTexture", 0);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteTextures(1, &white_texture);
        
        printf("String texture debug '%s': pos=(%.1f,%.1f), size=(%.1fx%.1f)\n", 
               text.toStdString().c_str(), xpos, ypos, w, h);
    }
    */
    
    // Generate quad vertices for the entire string texture
    std::vector<float> vertices = {
        // positions        // texture coords   // colors
        xpos,     ypos + h,   z,   0.0f, 0.0f,   color.x(), color.y(), color.z(),
        xpos,     ypos,       z,   0.0f, 1.0f,   color.x(), color.y(), color.z(),
        xpos + w, ypos,       z,   1.0f, 1.0f,   color.x(), color.y(), color.z(),
        
        xpos,     ypos + h,   z,   0.0f, 0.0f,   color.x(), color.y(), color.z(),
        xpos + w, ypos,       z,   1.0f, 1.0f,   color.x(), color.y(), color.z(),
        xpos + w, ypos + h,   z,   1.0f, 0.0f,   color.x(), color.y(), color.z()
    };
    
    // Upload vertex data
    vbo_.bind();
    vbo_.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    // Bind string texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stringTexture.textureId);
    text_shader_.setUniformValue("textTexture", 0);
    
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    vao_.release();
    text_shader_.release();
    
    // Restore blend state
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
}