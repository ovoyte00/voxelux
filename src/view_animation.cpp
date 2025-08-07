#include "view_animation.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

// ============================================================================
// ViewAnimation Implementation
// ============================================================================

ViewAnimation::ViewAnimation(QObject* parent)
    : QObject(parent)
    , animation_timer_(new QTimer(this))
    , animation_progress_(0.0f)
    , animation_duration_(500.0f)
    , easing_type_(EasingType::EaseInOut)
    , is_animating_(false)
{
    connect(animation_timer_, &QTimer::timeout, this, &ViewAnimation::updateAnimation);
    setFrameRate(60); // Default to 60fps
}

ViewAnimation::~ViewAnimation() {
    // Destructor implementation for vtable
}

void ViewAnimation::setDuration(float milliseconds) {
    animation_duration_ = std::max(50.0f, milliseconds); // Minimum 50ms
}

void ViewAnimation::setEasingType(EasingType type) {
    easing_type_ = type;
}

void ViewAnimation::setFrameRate(int fps) {
    int interval = std::max(8, 1000 / fps); // Maximum 125fps, minimum ~8fps
    animation_timer_->setInterval(interval);
}

void ViewAnimation::startAnimation(AnimationCallback update_callback, CompletionCallback completion_callback) {
    if (is_animating_) {
        stopAnimation();
    }

    update_callback_ = update_callback;
    completion_callback_ = completion_callback;
    animation_progress_ = 0.0f;
    is_animating_ = true;

    animation_timer_->start();
    
    // Call immediately with 0.0 progress
    if (update_callback_) {
        update_callback_(0.0f);
    }
}

void ViewAnimation::stopAnimation() {
    if (!is_animating_) return;

    animation_timer_->stop();
    is_animating_ = false;
    animation_progress_ = 0.0f;
}

bool ViewAnimation::isAnimating() const {
    return is_animating_;
}

void ViewAnimation::updateAnimation() {
    if (!is_animating_) return;

    // Increment progress based on timer interval
    animation_progress_ += animation_timer_->interval();
    float progress_factor = animation_progress_ / animation_duration_;

    if (progress_factor >= 1.0f) {
        // Animation complete
        progress_factor = 1.0f;
        is_animating_ = false;
        animation_timer_->stop();
    }

    // Apply easing curve
    float eased_progress = applyEasing(progress_factor);

    // Update animation
    if (update_callback_) {
        update_callback_(eased_progress);
    }

    // Call completion callback if finished
    if (progress_factor >= 1.0f && completion_callback_) {
        completion_callback_();
    }
}

float ViewAnimation::applyEasing(float t) const {
    switch (easing_type_) {
        case EasingType::EaseInOut:
            // Professional ease-in-out: 3t² - 2t³ (matches reference software)
            return (3.0f * t * t - 2.0f * t * t * t);
            
        case EasingType::EaseOut:
            // Ease-out cubic: 1 - (1-t)³
            return 1.0f - std::pow(1.0f - t, 3.0f);
            
        case EasingType::EaseIn:
            // Ease-in cubic: t³
            return t * t * t;
            
        case EasingType::Linear:
        default:
            return t;
    }
}

float ViewAnimation::calculateRotationDuration(const QQuaternion& from, const QQuaternion& to, float base_duration) {
    // Calculate angular distance between quaternions
    float dot_product = QQuaternion::dotProduct(from, to);
    float rotation_angle = 2.0f * std::acos(std::min(1.0f, std::abs(dot_product)));
    
    // Scale duration based on rotation angle (larger rotations take more time)
    float angle_factor = rotation_angle / (M_PI * 0.5f); // Normalize to quarter turn
    float duration = base_duration * (0.5f + angle_factor * 0.5f);
    
    // Clamp to reasonable bounds
    return std::max(0.0f, std::min(800.0f, duration));
}

// ============================================================================
// CameraTransition Implementation  
// ============================================================================

CameraTransition::CameraTransition(QObject* parent)
    : animation_(new ViewAnimation(parent))
{
    animation_.setEasingType(ViewAnimation::EasingType::EaseInOut);
}

void CameraTransition::startRotationTransition(const QQuaternion& from, const QQuaternion& to,
                                              std::function<void(const QQuaternion&)> update_callback,
                                              std::function<void()> completion_callback) {
    source_rotation_ = from;
    target_rotation_ = to;
    rotation_callback_ = update_callback;

    // Calculate appropriate duration for this rotation
    float duration = ViewAnimation::calculateRotationDuration(from, to, base_duration_);
    animation_.setDuration(duration);

    // Start animation with rotation interpolation
    animation_.startAnimation(
        [this](float progress) {
            // Spherical linear interpolation
            QQuaternion interpolated = QQuaternion::slerp(source_rotation_, target_rotation_, progress);
            if (rotation_callback_) {
                rotation_callback_(interpolated);
            }
        },
        completion_callback
    );
}

void CameraTransition::stopTransition() {
    animation_.stopAnimation();
}

bool CameraTransition::isTransitioning() const {
    return animation_.isAnimating();
}