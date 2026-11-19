#pragma once

#include "Vec3.h"

class OrbitCamera {
public:
    void apply(int width, int height) const;
    void update(float dt);

    void beginDrag(int x, int y);
    void dragTo(int x, int y);
    void endDrag();
    void rotate(float deltaYaw, float deltaPitch);
    void zoom(int steps);

    Vec3 eye() const;

private:
    Vec3 target_{0.0f, 1.0f, 0.0f};

    float yaw_ = 0.6f;
    float pitch_ = 0.22f;
    float distance_ = 6.5f;

    float goalYaw_ = 0.6f;
    float goalPitch_ = 0.22f;
    float goalDistance_ = 6.5f;

    bool dragging_ = false;
    int lastX_ = 0;
    int lastY_ = 0;
};
