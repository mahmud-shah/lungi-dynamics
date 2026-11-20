#include "Speech.h"

#include <algorithm>

namespace {

const float kShowSeconds = 2.8f;
const float kCooldownSeconds = 4.0f;

const char* const kCalm[] = {
    "Ahh, a lovely Boishakh afternoon.",
    "Cha khabo naki? The weather is perfect.",
};
const char* const kWorried[] = {
    "Ektu batash beshi lagche...",
    "Gero ta thik ache to?",
    "Hmm, the sky looks angry.",
    "Lungi, please behave today.",
};
const char* const kPanic[] = {
    "Dhoro dhoro dhoro!",
    "Allah bachao, lungi ta jacche!",
    "Not in front of the whole village!",
    "Hold on, my faithful lungi!",
    "Ei batash amar shotru!",
};
const char* const kUpdraft[] = {
    "Uporer dike tanche keno?!",
    "Why is the wind pulling UP?!",
};
const char* const kSlipped[] = {
    "Haath fosshe gelo!",
    "My hands! The wind took my hands!",
};
const char* const kFellDown[] = {
    "Lungi gelo paye! Thank goodness for these pants.",
    "Down it goes... at least I wore pants today.",
};
const char* const kFlewUp[] = {
    "Uronto lungi! It is flying to Sylhet!",
    "Come back! My 20 taka was tucked in there!",
    "There goes my lungi... right over my head!",
};

template <size_t N>
const char* pick(const char* const (&lines)[N], std::mt19937& rng)
{
    std::uniform_int_distribution<size_t> index(0, N - 1);
    return lines[index(rng)];
}

} // namespace

void Speech::reset(unsigned int seed)
{
    rng_.seed(seed);
    text_ = "";
    age_ = 99.0f;
    cooldown_ = 0.0f;
}

void Speech::say(Line line, bool urgent)
{
    if (!urgent && cooldown_ > 0.0f) {
        return;
    }
    switch (line) {
    case Line::Calm:     text_ = pick(kCalm, rng_); break;
    case Line::Worried:  text_ = pick(kWorried, rng_); break;
    case Line::Panic:    text_ = pick(kPanic, rng_); break;
    case Line::Updraft:  text_ = pick(kUpdraft, rng_); break;
    case Line::Slipped:  text_ = pick(kSlipped, rng_); break;
    case Line::FellDown: text_ = pick(kFellDown, rng_); break;
    case Line::FlewUp:   text_ = pick(kFlewUp, rng_); break;
    }
    age_ = 0.0f;
    cooldown_ = kCooldownSeconds;
}

void Speech::update(float dt)
{
    age_ += dt;
    cooldown_ = std::max(0.0f, cooldown_ - dt);
}

float Speech::alpha() const
{
    if (age_ > kShowSeconds) {
        return 0.0f;
    }
    return std::min(1.0f, std::min(age_ * 6.0f, (kShowSeconds - age_) * 2.5f));
}
