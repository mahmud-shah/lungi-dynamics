#include "Knot.h"

#include <GL/freeglut.h>
#include <cmath>

namespace {

const float kSmoothingSeconds = 0.15f;
const float kWearRate = 0.35f;
const float kStrainingLoad = 0.7f;
const float kMaxWaistDrop = 0.10f;

const int kRollSegments = 64;
const int kRollSides = 8;
const float kRollMinRadius = 0.008f;
const float kRollExtraRadius = 0.024f;
const float kRollBackFraction = 0.45f;
const float kFrontAngle = 1.5707963f;
const float kTwistAngle = kFrontAngle - 0.08f;
const float kTwoPi = 6.2831853f;

float frontness(float theta)
{
    const float c = std::cos(theta - kFrontAngle);
    return c > 0.0f ? c * c : 0.0f;
}

} // namespace

Knot::Knot()
{
    const int ring = kRollSides + 1;
    rollPositions_.resize((kRollSegments + 1) * ring);
    rollCoords_.resize(rollPositions_.size());
    for (int s = 0; s < kRollSegments; ++s) {
        for (int k = 0; k < kRollSides; ++k) {
            const unsigned int a = s * ring + k;
            const unsigned int b = a + ring;
            rollIndices_.push_back(a);
            rollIndices_.push_back(b);
            rollIndices_.push_back(a + 1);
            rollIndices_.push_back(a + 1);
            rollIndices_.push_back(b);
            rollIndices_.push_back(b + 1);
        }
    }
}

void Knot::reset()
{
    load_ = 0.0f;
    health_ = 1.0f;
    failed_ = false;
}

// load = smoothed knot force / knot strength, so 1.0 means "at the limit".
// While load > 1 the knot wears out at a rate proportional to the overload:
//   health -= (load - 1) * kWearRate * dt
// It fails when health reaches zero, so one short spike is survivable but a
// long strong gust is not.
void Knot::update(float dt, float loadNewtons)
{
    if (failed_) {
        return;
    }

    const float raw = loadNewtons / strength_;
    const float blend = 1.0f - std::exp(-dt / kSmoothingSeconds);
    load_ += (raw - load_) * blend;

    if (load_ > 1.0f) {
        health_ -= (load_ - 1.0f) * kWearRate * dt;
        if (health_ <= 0.0f) {
            health_ = 0.0f;
            failed_ = true;
        }
    }
}

// As the knot wears, the waistline slides down, so the player can see it
// loosening before it lets go.
float Knot::waistDrop() const
{
    return (1.0f - health_) * kMaxWaistDrop;
}

const char* Knot::status() const
{
    if (failed_) {
        return "FAILED";
    }
    if (load_ >= 1.0f) {
        return health_ > 0.5f ? "Roll unrolling!" : "Twist slipping out!";
    }
    if (load_ >= kStrainingLoad) {
        return "Straining";
    }
    return "Tight";
}

// A Bangladeshi lungi is held by rolling its top edge down into a thick band
// and tucking a twisted fold in at the front. Knot health is shown as that
// band getting thinner (the roll unrolling) and the twist shrinking.
void Knot::draw(const WaistBand& band, const Vec3& clothColor, const ToonRamp& ramp,
                const Vec3& lightDir, float brightness)
{
    if (failed_) {
        return;
    }

    ramp.bind();
    const Vec3 rollColor = clothColor * (0.9f * brightness);
    glColor3f(rollColor.x, rollColor.y, rollColor.z);
    drawRoll(band, lightDir);
    drawTwist(band, clothColor, lightDir, brightness);
    ramp.unbind();
}

