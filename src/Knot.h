#pragma once

class Knot {
public:
    void reset();
    void update(float dt, float loadNewtons);

    float load() const { return load_; }
    float health() const { return health_; }
    bool failed() const { return failed_; }
    float waistDrop() const;
    const char* status() const;

    void setStrength(float newtons) { strength_ = newtons; }
    float strength() const { return strength_; }

private:
    float strength_ = 20.0f;
    float load_ = 0.0f;
    float health_ = 1.0f;
    bool failed_ = false;
};
