#pragma once

#include <vector>

#include "Body.h"
#include "Camera.h"
#include "Cloth.h"
#include "Environment.h"
#include "FpsCounter.h"
#include "Game.h"
#include "Knot.h"
#include "Pattern.h"
#include "PhotoLab.h"
#include "ToonRamp.h"
#include "Wind.h"

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
    Wind wind;
    Knot knot;
    Game game;

    std::vector<FabricPattern> patterns;
    FabricTexture fabric;
    int patternIndex = 3;
    PhotoLab photoLab;
    bool inPhotoLab = false;

    bool paused = false;
    bool showDebug = false;
    bool sandbox = false;
    bool gripKeyHeld = false;
    float timeScale = 1.0f;

    double lastFrameTime = 0.0;
    double physicsAccumulator = 0.0;
    int substepsLastFrame = 0;
};
