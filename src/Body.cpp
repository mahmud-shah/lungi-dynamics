#include "Body.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

namespace {

constexpr Vec3 kSkin(0.96f, 0.76f, 0.58f);
constexpr Vec3 kVest(0.95f, 0.93f, 0.86f);
constexpr Vec3 kShorts(0.98f, 0.76f, 0.22f);
constexpr Vec3 kHair(0.16f, 0.12f, 0.10f);
constexpr Vec3 kEye(0.10f, 0.08f, 0.08f);

const int kRingSegments = 24;
const int kHemisphereStacks = 8;

} // namespace

Body::Body()
{
    addCapsule(Vec3(-0.11f, 0.86f, 0.0f), Vec3(-0.11f, 0.10f, 0.0f), 0.085f, kSkin, true);
    addCapsule(Vec3(0.11f, 0.86f, 0.0f), Vec3(0.11f, 0.10f, 0.0f), 0.085f, kSkin, true);
    addCapsule(Vec3(-0.10f, 0.86f, 0.0f), Vec3(0.10f, 0.86f, 0.0f), 0.15f, kShorts, true);
    addCapsule(Vec3(0.0f, 0.98f, 0.0f), Vec3(0.0f, 1.32f, 0.0f), 0.21f, kVest, true);
    addCapsule(Vec3(-0.27f, 1.36f, 0.0f), Vec3(-0.41f, 1.02f, 0.06f), 0.06f, kSkin, true);
    addCapsule(Vec3(0.27f, 1.36f, 0.0f), Vec3(0.41f, 1.02f, 0.06f), 0.06f, kSkin, true);
    addCapsule(Vec3(0.0f, 1.66f, 0.0f), Vec3(0.0f, 1.66f, 0.0f), 0.23f, kSkin, false);
    addCapsule(Vec3(0.0f, 1.74f, -0.03f), Vec3(0.0f, 1.74f, -0.03f), 0.215f, kHair, false);
    addCapsule(Vec3(-0.08f, 1.69f, 0.205f), Vec3(-0.08f, 1.69f, 0.205f), 0.028f, kEye, false);
    addCapsule(Vec3(0.08f, 1.69f, 0.205f), Vec3(0.08f, 1.69f, 0.205f), 0.028f, kEye, false);

    waistCenter_ = Vec3(0.0f, 0.95f, 0.0f);
    waistRadiusX_ = 0.32f;
    waistRadiusZ_ = 0.245f;
}

void Body::addCapsule(const Vec3& a, const Vec3& b, float radius, const Vec3& color, bool collides)
{
    const Capsule c{a, b, radius, color};
    if (collides) {
        colliders_.push_back(c);
        collisionTop_ = std::max(collisionTop_, std::max(a.y, b.y) + radius);
    }
    buildCapsuleMesh(c);
}

// A capsule is a sphere swept along a segment, so it is drawn as a sphere
// split at the equator: the upper half is centred on b, the lower half on a.
void Body::buildCapsuleMesh(const Capsule& c)
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
    ranges_.push_back({firstIndex, indices_.size() - firstIndex, c.color});
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
        const Vec3 c = r.color * brightness;
        glColor3f(c.x, c.y, c.z);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(r.indexCount), GL_UNSIGNED_INT,
                       indices_.data() + r.firstIndex);
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    ramp.unbind();
}
