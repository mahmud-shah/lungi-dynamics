#include "Scenery.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

#include "Wind.h"

namespace {

const float kTwoPi = 6.2831853f;

const float kGroundHalfSize = 40.0f;
const int kGroundCells = 96;
const Vec3 kYardCenter(-2.8f, 0.0f, -3.6f);
const float kYardRadius = 4.4f;

constexpr Vec3 kEarth(0.74f, 0.60f, 0.42f);
constexpr Vec3 kGrassDark(0.30f, 0.50f, 0.24f);
constexpr Vec3 kGrassLight(0.50f, 0.68f, 0.32f);
constexpr Vec3 kRice(0.80f, 0.74f, 0.40f);
constexpr Vec3 kRiceGreen(0.58f, 0.70f, 0.30f);

constexpr Vec3 kMud(0.80f, 0.62f, 0.42f);
constexpr Vec3 kPlinth(0.56f, 0.42f, 0.30f);
constexpr Vec3 kWood(0.32f, 0.21f, 0.14f);
constexpr Vec3 kTin(0.70f, 0.72f, 0.74f);
constexpr Vec3 kTinDark(0.52f, 0.55f, 0.58f);
constexpr Vec3 kRust(0.64f, 0.42f, 0.28f);
constexpr Vec3 kBamboo(0.72f, 0.62f, 0.38f);
constexpr Vec3 kStraw(0.88f, 0.74f, 0.40f);
constexpr Vec3 kStrawShade(0.74f, 0.58f, 0.30f);
constexpr Vec3 kFoliage(0.24f, 0.42f, 0.22f);
constexpr Vec3 kFoliageLight(0.36f, 0.54f, 0.26f);
constexpr Vec3 kBananaStem(0.46f, 0.56f, 0.26f);
constexpr Vec3 kBananaBase(0.46f, 0.37f, 0.22f);
constexpr Vec3 kBananaLeaf(0.40f, 0.64f, 0.24f);
constexpr Vec3 kBananaDry(0.64f, 0.56f, 0.28f);
constexpr Vec3 kGrassRoot(0.24f, 0.40f, 0.18f);
constexpr Vec3 kGrassTip(0.64f, 0.76f, 0.36f);

const int kCloudCount = 14;
const int kTuftCount = 650;
const int kBladesPerTuft = 7;
const int kDebrisCount = 70;
const int kMaxRain = 900;
const float kRainBox = 9.0f;
const float kRainTop = 9.0f;

float hash2(int i, int j)
{
    const float s = std::sin(i * 127.1f + j * 311.7f) * 43758.5453f;
    return s - std::floor(s);
}

// Value noise: random values on an integer grid, blended with a smooth curve.
float valueNoise(float x, float z)
{
    const int ix = static_cast<int>(std::floor(x));
    const int iz = static_cast<int>(std::floor(z));
    float fx = x - ix;
    float fz = z - iz;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);
    const float a = hash2(ix, iz);
    const float b = hash2(ix + 1, iz);
    const float c = hash2(ix, iz + 1);
    const float d = hash2(ix + 1, iz + 1);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fz;
}

float smooth01(float edge0, float edge1, float x)
{
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

Vec3 rotateY(const Vec3& p, float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return Vec3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

} // namespace

// ---------------------------------------------------------------- PropMesh

void PropMesh::addVertex(const Vec3& p, const Vec3& n, const Vec3& c)
{
    positions_.push_back(p);
    normals_.push_back(n);
    colors_.push_back(c);
}

void PropMesh::addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color)
{
    const Vec3 n = normalized(cross(b - a, c - a));
    addVertex(a, n, color);
    addVertex(b, n, color);
    addVertex(c, n, color);
}

void PropMesh::addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color)
{
    addTriangle(a, b, c, color);
    addTriangle(a, c, d, color);
}

void PropMesh::addQuadWithNormal(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                                 const Vec3& normal, const Vec3& color)
{
    const Vec3 corners[6] = {a, b, c, a, c, d};
    for (const Vec3& p : corners) {
        addVertex(p, normal, color);
    }
}

void PropMesh::addBox(const Vec3& lo, const Vec3& hi, const Vec3& color)
{
    const Vec3 p000(lo.x, lo.y, lo.z), p100(hi.x, lo.y, lo.z), p010(lo.x, hi.y, lo.z), p110(hi.x, hi.y, lo.z);
    const Vec3 p001(lo.x, lo.y, hi.z), p101(hi.x, lo.y, hi.z), p011(lo.x, hi.y, hi.z), p111(hi.x, hi.y, hi.z);
    addQuad(p001, p101, p111, p011, color);
    addQuad(p100, p000, p010, p110, color);
    addQuad(p101, p100, p110, p111, color);
    addQuad(p000, p001, p011, p010, color);
    addQuad(p011, p111, p110, p010, color);
}

