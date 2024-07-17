#pragma once

#include "Project/Project.h"

namespace Flameberry {

	class ProjectSerializer
	{
	public:
		static Ref<Project> DeserializeIntoNewProject(const std::filesystem::path& filePath);
		static bool DeserializeIntoExistingProject(const std::filesystem::path& filePath, const Ref<Project>& dest);
		static void SerializeProject(const std::filesystem::path& filePath, const Project* project);
	};

} // namespace Flameberry
