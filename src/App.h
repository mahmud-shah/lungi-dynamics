#pragma once

#include <string>
#include <vector>

#include "Audio.h"
#include "Body.h"
#include "Camera.h"
#include "Cloth.h"
#include "Environment.h"
#include "Evaluation.h"
#include "FilmOverlay.h"
#include "FpsCounter.h"
#include "Game.h"
#include "Knot.h"
#include "Pattern.h"
#include "PhotoLab.h"
#include "Scenery.h"
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
    Scenery scenery;
    FilmOverlay film;
    AudioPlayer audio;
    Evaluation evaluation;

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
    float physicsDt = 1.0f / 120.0f;
    bool screenshotRequested = false;
    std::string toast;
    float toastTime = 0.0f;
    float runSeconds = 0.0f;
    int runFrames = 0;
    GameState lastState = GameState::Title;
    bool lastKnotFailed = false;
    Outcome lastOutcome = Outcome::None;
    float headScreenX = 0.0f;
    float headScreenY = 0.0f;

    double lastFrameTime = 0.0;
    double physicsAccumulator = 0.0;
    int substepsLastFrame = 0;
};