void PropMesh::addSphere(const Vec3& center, float radius, const Vec3& color, int slices, int stacks)
{
    const float pi = 3.14159265f;
    for (int st = 0; st < stacks; ++st) {
        const float p0 = pi * st / stacks;
        const float p1 = pi * (st + 1) / stacks;
        for (int sl = 0; sl < slices; ++sl) {
            const float t0 = kTwoPi * sl / slices;
            const float t1 = kTwoPi * (sl + 1) / slices;
            const Vec3 n00(std::sin(p0) * std::cos(t0), std::cos(p0), std::sin(p0) * std::sin(t0));
            const Vec3 n01(std::sin(p0) * std::cos(t1), std::cos(p0), std::sin(p0) * std::sin(t1));
            const Vec3 n10(std::sin(p1) * std::cos(t0), std::cos(p1), std::sin(p1) * std::sin(t0));
            const Vec3 n11(std::sin(p1) * std::cos(t1), std::cos(p1), std::sin(p1) * std::sin(t1));
            const Vec3 corners[6] = {n00, n10, n11, n00, n11, n01};
            for (const Vec3& n : corners) {
                addVertex(center + n * radius, n, color);
            }
        }
    }
}

void PropMesh::addCone(const Vec3& base, float radius, float height, const Vec3& color, int slices)
{
    const Vec3 tip = base + Vec3(0.0f, height, 0.0f);
    const float slope = radius / height;
    for (int sl = 0; sl < slices; ++sl) {
        const float t0 = kTwoPi * sl / slices;
        const float t1 = kTwoPi * (sl + 1) / slices;
        const Vec3 a = base + Vec3(std::cos(t0) * radius, 0.0f, std::sin(t0) * radius);
        const Vec3 b = base + Vec3(std::cos(t1) * radius, 0.0f, std::sin(t1) * radius);
        const Vec3 na = normalized(Vec3(std::cos(t0), slope, std::sin(t0)));
        const Vec3 nb = normalized(Vec3(std::cos(t1), slope, std::sin(t1)));
        addVertex(a, na, color);
        addVertex(tip, normalized(na + nb), color);
        addVertex(b, nb, color);
    }
}

// Normals never change, so the toon ramp coordinate of every vertex is
// computed once here.
void PropMesh::finish(const Vec3& lightDir)
{
    rampCoords_.resize(normals_.size());
    for (size_t i = 0; i < normals_.size(); ++i) {
        rampCoords_[i] = ToonRamp::coordinate(normals_[i], lightDir);
    }
    drawColors_.resize(colors_.size());
}

void PropMesh::clear()
{
    positions_.clear();
    normals_.clear();
    colors_.clear();
}

void PropMesh::draw(const ToonRamp& ramp, float brightness)
{
    if (positions_.empty()) {
        return;
    }
    for (size_t i = 0; i < colors_.size(); ++i) {
        drawColors_[i] = colors_[i] * brightness;
    }
    ramp.bind();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &positions_[0].x);
    glColorPointer(3, GL_FLOAT, sizeof(Vec3), &drawColors_[0].x);
    glTexCoordPointer(1, GL_FLOAT, 0, rampCoords_.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(positions_.size()));
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    ramp.unbind();
}

// ---------------------------------------------------------------- Scenery

float Scenery::random(float lo, float hi)
{
    std::uniform_real_distribution<float> d(lo, hi);
    return d(rng_);
}

bool Scenery::blocked(float x, float z) const
{
    const float dx = x - kYardCenter.x;
    const float dz = z - kYardCenter.z;
    if (dx * dx + dz * dz < kYardRadius * kYardRadius * 0.8f) {
        return true;
    }
    for (const BananaTree& b : bananas_) {
        const float bx = x - b.base.x;
        const float bz = z - b.base.z;
        if (bx * bx + bz * bz < 0.6f) {
            return true;
        }
    }
    return false;
}

void Scenery::build(const Vec3& lightDir)
{
    rng_.seed(20261120u);
    lightDir_ = lightDir;
    buildGround();
    buildClouds();
    staticProps_.clear();
    buildHut();
    buildBananaTrees();
    buildHaystacks();
    staticProps_.finish(lightDir);
    treeLine_.clear();
    buildTreeLine();
    treeLine_.finish(lightDir);
    buildGrass();
    buildDebris();
    rain_.resize(kMaxRain);
    for (Vec3& r : rain_) {
        r = Vec3(random(-kRainBox, kRainBox), random(0.0f, kRainTop), random(-kRainBox, kRainBox));
    }
}

