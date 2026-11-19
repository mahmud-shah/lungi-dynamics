#pragma once

#include <random>
#include <vector>

#include "Vec3.h"

struct WindPreset {
    const char* name;
    float baseSpeed;
    float gustSpeed;
    float storm;
};

class Wind {
public:
    Wind();

    void setPreset(int index);
    int presetIndex() const { return presetIndex_; }
    static int presetCount();
    const WindPreset& preset() const;

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

    void reset(unsigned int seed = 20261125u);
    void triggerGust();
    void update(float dt);

    Vec3 velocityAt(const Vec3& point) const;
    Vec3 direction() const;
    float speed() const { return enabled_ ? speed_ : 0.0f; }
    float strongestGust() const { return strongestGust_; }

    void drawStreaks(float brightness) const;

private:
    void updateGust(float dt);
    void updateStreaks(float dt);
    void respawnStreak(size_t i, bool anywhere);
    float randomRange(float lo, float hi);

    std::mt19937 rng_;
    int presetIndex_ = 0;
    bool enabled_ = true;

    float time_ = 0.0f;
    float speed_ = 0.0f;
    float heading_ = 0.0f;
    float strongestGust_ = 0.0f;

    float gustAge_ = -1.0f;
    float gustPeak_ = 0.0f;
    float gustLevel_ = 0.0f;
    float timeToNextGust_ = 2.0f;

    std::vector<Vec3> streaks_;
};
