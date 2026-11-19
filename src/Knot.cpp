#include "Knot.h"

#include <cmath>

namespace {

const float kSmoothingSeconds = 0.15f;
const float kWearRate = 0.35f;
const float kStrainingLoad = 0.7f;
const float kMaxWaistDrop = 0.10f;

} // namespace

void Knot::reset()
{
    load_ = 0.0f;
    health_ = 1.0f;
    failed_ = false;
}

// load = smoothed knot force / knot strength, so 1.0 means "at the limit".
// While load > 1 the knot wears out at a rate proportional to the overload:
//   health -= (load - 1) * kWearRate * dt
// It fails when health reaches zero, so one short spike is survivable but a
// long strong gust is not.
void Knot::update(float dt, float loadNewtons)
{
    if (failed_) {
        return;
    }

    const float raw = loadNewtons / strength_;
    const float blend = 1.0f - std::exp(-dt / kSmoothingSeconds);
    load_ += (raw - load_) * blend;

    if (load_ > 1.0f) {
        health_ -= (load_ - 1.0f) * kWearRate * dt;
        if (health_ <= 0.0f) {
            health_ = 0.0f;
            failed_ = true;
        }
    }
}

// As the knot wears, the waistline slides down, so the player can see it
// loosening before it lets go.
float Knot::waistDrop() const
{
    return (1.0f - health_) * kMaxWaistDrop;
}

const char* Knot::status() const
{
    if (failed_) {
        return "FAILED";
    }
    if (load_ >= 1.0f) {
        return "SLIPPING!";
    }
    if (load_ >= kStrainingLoad) {
        return "Straining";
    }
    return "Tight";
}
