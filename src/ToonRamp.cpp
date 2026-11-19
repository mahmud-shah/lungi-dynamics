#include "ToonRamp.h"

#include <GL/freeglut.h>

namespace {

const int kRampSize = 256;
const float kShadowLevel = 0.52f;
const float kMidLevel = 0.78f;
const float kShadowEdge = -0.05f;
const float kLitEdge = 0.45f;
const float kEdgeSoftness = 0.03f;

constexpr Vec3 kShadowTint(0.80f, 0.82f, 1.00f);
constexpr Vec3 kLightTint(1.00f, 0.96f, 0.88f);

float smoothstep(float edge0, float edge1, float x)
{
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

// The ramp maps N.L in [-1, 1] to three soft-edged bands (shadow, mid, lit),
// tinted cool in shadow and warm in light. Vertices only send N.L as a
// texture coordinate; the texture lookup happens per pixel, so band edges
// stay smooth even on coarse meshes.
void ToonRamp::create()
{
    unsigned char texels[kRampSize * 3];
    for (int i = 0; i < kRampSize; ++i) {
        const float nDotL = 2.0f * i / (kRampSize - 1) - 1.0f;
        const float toMid = smoothstep(kShadowEdge - kEdgeSoftness, kShadowEdge + kEdgeSoftness, nDotL);
        const float toLit = smoothstep(kLitEdge - kEdgeSoftness, kLitEdge + kEdgeSoftness, nDotL);
        const float level = kShadowLevel + (kMidLevel - kShadowLevel) * toMid + (1.0f - kMidLevel) * toLit;
        const Vec3 tint = lerp(kShadowTint, kLightTint, (level - kShadowLevel) / (1.0f - kShadowLevel));
        const Vec3 c = tint * level;
        texels[i * 3 + 0] = static_cast<unsigned char>(clamp01(c.x) * 255.0f);
        texels[i * 3 + 1] = static_cast<unsigned char>(clamp01(c.y) * 255.0f);
        texels[i * 3 + 2] = static_cast<unsigned char>(clamp01(c.z) * 255.0f);
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    textureId_ = id;
    glBindTexture(GL_TEXTURE_1D, textureId_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, kRampSize, 0, GL_RGB, GL_UNSIGNED_BYTE, texels);
    glBindTexture(GL_TEXTURE_1D, 0);
}

void ToonRamp::bind() const
{
    glEnable(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, textureId_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void ToonRamp::unbind() const
{
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_1D);
}

float ToonRamp::coordinate(const Vec3& normal, const Vec3& lightDir)
{
    return clamp01(0.5f * (dot(normal, lightDir) + 1.0f));
}
