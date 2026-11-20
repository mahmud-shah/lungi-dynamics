#include "Hold.h"

#include <algorithm>

namespace {

const float kEasyWind = 6.0f;
const float kHardWind = 22.0f;
const float kMinEffectiveness = 0.15f;

const float kReflexLoad = 0.75f;
const float kReflexBonus = 0.35f;
const float kPlayerBonus = 0.9f;

const float kStaminaDrainPerSecond = 0.30f;
const float kStaminaRefillPerSecond = 0.15f;
const float kStaminaRecoverLevel = 0.25f;

const float kSlipStartWind = 10.0f;
const float kSlipWindRange = 25.0f;
const float kSlipSeconds = 0.8f;

} // namespace

void Hold::reset(unsigned int seed)
{
    rng_.seed(seed);
    state_ = HoldState::None;
    effectiveness_ = 1.0f;
    stamina_ = 1.0f;
    exhausted_ = false;
    slipTimer_ = 0.0f;
    justSlipped_ = false;
}

// How much a grip helps at a given wind speed. In a light wind the hands
// hold the knot properly (1.0); in a Nor'wester the cloth is pulled so hard
// that the hands add very little:
//   e(w) = clamp((w_hard - w) / (w_hard - w_easy), 0.15, 1)
float Hold::effectivenessAt(float windSpeed)
{
    const float e = (kHardWind - windSpeed) / (kHardWind - kEasyWind);
    return std::max(kMinEffectiveness, std::min(1.0f, e));
}

// Each step:
//  1. If the hands were torn off, they stay off for kSlipSeconds.
//  2. Space (with stamina) is a strong grip; otherwise the character grabs
//     the knot by reflex whenever it is straining (load > 0.75).
//  3. While holding in wind above 10 m/s the hands may be torn off, with a
//     chance per second of (w - 10) / 25, i.e. 40% per second at 20 m/s.
void Hold::update(float dt, float windSpeed, float knotLoad, bool playerWantsGrip)
{
    justSlipped_ = false;
    effectiveness_ = effectivenessAt(windSpeed);

    if (exhausted_ && stamina_ >= kStaminaRecoverLevel) {
        exhausted_ = false;
    }

    if (slipTimer_ > 0.0f) {
        slipTimer_ -= dt;
        state_ = HoldState::Slipped;
    } else if (playerWantsGrip && !exhausted_ && stamina_ > 0.0f) {
        state_ = HoldState::Player;
    } else if (knotLoad > kReflexLoad) {
        state_ = HoldState::Reflex;
    } else {
        state_ = HoldState::None;
    }

    if (state_ == HoldState::Player) {
        stamina_ -= kStaminaDrainPerSecond * dt;
        if (stamina_ <= 0.0f) {
            stamina_ = 0.0f;
            exhausted_ = true;
        }
    } else {
        stamina_ = std::min(1.0f, stamina_ + kStaminaRefillPerSecond * dt);
    }

    if (holding() && windSpeed > kSlipStartWind) {
        const float chancePerSecond = (windSpeed - kSlipStartWind) / kSlipWindRange;
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        if (roll(rng_) < chancePerSecond * dt) {
            slipTimer_ = kSlipSeconds;
            state_ = HoldState::Slipped;
            justSlipped_ = true;
        }
    }
}

float Hold::knotStrengthMultiplier() const
{
    switch (state_) {
    case HoldState::Player: return 1.0f + kPlayerBonus * effectiveness_;
    case HoldState::Reflex: return 1.0f + kReflexBonus * effectiveness_;
    default: return 1.0f;
    }
}
