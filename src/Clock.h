#pragma once

#include <chrono>

namespace Clock {

inline double nowSeconds()
{
    using SteadyClock = std::chrono::steady_clock;
    static const SteadyClock::time_point start = SteadyClock::now();
    return std::chrono::duration<double>(SteadyClock::now() - start).count();
}

} // namespace Clock
