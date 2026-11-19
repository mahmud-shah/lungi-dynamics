#include "Cloth.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

#include "Body.h"
#include "Wind.h"

namespace {

const float kStructuralStiffness = 1.0f;
const float kShearStiffness = 0.8f;
const float kBendStiffness = 0.15f;
const int kFlareRows = 4;
const float kLungiMass = 0.5f;
const float kAirDragCoefficient = 0.6f;

constexpr Vec3 kOutsideColor(0.20f, 0.38f, 0.56f);
constexpr Vec3 kInsideColor(0.13f, 0.25f, 0.38f);

} // namespace

void Cloth::buildTube(const TubeShape& shape)
{
    cols_ = shape.around;
    rows_ = shape.down;
    const int count = cols_ * rows_;
    const float pi = 3.14159265f;

    restPositions_.resize(count);
    inverseMass_.assign(count, 1.0f);

    for (int row = 0; row < rows_; ++row) {
        const float flare = std::min(1.0f, static_cast<float>(row) / kFlareRows);
        const float s = flare * flare * (3.0f - 2.0f * flare);
        const float rx = shape.waistRadiusX + (shape.hemRadius - shape.waistRadiusX) * s;
        const float rz = shape.waistRadiusZ + (shape.hemRadius - shape.waistRadiusZ) * s;
        const float y = shape.waistCenter.y - shape.length * row / (rows_ - 1);

        for (int col = 0; col < cols_; ++col) {
            const float theta = 2.0f * pi * col / cols_;
            restPositions_[index(col, row)] = Vec3(shape.waistCenter.x + std::cos(theta) * rx, y,
                                                   shape.waistCenter.z + std::sin(theta) * rz);
        }
    }

    constraints_.clear();
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            addConstraint(index(col, row), index(col + 1, row), kStructuralStiffness);
            if (row + 1 < rows_) {
                addConstraint(index(col, row), index(col, row + 1), kStructuralStiffness);
            }
        }
    }
    structuralCount_ = static_cast<int>(constraints_.size());

    for (int row = 0; row + 1 < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            addConstraint(index(col, row), index(col + 1, row + 1), kShearStiffness);
            addConstraint(index(col + 1, row), index(col, row + 1), kShearStiffness);
        }
    }

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            addConstraint(index(col, row), index(col + 2, row), kBendStiffness);
            if (row + 2 < rows_) {
                addConstraint(index(col, row), index(col, row + 2), kBendStiffness);
            }
        }
    }

    indices_.clear();
    for (int row = 0; row + 1 < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const unsigned int a = index(col, row);
            const unsigned int b = index(col + 1, row);
            const unsigned int c = index(col, row + 1);
            const unsigned int d = index(col + 1, row + 1);
            indices_.push_back(a);
            indices_.push_back(b);
            indices_.push_back(c);
            indices_.push_back(b);
            indices_.push_back(d);
            indices_.push_back(c);
        }
    }

    tetherLength_.assign(count, 0.0f);
    for (int row = 1; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const float segment = length(restPositions_[index(col, row)] - restPositions_[index(col, row - 1)]);
            tetherLength_[index(col, row)] = tetherLength_[index(col, row - 1)] + segment;
        }
    }

    particleMass_ = kLungiMass / count;
    for (int col = 0; col < cols_; ++col) {
        inverseMass_[index(col, 0)] = 0.0f;
    }
    forces_.resize(count);
    normals_.resize(count);
    outsideCoords_.resize(count);
    insideCoords_.resize(count);

    reset();
}

void Cloth::addConstraint(int a, int b, float stiffness)
{
    constraints_.push_back({a, b, length(restPositions_[a] - restPositions_[b]), stiffness});
}

void Cloth::reset()
{
    positions_ = restPositions_;
    predicted_ = restPositions_;
    velocities_.assign(restPositions_.size(), Vec3());
    recoveries_ = 0;
    knotAttached_ = true;
    waistDrop_ = 0.0f;
    knotLoad_ = 0.0f;
    for (int col = 0; col < cols_; ++col) {
        inverseMass_[index(col, 0)] = 0.0f;
    }
}

void Cloth::setSolverIterations(int iterations)
{
    iterations_ = std::max(1, std::min(60, iterations));
}

