#pragma once

#include <string>
#include <vector>

struct EvalConfig {
    float timeStep;
    int windPreset;
};

struct FrameSample {
    float frameSeconds;
    float physicsMs;
    float maxStretch;
    float meanStretch;
    float knotForce;
    int recoveries;
};

// Automatic test matrix for the report: every combination of physics time
// step and wind preset runs for a fixed time (after a short warm-up) and the
// averages are written to logs/evaluation_<date>.csv.
class Evaluation {
public:
    void start();
    void stop() { running_ = false; }
    bool running() const { return running_; }
    bool update(const FrameSample& sample, int solverIterations);

    const EvalConfig& config() const { return configs_[index_]; }
    int index() const { return index_; }
    int count() const { return static_cast<int>(configs_.size()); }
    float elapsed() const { return elapsed_; }
    const std::string& savedPath() const { return savedPath_; }

private:
    struct Stats {
        int frames = 0;
        float time = 0.0f;
        float worstFrame = 0.0f;
        float physicsMs = 0.0f;
        float maxStretch = 0.0f;
        float meanStretch = 0.0f;
        float knotForce = 0.0f;
        int recoveries = 0;
    };

    void finishConfig(int solverIterations);
    void writeFile();

    std::vector<EvalConfig> configs_;
    std::vector<std::string> rows_;
    int index_ = 0;
    float elapsed_ = 0.0f;
    Stats stats_;
    bool running_ = false;
    std::string savedPath_;
};

std::string timestampString();
bool appendRunLog(const std::string& line);
std::string saveScreenshot(int width, int height);