// The roll is a torus that follows the elliptical waistline. The rolled edge
// is thickest at the front, where the extra cloth was gathered, and thinner
// at the back. Each vertex sits on a small circle around the waistline, and
// its normal points from the waistline out to the vertex.
void Knot::drawRoll(const WaistBand& band, const Vec3& lightDir)
{
    const float frontTube = kRollMinRadius + kRollExtraRadius * health_;
    const int ring = kRollSides + 1;

    for (int s = 0; s <= kRollSegments; ++s) {
        const float theta = kTwoPi * s / kRollSegments;
        const float tube = frontTube * (kRollBackFraction + (1.0f - kRollBackFraction) * frontness(theta));
        const Vec3 outward = normalized(Vec3(std::cos(theta) / band.radiusX, 0.0f, std::sin(theta) / band.radiusZ));
        const Vec3 core = band.center + Vec3(std::cos(theta) * band.radiusX, 0.0f, std::sin(theta) * band.radiusZ)
                        - outward * (0.15f * tube) + Vec3(0.0f, 0.4f * tube, 0.0f);
        for (int k = 0; k <= kRollSides; ++k) {
            const float phi = kTwoPi * k / kRollSides;
            const Vec3 n = outward * std::cos(phi) + Vec3(0.0f, std::sin(phi), 0.0f);
            const int i = s * ring + k;
            rollPositions_[i] = core + n * tube;
            rollCoords_[i] = ToonRamp::coordinate(n, lightDir);
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &rollPositions_[0].x);
    glTexCoordPointer(1, GL_FLOAT, 0, rollCoords_.data());
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(rollIndices_.size()), GL_UNSIGNED_INT, rollIndices_.data());
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// The "double twist": two points of the top edge are twisted around each
// other into a fat rope lying across the front of the roll. Each strand is a
// chain of overlapping balls on a helix around that rope's axis:
//   p(t) = c + side * (t * L) + (out * cos(a) + up * sin(a)) * h,  a = t * turns * pi + strand * pi
// The two strands are shaded differently so the twist reads clearly. As the
// knot weakens the rope thins and loosens, and the tucked end slides out.
void Knot::drawTwist(const WaistBand& band, const Vec3& clothColor, const Vec3& lightDir, float brightness)
{
    const float frontTube = kRollMinRadius + kRollExtraRadius * health_;
    const Vec3 up(0.0f, 1.0f, 0.0f);
    const Vec3 out = normalized(Vec3(std::cos(kTwistAngle) / band.radiusX, 0.0f,
                                     std::sin(kTwistAngle) / band.radiusZ));
    const Vec3 side = normalized(cross(up, out));
    const Vec3 center = band.center + Vec3(std::cos(kTwistAngle) * band.radiusX, 0.0f,
                                           std::sin(kTwistAngle) * band.radiusZ)
                      + out * (1.2f * frontTube + 0.012f) + up * 0.006f;

    const float size = 0.65f + 0.35f * health_;
    const float halfLength = 0.085f * size;
    const float helixRadius = 0.022f * size;
    const float strandRadius = 0.034f * size;
    const float turns = 1.5f + 0.5f * health_;
    const int balls = 16;

    const Vec3 light = clothColor * 1.2f;
    const Vec3 colors[2] = {Vec3(clamp01(light.x), clamp01(light.y), clamp01(light.z)) * brightness,
                            clothColor * (0.75f * brightness)};

    for (int strand = 0; strand < 2; ++strand) {
        glColor3f(colors[strand].x, colors[strand].y, colors[strand].z);
        for (int i = 0; i < balls; ++i) {
            const float t = -1.0f + 2.0f * i / (balls - 1);
            const float a = t * turns * 3.14159265f + strand * 3.14159265f;
            const float taper = 1.0f - 0.45f * t * t;
            const Vec3 p = center + side * (t * halfLength) - up * (0.010f * t)
                         + (out * std::cos(a) + up * std::sin(a)) * (helixRadius * taper);
            drawBall(p, strandRadius * taper, lightDir);
        }
    }

    if (health_ < 0.9f) {
        glColor3f(colors[1].x, colors[1].y, colors[1].z);
        const float tailLength = 0.18f * (0.9f - health_);
        const Vec3 tailTop = center + side * (0.3f * halfLength) - up * 0.025f;
        const int steps = 10;
        for (int i = 0; i < steps; ++i) {
            const float t = static_cast<float>(i) / (steps - 1);
            drawBall(tailTop - up * (tailLength * t) + out * 0.004f + side * (0.008f * std::sin(t * 6.0f)),
                     0.019f * (1.0f - 0.35f * t), lightDir);
        }
    }
}

void Knot::drawBall(const Vec3& center, float radius, const Vec3& lightDir) const
{
    const int stacks = 8;
    const int slices = 12;
    const float pi = 3.14159265f;

    for (int st = 0; st < stacks; ++st) {
        const float phi0 = pi * st / stacks;
        const float phi1 = pi * (st + 1) / stacks;
        glBegin(GL_TRIANGLE_STRIP);
        for (int sl = 0; sl <= slices; ++sl) {
            const float theta = kTwoPi * sl / slices;
            const Vec3 n0(std::sin(phi0) * std::cos(theta), std::cos(phi0), std::sin(phi0) * std::sin(theta));
            const Vec3 n1(std::sin(phi1) * std::cos(theta), std::cos(phi1), std::sin(phi1) * std::sin(theta));
            const Vec3 p0 = center + n0 * radius;
            const Vec3 p1 = center + n1 * radius;
            glTexCoord1f(ToonRamp::coordinate(n0, lightDir));
            glVertex3f(p0.x, p0.y, p0.z);
            glTexCoord1f(ToonRamp::coordinate(n1, lightDir));
            glVertex3f(p1.x, p1.y, p1.z);
        }
        glEnd();
    }
}
