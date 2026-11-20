#include "Body.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

namespace {

constexpr Vec3 kSkin(0.96f, 0.76f, 0.58f);
constexpr Vec3 kVest(0.95f, 0.93f, 0.86f);
constexpr Vec3 kPants(0.33f, 0.42f, 0.27f);
constexpr Vec3 kHair(0.16f, 0.12f, 0.10f);
constexpr Vec3 kEye(0.10f, 0.08f, 0.08f);
constexpr Vec3 kEyeWhite(0.98f, 0.97f, 0.94f);
constexpr Vec3 kMouth(0.40f, 0.10f, 0.10f);

// Each mesh piece is tagged with the poses and faces it belongs to. Every
// frame one arm bit and one face bit are switched on, and only pieces that
// share a bit with them are drawn.
const unsigned int kArmsDown = 1u << 0;
const unsigned int kArmsGrip = 1u << 1;
const unsigned int kArmsFlail = 1u << 2;
const unsigned int kFaceCalm = 1u << 3;
const unsigned int kFaceWorried = 1u << 4;
const unsigned int kFacePanic = 1u << 5;
const unsigned int kFaceShock = 1u << 6;
const unsigned int kAlways = 0xFFFFu;

const int kRingSegments = 24;
const int kHemisphereStacks = 8;

} // namespace

Body::Body()
{
    addCapsule(Vec3(-0.11f, 0.86f, 0.0f), Vec3(-0.11f, 0.37f, 0.0f), 0.095f, kPants, true, kAlways);
    addCapsule(Vec3(0.11f, 0.86f, 0.0f), Vec3(0.11f, 0.37f, 0.0f), 0.095f, kPants, true, kAlways);
    addCapsule(Vec3(-0.11f, 0.38f, 0.0f), Vec3(-0.11f, 0.10f, 0.0f), 0.078f, kSkin, true, kAlways);
    addCapsule(Vec3(0.11f, 0.38f, 0.0f), Vec3(0.11f, 0.10f, 0.0f), 0.078f, kSkin, true, kAlways);
    addCapsule(Vec3(-0.10f, 0.86f, 0.0f), Vec3(0.10f, 0.86f, 0.0f), 0.15f, kPants, true, kAlways);
    addCapsule(Vec3(0.0f, 0.98f, 0.0f), Vec3(0.0f, 1.32f, 0.0f), 0.21f, kVest, true, kAlways);

    for (float side = -1.0f; side <= 1.0f; side += 2.0f) {
        const Vec3 shoulder(side * 0.27f, 1.36f, 0.0f);
        addCapsule(shoulder, Vec3(side * 0.41f, 1.02f, 0.06f), 0.06f, kSkin, false, kArmsDown);

        const Vec3 gripElbow(side * 0.34f, 1.12f, 0.10f);
        const Vec3 gripHand(side * 0.10f, 0.99f, 0.30f);
        addCapsule(shoulder, gripElbow, 0.06f, kSkin, false, kArmsGrip);
        addCapsule(gripElbow, gripHand, 0.055f, kSkin, false, kArmsGrip);
        addCapsule(gripHand, gripHand, 0.058f, kSkin, false, kArmsGrip);

        const Vec3 flailElbow(side * 0.44f, 1.55f, 0.05f);
        const Vec3 flailHand(side * 0.36f, 1.86f, 0.12f);
        addCapsule(shoulder, flailElbow, 0.06f, kSkin, false, kArmsFlail);
        addCapsule(flailElbow, flailHand, 0.055f, kSkin, false, kArmsFlail);
        addCapsule(flailHand, flailHand, 0.058f, kSkin, false, kArmsFlail);
    }

    addCapsule(Vec3(0.0f, 1.66f, 0.0f), Vec3(0.0f, 1.66f, 0.0f), 0.23f, kSkin, true, kAlways);
    addCapsule(Vec3(0.0f, 1.74f, -0.03f), Vec3(0.0f, 1.74f, -0.03f), 0.215f, kHair, false, kAlways);
    addFace();

    waistCenter_ = Vec3(0.0f, 0.95f, 0.0f);
    waistRadiusX_ = 0.27f;
    waistRadiusZ_ = 0.235f;
    setPose(ArmPose::Down, Face::Calm);
}

