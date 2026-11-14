#pragma once

#include <chrono>

namespace Clock {

// Seconds since the first call. steady_clock never jumps backwards, unlike
// wall-clock time, so differences between two calls are always safe to use
// as a physics time step.
inline double nowSeconds()
{
    using SteadyClock = std::chrono::steady_clock;
    static const SteadyClock::time_point start = SteadyClock::now();
    return std::chrono::duration<double>(SteadyClock::now() - start).count();
}

} // namespace Clock
