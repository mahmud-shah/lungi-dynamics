#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "App.h"
#include "Clock.h"
#include "GlCheck.h"
#include "Hud.h"
#include "Platform.h"

namespace {

AppState g_app;

const char* const kWindowTitle = "Lungi Dynamics";

const int kMaxSubsteps = 8;
const double kMaxFrameTime = 0.25;
constexpr Vec3 kGravity(0.0f, -9.81f, 0.0f);

const int kLungiAround = 40;
const int kLungiDown = 30;
const float kLungiFlatRadius = 0.30f;
const float kLungiLength = 0.84f;

const float kStormEaseRate = 0.8f;
const float kTitleOrbitSpeed = 0.15f;
const float kSlowMotionScale = 0.25f;
const float kBuiltInTileMetres = 0.32f;
const float kPhotoTileMetres = 0.16f;
const float kDefaultPhysicsDt = 1.0f / 120.0f;
const float kToastSeconds = 6.0f;

void printGlInfo()
{
    std::printf("GL vendor   : %s\n", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    std::printf("GL renderer : %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::printf("GL version  : %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void selectPattern(int index)
{
    const int count = static_cast<int>(g_app.patterns.size());
    g_app.patternIndex = (index % count + count) % count;
    g_app.fabric.upload(g_app.patterns[g_app.patternIndex]);
    g_app.cloth.setPatternTileMetres(kBuiltInTileMetres);
}

void openPhotoDialog()
{
    std::string path;
    if (Platform::chooseImageFile(path)) {
        g_app.photoLab.loadPhoto(path);
    }
}

void enterPhotoLab()
{
    g_app.inPhotoLab = true;
    if (!g_app.photoLab.hasPhoto()) {
        openPhotoDialog();
    }
}

// The extracted tile replaces the lungi texture. A photo tile holds one real
// repeat of the cloth, which is smaller than the built-in tiles, so it is
// mapped onto a smaller piece of the lungi.
void wearPhotoFabric()
{
    const FabricAnalysis& a = g_app.photoLab.analysis();
    g_app.fabric.uploadTexels(a.tile, a.tileSize, a.mainColor);
    g_app.cloth.setPatternTileMetres(kPhotoTileMetres);
    g_app.patternIndex = -1;
    g_app.inPhotoLab = false;
}

void resetWorld(unsigned int stormSeed)
{
    g_app.cloth.reset();
    g_app.knot.reset();
    g_app.wind.reset(stormSeed);
    g_app.physicsAccumulator = 0.0;
}

void showToast(const std::string& text)
{
    g_app.toast = text;
    g_app.toastTime = kToastSeconds;
}

void showTitle()
{
    g_app.game.showTitle();
    g_app.physicsDt = kDefaultPhysicsDt;
    g_app.sandbox = false;
    g_app.paused = false;
    g_app.timeScale = 1.0f;
    g_app.wind.setPreset(0);
    resetWorld(20261125u);
}

void startRun()
{
    g_app.game.startRun();
    g_app.camera.faceFront();
    g_app.sandbox = false;
    g_app.paused = false;
    g_app.timeScale = 1.0f;
    g_app.wind.setPreset(0);
    resetWorld(20261125u + static_cast<unsigned int>(g_app.game.runNumber()));
}

void initScene(const char* startupPhoto)
{
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    g_app.toonRamp.create();
    g_app.patterns = Patterns::builtIn();
    g_app.fabric.create();
    g_app.photoLab.create();
    g_app.film.create();
    g_app.scenery.build(g_app.environment.lightDir());
    g_app.audio.init();

    const Body& body = g_app.body;
    g_app.cloth.buildTube(TubeShape{body.waistCenter(), body.waistRadiusX(), body.waistRadiusZ(),
                                    kLungiFlatRadius, kLungiLength, kLungiAround, kLungiDown});
    selectPattern(g_app.patternIndex);
    showTitle();
    if (startupPhoto) {
        g_app.photoLab.loadPhoto(startupPhoto);
        g_app.inPhotoLab = true;
    }
    g_app.lastFrameTime = Clock::nowSeconds();
    glCheck("initScene");
}

void physicsStep(float dt)
{
    Game& game = g_app.game;
    const bool playing = game.state() == GameState::Playing;

    if (playing && !g_app.sandbox) {
        g_app.wind.setPreset(game.stormStage());
    }
    g_app.wind.update(dt);
    if (g_app.wind.consumeGustStart() && g_app.wind.speed() > 4.0f) {
        g_app.audio.play(Sound::Gust);
    }

    g_app.knot.setStrength(game.difficulty().knotStrength * game.knotStrengthMultiplier());
    g_app.cloth.setWaistDrop(g_app.knot.waistDrop());
    g_app.cloth.step(dt, kGravity, g_app.body, g_app.wind);

    if (game.state() == GameState::Title) {
        g_app.body.setPose(ArmPose::Down, Face::Calm);
        return;
    }
    g_app.knot.update(dt, g_app.cloth.knotLoad());
    if (g_app.knot.failed() && g_app.cloth.knotAttached()) {
        g_app.cloth.releaseKnot();
    }
    const GameInput input{g_app.knot.failed(), g_app.wind.speed(), g_app.wind.updraftActive(),
                          g_app.knot.waistDrop(), g_app.knot.load(), g_app.cloth.centroid().y,
                          g_app.gripKeyHeld};
    game.update(dt, input);
    g_app.body.setPose(game.armPose(), game.face());

    if (game.hold().justSlipped()) {
        g_app.audio.play(Sound::Slip);
    }
    if (g_app.knot.failed() && !g_app.lastKnotFailed) {
        g_app.audio.play(Sound::KnotPop);
    }
    g_app.lastKnotFailed = g_app.knot.failed();
    if (game.outcome() == Outcome::FlewUp && g_app.lastOutcome != Outcome::FlewUp) {
        g_app.audio.play(Sound::FlyUp);
    }
    g_app.lastOutcome = game.outcome();
}

// Fixed-timestep loop: physics always advances in equal steps (1/120 s by
// default), no matter how fast frames are drawn. Slow motion only feeds less
// real time into the accumulator, so the physics itself is unchanged.
void advanceSimulation(double frameTime)
{
    g_app.substepsLastFrame = 0;
    if (g_app.paused) {
        return;
    }

    g_app.physicsAccumulator += frameTime * g_app.timeScale;
    const double dt = g_app.physicsDt;
    while (g_app.physicsAccumulator >= dt && g_app.substepsLastFrame < kMaxSubsteps) {
        physicsStep(g_app.physicsDt);
        g_app.physicsAccumulator -= dt;
        ++g_app.substepsLastFrame;
    }
    if (g_app.substepsLastFrame == kMaxSubsteps) {
        g_app.physicsAccumulator = 0.0;
    }
}

// The speech bubble is 2D, so the 3D point just above the head is projected
// to window coordinates with the current camera matrices.
void projectHeadToScreen()
{
    GLdouble model[16];
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLdouble x = 0.0;
    GLdouble y = 0.0;
    GLdouble z = 0.0;
    gluProject(0.0, 1.95, 0.0, model, projection, viewport, &x, &y, &z);
    g_app.headScreenX = static_cast<float>(x);
    g_app.headScreenY = static_cast<float>(y);
}

void easeStormTowardWind(float frameTime)
{
    Environment& env = g_app.environment;
    const float target = g_app.wind.enabled() ? g_app.wind.preset().storm : 0.0f;
    const float t = 1.0f - std::exp(-kStormEaseRate * frameTime);
    env.setStorm(env.storm() + (target - env.storm()) * t);
}

void applyEvaluationConfig()
{
    const EvalConfig& c = g_app.evaluation.config();
    g_app.physicsDt = c.timeStep;
    g_app.wind.setPreset(c.windPreset);
    resetWorld(20261125u);
}

void startEvaluation()
{
    g_app.evaluation.start();
    g_app.sandbox = true;
    applyEvaluationConfig();
}

void updateEvaluation(double frameTime, double physicsMs)
{
    Evaluation& eval = g_app.evaluation;
    if (!eval.running()) {
        return;
    }
    const Cloth& cloth = g_app.cloth;
    const FrameSample sample{static_cast<float>(frameTime), static_cast<float>(physicsMs), cloth.maxStretch(),
                             cloth.meanStretch(), cloth.knotLoad(), cloth.recoveries()};
    if (eval.update(sample, cloth.iterationsFor(g_app.physicsDt))) {
        applyEvaluationConfig();
    } else if (!eval.running()) {
        showToast("Evaluation finished. Saved " + eval.savedPath());
        showTitle();
    }
}

// One line per finished game in logs/runs.csv, for the report's results table.
void logFinishedRun()
{
    const Game& game = g_app.game;
    const char* lungi = g_app.patternIndex >= 0 ? g_app.patterns[g_app.patternIndex].name : "Your photo";
    const char* outcome = game.outcome() == Outcome::FlewUp ? "flew_up" : "fell_down";
    char line[256];
    std::snprintf(line, sizeof(line), "%s,%s,%s,%.1f,%.1f,%s,%d,%s,%.0f\n", timestampString().c_str(),
                  game.difficulty().name, lungi, game.survivalTime(), game.strongestGustSurvived(), outcome,
                  game.score(), game.tier().name, g_app.runFrames / std::max(0.001f, g_app.runSeconds));
    if (!appendRunLog(line)) {
        showToast("Could not write logs/runs.csv");
    }
}

void trackRunState(double frameTime)
{
    const GameState state = g_app.game.state();
    if (state == GameState::Playing) {
        if (g_app.lastState != GameState::Playing) {
            g_app.runSeconds = 0.0f;
            g_app.runFrames = 0;
        }
        g_app.runSeconds += static_cast<float>(frameTime);
        ++g_app.runFrames;
    }
    if (state == GameState::Results && g_app.lastState == GameState::Playing) {
        logFinishedRun();
    }
    g_app.lastState = state;
}

void updateSound()
{
    const float wind = g_app.wind.speed();
    const float windLevel = wind > 0.0f ? std::min(1.0f, 0.12f + wind / 20.0f) : 0.0f;
    const float storm = g_app.environment.storm();
    const float rainLevel = std::max(0.0f, std::min(1.0f, (storm - 0.45f) / 0.55f)) * 0.8f;
    g_app.audio.setLoopLevels(windLevel, rainLevel);
    if (g_app.scenery.consumeThunder()) {
        g_app.audio.play(Sound::Thunder);
    }
}

void onDisplay()
{
    const double now = Clock::nowSeconds();
    const double frameTime = std::min(now - g_app.lastFrameTime, kMaxFrameTime);
    g_app.lastFrameTime = now;
    g_app.fps.registerFrame(now);

    if (g_app.inPhotoLab) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_app.photoLab.draw(g_app.windowWidth, g_app.windowHeight);
        if (g_app.screenshotRequested) {
            g_app.screenshotRequested = false;
            saveScreenshot(g_app.windowWidth, g_app.windowHeight);
        }
        glutSwapBuffers();
        glCheck("photoLab");
        return;
    }

    const double physicsStart = Clock::nowSeconds();
    advanceSimulation(frameTime);
    const double physicsMs = 1000.0 * (Clock::nowSeconds() - physicsStart);
    updateEvaluation(frameTime, physicsMs);
    trackRunState(frameTime);
    g_app.toastTime = std::max(0.0f, g_app.toastTime - static_cast<float>(frameTime));

    const float visualDt = g_app.paused ? 0.0f : static_cast<float>(frameTime) * g_app.timeScale;
    g_app.scenery.update(visualDt, g_app.wind, g_app.environment.storm());
    updateSound();

    if (g_app.game.state() == GameState::Title) {
        g_app.camera.rotate(kTitleOrbitSpeed * static_cast<float>(frameTime), 0.0f);
    }
    g_app.camera.update(static_cast<float>(frameTime));
    easeStormTowardWind(static_cast<float>(frameTime));

    Environment& env = g_app.environment;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    env.setFlash(g_app.scenery.flash());
    env.drawSky();

    const bool lungiLeaving = g_app.game.state() == GameState::Playing && g_app.knot.failed();
    const float lungiHeight = g_app.cloth.centroid().y;
    g_app.camera.setTargetHeight(lungiLeaving ? std::max(0.9f, std::min(lungiHeight, 1.8f)) : 0.9f);

    g_app.camera.apply(g_app.windowWidth, g_app.windowHeight);
    projectHeadToScreen();
    env.drawSun(g_app.camera.eye());
    g_app.scenery.drawClouds(g_app.camera.eye(), env.storm(), g_app.scenery.flash());

    env.applyFog();
    glEnable(GL_FOG);
    const float brightness = env.brightness();
    g_app.scenery.drawGround(brightness);
    g_app.scenery.drawProps(g_app.toonRamp, env.lightDir(), brightness);
    g_app.scenery.drawGrass(brightness);
    g_app.body.draw(g_app.toonRamp, env.lightDir(), env.brightness());
    g_app.cloth.draw(g_app.toonRamp, g_app.fabric, env.lightDir(), env.brightness());
    const Body& body = g_app.body;
    const WaistBand band{body.waistCenter() - Vec3(0.0f, g_app.knot.waistDrop(), 0.0f),
                         body.waistRadiusX(), body.waistRadiusZ()};
    g_app.knot.draw(band, g_app.fabric.mainColor(), g_app.toonRamp, env.lightDir(), env.brightness());
    g_app.scenery.drawDebris(brightness);
    g_app.wind.drawStreaks(brightness);
    g_app.scenery.drawRain(brightness);
    g_app.scenery.drawLightningBolt();
    glDisable(GL_FOG);

    g_app.film.draw(g_app.windowWidth, g_app.windowHeight, env.storm(), g_app.scenery.flash());
    Hud::draw(g_app);

    if (g_app.screenshotRequested) {
        g_app.screenshotRequested = false;
        const std::string path = saveScreenshot(g_app.windowWidth, g_app.windowHeight);
        showToast(path.empty() ? "Could not save the screenshot" : "Saved " + path);
    }
    glutSwapBuffers();
    glCheck("onDisplay");

    if (g_app.fps.consumeNewReading()) {
        char title[128];
        std::snprintf(title, sizeof(title), "%s  |  %.0f FPS", kWindowTitle, g_app.fps.fps());
        glutSetWindowTitle(title);
    }
}

void onReshape(int width, int height)
{
    g_app.windowWidth = std::max(width, 1);
    g_app.windowHeight = std::max(height, 1);
    glViewport(0, 0, g_app.windowWidth, g_app.windowHeight);
}

void handlePhotoLabKey(unsigned char key)
{
    PhotoLab& lab = g_app.photoLab;
    switch (key) {
    case 27:
        g_app.inPhotoLab = false;
        break;
    case 'o': case 'O':
        openPhotoDialog();
        break;
    case 'k': case 'K':
        lab.cycleColorCount();
        break;
    case 'a': case 'A':
        lab.resetToAuto();
        break;
    case 13:
        if (lab.canApply()) {
            wearPhotoFabric();
        }
        break;
    default:
        break;
    }
}

void handleTitleKey(unsigned char key)
{
    switch (key) {
    case 13:
    case ' ':
        startRun();
        break;
    case 'o': case 'O':
        enterPhotoLab();
        break;
    case 'e': case 'E':
        startEvaluation();
        break;
    case 27:
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
}

void handleResultsKey(unsigned char key)
{
    switch (key) {
    case 13:
    case 'r': case 'R':
        startRun();
        break;
    case 27:
    case 't': case 'T':
        showTitle();
        break;
    default:
        break;
    }
}

void handlePlayKey(unsigned char key)
{
    Cloth& cloth = g_app.cloth;
    Wind& wind = g_app.wind;

    switch (key) {
    case 27:
    case 't': case 'T':
        showTitle();
        break;
    case ' ':
        g_app.gripKeyHeld = true;
        break;
    case '1': case '2': case '3': case '4':
        g_app.sandbox = true;
        wind.setPreset(key - '1');
        break;
    case 'p': case 'P':
        g_app.paused = !g_app.paused;
        break;
    case 'r': case 'R':
        startRun();
        break;
    case 's': case 'S':
        g_app.timeScale = g_app.timeScale < 1.0f ? 1.0f : kSlowMotionScale;
        break;
    case 'g': case 'G':
        wind.triggerGust();
        break;
    case 'u': case 'U':
        wind.triggerUpdraft();
        break;
    case 'w': case 'W':
        wind.setEnabled(!wind.enabled());
        break;
    case '+': case '=':
        cloth.setSolverIterations(cloth.solverIterations() + 1);
        break;
    case '-': case '_':
        cloth.setSolverIterations(cloth.solverIterations() - 1);
        break;
    default:
        break;
    }
}

void onKeyboard(unsigned char key, int, int)
{
    if (key == 'm' || key == 'M') {
        g_app.audio.toggleMute();
        showToast(g_app.audio.muted() ? "Sound off (M)" : "Sound on (M)");
        return;
    }
    if (g_app.inPhotoLab) {
        handlePhotoLabKey(key);
        return;
    }
    if (g_app.evaluation.running()) {
        if (key == 27) {
            g_app.evaluation.stop();
            showToast("Evaluation cancelled");
            showTitle();
        } else if (key == 'd' || key == 'D') {
            g_app.showDebug = !g_app.showDebug;
        }
        return;
    }
    switch (key) {
    case 'f': case 'F':
        glutFullScreenToggle();
        return;
    case 'd': case 'D':
        g_app.showDebug = !g_app.showDebug;
        return;
    case 'l': case 'L':
        selectPattern(g_app.patternIndex + 1);
        return;
    default:
        break;
    }

    switch (g_app.game.state()) {
    case GameState::Title:   handleTitleKey(key); break;
    case GameState::Playing: handlePlayKey(key); break;
    case GameState::Results: handleResultsKey(key); break;
    }
}

void onKeyboardUp(unsigned char key, int, int)
{
    if (key == ' ') {
        g_app.gripKeyHeld = false;
    }
}

void onSpecialKey(int key, int, int)
{
    if (key == GLUT_KEY_F12) {
        g_app.screenshotRequested = true;
        return;
    }
    if (g_app.evaluation.running()) {
        return;
    }
    if (g_app.inPhotoLab) {
        switch (key) {
        case GLUT_KEY_LEFT:  g_app.photoLab.adjustPeriod(-1.0f, 0.0f); break;
        case GLUT_KEY_RIGHT: g_app.photoLab.adjustPeriod(1.0f, 0.0f); break;
        case GLUT_KEY_DOWN:  g_app.photoLab.adjustPeriod(0.0f, -1.0f); break;
        case GLUT_KEY_UP:    g_app.photoLab.adjustPeriod(0.0f, 1.0f); break;
        default: break;
        }
        return;
    }
    if (g_app.game.state() == GameState::Title) {
        switch (key) {
        case GLUT_KEY_LEFT:  selectPattern(g_app.patternIndex - 1); break;
        case GLUT_KEY_RIGHT: selectPattern(g_app.patternIndex + 1); break;
        case GLUT_KEY_UP:    g_app.game.cycleDifficulty(-1); break;
        case GLUT_KEY_DOWN:  g_app.game.cycleDifficulty(1); break;
        default: break;
        }
        return;
    }

    const float step = 0.08f;
    switch (key) {
    case GLUT_KEY_LEFT:  g_app.camera.rotate(step, 0.0f); break;
    case GLUT_KEY_RIGHT: g_app.camera.rotate(-step, 0.0f); break;
    case GLUT_KEY_UP:    g_app.camera.rotate(0.0f, step); break;
    case GLUT_KEY_DOWN:  g_app.camera.rotate(0.0f, -step); break;
    default: break;
    }
}

void onMouse(int button, int state, int x, int y)
{
    if (g_app.inPhotoLab) {
        if (button == GLUT_LEFT_BUTTON) {
            if (state == GLUT_DOWN) {
                g_app.photoLab.mouseDown(x, y);
            } else {
                g_app.photoLab.mouseUp(x, y);
            }
        }
        return;
    }
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            g_app.camera.beginDrag(x, y);
        } else {
            g_app.camera.endDrag();
        }
    }
}

void onMotion(int x, int y)
{
    if (g_app.inPhotoLab) {
        g_app.photoLab.mouseDrag(x, y);
        return;
    }
    g_app.camera.dragTo(x, y);
}

void onMouseWheel(int, int direction, int, int)
{
    g_app.camera.zoom(direction);
}

void onIdle()
{
    glutPostRedisplay();
}

} // namespace

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(g_app.windowWidth, g_app.windowHeight);
    glutInitWindowPosition(100, 60);
    glutCreateWindow(kWindowTitle);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutIgnoreKeyRepeat(1);

    printGlInfo();
    initScene(argc > 1 ? argv[1] : nullptr);

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    glutSpecialFunc(onSpecialKey);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);
    glutMouseWheelFunc(onMouseWheel);
    glutIdleFunc(onIdle);

    glutMainLoop();

    g_app.audio.shutdown();
    std::printf("Exited cleanly.\n");
    return 0;
}
