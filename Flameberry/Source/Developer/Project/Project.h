#pragma once

#include <filesystem>

#include "Core/Core.h"
#include "Asset/IAssetManager.h"
#include "Project/ThumbnailCache.h"

namespace Flameberry {

	struct ProjectConfig
	{
		std::filesystem::path AssetDirectory /* = "Content" */;
		AssetHandle StartScene = 0;

		std::filesystem::path AssetRegistryPath;
		std::filesystem::path ThumbnailCacheDirectory;
		std::string Name = "FlameberryProject";
	};

	class Project
	{
	public:
		static Ref<Project> Load(const std::filesystem::path& path);

		static inline const std::filesystem::path& GetActiveProjectDirectory() { return s_ActiveProject->GetProjectDirectory(); }
		static Ref<Project> GetActiveProject() { return s_ActiveProject; }
		static void SetActive(const Ref<Project>& project) { s_ActiveProject = project; }

		// Standard procedure for creating a project on disk
		static Ref<Project> CreateProjectOnDisk(const std::filesystem::path& targetFolder, const std::string& projectName);

		// Instance specific methods
		Project(const std::filesystem::path& projectDirectory, const ProjectConfig& config);
		Project();
		~Project();

		void Save();

		Ref<IAssetManager> GetAssetManager() { return m_AssetManager; }
		Ref<ThumbnailCache> GetThumbnailCache() { return m_ThumbnailCache; }

		const std::filesystem::path& GetProjectDirectory() const { return m_ProjectDirectory; }
		std::filesystem::path GetAssetDirectory() const { return m_ProjectDirectory / m_Config.AssetDirectory; }
		std::filesystem::path GetAssetRegistryPath() const { return m_ProjectDirectory / m_Config.AssetRegistryPath; }
		std::filesystem::path GetThumbnailCacheDirectory() const { return m_ProjectDirectory / m_Config.ThumbnailCacheDirectory; }

		ProjectConfig& GetConfig() { return m_Config; }

	private:
		void Init();

	private:
		std::filesystem::path m_ProjectDirectory;
		ProjectConfig m_Config;

		Ref<IAssetManager> m_AssetManager;
		Ref<ThumbnailCache> m_ThumbnailCache;

	private:
		static Ref<Project> s_ActiveProject;

		friend class ProjectSerializer;
	};

} // namespace Flameberry