// The face sits on the front of the head sphere (centre y = 1.66, radius
// 0.23, facing +z). Each expression is a small set of balls and capsules:
// calm = dot eyes and a smile, worried = slanted brows and a flat mouth,
// panic/shock = big white eyes, raised brows and an open mouth.
void Body::addFace()
{
    for (float side = -1.0f; side <= 1.0f; side += 2.0f) {
        const Vec3 eye(side * 0.08f, 1.69f, 0.205f);
        addCapsule(eye, eye, 0.028f, kEye, false, kFaceCalm | kFaceWorried);

        const Vec3 bigEye(side * 0.085f, 1.69f, 0.19f);
        addCapsule(bigEye, bigEye, 0.045f, kEyeWhite, false, kFacePanic | kFaceShock);
        const Vec3 pupil(side * 0.085f, 1.69f, 0.228f);
        addCapsule(pupil, pupil, 0.018f, kEye, false, kFacePanic | kFaceShock);

        addCapsule(Vec3(side * 0.13f, 1.75f, 0.185f), Vec3(side * 0.04f, 1.775f, 0.21f), 0.012f, kHair, false,
                   kFaceWorried);
        addCapsule(Vec3(side * 0.13f, 1.79f, 0.17f), Vec3(side * 0.04f, 1.795f, 0.20f), 0.012f, kHair, false,
                   kFacePanic | kFaceShock);
    }

    addCapsule(Vec3(-0.055f, 1.59f, 0.207f), Vec3(-0.02f, 1.574f, 0.217f), 0.010f, kMouth, false, kFaceCalm);
    addCapsule(Vec3(-0.02f, 1.574f, 0.217f), Vec3(0.02f, 1.574f, 0.217f), 0.010f, kMouth, false, kFaceCalm);
    addCapsule(Vec3(0.02f, 1.574f, 0.217f), Vec3(0.055f, 1.59f, 0.207f), 0.010f, kMouth, false, kFaceCalm);
    addCapsule(Vec3(-0.04f, 1.58f, 0.214f), Vec3(0.04f, 1.58f, 0.214f), 0.011f, kMouth, false, kFaceWorried);
    addCapsule(Vec3(0.0f, 1.575f, 0.205f), Vec3(0.0f, 1.575f, 0.205f), 0.034f, kMouth, false, kFacePanic);
    addCapsule(Vec3(0.0f, 1.57f, 0.20f), Vec3(0.0f, 1.57f, 0.20f), 0.045f, kMouth, false, kFaceShock);
}

void Body::setPose(ArmPose arms, Face face)
{
    const unsigned int armBits[] = {kArmsDown, kArmsGrip, kArmsFlail};
    const unsigned int faceBits[] = {kFaceCalm, kFaceWorried, kFacePanic, kFaceShock};
    visible_ = armBits[static_cast<int>(arms)] | faceBits[static_cast<int>(face)];
}

void Body::addCapsule(const Vec3& a, const Vec3& b, float radius, const Vec3& color, bool collides,
                      unsigned int showIn)
{
    const Capsule c{a, b, radius, color};
    if (collides) {
        colliders_.push_back(c);
        collisionTop_ = std::max(collisionTop_, std::max(a.y, b.y) + radius);
    }
    buildCapsuleMesh(c, showIn);
}

