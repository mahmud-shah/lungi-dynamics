#pragma once

#include <vector>

#include "Pattern.h"
#include "ToonRamp.h"
#include "Vec3.h"

class Body;
class Wind;

struct TubeShape {
    Vec3 waistCenter;
    float waistRadiusX;
    float waistRadiusZ;
    float flatRadius;
    float length;
    int around;
    int down;
};

class Cloth {
public:
    void buildTube(const TubeShape& shape);
    void reset();

    void step(float dt, const Vec3& gravity, const Body& body, const Wind& wind);
    void releaseKnot();
    bool knotAttached() const { return knotAttached_; }
    void setWaistDrop(float metres) { waistDrop_ = metres; }
    float knotLoad() const { return knotLoad_; }
    void draw(const ToonRamp& ramp, const FabricTexture& fabric, const Vec3& lightDir, float brightness);

    void setSolverIterations(int iterations);
    int solverIterations() const { return iterations_; }

    int particleCount() const { return static_cast<int>(positions_.size()); }
    int constraintCount() const { return static_cast<int>(constraints_.size()); }
    int recoveries() const { return recoveries_; }
    float maxStretch() const;
    float meanStretch() const;

private:
    struct DistanceConstraint {
        int a;
        int b;
        float restLength;
        float stiffness;
    };

    int index(int col, int row) const { return row * cols_ + ((col % cols_) + cols_) % cols_; }
    void addConstraint(int a, int b, float stiffness);
    void solveDistanceConstraints();
    void solveCollisions(const Body& body, bool applyFriction);
    void accumulateWindForces(const Wind& wind);
    void solveTethers();
    void updateNormals();
    void buildRenderMesh();
    void drawSide(const ToonRamp& ramp, const FabricTexture& fabric, const std::vector<float>& rampCoords,
                  float shade, float brightness);

    int cols_ = 0;
    int rows_ = 0;
    int iterations_ = 15;
    int structuralCount_ = 0;
    float velocityRetention_ = 0.996f;
    float maxSpeed_ = 25.0f;
    float groundHeight_ = 0.01f;
    float thickness_ = 0.02f;
    float friction_ = 0.4f;
    float particleMass_ = 0.0f;

    bool knotAttached_ = true;
    float waistDrop_ = 0.0f;
    float knotLoad_ = 0.0f;

    std::vector<Vec3> restPositions_;
    std::vector<Vec3> naturalPositions_;
    std::vector<Vec3> positions_;
    std::vector<Vec3> predicted_;
    std::vector<Vec3> velocities_;
    std::vector<float> inverseMass_;
    std::vector<Vec3> forces_;
    std::vector<float> tetherLength_;
    std::vector<DistanceConstraint> constraints_;
    int recoveries_ = 0;

    std::vector<Vec3> normals_;
    std::vector<unsigned int> indices_;

    std::vector<Vec3> renderPositions_;
    std::vector<float> renderUVs_;
    std::vector<float> outsideCoords_;
    std::vector<float> insideCoords_;
    std::vector<unsigned int> renderIndices_;
};
