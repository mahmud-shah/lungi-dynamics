#include "Game.h"

#include <algorithm>

namespace {

const Difficulty kDifficulties[] = {
    {"Easy", 26.0f, 0.8f},
    {"Normal", 20.0f, 1.0f},
    {"Hard", 15.0f, 1.3f},
};
const int kDifficultyCount = sizeof(kDifficulties) / sizeof(kDifficulties[0]);

const float kStageStartTimes[] = {0.0f, 8.0f, 20.0f, 40.0f};
const int kStageCount = 4;

const char* const kAnnouncements[] = {
    "Mridu Batash... a lovely Boishakh afternoon.",
    "Jhor Asche! Hold on to your lungi!",
    "KALBOISHAKHI! The sky has gone dark!",
    "NOR'WESTER MODE! Pray for that knot!",
};
const float kAnnouncementSeconds = 3.0f;

const ScoreTier kTiers[] = {
    {"Lungi Newbie", {"Mama, gero ta aro shokto kore bandho!",
                      "The lungi left before the storm even arrived."}},
    {"Gaye Joth Ase", {"Not bad! The village tea stall is impressed.",
                       "Gaye joth ase, kintu gero te noy."}},
    {"Kalboishakhi Warrior", {"You held on like a true Kalboishakhi warrior!",
                              "Even the banana tree is jealous of that grip."}},
    {"Lungi Legend", {"Lungi Legend! They will talk about this at the haat.",
                      "The storm gave up before your knot did."}},
};
const int kTierThresholds[] = {0, 300, 450, 600};
const int kTierCount = 4;

const float kGripDrainPerSecond = 0.30f;
const float kGripRefillPerSecond = 0.15f;
const float kGripRecoverLevel = 0.25f;
const float kGripStrengthBonus = 1.6f;

const float kResultsDelay = 2.0f;

} // namespace

void Game::showTitle()
{
    state_ = GameState::Title;
}

void Game::startRun()
{
    state_ = GameState::Playing;
    ++runNumber_;
    survivalTime_ = 0.0f;
    strongestGust_ = 0.0f;
    dropIntegral_ = 0.0f;
    timeSinceFailure_ = 0.0f;
    lastStage_ = 0;
    announcementTime_ = 0.0f;
    stamina_ = 1.0f;
    gripping_ = false;
    gripExhausted_ = false;
}

int Game::difficultyCount()
{
    return kDifficultyCount;
}

void Game::cycleDifficulty(int step)
{
    difficultyIndex_ = (difficultyIndex_ + step + kDifficultyCount) % kDifficultyCount;
}

const Difficulty& Game::difficulty() const
{
    return kDifficulties[difficultyIndex_];
}

// Grip stamina drains while Space is held and refills when it is released.
// Once it hits zero the hands slip off and cannot grab again until it has
// refilled to kGripRecoverLevel, so the player has to save it for gusts.
void Game::update(float dt, bool knotFailed, float windSpeed, float waistDrop, bool gripHeld)
{
    if (state_ != GameState::Playing) {
        return;
    }

    if (knotFailed) {
        gripping_ = false;
        timeSinceFailure_ += dt;
        if (timeSinceFailure_ >= kResultsDelay) {
            state_ = GameState::Results;
        }
        return;
    }

    survivalTime_ += dt;
    strongestGust_ = std::max(strongestGust_, windSpeed);
    dropIntegral_ += waistDrop * dt;

    if (gripExhausted_ && stamina_ >= kGripRecoverLevel) {
        gripExhausted_ = false;
    }
    gripping_ = gripHeld && !gripExhausted_ && stamina_ > 0.0f;
    if (gripping_) {
        stamina_ -= kGripDrainPerSecond * dt;
        if (stamina_ <= 0.0f) {
            stamina_ = 0.0f;
            gripExhausted_ = true;
            gripping_ = false;
        }
    } else {
        stamina_ = std::min(1.0f, stamina_ + kGripRefillPerSecond * dt);
    }

    const int stage = stormStage();
    if (stage != lastStage_) {
        lastStage_ = stage;
        announcementTime_ = 0.0f;
    }
    announcementTime_ += dt;
}

int Game::stormStage() const
{
    int stage = 0;
    for (int i = 0; i < kStageCount; ++i) {
        if (survivalTime_ >= kStageStartTimes[i]) {
            stage = i;
        }
    }
    return stage;
}

const char* Game::announcement() const
{
    return kAnnouncements[lastStage_];
}

float Game::announcementAlpha() const
{
    if (state_ != GameState::Playing || announcementTime_ > kAnnouncementSeconds) {
        return 0.0f;
    }
    const float fadeOut = kAnnouncementSeconds - announcementTime_;
    return std::min(1.0f, std::min(announcementTime_ * 4.0f, fadeOut * 2.0f));
}

float Game::knotStrengthMultiplier() const
{
    return gripping_ ? kGripStrengthBonus : 1.0f;
}

// Average waist drop over the run, in centimetres: how far the knot was
// dragged down while it was holding.
float Game::displacementPenalty() const
{
    if (survivalTime_ <= 0.0f) {
        return 0.0f;
    }
    return 100.0f * dropIntegral_ / survivalTime_ * 3.0f;
}

// score = (10 * survival seconds + 4 * strongest gust survived (m/s)
//          - displacement penalty) * difficulty multiplier
int Game::score() const
{
    const float raw = 10.0f * survivalTime_ + 4.0f * strongestGust_ - displacementPenalty();
    return std::max(0, static_cast<int>(raw * difficulty().scoreMultiplier + 0.5f));
}

const ScoreTier& Game::tier() const
{
    int index = 0;
    for (int i = 0; i < kTierCount; ++i) {
        if (score() >= kTierThresholds[i]) {
            index = i;
        }
    }
    return kTiers[index];
}

const char* Game::tierMessage() const
{
    return tier().messages[runNumber_ % 2];
}
