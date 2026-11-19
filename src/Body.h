#pragma once

#include <vector>

#include "ToonRamp.h"
#include "Vec3.h"

struct Capsule {
    Vec3 a;
    Vec3 b;
    float radius;
    Vec3 color;
};

class Body {
public:
    Body();

    bool pushOut(Vec3& point, float thickness, Vec3& normal) const;
    void draw(const ToonRamp& ramp, const Vec3& lightDir, float brightness);

    Vec3 waistCenter() const { return waistCenter_; }
    float waistRadiusX() const { return waistRadiusX_; }
    float waistRadiusZ() const { return waistRadiusZ_; }

private:
    struct MeshRange {
        size_t firstIndex;
        size_t indexCount;
        Vec3 color;
    };

    void addCapsule(const Vec3& a, const Vec3& b, float radius, const Vec3& color, bool collides);
    void buildCapsuleMesh(const Capsule& c);

    std::vector<Capsule> colliders_;
    std::vector<Vec3> positions_;
    std::vector<Vec3> normals_;
    std::vector<float> rampCoords_;
    std::vector<unsigned int> indices_;
    std::vector<MeshRange> ranges_;

    Vec3 waistCenter_;
    float waistRadiusX_ = 0.0f;
    float waistRadiusZ_ = 0.0f;
    float collisionTop_ = 0.0f;
};
