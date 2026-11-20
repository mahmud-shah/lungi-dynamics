#pragma once

#include <random>

enum class HoldState {
    None,
    Reflex,
    Player,
    Slipped
};

// How a character holds on to their lungi. Every character can own one.
class Hold {
public:
    void reset(unsigned int seed);
    void update(float dt, float windSpeed, float knotLoad, bool playerWantsGrip);

    HoldState state() const { return state_; }
    bool holding() const { return state_ == HoldState::Reflex || state_ == HoldState::Player; }
    bool justSlipped() const { return justSlipped_; }

    float effectiveness() const { return effectiveness_; }
    float stamina() const { return stamina_; }
    float knotStrengthMultiplier() const;

    static float effectivenessAt(float windSpeed);

private:
    std::mt19937 rng_;
    HoldState state_ = HoldState::None;
    float effectiveness_ = 1.0f;
    float stamina_ = 1.0f;
    bool exhausted_ = false;
    float slipTimer_ = 0.0f;
    bool justSlipped_ = false;
};