// The ground is one smooth grid whose colours come from two octaves of value
// noise: a packed-earth courtyard (uthon) around the character, meadow grass
// around it, and golden rice fields further out. Colours blend smoothly, like
// a painted background, instead of hard tiles.
void Scenery::buildGround()
{
    groundSize_ = kGroundCells + 1;
    groundPositions_.clear();
    groundColors_.clear();
    for (int j = 0; j < groundSize_; ++j) {
        for (int i = 0; i < groundSize_; ++i) {
            const float x = -kGroundHalfSize + 2.0f * kGroundHalfSize * i / kGroundCells;
            const float z = -kGroundHalfSize + 2.0f * kGroundHalfSize * j / kGroundCells;
            const float n = 0.65f * valueNoise(x * 0.15f, z * 0.15f) + 0.35f * valueNoise(x * 0.6f, z * 0.6f);
            Vec3 c = lerp(kGrassDark, kGrassLight, n);

            const float field = valueNoise(x * 0.05f + 7.0f, z * 0.05f + 3.0f);
            const float distance = std::sqrt(x * x + z * z);
            const float rice = smooth01(0.45f, 0.6f, field) * smooth01(10.0f, 16.0f, distance);
            const float rows = 0.5f + 0.5f * std::sin(x * 2.2f + z * 0.4f);
            c = lerp(c, lerp(kRiceGreen, kRice, 0.4f + 0.6f * n) * (0.92f + 0.08f * rows), rice);

            const float dx = x - kYardCenter.x;
            const float dz = z - kYardCenter.z;
            const float yard = 1.0f - smooth01(kYardRadius - 1.5f, kYardRadius + 0.8f, std::sqrt(dx * dx + dz * dz));
            c = lerp(c, kEarth * (0.93f + 0.1f * n), yard);

            groundPositions_.push_back(Vec3(x, 0.0f, z));
            groundColors_.push_back(c);
        }
    }
    groundDrawColors_ = groundColors_;
    groundIndices_.clear();
    for (int j = 0; j < kGroundCells; ++j) {
        for (int i = 0; i < kGroundCells; ++i) {
            const unsigned int a = j * groundSize_ + i;
            const unsigned int b = a + 1;
            const unsigned int c = a + groundSize_;
            const unsigned int d = c + 1;
            groundIndices_.push_back(a);
            groundIndices_.push_back(c);
            groundIndices_.push_back(b);
            groundIndices_.push_back(b);
            groundIndices_.push_back(c);
            groundIndices_.push_back(d);
        }
    }
}

void Scenery::buildClouds()
{
    clouds_.clear();
    for (int i = 0; i < kCloudCount; ++i) {
        Cloud cloud;
        const float angle = kTwoPi * i / kCloudCount + random(-0.2f, 0.2f);
        const float distance = random(85.0f, 120.0f);
        cloud.center = Vec3(std::cos(angle) * distance, random(6.0f, 10.0f), std::sin(angle) * distance);
        cloud.height = random(13.0f, 21.0f);
        const int puffs = static_cast<int>(random(9.0f, 14.0f));
        for (int p = 0; p < puffs; ++p) {
            const float spread = random(-1.0f, 1.0f);
            const float rise = random(0.0f, 1.0f);
            cloud.puffOffsets.push_back(Vec3(spread * 16.0f * (1.0f - 0.5f * rise), rise * cloud.height * 0.85f,
                                             random(-5.0f, 5.0f)));
            cloud.puffRadii.push_back(random(8.0f, 13.0f) * (1.0f - 0.4f * rise) * (1.0f - 0.25f * std::fabs(spread)));
        }
        clouds_.push_back(cloud);
    }
}

