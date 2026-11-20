#pragma once

#include <vector>

#include "ToonRamp.h"
#include "Vec3.h"

enum class Face {
    Calm,
    Worried,
    Panic,
    Shock
};

enum class ArmPose {
    Down,
    Grip,
    Flail
};

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
    void setPose(ArmPose arms, Face face);

    Vec3 waistCenter() const { return waistCenter_; }
    float waistRadiusX() const { return waistRadiusX_; }
    float waistRadiusZ() const { return waistRadiusZ_; }

private:
    struct MeshRange {
        size_t firstIndex;
        size_t indexCount;
        Vec3 color;
        unsigned int showIn;
    };

    void addCapsule(const Vec3& a, const Vec3& b, float radius, const Vec3& color, bool collides,
                    unsigned int showIn);
    void addFace();
    void buildCapsuleMesh(const Capsule& c, unsigned int showIn);

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
    unsigned int visible_ = 0;
};
