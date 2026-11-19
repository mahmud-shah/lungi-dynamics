#pragma once

#include <vector>

#include "Vec3.h"

class Environment {
public:
    Environment();

    void setStorm(float storm);
    float storm() const { return storm_; }

    Vec3 lightDir() const { return lightDir_; }
    float brightness() const;

    void drawSky() const;
    void drawSun(const Vec3& eye) const;
    void applyFog() const;
    void drawGround() const;

private:
    struct SkyColors {
        Vec3 top;
        Vec3 middle;
        Vec3 horizon;
    };

    SkyColors skyColors() const;

    float storm_ = 0.0f;
    Vec3 lightDir_;
    std::vector<Vec3> groundTileColors_;
};
