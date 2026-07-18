#ifndef TIME_HPP
#define TIME_HPP

#include "util/common.hpp"

struct TimeInfo {
    s64 time = 0;  // miliseconds
    s64 deltaTime = 0;
    double timeSeconds = 0;
    double deltaTimeSeconds = 0;
};

#define NANOSECONDS_PER_SECOND  1'000'000'000
#define MICROSECONDS_PER_SECOND 1'000'000
#define MILLISECONDS_PER_SECOND 1'000


#endif // TIME_HPP