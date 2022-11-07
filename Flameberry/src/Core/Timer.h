#pragma once

#include <chrono>

namespace Flameberry {
    class Timer
    {
    public:
        Timer();
        ~Timer();

        double GetTimeElapsedMilliSeconds();
        double GetTimeElapsedNanoSeconds();
    private:
        decltype(std::chrono::high_resolution_clock::now()) m_Start;
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(double* durationVar);
        ScopedTimer(const char* scopeName);
        ~ScopedTimer();
    private:
        // Stores time in nanoseconds
        double* m_DurationVar = nullptr;
        const char* m_ScopeName = "default";
        decltype(std::chrono::high_resolution_clock::now()) m_Start;
    };

#define FL_SCOPED_TIMER(scopeName) ScopedTimer scopedTimer(scopeName)
}
