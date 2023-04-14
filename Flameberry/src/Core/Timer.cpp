#include "Timer.h"
#include "Core.h"

namespace Flameberry {
    Timer::Timer()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    Timer::~Timer()
    {
    }

    double Timer::GetTimeElapsedMilliSeconds()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }

    double Timer::GetTimeElapsedNanoSeconds()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }

    ScopedTimer::ScopedTimer(double* durationVar)
        : m_DurationVar(durationVar)
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    ScopedTimer::ScopedTimer(const char* scopeName)
        : m_ScopeName(scopeName)
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    ScopedTimer::~ScopedTimer()
    {
        if (m_DurationVar)
            *m_DurationVar = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
        else
            std::cout << "TIMER: " << m_ScopeName << ": Process took " << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001 * 0.001 << " ms" << std::endl;
        // FL_LOG("{0}: Process took {1} ms", m_ScopeName, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001 * 0.001);
    }
}