// A Bangladeshi village hut: mud walls on an earth plinth, a wooden door,
// bamboo posts and a corrugated tin roof. The corrugation is faked by
// splitting each roof slope into thin strips whose normals tilt left and
// right alternately, so the toon bands draw the ridges.
void Scenery::buildHut()
{
    const Vec3 origin(-4.4f, 0.0f, -5.0f);
    const float turn = 0.45f;
    PropMesh& m = staticProps_;
    auto place = [&](const Vec3& p) { return origin + rotateY(p, turn); };
    auto box = [&](const Vec3& lo, const Vec3& hi, const Vec3& color) {
        const Vec3 p000(lo.x, lo.y, lo.z), p100(hi.x, lo.y, lo.z), p010(lo.x, hi.y, lo.z), p110(hi.x, hi.y, lo.z);
        const Vec3 p001(lo.x, lo.y, hi.z), p101(hi.x, lo.y, hi.z), p011(lo.x, hi.y, hi.z), p111(hi.x, hi.y, hi.z);
        m.addQuad(place(p001), place(p101), place(p111), place(p011), color);
        m.addQuad(place(p100), place(p000), place(p010), place(p110), color);
        m.addQuad(place(p101), place(p100), place(p110), place(p111), color);
        m.addQuad(place(p000), place(p001), place(p011), place(p010), color);
        m.addQuad(place(p011), place(p111), place(p110), place(p010), color);
    };

    box(Vec3(-1.8f, 0.0f, -1.4f), Vec3(1.8f, 0.25f, 1.6f), kPlinth);
    box(Vec3(-1.6f, 0.25f, -1.2f), Vec3(1.6f, 1.9f, 1.2f), kMud);
    m.addQuad(place(Vec3(-0.4f, 0.25f, 1.205f)), place(Vec3(0.4f, 0.25f, 1.205f)),
              place(Vec3(0.4f, 1.55f, 1.205f)), place(Vec3(-0.4f, 1.55f, 1.205f)), kWood);
    m.addQuad(place(Vec3(0.8f, 1.0f, 1.205f)), place(Vec3(1.3f, 1.0f, 1.205f)),
              place(Vec3(1.3f, 1.4f, 1.205f)), place(Vec3(0.8f, 1.4f, 1.205f)), kWood * 0.8f);
    m.addTriangle(place(Vec3(1.6f, 1.9f, 1.2f)), place(Vec3(1.6f, 1.9f, -1.2f)), place(Vec3(1.6f, 2.75f, 0.0f)), kMud);
    m.addTriangle(place(Vec3(-1.6f, 1.9f, -1.2f)), place(Vec3(-1.6f, 1.9f, 1.2f)), place(Vec3(-1.6f, 2.75f, 0.0f)), kMud);
    box(Vec3(-1.78f, 0.25f, 1.42f), Vec3(-1.68f, 1.82f, 1.52f), kBamboo);
    box(Vec3(1.68f, 0.25f, 1.42f), Vec3(1.78f, 1.82f, 1.52f), kBamboo);

    const float ridgeY = 2.8f;
    const float eaveY = 1.78f;
    const float eaveZ = 1.65f;
    const float halfX = 1.95f;
    const int strips = 32;
    for (int side = -1; side <= 1; side += 2) {
        const Vec3 slopeNormal = normalized(Vec3(0.0f, eaveZ, side * (ridgeY - eaveY)));
        for (int s = 0; s < strips; ++s) {
            const float x0 = -halfX + 2.0f * halfX * s / strips;
            const float x1 = -halfX + 2.0f * halfX * (s + 1) / strips;
            const float tilt = (s % 2 == 0) ? 0.35f : -0.35f;
            const Vec3 n = normalized(slopeNormal + Vec3(tilt, 0.0f, 0.0f));
            Vec3 color = (s % 2 == 0) ? kTin : kTinDark;
            if (hash2(s, side) > 0.78f) {
                color = lerp(color, kRust, 0.7f);
            }
            const Vec3 a = place(Vec3(x0, eaveY, side * eaveZ));
            const Vec3 b = place(Vec3(x1, eaveY, side * eaveZ));
            const Vec3 c = place(Vec3(x1, ridgeY, 0.0f));
            const Vec3 d = place(Vec3(x0, ridgeY, 0.0f));
            const Vec3 worldNormal = rotateY(n, turn);
            if (side > 0) {
                m.addQuadWithNormal(a, b, c, d, worldNormal, color);
            } else {
                m.addQuadWithNormal(b, a, d, c, worldNormal, color);
            }
            const Vec3 under(0.0f, -1.0f, 0.0f);
            const Vec3 drop(0.0f, -0.02f, 0.0f);
            m.addQuadWithNormal(a + drop, d + drop, c + drop, b + drop, under, kTinDark * 0.6f);
        }
    }
}

void Scenery::buildBananaTrees()
{
    bananas_.clear();
    const Vec3 bases[] = {Vec3(2.4f, 0.0f, -5.2f), Vec3(3.6f, 0.0f, -4.1f), Vec3(-7.6f, 0.0f, -1.6f)};
    const float heights[] = {2.1f, 1.7f, 2.3f};
    for (int t = 0; t < 3; ++t) {
        BananaTree tree;
        tree.base = bases[t];
        tree.height = heights[t];
        tree.phase = random(0.0f, kTwoPi);
        const int leaves = 8;
        for (int l = 0; l < leaves; ++l) {
            tree.leafAngles.push_back(kTwoPi * l / leaves + random(-0.25f, 0.25f));
            tree.leafLengths.push_back(random(1.3f, 1.8f));
            tree.leafDry.push_back(l == 2 || (t == 2 && l == 5));
        }
        bananas_.push_back(tree);

        const int sides = 10;
        for (int s = 0; s < sides; ++s) {
            const float a0 = kTwoPi * s / sides;
            const float a1 = kTwoPi * (s + 1) / sides;
            for (int seg = 0; seg < 4; ++seg) {
                const float y0 = tree.height * seg / 4.0f;
                const float y1 = tree.height * (seg + 1) / 4.0f;
                const float r0 = 0.15f - 0.06f * seg / 4.0f;
                const float r1 = 0.15f - 0.06f * (seg + 1) / 4.0f;
                const Vec3 color = lerp(kBananaBase, kBananaStem, std::min(1.0f, seg / 2.0f));
                staticProps_.addQuadWithNormal(
                    tree.base + Vec3(std::cos(a0) * r0, y0, std::sin(a0) * r0),
                    tree.base + Vec3(std::cos(a0) * r1, y1, std::sin(a0) * r1),
                    tree.base + Vec3(std::cos(a1) * r1, y1, std::sin(a1) * r1),
                    tree.base + Vec3(std::cos(a1) * r0, y0, std::sin(a1) * r0),
                    Vec3(std::cos(0.5f * (a0 + a1)), 0.0f, std::sin(0.5f * (a0 + a1))), color);
            }
        }
    }
}

