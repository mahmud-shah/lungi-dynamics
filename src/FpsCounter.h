#pragma once

class FpsCounter {
public:
    explicit FpsCounter(double windowSeconds = 0.5);

    void registerFrame(double nowSeconds);
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
