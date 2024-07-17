#include "ProjectRegistry.h"

#include <fstream>

#include "Core/Core.h"
#include "Core/YamlUtils.h"

namespace Flameberry {

	void ProjectRegistryManager::AppendEntryToGlobalRegistry(Project* project)
	{
		YAML::Emitter out;
		const auto& filepath = fmt::format("{}/{}.fbproj", project->GetProjectDirectory(), project->GetConfig().Name);
		out << YAML::BeginMap;
		{
			out << YAML::Key << filepath << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "ProjectName" << YAML::Value << project->GetConfig().Name;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(c_GlobalProjectRegistryPath, std::ios_base::app);
		FBY_ASSERT(fout.is_open(), "Failed to find/open project registry: {}", c_GlobalProjectRegistryPath);
		fout << out.c_str() << '\n';
	}

	void ProjectRegistryManager::RemoveEntryFromGlobalRegistry(const std::filesystem::path& pathOfProjectEntry)
	{
	}

	ProjectRegistry ProjectRegistryManager::LoadEntireProjectRegistry()
	{
		std::ifstream in(c_GlobalProjectRegistryPath);
		std::stringstream ss;
		ss << in.rdbuf();

		YAML::Node data = YAML::Load(ss.str());

		ProjectRegistry registry;
		for (const auto& entry : data)
		{
			ProjectRegistryEntry projectEntry;
			projectEntry.ProjectFilePath = entry.first.as<std::string>();
			projectEntry.ProjectName = entry.second["ProjectName"].as<std::string>();
			registry.emplace_back(projectEntry);
		}
		return registry;
	}

	void ProjectRegistryManager::ClearAllNonExistingProjectRegistryEntries()
	{
		std::stringstream ss;
		{
			std::ifstream fin(c_GlobalProjectRegistryPath);
			ss << fin.rdbuf();
		}

		YAML::Node data = YAML::Load(ss.str());

		for (const auto& entry : data)
		{
			const auto& path = entry.first.as<std::string>();
			if (!std::filesystem::exists(path))
				data.remove(entry.first.as<std::string>());
		}

		std::ofstream fout(c_GlobalProjectRegistryPath, std::ios_base::out | std::ios::trunc);
		if (data.size())
		{
			fout << data << '\n';
		}
	}

} // namespace Flameberry