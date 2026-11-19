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
const int kGatherRows = 10;
const float kFrontAngle = 1.5707963f;
const float kGatherAmount = 0.75f;
const float kGatherWidth = 0.5f;
const float kLungiMass = 0.5f;
const float kAirDragCoefficient = 0.6f;

const float kPatternTileMetres = 0.32f;
const float kInsideShade = 0.7f;

float smoothstep01(float t)
{
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

// The worn shape follows the Bangladeshi way of tying: the tube is wider than
// the waist, so the extra cloth is pulled to the front, crossed and tucked.
// Here that is modelled by spacing the waist particles unevenly, with many of
// them bunched at the front, and by giving the constraints the rest lengths
// of the flat, unworn tube. The cloth near the front waist is therefore
// compressed and buckles into folds by itself.
void Cloth::buildTube(const TubeShape& shape)
{
    cols_ = shape.around;
    rows_ = shape.down;
    const int count = cols_ * rows_;
    const float twoPi = 6.2831853f;

    restPositions_.resize(count);
    naturalPositions_.resize(count);
    inverseMass_.assign(count, 1.0f);

    std::vector<float> density(cols_);
    float densitySum = 0.0f;
    for (int col = 0; col < cols_; ++col) {
        float offset = twoPi * col / cols_ - kFrontAngle;
        offset = std::atan2(std::sin(offset), std::cos(offset));
        density[col] = 1.0f - kGatherAmount * std::exp(-(offset * offset) / (kGatherWidth * kGatherWidth));
        densitySum += density[col];
    }

    std::vector<float> waistAngle(cols_);
    float cumulative = 0.0f;
    for (int col = 0; col < cols_; ++col) {
        waistAngle[col] = twoPi * cumulative / densitySum;
        cumulative += density[col];
    }

    for (int row = 0; row < rows_; ++row) {
        const float flare = smoothstep01(static_cast<float>(row) / kFlareRows);
        const float ungather = smoothstep01(static_cast<float>(row) / kGatherRows);
        const float y = shape.waistCenter.y - shape.length * row / (rows_ - 1);

        for (int col = 0; col < cols_; ++col) {
            const float flatAngle = twoPi * col / cols_;
            const float theta = waistAngle[col] + (flatAngle - waistAngle[col]) * ungather;
            const float rx = shape.waistRadiusX + (shape.flatRadius - shape.waistRadiusX) * flare;
            const float rz = shape.waistRadiusZ + (shape.flatRadius - shape.waistRadiusZ) * flare;
            restPositions_[index(col, row)] = Vec3(shape.waistCenter.x + std::cos(theta) * rx, y,
                                                   shape.waistCenter.z + std::sin(theta) * rz);
        }
    }

    tetherLength_.assign(count, 0.0f);
    for (int row = 1; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const float segment = length(restPositions_[index(col, row)] - restPositions_[index(col, row - 1)]);
            tetherLength_[index(col, row)] = tetherLength_[index(col, row - 1)] + segment;
        }
    }

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const float flatAngle = twoPi * col / cols_;
            naturalPositions_[index(col, row)] = Vec3(std::cos(flatAngle) * shape.flatRadius,
                                                      -tetherLength_[index(col, row)],
                                                      std::sin(flatAngle) * shape.flatRadius);
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

    particleMass_ = kLungiMass / count;
    for (int col = 0; col < cols_; ++col) {
        inverseMass_[index(col, 0)] = 0.0f;
    }
    forces_.resize(count);
    normals_.resize(count);
    buildRenderMesh();

    reset();
}

void Cloth::addConstraint(int a, int b, float stiffness)
{
    constraints_.push_back({a, b, length(naturalPositions_[a] - naturalPositions_[b]), stiffness});
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
        if (inverseMass_[c.a] == 0.0f && inverseMass_[c.b] == 0.0f) {
            continue;
        }
        worst = std::max(worst, length(positions_[c.a] - positions_[c.b]) / c.restLength - 1.0f);
    }
    return worst;
}

