#pragma once

#include <vector>

#include "ToonRamp.h"
#include "Vec3.h"

struct WaistBand {
    Vec3 center;
    float radiusX;
    float radiusZ;
};

class Knot {
public:
    Knot();

    void reset();
    void update(float dt, float loadNewtons);

    float load() const { return load_; }
    float health() const { return health_; }
    bool failed() const { return failed_; }
    float waistDrop() const;
    const char* status() const;

    void setStrength(float newtons) { strength_ = newtons; }
    float strength() const { return strength_; }

    void draw(const WaistBand& band, const Vec3& clothColor, const ToonRamp& ramp,
              const Vec3& lightDir, float brightness);

private:
    void drawRoll(const WaistBand& band, const Vec3& lightDir);
    void drawTwist(const WaistBand& band, const Vec3& clothColor, const Vec3& lightDir, float brightness);
    void drawBall(const Vec3& center, float radius, const Vec3& lightDir) const;

    float strength_ = 20.0f;
    float load_ = 0.0f;
    float health_ = 1.0f;
    bool failed_ = false;

    std::vector<Vec3> rollPositions_;
    std::vector<float> rollCoords_;
    std::vector<unsigned int> rollIndices_;
};
