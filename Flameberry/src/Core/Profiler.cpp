#include "Profiler.h"

#include <imgui/imgui.h>

namespace Flameberry {
	std::unordered_map<std::string, double> Profiler::s_ScopeExecutionTimes;

	Profiler::Profiler(const std::string& scopeName)
		: m_ScopeName(scopeName)
	{
		m_Start = std::chrono::high_resolution_clock::now();
	}

	Profiler::~Profiler()
	{
		double executionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
		s_ScopeExecutionTimes[m_ScopeName] = executionTime;
	}

	void Profiler::DisplayScopeDetailsImGui()
	{
		for (auto& [scope, time] : s_ScopeExecutionTimes)
			ImGui::Text("%s: %.4f ms", scope.c_str(), time * 0.001 * 0.001);
	}
} // namespace Flameberry
