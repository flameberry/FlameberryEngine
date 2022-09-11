#include "Timer.h"

namespace Flameberry {
    ScopedTimer::ScopedTimer(double* durationVar)
        : m_DurationVar(durationVar)
    {
        m_Start = std::chrono::high_resolution_clock::now();
        m_Started = true;
    }

    ScopedTimer::~ScopedTimer()
    {
        *m_DurationVar = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }
}