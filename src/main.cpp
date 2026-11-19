#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "App.h"
#include "Clock.h"
#include "GlCheck.h"
#include "Hud.h"

namespace {

AppState g_app;

const char* const kWindowTitle = "Lungi Dynamics";

const double kPhysicsDt = 1.0 / 120.0;
const int kMaxSubsteps = 8;
const double kMaxFrameTime = 0.25;
constexpr Vec3 kGravity(0.0f, -9.81f, 0.0f);

const int kLungiAround = 40;
const int kLungiDown = 30;
const float kLungiFlatRadius = 0.30f;
const float kLungiLength = 0.84f;

const float kStormEaseRate = 0.8f;

void printGlInfo()
{
    std::printf("GL vendor   : %s\n", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    std::printf("GL renderer : %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::printf("GL version  : %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void restartRun()
{
    g_app.cloth.reset();
    g_app.knot.reset();
    g_app.wind.reset();
    g_app.survivalTime = 0.0f;
    g_app.physicsAccumulator = 0.0;
}

void initScene()
{
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    g_app.toonRamp.create();
    const Body& body = g_app.body;
    g_app.cloth.buildTube(TubeShape{body.waistCenter(), body.waistRadiusX(), body.waistRadiusZ(),
                                    kLungiFlatRadius, kLungiLength, kLungiAround, kLungiDown});
    restartRun();
    g_app.lastFrameTime = Clock::nowSeconds();
    glCheck("initScene");
}

void physicsStep(float dt)
{
    g_app.wind.update(dt);
    g_app.cloth.setWaistDrop(g_app.knot.waistDrop());
    g_app.cloth.step(dt, kGravity, g_app.body, g_app.wind);

    g_app.knot.update(dt, g_app.cloth.knotLoad());
    if (g_app.knot.failed()) {
        if (g_app.cloth.knotAttached()) {
            g_app.cloth.releaseKnot();
        }
    } else {
        g_app.survivalTime += dt;
    }
}

// Fixed-timestep loop: physics always advances in equal 1/120 s steps, no
// matter how fast frames are drawn. That keeps the solver stable and makes
// results repeatable for the evaluation runs.
void advanceSimulation(double frameTime)
{
    g_app.substepsLastFrame = 0;
    if (g_app.paused) {
        return;
    }

    g_app.physicsAccumulator += frameTime;
    while (g_app.physicsAccumulator >= kPhysicsDt && g_app.substepsLastFrame < kMaxSubsteps) {
        physicsStep(static_cast<float>(kPhysicsDt));
        g_app.physicsAccumulator -= kPhysicsDt;
        ++g_app.substepsLastFrame;
    }
    if (g_app.substepsLastFrame == kMaxSubsteps) {
        g_app.physicsAccumulator = 0.0;
    }
}

void easeStormTowardWind(float frameTime)
{
    Environment& env = g_app.environment;
    const float target = g_app.wind.enabled() ? g_app.wind.preset().storm : 0.0f;
    const float t = 1.0f - std::exp(-kStormEaseRate * frameTime);
    env.setStorm(env.storm() + (target - env.storm()) * t);
}

void onDisplay()
{
    const double now = Clock::nowSeconds();
    const double frameTime = std::min(now - g_app.lastFrameTime, kMaxFrameTime);
    g_app.lastFrameTime = now;
    g_app.fps.registerFrame(now);

    advanceSimulation(frameTime);
    g_app.camera.update(static_cast<float>(frameTime));
    easeStormTowardWind(static_cast<float>(frameTime));

    Environment& env = g_app.environment;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    env.drawSky();

    g_app.camera.apply(g_app.windowWidth, g_app.windowHeight);
    env.drawSun(g_app.camera.eye());

    env.applyFog();
    glEnable(GL_FOG);
    env.drawGround();
    g_app.body.draw(g_app.toonRamp, env.lightDir(), env.brightness());
    g_app.cloth.draw(g_app.toonRamp, env.lightDir(), env.brightness());
    const Body& body = g_app.body;
    const WaistBand band{body.waistCenter() - Vec3(0.0f, g_app.knot.waistDrop(), 0.0f),
                         body.waistRadiusX(), body.waistRadiusZ()};
    g_app.knot.draw(band, Cloth::outsideColor(), g_app.toonRamp, env.lightDir(), env.brightness());
    g_app.wind.drawStreaks(env.brightness());
    glDisable(GL_FOG);

    Hud::draw(g_app);

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

void onKeyboard(unsigned char key, int, int)
{
    Cloth& cloth = g_app.cloth;
    Wind& wind = g_app.wind;

    switch (key) {
    case 27:
        glutLeaveMainLoop();
        break;
    case '1': case '2': case '3': case '4':
        wind.setPreset(key - '1');
        break;
    case 'f': case 'F':
        glutFullScreenToggle();
        break;
    case 'p': case 'P':
        g_app.paused = !g_app.paused;
        break;
    case 'r': case 'R':
        restartRun();
        break;
    case 'g': case 'G':
        wind.triggerGust();
        break;
    case 'w': case 'W':
        wind.setEnabled(!wind.enabled());
        break;
    case 'd': case 'D':
        g_app.showDebug = !g_app.showDebug;
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

void onSpecialKey(int key, int, int)
{
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

    printGlInfo();
    initScene();

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyboard);
    glutSpecialFunc(onSpecialKey);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);
    glutMouseWheelFunc(onMouseWheel);
    glutIdleFunc(onIdle);

    glutMainLoop();

    std::printf("Exited cleanly.\n");
    return 0;
}