float Cloth::meanStretch() const
{
    float total = 0.0f;
    int counted = 0;
    for (int i = 0; i < structuralCount_; ++i) {
        const DistanceConstraint& c = constraints_[i];
        if (inverseMass_[c.a] == 0.0f && inverseMass_[c.b] == 0.0f) {
            continue;
        }
        total += std::max(0.0f, length(positions_[c.a] - positions_[c.b]) / c.restLength - 1.0f);
        ++counted;
    }
    return counted > 0 ? total / counted : 0.0f;
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

// The render mesh has one extra column: a copy of column 0 placed after the
// last column. Its texture u is 1 full turn instead of 0, so the pattern
// wraps around the tube without being squeezed backwards across the seam.
// The pattern repeats a whole number of times around, so the seam is invisible.
void Cloth::buildRenderMesh()
{
    const int renderCols = cols_ + 1;
    const Vec3 rim = naturalPositions_[index(0, 0)];
    const float flatRadius = std::sqrt(rim.x * rim.x + rim.z * rim.z);
    const float repeatsAround = std::max(1.0f, std::round(6.2831853f * flatRadius / kPatternTileMetres));

    renderPositions_.resize(renderCols * rows_);
    renderUVs_.resize(renderCols * rows_ * 2);
    outsideCoords_.resize(renderCols * rows_);
    insideCoords_.resize(renderCols * rows_);

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < renderCols; ++col) {
            const int r = row * renderCols + col;
            renderUVs_[r * 2 + 0] = repeatsAround * col / cols_;
            renderUVs_[r * 2 + 1] = tetherLength_[index(col, row)] / kPatternTileMetres;
        }
    }

    renderIndices_.clear();
    for (int row = 0; row + 1 < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const unsigned int a = row * renderCols + col;
            const unsigned int b = a + 1;
            const unsigned int c = a + renderCols;
            const unsigned int d = c + 1;
            renderIndices_.push_back(a);
            renderIndices_.push_back(b);
            renderIndices_.push_back(c);
            renderIndices_.push_back(b);
            renderIndices_.push_back(d);
            renderIndices_.push_back(c);
        }
    }
}

// Pass 1 draws the fabric pattern. Pass 2 draws the same triangles again with
// the toon ramp and multiplies it onto the pattern (blend = dst * src), with
// depth test EQUAL so only the pixels from pass 1 are touched. This gives a
// patterned cloth with cel-shaded lighting using only OpenGL 1.1 features.
void Cloth::drawSide(const ToonRamp& ramp, const FabricTexture& fabric, const std::vector<float>& rampCoords,
                     float shade, float brightness)
{
    const GLsizei indexCount = static_cast<GLsizei>(renderIndices_.size());

    fabric.bind();
    glColor3f(shade * brightness, shade * brightness, shade * brightness);
    glTexCoordPointer(2, GL_FLOAT, 0, renderUVs_.data());
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, renderIndices_.data());
    fabric.unbind();

    const GLboolean fogWasOn = glIsEnabled(GL_FOG);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glDepthFunc(GL_EQUAL);
    glDepthMask(GL_FALSE);

    ramp.bind();
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoordPointer(1, GL_FLOAT, 0, rampCoords.data());
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, renderIndices_.data());
    ramp.unbind();

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    if (fogWasOn) {
        glEnable(GL_FOG);
    }
}

// Two-sided lighting: the outside is drawn with back faces culled and the
// normal as computed; the inside is drawn with front faces culled, the
// normal flipped and the cloth a little darker.
void Cloth::draw(const ToonRamp& ramp, const FabricTexture& fabric, const Vec3& lightDir, float brightness)
{
    updateNormals();
    const int renderCols = cols_ + 1;
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < renderCols; ++col) {
            const int sim = index(col, row);
            const int r = row * renderCols + col;
            renderPositions_[r] = positions_[sim];
            outsideCoords_[r] = ToonRamp::coordinate(normals_[sim], lightDir);
            insideCoords_[r] = ToonRamp::coordinate(-normals_[sim], lightDir);
        }
    }

    glEnable(GL_CULL_FACE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &renderPositions_[0].x);

    glCullFace(GL_BACK);
    drawSide(ramp, fabric, outsideCoords_, 1.0f, brightness);
    glCullFace(GL_FRONT);
    drawSide(ramp, fabric, insideCoords_, kInsideShade, brightness);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}
