#pragma once

#include <filesystem>

#include "Core/Core.h"

namespace Flameberry {
	struct ProjectConfig
	{
		std::filesystem::path AssetDirectory /* = "Content" */, ScriptAssemblyPath;
		std::filesystem::path StartScene;

		std::string Name = "Flameberry-Project";
	};

	class Project
	{
	public:
		// Static Utilities
		static const std::filesystem::path& GetActiveProjectDirectory()
		{
			return s_ActiveProject->GetProjectDirectory();
		}

		// Standard procedure for creating a project on disk
		static Ref<Project>
		CreateProjectOnDisk(const std::filesystem::path& targetFolder,
			const std::string& projectName);

		// Instance specific methods
		Project(const std::filesystem::path& projectDirectory,
			const ProjectConfig& config);
		Project() = default;
		~Project();

		void Save();

		const std::filesystem::path& GetProjectDirectory() const
		{
			return m_ProjectDirectory;
		}
		std::filesystem::path GetAssetDirectory() const
		{
			return m_ProjectDirectory / m_Config.AssetDirectory;
		}
		std::filesystem::path GetStartScenePath() const
		{
			return m_ProjectDirectory / m_Config.StartScene;
		}
		std::filesystem::path GetScriptAssemblyPath() const
		{
			return m_ProjectDirectory / m_Config.ScriptAssemblyPath;
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActiveProject() { return s_ActiveProject; }
		static void SetActive(const Ref<Project>& project)
		{
			s_ActiveProject = project;
		}

	private:
		std::filesystem::path m_ProjectDirectory;
		ProjectConfig m_Config;

		inline static Ref<Project> s_ActiveProject;

		friend class ProjectSerializer;
	};

	class ProjectSerializer
	{
	public:
		static Ref<Project>
		DeserializeIntoNewProject(const std::filesystem::path& filePath);
		static bool
		DeserializeIntoExistingProject(const std::filesystem::path& filePath,
			const Ref<Project>& dest);
		static void SerializeProject(const std::filesystem::path& filePath,
			const Project* project);
	};

	struct ProjectRegistryEntry
	{
		std::string ProjectName;
		std::filesystem::path ProjectFilePath;

		const bool operator==(const ProjectRegistryEntry& other)
		{
			// This assumes that ProjectName will always be derived from the project
			// file name so no need to compare `ProjectName`
			return ProjectFilePath == other.ProjectFilePath;
		}

		const bool operator!=(const ProjectRegistryEntry& other)
		{
			// This assumes that ProjectName will always be derived from the project
			// file name so no need to compare `ProjectName`
			return !(*this == other);
		}

		// TODO: Add Timestamps, Project Type etc in the future
	};

	typedef std::vector<ProjectRegistryEntry> ProjectRegistry;

	class ProjectRegistryManager
	{
	public:
		static void AppendEntryToGlobalRegistry(Project* project);
		static void RemoveEntryFromGlobalRegistry(
			const std::filesystem::path& pathOfProjectEntry);
		static ProjectRegistry LoadEntireProjectRegistry();
		static void ClearAllNonExistingProjectRegistryEntries();

	private:
		constexpr static const char* c_GlobalProjectRegistryPath =
			FBY_PROJECT_DIR "FlameberryEditor/GlobalConfig/ProjectRegistry.fbreg";
	};
} // namespace Flameberry
