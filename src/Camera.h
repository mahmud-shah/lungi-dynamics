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
    void faceFront();
    void setTargetHeight(float y) { goalTargetY_ = y; }

    Vec3 eye() const;

private:
    Vec3 target_{0.0f, 0.9f, 0.0f};

    float yaw_ = 0.6f;
    float pitch_ = 0.22f;
    float distance_ = 4.0f;

    float goalYaw_ = 0.6f;
    float goalPitch_ = 0.22f;
    float goalDistance_ = 4.0f;
    float goalTargetY_ = 0.9f;

    bool dragging_ = false;
    int lastX_ = 0;
    int lastY_ = 0;
};