void Cloth::step(float dt, const Vec3& gravity, const Body& body, const Wind& wind)
{
    const size_t count = positions_.size();
    accumulateWindForces(wind);

    // Everything the knot holds up: the cloth's weight plus the total wind
    // drag on it. Contact with the legs is ignored, so this is a safe
    // upper bound on the real knot tension.
    Vec3 totalForce = gravity * (particleMass_ * positions_.size());
    for (const Vec3& f : forces_) {
        totalForce += f;
    }
    knotLoad_ = knotAttached_ ? length(totalForce) : 0.0f;

    const float inverseParticleMass = 1.0f / particleMass_;

    for (size_t i = 0; i < count; ++i) {
        if (inverseMass_[i] == 0.0f) {
            predicted_[i] = restPositions_[i] - Vec3(0.0f, waistDrop_, 0.0f);
            continue;
        }
        const Vec3 acceleration = gravity + forces_[i] * inverseParticleMass;
        Vec3 v = (velocities_[i] + acceleration * dt) * velocityRetention_;
        const float speed = length(v);
        if (speed > maxSpeed_) {
            v *= maxSpeed_ / speed;
        }
        velocities_[i] = v;
        predicted_[i] = positions_[i] + v * dt;
    }

    for (int it = 0; it < iterations_; ++it) {
        solveDistanceConstraints();
        solveTethers();
        solveCollisions(body, it == iterations_ - 1);
    }

    for (size_t i = 0; i < count; ++i) {
        Vec3 p = predicted_[i];
        if (!isFinite(p)) {
            p = restPositions_[i];
            velocities_[i] = Vec3();
            positions_[i] = p;
            ++recoveries_;
            continue;
        }
        if (p.y < groundHeight_) {
            p.y = groundHeight_;
        }
        if (inverseMass_[i] != 0.0f) {
            velocities_[i] = (p - positions_[i]) / dt;
        }
        positions_[i] = p;
    }
}

// Long-range tether: a particle may never be farther from its column's knot
// point than the cloth length between them. Only too-far particles are
// pulled back, so folds and slack are still free. This stops the rubbery
// stretching that plain distance constraints allow under strong wind.
void Cloth::solveTethers()
{
    if (!knotAttached_) {
        return;
    }
    for (int row = 1; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const int i = index(col, row);
            const Vec3 anchor = predicted_[index(col, 0)];
            const Vec3 offset = predicted_[i] - anchor;
            const float dist = length(offset);
            if (dist > tetherLength_[i]) {
                predicted_[i] = anchor + offset * (tetherLength_[i] / dist);
            }
        }
    }
}

void Cloth::releaseKnot()
{
    knotAttached_ = false;
    knotLoad_ = 0.0f;
    for (int col = 0; col < cols_; ++col) {
        inverseMass_[index(col, 0)] = 1.0f;
    }
}

// Aerodynamic drag on each triangle, using only the part of the relative air
// velocity that hits the surface head-on:
//   F = c * area * (n . v_rel) * |n . v_rel| * n,   v_rel = v_wind - v_cloth
// A triangle facing the wind catches the full force; one edge-on to it
// catches almost none. Each corner receives a third of the force.
void Cloth::accumulateWindForces(const Wind& wind)
{
    std::fill(forces_.begin(), forces_.end(), Vec3());
    if (wind.speed() <= 0.0f) {
        return;
    }

    for (size_t t = 0; t + 2 < indices_.size(); t += 3) {
        const unsigned int a = indices_[t];
        const unsigned int b = indices_[t + 1];
        const unsigned int c = indices_[t + 2];
        const Vec3 scaledNormal = cross(positions_[b] - positions_[a], positions_[c] - positions_[a]);
        const float doubleArea = length(scaledNormal);
        if (doubleArea < 1e-10f) {
            continue;
        }
        const Vec3 n = scaledNormal / doubleArea;
        const Vec3 center = (positions_[a] + positions_[b] + positions_[c]) / 3.0f;
        const Vec3 clothVelocity = (velocities_[a] + velocities_[b] + velocities_[c]) / 3.0f;
        const Vec3 relative = wind.velocityAt(center) - clothVelocity;
        const float normalSpeed = dot(n, relative);
        const Vec3 force = n * (kAirDragCoefficient * 0.5f * doubleArea * normalSpeed * std::fabs(normalSpeed));
        const Vec3 share = force / 3.0f;
        forces_[a] += share;
        forces_[b] += share;
        forces_[c] += share;
    }
}

