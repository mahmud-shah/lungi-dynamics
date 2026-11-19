#include "Environment.h"

#include <GL/freeglut.h>
#include <cmath>

namespace {

const int kGroundTiles = 24;
const float kTileSize = 2.0f;

const Vec3 kGrassGreen(0.45f, 0.58f, 0.28f);
const Vec3 kRiceGold(0.66f, 0.64f, 0.33f);
const Vec3 kBambooBrown(0.42f, 0.30f, 0.18f);

float hash2(int i, int j)
{
    const float s = std::sin(i * 12.9898f + j * 78.233f) * 43758.5453f;
    return s - std::floor(s);
}

} // namespace

Environment::Environment()
    : lightDir_(normalized(Vec3(-0.75f, 0.40f, 0.35f)))
{
    groundTileColors_.reserve(kGroundTiles * kGroundTiles);
    for (int i = 0; i < kGroundTiles; ++i) {
        for (int j = 0; j < kGroundTiles; ++j) {
            groundTileColors_.push_back(lerp(kGrassGreen, kRiceGold, hash2(i, j) * 0.6f));
        }
    }
}

void Environment::setStorm(float storm)
{
    storm_ = clamp01(storm);
}

float Environment::brightness() const
{
    return 1.0f - 0.45f * storm_;
}

Environment::SkyColors Environment::skyColors() const
{
    const SkyColors calm{Vec3(0.40f, 0.60f, 0.84f), Vec3(0.93f, 0.80f, 0.62f), Vec3(1.00f, 0.72f, 0.46f)};
    const SkyColors brewing{Vec3(0.33f, 0.30f, 0.45f), Vec3(0.58f, 0.50f, 0.58f), Vec3(0.80f, 0.58f, 0.48f)};
    const SkyColors dark{Vec3(0.10f, 0.11f, 0.16f), Vec3(0.22f, 0.23f, 0.28f), Vec3(0.36f, 0.35f, 0.38f)};

    const SkyColors& from = storm_ < 0.5f ? calm : brewing;
    const SkyColors& to = storm_ < 0.5f ? brewing : dark;
    const float t = storm_ < 0.5f ? storm_ * 2.0f : (storm_ - 0.5f) * 2.0f;

    return SkyColors{lerp(from.top, to.top, t), lerp(from.middle, to.middle, t), lerp(from.horizon, to.horizon, t)};
}

void Environment::drawSky() const
{
    const SkyColors sky = skyColors();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUAD_STRIP);
    glColor3f(sky.horizon.x, sky.horizon.y, sky.horizon.z);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(0.0f, 0.40f);
    glVertex2f(1.0f, 0.40f);
    glColor3f(sky.middle.x, sky.middle.y, sky.middle.z);
    glVertex2f(0.0f, 0.62f);
    glVertex2f(1.0f, 0.62f);
    glColor3f(sky.top.x, sky.top.y, sky.top.z);
    glVertex2f(0.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Environment::drawSun(const Vec3& eye) const
{
    const float fade = 1.0f - clamp01(storm_ * 1.6f);
    if (fade <= 0.0f) {
        return;
    }

    // Billboard: take the camera's right and up axes from the modelview matrix
    // so the disc always faces the viewer.
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    const Vec3 right(m[0], m[4], m[8]);
    const Vec3 up(m[1], m[5], m[9]);
    const Vec3 center = eye + lightDir_ * 80.0f;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float radii[2] = {14.0f, 4.5f};
    const float alphas[2] = {0.35f, 0.95f};
    const int segments = 40;

    for (int layer = 0; layer < 2; ++layer) {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.95f, 0.80f, alphas[layer] * fade);
        glVertex3f(center.x, center.y, center.z);
        glColor4f(1.0f, 0.85f, 0.60f, layer == 0 ? 0.0f : alphas[layer] * fade);
        for (int s = 0; s <= segments; ++s) {
            const float a = 6.2831853f * s / segments;
            const Vec3 p = center + right * (std::cos(a) * radii[layer]) + up * (std::sin(a) * radii[layer]);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Environment::applyFog() const
{
    const Vec3 horizon = skyColors().horizon;
    const GLfloat color[4] = {horizon.x, horizon.y, horizon.z, 1.0f};
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, color);
    glFogf(GL_FOG_START, 10.0f - 4.0f * storm_);
    glFogf(GL_FOG_END, 42.0f - 18.0f * storm_);
}

void Environment::drawGround() const
{
    const float b = brightness();
    const float half = kGroundTiles * kTileSize * 0.5f;

    glBegin(GL_QUADS);
    for (int i = 0; i < kGroundTiles; ++i) {
        for (int j = 0; j < kGroundTiles; ++j) {
            const Vec3 c = groundTileColors_[i * kGroundTiles + j] * b;
            const float x0 = -half + i * kTileSize;
            const float z0 = -half + j * kTileSize;
            glColor3f(c.x, c.y, c.z);
            glVertex3f(x0, 0.0f, z0);
            glVertex3f(x0, 0.0f, z0 + kTileSize);
            glVertex3f(x0 + kTileSize, 0.0f, z0 + kTileSize);
            glVertex3f(x0 + kTileSize, 0.0f, z0);
        }
    }
    glEnd();
}

void Environment::drawClothesLine(const Vec3& left, const Vec3& right) const
{
    const Vec3 c = kBambooBrown * brightness();
    glColor3f(c.x, c.y, c.z);

    glLineWidth(5.0f);
    glBegin(GL_LINES);
    glVertex3f(left.x, 0.0f, left.z);
    glVertex3f(left.x, left.y + 0.15f, left.z);
    glVertex3f(right.x, 0.0f, right.z);
    glVertex3f(right.x, right.y + 0.15f, right.z);
    glEnd();

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(left.x, left.y, left.z);
    glVertex3f(right.x, right.y, right.z);
    glEnd();
    glLineWidth(1.0f);
}
