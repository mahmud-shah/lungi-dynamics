#include "Cloth.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

void Cloth::buildHangingSheet(const Vec3& topLeft, float width, float height, int cols, int rows)
{
    cols_ = cols;
    rows_ = rows;
    const int count = cols * rows;

    restPositions_.resize(count);
    inverseMass_.assign(count, 1.0f);

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const float u = static_cast<float>(col) / (cols - 1);
            const float v = static_cast<float>(row) / (rows - 1);
            restPositions_[index(col, row)] = topLeft + Vec3(u * width, -v * height, 0.0f);
        }
    }

    inverseMass_[index(0, 0)] = 0.0f;
    inverseMass_[index(cols - 1, 0)] = 0.0f;

    constraints_.clear();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (col + 1 < cols) {
                addConstraint(index(col, row), index(col + 1, row));
            }
            if (row + 1 < rows) {
                addConstraint(index(col, row), index(col, row + 1));
            }
        }
    }

    reset();
}

void Cloth::addConstraint(int a, int b)
{
    constraints_.push_back({a, b, length(restPositions_[a] - restPositions_[b])});
}

void Cloth::reset()
{
    positions_ = restPositions_;
    predicted_ = restPositions_;
    velocities_.assign(restPositions_.size(), Vec3());
    recoveries_ = 0;
}

void Cloth::setSolverIterations(int iterations)
{
    iterations_ = std::max(1, std::min(60, iterations));
}

void Cloth::step(float dt, const Vec3& gravity)
{
    const size_t count = positions_.size();

    for (size_t i = 0; i < count; ++i) {
        if (inverseMass_[i] == 0.0f) {
            predicted_[i] = positions_[i];
            continue;
        }
        Vec3 v = (velocities_[i] + gravity * dt) * velocityRetention_;
        const float speed = length(v);
        if (speed > maxSpeed_) {
            v *= maxSpeed_ / speed;
        }
        velocities_[i] = v;
        predicted_[i] = positions_[i] + v * dt;
    }

    for (int it = 0; it < iterations_; ++it) {
        solveDistanceConstraints();
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

// PBD distance constraint C = |pa - pb| - L.
// Each end moves along the line between them, split by inverse mass:
//   dpa = -wa / (wa + wb) * C * n,   dpb = +wb / (wa + wb) * C * n
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
        const Vec3 correction = delta * ((len - c.restLength) / (len * wSum));
        predicted_[c.a] += correction * wa;
        predicted_[c.b] -= correction * wb;
    }
}

void Cloth::applyGust(const Vec3& deltaVelocity)
{
    for (size_t i = 0; i < velocities_.size(); ++i) {
        if (inverseMass_[i] != 0.0f) {
            const float variation = 0.7f + 0.3f * std::sin(static_cast<float>(i) * 1.7f);
            velocities_[i] += deltaVelocity * variation;
        }
    }
}

float Cloth::maxStretch() const
{
    float worst = 0.0f;
    for (const DistanceConstraint& c : constraints_) {
        const float ratio = length(positions_[c.a] - positions_[c.b]) / c.restLength - 1.0f;
        worst = std::max(worst, ratio);
    }
    return worst;
}

void Cloth::draw(float brightness) const
{
    const Vec3 thread = Vec3(0.62f, 0.16f, 0.14f) * brightness;
    glColor3f(thread.x, thread.y, thread.z);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (const DistanceConstraint& c : constraints_) {
        glVertex3f(positions_[c.a].x, positions_[c.a].y, positions_[c.a].z);
        glVertex3f(positions_[c.b].x, positions_[c.b].y, positions_[c.b].z);
    }
    glEnd();
    glLineWidth(1.0f);

    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < positions_.size(); ++i) {
        if (inverseMass_[i] == 0.0f) {
            glColor3f(0.15f, 0.15f, 0.15f);
        } else {
            glColor3f(0.95f * brightness, 0.90f * brightness, 0.80f * brightness);
        }
        glVertex3f(positions_[i].x, positions_[i].y, positions_[i].z);
    }
    glEnd();
    glPointSize(1.0f);
}
