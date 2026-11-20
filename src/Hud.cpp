#include "Hud.h"

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>

#include "App.h"

namespace {

const int kMargin = 14;
const int kLine = 18;
const float kLoadBarMax = 2.0f;

void* const kSmall = GLUT_BITMAP_HELVETICA_12;
void* const kMedium = GLUT_BITMAP_HELVETICA_18;
void* const kLarge = GLUT_BITMAP_TIMES_ROMAN_24;

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

int textWidth(const char* s, void* font)
{
    return glutBitmapLength(font, reinterpret_cast<const unsigned char*>(s));
}

void text(int x, int y, const char* s, void* font = kSmall)
{
    glRasterPos2i(x, y);
    glutBitmapString(font, reinterpret_cast<const unsigned char*>(s));
}

void centeredText(int cx, int y, const char* s, void* font)
{
    text(cx - textWidth(s, font) / 2, y, s, font);
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

void drawBar(float x, float y, float w, float h, float fill, float r, float g, float b)
{
    fill = fill < 0.0f ? 0.0f : (fill > 1.0f ? 1.0f : fill);
    rect(x, y, w, h, 0.10f, 0.08f, 0.08f, 0.55f);
    rect(x + 2, y + 2, (w - 4) * fill, h - 4, r, g, b, 0.95f);
}

void drawLoadBar(float x, float y, float w, float h, float load)
{
    const float fill = load < kLoadBarMax ? load / kLoadBarMax : 1.0f;
    float r = 0.35f + 0.6f * fill;
    float g = 0.80f - 0.6f * fill;
    if (load >= 1.0f) {
        r = 0.92f;
        g = 0.22f;
    }
    drawBar(x, y, w, h, fill, r, g, 0.18f);
    rect(x + w / kLoadBarMax - 1, y - 3, 2, h + 6, 1.0f, 1.0f, 1.0f, 0.95f);
}

void drawPlayPanel(const AppState& app)
{
    const Game& game = app.game;
    const int top = app.windowHeight - kMargin - 12;
    char line[128];

    rect(kMargin - 6, top - 7 * kLine - 4, 330, 7 * kLine + 22, 0.98f, 0.95f, 0.88f, 0.55f);
    glColor3f(0.12f, 0.10f, 0.10f);

    const float speed = app.wind.speed();
    std::snprintf(line, sizeof(line), "Wind: %s  %.1f m/s (%.0f km/h)%s%s",
                  app.wind.preset().name, speed, speed * 3.6f, app.wind.updraftActive() ? "  UPDRAFT!" : "",
                  app.sandbox ? "  [sandbox]" : "");
    text(kMargin, top, line);

    std::snprintf(line, sizeof(line), "Knot: %s   health %.0f%%", app.knot.status(), app.knot.health() * 100.0f);
    text(kMargin, top - kLine, line);

    drawLoadBar(kMargin, top - 2 * kLine - 6, 270, 14, app.knot.load());
    glColor3f(0.12f, 0.10f, 0.10f);
    std::snprintf(line, sizeof(line), "Load %.0f%% of knot strength (%.0f N)",
                  app.knot.load() * 100.0f, app.knot.strength());
    text(kMargin, top - 3 * kLine - 4, line);

    const Hold& hold = game.hold();
    drawBar(kMargin, top - 4 * kLine - 10, 270, 12, hold.stamina(), 0.30f, 0.55f, 0.90f);
    glColor3f(0.12f, 0.10f, 0.10f);
    const char* holdText = "hold SPACE to grab the knot";
    switch (hold.state()) {
    case HoldState::Player:  holdText = "HOLDING ON WITH BOTH HANDS!"; break;
    case HoldState::Reflex:  holdText = "grabbed the knot by reflex"; break;
    case HoldState::Slipped: holdText = "HANDS SLIPPED!"; break;
    default: break;
    }
    std::snprintf(line, sizeof(line), "Grip: %s   (hold power %.0f%%)", holdText, hold.effectiveness() * 100.0f);
    if (hold.state() == HoldState::Slipped) {
        glColor3f(0.75f, 0.12f, 0.10f);
    }
    text(kMargin, top - 5 * kLine - 8, line);
    glColor3f(0.12f, 0.10f, 0.10f);

    std::snprintf(line, sizeof(line), "Survived: %.1f s   score %d   (%s)",
                  game.survivalTime(), game.score(), game.difficulty().name);
    text(kMargin, top - 6 * kLine - 8, line);
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

    std::snprintf(line, sizeof(line), "FPS %.0f (%.2f ms)   substeps %d%s%s",
                  app.fps.fps(), app.fps.frameTimeMs(), app.substepsLastFrame,
                  app.paused ? "   PAUSED" : "", app.timeScale < 1.0f ? "   SLOW-MO" : "");
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

void drawAnnouncement(const AppState& app)
{
    const float alpha = app.game.announcementAlpha();
    if (alpha <= 0.0f) {
        return;
    }
    const char* s = app.game.announcement();
    const int cx = app.windowWidth / 2;
    const int y = app.windowHeight - 90;
    const int w = textWidth(s, kLarge) + 40;
    rect(cx - w / 2.0f, y - 14, w, 44, 0.12f, 0.10f, 0.16f, 0.6f * alpha);
    glColor4f(1.0f, 0.88f, 0.55f, alpha);
    centeredText(cx, y, s, kLarge);
}

// A speech bubble beside the character's head, with a small tail pointing
// at the projected head position.
void drawSpeech(const AppState& app)
{
    const Speech& speech = app.game.speech();
    const float alpha = speech.alpha();
    if (alpha <= 0.0f) {
        return;
    }
    const char* s = speech.text();
    const float w = textWidth(s, kMedium) + 24.0f;
    const float h = 34.0f;
    float x = app.headScreenX + 40.0f;
    if (x + w > app.windowWidth - 10) {
        x = app.headScreenX - 40.0f - w;
    }
    const float y = app.headScreenY + 10.0f;

    rect(x, y, w, h, 1.0f, 1.0f, 1.0f, 0.92f * alpha);
    glColor4f(1.0f, 1.0f, 1.0f, 0.92f * alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + (x > app.headScreenX ? 6.0f : w - 6.0f), y + 6.0f);
    glVertex2f(x + (x > app.headScreenX ? 22.0f : w - 22.0f), y);
    glVertex2f(app.headScreenX, app.headScreenY - 6.0f);
    glEnd();
    glColor4f(0.15f, 0.12f, 0.10f, alpha);
    text(static_cast<int>(x + 12), static_cast<int>(y + 11), s, kMedium);
}

void drawShockBubble(const AppState& app)
{
    if (app.game.face() != Face::Shock) {
        return;
    }
    const float cx = app.headScreenX;
    const float cy = app.headScreenY + 70.0f;
    glColor4f(1.0f, 0.95f, 0.30f, 0.95f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 16; ++i) {
        const float a = 6.2831853f * i / 16;
        const float r = (i % 2 == 0) ? 34.0f : 24.0f;
        glVertex2f(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    glEnd();
    glColor3f(0.80f, 0.10f, 0.08f);
    centeredText(static_cast<int>(cx), static_cast<int>(cy - 8), "!!", kLarge);
}

void drawFailureBanner(const AppState& app)
{
    const int cx = app.windowWidth / 2;
    const int cy = 110;
    const char* s = "!!  THE KNOT GAVE UP  !!";
    const int w = textWidth(s, kLarge) + 50;
    rect(cx - w / 2.0f, cy - 16, w, 48, 0.98f, 0.93f, 0.80f, 0.9f);
    glColor3f(0.70f, 0.12f, 0.10f);
    centeredText(cx, cy, s, kLarge);
}

int leftPanelCenter(const AppState& app)
{
    const int cx = app.windowWidth / 4;
    return cx < 310 ? 310 : cx;
}

void drawTitle(const AppState& app)
{
    const int cx = leftPanelCenter(app);
    const int top = app.windowHeight - 110;
    char line[128];

    rect(cx - 280, top - 250, 560, 300, 0.98f, 0.94f, 0.85f, 0.82f);

    glColor3f(0.55f, 0.16f, 0.12f);
    centeredText(cx, top, "LUNGI DYNAMICS", kLarge);
    glColor3f(0.25f, 0.20f, 0.18f);
    centeredText(cx, top - 28, "A Kalboishakhi Survival Simulator", kMedium);

    std::snprintf(line, sizeof(line), "Difficulty:  <  %s  >      (Up / Down)", app.game.difficulty().name);
    centeredText(cx, top - 80, line, kMedium);
    const char* lungiName = app.patternIndex >= 0 ? app.patterns[app.patternIndex].name : "Your photo";
    std::snprintf(line, sizeof(line), "Lungi:  <  %s  >      (Left / Right)", lungiName);
    centeredText(cx, top - 110, line, kMedium);

    glColor3f(0.55f, 0.16f, 0.12f);
    centeredText(cx, top - 160, "Press ENTER to face the storm", kLarge);
    glColor3f(0.25f, 0.20f, 0.18f);
    centeredText(cx, top - 195, "In the storm, hold SPACE to grab the knot. Grip runs out, so save it for the gusts!", kSmall);
    centeredText(cx, top - 215, "O: make a lungi from your own fabric photo      Esc: quit", kSmall);
}

void drawResults(const AppState& app)
{
    const Game& game = app.game;
    const int cx = leftPanelCenter(app);
    const int top = app.windowHeight - 120;
    char line[160];

    rect(cx - 280, top - 300, 560, 350, 0.98f, 0.94f, 0.85f, 0.88f);

    glColor3f(0.70f, 0.12f, 0.10f);
    centeredText(cx, top, "!!  THE KNOT GAVE UP  !!", kLarge);
    glColor3f(0.40f, 0.20f, 0.10f);
    centeredText(cx, top - 24, game.outcomeText(), kMedium);

    glColor3f(0.20f, 0.16f, 0.14f);
    std::snprintf(line, sizeof(line), "Survived: %.1f s", game.survivalTime());
    centeredText(cx, top - 52, line, kMedium);
    std::snprintf(line, sizeof(line), "Strongest gust survived: %.1f m/s (%.0f km/h)",
                  game.strongestGustSurvived(), game.strongestGustSurvived() * 3.6f);
    centeredText(cx, top - 76, line, kMedium);
    std::snprintf(line, sizeof(line), "Lungi displacement penalty: -%.0f", game.displacementPenalty());
    centeredText(cx, top - 100, line, kMedium);
    std::snprintf(line, sizeof(line), "Difficulty: %s (x%.1f)", game.difficulty().name, game.difficulty().scoreMultiplier);
    centeredText(cx, top - 124, line, kMedium);

    glColor3f(0.12f, 0.30f, 0.20f);
    std::snprintf(line, sizeof(line), "Lungi Survival Score: %d", game.score());
    centeredText(cx, top - 165, line, kLarge);
    glColor3f(0.55f, 0.16f, 0.12f);
    centeredText(cx, top - 200, game.tier().name, kLarge);
    glColor3f(0.25f, 0.20f, 0.18f);
    std::snprintf(line, sizeof(line), "\"%s\"", game.tierMessage());
    centeredText(cx, top - 230, line, kMedium);

    centeredText(cx, top - 280, "ENTER: play again      T: title screen", kSmall);
}

} // namespace

namespace Hud {

void draw(const AppState& app)
{
    beginOverlay(app.windowWidth, app.windowHeight);

    const GameState state = app.game.state();
    if (state == GameState::Title) {
        drawTitle(app);
    } else if (state == GameState::Playing) {
        drawPlayPanel(app);
        drawAnnouncement(app);
        drawShockBubble(app);
        drawSpeech(app);
        if (app.knot.failed()) {
            drawFailureBanner(app);
        }
        glColor3f(0.12f, 0.10f, 0.10f);
        text(kMargin, kMargin,
             "SPACE: grip   L: lungi   S: slow-mo   P: pause   R: restart   T: title   1-4: sandbox wind   G/U: gust/updraft   D: debug   Drag/wheel: camera");
    } else {
        drawResults(app);
    }

    if (app.showDebug) {
        drawDebugPanel(app);
    }

    endOverlay();
}

} // namespace Hud
