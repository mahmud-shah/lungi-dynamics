#include "FpsCounter.h"

FpsCounter::FpsCounter(double windowSeconds)
    : windowSeconds_(windowSeconds)
{
}

void FpsCounter::registerFrame(double nowSeconds)
{
    if (windowStart_ < 0.0) {
        windowStart_ = nowSeconds;
        framesInWindow_ = 0;
        return;
    }

    ++framesInWindow_;
    const double elapsed = nowSeconds - windowStart_;

    if (elapsed >= windowSeconds_) {
        fps_ = framesInWindow_ / elapsed;
        frameTimeMs_ = 1000.0 * elapsed / framesInWindow_;
        framesInWindow_ = 0;
        windowStart_ = nowSeconds;
        hasNewReading_ = true;
    }
}

bool FpsCounter::consumeNewReading()
{
    const bool hadReading = hasNewReading_;
    hasNewReading_ = false;
    return hadReading;
}
