#include "Camera.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

namespace {

const float kMinPitch = -0.05f;
const float kMaxPitch = 1.35f;
const float kMinDistance = 2.5f;
const float kMaxDistance = 16.0f;
const float kDragSensitivity = 0.006f;
const float kEaseRate = 10.0f;

} // namespace

void OrbitCamera::apply(int width, int height) const
{
    const double aspect = static_cast<double>(width) / std::max(height, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, aspect, 0.1, 200.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const Vec3 e = eye();
    gluLookAt(e.x, e.y, e.z, target_.x, target_.y, target_.z, 0.0, 1.0, 0.0);
}

void OrbitCamera::update(float dt)
{
    // Exponential easing: frame-rate independent, unlike "value += diff * 0.1".
    const float t = 1.0f - std::exp(-kEaseRate * dt);
    yaw_ += (goalYaw_ - yaw_) * t;
    pitch_ += (goalPitch_ - pitch_) * t;
    distance_ += (goalDistance_ - distance_) * t;
}

void OrbitCamera::beginDrag(int x, int y)
{
    dragging_ = true;
    lastX_ = x;
    lastY_ = y;
}

void OrbitCamera::dragTo(int x, int y)
{
    if (!dragging_) {
        return;
    }
    rotate(-(x - lastX_) * kDragSensitivity, (y - lastY_) * kDragSensitivity);
    lastX_ = x;
    lastY_ = y;
}

void OrbitCamera::endDrag()
{
    dragging_ = false;
}

void OrbitCamera::rotate(float deltaYaw, float deltaPitch)
{
    goalYaw_ += deltaYaw;
    goalPitch_ = std::min(kMaxPitch, std::max(kMinPitch, goalPitch_ + deltaPitch));
}

void OrbitCamera::zoom(int steps)
{
    goalDistance_ *= std::pow(0.9f, static_cast<float>(steps));
    goalDistance_ = std::min(kMaxDistance, std::max(kMinDistance, goalDistance_));
}

Vec3 OrbitCamera::eye() const
{
    const float horizontal = distance_ * std::cos(pitch_);
    return target_ + Vec3(horizontal * std::sin(yaw_),
                          distance_ * std::sin(pitch_),
                          horizontal * std::cos(yaw_));
}
