#pragma once

#include <QTimer>
#include <QQuaternion>
#include <functional>
#include <memory>

/**
 * Generic smooth animation system for viewport transitions.
 * Inspired by professional 3D software animation systems.
 * Supports quaternion interpolation with professional easing curves.
 */
class ViewAnimation : public QObject {
    Q_OBJECT

public:
    // Animation easing types
    enum class EasingType {
        EaseInOut,      // Professional 3t² - 2t³ (recommended for view transitions)  
        EaseOut,        // 1 - (1-t)³ 
        EaseIn,         // t³
        Linear          // t
    };

    // Animation state callback
    using AnimationCallback = std::function<void(float progress)>;
    using CompletionCallback = std::function<void()>;

    explicit ViewAnimation(QObject* parent = nullptr);
    ~ViewAnimation();

    // Configuration
    void setDuration(float milliseconds);
    void setEasingType(EasingType type);
    void setFrameRate(int fps); // Default: 60fps

    // Animation control
    void startAnimation(AnimationCallback update_callback, CompletionCallback completion_callback = nullptr);
    void stopAnimation();
    bool isAnimating() const;

    // Duration calculation helpers
    static float calculateRotationDuration(const QQuaternion& from, const QQuaternion& to, float base_duration = 500.0f);

private slots:
    void updateAnimation();

private:
    float applyEasing(float t) const;

    QTimer* animation_timer_;
    float animation_progress_;
    float animation_duration_;
    EasingType easing_type_;
    bool is_animating_;

    AnimationCallback update_callback_;
    CompletionCallback completion_callback_;
};

/**
 * Camera-specific smooth transition system.
 * Handles quaternion interpolation for camera rotations.
 */
class CameraTransition {
public:
    explicit CameraTransition(QObject* parent = nullptr);

    // Camera transition types
    void startRotationTransition(const QQuaternion& from, const QQuaternion& to, 
                               std::function<void(const QQuaternion&)> update_callback,
                               std::function<void()> completion_callback = nullptr);

    void stopTransition();
    bool isTransitioning() const;

    // Configuration
    void setBaseDuration(float milliseconds) { base_duration_ = milliseconds; }
    void setEasingType(ViewAnimation::EasingType type) { animation_.setEasingType(type); }

private:
    ViewAnimation animation_;
    QQuaternion source_rotation_;
    QQuaternion target_rotation_;
    std::function<void(const QQuaternion&)> rotation_callback_;

    float base_duration_ = 500.0f;
};