void Scenery::buildHaystacks()
{
    const Vec3 spots[] = {Vec3(-9.0f, 0.0f, -7.5f), Vec3(-7.2f, 0.0f, -9.2f), Vec3(6.5f, 0.0f, -9.0f)};
    const float sizes[] = {1.0f, 0.75f, 0.9f};
    for (int i = 0; i < 3; ++i) {
        const float s = sizes[i];
        staticProps_.addCone(spots[i], 1.05f * s, 0.9f * s, kStrawShade, 16);
        staticProps_.addCone(spots[i] + Vec3(0.0f, 0.75f * s, 0.0f), 0.85f * s, 1.25f * s, kStraw, 16);
        staticProps_.addBox(spots[i] + Vec3(-0.03f, 1.9f * s, -0.03f), spots[i] + Vec3(0.03f, 2.3f * s, 0.03f), kWood);
    }
}

// Distant tree clumps on the horizon: a few overlapping spheres each. They
// get their own, thinner haze so they stay green and read as a soft painted
// tree line instead of fading completely into the sky.
void Scenery::buildTreeLine()
{
    for (int i = 0; i < 40; ++i) {
        const float angle = kTwoPi * i / 40.0f + random(-0.06f, 0.06f);
        const float distance = random(38.0f, 46.0f);
        const Vec3 base(std::cos(angle) * distance, 0.0f, std::sin(angle) * distance);
        const float size = random(1.6f, 2.6f);
        treeLine_.addBox(base + Vec3(-0.15f, 0.0f, -0.15f), base + Vec3(0.15f, size * 1.4f, 0.15f), kWood);
        for (int k = 0; k < 3; ++k) {
            const Vec3 offset(random(-1.4f, 1.4f), size * (1.5f + random(0.0f, 0.7f)), random(-1.0f, 1.0f));
            treeLine_.addSphere(base + offset, size * random(0.75f, 1.05f), k == 0 ? kFoliageLight : kFoliage, 10, 7);
        }
    }
}

void Scenery::buildGrass()
{
    blades_.clear();
    for (int t = 0; t < kTuftCount; ++t) {
        const float angle = random(0.0f, kTwoPi);
        const float distance = 3.0f + std::sqrt(random(0.0f, 1.0f)) * 19.0f;
        const float x = std::cos(angle) * distance;
        const float z = std::sin(angle) * distance;
        if (blocked(x, z)) {
            continue;
        }
        for (int b = 0; b < kBladesPerTuft; ++b) {
            Blade blade;
            blade.base = Vec3(x + random(-0.12f, 0.12f), 0.0f, z + random(-0.12f, 0.12f));
            blade.height = random(0.22f, 0.5f);
            blade.lean = random(0.0f, kTwoPi);
            blade.phase = random(0.0f, kTwoPi);
            blades_.push_back(blade);
        }
    }
    grassPositions_.resize(blades_.size() * 3);
    grassColors_.resize(blades_.size() * 3);
    grassDrawColors_.resize(blades_.size() * 3);
    for (size_t i = 0; i < blades_.size(); ++i) {
        const float shade = 0.85f + 0.15f * hash2(static_cast<int>(i), 7);
        grassColors_[i * 3 + 0] = kGrassRoot * shade;
        grassColors_[i * 3 + 1] = kGrassRoot * shade;
        grassColors_[i * 3 + 2] = kGrassTip * shade;
    }
}

void Scenery::buildDebris()
{
    debris_.resize(kDebrisCount);
    for (size_t i = 0; i < debris_.size(); ++i) {
        Debris& d = debris_[i];
        d.position = Vec3(random(-10.0f, 10.0f), random(0.0f, 0.05f), random(-10.0f, 10.0f));
        d.spin = random(3.0f, 9.0f);
        d.angle = random(0.0f, kTwoPi);
        d.paper = i % 4 == 0;
    }
}

