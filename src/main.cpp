#include <GL/freeglut.h>
#include <algorithm>
#include <cstdio>

#include "App.h"
#include "Clock.h"
#include "GlCheck.h"

namespace {

AppState g_app;

const char* const kWindowTitle = "Lungi Dynamics";

const double kPhysicsDt = 1.0 / 120.0;
const int kMaxSubsteps = 8;
const double kMaxFrameTime = 0.25;
const Vec3 kGravity(0.0f, -9.81f, 0.0f);

const Vec3 kLineLeft(0.2f, 2.4f, 0.0f);
const Vec3 kLineRight(2.4f, 2.4f, 0.0f);

void printGlInfo()
{
    std::printf("GL vendor   : %s\n", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    std::printf("GL renderer : %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::printf("GL version  : %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void initScene()
{
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    g_app.toonRamp.create();
    g_app.cloth.buildHangingSheet(kLineLeft, kLineRight.x - kLineLeft.x, 1.6f, 24, 18);
    g_app.lastFrameTime = Clock::nowSeconds();
    glCheck("initScene");
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
        g_app.cloth.step(static_cast<float>(kPhysicsDt), kGravity);
        g_app.physicsAccumulator -= kPhysicsDt;
        ++g_app.substepsLastFrame;
    }
    if (g_app.substepsLastFrame == kMaxSubsteps) {
        g_app.physicsAccumulator = 0.0;
    }
}

void drawText(int x, int y, const char* text)
{
    glRasterPos2i(x, y);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, reinterpret_cast<const unsigned char*>(text));
}

void drawDebugText()
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, g_app.windowWidth, 0.0, g_app.windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const Cloth& cloth = g_app.cloth;
    char line[160];
    const int top = g_app.windowHeight - 20;

    glColor3f(0.12f, 0.10f, 0.10f);
    std::snprintf(line, sizeof(line), "FPS %.0f   substeps %d   %s",
                  g_app.fps.fps(), g_app.substepsLastFrame, g_app.paused ? "PAUSED" : "");
    drawText(12, top, line);
    std::snprintf(line, sizeof(line), "Particles %d   constraints %d   iterations %d",
                  cloth.particleCount(), cloth.constraintCount(), cloth.solverIterations());
    drawText(12, top - 16, line);
    std::snprintf(line, sizeof(line), "Max stretch %.2f%%   recoveries %d   storm %.2f",
                  cloth.maxStretch() * 100.0f, cloth.recoveries(), g_app.environment.storm());
    drawText(12, top - 32, line);
    drawText(12, 14, "Drag/arrows: orbit   Wheel: zoom   G: gust   R: reset   P: pause   +/-: iterations   [ ]: storm");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void onDisplay()
{
    const double now = Clock::nowSeconds();
    const double frameTime = std::min(now - g_app.lastFrameTime, kMaxFrameTime);
    g_app.lastFrameTime = now;
    g_app.fps.registerFrame(now);

    advanceSimulation(frameTime);
    g_app.camera.update(static_cast<float>(frameTime));

    Environment& env = g_app.environment;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    env.drawSky();

    g_app.camera.apply(g_app.windowWidth, g_app.windowHeight);
    env.drawSun(g_app.camera.eye());

    env.applyFog();
    glEnable(GL_FOG);
    env.drawGround();
    env.drawClothesLine(kLineLeft, kLineRight);
    g_app.sphere.draw(g_app.toonRamp, env.lightDir(), env.brightness());
    g_app.cloth.draw(env.brightness());
    glDisable(GL_FOG);

    drawDebugText();

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
    Environment& env = g_app.environment;
    Cloth& cloth = g_app.cloth;

    switch (key) {
    case 27:
        glutLeaveMainLoop();
        break;
    case 'f': case 'F':
        glutFullScreenToggle();
        break;
    case 'p': case 'P':
        g_app.paused = !g_app.paused;
        break;
    case 'r': case 'R':
        cloth.reset();
        break;
    case 'g': case 'G':
        cloth.applyGust(Vec3(0.6f, 0.8f, -4.0f));
        break;
    case '+': case '=':
        cloth.setSolverIterations(cloth.solverIterations() + 2);
        break;
    case '-': case '_':
        cloth.setSolverIterations(cloth.solverIterations() - 2);
        break;
    case '[':
        env.setStorm(env.storm() - 0.1f);
        break;
    case ']':
        env.setStorm(env.storm() + 0.1f);
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
