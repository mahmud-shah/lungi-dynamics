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
const int kTierThresholds[] = {0, 300, 430, 520};
const int kTierCount = 4;

const float kWorriedLoad = 0.5f;
const float kPanicLoad = 0.9f;

const float kOverHeadHeight = 1.9f;
const float kOutcomeDecisionSeconds = 1.5f;
const float kResultsDelay = 2.8f;

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
    highestAfterFailure_ = 0.0f;
    lastStage_ = 0;
    announcementTime_ = 0.0f;
    lastUpdraft_ = false;
    hold_.reset(1000u + static_cast<unsigned int>(runNumber_));
    speech_.reset(2000u + static_cast<unsigned int>(runNumber_));
    face_ = Face::Calm;
    outcome_ = Outcome::None;
    speech_.say(Line::Calm, true);
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

void Game::update(float dt, const GameInput& in)
{
    if (state_ != GameState::Playing) {
        return;
    }
    speech_.update(dt);

    if (in.knotFailed) {
        updateFailure(dt, in);
        return;
    }

    survivalTime_ += dt;
    strongestGust_ = std::max(strongestGust_, in.windSpeed);
    dropIntegral_ += in.waistDrop * dt;

    hold_.update(dt, in.windSpeed, in.knotLoad, in.gripKeyHeld);
    if (hold_.justSlipped()) {
        speech_.say(Line::Slipped, true);
    }
    if (in.updraft && !lastUpdraft_) {
        speech_.say(Line::Updraft, true);
    }
    lastUpdraft_ = in.updraft;

    updateFace(in);

    const int stage = stormStage();
    if (stage != lastStage_) {
        lastStage_ = stage;
        announcementTime_ = 0.0f;
    }
    announcementTime_ += dt;
}

void Game::updateFace(const GameInput& in)
{
    Face next = Face::Calm;
    if (hold_.state() == HoldState::Slipped || in.knotLoad >= kPanicLoad) {
        next = Face::Panic;
    } else if (in.knotLoad >= kWorriedLoad) {
        next = Face::Worried;
    }

    if (next == Face::Panic && face_ != Face::Panic) {
        speech_.say(Line::Panic, false);
    } else if (next == Face::Worried && face_ == Face::Calm) {
        speech_.say(Line::Worried, false);
    }
    face_ = next;
}

// After the knot fails, the lungi's height decides how it left: if it rises
// above the head within 1.5 s it was carried up by an updraft, otherwise it
// dropped. The results screen appears a little later, so the player sees it.
void Game::updateFailure(float dt, const GameInput& in)
{
    face_ = Face::Shock;
    timeSinceFailure_ += dt;
    highestAfterFailure_ = std::max(highestAfterFailure_, in.lungiHeight);

    if (outcome_ == Outcome::None) {
        if (highestAfterFailure_ > kOverHeadHeight) {
            outcome_ = Outcome::FlewUp;
            speech_.say(Line::FlewUp, true);
        } else if (timeSinceFailure_ > kOutcomeDecisionSeconds) {
            outcome_ = Outcome::FellDown;
            speech_.say(Line::FellDown, true);
        }
    }
    if (timeSinceFailure_ >= kResultsDelay) {
        state_ = GameState::Results;
    }
}

ArmPose Game::armPose() const
{
    if (face_ == Face::Shock || hold_.state() == HoldState::Slipped) {
        return ArmPose::Flail;
    }
    return hold_.holding() ? ArmPose::Grip : ArmPose::Down;
}

float Game::knotStrengthMultiplier() const
{
    return hold_.knotStrengthMultiplier();
}

const char* Game::outcomeText() const
{
    switch (outcome_) {
    case Outcome::FlewUp: return "Your lungi flew up over your head!";
    case Outcome::FellDown: return "Your lungi dropped down to your ankles!";
    default: return "";
    }
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

// Average waist drop over the run, in centimetres, times 3: how far the knot
// was dragged down while it was still holding.
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
