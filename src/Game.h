#pragma once

enum class GameState {
    Title,
    Playing,
    Results
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

class Game {
public:
    GameState state() const { return state_; }
    void showTitle();
    void startRun();
    void update(float dt, bool knotFailed, float windSpeed, float waistDrop, bool gripHeld);

    static int difficultyCount();
    void cycleDifficulty(int step);
    const Difficulty& difficulty() const;

    int stormStage() const;
    const char* announcement() const;
    float announcementAlpha() const;

    bool gripping() const { return gripping_; }
    float gripStamina() const { return stamina_; }
    float knotStrengthMultiplier() const;

    int runNumber() const { return runNumber_; }
    float survivalTime() const { return survivalTime_; }
    float strongestGustSurvived() const { return strongestGust_; }
    float displacementPenalty() const;
    int score() const;
    const ScoreTier& tier() const;
    const char* tierMessage() const;
    bool resultsReady() const { return state_ == GameState::Results; }

private:
    GameState state_ = GameState::Title;
    int difficultyIndex_ = 1;
    int runNumber_ = 0;

    float survivalTime_ = 0.0f;
    float strongestGust_ = 0.0f;
    float dropIntegral_ = 0.0f;
    float timeSinceFailure_ = 0.0f;

    int lastStage_ = 0;
    float announcementTime_ = 0.0f;

    float stamina_ = 1.0f;
    bool gripping_ = false;
    bool gripExhausted_ = false;
};
