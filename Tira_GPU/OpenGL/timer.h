#pragma once

#include <chrono>
#include <string>

namespace GL {

struct Timer
{
    Timer();

    auto update() noexcept -> void;
    auto deltaTime() noexcept -> double;
    auto totalTime() noexcept -> double;

    std::chrono::steady_clock::time_point startTimePoint;
    std::chrono::steady_clock::time_point prevTimePoint;
    double _deltaTime = 0.0;
};

inline Timer::Timer() {
    startTimePoint = std::chrono::steady_clock::now();
    prevTimePoint = startTimePoint;
}

inline auto Timer::update() noexcept -> void {
    auto now = std::chrono::steady_clock::now();
    uint64_t deltaTimeCount = std::chrono::duration<double, std::micro>(now - prevTimePoint).count();
    _deltaTime = 0.000001 * deltaTimeCount;
    prevTimePoint = now;
}

inline auto Timer::deltaTime() noexcept -> double {
    return _deltaTime;
}

inline auto Timer::totalTime() noexcept -> double {
    uint64_t totalTimeCount = std::chrono::duration<double, std::micro>(prevTimePoint - startTimePoint).count();
    return 0.000001 * totalTimeCount;
}

}
