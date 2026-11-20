#pragma once

#include "Vec3.h"

class Environment {
public:
    Environment();

    void setStorm(float storm);
    void setFlash(float flash) { flash_ = flash; }
    float storm() const { return storm_; }

    Vec3 lightDir() const { return lightDir_; }
    float brightness() const;

    void drawSky() const;
    void drawSun(const Vec3& eye) const;
    void applyFog() const;

private:
    struct SkyColors {
        Vec3 top;
        Vec3 middle;
        Vec3 horizon;
    };

    SkyColors skyColors() const;

    float storm_ = 0.0f;
    float flash_ = 0.0f;
    Vec3 lightDir_;
};
