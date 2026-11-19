#pragma once

#include <vector>

#include "ToonRamp.h"
#include "Vec3.h"

class TestSphere {
public:
    TestSphere(const Vec3& center, float radius, const Vec3& color, int slices, int stacks);

    void draw(const ToonRamp& ramp, const Vec3& lightDir, float brightness);

private:
    Vec3 center_;
    Vec3 color_;
    std::vector<Vec3> positions_;
    std::vector<Vec3> normals_;
    std::vector<float> rampCoords_;
    std::vector<unsigned int> indices_;
};
