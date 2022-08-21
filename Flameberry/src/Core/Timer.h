#pragma once

#include <chrono>

namespace Flameberry {
    class ScopedTimer
    {
    public:
        ScopedTimer(float* durationVar);
        ~ScopedTimer();
    private:
        float* m_DurationVar;
        bool m_Started = false;
        std::__1::chrono::steady_clock::time_point m_Start;
    };
}