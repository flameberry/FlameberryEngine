#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

namespace Flameberry {
    class Profiler
    {
    public:
        Profiler(const std::string& scopeName);
        ~Profiler();

        static double GetExecutionTime(const std::string& scopeName) { return s_ScopeExecutionTimes[scopeName]; }
        static void DisplayScopeDetailsImGui();
    private:
        std::string m_ScopeName;
        decltype(std::chrono::high_resolution_clock::now()) m_Start;

        static std::unordered_map<std::string, double> s_ScopeExecutionTimes;
    };
}

#ifdef FL_DEBUG
#define FL_PROFILE_SCOPE(scopeName) Flameberry::Profiler profiler(scopeName);
#define FL_DISPLAY_SCOPE_DETAILS_IMGUI() Flameberry::Profiler::DisplayScopeDetailsImGui()
#else
#define FL_PROFILE_SCOPE(scopeName)
#define FL_DISPLAY_SCOPE_DETAILS_IMGUI()
#endif
