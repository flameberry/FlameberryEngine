#pragma once

#include <chrono>

namespace Flameberry {
    class ScopedTimer
    {
    public:
        ScopedTimer(double* durationVar);
        ~ScopedTimer();
    private:
        // Stores time in nanoseconds
        double* m_DurationVar;
        bool m_Started = false;
        std::__1::chrono::steady_clock::time_point m_Start;
    };
}