#include "FilmOverlay.h"

#include <GL/freeglut.h>
#include <cmath>
#include <random>
#include <vector>

namespace {

const int kGrainSize = 128;
const float kGrainAlpha = 0.05f;
const float kTwoPi = 6.2831853f;

float fract(float v)
{
    return v - std::floor(v);
}

} // namespace

void FilmOverlay::create()
{
    std::mt19937 rng(42u);
    std::uniform_int_distribution<int> value(0, 255);
    std::vector<unsigned char> texels(kGrainSize * kGrainSize * 4);
    for (int i = 0; i < kGrainSize * kGrainSize; ++i) {
        const unsigned char v = static_cast<unsigned char>(value(rng));
        texels[i * 4 + 0] = v;
        texels[i * 4 + 1] = v;
        texels[i * 4 + 2] = v;
        texels[i * 4 + 3] = 255;
    }
    GLuint id = 0;
    glGenTextures(1, &id);
    grainTexture_ = id;
    glBindTexture(GL_TEXTURE_2D, grainTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kGrainSize, kGrainSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Vignette: a ring from a transparent inner ellipse to a dark warm edge.
// Grain: a random noise tile, shifted to a new random offset every frame and
// blended at 5% so it flickers like film without hiding detail.
void FilmOverlay::draw(int width, int height, float storm, float flash)
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

    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float edgeAlpha = 0.30f + 0.15f * storm;
    const int segments = 48;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        const float a = kTwoPi * i / segments;
        const float c = std::cos(a);
        const float s = std::sin(a);
        glColor4f(0.10f, 0.07f, 0.06f, 0.0f);
        glVertex2f(cx + c * width * 0.40f, cy + s * height * 0.40f);
        glColor4f(0.10f, 0.07f, 0.06f, edgeAlpha);
        glVertex2f(cx + c * width * 0.76f, cy + s * height * 0.76f);
    }
    glEnd();

    if (flash > 0.0f) {
        glColor4f(0.92f, 0.92f, 1.0f, 0.35f * flash);
        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(static_cast<float>(width), 0.0f);
        glVertex2f(static_cast<float>(width), static_cast<float>(height));
        glVertex2f(0.0f, static_cast<float>(height));
        glEnd();
    }

    ++frame_;
    const float u = fract(std::sin(frame_ * 12.9898f) * 43758.5453f);
    const float v = fract(std::sin(frame_ * 78.233f) * 12345.678f);
    const float su = static_cast<float>(width) / kGrainSize;
    const float sv = static_cast<float>(height) / kGrainSize;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, grainTexture_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, kGrainAlpha);
    glBegin(GL_QUADS);
    glTexCoord2f(u, v);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(u + su, v);
    glVertex2f(static_cast<float>(width), 0.0f);
    glTexCoord2f(u + su, v + sv);
    glVertex2f(static_cast<float>(width), static_cast<float>(height));
    glTexCoord2f(u, v + sv);
    glVertex2f(0.0f, static_cast<float>(height));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}
