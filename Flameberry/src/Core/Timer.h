#pragma once

#include <iostream>
#include <chrono>
#include "Core/Core.h"

namespace Flameberry {
    class Timer
    {
    public:
        Timer() { m_Start = std::chrono::high_resolution_clock::now(); }
        ~Timer() = default;

        float GetTimeEllapsed() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
        float GetTimeEllapsedMilliseconds() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
        float GetTimeEllapsedMicroseconds() const { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
    private:
        decltype(std::chrono::high_resolution_clock::now()) m_Start;
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(const std::string& scopeName) : m_ScopeName(scopeName) { m_Start = std::chrono::high_resolution_clock::now(); }
        ~ScopedTimer() {
#ifdef FBY_DEBUG
            FBY_WARN("{0}: process took: {1} ms", m_ScopeName, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count());
#elif defined(FBY_RELEASE)
            std::cout << m_ScopeName << ": process took: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() << " ms\n";
#endif
        }
    private:
        decltype(std::chrono::high_resolution_clock::now()) m_Start;
        std::string m_ScopeName;
    };
}

#define FBY_SCOPED_TIMER(name) Flameberry::ScopedTimer timer(name)
