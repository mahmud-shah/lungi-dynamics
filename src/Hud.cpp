#include "Hud.h"

#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>

#include "App.h"

namespace {

const int kMargin = 14;
const int kLine = 18;
const float kLoadBarMax = 2.0f;

void beginOverlay(int width, int height)
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, width, 0.0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void endOverlay()
{
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void text(int x, int y, const char* s, void* font = GLUT_BITMAP_HELVETICA_12)
{
    glRasterPos2i(x, y);
    glutBitmapString(font, reinterpret_cast<const unsigned char*>(s));
}

int textWidth(const char* s, void* font)
{
    return glutBitmapLength(font, reinterpret_cast<const unsigned char*>(s));
}

void rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawLoadBar(float x, float y, float w, float h, float load)
{
    rect(x, y, w, h, 0.10f, 0.08f, 0.08f, 0.55f);

    const float fill = load < kLoadBarMax ? load / kLoadBarMax : 1.0f;
    float r = 0.35f + 0.6f * fill;
    float g = 0.80f - 0.6f * fill;
    if (load >= 1.0f) {
        r = 0.92f;
        g = 0.22f;
    }
    rect(x + 2, y + 2, (w - 4) * fill, h - 4, r, g, 0.18f, 0.95f);

    const float thresholdX = x + w / kLoadBarMax;
    rect(thresholdX - 1, y - 3, 2, h + 6, 1.0f, 1.0f, 1.0f, 0.95f);
}

void drawMainPanel(const AppState& app)
{
    const int top = app.windowHeight - kMargin - 12;
    char line[128];

    rect(kMargin - 6, top - 5 * kLine - 4, 300, 5 * kLine + 22, 0.98f, 0.95f, 0.88f, 0.55f);
    glColor3f(0.12f, 0.10f, 0.10f);

    const float speed = app.wind.speed();
    std::snprintf(line, sizeof(line), "Wind: %s  %.1f m/s (%.0f km/h)%s",
                  app.wind.preset().name, speed, speed * 3.6f, app.wind.enabled() ? "" : "  [off]");
    text(kMargin, top, line);

    std::snprintf(line, sizeof(line), "Knot: %s   health %.0f%%", app.knot.status(), app.knot.health() * 100.0f);
    text(kMargin, top - kLine, line);

    drawLoadBar(kMargin, top - 2 * kLine - 6, 270, 14, app.knot.load());
    glColor3f(0.12f, 0.10f, 0.10f);
    std::snprintf(line, sizeof(line), "Load %.0f%% of knot strength (%.0f N)",
                  app.knot.load() * 100.0f, app.knot.strength());
    text(kMargin, top - 3 * kLine - 4, line);

    std::snprintf(line, sizeof(line), "Survived: %.1f s   strongest gust %.1f m/s",
                  app.survivalTime, app.wind.strongestGust());
    text(kMargin, top - 4 * kLine - 4, line);
}

void drawDebugPanel(const AppState& app)
{
    const int x = app.windowWidth - 330;
    const int top = app.windowHeight - kMargin - 12;
    const Cloth& cloth = app.cloth;
    const Vec3 w = app.wind.direction() * app.wind.speed();
    char line[128];

    rect(x - 8, top - 6 * kLine - 4, 330, 6 * kLine + 22, 0.05f, 0.05f, 0.08f, 0.55f);
    glColor3f(0.95f, 0.95f, 0.90f);

    std::snprintf(line, sizeof(line), "FPS %.0f (%.2f ms)   substeps %d%s",
                  app.fps.fps(), app.fps.frameTimeMs(), app.substepsLastFrame, app.paused ? "   PAUSED" : "");
    text(x, top, line);
    std::snprintf(line, sizeof(line), "Particles %d   constraints %d", cloth.particleCount(), cloth.constraintCount());
    text(x, top - kLine, line);
    std::snprintf(line, sizeof(line), "Solver iterations %d   dt 1/120 s", cloth.solverIterations());
    text(x, top - 2 * kLine, line);
    std::snprintf(line, sizeof(line), "Wind vector (%.1f, %.1f, %.1f) m/s", w.x, w.y, w.z);
    text(x, top - 3 * kLine, line);
    std::snprintf(line, sizeof(line), "Knot force %.1f N   load %.2f", cloth.knotLoad(), app.knot.load());
    text(x, top - 4 * kLine, line);
    std::snprintf(line, sizeof(line), "Stretch mean %.2f%%  max %.1f%%   recoveries %d",
                  cloth.meanStretch() * 100.0f, cloth.maxStretch() * 100.0f, cloth.recoveries());
    text(x, top - 5 * kLine, line);
}

void drawFailureBanner(const AppState& app)
{
    void* big = GLUT_BITMAP_TIMES_ROMAN_24;
    char line[128];
    std::snprintf(line, sizeof(line), "THE KNOT GAVE UP!  You survived %.1f seconds.", app.survivalTime);
    const char* hint = "Press R to tie the lungi again";

    const int cx = app.windowWidth / 2;
    const int cy = app.windowHeight / 2 + 120;
    const int w = textWidth(line, big) + 40;
    rect(cx - w / 2.0f, cy - 34, w, 70, 0.98f, 0.93f, 0.80f, 0.85f);

    glColor3f(0.70f, 0.12f, 0.10f);
    text(cx - textWidth(line, big) / 2, cy + 6, line, big);
    glColor3f(0.20f, 0.16f, 0.14f);
    text(cx - textWidth(hint, GLUT_BITMAP_HELVETICA_18) / 2, cy - 22, hint, GLUT_BITMAP_HELVETICA_18);
}

} // namespace

namespace Hud {

void draw(const AppState& app)
{
    beginOverlay(app.windowWidth, app.windowHeight);

    drawMainPanel(app);
    if (app.showDebug) {
        drawDebugPanel(app);
    }
    if (app.knot.failed()) {
        drawFailureBanner(app);
    }

    glColor3f(0.12f, 0.10f, 0.10f);
    text(kMargin, kMargin,
         "1-4: wind   G: gust   W: wind on/off   R: retie   P: pause   D: debug   +/-: iterations   Drag/wheel: camera");

    endOverlay();
}

} // namespace Hud
