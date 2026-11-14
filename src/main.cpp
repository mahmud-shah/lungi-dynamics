#include <GL/freeglut.h>
#include <cstdio>

#include "App.h"
#include "Clock.h"
#include "GlCheck.h"

namespace {

AppState g_app;

const char* const kWindowTitle = "Lungi Dynamics";

void printGlInfo()
{
    // Useful in the report and when debugging driver problems: confirms which
    // GPU actually runs the program (laptops can have two).
    std::printf("GL vendor   : %s\n", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    std::printf("GL renderer : %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::printf("GL version  : %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void initGl()
{
    // Warm golden-hour colour: the calm before the Kalboishakhi storm.
    glClearColor(0.96f, 0.78f, 0.52f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glCheck("initGl");
}

void updateWindowTitle()
{
    char title[128];
    std::snprintf(title, sizeof(title), "%s  |  %.0f FPS  (%.2f ms)",
                  kWindowTitle, g_app.fps.fps(), g_app.fps.frameTimeMs());
    glutSetWindowTitle(title);
}

void onDisplay()
{
    g_app.fps.registerFrame(Clock::nowSeconds());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Scene drawing starts in Milestone 1.

    glutSwapBuffers();
    glCheck("onDisplay");

    // Changing the title every frame is slow on Windows, so only on new readings.
    if (g_app.fps.consumeNewReading()) {
        updateWindowTitle();
    }
}

void onReshape(int width, int height)
{
    // A minimised window reports height 0; avoid a zero-size viewport and a
    // division by zero in the projection maths added later.
    if (height <= 0) {
        height = 1;
    }
    g_app.windowWidth = width;
    g_app.windowHeight = height;
    glViewport(0, 0, width, height);
}

void onKeyboard(unsigned char key, int, int)
{
    switch (key) {
    case 27: // Esc
        glutLeaveMainLoop();
        break;
    case 'f':
    case 'F':
        glutFullScreenToggle();
        break;
    default:
        break;
    }
}

void onIdle()
{
    // Ask for a new frame as soon as the previous one is done.
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

    // Make glutMainLoop() return when the window closes, so code after it
    // (saving logs in later milestones) still runs.
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    printGlInfo();
    initGl();

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyboard);
    glutIdleFunc(onIdle);

    glutMainLoop();

    std::printf("Exited cleanly.\n");
    return 0;
}
