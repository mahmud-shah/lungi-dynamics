#include "Wind.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

namespace {

const WindPreset kPresets[] = {
    {"Mridu Batash", 1.5f, 2.0f, 0.0f, 0.0f, 0.0f},
    {"Jhor Asche!", 4.0f, 5.0f, 0.35f, 0.15f, 6.0f},
    {"Kalboishakhi", 6.0f, 8.0f, 0.7f, 0.45f, 13.0f},
    {"Nor'wester Mode", 8.0f, 12.0f, 1.0f, 0.50f, 16.0f},
};
const int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

const float kBaseHeading = -1.25f;
const float kGustRise = 0.5f;
const float kGustHold = 0.8f;
const float kUpdraftHold = 2.4f;
const float kGustFall = 1.4f;

const int kStreakCount = 160;
const float kStreakHalfWidth = 4.0f;
const float kStreakTop = 3.0f;

float smoothstep01(float t)
{
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

Wind::Wind()
    : rng_(20261125u)
{
    streaks_.resize(kStreakCount);
    reset();
}

int Wind::presetCount()
{
    return kPresetCount;
}

const char* Wind::presetName(int index)
{
    return kPresets[std::max(0, std::min(kPresetCount - 1, index))].name;
}

const WindPreset& Wind::preset() const
{
    return kPresets[presetIndex_];
}

void Wind::setPreset(int index)
{
    presetIndex_ = std::max(0, std::min(kPresetCount - 1, index));
}

float Wind::randomRange(float lo, float hi)
{
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng_);
}

void Wind::reset(unsigned int seed)
{
    rng_.seed(seed);
    time_ = 0.0f;
    speed_ = preset().baseSpeed;
    heading_ = kBaseHeading;
    strongestGust_ = 0.0f;
    gustAge_ = -1.0f;
    gustLevel_ = 0.0f;
    timeToNextGust_ = 2.0f;
    gustIsUpdraft_ = false;
    for (size_t i = 0; i < streaks_.size(); ++i) {
        respawnStreak(i, true);
    }
}

void Wind::triggerGust()
{
    gustAge_ = 0.0f;
    gustPeak_ = 1.0f;
    gustIsUpdraft_ = false;
    gustStarted_ = true;
}

void Wind::triggerUpdraft()
{
    gustAge_ = 0.0f;
    gustPeak_ = 1.0f;
    gustIsUpdraft_ = true;
    gustStarted_ = true;
}

bool Wind::consumeGustStart()
{
    const bool started = gustStarted_;
    gustStarted_ = false;
    return started;
}

// Thunderstorm gusts are not always sideways: some carry rising air. The
// rising part follows the same gust envelope as the gust itself.
float Wind::updraftSpeed() const
{
    if (!enabled_ || !gustIsUpdraft_) {
        return 0.0f;
    }
    return preset().updraftSpeed * gustLevel_;
}

void Wind::updateGust(float dt)
{
    if (gustAge_ < 0.0f) {
        timeToNextGust_ -= dt;
        if (timeToNextGust_ <= 0.0f) {
            gustAge_ = 0.0f;
            gustPeak_ = randomRange(0.45f, 1.0f);
            gustIsUpdraft_ = randomRange(0.0f, 1.0f) < preset().updraftChance;
            gustStarted_ = gustPeak_ > 0.7f;
        }
        gustLevel_ = 0.0f;
        return;
    }

    // Thunderstorm updrafts are steady columns of rising air, so they last
    // longer than an ordinary sideways gust.
    const float hold = gustIsUpdraft_ ? kUpdraftHold : kGustHold;
    gustAge_ += dt;
    if (gustAge_ < kGustRise) {
        gustLevel_ = smoothstep01(gustAge_ / kGustRise);
    } else if (gustAge_ < kGustRise + hold) {
        gustLevel_ = 1.0f;
    } else if (gustAge_ < kGustRise + hold + kGustFall) {
        gustLevel_ = 1.0f - smoothstep01((gustAge_ - kGustRise - hold) / kGustFall);
    } else {
        gustAge_ = -1.0f;
        gustLevel_ = 0.0f;
        timeToNextGust_ = randomRange(1.5f, 4.5f);
    }
    gustLevel_ *= gustPeak_;
}

// Wind speed = steady base with a slow wobble, plus a gust envelope
// (rise, hold, fall) that arrives at random intervals. The heading also
// wanders slowly so the cloth is not pushed from one fixed side.
void Wind::update(float dt)
{
    time_ += dt;
    updateGust(dt);

    const WindPreset& p = preset();
    const float wobble = 1.0f + 0.12f * std::sin(1.7f * time_) + 0.08f * std::sin(3.1f * time_ + 1.0f);
    speed_ = p.baseSpeed * wobble + p.gustSpeed * gustLevel_;
    heading_ = kBaseHeading + 0.30f * std::sin(0.23f * time_) + 0.15f * std::sin(0.61f * time_);

    if (enabled_) {
        strongestGust_ = std::max(strongestGust_, speed_);
    }
    updateStreaks(dt);
}

Vec3 Wind::direction() const
{
    return Vec3(std::cos(heading_), 0.0f, std::sin(heading_));
}

// Small travelling waves in space and time make neighbouring parts of the
// cloth feel slightly different air, which is what makes cloth flutter.
Vec3 Wind::velocityAt(const Vec3& p) const
{
    if (!enabled_) {
        return Vec3();
    }
    const float t = time_;
    const float ripple = 1.0f + 0.20f * std::sin(1.3f * p.x + 2.1f * t) * std::cos(1.7f * p.y - 1.4f * t)
                              + 0.15f * std::sin(2.3f * p.z + 3.3f * t);
    const float lift = 0.15f * std::sin(2.0f * p.y + 2.7f * t + p.x);
    return direction() * (speed_ * ripple) + Vec3(0.0f, lift * speed_ + updraftSpeed(), 0.0f);
}

void Wind::respawnStreak(size_t i, bool anywhere)
{
    const Vec3 dir = direction();
    Vec3 p(randomRange(-kStreakHalfWidth, kStreakHalfWidth), randomRange(0.1f, kStreakTop),
           randomRange(-kStreakHalfWidth, kStreakHalfWidth));
    if (!anywhere) {
        p = p - dir * (dot(p, dir) + kStreakHalfWidth);
    }
    streaks_[i] = p;
}

void Wind::updateStreaks(float dt)
{
    for (size_t i = 0; i < streaks_.size(); ++i) {
        streaks_[i] += velocityAt(streaks_[i]) * dt;
        const Vec3& p = streaks_[i];
        if (std::fabs(p.x) > kStreakHalfWidth || std::fabs(p.z) > kStreakHalfWidth || p.y < 0.0f || p.y > kStreakTop) {
            respawnStreak(i, false);
        }
    }
}

void Wind::drawStreaks(float brightness) const
{
    const float s = speed();
    if (s < 0.5f) {
        return;
    }
    const float alpha = std::min(0.45f, 0.05f + s * 0.025f);
    const float tailSeconds = 0.05f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (const Vec3& p : streaks_) {
        const Vec3 tail = p - velocityAt(p) * tailSeconds;
        glColor4f(brightness, brightness, brightness, 0.0f);
        glVertex3f(tail.x, tail.y, tail.z);
        glColor4f(brightness, brightness, brightness, alpha);
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