// PBD distance constraint C = |pa - pb| - L.
// Each end moves along the line between them, split by inverse mass and
// scaled by the constraint's stiffness k in [0, 1]:
//   dpa = -k * wa / (wa + wb) * C * n,   dpb = +k * wb / (wa + wb) * C * n
void Cloth::solveDistanceConstraints()
{
    for (const DistanceConstraint& c : constraints_) {
        const float wa = inverseMass_[c.a];
        const float wb = inverseMass_[c.b];
        const float wSum = wa + wb;
        if (wSum == 0.0f) {
            continue;
        }
        const Vec3 delta = predicted_[c.b] - predicted_[c.a];
        const float len = length(delta);
        if (len < 1e-6f) {
            continue;
        }
        const Vec3 correction = delta * (c.stiffness * (len - c.restLength) / (len * wSum));
        predicted_[c.a] += correction * wa;
        predicted_[c.b] -= correction * wb;
    }
}

// Friction removes part of the sliding motion along the body surface during
// this step, so the cloth grips the legs instead of sliding like ice.
void Cloth::solveCollisions(const Body& body, bool applyFriction)
{
    for (size_t i = 0; i < predicted_.size(); ++i) {
        if (inverseMass_[i] == 0.0f) {
            continue;
        }
        Vec3 normal;
        if (body.pushOut(predicted_[i], thickness_, normal) && applyFriction) {
            const Vec3 moved = predicted_[i] - positions_[i];
            const Vec3 tangential = moved - normal * dot(moved, normal);
            predicted_[i] -= tangential * friction_;
        }
    }
}

float Cloth::maxStretch() const
{
    float worst = 0.0f;
    for (int i = 0; i < structuralCount_; ++i) {
        const DistanceConstraint& c = constraints_[i];
        worst = std::max(worst, length(positions_[c.a] - positions_[c.b]) / c.restLength - 1.0f);
    }
    return worst;
}

float Cloth::meanStretch() const
{
    float total = 0.0f;
    for (int i = 0; i < structuralCount_; ++i) {
        const DistanceConstraint& c = constraints_[i];
        total += length(positions_[c.a] - positions_[c.b]) / c.restLength - 1.0f;
    }
    return structuralCount_ > 0 ? total / structuralCount_ : 0.0f;
}

void Cloth::updateNormals()
{
    std::fill(normals_.begin(), normals_.end(), Vec3());
    for (size_t t = 0; t + 2 < indices_.size(); t += 3) {
        const unsigned int a = indices_[t];
        const unsigned int b = indices_[t + 1];
        const unsigned int c = indices_[t + 2];
        const Vec3 faceNormal = cross(positions_[b] - positions_[a], positions_[c] - positions_[a]);
        normals_[a] += faceNormal;
        normals_[b] += faceNormal;
        normals_[c] += faceNormal;
    }
    for (Vec3& n : normals_) {
        n = normalized(n);
    }
}

// Two-sided lighting: the outside is drawn with back faces culled and the
// normal as computed; the inside is drawn with front faces culled and the
// normal flipped, so the inner surface is lit from its own side.
void Cloth::draw(const ToonRamp& ramp, const Vec3& lightDir, float brightness)
{
    updateNormals();
    for (size_t i = 0; i < normals_.size(); ++i) {
        outsideCoords_[i] = ToonRamp::coordinate(normals_[i], lightDir);
        insideCoords_[i] = ToonRamp::coordinate(-normals_[i], lightDir);
    }

    ramp.bind();
    glEnable(GL_CULL_FACE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &positions_[0].x);

    const GLsizei indexCount = static_cast<GLsizei>(indices_.size());

    glCullFace(GL_BACK);
    glColor3f(kOutsideColor.x * brightness, kOutsideColor.y * brightness, kOutsideColor.z * brightness);
    glTexCoordPointer(1, GL_FLOAT, 0, outsideCoords_.data());
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices_.data());

    glCullFace(GL_FRONT);
    glColor3f(kInsideColor.x * brightness, kInsideColor.y * brightness, kInsideColor.z * brightness);
    glTexCoordPointer(1, GL_FLOAT, 0, insideCoords_.data());
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices_.data());

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    ramp.unbind();
}
