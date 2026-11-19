#include "Pattern.h"

#include <GL/freeglut.h>

namespace {

constexpr Vec3 kNavy(0.10f, 0.16f, 0.34f);
constexpr Vec3 kWhite(0.92f, 0.91f, 0.86f);
constexpr Vec3 kGreen(0.12f, 0.40f, 0.30f);
constexpr Vec3 kMaroon(0.50f, 0.12f, 0.12f);
constexpr Vec3 kCream(0.88f, 0.85f, 0.76f);
constexpr Vec3 kBrown(0.42f, 0.26f, 0.16f);
constexpr Vec3 kBlue(0.16f, 0.32f, 0.62f);
constexpr Vec3 kSky(0.62f, 0.76f, 0.90f);
constexpr Vec3 kTeal(0.14f, 0.46f, 0.48f);
constexpr Vec3 kRust(0.60f, 0.34f, 0.20f);
constexpr Vec3 kBottleGreen(0.10f, 0.32f, 0.22f);

Vec3 colorAt(const std::vector<Stripe>& sett, float position)
{
    float total = 0.0f;
    for (const Stripe& s : sett) {
        total += s.width;
    }
    float x = position * total;
    for (const Stripe& s : sett) {
        if (x < s.width) {
            return s.color;
        }
        x -= s.width;
    }
    return sett.back().color;
}

} // namespace

namespace Patterns {

std::vector<FabricPattern> builtIn()
{
    std::vector<FabricPattern> list;

    const std::vector<Stripe> check = {{kNavy, 8}, {kWhite, 1}, {kGreen, 6}, {kWhite, 1}, {kNavy, 8}, {kMaroon, 2}};
    list.push_back({"Classic Check", check, check});

    list.push_back({"Stripe", {{kCream, 1}}, {{kCream, 7}, {kBrown, 1}, {kCream, 3}, {kBrown, 1}}});

    const std::vector<Stripe> gingham = {{kBlue, 4}, {kSky, 4}};
    list.push_back({"Dual-tone Check", gingham, gingham});

    const std::vector<Stripe> tartan = {{kTeal, 6}, {kRust, 3}, {kNavy, 4}, {kRust, 1}, {kTeal, 2}, {kCream, 0.6f}};
    list.push_back({"Tartan", tartan, tartan});

    list.push_back({"Plain", {{kBottleGreen, 1}}, {{kBottleGreen, 1}}});

    return list;
}

} // namespace Patterns

void FabricTexture::create()
{
    GLuint id = 0;
    glGenTextures(1, &id);
    textureId_ = id;
    texels_.resize(kSize * kSize * 3);
}

// Each texel shows either the warp thread or the weft thread crossing at that
// point. A 2/2 twill weave decides which one is on top: the choice flips every
// two texels along the diagonal, which gives woven cloth its fine diagonal
// texture. Because the sett is stretched over exactly one texture tile, the
// tile repeats without a visible seam.
void FabricTexture::upload(const FabricPattern& pattern)
{
    Vec3 sum;
    for (int y = 0; y < kSize; ++y) {
        const Vec3 weft = colorAt(pattern.weft, (y + 0.5f) / kSize);
        for (int x = 0; x < kSize; ++x) {
            const Vec3 warp = colorAt(pattern.warp, (x + 0.5f) / kSize);
            const bool warpOnTop = ((x + y) / 2) % 2 == 0;
            Vec3 c = warpOnTop ? warp * 0.8f + weft * 0.2f : weft * 0.8f + warp * 0.2f;
            if ((x + y) % 2 == 1) {
                c *= 0.94f;
            }
            sum += c;
            const int i = (y * kSize + x) * 3;
            texels_[i + 0] = static_cast<unsigned char>(clamp01(c.x) * 255.0f);
            texels_[i + 1] = static_cast<unsigned char>(clamp01(c.y) * 255.0f);
            texels_[i + 2] = static_cast<unsigned char>(clamp01(c.z) * 255.0f);
        }
    }
    mainColor_ = sum / static_cast<float>(kSize * kSize);

    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, kSize, kSize, GL_RGB, GL_UNSIGNED_BYTE, texels_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FabricTexture::bind() const
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void FabricTexture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