// A capsule is a sphere swept along a segment, so it is drawn as a sphere
// split at the equator: the upper half is centred on b, the lower half on a.
void Body::buildCapsuleMesh(const Capsule& c, unsigned int showIn)
{
    const float pi = 3.14159265f;
    const Vec3 axisVec = c.b - c.a;
    const Vec3 axis = length(axisVec) > 1e-5f ? normalized(axisVec) : Vec3(0.0f, 1.0f, 0.0f);
    const Vec3 helper = std::fabs(axis.y) < 0.9f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
    const Vec3 side = normalized(cross(helper, axis));
    const Vec3 front = cross(axis, side);

    const unsigned int base = static_cast<unsigned int>(positions_.size());
    const int stacks = 2 * kHemisphereStacks;
    const unsigned int ring = kRingSegments + 1;
    int ringCount = 0;

    for (int k = 0; k <= stacks + 1; ++k) {
        const bool upper = k <= kHemisphereStacks;
        const int stack = upper ? k : k - 1;
        const float phi = pi * stack / stacks;
        const Vec3 center = upper ? c.b : c.a;
        for (int s = 0; s <= kRingSegments; ++s) {
            const float theta = 2.0f * pi * s / kRingSegments;
            const Vec3 n = axis * std::cos(phi)
                         + side * (std::sin(phi) * std::cos(theta))
                         + front * (std::sin(phi) * std::sin(theta));
            normals_.push_back(n);
            positions_.push_back(center + n * c.radius);
        }
        ++ringCount;
    }

    const size_t firstIndex = indices_.size();
    for (int k = 0; k < ringCount - 1; ++k) {
        for (int s = 0; s < kRingSegments; ++s) {
            const unsigned int i0 = base + k * ring + s;
            const unsigned int i1 = i0 + ring;
            indices_.push_back(i0);
            indices_.push_back(i0 + 1);
            indices_.push_back(i1);
            indices_.push_back(i0 + 1);
            indices_.push_back(i1 + 1);
            indices_.push_back(i1);
        }
    }
    ranges_.push_back({firstIndex, indices_.size() - firstIndex, c.color, showIn});
    rampCoords_.resize(positions_.size());
}

// Closest point on the capsule's core segment: t = clamp((p - a).(b - a) / |b - a|^2, 0, 1).
// If the point is closer to that core than radius + thickness, project it
// back onto the surface along the line from the core.
bool Body::pushOut(Vec3& point, float thickness, Vec3& normal) const
{
    if (point.y > collisionTop_ + thickness) {
        return false;
    }

    bool hit = false;
    for (const Capsule& c : colliders_) {
        const Vec3 ab = c.b - c.a;
        const float abLengthSq = dot(ab, ab);
        const float t = abLengthSq > 1e-8f ? clamp01(dot(point - c.a, ab) / abLengthSq) : 0.0f;
        const Vec3 closest = c.a + ab * t;
        const Vec3 offset = point - closest;
        const float minDistance = c.radius + thickness;
        const float distSq = dot(offset, offset);

        if (distSq < minDistance * minDistance) {
            const float dist = std::sqrt(distSq);
            const Vec3 n = dist > 1e-6f ? offset / dist : Vec3(0.0f, 0.0f, 1.0f);
            point = closest + n * minDistance;
            normal = n;
            hit = true;
        }
    }
    return hit;
}

void Body::draw(const ToonRamp& ramp, const Vec3& lightDir, float brightness)
{
    for (size_t i = 0; i < normals_.size(); ++i) {
        rampCoords_[i] = ToonRamp::coordinate(normals_[i], lightDir);
    }

    ramp.bind();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &positions_[0].x);
    glTexCoordPointer(1, GL_FLOAT, 0, rampCoords_.data());

    for (const MeshRange& r : ranges_) {
        if ((r.showIn & visible_) == 0) {
            continue;
        }
        const Vec3 c = r.color * brightness;
        glColor3f(c.x, c.y, c.z);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(r.indexCount), GL_UNSIGNED_INT,
                       indices_.data() + r.firstIndex);
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    ramp.unbind();
}
