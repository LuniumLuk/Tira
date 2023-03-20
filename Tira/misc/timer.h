//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

namespace tira {

    struct Timer {
        Timer() {
            reset();
        }

        void update() {
            auto now = std::chrono::steady_clock::now();
            auto count = std::chrono::duration<double, std::micro>(now - prev).count();
            delta = 1e-6 * count;
            prev = now;
        }

        void reset() {
            prev = start = std::chrono::steady_clock::now();
        }

        double delta_time() {
            return delta;
        }

        double total_time() {
            auto count = std::chrono::duration<double, std::micro>(prev - start).count();
            return 1e-6 * count;
        }

    private:
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point prev;
        double delta = 0.0;
    };

} // namespace tira

#endif
