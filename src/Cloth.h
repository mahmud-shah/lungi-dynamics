#pragma once

#include <vector>

#include "Vec3.h"

class Cloth {
public:
    void buildHangingSheet(const Vec3& topLeft, float width, float height, int cols, int rows);
    void reset();

    void step(float dt, const Vec3& gravity);
    void applyGust(const Vec3& deltaVelocity);
    void draw(float brightness) const;

    void setSolverIterations(int iterations);
    int solverIterations() const { return iterations_; }

    int particleCount() const { return static_cast<int>(positions_.size()); }
    int constraintCount() const { return static_cast<int>(constraints_.size()); }
    int recoveries() const { return recoveries_; }
    float maxStretch() const;

private:
    struct DistanceConstraint {
        int a;
        int b;
        float restLength;
    };

    int index(int col, int row) const { return row * cols_ + col; }
    void addConstraint(int a, int b);
    void solveDistanceConstraints();

    int cols_ = 0;
    int rows_ = 0;
    int iterations_ = 20;
    float velocityRetention_ = 0.998f;
    float maxSpeed_ = 25.0f;
    float groundHeight_ = 0.01f;

    std::vector<Vec3> restPositions_;
    std::vector<Vec3> positions_;
    std::vector<Vec3> predicted_;
    std::vector<Vec3> velocities_;
    std::vector<float> inverseMass_;
    std::vector<DistanceConstraint> constraints_;
    int recoveries_ = 0;
};
