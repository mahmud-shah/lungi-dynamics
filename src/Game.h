#pragma once

#include "Body.h"
#include "Hold.h"
#include "Speech.h"

enum class GameState {
    Title,
    Playing,
    Results
};

enum class Outcome {
    None,
    FellDown,
    FlewUp
};

struct Difficulty {
    const char* name;
    float knotStrength;
    float scoreMultiplier;
};

struct ScoreTier {
    const char* name;
    const char* messages[2];
};

struct GameInput {
    bool knotFailed;
    float windSpeed;
    bool updraft;
    float waistDrop;
    float knotLoad;
    float lungiHeight;
    bool gripKeyHeld;
};

class Game {
public:
    GameState state() const { return state_; }
    void showTitle();
    void startRun();
    void update(float dt, const GameInput& in);

    static int difficultyCount();
    void cycleDifficulty(int step);
    const Difficulty& difficulty() const;

    int stormStage() const;
    const char* announcement() const;
    float announcementAlpha() const;

    const Hold& hold() const { return hold_; }
    float knotStrengthMultiplier() const;
    Face face() const { return face_; }
    ArmPose armPose() const;
    const Speech& speech() const { return speech_; }
    Outcome outcome() const { return outcome_; }
    const char* outcomeText() const;

    int runNumber() const { return runNumber_; }
    float survivalTime() const { return survivalTime_; }
    float strongestGustSurvived() const { return strongestGust_; }
    float displacementPenalty() const;
    int score() const;
    const ScoreTier& tier() const;
    const char* tierMessage() const;

private:
    void updateFace(const GameInput& in);
    void updateFailure(float dt, const GameInput& in);

    GameState state_ = GameState::Title;
    int difficultyIndex_ = 1;
    int runNumber_ = 0;

    float survivalTime_ = 0.0f;
    float strongestGust_ = 0.0f;
    float dropIntegral_ = 0.0f;
    float timeSinceFailure_ = 0.0f;
    float highestAfterFailure_ = 0.0f;

    int lastStage_ = 0;
    float announcementTime_ = 0.0f;
    bool lastUpdraft_ = false;

    Hold hold_;
    Speech speech_;
    Face face_ = Face::Calm;
    Outcome outcome_ = Outcome::None;
};
