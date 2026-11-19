#pragma once

#include "Camera.h"
#include "Cloth.h"
#include "Environment.h"
#include "FpsCounter.h"
#include "TestSphere.h"
#include "ToonRamp.h"

// GLUT callbacks have no user pointer, so this struct is the single place
// they reach shared state through.
struct AppState {
    int windowWidth = 1280;
    int windowHeight = 720;

    FpsCounter fps;
    OrbitCamera camera;
    Environment environment;
    ToonRamp toonRamp;
    TestSphere sphere{Vec3(-1.4f, 0.7f, 0.3f), 0.7f, Vec3(0.95f, 0.78f, 0.62f), 40, 28};
    Cloth cloth;

    bool paused = false;
    double lastFrameTime = 0.0;
    double physicsAccumulator = 0.0;
    int substepsLastFrame = 0;
};
