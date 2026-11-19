#pragma once

#include "Body.h"
#include "Camera.h"
#include "Cloth.h"
#include "Environment.h"
#include "FpsCounter.h"
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
    Body body;
    Cloth cloth;

    bool paused = false;
    double lastFrameTime = 0.0;
    double physicsAccumulator = 0.0;
    int substepsLastFrame = 0;
};