// Banana leaves are rebuilt every frame because they sway. Each leaf follows
// an arching centre line P(t) = top + dir * L t + up * L (0.5 t - 0.8 t^2),
// is widest in the middle, and is bent downwind by an amount that grows
// with the wind speed. Some segments on one side are left out, which gives
// the torn look real banana leaves get in the wind.
void Scenery::buildBananaLeaves(float time, const Vec3& windDir, float windSpeed)
{
    bananaLeaves_.clear();
    const int segments = 9;
    const float bend = 0.04f + 0.022f * windSpeed;
    for (size_t t = 0; t < bananas_.size(); ++t) {
        const BananaTree& tree = bananas_[t];
        const Vec3 top = tree.base + Vec3(0.0f, tree.height, 0.0f);
        for (size_t l = 0; l < tree.leafAngles.size(); ++l) {
            const float a = tree.leafAngles[l];
            const float length = tree.leafLengths[l];
            const Vec3 dir(std::cos(a), 0.0f, std::sin(a));
            const Vec3 side = normalized(cross(Vec3(0.0f, 1.0f, 0.0f), dir));
            const float droop = tree.leafDry[l] ? 1.4f : 0.8f;
            const float sway = 0.7f + 0.3f * std::sin(time * (1.8f + 0.25f * windSpeed) + tree.phase + l);
            const Vec3 color = tree.leafDry[l] ? kBananaDry : kBananaLeaf;

            Vec3 prevCenter = top;
            Vec3 prevLeft = top;
            Vec3 prevRight = top;
            for (int s = 1; s <= segments; ++s) {
                const float u = static_cast<float>(s) / segments;
                const float flutter = std::sin(time * 7.0f + u * 5.0f + l) * 0.012f * windSpeed * u;
                const Vec3 center = top + dir * (length * u)
                                  + Vec3(0.0f, length * (0.5f * u - droop * u * u), 0.0f)
                                  + windDir * (bend * length * u * u * sway) + side * flutter;
                const float width = 0.28f * std::sin(3.14159265f * std::min(1.0f, u * 1.08f));
                const Vec3 left = center + side * width + Vec3(0.0f, -0.05f * width, 0.0f);
                const Vec3 right = center - side * width + Vec3(0.0f, -0.05f * width, 0.0f);
                const bool tornLeft = u > 0.3f && hash2(static_cast<int>(t * 31 + l), s) < 0.22f;
                const bool tornRight = u > 0.3f && hash2(static_cast<int>(t * 17 + l), s + 50) < 0.22f;
                if (!tornLeft) {
                    bananaLeaves_.addQuad(prevCenter, center, left, prevLeft, color);
                }
                if (!tornRight) {
                    bananaLeaves_.addQuad(prevRight, right, center, prevCenter, color * 0.92f);
                }
                prevCenter = center;
                prevLeft = left;
                prevRight = right;
            }
        }
    }
    bananaLeaves_.finish(lightDir_);
}

void Scenery::update(float dt, const Wind& wind, float storm)
{
    time_ += dt;
    windDir_ = wind.direction();
    windSpeed_ = wind.speed();

    for (Cloud& c : clouds_) {
        c.center += windDir_ * ((0.6f + windSpeed_ * 0.35f) * dt);
        const float d = std::sqrt(c.center.x * c.center.x + c.center.z * c.center.z);
        if (d > 140.0f) {
            c.center.x = -c.center.x * 0.6f;
            c.center.z = -c.center.z * 0.6f;
        }
    }

    for (size_t i = 0; i < debris_.size(); ++i) {
        Debris& d = debris_[i];
        if (windSpeed_ < 2.5f) {
            d.position.y = std::max(0.02f, d.position.y - 0.8f * dt);
            continue;
        }
        Vec3 v = wind.velocityAt(d.position) * 0.75f;
        v.y += std::sin(time_ * 3.0f + i) * 0.8f + (d.paper ? 0.3f : 0.0f) - 0.4f;
        d.position += v * dt;
        d.angle += d.spin * dt;
        if (d.position.y < 0.02f) {
            d.position.y = 0.02f;
        }
        if (std::fabs(d.position.x) > 12.0f || std::fabs(d.position.z) > 12.0f || d.position.y > 6.0f) {
            d.position = Vec3(random(-10.0f, 10.0f), random(0.05f, 1.5f), random(-10.0f, 10.0f))
                       - windDir_ * 10.0f;
        }
    }

    updateRain(dt, wind, storm);
    updateLightning(dt, storm);
}

void Scenery::updateRain(float dt, const Wind& wind, float storm)
{
    activeRain_ = static_cast<int>(kMaxRain * smooth01(0.45f, 1.0f, storm));
    const Vec3 fall = wind.direction() * (wind.speed() * 0.6f) + Vec3(0.0f, -9.0f, 0.0f);
    for (int i = 0; i < activeRain_; ++i) {
        Vec3& r = rain_[i];
        r += fall * dt;
        if (r.y < 0.0f || std::fabs(r.x) > kRainBox || std::fabs(r.z) > kRainBox) {
            r = Vec3(random(-kRainBox, kRainBox), kRainTop, random(-kRainBox, kRainBox)) - fall * random(0.0f, 0.6f);
        }
    }
}

