#pragma once

// Averages the frame rate over a short window. A per-frame value jumps too
// much to read, and averaging is also what we will log for the evaluation.
class FpsCounter {
public:
    explicit FpsCounter(double windowSeconds = 0.5);

    // Call exactly once per rendered frame.
    void registerFrame(double nowSeconds);

    // Returns true once after each finished averaging window, then resets.
    bool consumeNewReading();

    double fps() const { return fps_; }
    double frameTimeMs() const { return frameTimeMs_; }

private:
    double windowSeconds_;
    double windowStart_ = -1.0;
    int framesInWindow_ = 0;
    double fps_ = 0.0;
    double frameTimeMs_ = 0.0;
    bool hasNewReading_ = false;
};
