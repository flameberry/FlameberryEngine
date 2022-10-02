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
        decltype(std::chrono::high_resolution_clock::now()) m_Start;
    };
}