// Lightning only in the strongest storm. A flash decays quickly with a
// second flicker, and thunder follows 0.4 - 1.2 s later (light is faster
// than sound).
void Scenery::updateLightning(float dt, float storm)
{
    const float before = flash_;
    flash_ = std::max(0.0f, flash_ - dt * 5.0f);
    if (before > 0.55f && flash_ <= 0.55f && random(0.0f, 1.0f) < 0.6f) {
        flash_ = 0.9f;
    }

    if (thunderDelay_ >= 0.0f) {
        thunderDelay_ -= dt;
        if (thunderDelay_ < 0.0f) {
            thunderPending_ = true;
        }
    }

    if (storm < 0.85f) {
        timeToLightning_ = random(2.0f, 4.0f);
        return;
    }
    timeToLightning_ -= dt;
    if (timeToLightning_ <= 0.0f) {
        flash_ = 1.0f;
        thunderDelay_ = random(0.4f, 1.2f);
        timeToLightning_ = random(4.0f, 9.0f);

        bolt_.clear();
        const float angle = random(0.0f, kTwoPi);
        Vec3 p(std::cos(angle) * 60.0f, 38.0f, std::sin(angle) * 60.0f);
        bolt_.push_back(p);
        while (p.y > 2.0f) {
            p += Vec3(random(-2.5f, 2.5f), -random(2.5f, 5.0f), random(-2.5f, 2.5f));
            bolt_.push_back(p);
        }
    }
}

bool Scenery::consumeThunder()
{
    const bool pending = thunderPending_;
    thunderPending_ = false;
    return pending;
}

