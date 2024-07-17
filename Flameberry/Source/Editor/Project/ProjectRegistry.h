#pragma once

#include "Project/Project.h"

namespace Flameberry {

	struct ProjectRegistryEntry
	{
		std::string ProjectName;
		std::filesystem::path ProjectFilePath;

		const bool operator==(const ProjectRegistryEntry& other)
		{
			// This assumes that ProjectName will always be derived from the project file name so no need to compare `ProjectName`
			return ProjectFilePath == other.ProjectFilePath;
		}

		const bool operator!=(const ProjectRegistryEntry& other)
		{
			// This assumes that ProjectName will always be derived from the project file name so no need to compare `ProjectName`
			return !(*this == other);
		}

		// TODO: Add Timestamps, Project Type etc in the future
	};

	using ProjectRegistry = std::vector<ProjectRegistryEntry>;

	class ProjectRegistryManager
	{
	public:
		static void AppendEntryToGlobalRegistry(Project* project);
		static void RemoveEntryFromGlobalRegistry(const std::filesystem::path& pathOfProjectEntry);
		static ProjectRegistry LoadEntireProjectRegistry();
		static void ClearAllNonExistingProjectRegistryEntries();

	private:
		constexpr static const char* c_GlobalProjectRegistryPath = FBY_PROJECT_DIR "Flameberry/Config/ProjectRegistry.yaml";
	};

} // namespace Flameberry