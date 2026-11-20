#pragma once

#include <random>

enum class Line {
    Calm,
    Worried,
    Panic,
    Updraft,
    Slipped,
    FellDown,
    FlewUp
};

// The character's speech bubble: picks a line for an event and shows it for
// a few seconds. Urgent lines interrupt; others wait for a short cooldown so
// the character does not talk non-stop.
class Speech {
public:
    void reset(unsigned int seed);
    void say(Line line, bool urgent);
    void update(float dt);

    const char* text() const { return text_; }
    float alpha() const;

private:
    std::mt19937 rng_;
    const char* text_ = "";
    float age_ = 99.0f;
    float cooldown_ = 0.0f;
};