// Cumulus clouds are groups of soft billboard discs. Each disc is brighter
// at the top and shaded underneath (by its height inside the cloud), which
// gives the big, sunlit, painted clouds of a Boishakh afternoon. As the storm
// grows they turn bruised purple and then dark grey.
void Scenery::drawClouds(const Vec3& eye, float storm, float flash) const
{
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    const Vec3 right(m[0], m[4], m[8]);
    const Vec3 up(m[1], m[5], m[9]);

    const float s = clamp01(storm);
    const Vec3 lit = s < 0.5f ? lerp(Vec3(1.0f, 0.97f, 0.92f), Vec3(0.68f, 0.62f, 0.70f), s * 2.0f)
                              : lerp(Vec3(0.68f, 0.62f, 0.70f), Vec3(0.34f, 0.34f, 0.40f), (s - 0.5f) * 2.0f);
    const Vec3 shade = s < 0.5f ? lerp(Vec3(0.80f, 0.78f, 0.90f), Vec3(0.46f, 0.42f, 0.54f), s * 2.0f)
                                : lerp(Vec3(0.46f, 0.42f, 0.54f), Vec3(0.20f, 0.20f, 0.26f), (s - 0.5f) * 2.0f);
    const Vec3 flashTint(0.85f, 0.85f, 1.0f);

    int order[kCloudCount];
    for (int i = 0; i < kCloudCount; ++i) {
        order[i] = i;
    }
    std::sort(order, order + kCloudCount, [&](int a, int b) {
        return length(clouds_[a].center - eye) > length(clouds_[b].center - eye);
    });

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const int segments = 18;
    for (int idx = 0; idx < kCloudCount; ++idx) {
        const Cloud& c = clouds_[order[idx]];
        for (size_t p = 0; p < c.puffOffsets.size(); ++p) {
            const Vec3 center = c.center + c.puffOffsets[p];
            const float r = c.puffRadii[p];
            auto colorAt = [&](const Vec3& point, float alpha) {
                const float h = clamp01((point.y - c.center.y + r * 0.3f) / (c.height + r * 0.6f));
                Vec3 col = lerp(shade, lit, h * h * (3.0f - 2.0f * h));
                col = lerp(col, flashTint, flash * 0.6f);
                glColor4f(col.x, col.y, col.z, alpha);
            };
            glBegin(GL_TRIANGLE_FAN);
            colorAt(center, 0.95f);
            glVertex3f(center.x, center.y, center.z);
            for (int k = 0; k <= segments; ++k) {
                const float a = kTwoPi * k / segments;
                const Vec3 q = center + right * (std::cos(a) * r * 0.62f) + up * (std::sin(a) * r * 0.62f);
                colorAt(q, 0.9f);
                glVertex3f(q.x, q.y, q.z);
            }
            glEnd();
            glBegin(GL_QUAD_STRIP);
            for (int k = 0; k <= segments; ++k) {
                const float a = kTwoPi * k / segments;
                const Vec3 inner = center + right * (std::cos(a) * r * 0.62f) + up * (std::sin(a) * r * 0.62f);
                const Vec3 outer = center + right * (std::cos(a) * r) + up * (std::sin(a) * r);
                colorAt(inner, 0.9f);
                glVertex3f(inner.x, inner.y, inner.z);
                colorAt(outer, 0.0f);
                glVertex3f(outer.x, outer.y, outer.z);
            }
            glEnd();
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Scenery::drawGround(float brightness)
{
    for (size_t i = 0; i < groundColors_.size(); ++i) {
        groundDrawColors_[i] = groundColors_[i] * brightness;
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &groundPositions_[0].x);
    glColorPointer(3, GL_FLOAT, sizeof(Vec3), &groundDrawColors_[0].x);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(groundIndices_.size()), GL_UNSIGNED_INT, groundIndices_.data());
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Scenery::drawProps(const ToonRamp& ramp, const Vec3& lightDir, float brightness)
{
    (void)lightDir;
    GLfloat fogStart = 0.0f;
    GLfloat fogEnd = 0.0f;
    glGetFloatv(GL_FOG_START, &fogStart);
    glGetFloatv(GL_FOG_END, &fogEnd);
    glFogf(GL_FOG_START, fogStart + 18.0f);
    glFogf(GL_FOG_END, fogEnd + 60.0f);
    treeLine_.draw(ramp, brightness);
    glFogf(GL_FOG_START, fogStart);
    glFogf(GL_FOG_END, fogEnd);

    staticProps_.draw(ramp, brightness);
    buildBananaLeaves(time_, windDir_, windSpeed_);
    bananaLeaves_.draw(ramp, brightness);
}

// Each blade is one triangle. Its tip leans downwind by an amount that grows
// with the wind speed and wobbles over time, so the meadow ripples.
void Scenery::drawGrass(float brightness)
{
    const float push = 0.05f + 0.03f * windSpeed_;
    for (size_t i = 0; i < blades_.size(); ++i) {
        const Blade& b = blades_[i];
        const float wobble = 0.6f + 0.4f * std::sin(time_ * (2.0f + 0.3f * windSpeed_) + b.phase
                                                     + b.base.x * 0.5f + b.base.z * 0.5f);
        const Vec3 rest(std::cos(b.lean) * 0.06f, b.height, std::sin(b.lean) * 0.06f);
        Vec3 tip = b.base + rest + windDir_ * (push * b.height * 2.5f * wobble);
        tip.y = b.base.y + b.height * (1.0f - 0.25f * clamp01(push * wobble));
        const Vec3 side(-std::sin(b.lean) * 0.028f, 0.0f, std::cos(b.lean) * 0.028f);
        grassPositions_[i * 3 + 0] = b.base - side;
        grassPositions_[i * 3 + 1] = b.base + side;
        grassPositions_[i * 3 + 2] = tip;
    }
    for (size_t i = 0; i < grassColors_.size(); ++i) {
        grassDrawColors_[i] = grassColors_[i] * brightness;
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &grassPositions_[0].x);
    glColorPointer(3, GL_FLOAT, sizeof(Vec3), &grassDrawColors_[0].x);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(grassPositions_.size()));
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Scenery::drawDebris(float brightness) const
{
    glBegin(GL_QUADS);
    for (const Debris& d : debris_) {
        const float c = std::cos(d.angle);
        const float s = std::sin(d.angle);
        const Vec3 a(c * 0.055f, s * 0.035f, -s * 0.03f);
        const Vec3 b(-s * 0.03f, c * 0.02f, c * 0.055f);
        if (d.paper) {
            glColor3f(0.95f * brightness, 0.93f * brightness, 0.86f * brightness);
        } else {
            glColor3f(0.36f * brightness, 0.58f * brightness, 0.22f * brightness);
        }
        const Vec3 p = d.position;
        glVertex3f(p.x - a.x - b.x, p.y - a.y - b.y, p.z - a.z - b.z);
        glVertex3f(p.x + a.x - b.x, p.y + a.y - b.y, p.z + a.z - b.z);
        glVertex3f(p.x + a.x + b.x, p.y + a.y + b.y, p.z + a.z + b.z);
        glVertex3f(p.x - a.x + b.x, p.y - a.y + b.y, p.z - a.z + b.z);
    }
    glEnd();
}

void Scenery::drawRain(float brightness) const
{
    if (activeRain_ <= 0) {
        return;
    }
    const Vec3 streak = windDir_ * (windSpeed_ * 0.6f * 0.035f) + Vec3(0.0f, -0.32f, 0.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBegin(GL_LINES);
    for (int i = 0; i < activeRain_; ++i) {
        const Vec3& r = rain_[i];
        glColor4f(0.78f * brightness, 0.82f * brightness, 0.92f * brightness, 0.0f);
        glVertex3f(r.x, r.y, r.z);
        glColor4f(0.78f * brightness, 0.82f * brightness, 0.92f * brightness, 0.45f);
        glVertex3f(r.x + streak.x, r.y + streak.y, r.z + streak.z);
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Scenery::drawLightningBolt() const
{
    if (flash_ < 0.5f || bolt_.size() < 2) {
        return;
    }
    glDisable(GL_FOG);
    glLineWidth(3.0f);
    glColor3f(0.95f, 0.95f, 1.0f);
    glBegin(GL_LINE_STRIP);
    for (const Vec3& p : bolt_) {
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
    glLineWidth(1.0f);
}
